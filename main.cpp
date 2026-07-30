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
            if (turnPlayer->GetMissingHP())
            {
                // TODO : 인벤토리에 HP 포션 있는지 확인
            }
            else if (0)
            {
                // 인벤토리에 강화 물약이 있고, 강화 하지 않았다면
                buffedPlayer.insert(turnPlayer);
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
                    // TODO : 체력 부분 targetMonster->GetName() -> targetMonster->GetHp()로 수정
                    rpgLogger.AddLog(turnPlayer->GetName() + "(이)가 " + targetMonster->GetName() + "을(를) 공격합니다! " + targetMonster->GetName() + " 체력 : " + targetMonster->GetName());
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
            std::vector<Player*> players = battleManager.GetLivingPlayers();
            std::uniform_int_distribution<int> monsterDist(0, players.size() - 1);
            Player* targetPlayer = players[monsterDist(gen)];
            battleManager.MonsterHitPlayer(targetPlayer, turnMonster->GetPower());

            // 플레이어가 죽었는지 확인
            if (targetPlayer->IsDead())
            {
                rpgLogger.AddLog(turnMonster->GetName() + "(이)가 " + turnMonster->GetName() + "을(를) 공격합니다! " + targetPlayer->GetName() + "(이)가 전투불능!");
                break;
            }
            else
            {
                // TODO : 체력 부분 targetPlayer->GetName() -> targetPlayer->GetHp()로 수정
                rpgLogger.AddLog(turnMonster->GetName() + "(이)가 " + turnMonster->GetName() + "을(를) 공격합니다! " + targetPlayer->GetName() + " 체력 : " + targetPlayer->GetName());
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
    //TODO 플레이어들 추가
    //battleManager.AddPlayer();

    //TODO 플레이어들 레벨 평균 값으로 바꿀 것
    int avgLv = 5;

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
    // TODO 플레이어들 추가
    // battleManager.AddPlayer();

    // TODO 플레이어들 레벨 평균 값으로 바꿀 것
    int avgLv = 5;

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