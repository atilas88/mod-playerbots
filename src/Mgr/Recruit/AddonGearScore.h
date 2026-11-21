/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ADDONGEARSCORE_H
#define PLAYERBOTS_ADDONGEARSCORE_H

#include "Define.h"

class Player;

struct ItemTemplate;

// GearScore on the scale players quote in chat ("5.8k gs"): reimplements the formula of the GearScoreLite 3.1.15
// client addon by Mirrikat45. PlayerbotAI::GetEquipGearScore is an average item level, a different number.
namespace AddonGearScore
{
uint32 ItemScore(ItemTemplate const* proto);
uint32 Calculate(Player* player);
}  // namespace AddonGearScore

#endif
