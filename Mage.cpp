#include "Mage.h"

Mage::Mage(const string& playerName)
    : Player(playerName)
{
}

void Mage::LevelUp()
{
    level++;
    hpMax += 20;
    hp = hpMax;
    power += 5 + 8;
}