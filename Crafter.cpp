#include "Crafter.h"

#include <iostream>

std::string Crafter::GetLowerString(const std::string& str) const
{
    std::string lowerStr = str;
    for (char& c : lowerStr)
    {
        tolower(c);
    }
    return lowerStr;
}

bool Crafter::IsRecipeMatchingFilter(const CraftingRecipe& craftingRecipe, const std::string& filterKeyword, const EFilterFlag filterFlag) const
{
    if (filterKeyword.empty()) return true;  // 필터 키워드 없음 -> 항상 true

    const ItemData& craftingItem = ITEM_TABLE.at(craftingRecipe.itemID);
    std::string lowerFilterKeyword = GetLowerString(filterKeyword);

    switch (filterFlag)
    {
        case EFilterFlag::ALL_NAME:  // ITEM_NAME or INGREDIENT_NAME 중 하나만 만족하면 true
            return IsRecipeMatchingFilter(craftingRecipe, filterKeyword, EFilterFlag::ITEM_NAME) || IsRecipeMatchingFilter(craftingRecipe, filterKeyword, EFilterFlag::INGREDIENT_NAME);
        case EFilterFlag::ITEM_NAME:  // 제작 아이템 이름 확인
            if (GetLowerString(craftingItem.name).find(lowerFilterKeyword) != std::string::npos)
            {
                return true;
            }
            break;
        case EFilterFlag::INGREDIENT_NAME:  // 재료 아이템 이름 확인
            for (const auto& [ingredientItemID, IngredientCount] : craftingRecipe.ingredients)
            {
                if (GetLowerString(ITEM_TABLE.at(ingredientItemID).name).find(lowerFilterKeyword) != std::string::npos)
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

void Crafter::SHOW_ALL_RECIPES()
{
    std::cout << "============== < 레시피 > ==============" << "\n";
    int row = 1;
    for (const auto& [targetItemID, craftingRecipe] : CRAFTING_RECIPE_TABLE)
    {
        const ItemData& targetItem = ITEM_TABLE.at(targetItemID);
        std::cout << row++ << ". " << targetItem.name << " (" << targetItem.description << ") —— [";
        std::string ingredientsStr;
        for (const auto& [ingredientItemID, ingredientCount] : craftingRecipe.ingredients)
        {
            const ItemData& ingredientItem = ITEM_TABLE.at(ingredientItemID);
            ingredientsStr += ingredientItem.name + " x" + std::to_string(ingredientCount) + ", ";
        }
        ingredientsStr.erase(ingredientsStr.length() - 2);
        std::cout << "]" << "\n";
    }
    std::cout << "========================================" << "\n";
}

int Crafter::TRY_CRAFT_ITEM(Inventory* inventory, EItemID targetItemID, int count)
{
    // 유효하지 않는 인벤토리
    if (!inventory) return 0;

    // 제작 레시피가 존재하지 않음
    if (CRAFTING_RECIPE_TABLE.find(targetItemID) == CRAFTING_RECIPE_TABLE.end()) return 0;

    // 유효하지 않는 제작 개수
    if (count <= 0) return 0;

    // 제작할 개수 구하기
    int addableCount = inventory->GetMaxAddableItemCount(targetItemID);  // 인벤토리에 추가 가능한 제작 아이템 개수
    int craftableCount = addableCount;                                   // 제작 가능한 개수
    for (const auto& [ingredientItemID, ingredientCount] : CRAFTING_RECIPE_TABLE.at(targetItemID).ingredients)
    {
        craftableCount = std::min(craftableCount, inventory->GetItemCount(ingredientItemID) / ingredientCount);  // 재료 개수를 보고 제작 가능한 최대 개수 설정
    }

    int finalCount = std::min({count, addableCount, craftableCount});  // 최종 제작할 아이템 개수
    if (finalCount <= 0) return 0;                                     // 제작 불가

    // 아이템 제작 - 재료 소모 & 아이템 추가
    for (const auto& [ingredientItemID, ingredientCount] : CRAFTING_RECIPE_TABLE.at(targetItemID).ingredients)
    {
        inventory->ConsumeItem(ingredientItemID, finalCount * ingredientCount);  // 제작 재료 소모
    }
    inventory->AddItem(targetItemID, finalCount);  // 제작한 아이템 추가

    return finalCount;
}

void Crafter::SetFilter(std::string keyword, EFilterFlag filterFlag)
{
    this->filterKeyword = keyword;
    this->filterFlag = filterFlag;
}

void Crafter::ClearFilter()
{
    this->filterKeyword.clear();
    this->filterFlag = EFilterFlag::ALL_NAME;
}

const std::vector<CraftingRecipe*>& Crafter::GetFilteredRecipes() const
{
    return filteredRecipes;
}

void Crafter::ShowFilteredRecipes() const
{
    std::cout << "============== < 레시피 > ==============" << "\n";
    int row = 1;
    for (const auto& [targetItemID, craftingRecipe] : CRAFTING_RECIPE_TABLE)
    {
        if (!IsRecipeMatchingFilter(craftingRecipe, filterKeyword, filterFlag)) continue;  // 필터 매칭 X

        const ItemData& targetItem = ITEM_TABLE.at(targetItemID);
        std::cout << row++ << ". " << targetItem.name << " (" << targetItem.description << ") —— [";
        std::string ingredientsStr;
        for (const auto& [ingredientItemID, ingredientCount] : craftingRecipe.ingredients)
        {
            const ItemData& ingredientItem = ITEM_TABLE.at(ingredientItemID);
            ingredientsStr += ingredientItem.name + " x" + std::to_string(ingredientCount) + ", ";
        }
        ingredientsStr.erase(ingredientsStr.length() - 2);
        std::cout << "]" << "\n";
    }
    std::cout << "========================================" << "\n";
}