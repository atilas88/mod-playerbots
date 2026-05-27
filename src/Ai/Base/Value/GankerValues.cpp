/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "GankerValues.h"

#include "ObjectAccessor.h"
#include "Player.h"
#include "Playerbots.h"

Unit* GankerTargetUnitValue::Calculate()
{
    ObjectGuid guid = AI_VALUE(ObjectGuid, "ganker target guid");
    if (guid.IsEmpty())
        return nullptr;

    Player* target = ObjectAccessor::FindConnectedPlayer(guid);
    if (!target)
        return nullptr;

    return target;
}
