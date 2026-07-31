#include "Item.h"

ItemData::ItemData(EItemID id, std::string name, std::string description, int purchasePrice, bool isConsumable, EItemEffectType effectType, int effectValue)
    : id(id), name(name), description(description), purchasePrice(purchasePrice), isConsumable(isConsumable), effectType(effectType), effectValue(effectValue)
{
}

const std::map<EItemID, ItemData> ITEM_TABLE = {
    {EItemID::HP_POTION, ItemData(EItemID::HP_POTION,
                                  "HP 포션",
                                  "플레이어의 체력을 회복한다.",
                                  30,
                                  true,
                                  EItemEffectType::HEAL_HP,
                                  50)},
    {EItemID::POWER_POTION, ItemData(EItemID::POWER_POTION,
                                     "공격력 증가 포션", 
                                     "플레이어의 공격력을 이번 전투 동안만 증가시킨다.",
                                     40,
                                     true,
                                     EItemEffectType::BUFF_POWER,
                                     10)},
};
