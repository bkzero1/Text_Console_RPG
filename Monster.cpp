#include "Monster.h"

std::string Monster::Deploy(const FMonsterData &monsterData, int playerLevel)
{
    return std::string();
}

void Monster::ShowStatus() const
{
}

void Monster::TakeDamage(int damage)
{
}

int Monster::GetPower() const
{
    return 0;
}

bool Monster::IsDead() const
{
    return false;
}

const std::vector<EItemID> &Monster::GetDropItems() const
{
    // TODO: 여기에 return 문을 삽입합니다.
}

const std::string &Monster::GetName() const
{
    // TODO: 여기에 return 문을 삽입합니다.
}

int Monster::GetGold() const
{
    return 0;
}

int Monster::GetExp() const
{
    return 0;
}
