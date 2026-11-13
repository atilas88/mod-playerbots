/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoRShared.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "Player.h"
#include "SpellMgr.h"

namespace HallsOfReflection
{
namespace
{
InstanceScript* GetHoRInstance(Player* bot)
{
    if (!bot || bot->GetMapId() != MAP_HALLS_OF_REFLECTION)
        return nullptr;

    return bot->GetInstanceScript();
}

bool IsEscapeActive(InstanceScript* instance)
{
    return instance && instance->GetBossState(DATA_LICH_KING) == IN_PROGRESS;
}
}

Creature* GetEscapeLichKing(Player* bot)
{
    InstanceScript* instance = GetHoRInstance(bot);
    if (!IsEscapeActive(instance))
        return nullptr;

    Creature* lichKing = instance->GetCreature(NPC_LICH_KING_BOSS);
    return lichKing && lichKing->IsAlive() ? lichKing : nullptr;
}

Creature* GetEscapeLeader(Player* bot)
{
    InstanceScript* instance = GetHoRInstance(bot);
    if (!IsEscapeActive(instance))
        return nullptr;

    Creature* leader = instance->GetCreature(NPC_LEADER_KEY);
    return leader && leader->IsAlive() ? leader : nullptr;
}

Creature* GetLichKingAnyPhase(Player* bot)
{
    InstanceScript* instance = GetHoRInstance(bot);
    if (!instance)
        return nullptr;

    return instance->GetCreature(NPC_LICH_KING_BOSS);
}

Creature* GetActiveFrostswornGeneral(Player* bot)
{
    InstanceScript* instance = GetHoRInstance(bot);
    if (!instance)
        return nullptr;

    Creature* general = instance->GetCreature(NPC_FROSTSWORN_GENERAL);
    if (!general || !general->IsAlive() || !general->IsInCombat())
        return nullptr;

    return general;
}

bool HasRemorselessWinter(Player* bot, Unit* lichKing)
{
    if (!lichKing)
        return false;

    return lichKing->HasAura(sSpellMgr->GetSpellIdForDifficulty(SPELL_REMORSELESS_WINTER, bot));
}

bool IsInWinter(Player* bot, Creature* lichKing)
{
    return HasRemorselessWinter(bot, lichKing) && bot->GetExactDist2d(lichKing) < WINTER_ENTER_DISTANCE;
}

bool IsBehindLichKing(Player* bot, Creature* lichKing)
{
    if (!HasRemorselessWinter(bot, lichKing))
        return false;

    // The core zaps along a fixed (+X, +Y) axis, not relative to the Lich King's facing.
    float const metric = (bot->GetPositionX() - lichKing->GetPositionX()) +
                         (bot->GetPositionY() - lichKing->GetPositionY());
    return metric > ZAP_SAFE_METRIC;
}

bool IsEscapeDanger(Player* bot)
{
    Creature* lichKing = GetEscapeLichKing(bot);
    if (!lichKing)
        return false;

    return IsInWinter(bot, lichKing) || IsBehindLichKing(bot, lichKing);
}

bool IsWaveInProgress(Player* bot)
{
    InstanceScript* instance = GetHoRInstance(bot);
    if (!instance)
        return false;

    return instance->GetData(DATA_WAVE_NUMBER) > 0 && instance->GetBossState(DATA_MARWYN) != DONE;
}

bool IsPriorityEntry(uint32 entry)
{
    return entry == NPC_RISEN_WITCH_DOCTOR || entry == NPC_SHADOWY_MERCENARY || entry == NPC_GHOSTLY_PRIEST;
}
}
