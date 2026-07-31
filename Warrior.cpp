#include "Warrior.h"

Warrior::Warrior(const string& playerName)
    : Player(playerName)
{
}

void Warrior::LevelUp()
{
    level++;
    hpMax += 20 + 5;
    hp = hpMax;
    power += 5 + 3;
}