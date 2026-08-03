#pragma once

#include "Item.h"

struct InventorySlot
{
    EItemID id;  // 아이템 ID
    int count;   // 아이템 개수

    InventorySlot(EItemID id, int count = 1);

    bool IsEmpty() const;  // 빈 슬롯 여부 반환
    void Clear();          // 슬롯 클리어

    static bool COMPARE_BY_NAME(const InventorySlot& slot1, const InventorySlot& slot2);   // 이름
    static bool COMPARE_BY_COUNT(const InventorySlot& slot1, const InventorySlot& slot2);  // 개수
    static bool COMPARE_BY_PRICE(const InventorySlot& slot1, const InventorySlot& slot2);  // 가격
};
