#pragma once

#include <stack>
#include <vector>
#include <set>

class Monster;

class MonsterPool
{
public:
    MonsterPool(int poolSize = 5);
	~MonsterPool();

	Monster* Acquire();
    void Release(Monster* monster);
    void Shrink(int poolSize);

private:
	std::stack<Monster*> monsterStack;
	std::vector<Monster*> monsterVector;
    std::set<Monster*> monsterSet;
};


