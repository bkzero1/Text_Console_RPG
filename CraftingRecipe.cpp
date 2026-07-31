#include "CraftingRecipe.h"

CraftingRecipe::CraftingRecipe(EItemID itemID, std::map<EItemID, int> ingredients)
    : itemID(itemID), ingredients(ingredients)
{
}

const std::map<EItemID, CraftingRecipe> CRAFTING_RECIPE_TABLE = {
    {EItemID::HP_POTION, CraftingRecipe(EItemID::HP_POTION,  // 임시 데이터
                                        {
                                            {EItemID::NONE, 1},
                                            {EItemID::HP_POTION, 2},
                                            {EItemID::POWER_POTION, 3},
                                        })},
};