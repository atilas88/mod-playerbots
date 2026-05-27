/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "GankerActions.h"

#include <ctime>

#include "GankerScheduler.h"
#include "Group.h"
#include "Map.h"
#include "Player.h"
#include "Playerbots.h"
#include "RandomPlayerbotMgr.h"
#include "SharedDefines.h"

bool StalkGankerTargetAction::isUseful()
{
    Unit* target = AI_VALUE(Unit*, "ganker target unit");
    return target && target->IsAlive() && !bot->IsInCombat();
}

bool StalkGankerTargetAction::Execute(Event /*event*/)
{
    Unit* target = AI_VALUE(Unit*, "ganker target unit");
    if (!target)
        return false;

    float standoff = sPlayerbotAIConfig.gankerEngagementRadiusYards;
    float distance = bot->GetDistance(target);

    // Already at standoff range — stay in place to keep LOS without breaking it.
    if (distance <= standoff && distance >= sPlayerbotAIConfig.contactDistance + 1.0f)
        return false;

    return MoveNear(target, standoff);
}

bool PursueGankerTargetAction::isUseful()
{
    Unit* target = AI_VALUE(Unit*, "ganker target unit");
    return target && target->IsAlive();
}

bool PursueGankerTargetAction::Execute(Event /*event*/)
{
    Unit* target = AI_VALUE(Unit*, "ganker target unit");
    if (!target)
        return false;

    return MoveNear(target, sPlayerbotAIConfig.gankerEngagementRadiusYards);
}

bool ReleaseGankerAction::Execute(Event /*event*/)
{
    bool isTempGroup = AI_VALUE(bool, "ganker is temp group");
    uint32 deathCount = AI_VALUE(uint32, "ganker death count");
    bool retreat = deathCount >= sPlayerbotAIConfig.gankerMaxDeathsBeforeRetreat;

    SET_AI_VALUE(ObjectGuid, "ganker target guid", ObjectGuid::Empty);
    SET_AI_VALUE(bool, "ganker is temp group", false);
    SET_AI_VALUE(uint32, "ganker camp until", 0);
    SET_AI_VALUE(uint32, "ganker next emote at", 0);
    SET_AI_VALUE(uint32, "ganker death count", 0);
    SET_AI_VALUE(uint32, "ganker revive at", 0);

    botAI->ChangeStrategy("-ganker", BOT_STATE_NON_COMBAT);
    botAI->ChangeStrategy("-ganker dead", BOT_STATE_DEAD);
    botAI->ChangeStrategy("+new rpg", BOT_STATE_NON_COMBAT);

    if (isTempGroup && bot->GetGroup())
    {
        Group* group = bot->GetGroup();
        group->RemoveMember(bot->GetGUID());
        if (group->GetMembersCount() < 2)
            group->Disband();
    }

    sRandomPlayerbotMgr.GetGankerScheduler().OnGankerReleased(bot->GetGUID(), retreat);

    LOG_INFO("playerbots",
             "Ganker bot {} <{}> released ({}), restoring new rpg",
             bot->GetGUID().ToString().c_str(), bot->GetName().c_str(),
             retreat ? "retreat" : "normal");

    return true;
}

bool CampGankerCorpseAction::isUseful()
{
    return AI_VALUE(Unit*, "ganker target unit") != nullptr;
}

bool CampGankerCorpseAction::Execute(Event /*event*/)
{
    Unit* target = AI_VALUE(Unit*, "ganker target unit");
    if (!target)
        return false;

    uint32 now = (uint32)time(nullptr);
    uint32 campUntil = AI_VALUE(uint32, "ganker camp until");

    if (campUntil == 0)
    {
        uint32 minDur = sPlayerbotAIConfig.gankerCampDurationMinSeconds;
        uint32 maxDur = sPlayerbotAIConfig.gankerCampDurationMaxSeconds;
        if (maxDur < minDur)
            maxDur = minDur;
        uint32 dur = (minDur == maxDur) ? minDur : urand(minDur, maxDur);
        campUntil = now + dur;
        SET_AI_VALUE(uint32, "ganker camp until", campUntil);
        SET_AI_VALUE(uint32, "ganker next emote at", now + urand(2, 4));

        bot->HandleEmoteCommand(EMOTE_ONESHOT_RUDE);

        LOG_INFO("playerbots",
                 "Ganker bot {} <{}> camping corpse of victim {} for {}s",
                 bot->GetGUID().ToString().c_str(), bot->GetName().c_str(),
                 target->GetGUID().ToString().c_str(), dur);
    }

    float distance = bot->GetDistance(target);
    if (distance > 6.0f)
        return MoveNear(target, 3.0f);

    bot->SetFacingToObject(target);

    uint32 nextEmote = AI_VALUE(uint32, "ganker next emote at");
    if (nextEmote && now >= nextEmote)
    {
        uint32 pick = urand(0, 1);
        bot->HandleEmoteCommand(pick == 0 ? EMOTE_ONESHOT_LAUGH : EMOTE_ONESHOT_RUDE);
        SET_AI_VALUE(uint32, "ganker next emote at", now + urand(15, 25));
    }

    return true;
}

bool GankerStartReviveTimerAction::Execute(Event /*event*/)
{
    uint32 count = AI_VALUE(uint32, "ganker death count") + 1;
    SET_AI_VALUE(uint32, "ganker death count", count);

    uint32 minDelay = sPlayerbotAIConfig.gankerRevivalDelayMinSeconds;
    uint32 maxDelay = sPlayerbotAIConfig.gankerRevivalDelayMaxSeconds;
    if (maxDelay < minDelay)
        maxDelay = minDelay;
    uint32 delay = (minDelay == maxDelay) ? minDelay : urand(minDelay, maxDelay);
    uint32 reviveAt = (uint32)time(nullptr) + delay;
    SET_AI_VALUE(uint32, "ganker revive at", reviveAt);

    LOG_INFO("playerbots",
             "Ganker bot {} <{}> killed by victim, reviving in {}s (death {}/{})",
             bot->GetGUID().ToString().c_str(), bot->GetName().c_str(),
             delay, count, sPlayerbotAIConfig.gankerMaxDeathsBeforeRetreat);

    return true;
}

bool GankerResurrectAction::Execute(Event /*event*/)
{
    if (bot->IsAlive())
    {
        SET_AI_VALUE(uint32, "ganker revive at", 0);
        return true;
    }

    bot->ResurrectPlayer(1.0f, false);
    bot->SpawnCorpseBones();
    bot->SetHealth(bot->GetMaxHealth());
    if (bot->getPowerType() == POWER_MANA)
        bot->SetPower(POWER_MANA, bot->GetMaxPower(POWER_MANA));

    SET_AI_VALUE(uint32, "ganker revive at", 0);

    Unit* target = AI_VALUE(Unit*, "ganker target unit");
    Player* victim = target ? target->ToPlayer() : nullptr;
    if (victim && victim->IsInWorld() && victim->GetMapId() == bot->GetMapId())
    {
        float tx = 0.f, ty = 0.f, tz = 0.f;
        if (sRandomPlayerbotMgr.GetGankerScheduler().ComputeTeleportPoint(victim, 0.0f, tx, ty, tz))
        {
            bot->GetMotionMaster()->Clear();
            bot->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED |
                                               AURA_INTERRUPT_FLAG_CHANGE_MAP);
            bot->TeleportTo(victim->GetMapId(), tx, ty, tz, victim->GetOrientation());
        }
    }

    LOG_INFO("playerbots",
             "Ganker bot {} <{}> resurrected, resuming pursuit (death {}/{})",
             bot->GetGUID().ToString().c_str(), bot->GetName().c_str(),
             AI_VALUE(uint32, "ganker death count"),
             sPlayerbotAIConfig.gankerMaxDeathsBeforeRetreat);

    return true;
}
