#include "Warrior.h"

#include <iostream>

Warrior::Warrior(const string& playerName)
    : Player(playerName)
{
}

void Warrior::LevelUp()
{
    if (level >= MAX_LEVEL)
    {
        return;
    }

    Player::LevelUp();  
    hpMax += 5;
    hp = hpMax;
    power += 3;
}