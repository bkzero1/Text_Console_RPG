#include "Crafter.h"

#include <iostream>

std::string Crafter::GET_LOWER_STRING(const std::string& str)
{
    std::string lowerStr = str;
    for (char& c : lowerStr)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return lowerStr;
}

bool Crafter::IS_RECIPE_MATCHING_FILTER(const CraftingRecipe* craftingRecipe, const std::string& filterKeyword, const EFilterFlag filterFlag)
{
    if (filterKeyword.empty()) return true;  // 필터 키워드 없음 -> 항상 true

    const ItemData& craftingItem = ITEM_TABLE.at(craftingRecipe->itemID);
    std::string lowerFilterKeyword = GET_LOWER_STRING(filterKeyword);

    switch (filterFlag)
    {
        case EFilterFlag::ALL_NAME:  // ITEM_NAME or INGREDIENT_NAME 중 하나만 만족하면 true
            return IS_RECIPE_MATCHING_FILTER(craftingRecipe, filterKeyword, EFilterFlag::ITEM_NAME) || IS_RECIPE_MATCHING_FILTER(craftingRecipe, filterKeyword, EFilterFlag::INGREDIENT_NAME);
        case EFilterFlag::ITEM_NAME:  // 제작 아이템 이름 확인
            if (GET_LOWER_STRING(craftingItem.name).find(lowerFilterKeyword) != std::string::npos)
            {
                return true;
            }
            break;
        case EFilterFlag::INGREDIENT_NAME:  // 재료 아이템 이름 확인
            for (const auto& [ingredientItemID, IngredientCount] : craftingRecipe->ingredients)
            {
                if (GET_LOWER_STRING(ITEM_TABLE.at(ingredientItemID).name).find(lowerFilterKeyword) != std::string::npos)
                {
                    return true;
                }
            }
            break;
        default:
            break;
    }

    return false;
}

Crafter::Crafter()
    : filterKeyword(""), filterFlag(EFilterFlag::ALL_NAME)
{
}

int Crafter::TRY_CRAFT_ITEM(Inventory* inventory, const CraftingRecipe* craftingRecipe, const int& count)
{
    // 유효하지 않는 인벤토리
    if (!inventory) return 0;

    // 유효하지 않는 제작 개수
    if (count <= 0) return 0;

    // 제작할 개수 구하기
    int addableCount = inventory->GetMaxAddableItemCount(craftingRecipe->itemID);  // 인벤토리에 추가 가능한 제작 아이템 개수
    int craftableCount = addableCount;                                             // 제작 가능한 개수
    for (const auto& [ingredientItemID, ingredientCount] : craftingRecipe->ingredients)
    {
        craftableCount = std::min(craftableCount, inventory->GetItemCount(ingredientItemID) / ingredientCount);  // 재료 개수를 보고 제작 가능한 최대 개수 설정
    }

    int finalCount = std::min({count, addableCount, craftableCount});  // 최종 제작할 아이템 개수
    if (finalCount <= 0) return 0;                                     // 제작 불가

    // 아이템 제작 - 재료 소모 & 아이템 추가
    for (const auto& [ingredientItemID, ingredientCount] : craftingRecipe->ingredients)
    {
        inventory->ConsumeItem(ingredientItemID, finalCount * ingredientCount);  // 제작 재료 소모
    }
    inventory->AddItem(craftingRecipe->itemID, finalCount);  // 제작한 아이템 추가

    // 최종 제작 개수 반환
    return finalCount;
}

void Crafter::SetFilter(std::string filterKeyword, EFilterFlag filterFlag)
{
    this->filterKeyword = filterKeyword;
    this->filterFlag = filterFlag;
}

void Crafter::ClearFilter()
{
    this->filterKeyword.clear();
    this->filterFlag = EFilterFlag::ALL_NAME;
}

void Crafter::ApplyFilter()
{
    filteredCraftingRecipes.clear();
    for (const auto& [targetItemID, craftingRecipes] : CRAFTING_RECIPE_TABLE)
    {
        for (const CraftingRecipe& recipe : craftingRecipes)
        {
            if (IS_RECIPE_MATCHING_FILTER(&recipe, filterKeyword, filterFlag))
            {
                filteredCraftingRecipes.push_back(&recipe);
            }
        }
    }
}

const std::vector<const CraftingRecipe*>& Crafter::GetFilteredCraftingRecipes() const
{
    return filteredCraftingRecipes;
}

int Crafter::GetFilteredCraftingRecipesSize() const
{
    return (int)filteredCraftingRecipes.size();
}

const CraftingRecipe* Crafter::GetCraftingRecipeByIndex(int index) const
{
    return filteredCraftingRecipes.at(index);
}

void Crafter::ShowFilteredRecipes() const
{
    std::cout << "============== < 레시피 > ==============" << "\n";
    for (int i = 0; i < GetFilteredCraftingRecipesSize(); i++)
    {
        const CraftingRecipe* craftingRecipe = GetCraftingRecipeByIndex(i);
        const ItemData& craftingItem = ITEM_TABLE.at(craftingRecipe->itemID);

        std::cout << i + 1 << ". " << craftingItem.name << " (" << craftingItem.description << ") —— [";
        if (!craftingRecipe->ingredients.empty())  // 재료가 있는 경우만 표시
        {
            std::string ingredientsStr;
            for (const auto& [ingredientItemID, ingredientCount] : craftingRecipe->ingredients)
            {
                const ItemData& ingredientItem = ITEM_TABLE.at(ingredientItemID);
                ingredientsStr += ingredientItem.name + " x" + std::to_string(ingredientCount) + ", ";
            }
            ingredientsStr.erase(ingredientsStr.length() - 2);
            std::cout << ingredientsStr;
        }
        std::cout << "]" << "\n";
    }
    std::cout << "========================================" << "\n";
}