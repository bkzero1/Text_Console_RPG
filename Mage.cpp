#include "Mage.h"

Mage::Mage(const string& playerName)
    : Player(playerName)
{
}

void Mage::LevelUp()
{
 
    hp = hpMax;
    power += 8;
}