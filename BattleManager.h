#pragma once

#include <vector>

class Player;
class Monster;

class BattleManager
{
public:
	BattleManager() {};
	BattleManager(const BattleManager&) = delete;

	void AddPlayer(Player* player);
	void AddMonster(Monster* monster);
	void BattleEnd();

private:
	std::vector<Player*> players;
	std::vector<Monster*> monsters;
};

