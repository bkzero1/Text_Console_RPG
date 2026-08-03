#pragma once

#include <map>

#include "Item.h"

struct CraftingRecipe
{
    EItemID itemID;                      // 제작 아이템
    std::map<EItemID, int> ingredients;  // 제작 재료

    CraftingRecipe(EItemID itemID, std::map<EItemID, int> ingredients);
};

extern const std::map<EItemID, CraftingRecipe> CRAFTING_RECIPE_TABLE;