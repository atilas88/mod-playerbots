/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_GANKERTRIGGERS_H
#define _PLAYERBOT_GANKERTRIGGERS_H

#include "Trigger.h"

class PlayerbotAI;

class GankerTargetLostTrigger : public Trigger
{
public:
    GankerTargetLostTrigger(PlayerbotAI* botAI) : Trigger(botAI, "ganker target lost", 2) {}

    bool IsActive() override;
};

class GankerTargetOutOfRangeTrigger : public Trigger
{
public:
    GankerTargetOutOfRangeTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "ganker target out of range", 1)
    {
    }

    bool IsActive() override;
};

class GankerTargetDeadTrigger : public Trigger
{
public:
    GankerTargetDeadTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "ganker target dead", 1)
    {
    }

    bool IsActive() override;
};

class GankerCampExpiredTrigger : public Trigger
{
public:
    GankerCampExpiredTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "ganker camp expired", 1)
    {
    }

    bool IsActive() override;
};

class GankerSelfDeadTrigger : public Trigger
{
public:
    GankerSelfDeadTrigger(PlayerbotAI* botAI) : Trigger(botAI, "ganker self dead", 1) {}
    bool IsActive() override;
};

class GankerReviveReadyTrigger : public Trigger
{
public:
    GankerReviveReadyTrigger(PlayerbotAI* botAI) : Trigger(botAI, "ganker revive ready", 1) {}
    bool IsActive() override;
};

class GankerShouldRetreatTrigger : public Trigger
{
public:
    GankerShouldRetreatTrigger(PlayerbotAI* botAI) : Trigger(botAI, "ganker should retreat", 1) {}
    bool IsActive() override;
};

#endif
