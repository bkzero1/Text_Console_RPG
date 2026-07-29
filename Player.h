#pragma once
#include <string>
using namespace std;

class Player
{
   private:
    string name;
    int level;
    int expMax;
    int exp;
    int power;
    int hpMax;
    int hp;

   public:
    Player(const string& playerName);

    string GetName();      // 이름 얻기
    int GetLevel();        // 레벨 얻기
    void AddExp(int exp);  // 경험치 추가

    void TakeDamage(int damage);  // 피격
    int GetPower();

    void ShowStatus();  // 정보 출력

    void HealHP(int hp);  // 체력 회복
    bool IsDead();
    bool IsFullHP();     // 최대체력 여부
    int GetMissingHP();  // 부족한 체력 값

    void AddPower(int power);     // 공격력 증가 (아이템 버프, 파티 레벨업 등에 사용)
    void RemovePower(int power);  // 공격력 되돌리기 (버프 종료 시 아이템 쪽에서 호출)

   private:
    void LevelUp();
};