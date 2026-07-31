#include "Warrior.h"

Warrior::Warrior(const string& playerName)
    : Player(playerName)
{
}

void Warrior::LevelUp()
{
    hpMax +=  5;
    hp = hpMax;
    power +=  3;
}