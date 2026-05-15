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
    std::vector<Player*> SelectGankers(Player* victim, uint32 count, uint32* outPoolSize = nullptr) const;
    bool DispatchGank(Player* victim, std::vector<Player*> const& gankers);

    uint32 ClassWeight(uint8 cls) const;

    time_t lastTickAt = 0;
    std::unordered_map<ObjectGuid, time_t> nextEligibleAt;  // by victim guid
    std::unordered_set<ObjectGuid> activeGankers;           // bot guids
    std::unordered_map<ObjectGuid, ObjectGuid> ganker2victim;
};

#endif
