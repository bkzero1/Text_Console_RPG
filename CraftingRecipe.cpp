#include "CraftingRecipe.h"

CraftingRecipe::CraftingRecipe(EItemID itemID, std::map<EItemID, int> ingredients)
    : itemID(itemID), ingredients(ingredients)
{
}
