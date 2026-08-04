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
        Monster* newMonster = new Monster();
        monsterVector.push_back(newMonster);
        return newMonster;
    }
    Monster* monster = monsterStack.top();
    monsterStack.pop();
    monsterSet.erase(monster);
    return monster;
}

void MonsterPool::Release(Monster* monster)
{
    if (!monster || monsterSet.find(monster) != monsterSet.end())
    {
        return;
    }
    monsterSet.insert(monster);
    monsterStack.push(monster);
}

void MonsterPool::Shrink(int poolSize)
{
    if (poolSize >= static_cast<int>(monsterVector.size()))
    {
        return;
    }

    size_t purgeCount = monsterVector.size() - poolSize;
    for (size_t i = 0; i < purgeCount; i++)
    {
        if (monsterStack.empty())
        {
            return;
        }
        Monster* monster = monsterStack.top();
        monsterStack.pop();
        monsterSet.erase(monster);
        for (auto itr = monsterVector.begin(); itr != monsterVector.end(); itr++)
        {
            if (*itr != monster)
            {
                continue;
            }
            monsterVector[i] = nullptr;
            delete monster;
            break;
        }
    }

    std::vector<Monster*> newMonsterVector;
    for (size_t i = 0; i < monsterVector.size(); i++)
    {
        Monster* monster = monsterVector[i];
        if (!monster)
        {
            continue;
        }
        newMonsterVector.push_back(monster);
    }

    monsterVector = newMonsterVector;
}
