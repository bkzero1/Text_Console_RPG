#include "Tank.h"

Tank::Tank(const string& playerName)
    : Player(playerName)
{
}

void Tank::LevelUp()
{
    Player::LevelUp();  // 부모 클래스의 LevelUp() 호출
    hpMax += 20 ;
    hp = hpMax;
}