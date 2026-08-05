#include "ShopManager.h"

#include <iostream>
#include <iterator>
#include <sstream>

#include "Item.h"
#include "SoundManager.h"

void ShopManager::ShowBuyableList() const
{
    std::cout << "-------- 구매 가능한 아이템 --------" << std::endl;
    int count = 1;
    for (auto& item : ITEM_TABLE)
    {
        std::cout << count << ". " << item.second.name << std::endl;
        count++;
    }
}

std::vector<EItemID> ShopManager::GetBuyableItemIDs() const
{
    std::vector<EItemID> ids;
    for (auto& item : ITEM_TABLE)
    {
        ids.push_back(item.first);
    }
    return ids;
}

void ShopManager::BuyItem(const ItemData& item, int count, SoundManager& soundManager) const
{
    // 총 비용
    int totalPrice = item.purchasePrice * count;

    // 골드가 부족할 때
    if (inventory->GetGold() < totalPrice)
    {
        std::cout << "골드가 부족합니다." << std::endl;
        return;
    }

    int remainingCount = inventory->AddItem(item.id, count);

    // 인벤토리 부족
    if (remainingCount == count)
    {
        std::cout << "인벤토리가 가득 차서 구매하지 못하였습니다." << std::endl;
        return;
    }

    // 실제 구매된 수량, 금액 (10개 사려는데 슬롯이 부족할 수도 있어 9개 구매)
    int purchasedCount = count - remainingCount;
    int purchasedPrice = item.purchasePrice * purchasedCount;

    inventory->AddGold(-purchasedPrice);
    soundManager.PlaySFX(soundMap.at(SoundStates::MONEY_USE));

    // 로거 저장 및 출력
    std::ostringstream oss;
    if (remainingCount > 0)
    {
        std::cout << "인벤토리에 추가 가능한 만큼 자동 조절되어 구매합니다." << std::endl;
    }
    oss << purchasedPrice << "골드를 지불하여 " << item.name
        << "을(를) " << purchasedCount << "개 구매하였습니다. "
        << "남은 골드: " << inventory->GetGold();
    rpgLogger.AddLog(oss.str());
}

void ShopManager::ShowSellableList() const
{
    std::cout << "----- 아이템 판매 리스트 -----" << std::endl;
    // 모든 아이템과 개수 가져오기
    std::map<EItemID, int> itemList = inventory->GetItemCounts();
    if (itemList.size() == 0)
    {
        std::cout << "** 판매 가능한 아이템이 없습니다. **" << std::endl;
    }
    else
    {
        int count = 1;
        for (auto& it : itemList)
        {
            const ItemData& item = ITEM_TABLE.at(it.first);
            std::cout << count << ". " << item.name << " | 보유수량: " << it.second << "개 | 개당 판매 가격: " << static_cast<int>(item.purchasePrice * 0.6) << "골드" << std::endl;
            count++;
        }
    }
}

std::vector<EItemID> ShopManager::GetSellableItemIDs() const
{
    std::vector<EItemID> ids;
    for (auto& item : inventory->GetItemCounts())
    {
        ids.push_back(item.first);
    }
    return ids;
}

void ShopManager::SellItem(const ItemData& item, int count, SoundManager& soundManager) const
{
    // 아이템 소모(판매)
    bool isConsumed = inventory->ConsumeItem(item.id, count);
    // 선택 슬롯보다 큰 수를 입력했을 때 판매불가 메시지
    if (!isConsumed)
    {
        std::cout << "판매 수량이 보유 수량보다 많습니다." << std::endl;
        return;
    }

    // 가격의 60% * 판매개수
    int sellPrice = static_cast<int>(item.purchasePrice * 0.6) * count;

    // 총 판매 가격만큼 골드 추가
    inventory->AddGold(sellPrice);
    soundManager.PlaySFX(soundMap.at(SoundStates::MONEY_USE));

    // 로거 저장 및 출력
    std::ostringstream oss;
    oss << item.name << " " << count << "개를 판매하여 "
        << sellPrice << "골드를 획득하였습니다."
        << " 보유 골드: " << inventory->GetGold();
    rpgLogger.AddLog(oss.str());
}
