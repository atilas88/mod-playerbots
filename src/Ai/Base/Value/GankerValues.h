/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_GANKERVALUES_H
#define _PLAYERBOT_GANKERVALUES_H

#include "ObjectGuid.h"
#include "Value.h"

class PlayerbotAI;
class Unit;

class GankerTargetGuidValue : public ManualSetValue<ObjectGuid>
{
public:
    GankerTargetGuidValue(PlayerbotAI* botAI)
        : ManualSetValue<ObjectGuid>(botAI, ObjectGuid::Empty, "ganker target guid")
    {
    }
};

class GankerTargetUnitValue : public CalculatedValue<Unit*>
{
public:
    GankerTargetUnitValue(PlayerbotAI* botAI)
        : CalculatedValue<Unit*>(botAI, "ganker target unit", 1)
    {
    }

protected:
    Unit* Calculate() override;
};

class GankerIsTempGroupValue : public ManualSetValue<bool>
{
public:
    GankerIsTempGroupValue(PlayerbotAI* botAI)
        : ManualSetValue<bool>(botAI, false, "ganker is temp group")
    {
    }
};

class GankerCampUntilValue : public ManualSetValue<uint32>
{
public:
    GankerCampUntilValue(PlayerbotAI* botAI)
        : ManualSetValue<uint32>(botAI, 0, "ganker camp until")
    {
    }
};

class GankerNextEmoteAtValue : public ManualSetValue<uint32>
{
public:
    GankerNextEmoteAtValue(PlayerbotAI* botAI)
        : ManualSetValue<uint32>(botAI, 0, "ganker next emote at")
    {
    }
};

class GankerDeathCountValue : public ManualSetValue<uint32>
{
public:
    GankerDeathCountValue(PlayerbotAI* botAI)
        : ManualSetValue<uint32>(botAI, 0, "ganker death count")
    {
    }
};

class GankerReviveAtValue : public ManualSetValue<uint32>
{
public:
    GankerReviveAtValue(PlayerbotAI* botAI)
        : ManualSetValue<uint32>(botAI, 0, "ganker revive at")
    {
    }
};

#endif
