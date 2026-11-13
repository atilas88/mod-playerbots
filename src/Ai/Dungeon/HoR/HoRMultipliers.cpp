/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoRMultipliers.h"
#include "Creature.h"
#include "FollowActions.h"
#include "HoRActions.h"
#include "HoRShared.h"
#include "MovementActions.h"
#include "Playerbots.h"

using namespace HallsOfReflection;

namespace
{
bool IsFlightAction(Action* action)
{
    return dynamic_cast<FleeAction*>(action) || dynamic_cast<FleeWithPetAction*>(action) ||
           dynamic_cast<RunAwayAction*>(action);
}
}

float HoREscapeMovementMultiplier::GetValue(Action* action)
{
    if (!dynamic_cast<MovementAction*>(action) || dynamic_cast<HoRRunToLeaderAction*>(action))
        return 1.0f;

    Creature* lichKing = GetEscapeLichKing(bot);
    if (!lichKing)
        return 1.0f;

    // The corridor runs one way; generic flight can carry a bot behind the Lich King into the Zap.
    if (dynamic_cast<FollowAction*>(action) || IsFlightAction(action))
        return 0.0f;

    if (IsInWinter(bot, lichKing) || IsBehindLichKing(bot, lichKing))
        return 0.0f;

    return 1.0f;
}

float HoRWaveLeashMultiplier::GetValue(Action* action)
{
    if (!IsFlightAction(action))
        return 1.0f;

    if (bot->GetExactDist2d(WAVE_CENTER) <= WAVE_FLEE_LEASH_DISTANCE || !IsWaveInProgress(bot))
        return 1.0f;

    return 0.0f;
}
