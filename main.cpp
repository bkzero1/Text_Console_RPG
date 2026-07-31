#include <chrono>
#include <iostream>
#include <random>
#include <thread>

#include "BattleManager.h"
#include "FMonsterData.h"
#include "Inventory.h"
#include "ItemUseHandler.h"
#include "Mage.h"
#include "Monster.h"
#include "MonsterPool.h"
#include "Player.h"
#include "RpgLogger.h"
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
                std::string itemName = hpPotion.name;
                rpgLogger.AddLog(turnPlayer->GetName() + "(이)가 " + itemName + "을(를) 사용 체력 : " + to_string(turnPlayer->GetHp()));
            }
            else if (buffedPlayer.find(turnPlayer) == buffedPlayer.end() && buffItr != consumableItems.end())
            {
                inventory->ConsumeItem(EItemID::POWER_POTION);
                itemHandler.USE_ITEM(turnPlayer, EItemID::POWER_POTION);
                buffedPlayer.insert(turnPlayer);

                ItemData powerPotion = ITEM_TABLE.at(EItemID::HP_POTION);
                std::string itemName = powerPotion.name;
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

    // 랜덤 준비
    std::random_device rd;
    std::mt19937 gen(rd());
    // ENum에서 랜덤 값 가져오기 위한 준비
    std::uniform_int_distribution<int> deployDist(
        1,
        static_cast<int>(EMonsterID::MAX) - 1);

    for (int i = 0; i < monsterCount; i++)
    {
        Monster* monster = monsterPool.Acquire();
        //EMonsterID randomMonster = static_cast<EMonsterID>(deployDist(gen));
        EMonsterID randomMonster = EMonsterID::GOBLIN;
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
        1,
        static_cast<int>(EMonsterID::MAX) - 1);

    for (int i = 0; i < monsterCount; i++)
    {
        Monster* monster = monsterPool.Acquire();
        // EMonsterID randomMonster = static_cast<EMonsterID>(deployDist(gen));
        EMonsterID randomMonster = EMonsterID::GOBLIN;
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
    std::cout << "========================================" << "\n";
    std::cout << " 1. 전투" << "\n";
    std::cout << " 2. 상점" << "\n";
    std::cout << "========================================" << "\n";
    std::cout << "입력: ";
    int option;
    std::cin >> option;

    // 유효하지 않은 입력
    if (option < 1 || 2 < option)
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
        case 1:
            if (avgLv < 10)
            {
                SwitchState(EGameState::NORMAL_BATTLE);
            }
            else
            {
                SwitchState(EGameState::BOSS_BATTLE);
            }
            break;
        case 2:
            SwitchState(EGameState::SHOP);
            break;
        default:
            break;
    }
}

// 상점
void Shop()
{
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