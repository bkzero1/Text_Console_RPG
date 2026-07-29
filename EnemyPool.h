#pragma once

#include <stack>
#include <vector>

class Enemy;

class EnemyPool
{
public:
	EnemyPool(int poolSize=5);
	~EnemyPool();
	EnemyPool(const EnemyPool&) = delete;

	Enemy* Acquire();
	void Release(Enemy* enemy);
private:
	std::stack<Enemy*> enemyStack;
	std::vector<Enemy*> enemyVector;
};

