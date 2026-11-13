/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORVALUECONTEXT_H
#define PLAYERBOTS_HORVALUECONTEXT_H

#include "HoRShared.h"
#include "NamedObjectContext.h"
#include "ObjectGuid.h"
#include "PlayerbotAI.h"
#include "Value.h"

class HoRPriorityTargetValue : public ObjectGuidCalculatedValue
{
public:
    HoRPriorityTargetValue(PlayerbotAI* botAI)
        : ObjectGuidCalculatedValue(botAI, "hor priority target", HallsOfReflection::VALUE_CHECK_INTERVAL)
    {
    }

protected:
    ObjectGuid Calculate() override;
};

class HoRMeleeExclusionsValue : public ObjectGuidListCalculatedValue
{
public:
    HoRMeleeExclusionsValue(PlayerbotAI* botAI)
        : ObjectGuidListCalculatedValue(botAI, "hor melee exclusions", HallsOfReflection::VALUE_CHECK_INTERVAL)
    {
    }

protected:
    GuidVector Calculate() override;
};

class HoREscapeHasTargetsValue : public BoolCalculatedValue
{
public:
    HoREscapeHasTargetsValue(PlayerbotAI* botAI)
        : BoolCalculatedValue(botAI, "hor escape has targets", HallsOfReflection::VALUE_CHECK_INTERVAL)
    {
    }

protected:
    bool Calculate() override;
};

class WotlkDungeonHoRValueContext : public NamedObjectContext<UntypedValue>
{
public:
    WotlkDungeonHoRValueContext()
    {
        creators["hor priority target"] = &WotlkDungeonHoRValueContext::hor_priority_target;
        creators["hor melee exclusions"] = &WotlkDungeonHoRValueContext::hor_melee_exclusions;
        creators["hor escape has targets"] = &WotlkDungeonHoRValueContext::hor_escape_has_targets;
    }

private:
    static UntypedValue* hor_priority_target(PlayerbotAI* botAI) { return new HoRPriorityTargetValue(botAI); }
    static UntypedValue* hor_melee_exclusions(PlayerbotAI* botAI) { return new HoRMeleeExclusionsValue(botAI); }
    static UntypedValue* hor_escape_has_targets(PlayerbotAI* botAI) { return new HoREscapeHasTargetsValue(botAI); }
};

#endif
