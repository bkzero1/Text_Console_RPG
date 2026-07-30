#include "Crafter.h"

const std::map<EItemID, CraftingRecipe> CRAFTING_RECIPE_TABLE = {

};

bool Crafter::TRY_CRAFT_ITEM(Inventory* inventory, EItemID targetItemID)
{
    // 제작 레시피가 존재하지 않음
    if (CRAFTING_RECIPE_TABLE.find(targetItemID) == CRAFTING_RECIPE_TABLE.end())
    {
        return false;
    }

    // 인벤토리에 제작할 아이템을 추가할 공간 부족
    if (inventory->GetMaxAddableItemCount(targetItemID) == 0)
    {
        return false;
    }

    // 인벤토리 내 제작 재료 부족
    for (const auto& [ingredientItemID, ingredientCount] : CRAFTING_RECIPE_TABLE.at(targetItemID).ingredients)
    {
        if (inventory->GetItemCount(ingredientItemID) < ingredientCount)
        {
            return false;
        }
    }

    // 아이템 제작 - 재료 소모 & 아이템 추가
    for (const auto& [ingredientItemID, ingredientCount] : CRAFTING_RECIPE_TABLE.at(targetItemID).ingredients)
    {
        inventory->ConsumeItem(ingredientItemID, ingredientCount);  // 제작 재료 소모
    }
    inventory->AddItem(targetItemID);  // 제작한 아이템 추가

    return true;
}
