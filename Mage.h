#pragma once
#include "Player.h"

class Mage : public Player
{
   public:
    Mage(const string& playerName);
    void LevelUp() override;
};