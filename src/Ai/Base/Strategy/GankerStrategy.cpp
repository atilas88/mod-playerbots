/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "GankerStrategy.h"

#include "Action.h"
#include "Multiplier.h"
#include "Playerbots.h"

namespace
{
class GankerSuppressIdleMultiplier : public Multiplier
{
public:
    GankerSuppressIdleMultiplier(PlayerbotAI* botAI)
        : Multiplier(botAI, "ganker suppress idle")
    {
    }

    float GetValue(Action* action) override
    {
        if (!action)
            return 1.0f;

        std::string const& name = action->getName();
        if (name.rfind("new rpg", 0) == 0)
            return 0.0f;
        if (name.rfind("travel", 0) == 0)
            return 0.0f;
        if (name.rfind("go grind", 0) == 0)
            return 0.0f;
        if (name.rfind("choose travel target", 0) == 0)
            return 0.0f;
        if (name.rfind("move to travel target", 0) == 0)
            return 0.0f;
        if (name.rfind("choose rpg target", 0) == 0)
            return 0.0f;
        if (name.rfind("move to rpg target", 0) == 0)
            return 0.0f;
        if (name == "rpg")
            return 0.0f;
        if (name == "crpg")
            return 0.0f;

        return 1.0f;
    }
};

class GankerSuppressAutoReleaseMultiplier : public Multiplier
{
public:
    GankerSuppressAutoReleaseMultiplier(PlayerbotAI* botAI)
        : Multiplier(botAI, "ganker suppress auto release")
    {
    }

    float GetValue(Action* action) override
    {
        if (!action)
            return 1.0f;

        ObjectGuid guid = AI_VALUE(ObjectGuid, "ganker target guid");
        if (guid.IsEmpty())
            return 1.0f;

        // Allow only our M3 dead-engine flow; suppress every other dead-engine
        // action (auto release, find corpse, revive from corpse, self resurrect,
        // repop, etc.) so the bot stays in place until ganker resurrect fires.
        std::string const& name = action->getName();
        if (name == "ganker start revive timer" ||
            name == "ganker resurrect" ||
            name == "release ganker")
        {
            return 1.0f;
        }

        return 0.0f;
    }
};
}  // namespace

std::vector<NextAction> GankerStrategy::getDefaultActions()
{
    return { NextAction("stalk ganker target", 4.0f) };
}

void GankerStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    NonCombatStrategy::InitTriggers(triggers);

    triggers.push_back(
        new TriggerNode("ganker target lost", { NextAction("release ganker", 90.0f) }));
    triggers.push_back(
        new TriggerNode("ganker target out of range", { NextAction("pursue ganker target", 30.0f) }));
    triggers.push_back(
        new TriggerNode("enemy player near", { NextAction("attack enemy player", 60.0f) }));
    triggers.push_back(
        new TriggerNode("ganker target dead", { NextAction("camp ganker corpse", 95.0f) }));
    triggers.push_back(
        new TriggerNode("ganker camp expired", { NextAction("release ganker", 95.0f) }));
}

void GankerStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new GankerSuppressIdleMultiplier(botAI));
}

void GankerDeadStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    PassTroughStrategy::InitTriggers(triggers);

    triggers.push_back(
        new TriggerNode("ganker should retreat", { NextAction("release ganker", 95.0f) }));
    triggers.push_back(
        new TriggerNode("ganker self dead", { NextAction("ganker start revive timer", 90.0f) }));
    triggers.push_back(
        new TriggerNode("ganker revive ready", { NextAction("ganker resurrect", 90.0f) }));
}

void GankerDeadStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new GankerSuppressAutoReleaseMultiplier(botAI));
}
