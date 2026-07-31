#include "CraftingRecipe.h"

CraftingRecipe::CraftingRecipe(EItemID itemID, std::map<EItemID, int> ingredients)
    : itemID(itemID), ingredients(ingredients)
{
}

const std::map<EItemID, CraftingRecipe> CRAFTING_RECIPE_TABLE = {
    // 임시 레시피
    {EItemID::HP_POTION, CraftingRecipe(EItemID::HP_POTION,
                                        {
                                            {EItemID::INGREDIENT_1, 2},
                                        })},
    {EItemID::POWER_POTION, CraftingRecipe(EItemID::POWER_POTION,
                                           {
                                               {EItemID::INGREDIENT_2, 1},
                                               {EItemID::INGREDIENT_3, 3},
                                           })},
};