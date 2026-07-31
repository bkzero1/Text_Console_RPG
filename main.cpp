#include <chrono>
#include <iostream>
#include <random>
#include <thread>

#include "BattleManager.h"
#include "FMonsterData.h"
#include "RpgLogger.h"
#include "Inventory.h"
#include "Monster.h"
#include "MonsterPool.h"
#include "Player.h"
#include "ItemUseHandler.h"
#include "ShopManager.h"

// 게임 상태
enum class EGameState
{
    PLAYER_INIT,    // 캐릭터 생성 (이름, 스탯 초기 설정)
    NORMAL_BATTLE,  // 일반 전투
    BOSS_BATTLE,    // 보스 전투
    MAIN_MEMU,      // 메인 메뉴
    SHOP,           // 상점
    GAME_OVER,      // 게임 패배
    GAME_CLEAR,     // 게임 승리
};

// 전역 변수
EGameState CurrentGameState = EGameState::PLAYER_INIT;
bool IsRunning = true;
RpgLogger rpgLogger;

Inventory* inventory;  // 인벤토리
std::vector<Player*> players; // 플레이어 목록
// 게임 상태 전환
void SwitchState(EGameState newGameState)
{
    CurrentGameState = newGameState;
}

// 캐릭터 생성
void PlayerInit()
{
    // 캐릭터 생성

    // 인벤토리 생성
    inventory = new Inventory();
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
            if (potionItr != consumableItems.end() 
                && turnPlayer->GetMissingHP() >= turnPlayer->GetHp())
            {
                inventory->ConsumeItem(EItemID::HP_POTION);
                itemHandler.USE_ITEM(turnPlayer, EItemID::HP_POTION);
                auto tableItr = ITEM_TABLE.find(EItemID::HP_POTION);
                std::string itemName = tableItr->second.name;
                rpgLogger.AddLog(turnPlayer->GetName() + "(이)가 " + itemName + "을(를) 사용 체력 : " + to_string(turnPlayer->GetHp()));
            }
            else if (buffedPlayer.find(turnPlayer) == buffedPlayer.end()
                && buffItr != consumableItems.end())
            {
                inventory->ConsumeItem(EItemID::POWER_POTION);
                itemHandler.USE_ITEM(turnPlayer, EItemID::POWER_POTION);
                buffedPlayer.insert(turnPlayer);

                auto tableItr = ITEM_TABLE.find(EItemID::POWER_POTION);
                std::string itemName = tableItr->second.name;
                rpgLogger.AddLog(turnPlayer->GetName() + "(이)가 " + itemName + "을(를) 사용 공격력 : " + to_string(turnPlayer->GetPower()));
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
                rpgLogger.AddLog(turnMonster->GetName() + "(이)가 " + turnMonster->GetName() + "을(를) 공격합니다! " + targetPlayer->GetName() + "(이)가 전투불능!");
                break;
            }
            else
            {
                rpgLogger.AddLog(turnMonster->GetName() + "(이)가 " + turnMonster->GetName() + "을(를) 공격합니다! " + targetPlayer->GetName() + " 체력 : " + to_string(targetPlayer->GetHp()));
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
    return isWin;
}

// 일반 전투
void NormalBattle()
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

    int monsterCount = std::max(1, avgLv / 2);

    //랜덤 준비
    std::random_device rd;
    std::mt19937 gen(rd());
    //ENum에서 랜덤 값 가져오기 위한 준비
    std::uniform_int_distribution<int> deployDist(
        0,
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
        CurrentGameState = EGameState::GAME_OVER;
        return;
    }

    // 전리품 인벤토리에 추가
    inventory->AddGold(battleManager.GetEarnGold());                                            // 골드 획득
    std::map<EItemID, int> remainingItems = inventory->AddItems(battleManager.GetEarnItems());  // 아이템 획득
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
    rpgLogger.AddLog("파티는 " + to_string(battleManager.GetEarnExp()) + " exp 를 얻었다");
    battleManager.BattleEnd(isWin);

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
        0,
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
        CurrentGameState = EGameState::GAME_OVER;
        return;
    }
    else 
    {
        CurrentGameState = EGameState::GAME_CLEAR;
        return;
    }
}

// 메인 메뉴
void MainMenu()
{
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
                    if (choice == 0) { return; }

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


// 게임 패배
void GameOver()
{
}

// 게임 승리
void GameClear()
{
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
    Run();
}