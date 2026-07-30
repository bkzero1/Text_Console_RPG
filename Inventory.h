#pragma once

#include <map>
#include <vector>

#include "InventorySlot.h"

enum class EInventorySortKey
{
    NAME,   // 이름
    COUNT,  // 개수
    PRICE,  // 가격
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

    int GetGold() const;     // 골드 획득
    void AddGold(int gold);  // 골드 추가

    int AddItem(EItemID itemID, int count = 1);                            // 아이템 추가 (추가하지 못하고 남은 개수 반환)
    bool ConsumeItem(EItemID itemID, int count = 1);                       // 아이템 소모 (부족하면 false 반환, 충분하면 개수 차감 후 true 반환)
    std::map<EItemID, int> AddItems(const std::map<EItemID, int>& items);  // 아이템들 추가 후 남은 아이템들 반환

    const std::vector<InventorySlot>& GetSlots() const;  // 인벤토리 슬롯들 반환
    const InventorySlot& GetSlot(int index) const;       // 지정된 인덱스의 슬롯 반환
    void RemoveSlot(int index);                          // 지정된 인덱스의 슬롯 제거

    int GetItemCount(EItemID itemID) const;             // 아이템 개수 반환
    int GetMaxAddableItemCount(EItemID itemID) const;   // 아이템 추가 가능한 최대 개수 반환
    std::map<EItemID, int> GetConsumableItems() const;  // 사용 가능한 아이템 및 개수 반환

    void ShowInventory() const;  // 인벤토리 출력

    void SortInventory(EInventorySortKey sortKey, bool reverse = false);  // 인벤토리 정렬
};
