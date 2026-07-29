#include "BattleManager.h"

void BattleManager::AddPlayer(Player* player)
{
	for (int i = 0; i < players.size(); i++) {
		if (players[i]) {
			continue;
		}
		players[i] = player;
		return;
	}
	players.push_back(player);
}

void BattleManager::AddEnemy(Enemy* enemy)
{
	for (int i = 0; i < enemies.size(); i++) {
		if (enemies[i]) {
			continue;
		}
		enemies[i] = enemy;
		return;
	}
	enemies.push_back(enemy);
}

void BattleManager::BattleEnd()
{
	players.clear();
	enemies.clear();
}
