#pragma once

#include <map>
#include <string>

enum class EItemID
{
    NONE,
    HP_POTION,
    POWER_POTION,
};

enum class EItemEffectType
{
    NONE,
    HEAL_HP,     // 체력 회복
    BUFF_POWER,  // 공격력 증가
};

struct ItemData
{
    EItemID id;                  // 아이템 ID
    std::string name;            // 이름
    std::string description;     // 설명
    int purchasePrice;           // 가격 (판매 = 구매 * 0.6)
    bool isConsumable;           // 사용가능여부
    EItemEffectType effectType;  // 사용 아이템 타입
    int effectValue;             // 효과 값

    ItemData(EItemID id, std::string name, std::string description = "", int purchasePrice = 0, bool isConsumable = false, EItemEffectType effectType = EItemEffectType::NONE, int effectValue = 0);
};

extern const std::map<EItemID, ItemData> ITEM_TABLE;