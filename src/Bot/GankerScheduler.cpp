/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "GankerScheduler.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>

#include "DBCStores.h"
#include "Group.h"
#include "GroupMgr.h"
#include "Map.h"
#include "MapMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Playerbots.h"
#include "RandomPlayerbotMgr.h"

void GankerScheduler::Tick()
{
    if (!sPlayerbotAIConfig.enabled || !sPlayerbotAIConfig.gankerEnabled)
        return;

    time_t now = time(nullptr);
    if (lastTickAt && (uint32)(now - lastTickAt) < sPlayerbotAIConfig.gankerSchedulerIntervalSeconds)
        return;
    lastTickAt = now;

    if (activeGankers.size() >= sPlayerbotAIConfig.gankerMaxConcurrentGankers)
    {
        LOG_DEBUG("playerbots",
                  "Ganker tick: skipped (activeGankers={}/{})",
                  (uint32)activeGankers.size(), sPlayerbotAIConfig.gankerMaxConcurrentGankers);
        return;
    }

    std::vector<Player*> victims = sRandomPlayerbotMgr.GetPlayers();
    if (victims.empty())
        return;

    // Shuffle for fairness when multiple victims qualify in the same tick.
    std::shuffle(victims.begin(), victims.end(),
                 std::mt19937(static_cast<uint32_t>(now)));

    uint32 dispatchedVictims = 0;
    for (Player* victim : victims)
    {
        if (!victim)
            continue;
        if (sRandomPlayerbotMgr.IsRandomBot(victim))
            continue;
        if (dispatchedVictims >= sPlayerbotAIConfig.gankerMaxConcurrentVictims)
            break;
        if (activeGankers.size() >= sPlayerbotAIConfig.gankerMaxConcurrentGankers)
            break;

        std::string reason;
        if (!IsEligibleVictim(victim, &reason))
        {
            LOG_DEBUG("playerbots",
                      "Ganker: victim {} <{}> not eligible ({})",
                      victim->GetGUID().ToString().c_str(), victim->GetName().c_str(),
                      reason.c_str());
            continue;
        }

        ObjectGuid vguid = victim->GetGUID();
        auto cdIt = nextEligibleAt.find(vguid);
        if (cdIt != nextEligibleAt.end() && cdIt->second > now)
            continue;

        if (urand(1, 100) > sPlayerbotAIConfig.gankerSpawnChancePercent)
        {
            // Apply a short cooldown so we don't reroll every tick.
            nextEligibleAt[vguid] = now + sPlayerbotAIConfig.gankerSchedulerIntervalSeconds;
            LOG_DEBUG("playerbots",
                      "Ganker: victim {} <{}> roll skipped this tick ({}% spawn chance, retry in {}s)",
                      victim->GetGUID().ToString().c_str(), victim->GetName().c_str(),
                      sPlayerbotAIConfig.gankerSpawnChancePercent,
                      sPlayerbotAIConfig.gankerSchedulerIntervalSeconds);
            continue;
        }

        uint32 wantCount = (urand(1, 100) <= sPlayerbotAIConfig.gankerPairProbabilityPercent) ? 2 : 1;
        uint32 poolSize = 0;
        std::vector<Player*> candidates = SelectGankers(victim, wantCount, &poolSize);
        if (candidates.empty())
        {
            // Retry soon rather than locking the victim for MinSecondsBetweenAttempts:
            // a candidate may come online or level up at any moment.
            nextEligibleAt[vguid] = now + sPlayerbotAIConfig.gankerSchedulerIntervalSeconds;
            LOG_INFO("playerbots",
                     "Ganker: no candidates for victim {} <{}> (lvl {}, team {}); "
                     "random-bot pool in level range had {} bot(s) opposite-faction. "
                     "Tip: ensure random bots exist between lvl {} and {} on the "
                     "opposite faction (online, alive, not in BG/dungeon, not in combat, "
                     "not already in a group).",
                     victim->GetGUID().ToString().c_str(), victim->GetName().c_str(),
                     victim->GetLevel(), (uint32)victim->GetTeamId(), poolSize,
                     std::max<int32>(1, (int32)victim->GetLevel() + sPlayerbotAIConfig.gankerLevelOffsetMin),
                     (int32)victim->GetLevel() + sPlayerbotAIConfig.gankerLevelOffsetMax);
            continue;
        }

        if (!DispatchGank(victim, candidates))
        {
            nextEligibleAt[vguid] = now + sPlayerbotAIConfig.gankerSchedulerIntervalSeconds;
            LOG_INFO("playerbots",
                     "Ganker: DispatchGank failed for victim {} <{}> "
                     "(terrain validation likely failed); retry in {}s",
                     victim->GetGUID().ToString().c_str(), victim->GetName().c_str(),
                     sPlayerbotAIConfig.gankerSchedulerIntervalSeconds);
            continue;
        }

        nextEligibleAt[vguid] = now + urand(sPlayerbotAIConfig.gankerMinSecondsBetweenAttempts,
                                            sPlayerbotAIConfig.gankerMaxSecondsBetweenAttempts);
        ++dispatchedVictims;
    }
}

bool GankerScheduler::IsEligibleVictim(Player* victim, std::string* reason) const
{
    auto fail = [reason](char const* r) -> bool
    {
        if (reason) *reason = r;
        return false;
    };

    if (!victim || !victim->IsInWorld())
        return fail("not in world");
    if (!victim->IsAlive())
        return fail("dead");

    if (victim->InBattleground())
        return fail("in battleground");
    if (victim->GetMap() && victim->GetMap()->IsDungeon())
        return fail("in dungeon");

    if (!victim->IsPvP())
        return fail("not PvP flagged");

    uint32 zoneId = victim->GetZoneId();
    uint32 areaId = victim->GetAreaId();

    if (sPlayerbotAIConfig.IsPvpProhibited(zoneId, areaId))
        return fail("sanctuary / PvP prohibited zone");

    if (!sPlayerbotAIConfig.IsContestedZone(zoneId))
        return fail("not a contested zone");

    return true;
}

uint32 GankerScheduler::ClassWeight(uint8 cls) const
{
    switch (cls)
    {
        case CLASS_WARRIOR:      return sPlayerbotAIConfig.gankerWeightWarrior;
        case CLASS_PALADIN:      return sPlayerbotAIConfig.gankerWeightPaladin;
        case CLASS_HUNTER:       return sPlayerbotAIConfig.gankerWeightHunter;
        case CLASS_ROGUE:        return sPlayerbotAIConfig.gankerWeightRogue;
        case CLASS_PRIEST:       return sPlayerbotAIConfig.gankerWeightPriest;
        case CLASS_DEATH_KNIGHT: return sPlayerbotAIConfig.gankerWeightDeathKnight;
        case CLASS_SHAMAN:       return sPlayerbotAIConfig.gankerWeightShaman;
        case CLASS_MAGE:         return sPlayerbotAIConfig.gankerWeightMage;
        case CLASS_WARLOCK:      return sPlayerbotAIConfig.gankerWeightWarlock;
        case CLASS_DRUID:        return sPlayerbotAIConfig.gankerWeightDruid;
        default:                 return 0;
    }
}

std::vector<Player*> GankerScheduler::SelectGankers(Player* victim, uint32 count, uint32* outPoolSize) const
{
    std::vector<Player*> result;
    if (outPoolSize)
        *outPoolSize = 0;
    if (!victim || !count)
        return result;

    int32 victimLvl = static_cast<int32>(victim->GetLevel());
    int32 minLvl = std::max<int32>(1, victimLvl + sPlayerbotAIConfig.gankerLevelOffsetMin);
    int32 maxLvl = victimLvl + sPlayerbotAIConfig.gankerLevelOffsetMax;
    TeamId victimTeam = victim->GetTeamId();

    struct Candidate
    {
        Player* bot;
        uint32 weight;
    };
    std::vector<Candidate> pool;
    pool.reserve(64);

    for (auto it = sRandomPlayerbotMgr.GetPlayerBotsBegin();
         it != sRandomPlayerbotMgr.GetPlayerBotsEnd(); ++it)
    {
        Player* bot = it->second;
        if (!bot || !bot->IsInWorld())
            continue;
        if (!sRandomPlayerbotMgr.IsRandomBot(bot))
            continue;
        if (activeGankers.find(bot->GetGUID()) != activeGankers.end())
            continue;
        if (!bot->IsAlive())
            continue;
        if (bot->InBattleground() || (bot->GetMap() && bot->GetMap()->IsDungeon()))
            continue;
        if (bot->IsInCombat())
            continue;
        if (bot->GetGroup())
            continue;
        if (bot->GetTeamId() == victimTeam)
            continue;
        int32 lvl = static_cast<int32>(bot->GetLevel());
        if (lvl < minLvl || lvl > maxLvl)
            continue;

        uint32 w = ClassWeight(bot->getClass());
        if (!w)
            continue;

        pool.push_back({bot, w});
    }

    if (outPoolSize)
        *outPoolSize = (uint32)pool.size();

    if (pool.empty())
        return result;

    for (uint32 i = 0; i < count && !pool.empty(); ++i)
    {
        uint32 totalWeight = 0;
        for (auto const& c : pool)
            totalWeight += c.weight;
        if (!totalWeight)
            break;

        uint32 roll = urand(1, totalWeight);
        uint32 acc = 0;
        size_t pickedIdx = 0;
        for (size_t j = 0; j < pool.size(); ++j)
        {
            acc += pool[j].weight;
            if (roll <= acc)
            {
                pickedIdx = j;
                break;
            }
        }
        result.push_back(pool[pickedIdx].bot);
        pool.erase(pool.begin() + pickedIdx);
    }

    return result;
}

bool GankerScheduler::ComputeTeleportPoint(Player* victim, float angleOffsetRad,
                                           float& outX, float& outY, float& outZ) const
{
    if (!victim || !victim->GetMap())
        return false;

    float radius = sPlayerbotAIConfig.gankerEngagementRadiusYards;
    float facing = victim->GetOrientation();
    // Place the bot behind the victim (facing + PI), optionally offset by
    // angleOffsetRad to spread pair members.
    float angle = facing + static_cast<float>(M_PI) + angleOffsetRad;
    float x = victim->GetPositionX() + std::cos(angle) * radius;
    float y = victim->GetPositionY() + std::sin(angle) * radius;
    float z = victim->GetPositionZ();

    Map* map = victim->GetMap();
    if (map->IsInWater(victim->GetPhaseMask(), x, y, z, 2.0f))
        return false;

    float ground = map->GetHeight(victim->GetPhaseMask(), x, y, z + 5.0f);
    if (ground <= INVALID_HEIGHT)
        return false;

    outX = x;
    outY = y;
    outZ = ground + 0.5f;
    return true;
}

bool GankerScheduler::DispatchGank(Player* victim, std::vector<Player*> const& gankers)
{
    if (!victim || gankers.empty())
        return false;

    // Filter null/invalid bots up front so the pair logic only ever sees live ones.
    std::vector<Player*> live;
    live.reserve(gankers.size());
    for (Player* bot : gankers)
    {
        if (bot && GET_PLAYERBOT_AI(bot))
            live.push_back(bot);
    }
    if (live.empty())
        return false;

    bool isPair = live.size() >= 2;

    // Compute teleport points: solo at angle 0, pair at ±10° (M_PI / 18).
    float baseX = 0.f, baseY = 0.f, baseZ = 0.f;
    if (!ComputeTeleportPoint(victim, 0.0f, baseX, baseY, baseZ))
        return false;

    float pairOffset = static_cast<float>(M_PI) / 18.0f;
    float p0x = baseX, p0y = baseY, p0z = baseZ;
    float p1x = baseX, p1y = baseY, p1z = baseZ;
    if (isPair)
    {
        float ax = 0.f, ay = 0.f, az = 0.f;
        float bx = 0.f, by = 0.f, bz = 0.f;
        if (ComputeTeleportPoint(victim, -pairOffset, ax, ay, az))
        {
            p0x = ax; p0y = ay; p0z = az;
        }
        if (ComputeTeleportPoint(victim, +pairOffset, bx, by, bz))
        {
            p1x = bx; p1y = by; p1z = bz;
        }
    }

    LOG_INFO("playerbots",
             "Ganker scheduler: dispatching {} bot(s) on victim {} (lvl {}) in zone {}",
             (uint32)live.size(), victim->GetName().c_str(), victim->GetLevel(), victim->GetZoneId());

    bool dispatchedAny = false;
    for (size_t i = 0; i < live.size(); ++i)
    {
        Player* bot = live[i];
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);

        float tx = (i == 0) ? p0x : p1x;
        float ty = (i == 0) ? p0y : p1y;
        float tz = (i == 0) ? p0z : p1z;

        bot->GetMotionMaster()->Clear();
        bot->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED |
                                           AURA_INTERRUPT_FLAG_CHANGE_MAP);
        bot->TeleportTo(victim->GetMapId(), tx, ty, tz, victim->GetOrientation());

        AiObjectContext* context = botAI->GetAiObjectContext();
        context->GetValue<ObjectGuid>("ganker target guid")->Set(victim->GetGUID());
        context->GetValue<bool>("ganker is temp group")->Set(false);
        context->GetValue<uint32>("ganker camp until")->Set(0);
        context->GetValue<uint32>("ganker next emote at")->Set(0);
        context->GetValue<uint32>("ganker death count")->Set(0);
        context->GetValue<uint32>("ganker revive at")->Set(0);

        botAI->ChangeStrategy("-new rpg", BOT_STATE_NON_COMBAT);
        botAI->ChangeStrategy("+ganker", BOT_STATE_NON_COMBAT);
        botAI->ChangeStrategy("+ganker dead", BOT_STATE_DEAD);

        activeGankers.insert(bot->GetGUID());
        ganker2victim[bot->GetGUID()] = victim->GetGUID();
        dispatchedAny = true;

        LOG_INFO("playerbots",
                 "Ganker bot {} <{}> teleporting to victim {} <{}> at {:.1f},{:.1f},{:.1f}",
                 bot->GetGUID().ToString().c_str(), bot->GetName().c_str(),
                 victim->GetGUID().ToString().c_str(), victim->GetName().c_str(),
                 tx, ty, tz);
    }

    if (isPair && dispatchedAny)
    {
        Player* leader = live[0];
        Player* member = live[1];
        Group* group = new Group();
        if (!group->Create(leader))
        {
            delete group;
            LOG_INFO("playerbots",
                     "Ganker pair: failed to create temp group for {} + {}, continuing as solos",
                     leader->GetName().c_str(), member->GetName().c_str());
        }
        else
        {
            sGroupMgr->AddGroup(group);
            if (!group->AddMember(member))
            {
                LOG_INFO("playerbots",
                         "Ganker pair: AddMember failed for {}, leader continues solo",
                         member->GetName().c_str());
                // Leader remains in a 1-man group; treat as solo flag to avoid stuck disband.
                group->Disband();
            }
            else
            {
                if (PlayerbotAI* la = GET_PLAYERBOT_AI(leader))
                    la->GetAiObjectContext()->GetValue<bool>("ganker is temp group")->Set(true);
                if (PlayerbotAI* ma = GET_PLAYERBOT_AI(member))
                    ma->GetAiObjectContext()->GetValue<bool>("ganker is temp group")->Set(true);

                LOG_INFO("playerbots",
                         "Ganker pair formed: {} + {} on victim {}",
                         leader->GetName().c_str(), member->GetName().c_str(),
                         victim->GetName().c_str());
            }
        }
    }

    return dispatchedAny;
}

void GankerScheduler::OnGankerReleased(ObjectGuid bot, bool retreat)
{
    auto it = ganker2victim.find(bot);
    if (retreat && it != ganker2victim.end())
    {
        ObjectGuid vguid = it->second;
        time_t now = time(nullptr);
        float mult = sPlayerbotAIConfig.gankerRetreatCooldownMultiplier;
        if (mult < 1.0f)
            mult = 1.0f;
        uint32 cooldown =
            (uint32)(sPlayerbotAIConfig.gankerMaxSecondsBetweenAttempts * mult);
        time_t extendedUntil = now + cooldown;
        auto cdIt = nextEligibleAt.find(vguid);
        if (cdIt == nextEligibleAt.end() || cdIt->second < extendedUntil)
            nextEligibleAt[vguid] = extendedUntil;

        LOG_INFO("playerbots",
                 "Ganker bot {} retreating from victim {} (extended cooldown {}s)",
                 bot.ToString().c_str(), vguid.ToString().c_str(), cooldown);
    }

    activeGankers.erase(bot);
    ganker2victim.erase(bot);
}

void GankerScheduler::OnVictimGone(ObjectGuid victim)
{
    nextEligibleAt.erase(victim);

    std::vector<ObjectGuid> toRelease;
    for (auto const& kv : ganker2victim)
    {
        if (kv.second == victim)
            toRelease.push_back(kv.first);
    }

    for (ObjectGuid bguid : toRelease)
    {
        Player* bot = ObjectAccessor::FindConnectedPlayer(bguid);
        if (bot)
        {
            PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
            if (botAI)
            {
                AiObjectContext* context = botAI->GetAiObjectContext();
                context->GetValue<ObjectGuid>("ganker target guid")->Set(ObjectGuid::Empty);
                context->GetValue<uint32>("ganker camp until")->Set(0);
                context->GetValue<uint32>("ganker next emote at")->Set(0);
                context->GetValue<uint32>("ganker death count")->Set(0);
                context->GetValue<uint32>("ganker revive at")->Set(0);
                botAI->ChangeStrategy("-ganker", BOT_STATE_NON_COMBAT);
                botAI->ChangeStrategy("-ganker dead", BOT_STATE_DEAD);
                botAI->ChangeStrategy("+new rpg", BOT_STATE_NON_COMBAT);
            }
        }
        activeGankers.erase(bguid);
        ganker2victim.erase(bguid);
    }
}

std::vector<std::string> GankerScheduler::GetStatusReport() const
{
    std::vector<std::string> lines;
    std::ostringstream oss;
    oss << "Ganker scheduler: enabled=" << (sPlayerbotAIConfig.gankerEnabled ? "1" : "0")
        << " activeGankers=" << activeGankers.size()
        << "/" << sPlayerbotAIConfig.gankerMaxConcurrentGankers
        << " trackedVictims=" << nextEligibleAt.size();
    lines.push_back(oss.str());

    time_t now = time(nullptr);

    // Group ganker bots by victim so pairs render as "bot1+bot2 -> victim".
    std::unordered_map<ObjectGuid, std::vector<ObjectGuid>> victim2bots;
    for (auto const& kv : ganker2victim)
        victim2bots[kv.second].push_back(kv.first);

    for (auto const& kv : victim2bots)
    {
        ObjectGuid vguid = kv.first;
        Player* victim = ObjectAccessor::FindConnectedPlayer(vguid);

        std::ostringstream l;
        l << "  ";
        bool first = true;
        for (ObjectGuid bguid : kv.second)
        {
            Player* bot = ObjectAccessor::FindConnectedPlayer(bguid);
            if (!first)
                l << "+";
            l << bguid.ToString();
            if (bot)
                l << " <" << bot->GetName() << ">";
            first = false;
        }
        l << " -> victim " << vguid.ToString();
        if (victim)
            l << " <" << victim->GetName() << ">";
        lines.push_back(l.str());
    }

    for (auto const& kv : nextEligibleAt)
    {
        if (kv.second <= now)
            continue;
        std::ostringstream l;
        l << "  cooldown: victim " << kv.first.ToString()
          << " for " << (uint32)(kv.second - now) << "s";
        lines.push_back(l.str());
    }

    return lines;
}
