#include "MonsterPool.h"
#include "Monster.h"

MonsterPool::MonsterPool(int poolSize)
{
    for (int i = 0; i < poolSize; i++)
    {
        Monster* monster = new Monster();
        monsterVector.push_back(monster);
        monsterSet.insert(monster);
        monsterStack.push(monster);
    }
}

MonsterPool::~MonsterPool()
{
    for (int i = 0; i < monsterVector.size(); i++)
    {
        delete monsterVector[i];
        monsterVector[i] = nullptr;
    }
}

Monster* MonsterPool::Acquire()
{
    if (monsterStack.empty())
    {
        return new Monster();
    }
    Monster* monster = monsterStack.top();
    monsterSet.erase(monster);
    return monster;
}

void MonsterPool::Release(Monster* monster)
{
    if (monsterSet.find(monster) != monsterSet.end())
    {
        return;
    }
    monsterSet.insert(monster);
    monsterStack.push(monster);
}
