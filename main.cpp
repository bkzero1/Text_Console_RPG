#include <chrono>
#include <iostream>
#include <random>
#include <thread>

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

// 게임 상태
enum class EGameState
{
    PLAYER_INIT,    // 캐릭터 생성 (이름, 스탯 초기 설정)
    NORMAL_BATTLE,  // 일반 전투
    BOSS_BATTLE,    // 보스 전투
    MAIN_MEMU,      // 메인 메뉴
    SHOP,           // 상점
    CRAFTING,       // 아이템 제작소
    GAME_OVER,      // 게임 패배
    GAME_CLEAR,     // 게임 승리
};

// 전역 변수
EGameState CurrentGameState = EGameState::PLAYER_INIT;
bool IsRunning = true;
RpgLogger rpgLogger;

Inventory* inventory;          // 인벤토리
std::vector<Player*> players;  // 플레이어 목록
// 게임 상태 전환
void SwitchState(EGameState newGameState)
{
    CurrentGameState = newGameState;
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
    std::cout << "전사의 이름을 입력하세요: ";
    std::getline(std::cin, playerName);
    players.push_back(new Warrior(playerName));

    // 마법사 생성
    std::cout << "마법사의 이름을 입력하세요: ";
    std::getline(std::cin, playerName);
    players.push_back(new Mage(playerName));

    // 탱커 생성
    std::cout << "탱커의 이름을 입력하세요: ";
    std::getline(std::cin, playerName);
    players.push_back(new Tank(playerName));

    // 인벤토리 생성
    inventory = new Inventory();
    // 첫 전투 시작
    SwitchState(EGameState::NORMAL_BATTLE);
}

bool BattlePhase(BattleManager& battleManager, MonsterPool& monsterPool)
{
    ItemUseHandler itemHandler;
    std::set<Player*> buffedPlayer;
    bool isWin = false;
    // 랜덤 준비
    std::random_device rd;
    std::mt19937 gen(rd());
    while (true)
    {
        // 플레이어 턴 시작
        std::vector<Player*> turnPlayers = battleManager.GetLivingPlayers();
        for (int i = 0; i < turnPlayers.size(); i++)
        {
            Player* turnPlayer = turnPlayers[i];
            std::map<EItemID, int> consumableItems = inventory->GetConsumableItems();

            auto potionItr = consumableItems.find(EItemID::HP_POTION);
            auto buffItr = consumableItems.find(EItemID::POWER_POTION);
            if (potionItr != consumableItems.end() && turnPlayer->GetMissingHP() >= turnPlayer->GetHp())
            {
                inventory->ConsumeItem(EItemID::HP_POTION);
                itemHandler.USE_ITEM(turnPlayer, EItemID::HP_POTION);
                ItemData hpPotion = ITEM_TABLE.at(EItemID::HP_POTION);
                rpgLogger.AddLog(turnPlayer->GetName() + "(이)가 " + hpPotion.name + "을(를) 사용 체력 : " + to_string(turnPlayer->GetHp()));
            }
            else if (buffedPlayer.find(turnPlayer) == buffedPlayer.end() && buffItr != consumableItems.end())
            {
                inventory->ConsumeItem(EItemID::POWER_POTION);
                itemHandler.USE_ITEM(turnPlayer, EItemID::POWER_POTION);
                buffedPlayer.insert(turnPlayer);

                ItemData powerPotion = ITEM_TABLE.at(EItemID::POWER_POTION);
                rpgLogger.AddLog(turnPlayer->GetName() + "(이)가 " + powerPotion.name + "을(를) 사용 공격력 : " + to_string(turnPlayer->GetPower()));
            }
            else
            {
                std::vector<Monster*> monster = battleManager.GetLivingMonsters();
                std::uniform_int_distribution<int> monsterDist(0, monster.size() - 1);
                Monster* targetMonster = monster[monsterDist(gen)];
                battleManager.PlayerHitMonster(targetMonster, turnPlayer->GetPower());
                if (targetMonster->IsDead())
                {
                    rpgLogger.AddLog(turnPlayer->GetName() + "(이)가 " + targetMonster->GetName() + "을(를) 공격합니다! " + targetMonster->GetName() + " 처치!");
                    monsterPool.Release(targetMonster);
                }
                else
                {
                    rpgLogger.AddLog(turnPlayer->GetName() + "(이)가 " + targetMonster->GetName() + "을(를) 공격합니다! " + targetMonster->GetName() + " 체력 : " + to_string(targetMonster->GetHp()));
                }
            }

            // 모든 몬스터가 다 죽었는지 확인
            if (battleManager.IsMonstersDead())
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));  // 1초 대기
        }
        // 플레이어 턴 종료
        if (battleManager.IsMonstersDead())
        {
            isWin = true;
            break;
        }

        // 몬스터 턴 시작
        std::vector<Monster*> turnMonsters = battleManager.GetLivingMonsters();
        for (int i = 0; i < turnMonsters.size(); i++)
        {
            Monster* turnMonster = turnMonsters[i];
            std::vector<Player*> livingPlayers = battleManager.GetLivingPlayers();
            std::uniform_int_distribution<int> monsterDist(0, livingPlayers.size() - 1);
            Player* targetPlayer = livingPlayers[monsterDist(gen)];
            battleManager.MonsterHitPlayer(targetPlayer, turnMonster->GetPower());

            // 플레이어가 죽었는지 확인
            if (targetPlayer->IsDead())
            {
                rpgLogger.AddLog(turnMonster->GetName() + "(이)가 " + targetPlayer->GetName() + "을(를) 공격합니다! " + targetPlayer->GetName() + "(이)가 전투불능!");
            }
            else
            {
                rpgLogger.AddLog(turnMonster->GetName() + "(이)가 " + targetPlayer->GetName() + "을(를) 공격합니다! " + targetPlayer->GetName() + " 체력 : " + to_string(targetPlayer->GetHp()));
            }

            // 플레이어들이 다 죽었는지 확인
            if (battleManager.IsPlayersDead())
            {
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));  // 1초 대기
        }

        // 플레이어들이 다 죽었는지 확인
        if (battleManager.IsPlayersDead())
        {
            break;
        }
    }
    for (auto itr = buffedPlayer.begin(); itr != buffedPlayer.end(); itr++)
    {
        itemHandler.CLEAR_BUFF(*itr, EItemID::POWER_POTION);
    }
    return isWin;
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

    int avgLv = totalLv / players.size();

    int monsterCount = std::max(1, avgLv / 2);

    // 랜덤 준비
    std::random_device rd;
    std::mt19937 gen(rd());
    // ENum에서 랜덤 값 가져오기 위한 준비
    std::uniform_int_distribution<int> deployDist(
        static_cast<int>(EMonsterID::NONE) + 1,
        static_cast<int>(EMonsterID::MAX) - 1);

    for (int i = 0; i < monsterCount; i++)
    {
        Monster* monster = monsterPool.Acquire();
        EMonsterID randomMonster = static_cast<EMonsterID>(deployDist(gen));
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
            players[i]->HealHP(players[i]->GetHpMax());
        }
        CurrentGameState = EGameState::GAME_OVER;
        return;
    }

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

    // 전투 후 레벨 확인
    totalLv = 0;
    for (int i = 0; i < players.size(); i++)
    {
        totalLv += players[i]->GetLevel();
    }
    avgLv = totalLv / players.size();

    while (true)
    {
        std::cout << "마을로 돌아가겠습니까? [Y/N]" << endl;
        string answer;
        std::cin >> answer;
        if (StringCompare(answer, "Y"))
        {
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

    int avgLv = totalLv / players.size();

    int monsterCount = 1;

    // 랜덤 준비
    std::random_device rd;
    std::mt19937 gen(rd());
    // ENum에서 랜덤 값 가져오기 위한 준비
    std::uniform_int_distribution<int> deployDist(
        static_cast<int>(EMonsterID::NONE) + 1,
        static_cast<int>(EMonsterID::MAX) - 1);

    for (int i = 0; i < monsterCount; i++)
    {
        Monster* monster = monsterPool.Acquire();
        EMonsterID randomMonster = static_cast<EMonsterID>(deployDist(gen));
        std::string nanori = monster->Deploy(randomMonster, avgLv, true);
        rpgLogger.AddLog(nanori);
        battleManager.AddMonster(monster);
    }

    bool isWin = BattlePhase(battleManager, monsterPool);

    if (!isWin)
    {
        // 패배자들 체력 회복 시키고 내쫓기
        for (int i = 0; i < players.size(); i++)
        {
            players[i]->HealHP(players[i]->GetHpMax());
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
    std::cout << "========================================" << "\n";
    std::cout << " 1. 전투" << "\n";
    std::cout << " 2. 상점" << "\n";
    std::cout << " 3. 아이템 제작소" << "\n";
    std::cout << " 4. 플레이어 정보 확인" << "\n";
    std::cout << " 5. 몬스터 처치 기록 확인" << "\n";
    std::cout << "========================================" << "\n";
    std::cout << "입력: ";
    int option;
    std::cin >> option;
    // 유효하지 않은 입력
    if (option < 1 || 5 < option)
    {
        return;
    }

    // 플레이어 레벨
    int totalLv = 0;
    for (int i = 0; i < players.size(); i++)
    {
        totalLv += players[i]->GetLevel();
    }
    int avgLv = totalLv / players.size();

    // 상태 전이
    switch (option)
    {
        case 1:  //  전투
            if (avgLv < 10)
            {
                SwitchState(EGameState::NORMAL_BATTLE);
            }
            else
            {
                SwitchState(EGameState::BOSS_BATTLE);
            }
            break;
        case 2:  // 상점
            SwitchState(EGameState::SHOP);
            break;
        case 3:  // 아이템 제작소
            SwitchState(EGameState::CRAFTING);
            break;
        case 4:  // 플레이어 정보
            for (const auto& player : players)
                player->ShowStatus();
            break;
        case 5:  // 몬스터 처치 기록
            // 킬 몬스터 로그 출력 메뉴
            break;
        default:
            break;
    }
}

// 상점
void Shop()
{
    ShopManager shop;
    std::cout << "---------- TEAM_3 TRPG SHOP ----------" << std::endl;
    std::cout << "1. 아이템 구매" << std::endl;
    std::cout << "2. 아이템 판매" << std::endl;
    std::cout << "0. 돌아가기" << std::endl;
    std::cout << "보유 골드: " << inventory->GetGold() << std::endl;
    std::cout << "선택해주세요: ";
    int select = 0;
    std::cin >> select;

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

// 아이템 제작소
void Crafting()
{
    Crafter crafter = Crafter();

    std::cout << "=========== [ 아이템 제작소 ] ===========" << "\n";
    std::cout << " 1. 인벤토리 확인" << "\n";
    std::cout << " 2. 전체 레시피 확인" << "\n";
    std::cout << " 3. 전체 검색" << "\n";
    std::cout << " 4. 제작 아이템 검색" << "\n";
    std::cout << " 5. 재료 아이템 검색" << "\n";
    std::cout << "----------------------------------------" << "\n";
    std::cout << " 0. 마을로 돌아가기" << "\n";
    std::cout << "========================================" << "\n";

    std::cout << "입력: ";
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
    SwitchState(EGameState::MAIN_MEMU);
}

// 게임 승리
void GameClear()
{
    rpgLogger.AddLog("게임 클리어.");
    IsRunning = false;
}

// 게임 실행
void Run()
{
    while (IsRunning)
    {
        switch (CurrentGameState)
        {
            case EGameState::PLAYER_INIT:
                PlayerInit();
                break;
            case EGameState::NORMAL_BATTLE:
                NormalBattle();
                break;
            case EGameState::BOSS_BATTLE:
                BossBattle();
                break;
            case EGameState::MAIN_MEMU:
                MainMenu();
                break;
            case EGameState::SHOP:
                Shop();
                break;
            case EGameState::CRAFTING:
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

    // 메모리 해제
    delete inventory;
}

int main()
{
    // TestMonster(3);
    // ShowItemTable();
    Run();
}