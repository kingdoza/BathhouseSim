# 구현 프롬프트 — 컴퓨터 포커스 진입·이탈 수정

## 단계와 입력

- 2026-09-26 사용자가 기능 계약 `.md/PROMPT_ARCHITECTURE.md`(컴퓨터 포커스 진입·이탈 수정, CMP-001~020)를 승인했다. 기능 계약 머리의 "승인 대기" 문구는 기능 명세 단계 소유라 이 단계에서 고치지 않는다.
- 현재 단계: 수직 구현. 대표 시나리오가 전체 범위와 같아 CMP-001~020 전체를 한 단계에서 완성한다.
- `.md/AGENT_WORKFLOW.md` → `.md/AGENT_IMPLEMENTATION.md`를 읽고 C++ 구현 단계만 수행한다.
- 설계 정본(전체 읽기):
  - `.md/Architecture/ComputerSystem.md`
  - `.md/Architecture/CharacterSystem.md`의 `CancelAction`
  - `.md/Architecture/CoreSystem.md`의 Core Redirect·Module Rules
  - 이 문서와 정본이 다르면 구현을 멈추고 보고한다.
- 작업 트리에는 쿨러·순환기 확장의 미커밋 변경이 있고 그 통합 리뷰가 진행 중이다. Utility 관련 파일은 수정하지 않는다.

## 현재 → 목표

| 항목 | 현재 Source | 목표 |
|---|---|---|
| Active 입력 초점 | `FInputModeGameAndUI.SetWidgetToFocus(ScreenWidget->TakeWidget())`. keyboard focus가 viewport 밖 world widget으로 가서 클릭 전까지 E가 Enhanced Input에 닿지 않음 | widget focus 없이 GameAndUI, `UWidgetBlueprintLibrary::SetFocusToGameViewport()` |
| ESC | 없음 | 범용 `IA_Cancel` / `AFirstPersonCharacter::CancelAction`, computer capture 중에만 focus-out |
| 커서 | 표시만 함 | Active 진입마다 viewport 중앙 `SetMouseLocation`, 그 뒤 hit testing on |
| 정상 이탈 | 이전 view target으로 blend, 진입 직전 자리 | 고정 발바닥 위치·방향으로 teleport 후 pawn으로 blend |
| 막힘 | 해당 없음 | 최대 반경 안 도달 가능한 가장 가까운 빈자리, 없으면 강제 |
| 비정상 종료 | 즉시 복구 | 동일, teleport 없음 |

## 0. 사전 조건과 백업

- UnrealEditor가 모두 종료됐는지 확인한다.
- git 명령은 `--no-optional-locks`로 실행해 `.git/index.lock`을 남기지 않는다.
- 다음 두 파일을 `Saved/MigrationBackup/20260926_computer/`에 파일 복사한다.
  - `Content/Bathhouse/Blueprints/Computer/BP_BathhouseComputer.uasset`
  - DefaultMap 컴퓨터 외부 actor `Content/__ExternalActors__/Maps/DefaultMap/7/EH/E4FLO971KSWUJ40H7W7PHK.uasset`
- Content·Config·Level을 저장하지 않는다.

## 1. `ABathhouseComputerActor`

- `FocusExitPoint : USceneComponent`를 root 하위에 추가한다.
- `CreateEditorOnlyDefaultSubobject`로 `FocusExitArrow : UArrowComponent`를 `FocusExitPoint` 하위에 추가한다(표시 전용). null일 수 있으므로 runtime 코드는 참조하지 않는다.
- `UPROPERTY(EditAnywhere, meta=(ClampMin=0)) float FocusExitSearchRadiusCm = 100.f;`
- `FTransform GetFocusExitFootTransform() const`: `FocusExitPoint` 월드 위치, rotation은 `(Pitch, Yaw, 0)`.
- getter `GetFocusExitSearchRadiusCm()`. 값이 nonfinite거나 음수면 0으로 취급하고 경고를 남긴다.
- 기존 `ComputerMesh`, `ScreenWidget`, `FocusCamera`, `ManagedBathPlacementZone`, blend 값의 이름·기본값은 유지한다.

## 2. 이탈 위치 helper

- 신규 `Private/Computer/ComputerFocusExitPlacement.h/.cpp`에 비-UObject `FComputerFocusExitPlacement`를 둔다.
- 알고리즘은 `ComputerSystem.md` Focus Exit Placement 1~7을 그대로 따른다.
  - 고리 간격 10cm, 고리당 각도 수 `max(8, ceil(2πr/10))`, 시작 각도는 고정 yaw.
  - player capsule의 object type·channel response로 overlap과 sweep을 수행한다.
  - sweep은 `C0`에서 겹친 blocking component와 player를 무시한다.
- 결과 구조: `CapsuleCenter`, `EPath { Fixed, Searched, Forced }`, `SearchDistanceCm`.
- world query 외 상태나 side effect가 없다.

## 3. `UPlayerComputerUseComponent`

**`CompleteFocusIn` 순서:**

1. cursor를 켠다.
2. `FInputModeGameAndUI`에 `DoNotLock`과 `SetHideCursorDuringCapture(false)`를 적용한다. `SetWidgetToFocus` 호출은 제거한다.
3. `SetFocusToGameViewport()`를 호출한다.
4. `GetViewportSize` 중앙으로 `SetMouseLocation`한다.
5. `WidgetInteraction->bEnableHitTesting = true`.

기존 AA override 적용 위치는 유지한다.

**`RequestEndComputerUse`**(`FocusingIn`·`Active`에서만, 그 밖은 기존처럼 무시):

1. timer를 정리하고, AA override를 해제하고, pointer를 release하고, hit testing을 끄고, cursor를 숨기고, `FInputModeGameOnly`를 적용한다.
2. `ActiveComputer`와 owner `ACharacter`의 capsule로 `FComputerFocusExitPlacement`를 계산한다. 이어서 actor를 `SetActorLocationAndRotation(Center, FRotator(0,Yaw,0), false, nullptr, ETeleportType::TeleportPhysics)`로 옮기고, controller를 `SetControlRotation(FRotator(Pitch,Yaw,0))`로 둔다.
3. owner pawn을 view target으로 `FocusBlendOutSeconds` blend한다. `ResolveReturnViewTarget`은 비정상 경로 전용으로 남긴다.
4. `Phase = FocusingOut`. 이후 흐름(timer → `CompleteFocusOut`)은 기존과 같다.

- computer가 이미 invalid이거나 capsule을 얻을 수 없으면 teleport 없이 기존 `ForceCleanup` 경로를 쓴다.
- `HandleComputerUnavailable`, `ForceCleanup`, `EndPlay`는 teleport하지 않는다(CMP-017).
- 이탈 경로가 Fixed가 아니면 `LogBathhouseComputer` Verbose 로그를 남긴다. 해당 log category가 없으면 Computer 파일 안에 정의한다.

## 4. Character와 입력

- `AFirstPersonCharacter`에 `UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") TObjectPtr<UInputAction> CancelAction;`을 추가한다.
- `CancelAction`의 Started만 `CancelInput()`에 bind한다.
- `CancelInput()`: `PlayerComputerUse && IsCapturingInput()`이면 `RequestEndComputerUse()`, 아니면 아무것도 하지 않는다.
- E 경로(`InteractStartInput`/`InteractEndInput`과 `bComputerOwnsInteractPress`)는 바꾸지 않는다. 진입 E를 떼도 나가지 않는 동작(CMP-006)을 유지한다.
- 새 Input Action asset과 IMC 매핑은 Editor 단계에서 한다. Source는 `CancelAction`이 null이어도 동작한다.

## 5. 금지

- world screen widget이나 다른 UMG widget에 keyboard focus를 주는 것. Slate/SlateCore module 추가.
- 커서 lock behavior 변경, 게임 메뉴·일시정지, ESC를 다른 모드 취소에 연결하는 것.
- 바닥 높이 자동 맞춤, 다른 actor를 옮기는 depenetration 코드.
- 포커스인 시점의 캐릭터 이동.
- 기존 reflected 이름 rename·delete. Core Redirect 추가.
- Utility·Interaction·UI 등 무관한 시스템 수정.
- Content·Config·Level 저장과 Editor authoring.

## 6. 빌드

- `.md/AGENT_WORKFLOW.md`의 UE 5.8 `Build.bat`만 사용한다.

## 7. 로드 검증 — 복사본 먼저

신규 영구 automation `BathhouseSim.Computer.BlueprintLoad`를 만든다.

- 기본 대상 `/Game/Bathhouse/Blueprints/Computer/BP_BathhouseComputer`, `-BathhouseComputerLoadPath=`로 단일 package를 지정한다.
- `LoadPackage` 뒤 package 안의 `UBlueprint`를 찾는다. 저장하지 않는다.
- 확인:
  - native parent가 `ABathhouseComputerActor`다.
  - CDO에 `FocusExitPoint`가 있다.
  - `ScreenWidget` WidgetClass가 비어 있지 않다.
  - blend 값이 유지된다.
  - world에 컴퓨터 instance가 있으면 `ManagedBathPlacementZone` 참조가 유지되고 `FocusExitPoint`가 존재한다.

순서. 어느 단계든 Fatal·crash·`Serial size mismatch`·`Failed to load`가 나오면 즉시 멈추고 보고한다.

1. `BP_BathhouseComputer.uasset`을 `Content/Developers/MigrationCheck/BP_BathhouseComputer_LoadCheck.uasset`으로 복사한다. Template 맵 headless로 비저장 로드한다.
2. 복사본 폴더를 삭제하고 `git --no-optional-locks status`로 Content 무변경을 확인한다.
3. 원본을 비저장 로드한다(Template 맵).
4. DefaultMap 맵 인자로 같은 test를 실행한다.
5. Content·Config 무변경을 다시 확인한다.

## 8. 자동화

신규·확장 test. 기존 `ComputerAutomationTests.cpp`의 harness를 재사용한다.

| ID | 검증 |
|---|---|
| CMP-001, 003, 004, 009 | Active·FocusingIn에서 `RequestEndComputerUse` → pawn capsule 중심이 이탈 결과, actor yaw·control `(Pitch,Yaw,0)`. 서로 다른 시작 위치·방향 3곳에서 같은 결과 |
| CMP-002, 014, 015 | `CancelInput`: capturing이면 E와 같은 결과, `FocusingOut` 중 무시, 비사용 시 호출 없음·placement 등 다른 상태 불변 |
| CMP-005 | pointer down 상태에서 이탈 → release 1회, 이후 PressPointer 거부, 이후 hit testing off |
| CMP-006 | 기존 E press ownership test 유지 |
| CMP-011~013 | helper geometry: 고정 자리 free / capsule 장애물 → 가장 가까운 free 자리 / 벽 너머가 더 가까워도 같은 쪽 / 반경 안 전부 막힘 → Forced / R=0. 장애물 actor 위치 불변 |
| CMP-016 | 정상 이탈 완료 뒤 저장한 movement mode 복구, interaction suppression 해제, reservation 해제, 같은 컴퓨터 재진입 성공 |
| CMP-017 | `HandleComputerUnavailable`·EndPlay → teleport 없음, 입력·movement 복구 |
| CMP-019, 020 | 높이 보정 없음: 결과 Z = 발바닥 Z + half height. 낙하와 눈높이는 PIE |
| 회귀 | 기존 `BathhouseSim.Computer.*` 전부 |

- Slate keyboard focus와 실제 커서 위치(CMP-001, 002, 007, 008, 010)는 headless로 판정할 수 없다. 코드 경로를 test로 확인하고 수용은 PIE로 인계한다.
- 전체 회귀: `Automation RunTests BathhouseSim`(Template 맵 headless). 총/성공/실패/건너뜀 수와 실패 이름을 보고하고, 실패는 스스로 무관 판정하지 않는다.

## 단계 결과물

- `.md/PROMPT_REVIEW.md`: 변경 파일, 원인 수정 설명(widget focus 제거), 로드 검증 5단계 결과, automation 수치와 CMP별 대응, 미검증(PIE 전용 항목).
- `.md/PROMPT_UNREAL.md`: 아래 Editor 인계를 실제 결과에 맞게 구체화한다.
- `.md/Architecture/*`는 수정하지 않는다.

## 후속 Editor 인계 — 지금 실행하지 않음

- **입력 asset:**
  - `/Game/Input/Actions/IA_Cancel`(Digital bool) 생성.
  - `IA_Interact`를 매핑한 기존 IMC에 Escape → `IA_Cancel` 추가.
  - 플레이어 Character Blueprint의 `CancelAction` 지정.
  - 사용자 Editor의 PIE 종료 키 F2는 사용자 설정이며 프로젝트 파일로 바꾸지 않는다.
- **`BP_BathhouseComputer`:**
  - DefaultMap 컴퓨터용 `FocusExitPoint`를 모니터 앞 바닥 높이·모니터를 보는 방향으로 둔다. Arrow로 확인한다.
  - 컴퓨터가 한 대이므로 class 기본값으로 정해 World Partition 외부 actor 저장을 피하는 것을 우선한다. instance override가 필요하면 `USER_UNREAL.md`의 외부 actor 저장 기준을 따른다.
  - Compile, 개별 Save, 재로드 뒤 ScreenWidget, 관리 Zone, blend 값 유지를 확인한다(CMP-018).
- **PIE:**
  - 클릭 없이 E·ESC 이탈, 전환 중 이탈, 슬라이더 drag 중 이탈.
  - 커서 중앙과 재진입 시 다시 중앙, 세 위치에서 진입 → 같은 자리.
  - 0.25초 blend 외형, 손님·벽·전부 막힘.
  - 비사용 ESC 무반응(배치·레버·회수 중), 30cm 위 authoring 뒤 낙하, 바닥 높이.
- MCP로 불가한 작업은 exact path와 근거를 `USER_UNREAL.md`에 인계하고, `.md/Unreal/InteractionUISystem.md`는 Editor 단계가 갱신한다.
