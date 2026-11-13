/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORTRIGGERCONTEXT_H
#define PLAYERBOTS_HORTRIGGERCONTEXT_H

#include "HoRTriggers.h"
#include "NamedObjectContext.h"

class WotlkDungeonHoRTriggerContext : public NamedObjectContext<Trigger>
{
public:
    WotlkDungeonHoRTriggerContext()
    {
        creators["hor excluded current target"] = &WotlkDungeonHoRTriggerContext::hor_excluded_current_target;
        creators["hor priority target unmarked"] = &WotlkDungeonHoRTriggerContext::hor_priority_target_unmarked;
        creators["hor in remorseless winter"] = &WotlkDungeonHoRTriggerContext::hor_in_remorseless_winter;
        creators["hor behind lich king"] = &WotlkDungeonHoRTriggerContext::hor_behind_lich_king;
        creators["hor idle far from leader"] = &WotlkDungeonHoRTriggerContext::hor_idle_far_from_leader;
        creators["hor wave leash"] = &WotlkDungeonHoRTriggerContext::hor_wave_leash;
        creators["hor general leash"] = &WotlkDungeonHoRTriggerContext::hor_general_leash;
        creators["hor well of corruption"] = &WotlkDungeonHoRTriggerContext::hor_well_of_corruption;
    }

private:
    static Trigger* hor_excluded_current_target(PlayerbotAI* botAI)
    {
        return new HoRExcludedCurrentTargetTrigger(botAI);
    }
    static Trigger* hor_priority_target_unmarked(PlayerbotAI* botAI)
    {
        return new HoRPriorityTargetUnmarkedTrigger(botAI);
    }
    static Trigger* hor_in_remorseless_winter(PlayerbotAI* botAI) { return new HoRInRemorselessWinterTrigger(botAI); }
    static Trigger* hor_behind_lich_king(PlayerbotAI* botAI) { return new HoRBehindLichKingTrigger(botAI); }
    static Trigger* hor_idle_far_from_leader(PlayerbotAI* botAI) { return new HoRIdleFarFromLeaderTrigger(botAI); }
    static Trigger* hor_wave_leash(PlayerbotAI* botAI) { return new HoRWaveLeashTrigger(botAI); }
    static Trigger* hor_general_leash(PlayerbotAI* botAI) { return new HoRGeneralLeashTrigger(botAI); }
    static Trigger* hor_well_of_corruption(PlayerbotAI* botAI) { return new HoRWellOfCorruptionTrigger(botAI); }
};

#endif
