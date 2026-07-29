#pragma once

#include <vector>

class EnemyPool;
class Player;
class Enemy;

class BattleManager
{
public:
	BattleManager() {};
	BattleManager(const BattleManager&) = delete;

	void AddPlayer(Player* player);
	void AddEnemy(Enemy* enemy);
	void BattleEnd();

private:
	std::vector<Player*> players;
	std::vector<Enemy*> enemies;
};

