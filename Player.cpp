#include "Player.h"

#include <iostream>

// 캐릭터 생성 시 이름만 입력받고, 나머지는 기본값으로 시작
Player::Player(const string& playerName)
    : name(playerName), level(1), expMax(100), exp(0), power(30), hpMax(200), hp(200)
{
}

string Player::GetName()  // 이름 얻기
{
    return name;
}

int Player::GetLevel()  // 레벨 얻기
{
    return level;
}

void Player::AddExp(int exp)  // 경험치 추가
{
    this->exp += exp;

    // 경험치가 expMax를 넘을 때마다 반복 체크 (한 번에 여러 레벨업 가능하도록 while)
    while (this->exp >= expMax && level < MAX_LEVEL)
    {
        this->exp -= expMax;
        LevelUp();
    }
}

void Player::TakeDamage(int damage)  // 피격
{
    hp -= damage;
    if (hp < 0)
    {
        hp = 0;  // 체력이 음수로 내려가지 않도록 방지
    }
}

int Player::GetPower()
{
    return power;
}

int Player::GetHpMax()  // 최대 체력 얻기
{
    return hpMax;
}

int Player::GetHp()  // 현재 체력 얻기
{
    return hp;
}

void Player::ShowStatus()  // 정보 출력 (이 함수만 콘솔 출력 담당)
{
    cout << "==================== 캐릭터 정보 ====================\n";
    cout << "이름       : " << name << "\n";
    cout << "레벨       : " << level << " / " << MAX_LEVEL << "\n";
    cout << "체력       : " << hp << " / " << hpMax << "\n";
    cout << "공격력     : " << power << "\n";
    cout << "경험치     : " << exp << " / " << expMax << "\n";
    cout << "======================================================\n";
}

void Player::HealHP(int hp)  // 체력 회복
{
    this->hp += hp;
    if (this->hp > hpMax)
    {
        this->hp = hpMax;  // 최대체력을 넘지 않도록 방지
    }
}

bool Player::IsDead()
{
    return hp <= 0;
}

bool Player::IsFullHP()  // 최대체력 여부
{
    return hp == hpMax;
}

int Player::GetMissingHP()  // 부족한 체력 값
{
    return hpMax - hp;
}

void Player::AddPower(int power)  // 공격력 증가 (아이템 버프, 파티 레벨업 등에 사용)
{
    this->power += power;
}

void Player::RemovePower(int power)  // 공격력 되돌리기 (버프 종료 시 아이템 쪽에서 호출)
{
    this->power -= power;
    if (this->power < 0)
    {
        this->power = 0;  // 혹시 모를 상황 대비, 공격력이 음수로 내려가지 않도록 방지
    }
}

// 레벨업 처리 (외부 직접 호출 없이 AddExp 내부에서만 사용)
void Player::LevelUp()
{
    if (level >= MAX_LEVEL)
    {
        return;  // 최대 레벨에 도달하면 더 이상 레벨업 안함
    }

    level++;
    hpMax += 20;
    hp = hpMax;
    power += 5;

    //레벨업 할때마다 메시지 출력
    cout << "\n🎉 " << name << "(이)가 레벨업 했다! (레벨 " << level << ")\n";
    cout << "체력 : " << hpMax << " | 공격력 : " << power << "\n\n";

    // 레벨 10 도달 시 메시지 출력
    if (level == MAX_LEVEL)
    {
        cout << "\n 🌟 " << name << "(이)가 레벨 10에 도달했다!\n";
        cout << "이제 일반 몬스터는 상대도 안된다!\n\n";
    }
}