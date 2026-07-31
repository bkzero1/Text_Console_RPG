#pragma once
#include <vector>

#include "Inventory.h"
#include "RpgLogger.h"

extern Inventory* inventory;
extern RpgLogger rpgLogger;

class ShopManager
{
   public:
    // 유저에게 아이템 구매 할건지 판매 할건지
    // 구매 -> 구매 가능한 리스트 출력 -> 내가 보유중인 골드 출력
    // -> 어떤 아이템을 구매할지 선택 -> 충분한 골드가 있다면 골드 차감 후 인벤토리에 추가
    // 판매 -> 내 인벤토리 리스트 출력 -> 어떤 것을 판매할 것 인지 선택 -> 몇 개 판매할 것인지 선택
    // 아이템 가격의 60%만큼 플레이어 골드 추가, 인벤토리에서 차감

    // 구매 가능한 리스트 출력
    void ShowBuyableList() const;
    // 구매 가능한 아이템 ID 목록 (ShowBuyableList와 동일한 순서)
    std::vector<EItemID> GetBuyableItemIDs() const;
    // 아이템 구매
    void BuyItem(const ItemData& item, int count) const;
    // 판매 가능한 리스트 출력
    void ShowSellableList() const;
    // 판매 가능한 아이템 ID 목록 (ShowSellableList와 동일한 순서)
    std::vector<EItemID> GetSellableItemIDs() const;
    // 아이템 판매
    void SellItem(const ItemData& item, int count) const;

   private:
};