#include "RpgLogger.h"

#include <iostream>

// 임시 작성
namespace Test
{
enum class EMonsterID
{
    NONE,
    GOBLIN,
    ORC
};
std::map<EMonsterID, std::string> convertMonsterString = {
    {EMonsterID::NONE, "NONE"}, {EMonsterID::GOBLIN, "GOBLIN"}, {EMonsterID::ORC, "ORK"}};
}  // namespace Test

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
        std::cout << Test::convertMonsterString.at(key) << "을 " << value << "마리 처치하였습니다." << std::endl;
    }

    std::cout << "----------------------------------------------" << std::endl;
}

void RpgLogger::AddLog(std::string log)
{
    logs.push(log);

    if (logs.size() > MAX_SIZE)
    {
        logs.pop();
    }

    std::cout << log << std::endl;
}

void RpgLogger::OnMonsterKilled(Test::EMonsterID id)
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
    logger.AddLog("플레이어가 몬스터를 처치했습니다~!!");
    logger.OnMonsterKilled(Test::EMonsterID::GOBLIN);

    logger.ShowLogs();

    logger.ShowKillLogs();

    for (int i = 0; i < 15; i++)
    {
        logger.AddLog(std::to_string(i) + "번째 로그");
    }

    logger.ShowLogs();
}
