#pragma once

#include "Item.h"

struct InventorySlot
{
    EItemID id;  // 아이템 ID
    int count;   // 아이템 개수

    InventorySlot(EItemID id, int count = 1);

    bool IsEmpty() const;
};