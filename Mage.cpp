#include "Mage.h"

Mage::Mage(const string& playerName)
    : Player(playerName)
{
}

void Mage::LevelUp()
{
    Player::LevelUp();  // 부모 클래스의 LevelUp() 호출
    hp = hpMax;
    power += 8;
}