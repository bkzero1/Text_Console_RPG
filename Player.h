#pragma once
#include <string>
using namespace std;

//LevelUp() 메서드를 private에서 public으로 변경 + virtual로 만들기:
class Player
{
   protected:
    string name;
    int level;
    int expMax;
    int exp;
    int power;
    int hpMax;
    int hp;

   public:
    Player(const string& playerName);

    string GetName();
    int GetLevel();
    void AddExp(int exp);

    void TakeDamage(int damage);
    int GetPower();
    int GetHpMax();
    int GetHp();

    void ShowStatus();

    void HealHP(int hp);
    bool IsDead();
    bool IsFullHP();
    int GetMissingHP();

    void AddPower(int power);
    void RemovePower(int power);
    
    virtual void LevelUp();  // ← private에서 public + virtual로 변경
};