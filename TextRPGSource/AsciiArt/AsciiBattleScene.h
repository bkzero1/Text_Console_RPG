#pragma once

#include <string>
#include <vector>

// TextRPG의 Player / Monster 헤더를 포함하지 않는 화면 전용 계약입니다.
// 게임 규칙은 TextRPG에 남기고, 이 모듈은 무엇을 어디에 보여 줄지만 받습니다.
namespace AsciiArt
{
enum class EActorKind
{
    Player,
    Monster,
    Effect,
};

struct ActorVisual
{
    // 게임 객체를 다시 찾기 위한 식별자입니다. 예: "player_0", "monster_1"
    std::string id;
    EActorKind kind = EActorKind::Player;
    std::wstring imagePath;

    // SceneConfig.jsonc의 배치 값과 같은 가상 캔버스 좌표입니다.
    float x = 0.0f;
    float y = 0.0f;
    float width = 100.0f;
    float height = 100.0f;
    int layer = 0;
    bool visible = true;
};

// 게임 규칙 객체(Player, Monster)를 직접 포함하지 않고도 화면이 읽을 수 있는 전투 상태입니다.
// 실제 전투 쪽은 매 프레임 이 값만 만들어 전달합니다.
struct ActorBattleStatus
{
    std::string id;
    std::string displayName;
    int currentHp = 0;
    int maximumHp = 0;
    int previousHp = 0;
    bool isDead = false;
    // 전투 중 공격력 포션 효과가 유지되는지 HUD에 표시하기 위한 연출 전용 상태입니다.
    bool isPowerBuffed = false;
};

struct BattleSceneState
{
    std::vector<ActorVisual> players;
    std::vector<ActorVisual> monsters;
    int currentPlayerIndex = 0;
    bool useColor = true;
    bool manualAttackMode = false;
    std::vector<ActorBattleStatus> playerStatuses;
    std::vector<ActorBattleStatus> monsterStatuses;

    // 실제 전투 어댑터가 한 번의 피해/회복 결과를 잠깐만 화면에 전달합니다.
    std::string floatingTextTargetId;
    int floatingTextValue = 0;
    bool floatingTextIsHealing = false;
    // 회복과 공격력 버프는 둘 다 '+' 수치지만, 화면 연출과 색상을 다르게 보여 줍니다.
    bool floatingTextIsPowerBuff = false;
    double floatingTextAgeSeconds = 99.0;

    // 누가 행동 중인지 화면 상단에 분명히 표시합니다.
    bool isMonsterTurn = false;
    std::string turnActorName;

    // 턴 문구 아래에 남겨 둘 마지막 행동 결과입니다. (예: "피해: 54", "공격력 +10")
    std::string actionResultText;
    bool actionResultIsHealing = false;
    bool actionResultIsPowerBuff = false;

    // 임시 전투 조작 테스트 중에는 화면에 숫자 조작표를 계속 표시합니다.
    bool showTestControls = false;
};

enum class ESceneInputType
{
    None,
    SelectActor,
    RequestAttack,
    UseItem,
};

struct SceneInput
{
    ESceneInputType type = ESceneInputType::None;
    std::string actorId;
};
}
