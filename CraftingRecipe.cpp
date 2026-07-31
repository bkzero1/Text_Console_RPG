#include "CraftingRecipe.h"

CraftingRecipe::CraftingRecipe(EItemID itemID, int outputCount, std::map<EItemID, int> ingredients)
    : itemID(itemID), outputCount(outputCount), ingredients(ingredients)
{
}
