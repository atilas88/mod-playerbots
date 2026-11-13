/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoRStrategy.h"
#include "Creature.h"
#include "HoRMultipliers.h"
#include "HoRShared.h"
#include "Playerbots.h"

void WotlkDungeonHoRStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("hor excluded current target", {
        NextAction("drop target", ACTION_EMERGENCY + 6) }));

    triggers.push_back(new TriggerNode("hor in remorseless winter", {
        NextAction("hor run to leader", ACTION_EMERGENCY + 5) }));

    triggers.push_back(new TriggerNode("hor behind lich king", {
        NextAction("hor run to leader", ACTION_EMERGENCY + 4) }));

    triggers.push_back(new TriggerNode("hor well of corruption", {
        NextAction("hor leave well", ACTION_EMERGENCY + 3) }));

    triggers.push_back(new TriggerNode("hor wave leash", {
        NextAction("hor return to wave center", ACTION_RAID + 3) }));

    triggers.push_back(new TriggerNode("hor priority target unmarked", {
        NextAction("hor mark priority target", ACTION_RAID + 2) }));

    triggers.push_back(new TriggerNode("hor general leash", {
        NextAction("hor drag general home", ACTION_RAID + 1) }));

    triggers.push_back(new TriggerNode("hor idle far from leader", {
        NextAction("hor run to leader", ACTION_MOVE + 5) }));
}

void WotlkDungeonHoRStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new HoREscapeMovementMultiplier(botAI));
    multipliers.push_back(new HoRWaveLeashMultiplier(botAI));
}

void WotlkDungeonHoRStrategy::AppendTargetExclusions(GuidSet& exclusions, TargetValueExclusionType /*type*/)
{
    Player* bot = botAI->GetBot();
    if (Creature* lichKing = HallsOfReflection::GetLichKingAnyPhase(bot))
        exclusions.insert(lichKing->GetGUID());

    if (!PlayerbotAI::IsMelee(bot))
        return;

    AiObjectContext* context = botAI->GetAiObjectContext();
    auto const& melee = AI_VALUE_REF(GuidVector, "hor melee exclusions");
    exclusions.insert(melee.begin(), melee.end());
}
