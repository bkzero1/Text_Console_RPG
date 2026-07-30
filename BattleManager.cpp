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

    //nullPtr인 곳(몬스터가 죽어서 빈 공간인 곳)이 있으면 거기에 할당
    for (int i = 0; i < monsters.size(); i++)
    {
        if (!monsters[i])
        {
            monsters[i] = monster;
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
        if (monsters[i] && !monsters[i]->IsDead())
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

    for (int i = 0; i < monsters.size(); i++)
    {
        if (monsters[i] != target)
        {
            continue;
        }

        monsters[i] = nullptr;
        break;
    }
}

void BattleManager::MonsterHitPlayer(Player* target, int damage)
{
    target->TakeDamage(damage);
}

std::vector<Player*> BattleManager::GetLivingPlayers() const
{
    vector<Player*> livingPlayers;
    for (int i = 0; i < players.size(); i++)
    {
        if (players[i]->IsDead())
        {
            continue;
        }
        livingPlayers.push_back(players[i]);
    }
    return livingPlayers;
}

std::vector<Monster*> BattleManager::GetLivingMonsters() const
{
    vector<Monster*> livingMonsters;
    for (int i = 0; i < monsters.size(); i++)
    {
        if (!monsters[i] || monsters[i]->IsDead())
        {
            continue;
        }
        livingMonsters.push_back(monsters[i]);
    }
    return livingMonsters;
}

int BattleManager::GetEarnGold() const
{
    return earnGold;
}

int BattleManager::GetEarnExp() const
{
    return earnExp;
}

std::map<EItemID, int> BattleManager::GetEarnItems() const
{
    return earnItems;
}
