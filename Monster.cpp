#include "Monster.h"
#include "Item.h"

#include <iostream>
#include <vector>
#include <random>
#include <iomanip>
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
    hp = playerLevel * GetRandomInt(monsterData.minHpMulti, monsterData.maxHpMulti) ;
    
    power = playerLevel * GetRandomInt(monsterData.minPowerMulti, monsterData.maxPowerMulti);
    
    gold = GetRandomInt(monsterData.minGold, monsterData.maxGold);
    exp = GetRandomInt(monsterData.minExp, monsterData.maxExp);
    
    // 몬스터 강함에 따른 기본 수치 보정
    hp += static_cast<int>(eMonsterID) * monsterData.minHpMulti;
    power += static_cast<int>(eMonsterID) * monsterData.minPowerMulti;
    gold += static_cast<int>(eMonsterID) * 20;
    exp += static_cast<int>(eMonsterID) * 10;

    dropTable = monsterData.dropTable;

    if (IsBoss)
    {
        hp = static_cast<int>(hp * 1.5);
        power = static_cast<int>(power * 1.5);
        gold = static_cast<int>(gold * 1.5);
        exp = static_cast<int>(exp * 1.5);
    }
    // 배치가 끝난 시점의 체력을 최대 체력으로 기록합니다.
    hpMax = hp;
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

std::vector<EItemID> Monster::GetDropItems()
{
    std::vector<EItemID> dropItems;
    if (GetRandomInt(1, 100) > 30)
    {
        return dropItems;
    }

    for (const FDropData& dropData : dropTable)
    {
        if (GetRandomInt(1, 100) <= dropData.dropChance)
        {
            dropItems.push_back(dropData.itemID);
        }
    }
    return dropItems;
}

void TestMonster(int playerLevel)
{
    std::cout << "playerLevel: " << playerLevel << std::endl;

    Monster monster = Monster();
    
    std::cout << "\n[ 일반몬스터 스텟 ]\n";
    // 일반몹
    for (const auto& [id, monsterData] : MONSTER_MAP)
    {
        // 몬스터 정보 출력
        std::cout << monster.Deploy(id, playerLevel);
        if (id == EMonsterID::WANDERING_ARMOR || id == EMonsterID::RED_DRAGON)
        {
            std::cout << "\t";
        }
        else
        {
            std::cout << "\t\t";
        }
        std::cout << "exp: " << std::setw(3) << monster.GetExp() << "  gold: " << std::setw(3) << monster.GetGold();

        // 처치시 획득 아이템 시뮬레이션
        std::cout << "  드롭아이템: ";
        std::vector<EItemID> itemId = monster.GetDropItems();

        if (itemId.empty())
        {
            std::cout << "- ";
        }
        else
        {
            for (const auto& eItemId : itemId)
            {
                std::cout << ITEM_TABLE.at(eItemId).name << " / ";
            }
        }
        std::cout << std::endl;
    }

    std::cout << "\n[ 보스몬스터 스텟 ]\n";
    // 보스
    for (const auto& [id, monsterData] : MONSTER_MAP)
    {
        // 몬스터 정보 출력
        std::cout << monster.Deploy(id, playerLevel, true);
        if (id == EMonsterID::WANDERING_ARMOR || id == EMonsterID::RED_DRAGON)
        {
            std::cout << "\t";
        }
        else
        {
            std::cout << "\t\t";
        }
        std::cout << "exp: " << std::setw(3) << monster.GetExp() << "  gold: " << std::setw(3) << monster.GetGold();

        // 처치시 획득 아이템 시뮬레이션
        std::cout << "  드롭아이템: ";
        std::vector<EItemID> itemId = monster.GetDropItems();
        
        if (itemId.empty())
        {
            std::cout << "- ";
        }
        else
        {
            for (const auto& eItemId : itemId)
            {
                std::cout << ITEM_TABLE.at(eItemId).name << " / ";
            }
        }
        std::cout << std::endl;
    }
}
