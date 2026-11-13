/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORSHARED_H
#define PLAYERBOTS_HORSHARED_H

#include "AreaDefines.h"
#include "Define.h"
#include "Position.h"

class Creature;
class Player;
class Unit;

namespace HallsOfReflection
{
// Mirrors halls_of_reflection.h and boss_marwyn.cpp in the core scripts.
constexpr uint32 DATA_MARWYN = 1;
constexpr uint32 DATA_LICH_KING = 2;
constexpr uint32 DATA_WAVE_NUMBER = 8;

constexpr uint32 NPC_LICH_KING_BOSS = 36954;
constexpr uint32 NPC_LEADER_KEY = 37554;  // Sylvanas; the core keys Jaina under the same entry
constexpr uint32 NPC_FROSTSWORN_GENERAL = 36723;

constexpr uint32 NPC_RISEN_WITCH_DOCTOR = 36941;
constexpr uint32 NPC_SHADOWY_MERCENARY = 38177;
constexpr uint32 NPC_GHOSTLY_PRIEST = 38175;
constexpr uint32 NPC_PHANTOM_HALLUCINATION = 38567;

constexpr uint32 SPELL_REMORSELESS_WINTER = 69780;
constexpr uint32 SPELL_WELL_OF_CORRUPTION = 72362;

Position const WAVE_CENTER = {5309.46f, 2006.48f, 711.60f};
constexpr float WAVE_LEASH_DISTANCE = 60.0f;      // core resets every wave past 70.5
constexpr float WAVE_FLEE_LEASH_DISTANCE = 55.0f;
constexpr float WAVE_RETURN_RADIUS = 30.0f;

constexpr float WINTER_ENTER_DISTANCE = 13.0f;    // aura radius is 10
constexpr float ZAP_SAFE_METRIC = 12.0f;          // core zaps when (dx + dy) > 20

constexpr float LEADER_ARRIVED_DISTANCE = 6.0f;
constexpr float LEADER_IDLE_DISTANCE = 8.0f;
constexpr float LEADER_STANDOFF = 5.0f;

constexpr float GENERAL_LEASH_DISTANCE = 22.0f;   // core evades past 30

constexpr float WELL_FLEE_RADIUS = 5.0f;

constexpr uint32 VALUE_CHECK_INTERVAL = 500;

Creature* GetEscapeLichKing(Player* bot);
Creature* GetEscapeLeader(Player* bot);
Creature* GetLichKingAnyPhase(Player* bot);
Creature* GetActiveFrostswornGeneral(Player* bot);

bool HasRemorselessWinter(Player* bot, Unit* lichKing);
bool IsInWinter(Player* bot, Creature* lichKing);
bool IsBehindLichKing(Player* bot, Creature* lichKing);
bool IsEscapeDanger(Player* bot);
bool IsWaveInProgress(Player* bot);

bool IsPriorityEntry(uint32 entry);
}

#endif
