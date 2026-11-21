/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_RAIDRECRUITMGR_H
#define PLAYERBOTS_RAIDRECRUITMGR_H

#include <string>
#include <unordered_map>
#include <vector>

#include "Common.h"
#include "ObjectGuid.h"
#include "RaidAnnounceParser.h"

class Channel;
class Player;

// Makes random bots answer raid recruitment messages that real players write in General and LookingForGroup.
// World thread only: both the chat hook and Update run there.
class RaidRecruitMgr
{
public:
    static RaidRecruitMgr& instance()
    {
        static RaidRecruitMgr instance;

        return instance;
    }

    void OnChannelMessage(Player* player, std::string const& message, Channel* channel);
    void Update();

private:
    struct Request
    {
        ObjectGuid playerGuid;
        RaidAnnounce announce;
        std::string channelName;
    };

    struct Reply
    {
        ObjectGuid botGuid;
        ObjectGuid playerGuid;
        uint64 dueTimeMs = 0;
        uint32 gearScore = 0;
        bool classRequested = false;
        std::string channelName;
    };

    struct Candidate
    {
        ObjectGuid botGuid;
        uint32 gearScore = 0;
    };

    RaidRecruitMgr() = default;

    void SelectCandidates(Request const& request, uint64 nowMs);
    void SendReply(Reply const& reply) const;
    bool IsAvailable(Player* bot, Player* player) const;
    bool HasPendingReply(ObjectGuid botGuid) const;
    void PurgeCooldowns(time_t now);

    std::vector<Request> _requests;
    std::vector<Reply> _replies;
    std::unordered_map<ObjectGuid, time_t> _cooldownEnd;  // announcing player -> when they can get replies again
    time_t _nextCooldownPurge = 0;
};

#endif
