#pragma once

#include <string>
#include <functional>

#include "AsciiBattleScene.h"

namespace AsciiArt
{
    // 정지 이미지의 표현 방식입니다. Braille은 고해상도 점 표현,
    // LandscapeAscii는 풍경용 문자/윤곽선 표현입니다.
    enum class EStaticArtStyle
    {
        Braille,
        LandscapeAscii,
    };

    // AA 모듈은 BattleManager를 소유하지 않습니다. 실제 게임 쪽은 공격 이벤트와
    // 현재 전투 상태만 전달하고, 이 모듈은 표시·입력·애니메이션만 담당합니다.
    enum class EBattleActionType
    {
        PlayerAttack,
        PlayerUsePotion,
        PlayerUsePowerPotion,
        MonsterAttack,
    };

    struct BattleAction
    {
        EBattleActionType type = EBattleActionType::PlayerAttack;
        int attackerIndex = 0;
        int targetIndex = 0;
    };

    using BattleActionCallback = std::function<bool(const BattleAction&)>;
    using BattleStateProvider = std::function<BattleSceneState()>;

    int RunStandaloneDemo(
        int heroTurnCount = 2,
        const BattleActionCallback& onBattleAction = {},
        const BattleStateProvider& getBattleState = {},
        bool potionOnlyTestMode = false);

    bool RenderStaticImage(
        const std::wstring& imagePath,
        bool useColor = true,
        int monochromeInkDensity = 0,
        EStaticArtStyle style = EStaticArtStyle::Braille,
        int monochromeContrast = 850,
        int outputPixelWidth = 1000,
        int characterHeightScale = 500
    );
    bool RenderSavedStartScreenImage(const std::wstring& imagePath, bool useColor = false);
    bool RenderSavedMainMenuImage(const std::wstring& imagePath, bool useColor = true);
    // 전투 시작·연속 전투·마을 복귀 때만 쓰는 정지 연출입니다.
    // 실제 BattleManager 전투 로직은 이 함수들을 알 필요가 없습니다.
    void RenderBattleEntryTransition();
    // 보스 전투에만 사용하는 문 개방 → 암전 → 드래곤 등장 연출입니다.
    void RenderBossBattleEntryTransition();
    void RenderNextBattleTransition();
    void RenderBattleReturnTransition();
    // 원본 PNG를 수정하지 않고 지정된 빛 위치만 밝게/어둡게 합성해 출력합니다.
    bool RenderPulsingMainMenuImage(const std::wstring& imagePath, double elapsedSeconds);
    // 상점처럼 배경, 인물, 전경 오브젝트를 한 장면으로 합성해 출력합니다.
    bool RenderLayeredStaticImage(
        const std::wstring& backgroundImagePath,
        const std::wstring& characterImagePath,
        const std::wstring& foregroundImagePath,
        float characterX = 650.0f,
        float characterY = 86.0f,
        float characterWidth = 614.0f,
        float characterHeight = 842.0f,
        int outputPixelWidth = 1000,
        int characterHeightScale = 350,
        int contrast = 630
    );
    bool RunStaticImageTuner(const std::wstring& imagePath, bool useColor = false);
    bool RunMainMenuImageTuner(const std::wstring& imagePath);
    void ClearScreen();
    void ClearBottomRows(int rowCount);
    void ScrollScreenOneLine();
    void MoveCursorBelowStaticImage(int blankRowCount = 1);
    void DrawCenteredText(const std::wstring& text, float verticalRatio);
    void DrawCenteredTextOnClearPanel(const std::wstring& text, float verticalRatio);
    void DrawStaticImageText(
        const std::wstring& text,
        float horizontalRatio,
        float verticalRatio,
        bool useGoldColor = false,
        int rowOffset = 0
    );
    bool GetStaticImageBounds(short& left, short& top, short& width, short& height);
}
