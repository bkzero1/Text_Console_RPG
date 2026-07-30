#include "Monster.h"
#include "RpgLogger.h"

#include <iostream>
#include <vector>
#include <random>
namespace
{
int GetRandomInt(int min, int max)
{
    static std::random_device randomDevice;
    static std::mt19937 randomEngine(randomDevice());

    std::uniform_int_distribution<int> distribution(min, max);

    return distribution(randomEngine);
}
}  // namespace

std::string Monster::Deploy(const EMonsterID& eMonsterID, int playerLevel, bool IsBoss)
{
    // MONSTER_MAP에서 eMonsterID에 맞는 설계도를 찾음
    const FMonsterData& monsterData = MONSTER_MAP.at(eMonsterID);

    // 찾은 설계도의 id와 name을
    // Monster 멤버 변수 id, name에 저장
    id = monsterData.id;
    name = monsterData.name;

    // 설계도의 HP 범위와 playerLevel을 사용해 이번 몬스터의 스탯을 랜덤으로 결정
    hp = playerLevel * GetRandomInt(monsterData.minHpMulti, monsterData.maxHpMulti);
    
    power = playerLevel * GetRandomInt(monsterData.minPowerMulti, monsterData.maxPowerMulti);
    
    gold = GetRandomInt(monsterData.minGold, monsterData.maxGold);
    exp = GetRandomInt(monsterData.minExp, monsterData.maxExp);

    dropTable = monsterData.dropTable;

    if (IsBoss)
    {
        hp *= 1.5;
        power *= 1.5;
        gold *= 1.5;
        exp *= 1.5;
    }
    // 몬스터 Goblin 등장 !체력 : 40, 공격력 : 8
    std::string nanori = "몬스터 " + name + " 등장! 체력 : " + std::to_string(hp) + ", 공격력 : " + std::to_string(power);

    // 등장 이름을 반환
    return nanori;  
}

void Monster::ShowStatus() const
{
    std::cout << name << " 등장!" << std::endl;
    std::cout << "[ 몬스터 스탯 ] " << std::endl
              //<< "ID: " << static_cast<int>(id) << " " << std::endl
              << "이름: " << name << std::endl
              << "체력: " << hp
              << " | 공격력: " << power << std::endl;
}

void Monster::TakeDamage(int damage)
{
    hp -= damage;

    if (hp <= 0)
    {
        hp = 0;
    }
}

void Monster::RollDrops()
{
    dropItems.clear();
    
    // 아이템 드롭 자체가 발생하는지
    if (GetRandomInt(1, 100) > 30)
    {
        return;
    }

    for (const FDropData& dropData : dropTable)
    {
        if (GetRandomInt(1, 100) <= dropData.dropChance)
        {
            dropItems.push_back(dropData.itemID);
        }
    }
}

std::vector<EItemID> Monster::GetDropItems()
{
    RollDrops();
    return dropItems;
}