// TextRPGSource/ShopScreen.cpp
#include "ShopScreen.h"
#include "ShopLayout.h"
#include "AsciiArt/SceneConfig.h"

#include "AsciiArt/AsciiBattleDemo.h"

#include <algorithm>
#include <chrono>
#include <conio.h>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>

#define NOMINMAX
#include <windows.h>

namespace
{
    int GetMenuDisplayWidth(const std::wstring& text)
    {
        int width = 0;
        for (const wchar_t character : text)
        {
            const bool isKorean = character >= 0xAC00 && character <= 0xD7A3;
            width += isKorean ? 2 : 1;
        }
        return width;
    }

    void DrawMenuLine(
        const std::wstring& text,
        float verticalRatio,
        ShopScreen::EMenu selectedMenu,
        ShopScreen::EMenu thisMenu)
    {
        // 가운데 정렬을 유지하면서도 세 메뉴의 시작 열을 맞춥니다.
        constexpr int kMenuWidth = 14;
        std::wstring alignedText = text;
        alignedText.append((std::max)(0, kMenuWidth - GetMenuDisplayWidth(alignedText)), L' ');

        AsciiArt::DrawStaticImageText(
            alignedText,
            0.5f,
            verticalRatio,
            selectedMenu == thisMenu,
            4);
    }
    bool RenderWithLayout(const ShopScreen::FMenuText& menuText, int gold, const ShopLayout& layout)
    {
        const bool didRenderScene = AsciiArt::RenderLayeredStaticImage(
            L"Resources\\Images\\shop_background.png", L"Resources\\Images\\shopkeeper.png", L"",
            layout.shopkeeperX, layout.shopkeeperY, layout.shopkeeperWidth, layout.shopkeeperHeight,
            layout.outputPixelWidth, layout.characterHeightScale, layout.contrast);
        if (!didRenderScene) return false;
        AsciiArt::DrawStaticImageText(menuText.title, 0.16f, 0.08f);
        AsciiArt::DrawStaticImageText(L"\uBCF4\uC720 \uACE8\uB4DC: " + std::to_wstring(gold) + L" G", 0.84f, 0.08f, true);
        AsciiArt::MoveCursorBelowStaticImage(1);
        return true;
    }
}

bool ShopScreen::Render(const FMenuText& menuText, int gold, EMenu selectedMenu)
{
    const ShopLayout layout = LoadShopLayout();
    if (!RenderWithLayout(menuText, gold, layout))
    {
        return false;
    }

    AsciiArt::DrawStaticImageText(menuText.title, 0.16f, 0.08f);
    AsciiArt::DrawStaticImageText(
        L"\uBCF4\uC720 \uACE8\uB4DC: " + std::to_wstring(gold) + L" G",
        0.84f,
        0.08f,
        true);

    DrawMenuLine(menuText.buy, 0.74f, selectedMenu, EMenu::BUY);
    DrawMenuLine(menuText.sell, 0.80f, selectedMenu, EMenu::SELL);
    DrawMenuLine(menuText.back, 0.86f, selectedMenu, EMenu::BACK);

    AsciiArt::MoveCursorBelowStaticImage(1);
    return true;
}

int ShopScreen::RunAnimatedMenu(const FMenuText& menuText, int gold)
{
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD originalInputMode = 0;
    GetConsoleMode(input, &originalInputMode);
    SetConsoleMode(input, (originalInputMode | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) & ~ENABLE_QUICK_EDIT_MODE);
    FlushConsoleInputBuffer(input);
    const auto startedAt = std::chrono::steady_clock::now();

    while (true)
    {
        ShopLayout layout = LoadShopLayout();
        const float elapsedSeconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - startedAt).count();
        layout.shopkeeperY += std::sin(elapsedSeconds * 0.8f) * 3.0f;
        layout.shopkeeperX += std::sin(elapsedSeconds * 0.55f) * 1.0f;
        if (!RenderWithLayout(menuText, gold, layout)) return 0;
        DrawMenuLine(menuText.buy, 0.74f, EMenu::BUY, EMenu::BUY);
        DrawMenuLine(menuText.sell, 0.80f, EMenu::BUY, EMenu::SELL);
        DrawMenuLine(menuText.back, 0.86f, EMenu::BUY, EMenu::BACK);

        DWORD pendingCount = 0;
        if (GetNumberOfConsoleInputEvents(input, &pendingCount) && pendingCount > 0)
        {
            INPUT_RECORD record;
            DWORD read = 0;
            ReadConsoleInputW(input, &record, 1, &read);
            if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
            {
                const WORD key = record.Event.KeyEvent.wVirtualKeyCode;
                if (key == 'P') { RunLayoutEditor(menuText, gold); continue; }

                // 키보드 윗줄 숫자와 NumLock 숫자패드는 Windows에서 다른 키 코드로 들어옵니다.
                int selectedMenu = -1;
                if (key == '0' || key == VK_NUMPAD0) selectedMenu = 0;
                if (key == '1' || key == VK_NUMPAD1) selectedMenu = 1;
                if (key == '2' || key == VK_NUMPAD2) selectedMenu = 2;

                if (selectedMenu >= 0)
                {
                    SetConsoleMode(input, originalInputMode);
                    return selectedMenu;
                }
            }
        }

        const SceneConfig globalConfig = LoadSceneConfig();
        const int frameDelayMilliseconds = 1000 / std::clamp(globalConfig.framesPerSecond, 1, 240);
        std::this_thread::sleep_for(std::chrono::milliseconds(frameDelayMilliseconds));
    }
}

void ShopScreen::RunLayoutEditor(const FMenuText& menuText, int gold)
{
    ShopLayout layout = LoadShopLayout();
    const ShopLayout beforeEditing = layout;
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD originalInputMode = 0;
    GetConsoleMode(input, &originalInputMode);
    SetConsoleMode(input, (originalInputMode | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) & ~ENABLE_QUICK_EDIT_MODE);
    FlushConsoleInputBuffer(input);
    bool dragging = false;
    float grabOffsetX = 0.0f;
    float grabOffsetY = 0.0f;
    const auto startedAt = std::chrono::steady_clock::now();

    const auto drawSlider = [output](short y, const wchar_t* label, int value)
    {
        constexpr int kTrackWidth = 30;
        const int filled = value * kTrackWidth / 1000;
        std::wstring line = std::wstring(label) + L" [";
        line += std::wstring(filled, L'=');
        line += L'|';
        line += std::wstring(kTrackWidth - filled, L'-');
        line += L"] " + std::to_wstring(value);
        SetConsoleCursorPosition(output, {2, y});
        DWORD written = 0;
        WriteConsoleW(output, line.c_str(), static_cast<DWORD>(line.size()), &written, nullptr);
    };

    while (true)
    {
        ShopLayout animatedLayout = layout;
        const float elapsedSeconds = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - startedAt).count();
        animatedLayout.shopkeeperY += std::sin(elapsedSeconds * 0.8f) * 3.0f;
        animatedLayout.shopkeeperX += std::sin(elapsedSeconds * 0.55f) * 1.0f;
        if (!RenderWithLayout(menuText, gold, animatedLayout)) return;
        drawSlider(0, L"\uD574\uC0C1\uB3C4", layout.outputPixelWidth);
        drawSlider(1, L"\uC138\uB85C \uBE44\uC728", layout.characterHeightScale);
        drawSlider(2, L"\uB300\uBE44", layout.contrast);
        std::cout << "[\uBC30\uCE58 \uBAA8\uB4DC] NPC \uB4DC\uB798\uADF8 / +/-: \uD06C\uAE30 / S: \uC800\uC7A5 / X: \uCDE8\uC18C";

        INPUT_RECORD record;
        DWORD read = 0;
        DWORD pendingCount = 0;
        if (!GetNumberOfConsoleInputEvents(input, &pendingCount) || pendingCount == 0)
        {
            const SceneConfig globalConfig = LoadSceneConfig();
            const int frameDelayMilliseconds = 1000 / std::clamp(globalConfig.framesPerSecond, 1, 240);
            std::this_thread::sleep_for(std::chrono::milliseconds(frameDelayMilliseconds));
            continue;
        }
        ReadConsoleInputW(input, &record, 1, &read);
        // 드래그 중 쌓인 이동 이벤트는 마지막 좌표만 사용합니다.
        // 모든 중간 좌표를 각각 다시 그리면 콘솔 출력이 밀리면서 끊겨 보입니다.
        if (record.EventType == MOUSE_EVENT)
        {
            DWORD pendingCount = 0;
            while (GetNumberOfConsoleInputEvents(input, &pendingCount) && pendingCount > 0)
            {
                INPUT_RECORD pendingRecord;
                DWORD pendingRead = 0;
                ReadConsoleInputW(input, &pendingRecord, 1, &pendingRead);
                if (pendingRecord.EventType == MOUSE_EVENT)
                {
                    record = pendingRecord;
                }
            }
        }
        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
        {
            const WORD key = record.Event.KeyEvent.wVirtualKeyCode;
            if (key == 'S') { SaveShopLayout(layout); SetConsoleMode(input, originalInputMode); return; }
            if (key == 'X') { SaveShopLayout(beforeEditing); SetConsoleMode(input, originalInputMode); return; }
            if (key == VK_OEM_PLUS || key == VK_ADD) { layout.shopkeeperWidth *= 1.10f; layout.shopkeeperHeight *= 1.10f; }
            if (key == VK_OEM_MINUS || key == VK_SUBTRACT) { layout.shopkeeperWidth *= 0.90f; layout.shopkeeperHeight *= 0.90f; }
            continue;
        }
        if (record.EventType != MOUSE_EVENT) continue;

        const MOUSE_EVENT_RECORD& mouse = record.Event.MouseEvent;
        const bool pressed = (mouse.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
        if (pressed && mouse.dwMousePosition.Y >= 0 && mouse.dwMousePosition.Y <= 2 &&
            mouse.dwMousePosition.X >= 12 && mouse.dwMousePosition.X <= 43)
        {
            const int value = std::clamp((mouse.dwMousePosition.X - 13) * 1000 / 30, 0, 1000);
            if (mouse.dwMousePosition.Y == 0) layout.outputPixelWidth = value;
            else if (mouse.dwMousePosition.Y == 1) layout.characterHeightScale = value;
            else layout.contrast = value;
            continue;
        }
        short left, top, width, height;
        if (!AsciiArt::GetStaticImageBounds(left, top, width, height)) continue;
        const float sceneX = (mouse.dwMousePosition.X - left) * 1920.0f / width;
        const float sceneY = (mouse.dwMousePosition.Y - top) * 1080.0f / height;
        const bool isInsideNpc = sceneX >= layout.shopkeeperX && sceneX <= layout.shopkeeperX + layout.shopkeeperWidth &&
            sceneY >= layout.shopkeeperY && sceneY <= layout.shopkeeperY + layout.shopkeeperHeight;
        if (pressed && mouse.dwEventFlags == 0 && isInsideNpc)
        {
            dragging = true;
            grabOffsetX = sceneX - layout.shopkeeperX;
            grabOffsetY = sceneY - layout.shopkeeperY;
        }
        else if (pressed && mouse.dwEventFlags == MOUSE_MOVED && dragging)
        {
            layout.shopkeeperX = sceneX - grabOffsetX;
            layout.shopkeeperY = sceneY - grabOffsetY;
        }
        else if (!pressed) dragging = false;
    }
}
