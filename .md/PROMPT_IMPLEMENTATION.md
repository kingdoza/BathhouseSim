# 구현 프롬프트 — 들고 있는 물건 조작 LMB·RMB 통일(held-use)

## 단계와 입력

- 2026-09-28 사용자가 기능 계약 `.md/PROMPT_ARCHITECTURE.md`(CTRL-001~029, Q1~Q7)를 승인하고 설계 진행을 지시했다. 계약서 머리의 "승인 대기" 문구는 기능 명세 단계 소유라 이 단계에서 고치지 않는다.
- 현재 단계: **전체 구현**(Q7 A, 별도 수직 없음). 삽과 수건바구니를 한 번에 바꾼다.
- `.md/AGENT_WORKFLOW.md` → `.md/AGENT_IMPLEMENTATION.md`를 읽고 C++ 구현 단계만 수행한다.
- 설계 정본(전체 읽기): `.md/Architecture/HeldTargetUseSystem.md`.
- 관련 절:
  - `InteractionSystem.md`: 머리말, Held Equipment Use Contract, `UPlayerInteractionComponent`, Character Integration
  - `CharacterSystem.md`: Primary Use Ownership
  - `TowelSystem.md`: Inventory And Atomic Transfer, Clean Stack And Used Bin, Used Towel Overflow, Transfer Direction
  - `UtilityFuelSystem.md`: Fuel Kinds And Supply, Shovel, Input And Trace, Fuel Door Presentation
  - `UISystem.md`: Interaction Prompt
  - `CoreSystem.md`: Class Growth Policy
- 이 문서와 정본이 다르면 구현을 멈추고 보고한다.
- headless 실행은 `.md/AGENT_WORKFLOW.md` UE 5.8 Headless Automation Policy 형식을 따른다. git은 `--no-optional-locks`로 실행한다.
- 시작 전 `Source/`, `Content/`, `Config/`에 `72f85ec` 이후 미커밋 변경이 없는지 확인한다(`git --no-optional-locks status --short -- Source Content Config`). 있으면 멈추고 보고한다. `.md/` 변경(이번 설계 산출물: Architecture 문서 8개, `PROMPT_IMPLEMENTATION.md`)은 정상이며 시작 조건에 포함하지 않는다.

## 현재 → 목표

| 영역 | 현재 Source | 목표 |
|---|---|---|
| 삽 작업 | 공급함·투입 Volume의 E primary | 같은 평가·transaction을 LMB held-use Apply(`Instant`)로 |
| 수건 이동 | 대상의 E 1장, F 최대 | LMB Apply·RMB Take 1장, `Repeat`는 누르는 동안 반복 |
| RMB | 입력 없음(IMC에 RightMouseButton 없음 확인) | `SecondaryUseAction` → held-use Take |
| F | 수건 세 대상만 사용 | 제공 대상 없음. binding·계약은 예약 유지 |
| 연속 실행 | 없음 | 신규 `UPlayerHeldTargetUseComponent` |
| HUD | 키 라벨 없음, E/F/LMB 행 | LMB 행 = equipment 또는 Apply, RMB 행 신규, 키 라벨, 빈 E 행 접기 |
| 수건 실패 이유 | "현재 수건을 옮길 수 없습니다" 등 뭉뚱그림 | 정본 표의 방향별 이유 |
| 투입구 문 | E query `bVisible && bCanInteract` | `bHeldApplyVisible && bCanHeldApply` |

## 0. 사전 조건과 백업

- UnrealEditor가 모두 종료됐는지 확인한다.
- `Saved/MigrationBackup/20260928_heldtargetuse/`에 파일 복사한다:
  - `Content/FirstPersonCharacter/BP_FirstPersonCharacter.uasset`
  - `Content/Bathhouse/UI/WBP_InteractionPrompt.uasset`
- Content·Config·Level을 저장하지 않는다(아래 7의 복사본 확인만 예외).

## 1. Interaction 타입과 계약

- `InteractionTypes.h`:
  - `EPlayerHeldTargetUseDirection`(UENUM BlueprintType) `Apply`, `Take` 신규.
  - `EPlayerInteractionIntent` 끝에 `HeldApply`, `HeldTake`를 append한다.
  - `EPlayerInteractionActivationMode` 끝에 `Repeat`를 append한다. 기존 값 순서를 바꾸지 않는다.
  - `FPlayerInteractionQuery`에 정본 Contract Types의 10개 필드를 추가하고 `Equals`에 포함한다.
- `PlayerInteractable.h`: `virtual FPlayerInteractionResult ExecuteHeldTargetUse(const FPlayerInteractionContext&, EPlayerHeldTargetUseDirection)`. 기본은 빈 이유 실패이며 intent는 방향에 맞춘다.
- `UPlayerInteractionComponent`: C++ 전용 `bool ResolveFocusedInteraction(FPlayerInteractionContext&, IPlayerInteractable*&, UObject*&) const`. 기존 `BuildInteraction` 위임만 한다. 다른 추가 금지(CoreSystem 성장 정책).
- `UPlayerEquipmentUseComponent`:
  - `bool HasUsableHeldEquipment() const` 추가.
  - `MergeEquipmentQuery`에서 장비를 들었으면 held-use 10개 필드를 기본값으로 비운다.

## 2. `UPlayerHeldTargetUseComponent`

- 신규 `Public/Interaction/PlayerHeldTargetUseComponent.h`, `Private/Interaction/PlayerHeldTargetUseComponent.cpp`.
- 정본 `UPlayerHeldTargetUseComponent` 절을 그대로 구현한다: `Configure`, `BeginUse(Direction)`, `EndUse`, `CancelUse`, `IsUseActive`, `RepeatIntervalSeconds`(EditDefaultsOnly, 기본 0.15, ClampMin 0.05).
- 요점:
  - Tick은 반복 중에만 켠다.
  - 한 Tick 최대 1회 실행한다.
  - 조용한 중단과 이유 보고 중단을 정본대로 구분한다.
  - 멈춘 뒤에는 버튼을 뗄 때까지 재개하지 않는다.
- 결과 보고는 `ReportExternalInteractionAttempt`만 쓴다. 실행 뒤 `RefreshInteractionQuery`를 호출한다.
- 자동화용: 반복 진행을 DeltaTime으로 직접 밀 수 있는 friend test 또는 `TickRepeatForTest(float)`를 둔다. production 경로와 같은 함수를 호출해야 한다.
- EndPlay에서 `CancelUse`를 호출한다.

## 3. Character

- `SecondaryUseAction`(EditDefaultsOnly, `UInputAction`)을 추가한다. Started/Completed/Canceled를 bind한다.
- default subobject `PlayerHeldTargetUse`와 getter `GetPlayerHeldTargetUse`를 추가한다. `BeginPlay` 조립 지점에서 `Configure`한다.
- 사설 enum `EPrimaryUsePressOwner`에 `HeldTargetUse`, `Ignored`를 추가한다(reflected 아님).
- LMB owner 선택은 정본 Input Ownership 1~6단계를 따른다. RMB도 별도 press owner(`None/HeldTargetUse/Ignored`)로 같은 규칙을 적용한다.
- 5단계 조건 "focus query의 Apply 방향에 행이나 이유가 있음"은 `PlayerInteraction->GetCurrentInteractionQuery()`로 판정한다. 이 query는 이미 equipment merge를 거친 값이다.
- Completed/Canceled는 시작 owner에만 전달한다. Character에 삽·바구니 판별을 넣지 않는다.

## 4. 대상 이동

### 삽

- `AUtilityFuelSupplyActor`, `UUtilityFuelIntakeVolumeComponent`에서 E primary의 행동명·평가를 Apply 필드로 옮긴다.
  - E 필드: `bVisible` true, `ActionName` 비움, `bCanInteract` false, 이유 비움.
  - `ExecuteInteraction`: 빈 이유 실패.
  - `ExecuteHeldTargetUse(Apply)`: 기존 transaction. `Take`는 빈 이유 실패.
- 투입구 focus observer의 문 열림 판정을 Apply 필드로 바꾼다.
- `FUtilityFuelTransaction`, 재료 규칙, 실패 문구, fresh hit, `UtilityLaborInputGuard`는 바꾸지 않는다.

### 수건

- 신규 `Private/Towel/TowelHeldTransferRules.h/.cpp`: 정본 Towel Targets 표와 이유 우선순위를 구현한다. 순수 함수이며 inventory를 바꾸지 않는다.
- `ACleanTowelStackActor`, `AUsedTowelBinActor`, `UTowelTransferPortComponent`, `AWorldUsedTowelActor`:
  - query에서 E 이동·F를 없애고 Apply·Take 필드를 helper로 채운다. 선반·수건통의 placed-domain 비활성 fallback은 유지한다.
  - `ExecuteSecondaryInteraction` override를 제거한다.
  - E `ExecuteInteraction`은 이동하지 않는다(빈 이유 실패). 단, 비활성 fallback 경로는 기존대로 base에 위임한다.
  - `ExecuteHeldTargetUse`: helper 재평가 → `TryTransfer` RequestedCount 1. `MAX_int32` 경로를 삭제한다.
  - `AWorldUsedTowelActor`는 Take 성공 시 기존 consumed·제거 처리를 유지한다.
- `UTowelTransferSubsystem`, inventory, machine state 전환, customer 경로는 바꾸지 않는다.

## 5. HUD

- `UInteractionPromptWidget`에 정본 HUD Data 절을 구현한다.
  - `BindWidgetOptional`: `HeldTakeActionNameText`, `HeldTakeFailureReasonText`, `PrimaryKeyText`, `LmbKeyText`, `RmbKeyText`.
  - EditDefaultsOnly `FText` 표시값: `PrimaryKeyLabel`("E"), `LmbKeyLabel`("LMB"), `RmbKeyLabel`("RMB").
- 기존 필수 BindWidget 검사(`ensureMsgf`)에 새 optional 항목을 넣지 않는다. 없으면 그 표시만 생략한다.
- 규칙:
  - LMB 행은 equipment가 visible이면 equipment, 아니면 Apply 필드를 쓴다.
  - E 행은 `ActionName`이 비면 접는다.
  - 대상 이름 표시와 root visible/enabled 조건에 held 행을 포함한다.
  - `HeldApply` transient failure는 LMB 행, `HeldTake` transient failure는 RMB 행에 표시한다.
- 기존 reflected event signature를 바꾸지 않고 새 event를 추가하지 않는다.

## 6. 빌드

- `.md/AGENT_WORKFLOW.md`의 UE 5.8 `Build.bat`만 사용한다.

## 7. 로드 검증 — 복사본 먼저

- `BathhouseSim.Interaction.HeldTargetUse.BlueprintLoad`를 추가한다. 기존 Shop·Computer BlueprintLoad 형식을 따른다.
  - 대상: `BP_FirstPersonCharacter`, `WBP_InteractionPrompt`. `-BathhouseHeldUseLoadPath=`로 단일 package를 지정할 수 있다.
  - 확인:
    - native parent
    - Character의 `PlayerHeldTargetUse` subobject와 `RepeatIntervalSeconds` 기본 0.15
    - 기존 `PrimaryUseAction`, `InteractAction`, `SecondaryInteractAction` 값 유지
    - WBP 기존 필수 BindWidget 유지
  - 저장하지 않는다.
- 순서(Fatal·`Serial size mismatch`·`Failed to load`면 즉시 멈춤):
  1. 복사본을 `Content/Developers/MigrationCheck/`에 두고 Template 맵으로 로드한다.
  2. 복사본을 삭제하고 Content 무변경을 확인한다.
  3. 원본을 로드한다.
  4. DefaultMap으로 로드한다.
  5. 다시 무변경을 확인한다.

## 8. 자동화

정본 Verification 표 전체를 구현한다. 새 테스트는 `Private/Tests/HeldTargetUseAutomationTests.cpp`에 둔다. 기존 파일(`CleaningTowelAutomationTests.cpp` 1071줄 등)은 크므로 누적하지 않는다. 추가 조건:

- 수건 규칙 helper 표 테스트: 대상 5종 × 바구니 상태(없음·빔·Used·Wet·Clean·가득) × 방향. Visible·bCan·이유가 정본 표·우선순위와 같다.
- component 반복: DeltaTime을 직접 주입한다.
  - 0.15 간격 실행 횟수, 한 Tick 최대 1회.
  - 가득 참·빔 중단과 이유 보고 1회.
  - 조준 이탈 조용한 중단과 재조준 무재개, release 뒤 재시작.
  - drop·거치·suppression 중단.
  - 간격 0.3.
- owner 선택: Computer·Placement·장비·장비 아닌 물건·빈손(물 얼룩 hint fallback)·held-use 진행 중 다른 버튼. RMB는 같은 규칙.
- 삽: 기존 E 기반 fuel automation(`FuelInteractionAtomicity`, `FuelDoorFocusIntegration`, `CoolerDryIceIntegration` 등)의 실행 경로를 held-use Apply로 옮긴다. E·F·RMB가 아무 것도 바꾸지 않는다는 단언을 추가한다. 의미는 유지한다.
- 수건: `Towel.AtomicTransferMachineAndRecovery`, `UI.InteractionPromptPrimarySecondaryCapability`의 E/F 경로를 LMB/RMB로 옮긴다. F 최대 이동 단언은 "F 무변화"로 바꾼다. 토큰 보존 단언은 유지한다.
- Prompt widget: optional widget 없이도 동작, 있으면 키 라벨·RMB 행 표시, 빈 E 행 접힘, intent별 transient 행.
- 기존 회귀(유지 필수): Towel, Utility(Labor·Fuel·Lever), Cleaning, Combat, Computer, Placement, Shop, Interaction, Customer towel.
- 전체 회귀: `Automation RunTests BathhouseSim`. 수치와 실패 이름을 보고하고, 실패는 스스로 무관 판정하지 않는다.

## 금지

- 기존 export를 가진 class·property·subobject rename·삭제·class 변경, 기존 enum 값 순서 변경, native `Serialize` 변경, Core Redirect 추가.
- `UPlayerInteractionComponent`에 held-use 상태·Tick·concrete 판별 추가.
- Character·Interaction·Widget에 삽·바구니·수건 concrete cast.
- 이동 가능한 방향·조건·수량 규칙 변경(대기 기계에서 빼기, 선반에서 꺼내기 등).
- F 입력 binding·secondary 계약 삭제.
- Config·Content·Level 저장과 Editor authoring(IA·IMC·WBP는 Editor 단계).

## 단계 결과물

- `.md/PROMPT_REVIEW.md`: 변경 파일, 영역별 요약, 7의 결과와 로그 위치, automation 수치와 CTRL별 대응, 미검증(PIE 전용).
- `.md/PROMPT_UNREAL.md`: 정본 Compatibility의 Editor 목록을 실제 결과에 맞게 구체화한다.
  - `IA_SecondaryUse`(Boolean) 생성, `IMC_FirstPerson` RightMouseButton 연결, `BP_FirstPersonCharacter.SecondaryUseAction`.
  - `WBP_InteractionPrompt`에 RMB 행(`HeldTakeActionNameText`, `HeldTakeFailureReasonText`)과 키 라벨(`PrimaryKeyText`, `LmbKeyText`, `RmbKeyText`)을 추가한다. 기존에 고정 키 문자열이 있으면 중복을 정리한다.
  - PIE 확인: CTRL-003(문), 010·011·014(0.15초 감각), 019, 020, 025(HUD), 026, 027.
- `.md/Architecture/*`는 수정하지 않는다. 설계와 달라야 하면 멈추고 보고한다.
