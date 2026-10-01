# 구현 프롬프트 — 버그 수정: 집게를 들면 대상의 빼기 강조가 표시됨

## 단계와 입력

- 버그: `.md/BugReports/2026-10-01_litter_tongs_false_take_highlight.md`. 서비스 3단위(커밋 `2c14d8f`)의 설계 결함이다.
- 기능 계약은 바뀌지 않는다. 지켜야 할 기존 결과:
  - 서비스 1·2단위 DISP-006·007·013(꺼내기 강조), TOWL 프리뷰·강조·뚜껑
  - 3단위 TRSH-010·021·022(집게 RMB 묶기, 조준 무관), TRSH-024
- 단계: 단순 버그 수정, 전체 구현 경로(수직 구현 아님). Content 변경은 없다.
- `.md/AGENT_WORKFLOW.md` → `.md/AGENT_IMPLEMENTATION.md`를 읽고 C++ 구현 단계만 수행한다.
- 설계 정본:
  - `.md/Architecture/CleaningLitterSystem.md` Input Routing
  - `HeldTargetUseSystem.md` HUD Data
  - `InteractionSystem.md` Blueprint/API Contracts
  - `UISystem.md` Interaction Prompt
- 이 문서와 정본이 다르면 구현을 멈추고 보고한다.
- 시작 조건: `git --no-optional-locks status --short -- Source Content Config`가 비어 있어야 한다. `.md/` 변경은 정상이다.
- headless 실행은 `.md/AGENT_WORKFLOW.md` UE 5.8 Headless Automation Policy 형식을 따른다.

## 원인

- `UPlayerEquipmentUseComponent::MergeEquipmentQuery`가 장비 보조 사용(집게 `봉투 묶기`) query를 `HeldTake*` 필드에 채운다.
- focus observer는 합성된 query를 받아 `bHeldTakeVisible && bCanHeldTake`로 꺼내기 강조·뚜껑을 켠다. 이 observer는 다음 셋이다.
  - `UDisplaySpaceComponent`
  - `TowelDisplayCueUtils`
  - `UTowelTransferPortComponent`
- 그래서 봉투가 1개 이상이면 대상과 무관하게 강조가 켜진다.

## 구현

1. `Public/Interaction/InteractionTypes.h`:
   - `EPlayerInteractionIntent` 끝에 `EquipmentSecondaryUse`를 append한다. 기존 순서는 바꾸지 않는다.
   - `FPlayerInteractionQuery`에 equipment 필드와 같은 형식(BlueprintReadOnly, Category `Interaction`)으로 추가하고 `Equals`에 포함한다.
     - `bool bEquipmentSecondaryVisible = false`
     - `bool bCanEquipmentSecondary = false`
     - `FText EquipmentSecondaryActionName`
     - `FText EquipmentSecondaryFailureReason`
2. `UPlayerEquipmentUseComponent`:
   - `MergeEquipmentQuery`: held-use Apply·Take 필드를 지우는 것은 유지한다. 보조 사용 query를 `HeldTake*` 대신 새 필드에 채운다. 장비가 아니거나 보조 사용이 없으면 새 필드는 기본값이다.
   - `ExecuteSecondaryEquipmentUse`: 결과 intent를 `HeldTake`에서 `EquipmentSecondaryUse`로 바꾼다. 다른 흐름은 그대로다.
3. `UInteractionPromptWidget`:
   - RMB 행 source를 고른다. `bEquipmentSecondaryVisible`이면 새 필드, 아니면 기존 held Take 필드다. 기존 `HeldTakeActionNameText`, `HeldTakeFailureReasonText`, `RmbKeyText`에 그대로 쓴다.
   - transient failure 처리 두 곳(현재 `case HeldTake`, 75행·425행 부근)에 `EquipmentSecondaryUse`를 같은 RMB slot으로 추가한다.
   - root 표시·enabled 판정(218·220·399행 부근)에 새 필드를 held Take와 같은 방식으로 포함한다.
   - BindWidget과 Blueprint event는 바꾸지 않는다.
4. focus observer 세 곳과 수건·진열 규칙은 **수정하지 않는다**. 장비를 들면 `HeldTake*`가 비어 있으므로 자동으로 꺼진다.
5. 금지:
   - `UPlayerInteractionComponent`, `UPlayerHeldTargetUseComponent`, Character 입력 순서, 집게 행동 변경
   - observer에 장비·집게 판별 추가
   - `HeldTake*` 의미 변경

## 자동화

- 기존 `CleaningLitterToolAutomationTests.cpp` 46행 부근 단언을 새 필드로 바꾼다. 봉투 묶기 행은 `bEquipmentSecondaryVisible`·`EquipmentSecondaryActionName == 봉투 묶기`이고, `bHeldTakeVisible`은 false다.
- 버그 회귀(신규, 실제 `RefreshInteractionQuery` 경로)에서 집게 봉투 3개를 들고 다음 대상을 조준한다.
  - 냉장고 진열 공간(재고 있음): `TakeHighlightProxy`·`InsertPreview`가 숨겨져 있다.
  - 수건 선반·사용 수건통(재고 있음): 강조 cue가 숨겨져 있다.
  - 대기 세탁기 port(수건 있음): 강조가 숨겨지고 뚜껑 목표가 닫힘이다.
  - 같은 조준에서 RMB 행 새 필드는 `봉투 묶기` 가능이다.
  - 실행하면 봉투가 묶이고 대상 재고는 불변이다(TRSH-021).
- 대조: 같은 대상을 품목 박스·수건바구니로 조준하면 기존대로 강조·뚜껑이 켜진다(DISP-006, TOWL).
- 위젯: 장비 보조 필드가 있으면 RMB 행에 그 문구가 보인다. `EquipmentSecondaryUse` 실패 result는 RMB 행에 1.5초 표시된다. widget 테스트가 기존에 없으면 query 단위 검증으로 대신하고 보고한다.
- 회귀(유지 필수): Cleaning 전체, Service 전체, Towel 전체(`TowelDisplayCue` 포함), Interaction held-use, Shop.
- 전체 회귀: `Automation RunTests BathhouseSim`. 수치와 실패 이름을 보고하고, 실패는 스스로 무관 판정하지 않는다.

## 빌드와 로드

- UE 5.8 `Build.bat`로 빌드한다.
- struct field와 enum 추가는 additive이고 asset에 저장되지 않는다. BP native parent·subobject 변경이 없으므로 copy-first gate 대신 DefaultMap 로드 1회로 확인한다. `WBP_InteractionPrompt` 로드를 포함한다.

## 결과물

- `.md/PROMPT_REVIEW.md`: 원인, 변경 파일, 시나리오·버그 회귀 대응, 빌드·로드·전체 회귀 수치, Content·Config 무변경 확인.
- `.md/PROMPT_UNREAL.md`: Editor 작업 없음을 명시한다. PIE 관찰 항목:
  - 집게(봉투 1개 이상)로 냉장고·수건 선반·사용 수건통·세탁기·건조기를 조준하면 빼기 강조와 뚜껑 열림이 없고, RMB 행은 `봉투 묶기`다.
  - 품목 박스·수건바구니로 조준하면 기존대로 강조와 뚜껑이 보인다.
- `.md/Architecture/*`는 수정하지 않는다. 설계와 달라야 하면 멈추고 보고한다.
