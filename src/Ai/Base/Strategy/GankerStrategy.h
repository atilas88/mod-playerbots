/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_GANKERSTRATEGY_H
#define _PLAYERBOT_GANKERSTRATEGY_H

#include "NonCombatStrategy.h"
#include "PassTroughStrategy.h"

class PlayerbotAI;

class GankerStrategy : public NonCombatStrategy
{
public:
    GankerStrategy(PlayerbotAI* botAI) : NonCombatStrategy(botAI) {}

    std::string const getName() override { return "ganker"; }
    std::vector<NextAction> getDefaultActions() override;
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

class GankerDeadStrategy : public PassTroughStrategy
{
public:
    GankerDeadStrategy(PlayerbotAI* botAI) : PassTroughStrategy(botAI) {}

    std::string const getName() override { return "ganker dead"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
