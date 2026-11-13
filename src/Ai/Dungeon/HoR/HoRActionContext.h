/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORACTIONCONTEXT_H
#define PLAYERBOTS_HORACTIONCONTEXT_H

#include "Action.h"
#include "HoRActions.h"
#include "NamedObjectContext.h"

class WotlkDungeonHoRActionContext : public NamedObjectContext<Action>
{
public:
    WotlkDungeonHoRActionContext()
    {
        creators["hor run to leader"] = &WotlkDungeonHoRActionContext::hor_run_to_leader;
        creators["hor mark priority target"] = &WotlkDungeonHoRActionContext::hor_mark_priority_target;
        creators["hor return to wave center"] = &WotlkDungeonHoRActionContext::hor_return_to_wave_center;
        creators["hor drag general home"] = &WotlkDungeonHoRActionContext::hor_drag_general_home;
        creators["hor leave well"] = &WotlkDungeonHoRActionContext::hor_leave_well;
    }

private:
    static Action* hor_run_to_leader(PlayerbotAI* botAI) { return new HoRRunToLeaderAction(botAI); }
    static Action* hor_mark_priority_target(PlayerbotAI* botAI) { return new HoRMarkPriorityTargetAction(botAI); }
    static Action* hor_return_to_wave_center(PlayerbotAI* botAI) { return new HoRReturnToWaveCenterAction(botAI); }
    static Action* hor_drag_general_home(PlayerbotAI* botAI) { return new HoRDragGeneralHomeAction(botAI); }
    static Action* hor_leave_well(PlayerbotAI* botAI) { return new HoRLeaveWellAction(botAI); }
};

#endif
