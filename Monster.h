#pragma once
#include "FMonsterData.h"

#include <string>
#include <vector>

class Monster
{
   public:
	Monster()
		: hp(0), 
		power(0), 
		gold(0), 
		exp(0)
	{

	}

	std::string Deploy(const FMonsterData& monsterData, int playerLevel);
    void ShowStatus() const;
    
	void TakeDamage(int damage);
	int GetPower() const;
    bool IsDead() const;

    const std::vector<EItemID>& GetDropItems() const;	// 랜덤으로 반환

	const std::string& GetName() const;
	int GetGold() const;
    int GetExp() const;


   private:
	EMonsterID id = EMonsterID::NONE;
	std::string name;

	int hp;
    int power;

	int gold;
    int exp;

	std::vector<EItemID> dropItems;

};
