# TextRPG 연결 준비 문서

## 목적

아스키 화면은 **표시와 클릭 입력**만 담당합니다. 실제 체력 감소, 사망, 보상은
기존 `BattleManager`, `Player`, `Monster`가 계속 담당합니다.

## 화면 모듈이 받을 데이터

`AsciiBattleScene.h`의 `BattleSceneState`에 다음만 채워 전달합니다.

- `players`: 살아 있는 플레이어의 식별자, 이미지 파일, 배치 값
- `monsters`: 살아 있는 몬스터의 식별자, 이미지 파일, 배치 값
- `currentPlayerIndex`: 현재 행동할 플레이어의 순서

여기에는 `Player*`, `Monster*`를 넣지 않습니다. 화면 모듈이 게임 객체의
소유권을 갖지 않게 하기 위해서입니다.

## 화면 모듈이 돌려줄 입력

클릭한 몬스터는 `SceneInput`으로만 TextRPG에 알립니다.

```text
type    = RequestAttack
actorId = "monster_1"
```

TextRPG의 전투 루프가 `actorId`에 해당하는 기존 `Monster*`를 찾아
`BattleManager::PlayerHitMonster()`를 호출합니다. 따라서 현재 전투 규칙은 바꾸지 않습니다.

## TextRPG에 추가될 최소 변경

나중에 실제 연결할 때만 다음을 추가합니다.

1. 아스키 화면 파일과 이미지/설정 파일을 프로젝트에 추가
2. 전투 시작 시 `BattleSceneState`를 한 번 구성
3. 전투 루프에서 화면 입력이 `RequestAttack`이면 기존 공격 함수를 호출
4. 공격 후 또는 턴이 바뀔 때 화면 상태를 갱신

`BattleManager`, `Player`, `Monster`의 공개 함수와 기존 전투 규칙을 수정할 필요는 없습니다.
