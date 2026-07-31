#pragma once

#include <map>
#include <string>

enum class EItemID
{
    NONE,
    HP_POTION,
    POWER_POTION,

    //============ 일반몹 ============
    // 슬라임
    SLIME_JELLY,
    SLIME_CORE,
    SLIME_CLEAR_WATER_DROP,

    // 고블린
    GOBLIN_EAR,
    GOBLIN_DAGGER_FRAGMENT,
    GOBLIN_COIN_POUCH,

    // 스켈레톤
    SKELETON_BONE,
    SKELETON_RUSTED_BUCKLE,
    SKELETON_CURSED_TOOTH,

    // 좀비
    ZOMBIE_ROTTEN_FLESH,
    ZOMBIE_GRAVE_CLOTH,
    ZOMBIE_PLAGUE_FANG,

    //============ 정 예 =============
    // 코볼트
    KOBOLD_FANG,
    KOBOLD_CLAW,
    KOBOLD_LEATHER,
    KOBOLD_TRIBAL_PENDANT,

    // 골렘
    GOLEM_STONE_FRAGMENT,
    GOLEM_MAGIC_CORE,
    GOLEM_IRON_ARM,
    GOLEM_GEMSTONE,

    //============ 중보스 ============
    // 방황하는 갑옷
    WANDERING_ARMOR_CRACKED_ARMOR_PLATE,
    WANDERING_ARMOR_CURSED_HELM_FRAGMENT,
    WANDERING_ARMOR_RUSTED_SWORD_FRAGMENT,
    WANDERING_ARMOR_NOBLE_CREST,
    WANDERING_ARMOR_SOUL_STEEL,

    // 드라큘라
    DRACULA_BAT_WING,
    DRACULA_BLOOD_VIAL,
    DRACULA_VAMPIRE_FANG,
    DRACULA_DARK_CAPE_CLOTH,
    DRACULA_MOONSTONE,

    //============ 보스급 ============
    //레드 드래곤
    RED_DRAGON_SCALE,
    RED_DRAGON_CLAW,
    RED_DRAGON_FANG,
    RED_DRAGON_HORN,
    RED_DRAGON_DRAGON_WING_MEMBRANE,
    RED_DRAGON_FLAME_ORB,
    RED_DRAGON_DRAGON_HEART
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