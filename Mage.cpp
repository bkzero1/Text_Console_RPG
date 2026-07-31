#include "Mage.h"

#include <iostream>

Mage::Mage(const string& playerName)
    : Player(playerName)
{
}

void Mage::LevelUp()
{
    if (level >= MAX_LEVEL)
    {
        return;
    }

    Player::LevelUp();  
    hp = hpMax;
    power += 8;
}