/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORMULTIPLIERS_H
#define PLAYERBOTS_HORMULTIPLIERS_H

#include "Multiplier.h"

class HoREscapeMovementMultiplier : public Multiplier
{
public:
    HoREscapeMovementMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "hor escape movement") {}
    float GetValue(Action* action) override;
};

class HoRWaveLeashMultiplier : public Multiplier
{
public:
    HoRWaveLeashMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "hor wave leash") {}
    float GetValue(Action* action) override;
};

#endif
