#include "Tank.h"

Tank::Tank(const string& playerName)
    : Player(playerName)
{
}

void Tank::LevelUp()
{
  
    hpMax += 20 ;
    hp = hpMax;
}