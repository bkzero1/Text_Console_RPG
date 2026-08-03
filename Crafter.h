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
    std::vector<EItemID> filteredCraftingItemIDs;

    std::string GetLowerString(const std::string& str) const;                                                                                 // 소문자 문자열 반환
    bool IsRecipeMatchingFilter(const CraftingRecipe& craftingRecipe, const std::string& filterKeyword, const EFilterFlag filterFlag) const;  // 필터 매칭 여부 반환

   public:
    Crafter();

    static int TRY_CRAFT_ITEM(Inventory* inventory, EItemID targetItemID, int count = 1);  // 아이템 제작 시도 (가능한 최대로 제작 및 성공 개수 반환)

    void SetFilter(std::string keyword, EFilterFlag filterFlag = EFilterFlag::ALL_NAME);  // 필터 설정
    void ClearFilter();                                                                   // 필터 초기화

    const std::vector<EItemID>& GetFilteredCraftingItemIDs() const;  // 필터링된 제작 아이템 ID 리스트 반환
    EItemID GetFilteredCraftingItemIDByIndex(int index) const;       // 필터링된 제작 아이템 ID 리스트의 특정 인덱스 아이템 ID 반환

    void ShowFilteredRecipes() const;  // 필터링된 레시피 리스트 출력
};
