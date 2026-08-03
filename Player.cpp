#include "Player.h"

#include <iostream>

Player::Player(const string& playerName)
    : name(playerName), level(1), expMax(100), exp(0), power(30), hpMax(200), hp(200)
{
}

string Player::GetName() const
{
    return name;
}

int Player::GetLevel() const
{
    return level;
}

void Player::AddExp(int exp)
{
    this->exp += exp;

    while (this->exp >= expMax && level < MAX_LEVEL)
    {
        this->exp -= expMax;
        LevelUp();
    }
}

void Player::TakeDamage(int damage)
{
    hp -= damage;
    if (hp < 0)
    {
        hp = 0;
    }
}

int Player::GetPower() const
{
    return power;
}

int Player::GetHpMax() const
{
    return hpMax;
}

int Player::GetHp() const
{
    return hp;
}

void Player::ShowStatus() const
{
    cout << "==================== 캐릭터 정보 ====================\n";
    cout << "이름       : " << name << "\n";
    cout << "레벨       : " << level << " / " << MAX_LEVEL << "\n";
    cout << "체력       : " << hp << " / " << hpMax << "\n";
    cout << "공격력     : " << power << "\n";
    cout << "경험치     : " << exp << " / " << expMax << "\n";
    cout << "======================================================\n";
}

void Player::HealHP(int hp)
{
    this->hp += hp;
    if (this->hp > hpMax)
    {
        this->hp = hpMax;
    }
}

bool Player::IsDead() const
{
    return hp <= 0;
}

bool Player::IsFullHP() const
{
    return hp == hpMax;
}

int Player::GetMissingHP() const
{
    return hpMax - hp;
}

void Player::AddPower(int power)
{
    this->power += power;
}

void Player::RemovePower(int power)
{
    this->power -= power;
    if (this->power < 0)
    {
        this->power = 0;
    }
}

// 메시지 출력 함수 (따로 분리)
void Player::PrintLevelUpMessage()
{
    cout << "\n  " << name << "(이)가 레벨 " << level << "로 레벨업 했다!\n";
    cout << "체력: " << hpMax << " | 공격력: " << power << "\n\n";

    if (level == MAX_LEVEL)
    {
        cout << " " << name << "(이)가 레벨 10에 도달했다!\n";
        cout << "이제 일반 몬스터는 상대도 안된다!\n\n";
    }
}

// 부모 클래스 기본 레벨업 (메시지 없음)
void Player::LevelUp()
{
    level++;
    hpMax += 20;
    hp = hpMax;
    power += 5;
}