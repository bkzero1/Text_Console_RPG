#include <chrono>
#include <iostream>
#include <random>
#include <thread>

#define NOMINMAX
#include <windows.h>

#include "BattleManager.h"
#include "Crafter.h"
#include "FMonsterData.h"
#include "Inventory.h"
#include "ItemUseHandler.h"
#include "Mage.h"
#include "Monster.h"
#include "MonsterPool.h"
#include "Player.h"
#include "RpgLogger.h"
#include "ShopManager.h"
#include "Tank.h"
#include "Warrior.h"
#include "ShopManager.h"
#include "SoundManager.h"
#include "StartScreen.h"
#include "ShopScreen.h"
#include "GameClearScreen.h"
#include "AsciiArt/AsciiBattleBridge.h"

// 게임 상태
enum class EGameState
{
    START_SCREEN,   // 게임 시작 화면
    PLAYER_INIT,    // 캐릭터 생성 (이름, 스탯 초기 설정)
    NORMAL_BATTLE,  // 일반 전투
    BOSS_BATTLE,    // 보스 전투
    MAIN_MEMU,      // 메인 메뉴
    SHOP,           // 상점
    INN,            // 여관
    CRAFTING,       // 아이템 제작소
    GAME_OVER,      // 게임 패배
    GAME_CLEAR,     // 게임 승리
};

// 전역 변수
EGameState CurrentGameState = EGameState::START_SCREEN;
bool IsRunning = true;
RpgLogger rpgLogger;
SoundManager soundManager;

// 메인 메뉴 전용 연출 상태입니다.
// 처음 마을에 들어갈 때는 낮이며, 전투를 마치고 돌아올 때만 낮/밤이 바뀝니다.
bool IsMainMenuNight = false;
bool HasVisitedMainMenu = false;

Inventory* inventory;          // 인벤토리
std::vector<Player*> players;  // 플레이어 목록

namespace
{
// 임시 메인 메뉴 번호입니다.
// 팀에서 최종 번호를 정하면 아래 숫자 두 개만 바꾸면 메뉴 표시와 입력 처리가 함께 바뀝니다.
constexpr int kInnMenuNumber = 8;
constexpr int kCraftingMenuNumber = 9;

// 테스트 종료 후 false로 바꾸면 첫 전투 레벨 10 보정을 끌 수 있습니다.
constexpr bool kEnableFirstBattleLevelTenTest = true;
bool gHasAppliedFirstBattleLevelTenTest = false;

void ApplyFirstBattleLevelTenTest(const std::vector<Player*>& currentPlayers)
{
    if (!kEnableFirstBattleLevelTenTest || gHasAppliedFirstBattleLevelTenTest)
    {
        return;
    }

    for (Player* player : currentPlayers)
    {
        if (player == nullptr)
        {
            continue;
        }

        while (player->GetLevel() < 10)
        {
            // 직업별 LevelUp() 규칙을 그대로 사용합니다.
            player->LevelUp();
        }
    }

    gHasAppliedFirstBattleLevelTenTest = true;
    rpgLogger.AddLog("[테스트] 첫 전투 완료: 파티 레벨을 10으로 설정했습니다.");
}
}  // namespace

// 게임 상태 전환
void SwitchState(EGameState newGameState)
{
    CurrentGameState = newGameState;
}

void ToggleMainMenuTimeOfDay()
{
    IsMainMenuNight = !IsMainMenuNight;
}

void AdvanceMainMenuTimeAfterBattle()
{
    // 첫 전투 뒤 도착하는 첫 마을은 항상 낮으로 시작합니다.
    if (!HasVisitedMainMenu)
    {
        IsMainMenuNight = false;
        return;
    }

    ToggleMainMenuTimeOfDay();
}

bool StringCompare(string a, string b)
{
    if (a.size() != a.size())
    {
        return false;
    }

    for (int i = 0; i < a.size(); i++)
    {
        char aChar = std::tolower(a[i]);
        char bChar = std::tolower(b[i]);
        if (aChar != bChar)
        {
            return false;
        }
    }
    return true;
}

// 캐릭터 생성
void PlayerInit()
{
    std::string playerName;

    // 전사 생성
    while (true)
    {
        std::cout << "전사의 이름을 입력하세요: ";
        std::getline(std::cin, playerName);

        // 입력 검증: 빈 칸이면 다시 입력
        if (playerName.empty())
        {
            std::cout << " 이름을 입력해주세요!\n\n";
            continue;
        }

        players.push_back(new Warrior(playerName));
        break;
    }

    // 마법사 생성
    while (true)
    {
        std::cout << "마법사의 이름을 입력하세요: ";
        std::getline(std::cin, playerName);

        // 입력 검증: 빈 칸이면 다시 입력
        if (playerName.empty())
        {
            std::cout << " 이름을 입력해주세요!\n\n";
            continue;
        }

        players.push_back(new Mage(playerName));
        break;
    }

    // 탱커 생성
    while (true)
    {
        std::cout << "탱커의 이름을 입력하세요: ";
        std::getline(std::cin, playerName);

        // 입력 검증: 빈 칸이면 다시 입력
        if (playerName.empty())
        {
            std::cout << " 이름을 입력해주세요!\n\n";
            continue;
        }

        players.push_back(new Tank(playerName));
        break;
    }

    // 인벤토리 생성
    inventory = new Inventory();
    AsciiArt::GrantBattleTestPotions(*inventory);
    // 첫 전투 시작
    SwitchState(EGameState::NORMAL_BATTLE);
}
bool BattlePhase(BattleManager& battleManager, MonsterPool& monsterPool)
{
    return AsciiArt::RunBattlePresentation(battleManager, monsterPool, rpgLogger, *inventory);
}

// 일반 전투
void NormalBattle()
{
    BattleManager battleManager;
    MonsterPool monsterPool = MonsterPool();

    int totalLv = 0;
    for (int i = 0; i < players.size(); i++)
    {
        battleManager.AddPlayer(players[i]);
        totalLv += players[i]->GetLevel();
    }

    int avgLv = totalLv / (int)players.size();

    // 평균 레벨 2마다 한 마리씩 늘어나며, 기본 한 마리를 더 포함합니다.
    int monsterCount = std::max(1, avgLv / 2) + 1;
    vector<EMonsterID> monsterCards = battleManager.GetSpawanAbleMonsterIDs(avgLv);

    // 랜덤 준비
    std::random_device rd;
    std::mt19937 gen(rd());
    // 현재 평균 레벨에서 등장 가능한 몬스터 카드 중 하나를 고릅니다.
    std::uniform_int_distribution<size_t> deployDist(0, monsterCards.size() - 1);

    for (int i = 0; i < monsterCount; i++)
    {
        Monster* monster = monsterPool.Acquire();
        EMonsterID randomMonster = monsterCards[deployDist(gen)];
        std::string nanori = monster->Deploy(randomMonster, avgLv);
        rpgLogger.AddLog(nanori);
        battleManager.AddMonster(monster);
    }

    bool isWin = BattlePhase(battleManager, monsterPool);

    if (!isWin)
    {
        // 패배자들 체력 회복 시키고 내쫓기
        for (int i = 0; i < players.size(); i++)
        {
            players[i]->HealHP(1);
        }
        CurrentGameState = EGameState::GAME_OVER;
        return;
    }

    // 전투 종료 요약은 AA 화면 좌측 상단 로그 영역에서 이어서 표시합니다.
    AsciiArt::Presentation::PrepareBattleSummaryArea();

    // 전리품 인벤토리에 추가
    // 골드 획득
    inventory->AddGold(battleManager.GetEarnGold());
    rpgLogger.AddLog("파티는 " + to_string(battleManager.GetEarnGold()) + "골드를 얻었다");
    std::map<EItemID, int> earnItems = battleManager.GetEarnItems();
    for (auto itr = earnItems.begin(); itr != earnItems.end(); itr++)
    {
        std::string earnItemName = ITEM_TABLE.at(itr->first).name;
        int earnItemNumber = itr->second;
        rpgLogger.AddLog(earnItemName + " " + to_string(earnItemNumber) + "개 발견");
    }

    std::map<EItemID, int> remainingItems = inventory->AddItems(earnItems);  // 아이템 획득
    while (!remainingItems.empty())
    {
        inventory->ShowInventory();

        // 제거할 슬롯 번호 입력 (0: 남은 아이템 포기)
        std::cout << "제거할 아이템 슬롯 번호 선택 (0: 남은 아이템 포기): ";
        int slotNum;
        std::cin >> slotNum;

        // 유효하지 않은 입력
        if (slotNum < 0 || slotNum > inventory->GetSlots().size())
        {
            continue;
        }

        // 남은 아이템 포기
        if (slotNum == 0)
        {
            break;
        }

        // 슬롯 제거 후 다시 획득
        inventory->RemoveSlot(slotNum - 1);
        remainingItems = inventory->AddItems(remainingItems);
    }
    battleManager.EarnExpToParty();
    rpgLogger.AddLog("파티는 " + to_string(battleManager.GetEarnExp()) + " exp 를 얻었다");
    ApplyFirstBattleLevelTenTest(players);

    // 전투 후 레벨 확인
    totalLv = 0;
    for (int i = 0; i < players.size(); i++)
    {
        totalLv += players[i]->GetLevel();
    }
    avgLv = totalLv / (int)players.size();

    while (true)
    {
        std::cout << "마을로 돌아가겠습니까? [Y/N]" << endl;
        string answer;
        std::cin >> answer;
        if (StringCompare(answer, "Y"))
        {
            AsciiArt::ReturnToTownFromBattlePresentation();
            AdvanceMainMenuTimeAfterBattle();
            SwitchState(EGameState::MAIN_MEMU);
            break;
        }

        if (StringCompare(answer, "N"))
        {
            if (avgLv < 10)
            {
                SwitchState(EGameState::NORMAL_BATTLE);
                break;
            }
            else
            {
                SwitchState(EGameState::BOSS_BATTLE);
                break;
            }
        }
    }
}

// 보스 전투
void BossBattle()
{
    BattleManager battleManager = BattleManager();
    MonsterPool monsterPool = MonsterPool();

    int totalLv = 0;
    for (int i = 0; i < players.size(); i++)
    {
        battleManager.AddPlayer(players[i]);
        totalLv += players[i]->GetLevel();
    }

    int avgLv = totalLv / (int)players.size();

    // 보스전은 레드 드래곤 한 마리로 고정합니다.
    Monster* monster = monsterPool.Acquire();
    std::string nanori = monster->Deploy(EMonsterID::RED_DRAGON, avgLv, true);
    rpgLogger.AddLog(nanori);
    battleManager.AddMonster(monster);

    AsciiArt::PrepareBossBattlePresentation();
    bool isWin = BattlePhase(battleManager, monsterPool);

    if (!isWin)
    {
        // 패배자들 체력 회복 시키고 내쫓기
        for (int i = 0; i < players.size(); i++)
        {
            players[i]->HealHP(1);
        }
        CurrentGameState = EGameState::GAME_OVER;
        return;
    }
    else
    {
        CurrentGameState = EGameState::GAME_CLEAR;
        return;
    }
}

// main.cpp - 등록된 아이템 테이블 전체 출력
void ShowItemTable()
{
    std::cout << "\n========== 아이템 목록 ==========\n";

    for (const auto& [itemID, itemData] : ITEM_TABLE)
    {
        std::cout << "[" << static_cast<int>(itemID) << "] "
                  << itemData.name << '\n'
                  << "설명: " << itemData.description << '\n'
                  << "구매 가격: " << itemData.purchasePrice << " G\n"
                  << "사용 가능: " << (itemData.isConsumable ? "가능" : "불가능")
                  << "\n------------------------------\n";
    }
}

// 메인 메뉴
void MainMenu()
{
    constexpr const wchar_t* kMainMenuDayBackground = L"Resources\\Images\\main_menu_day_final_candidate_v5.png";
    constexpr const wchar_t* kMainMenuNightBackground = L"Resources\\Images\\main_menu_night.png";
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD originalInputMode = 0;
    const bool canRestoreInputMode = GetConsoleMode(input, &originalInputMode) != FALSE;
    // 이 시점부터는 다음 전투 복귀에서 낮/밤을 교대할 수 있습니다.
    HasVisitedMainMenu = true;
    if (canRestoreInputMode)
    {
        // 상점과 같은 방식으로 키 이벤트를 받아 P와 숫자 키를 즉시 처리합니다.
        SetConsoleMode(input,
            (originalInputMode | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) &
            ~(ENABLE_QUICK_EDIT_MODE | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
    }

    while (true)
    {
        const wchar_t* const mainMenuBackground =
            IsMainMenuNight ? kMainMenuNightBackground : kMainMenuDayBackground;

        AsciiArt::Presentation::ClearScreen();
        INPUT_RECORD record{};
        DWORD read = 0;
        const auto animationStartTime = std::chrono::steady_clock::now();
        auto previousFrameTime = animationStartTime - std::chrono::milliseconds(100);

        while (true)
        {
            const auto now = std::chrono::steady_clock::now();
            // 빛 변화는 초당 10번만 갱신합니다. 원본 PNG는 바꾸지 않고,
            // 달라진 Braille 행만 다시 출력하므로 메뉴 입력은 계속 받을 수 있습니다.
            if (now - previousFrameTime >= std::chrono::milliseconds(100))
            {
                const double elapsedSeconds = std::chrono::duration<double>(now - animationStartTime).count();
                if (!AsciiArt::Presentation::RenderPulsingMainMenuImage(mainMenuBackground, elapsedSeconds))
                {
                    if (canRestoreInputMode) SetConsoleMode(input, originalInputMode);
                    std::cout << "메인 메뉴 배경을 불러오지 못했습니다.\n";
                    return;
                }

                // 메뉴는 이미지가 끝난 바로 다음 줄에 가로 한 줄로 표시합니다.
                AsciiArt::Presentation::MoveCursorBelowStaticImage(0);
                std::cout << "1. 전투    2. 상점    3. 플레이어 정보    4. 처치 기록    "
                          << kInnMenuNumber << ". 여관    "
                          << kCraftingMenuNumber << ". 제작소\n";
                std::cout << "입력: ";
                previousFrameTime = now;
            }

            DWORD pendingCount = 0;
            if (GetNumberOfConsoleInputEvents(input, &pendingCount) && pendingCount > 0)
            {
                ReadConsoleInputW(input, &record, 1, &read);
                if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
                {
                    break;
                }
            }
            // 애니메이션을 멈추지 않게 짧게 입력을 확인합니다.
            Sleep(10);
        }

        // 문자 자체가 아니라 가상 키 코드를 읽으므로 한글 입력 상태에서도 P가 동작합니다.
        const WORD menuKey = record.Event.KeyEvent.wVirtualKeyCode;

        if (menuKey == 'P')
        {
            // 상점·전투처럼 해상도, 세로 비율, 대비를 메인 메뉴 전용으로 조절합니다.
            AsciiArt::Presentation::RunMainMenuImageTuner(mainMenuBackground);
            continue;
        }

        // 숫자열과 키패드 숫자를 같은 메뉴 번호로 바꿉니다.
        // 여관·제작소 번호를 나중에 바꿔도 위 상수만 수정하면 됩니다.
        const int option =
            menuKey >= '0' && menuKey <= '9' ? menuKey - '0' :
            menuKey >= VK_NUMPAD0 && menuKey <= VK_NUMPAD9 ? menuKey - VK_NUMPAD0 : 0;

        if (option == 0)
        {
            continue;
        }

    // 유효하지 않은 입력
        // 플레이어 레벨
        int totalLv = 0;
        for (int i = 0; i < players.size(); i++)
        {
            totalLv += players[i]->GetLevel();
        }
        int avgLv = totalLv / static_cast<int>(players.size());

        // 상태 전이
        switch (option)
        {
            case 1:
                // 실제 일반/보스 전투 진입은 기존 게임 상태 흐름을 그대로 사용합니다.
                if (avgLv < 10)
                {
                    SwitchState(EGameState::NORMAL_BATTLE);
                }
                else
                {
                    SwitchState(EGameState::BOSS_BATTLE);
                }
                if (canRestoreInputMode) SetConsoleMode(input, originalInputMode);
                return;
            case 2:
                SwitchState(EGameState::SHOP);
                if (canRestoreInputMode) SetConsoleMode(input, originalInputMode);
                return;
            case 3:
            {
                std::vector<std::string> statusLines;
                statusLines.reserve(players.size() * 5);
                for (const auto& player : players)
                {
                    statusLines.push_back("이름     : " + player->GetName());
                    statusLines.push_back("레벨     : " + std::to_string(player->GetLevel()));
                    statusLines.push_back("체력     : " + std::to_string(player->GetHp()) + " / " + std::to_string(player->GetHpMax()));
                    statusLines.push_back("공격력   : " + std::to_string(player->GetPower()));
                    statusLines.push_back("경험치   : " + std::to_string(player->GetExp()) + " / " + std::to_string(player->GetExpMax()));
                    statusLines.push_back("");
                }
                AsciiArt::Presentation::ShowInfoPanel("캐릭터 정보", statusLines);
                if (canRestoreInputMode) SetConsoleMode(input, originalInputMode);
                return;
            }
            case 4:
                // 처치 기록 화면은 아직 별도 상태가 없으므로, 현재 메뉴를 다시 표시합니다.
                break;
            case kInnMenuNumber:
                SwitchState(EGameState::INN);
                if (canRestoreInputMode) SetConsoleMode(input, originalInputMode);
                return;
            case kCraftingMenuNumber:
                SwitchState(EGameState::CRAFTING);
                if (canRestoreInputMode) SetConsoleMode(input, originalInputMode);
                return;
            case 7:
                rpgLogger.ShowKillLogs();
                if (canRestoreInputMode) SetConsoleMode(input, originalInputMode);
                return;
            default:
                break;
            }
        }
    }


// 상점
void Shop()
{
    // 구매/판매 목록이 콘솔을 스크롤한 뒤 상점으로 돌아와도 AA 장면을 처음부터 다시 그립니다.
    AsciiArt::Presentation::ClearScreen();

    // 상점에서 보이는 모든 메뉴 문구는 여기 한 곳에서 관리합니다.
    const ShopScreen::FMenuText shopMenuText{
        L"[ TEAM_3 TRPG SHOP ]",
        L"1. 아이템 구매",
        L"2. 아이템 판매",
        L"0. 돌아가기",
    };
    ShopManager shop;
    const int select = ShopScreen::RunAnimatedMenu(shopMenuText, inventory->GetGold());

    // AA 이미지 끝 다음 줄을 비운 뒤, 기존 상점의 구매/판매 목록을 출력합니다.
    AsciiArt::Presentation::MoveCursorBelowStaticImage(2);

    if (select >= 0 && select <= 2)
    {
        switch (select)
        {
            case 1:
            {
                // 구매 가능한 아이템 리스트 출력
                shop.ShowBuyableList();
                // 구매 가능한 아이템 ID값 가져오기 (ShowBuyableList와 Mapping을 위한 같은 순서)
                std::vector<EItemID> buyItemIDs = shop.GetBuyableItemIDs();

                int itemChoice, buyCount;
                while (true)
                {
                    std::cout << "구매할 아이템의 번호를 입력해주세요. (0: 돌아가기) : ";
                    std::cin >> itemChoice;
                    // 아이템 구매
                    if (itemChoice >= 1 && itemChoice <= buyItemIDs.size())
                    {
                        std::cout << "구매할 개수를 입력해주세요: ";
                        std::cin >> buyCount;
                        // 유저가 선택한 choice의 id값, 아이템 정보 찾기
                        EItemID id = buyItemIDs.at(itemChoice - 1);
                        const ItemData& itemTarget = ITEM_TABLE.at(id);
                        if (buyCount > 0)
                        {
                            shop.BuyItem(itemTarget, buyCount);
                        }
                        else
                        {
                            std::cout << "잘못 입력하셨습니다." << std::endl;
                        }
                    }
                    else if (itemChoice == 0)
                    {
                        return;
                    }
                    else
                    {
                        std::cout << "잘못 입력하셨습니다." << std::endl;
                    }
                }
                break;
            }
            case 2:
            {
                while (true)
                {
                    // 판매 가능한 리스트 출력
                    shop.ShowSellableList();
                    // 화면에 출력된 순서와 동일한 아이템 ID 목록
                    std::vector<EItemID> itemIDs = shop.GetSellableItemIDs();

                    int choice = 0;
                    std::cout << "판매할 아이템 번호를 입력해주세요. (0: 돌아가기) : ";
                    std::cin >> choice;
                    if (choice == 0)
                    {
                        return;
                    }

                    // 유저 입력이 0보다 작거나 판매 리스트의 사이즈 보다 클때
                    if (choice < 0 || choice > itemIDs.size())
                    {
                        std::cout << "존재하지 않는 슬롯입니다." << std::endl;
                        continue;
                    }

                    int sellCount = 0;
                    std::cout << "판매할 개수를 입력해주세요: ";
                    std::cin >> sellCount;
                    // 판매할 개수를 0이하로 입력했을때
                    if (sellCount <= 0)
                    {
                        std::cout << "잘못 입력하셨습니다." << std::endl;
                        continue;
                    }

                    // 유저가 choice한 아이템의 ID값, 아이템 정보 찾기
                    EItemID id = itemIDs.at(choice - 1);
                    const ItemData& item = ITEM_TABLE.at(id);

                    shop.SellItem(item, sellCount);
                }
                break;
            }
            case 0:
                // 메인 메뉴로 돌아가기
                SwitchState(EGameState::MAIN_MEMU);
                break;
            default:
                break;
        }
    }
}

// 여관
// 실제 회복 비용과 회복 규칙은 팀 기획이 정해지면 이 함수 안에만 추가하면 됩니다.
void Inn()
{
    while (true)
    {
        AsciiArt::Presentation::ClearScreen();
        AsciiArt::Presentation::RenderStaticScene(AsciiArt::EStaticScene::Inn);
        AsciiArt::Presentation::DrawStaticSceneMenu(
            L"[ TEAM_3 TRPG INN ]",
            inventory->GetGold(),
            {
                L"따뜻한 불빛이 비추는 여관입니다.",
                L"0. 마을로 돌아가기",
            });
        AsciiArt::Presentation::MoveCursorBelowStaticImage(2);
        std::cout << "선택해주세요: ";

        int option = -1;
        if (!(std::cin >> option))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (option == 0)
        {
            SwitchState(EGameState::MAIN_MEMU);
            return;
        }

        std::cout << "0을 입력하면 마을로 돌아갑니다.\n";
        Sleep(800);
    }
}

// 아이템 제작소
void Crafting()
{
    Crafter crafter = Crafter();

    AsciiArt::Presentation::ClearScreen();
    AsciiArt::Presentation::RenderStaticScene(AsciiArt::EStaticScene::Crafting);
    AsciiArt::Presentation::DrawStaticSceneMenu(
        L"[ TEAM_3 TRPG CRAFTING ]",
        inventory->GetGold(),
        {
            L"1. 인벤토리 확인",
            L"2. 전체 레시피 확인",
            L"3. 전체 검색",
            L"4. 제작 아이템 검색",
            L"5. 재료 아이템 검색",
            L"0. 마을로 돌아가기",
        });
    AsciiArt::Presentation::MoveCursorBelowStaticImage(2);

    std::cout << "선택해주세요: ";
    int option = 0;
    if (!(std::cin >> option))
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // 유효하지 않은 입력
    if (option < 0 || 5 < option)
    {
        return;
    }

    // 필터 초기화
    crafter.ClearFilter();

    std::string keyword;
    switch (option)
    {
        case 0:
            SwitchState(EGameState::MAIN_MEMU);
            return;
        case 1:
            inventory->ShowInventory();
            return;
        case 2:
            crafter.ClearFilter();
            break;
        case 3:
            std::cout << "전체 이름 검색하기: ";
            std::getline(std::cin, keyword);
            crafter.SetFilter(keyword, EFilterFlag::ALL_NAME);
            break;
        case 4:
            std::cout << "제작 아이템 이름 검색하기: ";
            std::getline(std::cin, keyword);
            crafter.SetFilter(keyword, EFilterFlag::ITEM_NAME);
            break;
        case 5:
            std::cout << "재료 아이템 이름 검색하기: ";
            std::getline(std::cin, keyword);
            crafter.SetFilter(keyword, EFilterFlag::INGREDIENT_NAME);
            break;
        default:
            break;
    }

    // 필터 적용
    crafter.ApplyFilter();

    // 필터링된 아이템 출력
    crafter.ShowFilteredRecipes();

    // 제작할 아이템 선택
    int craftinigItemNum = 0;
    while (true)
    {
        std::cout << "제작할 아이템 번호 입력 (0: 취소): " << std::flush;

        if (!(std::cin >> craftinigItemNum))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "숫자만 입력해주세요." << std::endl;
            continue;
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (craftinigItemNum == 0)
        {
            return;
        }

        if (craftinigItemNum < 1 || crafter.GetFilteredCraftingRecipesSize() < craftinigItemNum)
        {
            std::cout << "존재하지 않는 번호입니다." << std::endl;
            continue;
        }

        break;
    }

    // 제작 개수 입력
    int craftingCount = 0;
    while (true)
    {
        std::cout << "제작할 아이템 개수 입력 (0: 취소): " << std::flush;

        if (!(std::cin >> craftingCount))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "숫자만 입력해주세요." << std::endl;
            continue;
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (craftingCount == 0)
        {
            return;
        }

        if (craftingCount < 0)
        {
            std::cout << "0 이상의 숫자를 입력해주세요." << std::endl;
            continue;
        }

        break;
    }

    // 제작 시도 - 실제 제작한 개수 반환
    const CraftingRecipe* craftingRecipe = crafter.GetCraftingRecipeByIndex(craftinigItemNum - 1);
    int finalCraftingCount = crafter.TRY_CRAFT_ITEM(inventory, craftingRecipe, craftingCount);

    std::cout << " [" << ITEM_TABLE.at(craftingRecipe->itemID).name << "] (" << finalCraftingCount << ")개 제작 성공!" << "\n";
    inventory->ShowInventory();
}

// 게임 패배
void GameOver()
{
    rpgLogger.AddLog("상대가 너무 강하다! 일단 후퇴하자.");
    AdvanceMainMenuTimeAfterBattle();
    SwitchState(EGameState::MAIN_MEMU);
}

// 게임 승리
void GameClear()
{
    rpgLogger.AddLog("게임 클리어.");
    GameClearScreen::Show();
    IsRunning = false;
}

// 게임 실행
void Run()
{
    soundManager.Init();

    while (IsRunning)
    {
        switch (CurrentGameState)
        {
            case EGameState::START_SCREEN:
            {
                StartScreen::Render();

                const StartScreen::EAction action = StartScreen::HandleInput();

                if (action == StartScreen::EAction::StartGame)
                {
                    SwitchState(EGameState::PLAYER_INIT);
                }
                else if (action == StartScreen::EAction::ExitGame)
                {
                    IsRunning = false;
                    std::cout << "게임을 종료합니다\n";
                }

                break;
            }
            case EGameState::PLAYER_INIT:
                soundManager.PlayBGM(soundMap.at(SoundStates::INTRO));
                PlayerInit();
                break;
            case EGameState::NORMAL_BATTLE:
                soundManager.StopBGM();
                soundManager.PlayBGM(soundMap.at(SoundStates::NORMAL_BATTLE));
                NormalBattle();
                break;
            case EGameState::BOSS_BATTLE:
                soundManager.PlayBGM(soundMap.at(SoundStates::BOSS_BATTLE));
                BossBattle();
                break;
            case EGameState::MAIN_MEMU:
                soundManager.PlayBGM(soundMap.at(SoundStates::VILLAGE));
                MainMenu();
                break;
            case EGameState::SHOP:
                soundManager.PlayBGM(soundMap.at(SoundStates::SHOP));
                Shop();
                break;
            case EGameState::INN:
                soundManager.PlayBGM(soundMap.at(SoundStates::VILLAGE));
                Inn();
                break;
            case EGameState::CRAFTING:
                soundManager.PlayBGM(soundMap.at(SoundStates::VILLAGE));
                Crafting();
                break;
            case EGameState::GAME_OVER:
                GameOver();
                break;
            case EGameState::GAME_CLEAR:
                GameClear();
                break;
            default:
                break;
        }
    }

    soundManager.Shutdown();

    // 메모리 해제
    delete inventory;
    for (Player* player : players)
    {
        delete player;
    }
}

int main()
{
    // TestMonster(3);
    // ShowItemTable();

    // 이 파일은 통합 테스트용 복사본입니다. UTF-8 문자열을 콘솔이 올바르게 표시하게 합니다.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    Run();
}
