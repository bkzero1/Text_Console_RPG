#pragma once

#include <string>
#include <vector>

#include "CraftingRecipe.h"
#include "Inventory.h"

extern const std::map<EItemID, CraftingRecipe> CRAFTING_RECIPE_TABLE;

class Crafter
{
   public:
    static void SHOW_ALL_RECIPES();                                                // 모든 제작 레시피 출력
    static void SHOW_RECIPES(const std::vector<CraftingRecipe>& craftingRecipes);  // 주어진 제작 레시피 출력

    static int TRY_CRAFT_ITEM(Inventory* inventory, EItemID targetItemID, int count = 1);  // 아이템 제작 시도 (가능한 최대로 제작 및 성공 개수 반환)

    static std::vector<CraftingRecipe> SEARCH_RECIPES_BY_ITEM_NAME(const std::string keyword);        // 제작 아이템 이름으로 검색한 레시피들 반환
    static std::vector<CraftingRecipe> SEARCH_RECIPES_BY_INGREDIENT_NAME(const std::string keyword);  // 제작 재료 이름으로 검색한 레시피들 반환
};
