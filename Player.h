#pragma once
#include <string>
using namespace std;

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

    static const int MAX_LEVEL = 10;

    void PrintLevelUpMessage();  // ← 메시지 출력 함수 추가

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

    virtual void LevelUp();
};
