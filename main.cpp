#include <iostream>

#include "Monster.h"

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

// 일반 전투
void NormalBattle()
{
}

// 보스 전투
void BossBattle()
{
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