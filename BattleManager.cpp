#include "BattleManager.h"
#include "Item.h"

void BattleManager::AddPlayer(Player* player)
{
    for (int i = 0; i < players.size(); i++)
    {
        if (players[i])
        {
            continue;
        }
        players[i] = player;
        return;
    }
    players.push_back(player);
}

void BattleManager::AddMonster(Monster* enemy)
{
    for (int i = 0; i < monsters.size(); i++)
    {
        if (monsters[i])
        {
            continue;
        }
        monsters[i] = enemy;
        return;
    }
    monsters.push_back(enemy);
}

void BattleManager::BattleEnd()
{
    players.clear();
    monsters.clear();
    earnGold = 0;
    earnExp = 0;
}

bool BattleManager::IsMonstersDead() const
{
    for (int i = 0; i < monsters.size(); i++)
    {
        if (monsters[i])
        {
            return false;
        }
    }

    return true;
}

bool BattleManager::IsPlayersDead() const
{
    for (int i = 0; i < players.size(); i++)
    {
        if (players[i])
        {
            return false;
        }
    }

    return true;
}

void BattleManager::PlayerHitMonster(Monster* target, int damage)
{
    //1. 데미지 준다
    //2. 몬스터 사망 확인
    //3. 몬스터의 드랍 아이템과 골드를 적립한다
    //4. 킬 카운트를 높인다.
}

void BattleManager::MonsterHitPlayer(Player* target, int damage)
{
    //1. 데미지를 준다
}
