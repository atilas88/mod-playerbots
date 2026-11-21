/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "RaidRecruitMgr.h"

#include <algorithm>
#include <cstdlib>
#include <map>

#include "AddonGearScore.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "ChatHelper.h"
#include "GameTime.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotSecurity.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "Random.h"
#include "RandomPlayerbotMgr.h"

namespace
{
constexpr uint8 RAID_BOT_LEVEL = 80;
constexpr int32 MAX_LEVEL_DIFFERENCE_WITHOUT_RAID = 5;
constexpr time_t COOLDOWN_PURGE_INTERVAL = 5 * MINUTE;
constexpr uint32 PERCENT_ROLL_MAX = 100;

uint32 GetGearScoreThreshold(RaidAnnounce const& announce)
{
    if (announce.minGearScore)
        return announce.minGearScore;

    auto const itr = sPlayerbotAIConfig.RaidRecruitReplyMinGearScores.find(announce.instance);
    if (itr == sPlayerbotAIConfig.RaidRecruitReplyMinGearScores.end())
        return 0;

    uint32 threshold = itr->second;
    if (announce.size == 25)
        threshold += sPlayerbotAIConfig.RaidRecruitReplySize25GearScoreBonus;

    if (announce.heroic)
        threshold += sPlayerbotAIConfig.RaidRecruitReplyHeroicGearScoreBonus;

    return threshold;
}

bool HasRole(Player* bot, RecruitRole role)
{
    switch (role)
    {
        case RecruitRole::Tank:
            return PlayerbotAI::IsTank(bot);
        case RecruitRole::Heal:
            return PlayerbotAI::IsHeal(bot);
        case RecruitRole::Dps:
            // same split as the reply text: a bot is offered as dps only when it is neither of the other two
            return !PlayerbotAI::IsTank(bot) && !PlayerbotAI::IsHeal(bot);
    }

    return false;
}

// Index of the first slot the bot fills, or the slot count when it fills none
size_t FindSlot(Player* bot, std::vector<RecruitSlot> const& slots)
{
    for (size_t i = 0; i < slots.size(); ++i)
    {
        RecruitSlot const& slot = slots[i];
        if (slot.classId && slot.classId != bot->getClass())
            continue;

        if (!slot.role || HasRole(bot, *slot.role))
            return i;
    }

    return slots.size();
}

// Truncated, never rounded up: a bot must not claim more than it has
std::string FormatGearScore(uint32 gearScore)
{
    return std::to_string(gearScore / 1000) + "." + std::to_string(gearScore % 1000 / 100) + "k";
}

std::string BuildReplyText(Player* bot, uint32 gearScore, bool classRequested)
{
    std::map<std::string, std::string> placeholders;
    placeholders["%gs"] = FormatGearScore(gearScore);
    placeholders["%class"] = ChatHelper::FormatClass(bot->getClass());

    std::string name = classRequested ? "raid_recruit_reply_class_" : "raid_recruit_reply_";
    std::string fallback;
    if (PlayerbotAI::IsTank(bot))
    {
        name += "tank";
        fallback = classRequested ? "%class tank here, %gs gs, inv" : "tank here, %gs gs, inv";
    }
    else if (PlayerbotAI::IsHeal(bot))
    {
        name += "heal";
        fallback = classRequested ? "%class healer here, %gs gs, inv" : "healer here, %gs gs, inv";
    }
    else
    {
        name += "dps";
        fallback = classRequested ? "%class dps here, %gs gs, inv" : "dps here, %gs gs, inv";
    }

    return PlayerbotTextMgr::instance().GetBotTextOrDefault(name, fallback, placeholders);
}
}  // namespace

void RaidRecruitMgr::OnChannelMessage(Player* player, std::string const& message, Channel* channel)
{
    if (!sPlayerbotAIConfig.RaidRecruitReplyEnabled || !player || !channel)
        return;

    uint32 const channelId = channel->GetChannelId();
    if (channelId != ChatChannelId::GENERAL && channelId != ChatChannelId::LOOKING_FOR_GROUP)
        return;

    if (!IsRealPlayer(player))
        return;

    time_t const now = GameTime::GetGameTime().count();
    auto const cooldown = _cooldownEnd.find(player->GetGUID());
    if (cooldown != _cooldownEnd.end() && cooldown->second > now)
        return;

    std::optional<RaidAnnounce> announce = ParseRaidAnnounce(message);
    if (!announce)
        return;

    if (urand(1, PERCENT_ROLL_MAX) > sPlayerbotAIConfig.RaidRecruitReplyChance)
        return;

    _cooldownEnd[player->GetGUID()] = now + sPlayerbotAIConfig.RaidRecruitReplyPlayerCooldown;
    _requests.push_back({player->GetGUID(), std::move(*announce), channel->GetName()});
}

void RaidRecruitMgr::Update()
{
    time_t const now = GameTime::GetGameTime().count();
    if (_requests.empty() && _replies.empty() && now < _nextCooldownPurge)
        return;

    uint64 const nowMs = GameTime::GetGameTimeMS().count();
    for (Request const& request : _requests)
        SelectCandidates(request, nowMs);

    _requests.clear();

    std::erase_if(_replies, [this, nowMs](Reply const& reply)
    {
        if (reply.dueTimeMs > nowMs)
            return false;

        SendReply(reply);
        return true;
    });

    if (now >= _nextCooldownPurge)
        PurgeCooldowns(now);
}

void RaidRecruitMgr::PurgeCooldowns(time_t now)
{
    std::erase_if(_cooldownEnd, [now](auto const& entry) { return entry.second <= now; });
    _nextCooldownPurge = now + COOLDOWN_PURGE_INTERVAL;
}

bool RaidRecruitMgr::HasPendingReply(ObjectGuid botGuid) const
{
    return std::any_of(_replies.begin(), _replies.end(),
                       [botGuid](Reply const& reply) { return reply.botGuid == botGuid; });
}

// The state that can change between picking a bot and its delayed reply
bool RaidRecruitMgr::IsAvailable(Player* bot, Player* player) const
{
    if (!bot->IsInWorld() || !bot->IsAlive() || bot->GetTeamId() != player->GetTeamId())
        return false;

    if (bot->GetGroup() || bot->GetGroupInvite() || bot->IsInCombat())
        return false;

    if (bot->InBattleground() || bot->InBattlegroundQueue() || bot->IsBeingTeleported() || bot->IsInFlight())
        return false;

    Map const* map = bot->FindMap();
    return map && !map->Instanceable();
}

void RaidRecruitMgr::SelectCandidates(Request const& request, uint64 nowMs)
{
    Player* player = ObjectAccessor::FindPlayer(request.playerGuid);
    uint32 const maxReplies = sPlayerbotAIConfig.RaidRecruitReplyMaxReplies;
    if (!player || !maxReplies)
        return;

    RaidAnnounce const& announce = request.announce;
    uint32 const threshold = GetGearScoreThreshold(announce);
    bool const raidNamed = !announce.instance.empty();
    int32 const playerLevel = static_cast<int32>(player->GetLevel());

    // one reservoir sample of up to maxReplies bots per slot, filled in a single pass over the bots
    std::vector<std::vector<Candidate>> reservoirs(announce.slots.size());
    std::vector<uint32> matches(announce.slots.size(), 0);

    for (auto itr = sRandomPlayerbotMgr.GetPlayerBotsBegin(); itr != sRandomPlayerbotMgr.GetPlayerBotsEnd(); ++itr)
    {
        Player* bot = itr->second;
        if (!bot || !IsAvailable(bot, player))
            continue;

        int32 const botLevel = static_cast<int32>(bot->GetLevel());
        bool const levelFits = raidNamed ? botLevel >= RAID_BOT_LEVEL
                                         : std::abs(botLevel - playerLevel) <= MAX_LEVEL_DIFFERENCE_WITHOUT_RAID;
        if (!levelFits)
            continue;

        size_t const slotIndex = FindSlot(bot, announce.slots);
        if (slotIndex == announce.slots.size())
            continue;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (!botAI || IsRealPlayer(botAI->GetMaster()) || !sRandomPlayerbotMgr.IsRandomBot(bot))
            continue;

        if (HasPendingReply(bot->GetGUID()))
            continue;

        uint32 const gearScore = AddonGearScore::Calculate(bot);
        if (gearScore < threshold)
            continue;

        // only offer bots that will accept the invite the player sends next
        if (!botAI->GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_INVITE, true, player))
            continue;

        std::vector<Candidate>& reservoir = reservoirs[slotIndex];
        uint32 const seen = ++matches[slotIndex];
        if (reservoir.size() < maxReplies)
        {
            reservoir.push_back({bot->GetGUID(), gearScore});
            continue;
        }

        uint32 const replaced = urand(0, seen - 1);
        if (replaced < maxReplies)
            reservoir[replaced] = {bot->GetGUID(), gearScore};
    }

    // not always the maximum, but one bot per slot while the quota lasts
    uint32 const slotCount = static_cast<uint32>(announce.slots.size());
    uint32 remaining = urand(std::min(slotCount, maxReplies), maxReplies);
    for (uint32 round = 0; remaining && round < maxReplies; ++round)
    {
        for (size_t slotIndex = 0; remaining && slotIndex < reservoirs.size(); ++slotIndex)
        {
            if (round >= reservoirs[slotIndex].size())
                continue;

            Candidate const& candidate = reservoirs[slotIndex][round];
            Reply reply;
            reply.botGuid = candidate.botGuid;
            reply.playerGuid = request.playerGuid;
            reply.dueTimeMs = nowMs + urand(sPlayerbotAIConfig.RaidRecruitReplyMinDelay * IN_MILLISECONDS,
                                            sPlayerbotAIConfig.RaidRecruitReplyMaxDelay * IN_MILLISECONDS);
            reply.gearScore = candidate.gearScore;
            reply.classRequested = announce.slots[slotIndex].classId != 0;
            reply.channelName = request.channelName;
            _replies.push_back(std::move(reply));
            --remaining;
        }
    }
}

void RaidRecruitMgr::SendReply(Reply const& reply) const
{
    Player* bot = ObjectAccessor::FindPlayer(reply.botGuid);
    Player* player = ObjectAccessor::FindPlayer(reply.playerGuid);
    if (!bot || !player || !IsAvailable(bot, player))
        return;

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
        return;

    std::string const text = BuildReplyText(bot, reply.gearScore, reply.classRequested);

    // General is per zone: a bot elsewhere is not on the player's channel and whispers instead.
    // Channel membership is not public, so the zone stands in for it.
    if (sPlayerbotAIConfig.RaidRecruitReplyInChannel)
    {
        ChannelMgr* channelMgr = ChannelMgr::forTeam(bot->GetTeamId());
        Channel* channel = channelMgr ? channelMgr->GetChannel(reply.channelName, bot, false) : nullptr;
        if (channel && (channel->IsLFG() || bot->GetZoneId() == player->GetZoneId()))
        {
            channel->Say(bot->GetGUID(), text, LANG_UNIVERSAL);
            return;
        }
    }

    botAI->Whisper(text, player->GetName());
}
