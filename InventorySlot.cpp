#include "InventorySlot.h"

InventorySlot::InventorySlot(EItemID id, int count)
    : id(id), count(count)
{
}

bool InventorySlot::IsEmpty() const
{
    return id == EItemID::NONE || count == 0;
}
