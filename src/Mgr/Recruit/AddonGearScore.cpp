/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AddonGearScore.h"

#include <cmath>

#include "Item.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "SharedDefines.h"

namespace
{
constexpr float GEAR_SCORE_SCALE = 1.8618f;
constexpr float LEGENDARY_QUALITY_SCALE = 1.3f;
constexpr float JUNK_QUALITY_SCALE = 0.005f;
constexpr float HEIRLOOM_ITEM_LEVEL = 187.05f;
constexpr float HIGH_FORMULA_MIN_ITEM_LEVEL = 120.0f;
constexpr float TITAN_GRIP_MODIFIER = 0.5f;
constexpr float HUNTER_MELEE_MODIFIER = 0.3164f;
constexpr float HUNTER_RANGED_MODIFIER = 5.3224f;

struct FormulaConstants
{
    float offset;
    float divisor;
};

float SlotModifier(uint32 inventoryType)
{
    switch (inventoryType)
    {
        case INVTYPE_2HWEAPON:
            return 2.0f;
        case INVTYPE_HEAD:
        case INVTYPE_CHEST:
        case INVTYPE_ROBE:
        case INVTYPE_LEGS:
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_WEAPONOFFHAND:
        case INVTYPE_SHIELD:
        case INVTYPE_HOLDABLE:
            return 1.0f;
        case INVTYPE_SHOULDERS:
        case INVTYPE_WAIST:
        case INVTYPE_FEET:
        case INVTYPE_HANDS:
            return 0.75f;
        case INVTYPE_NECK:
        case INVTYPE_WRISTS:
        case INVTYPE_FINGER:
        case INVTYPE_TRINKET:
        case INVTYPE_CLOAK:
            return 0.5625f;
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_THROWN:
        case INVTYPE_RELIC:
            return 0.3164f;
        default:
            return 0.0f;
    }
}

// quality must already be folded into uncommon, rare or epic
FormulaConstants GetFormulaConstants(uint32 quality, float itemLevel)
{
    if (itemLevel > HIGH_FORMULA_MIN_ITEM_LEVEL)
    {
        switch (quality)
        {
            case ITEM_QUALITY_EPIC:
                return {91.45f, 0.65f};
            case ITEM_QUALITY_RARE:
                return {81.375f, 0.8125f};
            default:
                return {73.0f, 1.0f};
        }
    }

    switch (quality)
    {
        case ITEM_QUALITY_EPIC:
            return {26.0f, 1.2f};
        case ITEM_QUALITY_RARE:
            return {0.75f, 1.8f};
        default:
            return {8.0f, 2.0f};
    }
}

bool IsTwoHanded(Item const* item) { return item && item->GetTemplate()->InventoryType == INVTYPE_2HWEAPON; }
}  // namespace

uint32 AddonGearScore::ItemScore(ItemTemplate const* proto)
{
    if (!proto)
        return 0;

    float const slotModifier = SlotModifier(proto->InventoryType);
    if (slotModifier <= 0.0f)
        return 0;

    uint32 quality = proto->Quality;
    float itemLevel = static_cast<float>(proto->ItemLevel);
    float qualityScale = 1.0f;
    switch (quality)
    {
        case ITEM_QUALITY_LEGENDARY:
            qualityScale = LEGENDARY_QUALITY_SCALE;
            quality = ITEM_QUALITY_EPIC;
            break;
        case ITEM_QUALITY_POOR:
        case ITEM_QUALITY_NORMAL:
            qualityScale = JUNK_QUALITY_SCALE;
            quality = ITEM_QUALITY_UNCOMMON;
            break;
        case ITEM_QUALITY_HEIRLOOM:
            quality = ITEM_QUALITY_RARE;
            itemLevel = HEIRLOOM_ITEM_LEVEL;
            break;
        case ITEM_QUALITY_UNCOMMON:
        case ITEM_QUALITY_RARE:
        case ITEM_QUALITY_EPIC:
            break;
        default:
            return 0;
    }

    FormulaConstants const constants = GetFormulaConstants(quality, itemLevel);
    float const score =
        (itemLevel - constants.offset) / constants.divisor * slotModifier * GEAR_SCORE_SCALE * qualityScale;
    return score > 0.0f ? static_cast<uint32>(std::floor(score)) : 0;
}

uint32 AddonGearScore::Calculate(Player* player)
{
    if (!player)
        return 0;

    Item const* mainHand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    Item const* offHand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

    // The addon halves both hands on a dual-wielded two-hander by looking at the items alone, not at the talent
    bool const titanGrip = (mainHand && offHand && IsTwoHanded(mainHand)) || IsTwoHanded(offHand);
    bool const isHunter = player->getClass() == CLASS_HUNTER;

    float total = 0.0f;
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        if (slot == EQUIPMENT_SLOT_BODY || slot == EQUIPMENT_SLOT_TABARD)
            continue;

        Item const* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        float score = static_cast<float>(ItemScore(item->GetTemplate()));
        bool const isHandSlot = slot == EQUIPMENT_SLOT_MAINHAND || slot == EQUIPMENT_SLOT_OFFHAND;
        if (isHunter && isHandSlot)
            score *= HUNTER_MELEE_MODIFIER;
        else if (isHunter && slot == EQUIPMENT_SLOT_RANGED)
            score *= HUNTER_RANGED_MODIFIER;

        if (titanGrip && isHandSlot)
            score *= TITAN_GRIP_MODIFIER;

        total += score;
    }

    return static_cast<uint32>(std::floor(total));
}
