#include "InventorySlot.h"

InventorySlot::InventorySlot(EItemID id, int count)
    : id(id), count(count)
{
}

bool InventorySlot::IsEmpty() const
{
    return id == EItemID::NONE || count == 0;
}

void InventorySlot::Clear()
{
    id == EItemID::NONE;
    count == 0;
}

bool InventorySlot::COMPARE_BY_NAME(const InventorySlot &slot1, const InventorySlot &slot2)
{
    const std::string &name1 = ITEM_TABLE.at(slot1.id).name;
    const std::string &name2 = ITEM_TABLE.at(slot2.id).name;
    return name1 < name2;  // 아름 오름차순
}

bool InventorySlot::COMPARE_BY_COUNT(const InventorySlot &slot1, const InventorySlot &slot2)
{
    const int &count1 = slot1.count;
    const int &count2 = slot2.count;
    return count1 < count2;  // 개수 오름차순
}

bool InventorySlot::COMPARE_BY_PRICE(const InventorySlot &slot1, const InventorySlot &slot2)
{
    const int &price1 = ITEM_TABLE.at(slot1.id).purchasePrice;
    const int &price2 = ITEM_TABLE.at(slot2.id).purchasePrice;
    return price1 < price2;  // 가격 오름차순
}
