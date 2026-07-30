#pragma once
#include <string>
#include <vector>
#include <map>

enum class EItemID;

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
    KOBOLD,
    GOLEM,

    // 중보스
    WANDERING_ARMOR,    
    DRACULA,

    // 보스급
    RED_DRAGON
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
    {EMonsterID::GOBLIN, FMonsterData{EMonsterID::GOBLIN, "고블린", 
    20, 30, // hp
    5, 10,  // power
    10, 20, // gold
    50, 100, // exp
    {} // dropTable
    }}
};