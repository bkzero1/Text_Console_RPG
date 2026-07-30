#pragma once
#include <map>
#include <queue>
#include <string>

#include "FMonsterData.h"

class RpgLogger
{
   public:
    // Queue에 쌓인 전체 로그 출력
    void ShowLogs() const;

    // 킬 로그 출력
    void ShowKillLogs() const;

    // Queue에 로그 추가(10개 까지), 해당 로그 바로 출력
    void AddLog(std::string log);

    // 몬스터를 처치했을 때 호출, 킬카운트 증가
    void OnMonsterKilled(EMonsterID id);

   private:
    static constexpr size_t MAX_SIZE = 10;
    std::queue<std::string> logs;
    std::map<EMonsterID, int> killCounts;
};

void TestRpgLogger();