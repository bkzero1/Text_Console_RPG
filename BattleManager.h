#pragma once

#include <vector>
#include <map>

class Player;
class Monster;
class IDamageAble;
enum class EItemID;

class BattleManager
{
public:
	BattleManager() {};
	BattleManager(const BattleManager&) = delete;

	void AddPlayer(Player* player);
	void AddMonster(Monster* monster);
	void BattleEnd();

	bool IsMonstersDead() const;
    bool IsPlayersDead() const;

	void PlayerHitMonster(Monster* target, int damage);
    void MonsterHitPlayer(Player* target, int damage);

private:
	std::vector<Player*> players;
	std::vector<Monster*> monsters;

    std::map<Player*, int> dealPies; //데미지 기여
    int earnExp = 0;
    int earnGold = 0;
    std::map<EItemID, int> earnItems;
};

