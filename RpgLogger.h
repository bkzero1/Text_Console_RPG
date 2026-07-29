#pragma once
#include <string>
#include <queue>
#include <map>

enum class EMonsterID
{
    NONE
};

class RpgLogger
{
    int maxSize = 10;
    std::queue<std::string> logs;
    std::map<EMonsterID, int> killCounts;

    void showLogs();
    void showKillLogs();

    void AddLog(std::string log);
    void OnMonsterKilled(EMonsterID id);

};