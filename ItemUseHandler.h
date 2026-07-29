#pragma once

#include "Item.h"
#include "Player.h"

class ItemUseHandler
{
   public:
    static bool USE_ITEM(Player* player, EItemID itemID);
    static void CLEAR_BUFF(Player* player, EItemID itemID);
};
