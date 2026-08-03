#pragma once
#include "Item.h"

#include <string>
#include <vector>
#include <map>
#include <set>

struct FDropData
{
    EItemID itemID;
    int dropChance;
};

enum class EMonsterID
{
    NONE,

    // 일반몹
    SLIME,
    GOBLIN,
    SKELETON,
    ZOMBIE,

    // 정예
    LV3,
    KOBOLD,
    GOLEM,

    // 중보스
    LV6,
    WANDERING_ARMOR,    
    DRACULA,

    // 보스급
    LV10,
    RED_DRAGON,
};

// 레벨 구간 경계값은 실제 몬스터가 아니므로 무작위 선택 대상에서 제외합니다.
const std::set<EMonsterID> EXCLUDE_ID = {
    EMonsterID::NONE, EMonsterID::LV3, EMonsterID::LV6, EMonsterID::LV10
};

struct FMonsterData
{
    EMonsterID id;
    std::string name;

    // 최소값만 넘기고 디플로이 함수에서 내가 변경해도 됨
    // 기본값 +@ 형태도 디플로이에서 정의하면 됨
    int minHpMulti; // 20
    int maxHpMulti; // 30

    int minPowerMulti;  // 5
    int maxPowerMulti;  // 10

    int minGold;    //10
    int maxGold;    //20

    int minExp; // 50
    int maxExp; // 100

    std::vector<FDropData> dropTable;
};

// 지금 구조에서는 플레이어 레벨이 바뀌지 않으면 뒤의 몬스터들이 같은 스텟을 가짐
const std::map<EMonsterID, FMonsterData> MONSTER_MAP = {
    {EMonsterID::SLIME, FMonsterData{
    EMonsterID::SLIME, "슬라임",
    20, 30,  // hp
    5, 10,  // power
    10, 20,  // gold
    50, 100,  // exp
    {{EItemID::HP_POTION, 80},
     {EItemID::POWER_POTION, 60},
     {EItemID::SLIME_JELLY, 100},
     {EItemID::SLIME_CORE, 45},
     {EItemID::SLIME_CLEAR_WATER_DROP, 15}}  // dropTable
    }},

    {EMonsterID::GOBLIN, FMonsterData{
    EMonsterID::GOBLIN, "고블린",
    20, 30,  // hp
    5, 10,  // power
    10, 20,  // gold
    50, 100,  // exp
    {{EItemID::HP_POTION, 80},
     {EItemID::POWER_POTION, 60},
     {EItemID::GOBLIN_EAR, 100},
     {EItemID::GOBLIN_DAGGER_FRAGMENT, 45},
     {EItemID::GOBLIN_COIN_POUCH, 15}}
    }},

    {EMonsterID::SKELETON, FMonsterData{
    EMonsterID::SKELETON, "스켈레톤",
    20, 30,  // hp
    5, 10,  // power
    10, 20,  // gold
    50, 100,  // exp
    {{EItemID::HP_POTION, 80},
     {EItemID::POWER_POTION, 60},
     {EItemID::SKELETON_BONE, 100},
     {EItemID::SKELETON_RUSTED_BUCKLE, 45},
     {EItemID::SKELETON_CURSED_TOOTH, 15}}  // dropTable
    }},

    {EMonsterID::ZOMBIE, FMonsterData{
    EMonsterID::ZOMBIE, "좀비",
    20, 30,  // hp
    5, 10,  // power
    10, 20,  // gold
    50, 100,  // exp
    {{EItemID::HP_POTION, 80},
     {EItemID::POWER_POTION, 60},
     {EItemID::ZOMBIE_ROTTEN_FLESH, 100},
     {EItemID::ZOMBIE_GRAVE_CLOTH, 45},
     {EItemID::ZOMBIE_PLAGUE_FANG, 15}}  // dropTable
    }},

    {EMonsterID::KOBOLD, FMonsterData{
    EMonsterID::KOBOLD, "코볼트",
    20, 30,  // hp
    5, 10,  // power
    10, 20,  // gold
    50, 100,  // exp
    {{EItemID::HP_POTION, 80},
     {EItemID::POWER_POTION, 60},
     {EItemID::KOBOLD_FANG, 100},
     {EItemID::KOBOLD_CLAW, 55},
     {EItemID::KOBOLD_LEATHER, 25},
     {EItemID::KOBOLD_TRIBAL_PENDANT, 10}}  // dropTable
    }},

    {EMonsterID::GOLEM, FMonsterData{
    EMonsterID::GOLEM, "골렘",
    20, 30,  // hp
    5, 10,  // power
    10, 20,  // gold
    50, 100,  // exp
    {{EItemID::HP_POTION, 80},
     {EItemID::POWER_POTION, 60},
     {EItemID::GOLEM_STONE_FRAGMENT, 100},
     {EItemID::GOLEM_MAGIC_CORE, 55},
     {EItemID::GOLEM_IRON_ARM, 25},
     {EItemID::GOLEM_GEMSTONE, 10}}  // dropTable
    }},

    {EMonsterID::WANDERING_ARMOR, FMonsterData{
    EMonsterID::WANDERING_ARMOR, "방황하는 갑옷",
    20, 30,  // hp
    5, 10,  // power
    10, 20,  // gold
    50, 100,  // exp
    {{EItemID::HP_POTION, 80},
     {EItemID::POWER_POTION, 60},
     {EItemID::WANDERING_ARMOR_CRACKED_ARMOR_PLATE, 100},
     {EItemID::WANDERING_ARMOR_CURSED_HELM_FRAGMENT, 60},
     {EItemID::WANDERING_ARMOR_RUSTED_SWORD_FRAGMENT, 30},
     {EItemID::WANDERING_ARMOR_NOBLE_CREST, 12},
     {EItemID::WANDERING_ARMOR_SOUL_STEEL, 5}}  // dropTable
    }},

    {EMonsterID::DRACULA, FMonsterData{
    EMonsterID::DRACULA, "드라큘라",
    20, 30,  // hp
    5, 10,  // power
    10, 20,  // gold
    50, 100,  // exp
    {{EItemID::HP_POTION, 80},
     {EItemID::POWER_POTION, 60},
     {EItemID::DRACULA_BAT_WING, 100},
     {EItemID::DRACULA_BLOOD_VIAL, 60},
     {EItemID::DRACULA_VAMPIRE_FANG, 30},
     {EItemID::DRACULA_DARK_CAPE_CLOTH, 12},
     {EItemID::DRACULA_MOONSTONE, 5}}  // dropTable
    }},

    {EMonsterID::RED_DRAGON, FMonsterData{
    EMonsterID::RED_DRAGON, "레드 드래곤",
    20, 30,  // hp
    5, 10,  // power
    10, 20,  // gold
    50, 100,  // exp
    {{EItemID::HP_POTION, 80},
     {EItemID::POWER_POTION, 60},
     {EItemID::RED_DRAGON_SCALE, 100},
     {EItemID::RED_DRAGON_CLAW, 65},
     {EItemID::RED_DRAGON_FANG, 35},
     {EItemID::RED_DRAGON_HORN, 18},
     {EItemID::RED_DRAGON_DRAGON_WING_MEMBRANE, 8},
     {EItemID::RED_DRAGON_FLAME_ORB, 3},
     {EItemID::RED_DRAGON_DRAGON_HEART, 2}}  // dropTable
    }}
};
