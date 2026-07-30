#include <iostream>
#include <random>
#include <thread>
#include <chrono>

#include "Monster.h"
#include "Player.h"
#include "BattleManager.h"
#include "MonsterPool.h"
#include "FMonsterData.h"

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

// 게임 상태 전환
void SwitchState(EGameState newGameState)
{
    CurrentGameState = newGameState;
}

// 캐릭터 생성
void PlayerInit()
{
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
                    monsterPool.Release(targetMonster);
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
            std::vector<Player*> monster = battleManager.GetLivingPlayers();
            std::uniform_int_distribution<int> monsterDist(0, monster.size() - 1);
            battleManager.MonsterHitPlayer(monster[monsterDist(gen)], turnMonster->GetPower());

            // 모든 플레이어가 다 죽었는지 확인
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
        //TODO 로거에 나노리 전달
        battleManager.AddMonster(monster);
    }

    bool isWin = BattlePhase(battleManager, monsterPool);

    if (!isWin)
    {
        CurrentGameState = EGameState::GAME_OVER;
        return;
    }

    //TODO 전리품 인벤토리에

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
        // TODO 로거에 나노리 전달
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
}

int main()
{
    Run();
}