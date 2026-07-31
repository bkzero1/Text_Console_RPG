#pragma once

#include <map>

#include "Item.h"

struct CraftingRecipe
{
    EItemID itemID;                      // 제작 아이템
    int outputCount;                     // 제작 개수
    std::map<EItemID, int> ingredients;  // 제작 재료

    CraftingRecipe(EItemID itemID, int outputCount, std::map<EItemID, int> ingredients);
};
