#include "CraftingRecipe.h"

CraftingRecipe::CraftingRecipe(EItemID itemID, std::map<EItemID, int> ingredients)
    : itemID(itemID), ingredients(ingredients)
{
}

const std::map<EItemID, std::vector<CraftingRecipe>> CRAFTING_RECIPE_TABLE = {
    // HP 포션
    {
        EItemID::HP_POTION,
        {
            CraftingRecipe(EItemID::HP_POTION, {}),  // 임시 레피시 (재료 없이 제작 가능)
            CraftingRecipe(EItemID::HP_POTION, {
                                                   {EItemID::BOTTLE, 1},
                                                   {EItemID::WATER, 2},
                                                   {EItemID::SLIME_JELLY, 1},
                                               }),
        },
    },
    // 공격력 증가 포션
    {
        EItemID::POWER_POTION,
        {
            CraftingRecipe(EItemID::POWER_POTION, {}),  // 임시 레피시 (재료 없이 제작 가능)
            CraftingRecipe(EItemID::POWER_POTION, {
                                                      {EItemID::BOTTLE, 1},
                                                      {EItemID::SLIME_CORE, 1},
                                                      {EItemID::SLIME_CLEAR_WATER_DROP, 1},
                                                  }),
        },
    },
};