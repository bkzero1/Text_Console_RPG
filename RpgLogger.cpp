#include "RpgLogger.h"

#include <iostream>

void RpgLogger::ShowLogs() const
{
    std::cout << "------------- Recent Logs -------------" << std::endl;

    auto QueueCopy = logs;
    while (!QueueCopy.empty())
    {
        std::cout << QueueCopy.front() << "\n";
        QueueCopy.pop();
    }

    std::cout << "---------------------------------------" << std::endl;
}

void RpgLogger::ShowKillLogs() const
{
    std::cout << "------------- Monster Kill Count -------------" << std::endl;

    for (auto& [key, value] : killCounts)
    {
        std::cout << MONSTER_MAP.at(key).name << "을 " << value << "마리 처치하였습니다." << std::endl;
    }

    std::cout << "----------------------------------------------" << std::endl;
}

std::vector<std::string> RpgLogger::GetKillLogLines() const
{
    std::vector<std::string> killLogLines;

    if (killCounts.empty())
    {
        killLogLines.push_back("아직 처치한 몬스터가 없습니다.");
        return killLogLines;
    }

    for (const auto& [monsterID, count] : killCounts)
    {
        killLogLines.push_back(MONSTER_MAP.at(monsterID).name + " : " + std::to_string(count) + "마리 처치");
    }

    return killLogLines;
}

void RpgLogger::AddLog(std::string log, bool printToConsole)
{
    logs.push(log);

    if (logs.size() > MAX_SIZE)
    {
        logs.pop();
    }

    if (printToConsole)
    {
        std::cout << log << std::endl;
    }
}

void RpgLogger::OnMonsterKilled(EMonsterID id)
{
    killCounts[id]++;
}

void TestRpgLogger()
{
    RpgLogger logger;
    logger.AddLog("캐릭터를 생성했어요!");
    logger.AddLog("고블린 몬스터가 등장했어요!");
    logger.AddLog("플레이어가 몬스터를 공격합니다!");
    logger.AddLog("몬스터가 플레이어를 공격합니다!");
    logger.AddLog("플레이어가 몬스터를 공격합니다!");
    logger.AddLog("몬스터가 플레이어를 공격합니다!");
    logger.AddLog("플레이어가 몬스터를 공격합니다!");
    logger.AddLog("플레이어가 몬스터를 처치했습니다~!!!");
    logger.OnMonsterKilled(EMonsterID::GOBLIN);
    // logger.OnMonsterKilled(EMonsterID::DRACULA);

    logger.ShowLogs();

    logger.ShowKillLogs();

    for (int i = 0; i < 15; i++)
    {
        logger.AddLog(std::to_string(i) + "번째 로그");
    }

    logger.ShowLogs();
}
