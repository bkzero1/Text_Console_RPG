#include "Tank.h"

Tank::Tank(const string& playerName)
    : Player(playerName)
{
}

void Tank::LevelUp()
{
 
    Player::LevelUp();
    
  
    hpMax += 20;
    hp = hpMax;
    
    PrintLevelUpMessage();
}
