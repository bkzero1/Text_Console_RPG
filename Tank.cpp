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

    Player::LevelUp(); 
    hpMax += 20;
    hp = hpMax;
}