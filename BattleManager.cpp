#include "BattleManager.h"

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
}
