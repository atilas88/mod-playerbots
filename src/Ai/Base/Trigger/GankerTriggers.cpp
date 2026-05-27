/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "GankerTriggers.h"

#include <ctime>

#include "Map.h"
#include "Player.h"
#include "Playerbots.h"

bool GankerTargetLostTrigger::IsActive()
{
    ObjectGuid guid = AI_VALUE(ObjectGuid, "ganker target guid");
    if (guid.IsEmpty())
        return false;

    uint32 campUntil = AI_VALUE(uint32, "ganker camp until");
    if (campUntil && (uint32)time(nullptr) <= campUntil)
        return false;

    Unit* target = AI_VALUE(Unit*, "ganker target unit");
    if (!target)
        return true;

    Player* victim = target->ToPlayer();
    if (!victim)
        return true;

    if (!victim->IsInWorld() || victim->GetMapId() != bot->GetMapId())
        return true;

    if (bot->GetDistance(victim) > sPlayerbotAIConfig.gankerMaxPursueDistanceYards)
        return true;

    if (!victim->IsPvP() && !bot->IsInCombatWith(victim))
        return true;

    if (sPlayerbotAIConfig.IsPvpProhibited(victim->GetZoneId(), victim->GetAreaId()))
        return true;

    if (victim->InBattleground() || (victim->GetMap() && victim->GetMap()->IsDungeon()))
        return true;

    return false;
}

bool GankerTargetDeadTrigger::IsActive()
{
    ObjectGuid guid = AI_VALUE(ObjectGuid, "ganker target guid");
    if (guid.IsEmpty())
        return false;

    if (AI_VALUE(uint32, "ganker camp until") != 0)
        return false;

    Unit* target = AI_VALUE(Unit*, "ganker target unit");
    if (!target)
        return false;

    Player* victim = target->ToPlayer();
    if (!victim || !victim->IsInWorld())
        return false;

    if (victim->IsAlive())
        return false;

    if (victim->GetMapId() != bot->GetMapId())
        return false;

    if (bot->GetDistance(victim) > sPlayerbotAIConfig.gankerMaxPursueDistanceYards)
        return false;

    if (victim->InBattleground() || (victim->GetMap() && victim->GetMap()->IsDungeon()))
        return false;

    if (sPlayerbotAIConfig.IsPvpProhibited(victim->GetZoneId(), victim->GetAreaId()))
        return false;

    return true;
}

bool GankerCampExpiredTrigger::IsActive()
{
    uint32 until = AI_VALUE(uint32, "ganker camp until");
    if (!until)
        return false;

    return (uint32)time(nullptr) > until;
}

bool GankerSelfDeadTrigger::IsActive()
{
    ObjectGuid guid = AI_VALUE(ObjectGuid, "ganker target guid");
    if (guid.IsEmpty())
        return false;

    if (bot->IsAlive())
        return false;

    if (AI_VALUE(uint32, "ganker revive at") != 0)
        return false;

    if (AI_VALUE(uint32, "ganker death count") >= sPlayerbotAIConfig.gankerMaxDeathsBeforeRetreat)
        return false;

    return true;
}

bool GankerReviveReadyTrigger::IsActive()
{
    ObjectGuid guid = AI_VALUE(ObjectGuid, "ganker target guid");
    if (guid.IsEmpty())
        return false;

    if (bot->IsAlive())
        return false;

    uint32 reviveAt = AI_VALUE(uint32, "ganker revive at");
    if (!reviveAt)
        return false;

    return (uint32)time(nullptr) >= reviveAt;
}

bool GankerShouldRetreatTrigger::IsActive()
{
    ObjectGuid guid = AI_VALUE(ObjectGuid, "ganker target guid");
    if (guid.IsEmpty())
        return false;

    if (bot->IsAlive())
        return false;

    return AI_VALUE(uint32, "ganker death count") >= sPlayerbotAIConfig.gankerMaxDeathsBeforeRetreat;
}

bool GankerTargetOutOfRangeTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "ganker target unit");
    if (!target || !target->IsAlive())
        return false;

    if (bot->IsInCombat())
        return false;

    return bot->GetDistance(target) > sPlayerbotAIConfig.gankerEngagementRadiusYards;
}
