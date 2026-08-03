#include "Inventory.h"

#include <algorithm>
#include <iostream>

void Inventory::ClearEmptySlots()
{
    // 슬롯 순회하며 빈 슬롯 제거
    for (auto iter = slots.begin(); iter != slots.end();)
    {
        // 빈 슬롯 O -> 제거
        if (iter->IsEmpty())
        {
            iter = slots.erase(iter);
        }
        // 빈 슬롯 X -> 다음 슬롯 확인
        else
        {
            iter++;
        }
    }

    // itemCounts에서 값이 0인 아이템 제거
    for (auto iter = itemCounts.begin(); iter != itemCounts.end();)
    {
        // 빈 슬롯 O -> 제거
        if (iter->second == 0)
        {
            iter = itemCounts.erase(iter);
        }
        // 빈 슬롯 X -> 다음 슬롯 확인
        else
        {
            iter++;
        }
    }
}

void Inventory::CompactInventory()
{
    // 앞 슬롯부터 채우기
    for (int i = 0; i < slots.size(); i++)
    {
        InventorySlot& targetSlot = slots[i];

        // 반 슬롯이면 넘어가기
        if (targetSlot.IsEmpty())
        {
            continue;
        }

        // 슬롯이 가득 찬 상태면 넘어가기
        if (targetSlot.count == slotCapacity)
        {
            continue;
        }

        // 뒤에 있는 슬롯을 검사하여 같은 아이템이면 합치기
        for (int j = i + 1; j < slots.size(); j++)
        {
            InventorySlot& sourceSlot = slots[j];

            // 아이템을 이동시킬 슬롯이 비어있지 않고 동일한 아이템인지 확인
            if (!sourceSlot.IsEmpty() && sourceSlot.id == targetSlot.id)
            {
                int marginCount = slotCapacity - targetSlot.count;        // 슬롯 최대까지 남은 개수
                int moveCount = std::min(marginCount, sourceSlot.count);  // 이동 가능한 개수

                // 아이템 이동
                targetSlot.count += moveCount;
                sourceSlot.count -= moveCount;

                // targetSlot이 가득참 -> 다음 targetSlot 설정
                if (targetSlot.count == slotCapacity)
                {
                    break;
                }
            }
        }
    }

    // 빈 슬롯 제거
    ClearEmptySlots();
}

Inventory::Inventory()
    : gold(0), maxSlots(10), slotCapacity(5)
{
}

int Inventory::GetGold() const
{
    return gold;
}

void Inventory::AddGold(int gold)
{
    this->gold += gold;
}

int Inventory::AddItem(EItemID itemID, int count)
{
    // 아이템 넣을 자리 없음
    if (maxSlots == 0 || slotCapacity == 0)
    {
        return count;
    }

    // 슬롯 순회하여 추가
    for (InventorySlot& slot : slots)
    {
        // 동일 아이템 & 공간 여유 -> 아이템 추가 성공
        if (slot.id == itemID && slot.count < slotCapacity)
        {
            int marginCount = slotCapacity - slot.count;  // 슬롯 최대까지 남은 개수
            int addCount = std::min(marginCount, count);  // 추가 가능한 개수

            // 아이템 추가
            slot.count += addCount;
            count -= addCount;
            itemCounts[itemID] += addCount;

            // 전부 추가 완료
            if (count == 0)
            {
                return 0;
            }
        }
    }

    // 새로운 슬롯으로 추가
    while (slots.size() < maxSlots)
    {
        int addCount = std::min(count, slotCapacity);  // 슬롯 내 아이템 개수

        // 새 슬롯과 함께 아이템 추가
        slots.push_back(InventorySlot(itemID, addCount));
        count -= addCount;
        itemCounts[itemID] += addCount;

        // 전부 추가 완료
        if (count == 0)
        {
            return 0;
        }
    }

    // 추가하지 못하고 남은 개수 반환
    return count;
}

bool Inventory::ConsumeItem(EItemID itemID, int count)
{
    // 개수가 부족하여 소모 실패
    if (count > itemCounts[itemID])
    {
        return false;
    }

    // 슬롯 순회하여 제거
    for (InventorySlot& slot : slots)
    {
        // 동일 아이템 & 충분한 양 -> 아이템 제거
        if (slot.id == itemID && slot.count > 0)
        {
            // 아이템 제거
            int removeCount = std::min(slot.count, count);  // 현재 슬롯에서 제거 가능한 개수
            slot.count -= removeCount;
            count -= removeCount;
            itemCounts[itemID] -= removeCount;

            // 제거 완료
            if (count == 0)
            {
                break;
            }
        }
    }

    // 빈 슬롯 제거
    ClearEmptySlots();

    // 주어진 개수만큼 아이템 소모 완료
    return true;
}

std::map<EItemID, int> Inventory::AddItems(const std::map<EItemID, int>& items)
{
    std::map<EItemID, int> remainingItems;  // 추가 후 남는 아이템들

    // 추가할 아이템 순회하며 추가 및 남는 아이템 확인
    for (const auto& [itemID, count] : items)
    {
        int remainingCount = AddItem(itemID, count);  // 아이템 추가 후 남은 아이템 개수

        // 아이템이 남는 경우
        if (remainingCount > 0)
        {
            remainingItems[itemID] = remainingCount;
        }
    }

    return remainingItems;  // 남는 아이템들 반환
}

const std::vector<InventorySlot>& Inventory::GetSlots() const
{
    return slots;
}

const InventorySlot& Inventory::GetSlot(int index) const
{
    return slots.at(index);
}

void Inventory::RemoveSlot(int index)
{
    slots.at(index).Clear();
    ClearEmptySlots();  // 빈 슬롯 제거
}

void Inventory::ExpandSlotCount(int count)
{
    if (count > 0)  // 0 초과 확인
    {
        maxSlots += count;
    }
}

int Inventory::GetItemCount(EItemID itemID) const
{
    // 아이템이 없음 - 0개
    if (itemCounts.find(itemID) == itemCounts.end())
    {
        return 0;
    }

    // 아이템 개수 반환
    return itemCounts.at(itemID);
}

int Inventory::GetMaxAddableItemCount(EItemID itemID) const
{
    int count = 0;  // 추가 가능한 최대 개수

    // 빈 슬롯 확인
    count += slotCapacity * (maxSlots - static_cast<int>(slots.size()));

    // 기존 슬롯 확인
    for (const InventorySlot& slot : slots)
    {
        if (slot.id == itemID)  // 동일 아이템 확인
        {
            count += slotCapacity - slot.count;  // 여유 개수만큼 추가
        }
    }

    return count;
}

std::map<EItemID, int> Inventory::GetItemCounts() const
{
    return itemCounts;
}

std::map<EItemID, int> Inventory::GetConsumableItems() const
{
    std::map<EItemID, int> consumableItems;

    // 슬롯 순회하여 개수 확인
    for (const InventorySlot& slot : slots)
    {
        // 사용 가능한 아이템인지 확인
        if (ITEM_TABLE.at(slot.id).isConsumable)
        {
            consumableItems[slot.id] += slot.count;
        }
    }

    return consumableItems;
}

void Inventory::ShowInventory() const
{
    std::cout << "==============[ 인벤토리 ]===============" << "\n";

    // 슬롯 순회하여 차레대로 출력
    for (int i = 0; i < slots.size(); i++)
    {
        const InventorySlot& slot = slots.at(i);
        const ItemData& item = ITEM_TABLE.at(slot.id);

        std::cout << "  " << i + 1 << ". " << item.name << "      ( " << slot.count << " / " << slotCapacity << " )" << "\n";
    }
    std::cout << "----------------------------------------" << "\n";
    std::cout << "  [골드] " << gold << " G          [슬롯] " << slots.size() << " / " << maxSlots << "\n";
    std::cout << "========================================" << "\n";
}

void Inventory::SortInventory(EInventorySortKey sortKey, bool reverse)
{
    // 인벤토리 정리
    CompactInventory();

    // 주어진 기준에 따라 정렬
    switch (sortKey)
    {
        case EInventorySortKey::NAME:
            if (!reverse)
            {
                std::sort(slots.begin(), slots.end(), InventorySlot::COMPARE_BY_NAME);
            }
            else
            {
                std::sort(slots.rbegin(), slots.rend(), InventorySlot::COMPARE_BY_NAME);
            }
            break;
        case EInventorySortKey::COUNT:
            if (!reverse)
            {
                std::sort(slots.begin(), slots.end(), InventorySlot::COMPARE_BY_COUNT);
            }
            else
            {
                std::sort(slots.rbegin(), slots.rend(), InventorySlot::COMPARE_BY_COUNT);
            }
            break;
        case EInventorySortKey::PRICE:
            if (!reverse)
            {
                std::sort(slots.begin(), slots.end(), InventorySlot::COMPARE_BY_PRICE);
            }
            else
            {
                std::sort(slots.rbegin(), slots.rend(), InventorySlot::COMPARE_BY_PRICE);
            }
            break;
        default:
            break;
    }
}
