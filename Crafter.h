#pragma once

#include <string>
#include <vector>

#include "CraftingRecipe.h"
#include "Inventory.h"

enum class EFilterFlag
{
    ALL_NAME,         // 전체 이름
    ITEM_NAME,        // 제작 아이템 이름
    INGREDIENT_NAME,  // 재료 아이템 이름
};

class Crafter
{
   private:
    std::string filterKeyword;                                   // 필터 키워드
    EFilterFlag filterFlag;                                      // 필터 플래그
    std::vector<const CraftingRecipe*> filteredCraftingRecipes;  // 필터 적용된 레시피들

    static std::string GET_LOWER_STRING(const std::string& str);                                                                                  // 소문자 문자열 반환
    static bool IS_RECIPE_MATCHING_FILTER(const CraftingRecipe* craftingRecipe, const std::string& filterKeyword, const EFilterFlag filterFlag);  // 필터 매칭 여부 반환

   public:
    Crafter();

    static int TRY_CRAFT_ITEM(Inventory* inventory, const CraftingRecipe* recipe, const int& count = 1);  // 아이템 제작 시도 (가능한 최대로 제작 및 성공 개수 반환)

    void SetFilter(std::string filterKeyword, EFilterFlag filterFlag = EFilterFlag::ALL_NAME);  // 필터 설정
    void ClearFilter();                                                                         // 필터 초기화
    void ApplyFilter();                                                                         // 필터 적용

    const std::vector<const CraftingRecipe*>& GetFilteredCraftingRecipes() const;  // 필터링된 제작 레시피 리스트 반환
    int GetFilteredCraftingRecipesSize() const;                                    // 필터링된 제작 레시피 리스트 크기 반환
    const CraftingRecipe* GetCraftingRecipeByIndex(int index) const;               // 필터링된 제작 레시피 리스트의 특정 인덱스 값 반환

    void ShowFilteredRecipes() const;  // 필터링된 레시피 리스트 출력
};
