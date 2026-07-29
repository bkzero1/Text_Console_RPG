#include "EnemyPool.h"

EnemyPool::EnemyPool(int poolSize)
{
    for (int i = 0; i < poolSize; i++)
    {
    }
}

EnemyPool::~EnemyPool()
{
    for (int i = 0; i < enemyVector.size(); i++)
    {
        delete enemyVector[i];
        enemyVector[i] = nullptr;
    }
}

Enemy* EnemyPool::Acquire()
{
    if (enemyStack.empty())
    {
        return nullptr;
    }
    return enemyStack.top();
}

void EnemyPool::Release(Enemy* enemy)
{
    enemyStack.push(enemy);
}
