// TextRPGSource/AsciiArt/AsciiBattleBridge.h
#pragma once

#include <string>

class BattleManager;
class MonsterPool;
class RpgLogger;
class Inventory;

namespace AsciiArt
{
    // 기본 전투 객체를 AA 전투 화면에 연결하는 유일한 진입점입니다.
    // main.cpp는 이 함수만 호출하고, 상태 변환·입력·연출은 이 모듈이 맡습니다.
    bool RunBattlePresentation(BattleManager& battleManager, MonsterPool& monsterPool, RpgLogger& logger, Inventory& inventory);
    void GrantBattleTestPotions(Inventory& inventory);
    // AA 테스트가 켜진 동안에는 전투당 표시 가능한 최대 몬스터 수를 돌려줍니다.
    int GetBattleTestMonsterCount(int originalMonsterCount);
    // 다음 전투 화면 진입 시 보스 전용 연출을 한 번 재생하도록 예약합니다.
    void PrepareBossBattlePresentation();
    // 마을 복귀를 확정했을 때만 호출합니다. 귀환 연출 후 다음 전투는 다시 첫 진입 연출부터 시작합니다.
    void ReturnToTownFromBattlePresentation();

    // main.cpp가 사용하는 정적 화면 기능도 이 창구로 감쌉니다.
    // 실제 Braille 렌더링 구현은 AsciiBattleDemo.cpp에 남아 있습니다.
    namespace Presentation
    {
        void ClearScreen();
        bool RenderPulsingMainMenuImage(const std::wstring& imagePath, double elapsedSeconds);
        void MoveCursorBelowStaticImage(int blankRowCount = 1);
        void RunMainMenuImageTuner(const std::wstring& imagePath);
        void PrepareBattleSummaryArea();
    }
}
