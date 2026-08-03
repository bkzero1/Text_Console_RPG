// GameClearScreen.cpp
#include "GameClearScreen.h"
#include "AsciiArt/AsciiBattleDemo.h"

#include <iostream>
#include <windows.h>

namespace
{
    void WaitForAnyKey()
    {
        const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
        DWORD originalInputMode = 0;
        const bool canRestoreInputMode = GetConsoleMode(input, &originalInputMode) != FALSE;
        if (canRestoreInputMode)
        {
            SetConsoleMode(input, (originalInputMode | ENABLE_EXTENDED_FLAGS) & ~ENABLE_QUICK_EDIT_MODE);
        }

        while (true)
        {
            INPUT_RECORD record{};
            DWORD read = 0;
            if (ReadConsoleInputW(input, &record, 1, &read) &&
                record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
            {
                break;
            }
        }

        if (canRestoreInputMode)
        {
            SetConsoleMode(input, originalInputMode);
        }
    }
}

void GameClearScreen::Show()
{
    constexpr const wchar_t* kVictoryBackground = L"Resources\\Images\\game_clear_victory.png";

    AsciiArt::ClearScreen();
    if (!AsciiArt::RenderSavedMainMenuImage(kVictoryBackground, true))
    {
        std::cout << "게임 클리어!\n아무 키나 누르면 종료합니다.";
        WaitForAnyKey();
        return;
    }

    // 배경은 현재 마을 이미지 그대로 사용하고, 나중에는 여기만 전용 엔딩 일러스트로 교체하면 됩니다.
    AsciiArt::DrawCenteredTextOnClearPanel(L"GAME CLEAR", 0.43f);
    AsciiArt::DrawCenteredTextOnClearPanel(L"모험을 완수했습니다.", 0.50f);
    AsciiArt::DrawCenteredTextOnClearPanel(L"아무 키나 누르면 종료합니다.", 0.68f);
    WaitForAnyKey();
}
