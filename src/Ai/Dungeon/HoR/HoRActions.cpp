/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoRActions.h"
#include "Creature.h"
#include "DynamicObject.h"
#include "Group.h"
#include "HoRShared.h"
#include "Playerbots.h"
#include "RtiTargetValue.h"
#include "SpellAuras.h"
#include "SpellMgr.h"

using namespace HallsOfReflection;

bool HoRRunToLeaderAction::isUseful()
{
    Creature* leader = GetEscapeLeader(bot);
    return leader && bot->GetExactDist2d(leader) > LEADER_ARRIVED_DISTANCE;
}

bool HoRRunToLeaderAction::Execute(Event /*event*/)
{
    Creature* leader = GetEscapeLeader(bot);
    if (!leader)
        return false;

    return MoveNear(leader, LEADER_STANDOFF, MovementPriority::MOVEMENT_FORCED);
}

bool HoRMarkPriorityTargetAction::isUseful()
{
    return bot->GetGroup() && !AI_VALUE(ObjectGuid, "hor priority target").IsEmpty();
}

bool HoRMarkPriorityTargetAction::Execute(Event /*event*/)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    Unit* target = botAI->GetUnit(AI_VALUE(ObjectGuid, "hor priority target"));
    if (!target || !target->IsAlive())
        return false;

    group->SetTargetIcon(RtiTargetValue::skullIndex, bot->GetGUID(), target->GetGUID());
    return true;
}

bool HoRReturnToWaveCenterAction::Execute(Event /*event*/)
{
    return MoveInside(MAP_HALLS_OF_REFLECTION, WAVE_CENTER.GetPositionX(), WAVE_CENTER.GetPositionY(),
                      WAVE_CENTER.GetPositionZ(), WAVE_RETURN_RADIUS, MovementPriority::MOVEMENT_FORCED);
}

bool HoRDragGeneralHomeAction::isUseful()
{
    Creature* general = GetActiveFrostswornGeneral(bot);
    return general && general->GetExactDist(general->GetHomePosition()) > GENERAL_LEASH_DISTANCE;
}

bool HoRDragGeneralHomeAction::Execute(Event /*event*/)
{
    Creature* general = GetActiveFrostswornGeneral(bot);
    if (!general)
        return false;

    Position const& home = general->GetHomePosition();
    return MoveTo(bot->GetMapId(), home.GetPositionX(), home.GetPositionY(), home.GetPositionZ(), false, false,
                  false, false, MovementPriority::MOVEMENT_COMBAT);
}

bool HoRLeaveWellAction::Execute(Event /*event*/)
{
    Aura* well = bot->GetAura(sSpellMgr->GetSpellIdForDifficulty(SPELL_WELL_OF_CORRUPTION, bot));
    if (!well || well->GetType() != DYNOBJ_AURA_TYPE)
        return false;

    DynamicObject* pool = well->GetDynobjOwner();
    if (!pool)
        return false;

    return FleePosition(pool->GetPosition(), WELL_FLEE_RADIUS);
}
