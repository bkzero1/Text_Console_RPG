#include <iostream>
#include "RpgLogger.h"

void RpgLogger::showLogs()
{
    std::cout << "전체 로그 출력" << std::endl;
}

void RpgLogger::showKillLogs()
{
    std::cout << "킬 로그 출력" << std::endl;
}

void RpgLogger::AddLog(std::string log)
{
    // 로그 추가
    std::cout << log << std::endl;
}

void RpgLogger::OnMonsterKilled(EMonsterID id)
{
    std::cout << "몬스터 처치 횟수 증가" << std::endl;
}