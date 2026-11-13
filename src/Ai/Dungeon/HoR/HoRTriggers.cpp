/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoRTriggers.h"
#include "Creature.h"
#include "Group.h"
#include "HoRShared.h"
#include "Playerbots.h"
#include "RtiTargetValue.h"
#include "SpellMgr.h"
#include <algorithm>

using namespace HallsOfReflection;

bool HoRExcludedCurrentTargetTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
        return false;

    if (target->GetEntry() == NPC_LICH_KING_BOSS)
        return true;

    GuidVector const& excluded = AI_VALUE_REF(GuidVector, "hor melee exclusions");
    return std::find(excluded.begin(), excluded.end(), target->GetGUID()) != excluded.end();
}

bool HoRPriorityTargetUnmarkedTrigger::IsActive()
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    if (AI_VALUE(ObjectGuid, "hor priority target").IsEmpty())
        return false;

    Unit* skull = botAI->GetUnit(group->GetTargetIcon(RtiTargetValue::skullIndex));
    return !skull || !skull->IsAlive() || skull->GetEntry() == NPC_LICH_KING_BOSS;
}

bool HoRInRemorselessWinterTrigger::IsActive()
{
    Creature* lichKing = GetEscapeLichKing(bot);
    return lichKing && IsInWinter(bot, lichKing);
}

bool HoRBehindLichKingTrigger::IsActive()
{
    Creature* lichKing = GetEscapeLichKing(bot);
    return lichKing && IsBehindLichKing(bot, lichKing);
}

bool HoRIdleFarFromLeaderTrigger::IsActive()
{
    Creature* leader = GetEscapeLeader(bot);
    if (!leader || bot->GetExactDist2d(leader) <= LEADER_IDLE_DISTANCE)
        return false;

    if (botAI->HasStrategy("stay", botAI->GetState()))
        return false;

    return !AI_VALUE(bool, "hor escape has targets");
}

bool HoRWaveLeashTrigger::IsActive()
{
    return bot->GetExactDist2d(WAVE_CENTER) > WAVE_LEASH_DISTANCE && IsWaveInProgress(bot);
}

bool HoRGeneralLeashTrigger::IsActive()
{
    if (!PlayerbotAI::IsTank(bot))
        return false;

    Creature* general = GetActiveFrostswornGeneral(bot);
    if (!general || general->GetVictim() != bot)
        return false;

    return general->GetExactDist(general->GetHomePosition()) > GENERAL_LEASH_DISTANCE;
}

bool HoRWellOfCorruptionTrigger::IsActive()
{
    return bot->HasAura(sSpellMgr->GetSpellIdForDifficulty(SPELL_WELL_OF_CORRUPTION, bot));
}
