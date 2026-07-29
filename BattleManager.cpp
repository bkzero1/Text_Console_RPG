#include "BattleManager.h"
#include "Item.h"

void BattleManager::AddPlayer(Player* player)
{
    for (int i = 0; i < players.size(); i++)
    {
        if (players[i] == player)
        {
            //중복 플레이어 할당 방지
            return;
        }
        
    }
    players.push_back(player);
}

void BattleManager::AddMonster(Monster* monster)
{
    for (int i = 0; i < monsters.size(); i++)
    {
        if (monsters[i] == monster)
        {
            //중복 몬스터 할당 방지
            return;
        }
    }
    monsters.push_back(monster);
}

void BattleManager::BattleEnd(bool isWin)
{
    if (isWin)
    {
        for (int i = 0; i < players.size(); i++)
        {
            //ToDo : 경험치 전달
            players[i];
        }
    }
    players.clear();
    monsters.clear();
    dealPies.clear();
    earnGold = 0;
    earnExp = 0;
}

bool BattleManager::IsMonstersDead() const
{
    for (int i = 0; i < monsters.size(); i++)
    {
        //ToDo : IsDead로 변경
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
        //ToDo : IsDead로 변경
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

int BattleManager::GetEarnGold() const
{
    return earnGold;
}

std::map<EItemID, int> BattleManager::GetEarnItems() const
{
    return earnItems;
}
