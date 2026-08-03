#include "StartScreen.h"
#include "AsciiArt/AsciiBattleDemo.h"

#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <windows.h>

namespace
{
    bool RenderStartBackground(bool useColor)
    {
        return AsciiArt::RenderSavedStartScreenImage(
            L"Resources\\Images\\start_background_morning.png",
            useColor
        );
    }

    void RenderStartMenu()
    {
        AsciiArt::DrawCenteredTextOnClearPanel(L"1. 게임 시작", 0.65f);
        AsciiArt::DrawCenteredTextOnClearPanel(L"2. 종료", 0.72f);
    }

    void RenderStartPrompt()
    {
        AsciiArt::DrawCenteredTextOnClearPanel(L"ENTER 또는 SPACE를 누르세요", 0.65f);
    }

    // 상점과 같은 콘솔 이벤트 방식입니다.
    // 한글 입력 상태에서도 P와 숫자 키를 안정적으로 구분할 수 있습니다.
    WORD WaitForStartScreenKey()
    {
        const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
        DWORD originalInputMode = 0;
        const bool canRestoreInputMode = GetConsoleMode(input, &originalInputMode) != FALSE;
        if (canRestoreInputMode)
        {
            SetConsoleMode(input, (originalInputMode | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) & ~ENABLE_QUICK_EDIT_MODE);
        }

        while (true)
        {
            DWORD pendingCount = 0;
            if (GetNumberOfConsoleInputEvents(input, &pendingCount) && pendingCount > 0)
            {
                INPUT_RECORD record{};
                DWORD read = 0;
                ReadConsoleInputW(input, &record, 1, &read);
                if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
                {
                    if (canRestoreInputMode) SetConsoleMode(input, originalInputMode);
                    return record.Event.KeyEvent.wVirtualKeyCode;
                }
            }

            Sleep(16);
        }
    }
}

void StartScreen::Render()
{
    // 첫 장면은 흑백으로 보여 줍니다.
    const bool didRenderBackground = RenderStartBackground(false);

    if (!didRenderBackground)
    {
        std::cout << "시작 화면 배경을 불러오지 못했습니다.\n";
        return;
    }

    RenderStartPrompt();
}

StartScreen::EAction StartScreen::HandleInput()
{
    while (true)
    {
        const WORD startKey = WaitForStartScreenKey();

        // 시작 화면도 P로 점 농도와 대비를 조절할 수 있습니다.
        if (startKey == 'P')
        {
            AsciiArt::RunStaticImageTuner(L"Resources\\Images\\start_background_morning.png", false);
            Render();
            continue;
        }

        if (startKey == VK_RETURN || startKey == VK_SPACE)
        {
            break;
        }
    }

    // 컬러가 점점 길게 보이고 흑백 간격은 점점 짧아지는 전환입니다.
    // 마지막 컬러 프레임이 끝나면 메뉴를 출력합니다.
    constexpr std::array<std::pair<bool, int>, 10> kColorRevealFrames{
        std::pair{false, 1000},
        std::pair{true, 140},
        std::pair{false, 400},
        std::pair{true, 180},
        std::pair{false, 280},
        std::pair{true, 120},
        std::pair{false, 80},
        std::pair{true, 80},
        std::pair{false, 50},
        std::pair{true, 500},
    };
    for (size_t frameIndex = 0; frameIndex < kColorRevealFrames.size(); ++frameIndex)
    {
        const auto& [useColor, durationMilliseconds] = kColorRevealFrames[frameIndex];
        if (!RenderStartBackground(useColor))
        {
            return EAction::ExitGame;
        }

        const bool isLastColorFrame = frameIndex + 1 == kColorRevealFrames.size();
        if (isLastColorFrame)
        {
            // 마지막 컬러 전환과 동시에 안내 문구 대신 메뉴를 표시합니다.
            RenderStartMenu();
        }
        else
        {
            // 점멸 중에는 Enter/Space 안내 문구를 유지합니다.
            RenderStartPrompt();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMilliseconds));
    }

    while (true)
    {
        const WORD menuKey = WaitForStartScreenKey();

        // 메뉴가 나온 뒤에도 같은 시작 화면 조절 모드를 다시 열 수 있습니다.
        if (menuKey == 'P')
        {
            AsciiArt::RunStaticImageTuner(L"Resources\\Images\\start_background_morning.png", false);
            if (!RenderStartBackground(true))
            {
                return EAction::ExitGame;
            }
            RenderStartMenu();
            continue;
        }

        // 숫자줄 1/2와 오른쪽 숫자 키패드 1/2는 서로 다른 가상 키 코드입니다.
        if (menuKey == '1' || menuKey == VK_NUMPAD1)
        {
            AsciiArt::MoveCursorBelowStaticImage(2);
            return EAction::StartGame;
        }

        if (menuKey == '2' || menuKey == VK_NUMPAD2)
        {
            AsciiArt::MoveCursorBelowStaticImage(2);
            return EAction::ExitGame;
        }
    }
}
