# Held Target Use System

## Status And Scope

- 2026-09-28 설계, Source 미반영. 입력은 `.md/PROMPT_ARCHITECTURE.md`(들고 있는 물건 조작 LMB 통일, CTRL-001~029)와 `.md/QNA_FEATURE_SPEC.md` Q1~Q7이다.
- 들고 있는 물건으로 조준 대상에 하는 일을 LMB(물건 → 대상)와 RMB(대상 → 물건)로 옮기는 Interaction 하위 계약이다. 입력 소유, 연속 실행, HUD 표시 데이터를 정한다.
- 첫 사용처는 삽(공급함·투입구)과 수건바구니(선반·사용 수건통·세탁기/건조기 port·바닥 수건)다. 앞으로 "박스 → 진열대" 같은 작업이 같은 계약을 쓴다.
- 이동 방향·조건·transaction은 각 domain 정본([TowelSystem.md](TowelSystem.md), [UtilityFuelSystem.md](UtilityFuelSystem.md))을 그대로 따른다. 키와 실행 경로만 바뀐다.
- 비대상: F의 새 용도, 장비(`IHeldEquipmentUsable`)의 RMB, 키 재설정 UI.

## Source Scope

```text
Public/Interaction/PlayerHeldTargetUseComponent.h      신규
Private/Interaction/PlayerHeldTargetUseComponent.cpp   신규
Public/Interaction/InteractionTypes.h                  enum append, query field 추가
Public/Interaction/PlayerInteractable.h                ExecuteHeldTargetUse 추가
Private/Towel/TowelHeldTransferRules.h/.cpp            신규, 수건 방향 규칙 순수 helper
```

수정: `PlayerInteractionComponent`(focus 조회 accessor 하나), `PlayerEquipmentUseComponent`(merge 규칙), `FirstPersonCharacter`(RMB·owner), `InteractionPromptWidget`, 수건 대상 네 개, 연료 대상 두 개.

## Terms And Ownership

- Apply(LMB): 들고 있는 물건 → 대상. Take(RMB): 대상 → 들고 있는 물건. Q 설비 회수(Recovery)와 섞지 않도록 Take라고 부른다.

| 책임 | owner |
|---|---|
| 방향별 가능 여부·행동명·이유 | 대상 `QueryInteraction`의 held-use 필드 |
| 한 단위 실행(삽 한 동작, 수건 1장) | 대상 `ExecuteHeldTargetUse` → 기존 domain transaction |
| press 소유, 연속 실행, 중단 | `UPlayerHeldTargetUseComponent` |
| 버튼별 owner 선택 | `AFirstPersonCharacter` |
| 표시 | `UInteractionPromptWidget` |

대상은 들고 있는 물건 종류를 스스로 판별한다(기존 target-side 패턴). Character·Interaction·Widget은 삽·바구니·수건 같은 concrete type을 판별하지 않는다.

## Contract Types

- `EPlayerHeldTargetUseDirection`(UENUM BlueprintType, 신규): `Apply`, `Take`.
- `EPlayerInteractionIntent`: 끝에 `HeldApply`, `HeldTake`를 append한다.
- `EPlayerInteractionActivationMode`: 끝에 `Repeat`를 append한다. held-use는 `Instant`(press당 한 번) 또는 `Repeat`(press 즉시 한 번 + 간격 반복)만 쓴다. `Hold`는 held-use에 쓰지 않는다.
- `FPlayerInteractionQuery`에 추가(BlueprintReadOnly, `Equals`에 포함):
  - `bHeldApplyVisible`, `bCanHeldApply`, `HeldApplyActionName`, `HeldApplyFailureReason`, `HeldApplyActivationMode`(기본 Instant)
  - `bHeldTakeVisible`, `bCanHeldTake`, `HeldTakeActionName`, `HeldTakeFailureReason`, `HeldTakeActivationMode`(기본 Instant)
- 방향 필드의 의미:
  - Visible true: HUD 행을 표시한다. 불가면 이유를 함께 표시한다.
  - Visible false + 이유 있음: 행은 숨기고, 그 키를 누르면 이유를 한 번 보인다. 반대 방향이 없는 대상에 쓴다(CTRL-015, 023의 수건 대상 쪽).
  - 둘 다 비어 있음: 이 대상과 무관한 방향이다. 누르면 아무 일도 없다.
- `IPlayerInteractable::ExecuteHeldTargetUse(Context, Direction)`(C++ virtual, 기본 빈 이유 실패): 조건을 재검증하고 한 단위만 실행한다. 결과 intent는 `HeldApply`/`HeldTake`다.
- `UPlayerEquipmentUseComponent`:
  - `HasUsableHeldEquipment() const`(public, 신규): 들고 있는 Actor가 `IHeldEquipmentUsable`인지.
  - `MergeEquipmentQuery`: 들고 있는 Actor가 장비면 held-use 두 방향 필드를 모두 비운다. 장비가 LMB를 authoritative하게 소유하고 RMB는 아무 일도 없다(CTRL-026).

## Input Ownership

LMB Started(`PrimaryUseAction`), 위에서부터 처음 맞는 owner 하나:

1. Computer가 input capture 중 → Computer(기존)
2. Placement active → Placement confirm(기존)
3. held-use component가 다른 버튼으로 진행 중 → 이 press 무시
4. 들고 있는 Actor가 장비 → Equipment(기존)
5. 손에 장비 아닌 물건이 있음, 또는 focus query의 Apply 방향에 행이나 이유가 있음 → HeldTargetUse(Apply)
6. 그 밖(빈손이고 Apply 정보 없음) → Equipment 기존 fallback. 예: 빈손으로 물 얼룩을 조준하면 "물걸레가 필요합니다"

RMB Started(`SecondaryUseAction`, 신규):

1. Computer capture, Placement active, held-use 진행 중, 장비를 듦 → 무시(press만 소비)
2. 그 밖 → HeldTargetUse(Take)

- Completed/Canceled는 시작 owner에만 전달한다. press 도중 owner를 바꾸지 않는다.
- E/F/G/Q 입력 경로는 바꾸지 않는다. 삽·수건 이동이 E/F에서 사라지는 것은 대상 query·execute 변경으로 처리한다.

## `UPlayerHeldTargetUseComponent`

Character default subobject `PlayerHeldTargetUse`. `Configure(Interaction, Carry, EquipmentUse)`로 주입받는다.

- `RepeatIntervalSeconds`: EditDefaultsOnly, 기본 0.15, ClampMin 0.05. BP_FirstPersonCharacter의 component 기본값에서 조정한다(CTRL-029).
- `BeginUse(Direction)`:
  1. suppression·로컬 제어·주입 참조를 확인한다. 실패하면 조용히 끝낸다.
  2. `UPlayerInteractionComponent::ResolveFocusedInteraction`으로 fresh trace, target object, context를 얻는다.
  3. target의 `QueryInteraction`에서 그 방향 필드를 읽는다.
     - 행·이유가 모두 없으면 아무것도 하지 않는다. result를 보고하지 않고, 버튼을 뗄 때까지 press만 소유한다.
     - 불가면 그 이유로 실패 result를 한 번 보고하고 반복하지 않는다.
  4. `ExecuteHeldTargetUse`를 한 번 호출한다. 결과를 `ReportExternalInteractionAttempt`로 보고하고 `RefreshInteractionQuery`를 호출한다.
  5. 성공이고 mode가 `Repeat`면 반복 상태로 들어간다. 저장하는 값: weak target object, direction, 시작 시 held object identity, 누적 시간 0. 이때만 Tick을 켠다.
- Tick(반복 중에만):
  - 조용히 멈춤: held object 변경(내려놓기·거치 포함), suppression(컴퓨터 진입), 로컬 제어 상실, focus target이 저장한 target과 다르거나 없음(조준 이탈).
  - 이유를 보고하고 멈춤: target query에서 그 방향이 불가(가득 참·빔·상태 변화·작동 시작), 또는 execute 실패.
  - 누적 시간이 간격 이상이면 query 재확인 → execute 한 번 → 누적 시간에서 간격을 뺀다. 한 Tick에 최대 한 번 실행한다.
  - 멈춘 뒤에는 버튼을 뗄 때까지 다시 시작하지 않는다(CTRL-019). 다시 누르면 새 `BeginUse`다.
- `EndUse()`: 버튼을 뗌. `CancelUse()`: Character·EndPlay 정리. 둘 다 반복 상태를 비우고 Tick을 끈다. 이미 옮긴 것은 되돌리지 않는다(CTRL-020).
- 각 실행은 대상의 기존 atomic 한 단위 transaction이다. 실패한 실행은 아무것도 바꾸지 않는다.

`UPlayerInteractionComponent`에는 C++ 전용 `ResolveFocusedInteraction(OutContext, OutInteractable, OutTargetObject) const` 하나만 추가한다. 기존 private `BuildInteraction`에 위임하며 상태·Tick·concrete 판별을 추가하지 않는다([CoreSystem.md](CoreSystem.md) Class Growth Policy 예외).

## Shovel Targets

- 대상: `AUtilityFuelSupplyActor`, `UUtilityFuelIntakeVolumeComponent`.
- 기존 E primary의 행동명·평가(`EvaluateScoop`/`EvaluateReturn`/`EvaluateInsert`)를 Apply 필드로 옮긴다. mode는 `Instant`다(CTRL-005). Take 필드는 비운다(RMB 무관, CTRL-006).
- E primary: `bVisible` true(대상 이름 표시), `ActionName` 비움, `bCanInteract` false, 이유 비움. `ExecuteInteraction`은 빈 이유 실패를 반환해 E가 아무 일도 하지 않는다.
- `ExecuteHeldTargetUse(Apply)`는 기존 Scoop/Return/Insert transaction을 호출한다. fresh single-hit 재검증과 `UtilityLaborInputGuard`를 유지한다.
- 삽을 들지 않았으면(빈손·다른 물건) Apply 행을 불가로 표시하고 기존 "삽을 들고 있어야 합니다"를 쓴다.
- 문 자동 열림 observer는 `bHeldApplyVisible && bCanHeldApply`로 판정한다(CTRL-003).
- 빈 공간에서 삽 LMB는 5단계 owner가 되지만 target이 없어 아무 일도 없다(CTRL-007).

## Towel Targets

- 대상: `ACleanTowelStackActor`, `AUsedTowelBinActor`, `UTowelTransferPortComponent`(세탁기·건조기), `AWorldUsedTowelActor`.
- `FTowelHeldTransferRules`(Private/Towel, 순수 helper): 대상 종류, 대상 snapshot·machine state, held basket snapshot(없으면 없음)을 받아 두 방향의 {Visible, bCan, ActionName, FailureReason}를 만든다. 대상은 query와 execute 재검증에서 같은 helper를 쓴다.

| 대상 | Apply(LMB) | Take(RMB) | mode |
|---|---|---|---|
| 선반 | 표시 `넣기` | 숨김, 이유 `여기서는 꺼낼 수 없음` | Repeat |
| 사용 수건통 | 숨김, 이유 `여기에는 넣을 수 없음` | 표시 `빼기` | Repeat |
| 기계 Waiting | 표시 `넣기` | 표시·불가 `완료 후 뺄 수 있음` | Repeat |
| 기계 Complete | 표시·불가 `비운 뒤 넣을 수 있음` | 표시 `빼기` | Repeat |
| 기계 Processing | 표시·불가 `작동 중` | 표시·불가 `작동 중` | Repeat |
| 바닥 사용한 수건 | 숨김, 이유 `여기에는 넣을 수 없음` | 표시 `줍기` | Instant |

- 가능 방향의 불가 이유는 다음 순서로 첫 번째를 쓴다.
  1. 바구니 없음 `수건 바구니 필요`
  2. 작동 중 `작동 중`
  3. 상태 불일치 `다른 상태의 수건`
  4. Apply: 바구니 빔 `바구니 비어 있음`, 대상 가득 참 `{대상} 가득 참`
  5. Take: 대상 빔 `{대상} 비어 있음`, 바구니 가득 참 `바구니 가득 참`
- `{대상}`은 기존 TargetName이다. 행동명은 넣기·빼기·줍기이고 TargetName은 기존 문구를 유지한다.
- E/F: 네 대상 모두 E primary 이동과 F secondary를 없앤다(`ActionName` 비움, `bSecondaryVisible` false). 대상의 `ExecuteSecondaryInteraction` override를 제거해 기본 실패로 둔다. F 입력 binding과 secondary 계약은 예약으로 남긴다(CTRL-021, Q5 A).
- 선반·수건통의 placed-domain 비활성 fallback(base facility query/execute)은 유지한다.
- `ExecuteHeldTargetUse`: helper로 재평가한 뒤 기존 `UTowelTransferSubsystem::TryTransfer`를 RequestedCount 1로 호출한다. `MAX_int32` 요청 경로는 삭제한다.
- 연속 옮기기는 대상이 그 방향 불가가 되는 순간 component가 멈춘다(CTRL-010, 011, 014). 완료 세탁기가 비어 Waiting으로 돌아가면 Take가 불가가 되어 멈춘다(CTRL-012).
- 바닥 수건은 줍는 순간 소모돼 target이 사라지므로 Instant다(CTRL-018).

## HUD Data

[UISystem.md](UISystem.md) Interaction Prompt의 추가 규칙:

- LMB 행: `bEquipmentUseVisible`이면 equipment 필드, 아니면 held Apply 필드를 쓴다. 두 source를 두 행으로 동시에 표시하지 않는다.
- RMB 행: held Take 필드를 쓴다. 새 `BindWidgetOptional` `HeldTakeActionNameText`, `HeldTakeFailureReasonText`(UTextBlock)다.
- 키 라벨: `BindWidgetOptional` `PrimaryKeyText`, `LmbKeyText`, `RmbKeyText`(UTextBlock)와 EditDefaultsOnly 표시값 `PrimaryKeyLabel`(E), `LmbKeyLabel`(LMB), `RmbKeyLabel`(RMB). native가 해당 행과 같이 보이기·접기를 적용한다.
- E 행은 `ActionName`이 비면 키·행동·이유를 모두 접는다.
- F 행은 기존대로 `bSecondaryVisible`을 따른다. 이번 대상에서는 모두 false다(CTRL-025).
- 대상 이름 표시와 root visible/enabled 조건에 held 두 방향 행을 포함한다.
- transient failure: intent `HeldApply`는 LMB 행, `HeldTake`는 RMB 행에 표시한다. 기존 1.5초 규칙을 따른다.
- 새 Blueprint event는 추가하지 않는다. 기존 hook signature를 바꾸지 않는다.

## Compatibility

- 추가만 한다: enum append, struct field, interface virtual, 새 component, Character `SecondaryUseAction`, widget optional BindWidget과 표시값. rename·삭제·class 변경이 없어 Core Redirect가 필요 없다.
- 수건 대상의 `ExecuteSecondaryInteraction` override 제거는 reflected 변경이 아니다.
- Character 새 default subobject와 widget native 변경이 있으므로 `BP_FirstPersonCharacter`, `WBP_InteractionPrompt`는 copy-first load gate로 먼저 확인한다([CoreSystem.md](CoreSystem.md)).
- Editor:
  - `IA_SecondaryUse`(Boolean) 생성
  - `IMC_FirstPerson`에 RightMouseButton → `IA_SecondaryUse`
  - `BP_FirstPersonCharacter.SecondaryUseAction` 지정
  - `WBP_InteractionPrompt` RMB 행과 키 라벨 widget 추가. 기존에 고정 키 문자열이 있으면 중복되지 않게 정리한다.

## Verification

| 시나리오 | 자동화 |
|---|---|
| CTRL-001~005, 007 | 삽 Apply: 퍼담기·반환·투입·재료 불일치, 누른 채 유지해도 한 번, 빈 공간 LMB 무변화 |
| CTRL-003 | 문 열림이 Apply 가능과 같음, 투입 뒤 닫힘 |
| CTRL-006, 021 | 삽·바구니를 들고 공급함·투입구·수건 대상에 E·F·RMB(삽) → 변화 없음 |
| CTRL-008, 022, 028 | 거치대·키 걸이·쓰레기통 E 기존대로, 삽 재료·바구니 수량 보존 |
| CTRL-009~014, 018 | 수건 Apply/Take 한 장, Repeat 간격, 가득 참·빔·상태 변화에서 멈춤, 세탁기 완료 → Waiting 복귀, 바닥 수건 한 장 |
| CTRL-015~017, 023, 024 | 숨긴 방향의 이유, 완료 전 빼기·다른 상태·작동 중 이유, 수건 대상이 아닌 곳 무변화 |
| CTRL-019, 020 | 조준 이탈 멈춤·재조준 자동 재개 없음·떼고 다시 누르면 시작, drop·거치·suppression 멈춤, 이미 옮긴 수량 유지 |
| CTRL-025 | query 필드: LMB·RMB 행, 이유, F 행 없음, E 행 접힘 |
| CTRL-026, 027 | 렌치·걸레·상자 LMB 기존, RMB 무변화, 컴퓨터·배치 중 held-use 없음 |
| CTRL-029 | 간격 0.3에서 실행 시각 간격 |
| 회귀 | 기존 Towel·Utility·Cleaning·Combat·Computer·Placement·Shop·Interaction 전체 |

반복 타이밍은 component Tick에 DeltaTime을 직접 넣어 결정적으로 검증한다. PIE에서는 실제 LMB·RMB 입력, HUD 키 라벨, 0.15초 감각(CTRL-010, 025)을 확인한다.

## Dependencies

- Character → Interaction held-use component
- Towel·Utility → Interaction held-use target 계약
- UI → Interaction query 필드
- Interaction은 Towel·Utility concrete type에 의존하지 않는다.
