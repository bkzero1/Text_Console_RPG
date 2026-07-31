#include "Tank.h"

Tank::Tank(const string& playerName)
    : Player(playerName)
{
}

void Tank::LevelUp()
{
    level++;
    hpMax += 20 + 10;
    hp = hpMax;
    power += 5;
}