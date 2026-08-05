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

    string GetName() const;
    int GetLevel() const;
    int GetExp() const;
    int GetExpMax() const;
    void AddExp(int exp);

    void TakeDamage(int damage);
    int GetPower() const;
    int GetHpMax() const;
    int GetHp() const;

    void ShowStatus() const;

    void HealHP(int hp);
    bool IsDead() const;
    bool IsFullHP() const;
    int GetMissingHP() const;

    void AddPower(int power);
    void RemovePower(int power);

    virtual void LevelUp();
};
