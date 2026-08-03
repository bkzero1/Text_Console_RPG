#pragma once
#include "FMonsterData.h"

#include <string>
#include <vector>

class Monster
{
   public:
	Monster()
		: hp(0),
        hpMax(0),
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
    int GetHpMax() const { return hpMax; }
    int GetGold() const { return gold; }
    int GetExp() const { return exp; }
    EMonsterID GetMonsterId() const { return id; }


   private:
	EMonsterID id = EMonsterID::NONE;
	std::string name;

	int hp;
	// 생성 당시의 최대 체력입니다. AA 체력 바의 백분율과 감소 애니메이션에 사용합니다.
    int hpMax;
	int power;

	int gold;
    int exp;

	std::vector<FDropData> dropTable;
};

void TestMonster(int playerLevel);
