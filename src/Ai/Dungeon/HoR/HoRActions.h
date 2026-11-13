/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORACTIONS_H
#define PLAYERBOTS_HORACTIONS_H

#include "Action.h"
#include "MovementActions.h"

class HoRRunToLeaderAction : public MovementAction
{
public:
    HoRRunToLeaderAction(PlayerbotAI* botAI) : MovementAction(botAI, "hor run to leader") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class HoRMarkPriorityTargetAction : public Action
{
public:
    HoRMarkPriorityTargetAction(PlayerbotAI* botAI) : Action(botAI, "hor mark priority target") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class HoRReturnToWaveCenterAction : public MovementAction
{
public:
    HoRReturnToWaveCenterAction(PlayerbotAI* botAI) : MovementAction(botAI, "hor return to wave center") {}
    bool Execute(Event event) override;
};

class HoRDragGeneralHomeAction : public MovementAction
{
public:
    HoRDragGeneralHomeAction(PlayerbotAI* botAI) : MovementAction(botAI, "hor drag general home") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class HoRLeaveWellAction : public MovementAction
{
public:
    HoRLeaveWellAction(PlayerbotAI* botAI) : MovementAction(botAI, "hor leave well") {}
    bool Execute(Event event) override;
};

#endif
