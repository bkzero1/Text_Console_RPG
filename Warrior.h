#pragma once
#include "Player.h"

class Warrior : public Player
{
   public:
    Warrior(const string& playerName);
    void LevelUp() override;
};
