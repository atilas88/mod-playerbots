/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_RAIDANNOUNCEPARSER_H
#define PLAYERBOTS_RAIDANNOUNCEPARSER_H

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Define.h"

enum class RecruitRole : uint8
{
    Tank,
    Heal,
    Dps
};

struct RecruitSlot
{
    std::optional<RecruitRole> role;  // empty = any role
    uint8 classId = 0;                // CLASS_*, 0 = any class

    bool operator==(RecruitSlot const& other) const { return role == other.role && classId == other.classId; }
};

struct RaidAnnounce
{
    std::vector<RecruitSlot> slots;  // never empty, never a slot without both role and class
    std::string instance;            // normalized key ("icc", "toc", ...), empty when none was recognized
    uint8 size = 0;                  // 10, 25 or 0
    bool heroic = false;
    uint32 minGearScore = 0;  // addon-scale GearScore asked for in the message, 0 = none
};

// Keyword parser for "need a dk tank for icc 25 5.5k+" style recruitment messages. Pure: no game state.
std::optional<RaidAnnounce> ParseRaidAnnounce(std::string_view message);

#endif
