/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_GANKERSCHEDULER_H
#define _PLAYERBOT_GANKERSCHEDULER_H

#include <ctime>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ObjectGuid.h"

class Player;

// Per-filter rejection tally from SelectGankers. Without it the "no candidates"
// log can only report an empty pool, never which filter emptied it: the old
// pool-size counter was read after every filter had run, so on the one path that
// logs it, it was always zero.
struct GankerCandidateStats
{
    uint32 scanned = 0;
    uint32 notRandomBot = 0;
    uint32 alreadyGanking = 0;
    uint32 dead = 0;
    uint32 inBgOrDungeon = 0;
    uint32 phased = 0;
    uint32 inCombat = 0;
    uint32 inGroup = 0;
    uint32 sameFaction = 0;
    uint32 outOfLevelRange = 0;
    uint32 zeroClassWeight = 0;
    uint32 eligible = 0;

    std::string ToString() const;
};

class GankerScheduler
{
public:
    GankerScheduler() = default;
    ~GankerScheduler() = default;

    GankerScheduler(GankerScheduler const&) = delete;
    GankerScheduler& operator=(GankerScheduler const&) = delete;

    void Tick();

    void OnGankerReleased(ObjectGuid bot, bool retreat = false);
    void OnVictimGone(ObjectGuid victim);

    std::vector<std::string> GetStatusReport() const;

    bool ComputeTeleportPoint(Player* victim, float angleOffsetRad, float& outX, float& outY, float& outZ) const;

private:
    bool IsEligibleVictim(Player* victim, std::string* reason = nullptr) const;
    std::vector<Player*> SelectGankers(Player* victim, uint32 count, GankerCandidateStats* outStats = nullptr) const;
    bool DispatchGank(Player* victim, std::vector<Player*> const& gankers);
    void PruneStaleGankers();

    uint32 ClassWeight(uint8 cls) const;

    time_t lastTickAt = 0;
    std::unordered_map<ObjectGuid, time_t> nextEligibleAt;  // by victim guid
    std::unordered_set<ObjectGuid> activeGankers;           // bot guids
    std::unordered_map<ObjectGuid, ObjectGuid> ganker2victim;
};

#endif
