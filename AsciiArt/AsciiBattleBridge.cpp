// TextRPGSource/AsciiArt/AsciiBattleBridge.cpp
#define NOMINMAX
#include <windows.h>

#include "AsciiBattleBridge.h"

#include "AsciiBattleDemo.h"
#include "SceneConfig.h"
#include "../BattleManager.h"
#include "../Inventory.h"
#include "../ItemUseHandler.h"
#include "../Monster.h"
#include "../MonsterPool.h"
#include "../Player.h"
#include "../RpgLogger.h"
#include "../SoundManager.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>



namespace
{
    // 디버그 전투 메뉴 코드는 남겨 두되, 실제 게임에서는 기존 인벤토리·행동 흐름을 사용합니다.
    // 다시 시험할 때만 true로 바꾸면 포션 99개와 행동 선택 메뉴가 함께 켜집니다.
    constexpr bool kEnablePotionOnlyTest = false;
    // 현재 AA 전투 배치는 4마리까지 각각의 위치를 고정해 둔 상태입니다.
    // 배치 작업이 끝날 때까지 적 수만 독립적으로 4마리로 고정합니다.
    constexpr bool kForceFourMonsterLayoutTest = true;
    constexpr int kStarterPowerPotionCount = 99;
    constexpr int kMaximumBattleMonsterCount = 4;
    bool gHasEnteredBattleSequence = false;
    bool gPlayBossBattleIntro = false;

    std::wstring BridgeUtf8ToWide(const std::string& text)
    {
        if (text.empty()) return {};

        const int requiredLength = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
        if (requiredLength <= 0) return {};

        std::wstring result(static_cast<size_t>(requiredLength), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), requiredLength);
        return result;
    }

    std::string MakeActorId(const char* prefix, const void* address)
    {
        return std::string(prefix) + std::to_string(reinterpret_cast<std::uintptr_t>(address));
    }
}

void AsciiArt::GrantBattleTestPotions(Inventory& inventory)
{
    if (!kEnablePotionOnlyTest) return;

    const auto grant = [&](EItemID itemId)
    {
        int remaining = kStarterPowerPotionCount;
        while (remaining > 0)
        {
            remaining = inventory.AddItem(itemId, remaining);
            if (remaining > 0) inventory.ExpandSlotCount(1);
        }
    };
    grant(EItemID::HP_POTION);
    grant(EItemID::POWER_POTION);
}

int AsciiArt::GetBattleTestMonsterCount(int originalMonsterCount)
{
    return kForceFourMonsterLayoutTest ? kMaximumBattleMonsterCount : originalMonsterCount;
}

bool AsciiArt::RunBattlePresentation(BattleManager& battleManager, MonsterPool& monsterPool, RpgLogger& logger, Inventory& inventory, SoundManager& soundManager)
{
    if (gPlayBossBattleIntro)
    {
        RenderBossBattleEntryTransition();
        gPlayBossBattleIntro = false;
        gHasEnteredBattleSequence = true;
    }
    else if (gHasEnteredBattleSequence)
    {
        RenderNextBattleTransition();
    }
    else
    {
        RenderBattleEntryTransition();
        gHasEnteredBattleSequence = true;
    }
    // 이 변수들은 기본 전투 데이터가 아니라, "화면에 어떻게 보여 줄지"만 기록합니다.
    std::map<const void*, int> previousHp;
    std::string floatingTextTargetId;
    int floatingTextValue = 0;
    bool floatingTextIsHealing = false;
    bool floatingTextIsPowerBuff = false;
    auto floatingTextStartedAt = std::chrono::steady_clock::now() - std::chrono::seconds(2);
    bool isMonsterTurn = false;
    std::string turnActorName;
    std::string actionResultText;
    bool actionResultIsHealing = false;
    bool actionResultIsPowerBuff = false;
    std::set<const Player*> powerBuffedPlayers;

    // 전투가 시작된 시점의 몬스터 개체와 화면 슬롯을 1:1로 고정합니다.
    // 이후 한 마리가 죽어도 GetLivingMonsters()의 인덱스가 당겨지지 않으므로,
    // 살아 있는 다른 몬스터의 위치/크기가 바뀌지 않습니다.
    const std::vector<Monster*> battleMonsterSlots = battleManager.GetLivingMonsters();

    const auto makeSceneState = [&]()
    {
        BattleSceneState state;
        state.useColor = true;
        state.floatingTextTargetId = floatingTextTargetId;
        state.floatingTextValue = floatingTextValue;
        state.floatingTextIsHealing = floatingTextIsHealing;
        state.floatingTextIsPowerBuff = floatingTextIsPowerBuff;
        state.floatingTextAgeSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - floatingTextStartedAt).count();
        state.isMonsterTurn = isMonsterTurn;
        state.turnActorName = turnActorName;
        state.actionResultText = actionResultText;
        state.actionResultIsHealing = actionResultIsHealing;
        state.actionResultIsPowerBuff = actionResultIsPowerBuff;
        state.showTestControls = kEnablePotionOnlyTest;

        for (Player* player : battleManager.GetLivingPlayers())
        {
            const int oldHp = previousHp.emplace(player, player->GetHp()).first->second;
            state.playerStatuses.push_back({ MakeActorId("player_", player), player->GetName(), player->GetHp(), player->GetHpMax(), oldHp, player->IsDead(), powerBuffedPlayers.count(player) != 0 });
        }
        for (Monster* monster : battleMonsterSlots)
        {
            const int oldHp = previousHp.emplace(monster, monster->GetHp()).first->second;
            state.monsterStatuses.push_back({ MakeActorId("monster_", monster), monster->GetName(), monster->GetHp(), monster->GetHpMax(), oldHp, monster->IsDead() });
        }
        return state;
    };

    const int heroTurnCount = std::max(1, static_cast<int>(battleManager.GetLivingPlayers().size()));
    RunStandaloneDemo(heroTurnCount,
        [&](const BattleAction& action)
        {
            if (action.type == EBattleActionType::PlayerUsePotion || action.type == EBattleActionType::PlayerUsePowerPotion)
            {
                std::vector<Player*> livingPlayers = battleManager.GetLivingPlayers();
                if (livingPlayers.empty()) return false;

                Player* target = livingPlayers[std::clamp(action.attackerIndex, 0, static_cast<int>(livingPlayers.size()) - 1)];
                isMonsterTurn = false;
                const bool usePowerPotion = action.type == EBattleActionType::PlayerUsePowerPotion;
                const EItemID potionId = usePowerPotion ? EItemID::POWER_POTION : EItemID::HP_POTION;
                turnActorName = target->GetName() + (usePowerPotion ? " uses a power potion." : " uses an HP potion.");

                if (inventory.ConsumeItem(potionId) && ItemUseHandler::USE_ITEM(target, potionId))
                {
                    floatingTextTargetId = MakeActorId("player_", target);
                    floatingTextValue = ITEM_TABLE.at(potionId).effectValue;
                    floatingTextIsHealing = !usePowerPotion;
                    floatingTextIsPowerBuff = usePowerPotion;
                    if (usePowerPotion) powerBuffedPlayers.insert(target);
                    actionResultText = usePowerPotion
                        ? "공격력 +" + std::to_string(floatingTextValue)
                        : "회복: +" + std::to_string(floatingTextValue);
                    actionResultIsHealing = !usePowerPotion;
                    actionResultIsPowerBuff = usePowerPotion;
                    floatingTextStartedAt = std::chrono::steady_clock::now();
                    logger.AddLog(target->GetName() + " used a potion.", false);
                    soundManager.PlaySFX(soundMap.at(SoundStates::POTION_USE));
                }
                else
                {
                    actionResultText = usePowerPotion ? "공격력 포션을 사용할 수 없습니다." : "HP 포션을 사용할 수 없습니다.";
                    actionResultIsHealing = false;
                    actionResultIsPowerBuff = false;
                    logger.AddLog(target->GetName() + " could not use a power potion.", false);
                }
            }
            else if (action.type == EBattleActionType::PlayerAttack)
            {
                
                std::vector<Player*> livingPlayers = battleManager.GetLivingPlayers();
                if (livingPlayers.empty() || battleManager.GetLivingMonsters().empty() || battleMonsterSlots.empty()) return false;

                Player* attacker = livingPlayers[std::clamp(action.attackerIndex, 0, static_cast<int>(livingPlayers.size()) - 1)];
                Monster* target = battleMonsterSlots[std::clamp(action.targetIndex, 0, static_cast<int>(battleMonsterSlots.size()) - 1)];
                // 죽은 몬스터의 빈 자리는 유지하지만 다시 공격 대상으로 삼지는 않습니다.
                if (target->IsDead()) return !battleManager.IsMonstersDead() && !battleManager.IsPlayersDead();
                const int hpBefore = target->GetHp();
                previousHp[target] = hpBefore;
                isMonsterTurn = false;
                turnActorName = attacker->GetName() + "(이)가 " + target->GetName() + "을(를) 공격합니다.";
                battleManager.PlayerHitMonster(target, attacker->GetPower());
                // 타격 사운드
                soundManager.PlaySFX(soundMap.at(SoundStates::ATTACK_01));
                floatingTextTargetId = MakeActorId("monster_", target);
                floatingTextValue = std::max(0, hpBefore - target->GetHp());
                floatingTextIsHealing = false;
                floatingTextIsPowerBuff = false;
                actionResultText = "피해: " + std::to_string(floatingTextValue);
                actionResultIsHealing = false;
                actionResultIsPowerBuff = false;
                floatingTextStartedAt = std::chrono::steady_clock::now();
                logger.AddLog(turnActorName, false);
                if (target->IsDead())
                {
                    logger.AddLog(target->GetName() + " 처치!", false);
                    logger.OnMonsterKilled(target->GetMonsterId());
                    monsterPool.Release(target);
                }
            }
            else
            {
                std::vector<Player*> livingPlayers = battleManager.GetLivingPlayers();
                if (livingPlayers.empty() || battleManager.GetLivingMonsters().empty() || battleMonsterSlots.empty()) return false;

                Monster* attacker = battleMonsterSlots[std::clamp(action.attackerIndex, 0, static_cast<int>(battleMonsterSlots.size()) - 1)];
                if (attacker->IsDead()) return !battleManager.IsMonstersDead() && !battleManager.IsPlayersDead();
                Player* target = livingPlayers[std::clamp(action.targetIndex, 0, static_cast<int>(livingPlayers.size()) - 1)];
                const int hpBefore = target->GetHp();
                previousHp[target] = hpBefore;
                isMonsterTurn = true;
                turnActorName = attacker->GetName() + "(이)가 " + target->GetName() + "을(를) 공격합니다.";
                battleManager.MonsterHitPlayer(target, attacker->GetPower());
                // 타격 사운드 
                soundManager.PlaySFX(soundMap.at(SoundStates::ATTACK_02));
                floatingTextTargetId = MakeActorId("player_", target);
                floatingTextValue = std::max(0, hpBefore - target->GetHp());
                floatingTextIsHealing = false;
                floatingTextIsPowerBuff = false;
                actionResultText = "피해: " + std::to_string(floatingTextValue);
                actionResultIsHealing = false;
                actionResultIsPowerBuff = false;
                floatingTextStartedAt = std::chrono::steady_clock::now();
                logger.AddLog(attacker->GetName() + "의 공격! " + target->GetName() + " 피해", false);
            }
            return !battleManager.IsMonstersDead() && !battleManager.IsPlayersDead();
        },
        makeSceneState,
        kEnablePotionOnlyTest);

    return battleManager.IsMonstersDead();
}

void AsciiArt::PrepareBossBattlePresentation()
{
    gPlayBossBattleIntro = true;
}

void AsciiArt::ReturnToTownFromBattlePresentation()
{
    RenderBattleReturnTransition();
    gHasEnteredBattleSequence = false;
    gPlayBossBattleIntro = false;
}

void AsciiArt::Presentation::ClearScreen()
{
    AsciiArt::ClearScreen();
}

bool AsciiArt::Presentation::RenderStaticScene(EStaticScene scene)
{
    const SceneConfig config = LoadSceneConfig();
    const std::wstring& imagePath = scene == EStaticScene::Inn
        ? config.innBackgroundImagePath
        : config.craftingBackgroundImagePath;

    return AsciiArt::RenderStaticImage(
        imagePath,
        true,
        0,
        AsciiArt::EStaticArtStyle::Braille,
        config.mainMenuContrast,
        config.mainMenuOutputPixelWidth,
        config.mainMenuCharacterHeightScale);
}

void AsciiArt::Presentation::DrawStaticSceneMenu(
    const std::wstring& title,
    int gold,
    const std::vector<std::wstring>& menuLines)
{
    // 상점 화면과 같은 상단 정보 위치를 사용합니다.
    AsciiArt::DrawStaticImageText(title, 0.16f, 0.08f);
    AsciiArt::DrawStaticImageText(
        L"보유 골드: " + std::to_wstring(gold) + L" G",
        0.84f,
        0.08f,
        true);

    if (menuLines.empty())
    {
        return;
    }

    // 제작소처럼 항목이 많아도 겹치지 않도록 이미지 하단 영역에 고르게 배치합니다.
    constexpr float kFirstMenuVerticalRatio = 0.64f;
    constexpr float kLastMenuVerticalRatio = 0.88f;
    const float step = menuLines.size() > 1
        ? (kLastMenuVerticalRatio - kFirstMenuVerticalRatio) /
            static_cast<float>(menuLines.size() - 1)
        : 0.0f;

    for (size_t index = 0; index < menuLines.size(); ++index)
    {
        AsciiArt::DrawStaticImageText(
            menuLines[index],
            0.5f,
            kFirstMenuVerticalRatio + step * static_cast<float>(index),
            index == 0);
    }
}

bool AsciiArt::Presentation::RenderPulsingMainMenuImage(const std::wstring& imagePath, double elapsedSeconds)
{
    return AsciiArt::RenderPulsingMainMenuImage(imagePath, elapsedSeconds);
}

void AsciiArt::Presentation::MoveCursorBelowStaticImage(int blankRowCount)
{
    AsciiArt::MoveCursorBelowStaticImage(blankRowCount);
}

void AsciiArt::Presentation::RunMainMenuImageTuner(const std::wstring& imagePath)
{
    AsciiArt::RunMainMenuImageTuner(imagePath);
}

void AsciiArt::Presentation::PrepareBattleSummaryArea()
{
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(output, &info)) return;

    const short startY = static_cast<short>(info.srWindow.Top + 3);
    const DWORD width = static_cast<DWORD>(info.srWindow.Right - info.srWindow.Left + 1);
    DWORD written = 0;
    for (short row = 0; row < 16; ++row)
    {
        FillConsoleOutputCharacterW(output, L' ', width, {info.srWindow.Left, static_cast<short>(startY + row)}, &written);
    }
    SetConsoleCursorPosition(output, {info.srWindow.Left, startY});
}

void AsciiArt::Presentation::ShowInfoPanel(const std::string& title, const std::vector<std::string>& lines)
{
    std::vector<std::wstring> wideLines;
    wideLines.reserve(lines.size());
    for (const std::string& line : lines)
    {
        wideLines.push_back(BridgeUtf8ToWide(line));
    }
    DrawStaticImageInfoPanel(BridgeUtf8ToWide(title), wideLines);

    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD record{};
    DWORD read = 0;
    while (ReadConsoleInputW(input, &record, 1, &read))
    {
        if (record.EventType != KEY_EVENT || !record.Event.KeyEvent.bKeyDown) continue;

        const WORD key = record.Event.KeyEvent.wVirtualKeyCode;
        if (key == VK_RETURN || key == VK_SPACE || key == VK_ESCAPE)
        {
            return;
        }
    }
}
