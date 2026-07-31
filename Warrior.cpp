#include "Warrior.h"

Warrior::Warrior(const string& playerName)
    : Player(playerName)
{
}

void Warrior::LevelUp()
{
   
    Player::LevelUp();

  
    hpMax += 5;
    hp = hpMax;
    power += 3;

    
    PrintLevelUpMessage();
}