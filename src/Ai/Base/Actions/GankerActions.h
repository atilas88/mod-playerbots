/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_GANKERACTIONS_H
#define _PLAYERBOT_GANKERACTIONS_H

#include "Action.h"
#include "MovementActions.h"

class PlayerbotAI;

class StalkGankerTargetAction : public MovementAction
{
public:
    StalkGankerTargetAction(PlayerbotAI* botAI) : MovementAction(botAI, "stalk ganker target") {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class PursueGankerTargetAction : public MovementAction
{
public:
    PursueGankerTargetAction(PlayerbotAI* botAI) : MovementAction(botAI, "pursue ganker target") {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class ReleaseGankerAction : public Action
{
public:
    ReleaseGankerAction(PlayerbotAI* botAI) : Action(botAI, "release ganker") {}

    bool Execute(Event event) override;
};

class CampGankerCorpseAction : public MovementAction
{
public:
    CampGankerCorpseAction(PlayerbotAI* botAI) : MovementAction(botAI, "camp ganker corpse") {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class GankerStartReviveTimerAction : public Action
{
public:
    GankerStartReviveTimerAction(PlayerbotAI* botAI) : Action(botAI, "ganker start revive timer") {}

    bool Execute(Event event) override;
};

class GankerResurrectAction : public Action
{
public:
    GankerResurrectAction(PlayerbotAI* botAI) : Action(botAI, "ganker resurrect") {}

    bool Execute(Event event) override;
};

#endif
