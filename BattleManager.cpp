#include "BattleManager.h"
#include "Item.h"
#include "Player.h"
#include "Monster.h"

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
            //next : 딜 기여에 따라 경험치 전달
            players[i]->AddExp(earnExp);
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
        if (!monsters[i]->IsDead())
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
        if (!players[i]->IsDead())
        {
            return false;
        }
    }

    return true;
}

void BattleManager::PlayerHitMonster(Monster* target, int damage)
{
    target->TakeDamage(damage);
    if (!target->IsDead())
    {
        return;
    }
    earnGold += target->GetGold();

    std::vector<EItemID> dropItem = target->GetDropItems();

    for (int i = 0; i < dropItem.size(); i++)
    {
        EItemID item = dropItem[i];
        auto itr = earnItems.find(item);
        if (itr == earnItems.end())
        {
            earnItems.insert({item, 1});
        }
        else
        {
            itr->second++;
        }
    }
}

void BattleManager::MonsterHitPlayer(Player* target, int damage)
{
    target->TakeDamage(damage);
}

std::vector<Player*> BattleManager::GetPlayers()
{
    return players;
}

std::vector<Monster*> BattleManager::GetMonsters()
{
    return monsters;
}

int BattleManager::GetEarnGold() const
{
    return earnGold;
}

std::map<EItemID, int> BattleManager::GetEarnItems() const
{
    return earnItems;
}
