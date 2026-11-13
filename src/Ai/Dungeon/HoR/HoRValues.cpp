/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "Creature.h"
#include "HoRShared.h"
#include "HoRValueContext.h"
#include "Playerbots.h"
#include <algorithm>

using namespace HallsOfReflection;

ObjectGuid HoRPriorityTargetValue::Calculate()
{
    if (!IsWaveInProgress(bot) && !GetEscapeLichKing(bot))
        return ObjectGuid::Empty;

    Unit* best = nullptr;
    for (ObjectGuid const guid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || !IsPriorityEntry(unit->GetEntry()))
            continue;

        if (!best || unit->GetHealth() < best->GetHealth() ||
            (unit->GetHealth() == best->GetHealth() && unit->GetGUID() < best->GetGUID()))
            best = unit;
    }

    return best ? best->GetGUID() : ObjectGuid::Empty;
}

GuidVector HoRMeleeExclusionsValue::Calculate()
{
    GuidVector exclusions;
    if (!PlayerbotAI::IsMelee(bot))
        return exclusions;

    Creature* lichKing = GetEscapeLichKing(bot);
    bool const winter = HasRemorselessWinter(bot, lichKing);

    for (ObjectGuid const guid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        if (unit->GetEntry() == NPC_PHANTOM_HALLUCINATION ||
            (winter && unit != lichKing && unit->GetExactDist2d(lichKing) < WINTER_ENTER_DISTANCE))
            exclusions.push_back(guid);
    }

    return exclusions;
}

bool HoREscapeHasTargetsValue::Calculate()
{
    if (!GetEscapeLichKing(bot))
        return false;

    GuidVector const& excluded = AI_VALUE_REF(GuidVector, "hor melee exclusions");
    for (ObjectGuid const guid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || unit->GetEntry() == NPC_LICH_KING_BOSS)
            continue;

        if (std::find(excluded.begin(), excluded.end(), guid) == excluded.end())
            return true;
    }

    return false;
}
