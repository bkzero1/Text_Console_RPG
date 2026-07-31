#pragma once
#include "Player.h"

class Tank : public Player
{
   public:
    Tank(const string& playerName);
    void LevelUp() override;
};