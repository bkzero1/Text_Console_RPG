#pragma once

#include <map>
#include <vector>

#include "Item.h"

struct CraftingRecipe
{
    EItemID itemID;                      // 제작 아이템
    std::map<EItemID, int> ingredients;  // 제작 재료

    CraftingRecipe(EItemID itemID, std::map<EItemID, int> ingredients);
};

extern const std::map<EItemID, std::vector<CraftingRecipe>> CRAFTING_RECIPE_TABLE;