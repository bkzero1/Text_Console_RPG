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

    // Queue에 로그 추가(10개 까지). 기본값은 기존처럼 즉시 콘솔에도 출력합니다.
    // AA 전투처럼 화면을 직접 그리는 곳에서는 false로 넘겨 로그만 기록합니다.
    void AddLog(std::string log, bool printToConsole = true);

    // 몬스터를 처치했을 때 호출, 킬카운트 증가
    void OnMonsterKilled(EMonsterID id);

   private:
    static constexpr size_t MAX_SIZE = 10;
    std::queue<std::string> logs;
    std::map<EMonsterID, int> killCounts;
};

void TestRpgLogger();
