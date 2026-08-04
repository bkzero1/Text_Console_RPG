// TextRPGSource/ShopScreen.h
#pragma once

#include <string>

namespace ShopScreen
{
    struct FMenuText
    {
        std::wstring title;
        std::wstring buy;
        std::wstring sell;
        std::wstring back;
    };

    enum class EMenu
    {
        BUY,
        SELL,
        BACK,
    };

    // main.cpp가 전달한 메뉴 문구를 상점 AA 화면 위에 표시합니다.
    bool Render(const FMenuText& menuText, int gold, EMenu selectedMenu = EMenu::BUY);
    int RunAnimatedMenu(const FMenuText& menuText, int gold);
    void RunLayoutEditor(const FMenuText& menuText, int gold);
}
