#pragma once

#include <map>
#include <vector>

#include "InventorySlot.h"

enum class EInventorySortKey
{
    NAME,
    COUNT,
    PRICE,
};

class Inventory
{
   private:
    int gold;  // 골드

    int maxSlots;      // 인벤토리 최대 슬롯 수
    int slotCapacity;  // 각 슬롯 최대 용량

    std::map<EItemID, int> itemCounts;  // 아이템 개수들
    std::vector<InventorySlot> slots;   // 아이템 슬롯들

    void ClearEmptySlots();   // 빈 슬롯 제거
    void CompactInventory();  // 인벤토리 정리 (동일 아이템 슬롯 합치기 & 빈 슬롯 제거)

   public:
    Inventory();

    int GetGold() const;
    void AddGold(int gold);

    int AddItem(EItemID itemID, int count = 1);       // 아이템 추가 (추가하지 못하고 남은 개수 반환)
    bool ConsumeItem(EItemID itemID, int count = 1);  // 아이템 소모 (부족하면 false 반환, 충분하면 개수 차감 후 true 반환)

    int GetItemCount(EItemID itemID) const;             // 아이템 개수 반환
    int GetMaxAddableItemCount(EItemID itemID) const;   // 아이템 추가 가능한 최대 개수 반환
    std::map<EItemID, int> GetConsumableItems() const;  // 사용 가능한 아이템 및 개수 반환

    void ShowInventory() const;  // 인벤토리 출력

    void SortInventory(EInventorySortKey sortKey, bool reverse = false);  // 인벤토리 정렬
};
