#include "Warrior.h"

Warrior::Warrior(const string& playerName)
    : Player(playerName)
{
}

void Warrior::LevelUp()
{
    Player::LevelUp();  // 부모 클래스의 LevelUp() 호출
    hpMax +=  5;
    hp = hpMax;
    power +=  3;
}