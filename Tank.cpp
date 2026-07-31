#include "Tank.h"

#include <iostream>

Tank::Tank(const string& playerName)
    : Player(playerName)
{
}

void Tank::LevelUp()
{
    if (level >= MAX_LEVEL)
    {
        return;
    }

    Player::LevelUp();  // 부모 클래스의 LevelUp() 호출
    hpMax += 20;
    hp = hpMax;
}