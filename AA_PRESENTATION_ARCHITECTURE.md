# AA 연출 구조

## main.cpp가 맡는 일

기본 코드에서는 전투를 만들고, 결과에 따라 게임 상태를 전환합니다. 전투 화면을 직접 그리거나 마우스 입력을 해석하지 않습니다.

```cpp
const bool isVictory = AsciiArt::RunBattlePresentation(battleManager, monsterPool, rpgLogger);
```

`BattleManager`, `MonsterPool`, `RpgLogger` 세 객체만 넘기면 됩니다. 메인 메뉴의 AA 출력도 `AsciiArt::Presentation` 이름공간의 간단한 함수만 호출합니다.

## AsciiBattleBridge가 맡는 일

`TextRPGSource/AsciiArt/AsciiBattleBridge.cpp`는 기본 전투 객체와 AA 연출 사이의 어댑터입니다.

- `BattleManager`의 플레이어/몬스터 HP를 `BattleSceneState`로 변환합니다.
- 클릭·자동 공격 같은 화면 입력을 기존 `BattleManager` 공격 함수에 연결합니다.
- 피해 수치, 턴 문구, HP 애니메이션에 필요한 이전 HP를 관리합니다.
- 몬스터가 죽으면 기존 `MonsterPool`에 반환합니다.

따라서 전투 규칙을 바꾸려면 기본 코드 쪽을, 화면·배치·효과를 바꾸려면 `AsciiArt` 폴더 쪽을 수정하면 됩니다.

## AA 렌더러가 맡는 일

`AsciiBattleDemo.cpp`는 이미지를 점 문자와 ANSI 색으로 그리며 배치 모드, 슬라이더, 공격/피격 효과를 담당합니다. 이 파일은 전투 데미지 계산이나 보상 계산을 직접 하지 않습니다.

## 앞으로 새 화면을 붙이는 방식

상점·시작·승리 화면처럼 새 AA 화면도 `TextRPGSource/AsciiArt` 안에 화면 파일과 작은 진입 함수 하나를 둡니다. `main.cpp`에서는 화면이 끝난 뒤 필요한 선택 결과만 받아 상태를 바꾸는 방식으로 유지합니다.
