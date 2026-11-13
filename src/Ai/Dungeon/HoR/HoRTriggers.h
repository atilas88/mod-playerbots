/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORTRIGGERS_H
#define PLAYERBOTS_HORTRIGGERS_H

#include "Trigger.h"

class HoRExcludedCurrentTargetTrigger : public Trigger
{
public:
    HoRExcludedCurrentTargetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "hor excluded current target") {}
    bool IsActive() override;
};

class HoRPriorityTargetUnmarkedTrigger : public Trigger
{
public:
    HoRPriorityTargetUnmarkedTrigger(PlayerbotAI* botAI) : Trigger(botAI, "hor priority target unmarked") {}
    bool IsActive() override;
};

class HoRInRemorselessWinterTrigger : public Trigger
{
public:
    HoRInRemorselessWinterTrigger(PlayerbotAI* botAI) : Trigger(botAI, "hor in remorseless winter") {}
    bool IsActive() override;
};

class HoRBehindLichKingTrigger : public Trigger
{
public:
    HoRBehindLichKingTrigger(PlayerbotAI* botAI) : Trigger(botAI, "hor behind lich king") {}
    bool IsActive() override;
};

class HoRIdleFarFromLeaderTrigger : public Trigger
{
public:
    HoRIdleFarFromLeaderTrigger(PlayerbotAI* botAI) : Trigger(botAI, "hor idle far from leader") {}
    bool IsActive() override;
};

class HoRWaveLeashTrigger : public Trigger
{
public:
    HoRWaveLeashTrigger(PlayerbotAI* botAI) : Trigger(botAI, "hor wave leash") {}
    bool IsActive() override;
};

class HoRGeneralLeashTrigger : public Trigger
{
public:
    HoRGeneralLeashTrigger(PlayerbotAI* botAI) : Trigger(botAI, "hor general leash") {}
    bool IsActive() override;
};

class HoRWellOfCorruptionTrigger : public Trigger
{
public:
    HoRWellOfCorruptionTrigger(PlayerbotAI* botAI) : Trigger(botAI, "hor well of corruption") {}
    bool IsActive() override;
};

#endif
