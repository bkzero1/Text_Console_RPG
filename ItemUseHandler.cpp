#include "ItemUseHandler.h"

bool ItemUseHandler::USE_ITEM(Player *player, EItemID itemID)
{
    const ItemData &item = ITEM_TABLE.at(itemID);

    // 사용 불가능
    if (!item.isConsumable)
    {
        return false;
    }

    // 타입 별 적용
    switch (item.effectType)
    {
        // 체력 회복
        case EItemEffectType::HEAL_HP:
            if (player->IsFullHP())  // 이미 가득참
            {
                return false;
            }

            player->HealHP(item.effectValue);
            return true;
        // 공격력 버프
        case EItemEffectType::BUFF_POWER:
            player->AddPower(item.effectValue);
            return true;
        default:
            break;
    }

    return false;
}

void ItemUseHandler::CLEAR_BUFF(Player *player, EItemID itemID)
{
    const ItemData &item = ITEM_TABLE.at(itemID);

    // 사용 불가능
    if (!item.isConsumable)
    {
        return;
    }

    switch (item.effectType)
    {
        // 공격력 버프 제거
        case EItemEffectType::BUFF_POWER:
            player->RemovePower(item.effectValue);
            break;
        default:
            break;
    }
}

