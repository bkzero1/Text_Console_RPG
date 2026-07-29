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

    std::vector<InventorySlot> slots;  // 아이템 슬롯들

    void ClearEmptySlots();   // 빈 슬롯 제거
    void CompactInventory();  // 인벤토리 정리 (동일 아이템 슬롯 합치기 & 빈 슬롯 제거)

   public:
    Inventory();

    int GetGold() const;
    void AddGold(int gold);

    bool AddItem(EItemID itemID);
    void ConsumeItem(EItemID itemID, int count = 1);

    int GetItemCount(EItemID itemID) const;
    std::map<EItemID, int> GetConsumableItems() const;

    void ShowInventory() const;  // 인벤토리 출력

    void SortInventory(EInventorySortKey sortKey, bool reverse = false);  // 인벤토리 정렬
};
