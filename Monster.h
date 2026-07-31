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

	std::string Deploy(const EMonsterID& eMonsterID, int playerLevel, bool IsBoss = false);	// 불값 기본값 false, true -> 보스
    void ShowStatus() const;
    
	void TakeDamage(int damage);
    int GetPower() const { return power; }
    bool IsDead() const { return (hp <= 0); }
    
    std::vector<EItemID> GetDropItems();	// 결정된 아이템을 반환

	const std::string& GetName() const { return name; }
    int GetHp() const { return hp; }
    int GetGold() const { return gold; }
    int GetExp() const { return exp; }


   private:
	EMonsterID id = EMonsterID::NONE;
	std::string name;

	int hp;
    int power;

	int gold;
    int exp;

	std::vector<FDropData> dropTable;
	//std::vector<EItemID> dropItems; // 실제 드롭 결과

	//void RollDrops();  //  실제 랜덤값으로 드롭 아이템을 결정
};
