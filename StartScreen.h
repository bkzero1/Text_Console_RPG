#pragma once

namespace StartScreen
{
    enum class EAction
    {
        StartGame,
        ExitGame
    };

    void Render();
    EAction HandleInput();
}