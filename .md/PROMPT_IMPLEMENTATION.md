# 구현 프롬프트 — 설비 회수 아이템 scale 읽기 수정 (Placement)

## 단계와 입력

- 2026-09-28 상점 확장이 사용자 통합 승인됐다. 그 코드 리뷰가 인계한 상점 범위 밖 후속 과제 A1(회수 scale), A2(LOCTEXT 키 중복)를 하나의 작은 C++ 과제로 처리한다.
- 사용자 동작 변화 없음. 회수 아이템의 보이는 크기는 지금과 같고, 회수 공간 판정만 정확해진다. 기능 명세 단계 입력은 없다.
- `.md/AGENT_WORKFLOW.md` → `.md/AGENT_IMPLEMENTATION.md`를 읽고 C++ 구현 단계만 수행한다.
- 설계 정본: `.md/Architecture/PlacementSystem.md`
  - "활성 Definition은 하나의 공통 Blueprint 파생 클래스…" 문단(회수 scale 정본)
  - Verification의 회수 scale 항목
- 이 문서와 정본이 다르면 구현을 멈추고 보고한다.
- headless 실행은 `.md/AGENT_WORKFLOW.md` UE 5.8 Headless Automation Policy 형식을 따른다. git은 `--no-optional-locks`로 실행한다.
- 시작 전 작업 트리의 상점 확장 변경이 커밋됐는지 확인하고, 안 됐으면 사용자에게 알리고 멈춘다.

## 현재 → 목표

| 항목 | 현재 | 목표 |
|---|---|---|
| 회수 query scale | `FacilityActorConversionTransaction.cpp` `ValidateRecoveryCandidate` 경로가 `ItemCDO->GetActorScale3D()`를 읽음. Blueprint CDO는 component-to-world가 갱신되지 않아 항상 1 | `APlaceableFacilityItemActor::GetDefinitionItemScale`(CDO `ItemRoot` relative scale) |
| 회수 spawn | `SpawnActorDeferred`·`FinishSpawning` 기본 MultiplyWithRoot. transform scale 1 × root scale로 우연히 맞음 | 두 호출 모두 `ESpawnActorScaleMethod::OverrideRootScale` |
| LOCTEXT | `PlaceableFacilityItemCollision.cpp`에서 `InvalidRecoveryItemScale` 키가 두 문구에 쓰임 | `GetDefinitionItemScale`의 키를 `InvalidRecoveryItemRootScale`로 변경 |

## 1. 회수 scale

- `FacilityActorConversionTransaction.cpp`:
  - `OutItemTransform.SetScale3D(ItemCDO->GetActorScale3D())`를 `GetDefinitionItemScale(*Definition, Scale, OutFailureReason)` 결과로 바꾼다. 실패하면 회수 후보 실패로 반환한다.
  - `RecoverFacilityToItem`의 `SpawnActorDeferred`와 `FinishSpawning`에 `ESpawnActorScaleMethod::OverrideRootScale`을 준다. **scale 읽기만 바꾸고 이 변경을 빠뜨리면 root scale이 두 번 곱해져(예: 0.6 → 0.36) 회수 아이템이 작아진다.** 두 변경은 반드시 함께 한다.
  - 같은 파일의 배치 경로(현재 313·372행 부근)는 이미 규칙대로이므로 바꾸지 않는다.
- 회수 drop 위치 계산(`GetRecoveryDropTransform`)이 item extent를 쓰는지 확인한다. 쓴다면 같은 scale을 쓰게 하고, 쓰지 않는다면 바꾸지 않는다. 확인 결과를 보고한다.
- 그 밖에 CDO `GetActorScale3D()`로 authored scale을 읽는 곳이 없는지 `Source` 전체를 검색해 보고한다(테스트 제외). 발견되면 고치지 말고 위치만 보고한다.

## 2. LOCTEXT 키

- `PlaceableFacilityItemCollision.cpp`의 `GetDefinitionItemScale` 안 `"InvalidRecoveryItemScale"`을 `"InvalidRecoveryItemRootScale"`로 바꾼다. 문구는 유지한다. 기존 `BuildDefinitionCollisionQuery`의 키는 그대로 둔다.

## 3. 자동화

새 테스트는 `Private/Tests/FacilityRecoveryScaleAutomationTests.cpp`에 둔다(`FacilityPlacementAutomationTests.cpp`는 이미 1856줄). fixture class는 `FacilityPlacementAutomationTestProbe.h`에 추가할 수 있다.

- **scale 수치를 하드코딩해 단언하지 않는다.** 기대값은 항상 fixture나 asset의 authored relative scale에서 읽는다. `BP_PlaceableFacilityItem`의 `ItemRoot` scale은 사용자 조정값이며 asset이 정본이다.
- Blueprint식 fixture: 생성자에서 `ItemRoot` relative scale을 component-to-world 갱신 없이 설정하는 `APlaceableFacilityItemActor` 파생 class(예: `SetRelativeScale3D_Direct`).
  - 사전 기록: CDO `GetActorScale3D()`와 `ItemRoot->GetRelativeScale3D()`를 로그에 남긴다. 두 값이 다르면 Blueprint 상황을 재현한 것이다. 같게 나오면 그 사실을 보고한다(테스트 의미가 약해짐).
  - 회수 공간: 원래 설비 옆에 blocker를 두되, scale 1 크기의 회수 상자와는 겹치고 authored scale 크기와는 겹치지 않는 거리에 둔다. 회수가 성공해야 한다(현재 코드는 실패).
  - 회수 결과: 생성 아이템 world scale = authored relative scale(제곱 아님). physics·CCD·Pawn Ignore 유지.
  - E pickup → G drop 뒤에도 world scale이 같다.
- 실제 asset: `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem` CDO에서 `GetDefinitionItemScale` = `ItemRoot->GetRelativeScale3D()`. 이 class를 `RecoveryItemClass`로 쓰는 테스트 Definition으로 회수하면 world scale이 그 값과 같다.
- 기존 "Recovery uses the derived facility item CDO scale" 단언은 수치 비교 대신 fixture CDO의 relative scale과 비교하도록 바꾼다. 의미는 유지한다.
- 회귀: Placement 전체, Shop 전체(개봉·신규 설치·쓰레기통), Interaction carry.
- 전체 회귀: `Automation RunTests BathhouseSim`. 수치와 실패 이름을 보고하고, 실패는 스스로 무관 판정하지 않는다.

## 4. 빌드와 로드

- `.md/AGENT_WORKFLOW.md`의 UE 5.8 `Build.bat`만 사용한다.
- reflected property·class·component 변경이 없으므로 copy-first load gate는 필요 없다. 새 fixture class는 테스트 전용 native class다.

## 금지

- 기존 class·property·subobject rename·삭제·class 변경, Core Redirect, native `Serialize` 변경.
- `FacilityItemHeldTransform`, 설비 배치 transform, 상점 개봉 코드 변경.
- Content·Config·Level 저장, Editor authoring.
- `.md/Architecture/*`, `.md/Unreal/*` 수정.

## 단계 결과물

- `.md/PROMPT_REVIEW.md`: 변경 파일, 1의 확인 결과(drop 위치, 추가 검색), fixture 사전 기록 값, automation 수치.
- Editor 작업이 없으므로 `.md/PROMPT_UNREAL.md`는 만들지 않는다. PIE 확인이 필요하면 `PROMPT_REVIEW.md`에 "좁은 곳 회수 가능, 회수 아이템 크기 불변"을 사용자 확인 항목으로 적는다.
