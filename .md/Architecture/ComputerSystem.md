# Computer System

## Implementation Status

이 문서는 월드 모니터 기반 컴퓨터 상호작용의 현재 native 구현을 정의한다. Computer Actor, player session component, interaction suppression, sample widget과 욕탕 관리 native Widget은 Source에 구현되었고 Blueprint/Editor 연결은 후속 단계다. 욕탕 관리 화면의 상세 계약은 [BathWaterManagementUISystem.md](BathWaterManagementUISystem.md)를 따른다.

2026-09-26 설계(Source 미반영): 포커스인 뒤 클릭 없이 E로 이탈, ESC 이탈, 진입마다 커서 화면 중앙, 컴퓨터별 고정 위치·방향 이탈과 막힘 시 근처 빈자리 탐색을 추가한다. 입력은 `.md/PROMPT_ARCHITECTURE.md`(컴퓨터 포커스 진입·이탈 수정, CMP-001~020)와 `.md/QNA_FEATURE_SPEC.md` Q62~Q68이다.

2026-10-01 서비스 4단위 설계: 세신 포커스(`UPlayerScrubFocusComponent`)가 이탈 위치 helper `FComputerFocusExitPlacement::Resolve`를 재사용한다. 컴퓨터 component는 수정하지 않는다. Character의 컴퓨터 전용 입력 차단 검사는 `IsFocusCapturingInput()`(컴퓨터 || 세신)로 바뀌며, 컴퓨터 쪽 결과는 같다([ServiceAmenitySystem.md](ServiceAmenitySystem.md)).

기존 수직 범위는 컴퓨터 한 대의 포커스 진입/이탈과 클릭 확인용 샘플 화면이다. 다음 구현은 같은 session 계약 위에 욕탕 관리 화면을 연결한다. 범용 운영체제, 프로그램 목록과 저장 시스템은 포함하지 않는다.

## Source Scope

```text
Source/BathhouseSim/Public/Computer/
  BathhouseComputerActor.h
  PlayerComputerUseComponent.h

Source/BathhouseSim/Private/Computer/
  BathhouseComputerActor.cpp
  PlayerComputerUseComponent.cpp
  ComputerFocusExitPlacement.h
  ComputerFocusExitPlacement.cpp

Source/BathhouseSim/Public/UI/
  ComputerSampleScreenWidget.h

Source/BathhouseSim/Private/UI/
  ComputerSampleScreenWidget.cpp
```

focused native automation은 `Source/BathhouseSim/Private/Tests`에 둔다.

## Responsibilities

- 월드 컴퓨터 mesh, world-space screen과 focus camera 조립
- 한 명의 유효한 local player에 대한 컴퓨터 사용권 예약/해제
- 빈손일 때만 사용 가능한 primary interaction query/execute
- 컴퓨터 사용 단계, 이전 view target, 이동 모드, 커서와 pointer lifecycle 관리
- 일반 1인칭/월드 interaction과 컴퓨터 pointer input의 상호 배타적 routing
- 포커스아웃 뒤 actor lifetime 동안 screen widget instance와 표시 상태 유지
- 컴퓨터 또는 player EndPlay와 focus transition 중단 시 안전한 복구
- 정상 포커스아웃 시 컴퓨터별 고정 이탈 위치·방향 결정, 막힘 시 근처 빈자리 탐색과 캐릭터 배치

Computer는 physical carry state, 기존 interaction target 선정, 일반 이동/sprint 상태, 욕탕 condition·용량 원장과 concrete WBP hierarchy를 소유하지 않는다.

## Responsibility Change

| 대상 | 기존 책임 | 신규 책임 | 최종 판단 |
|---|---|---|---|
| `AFirstPersonCharacter` | component 조립과 input intent routing | computer component/widget interaction 조립, 입력 mode 분기 | 세션 상태는 추가하지 않고 composition/routing만 확장 |
| `UPlayerInteractionComponent` | world focus와 E/F/G lifecycle | 외부 focus mode 동안 query/hold 억제 | 도메인 판정 없이 suppression 계약만 확장 |
| `ABathhouseComputerActor` | 없음 | 월드 표현, focus camera, use reservation | 신규 Actor가 컴퓨터 한 대의 composition root |
| `UPlayerComputerUseComponent` | 없음 | player별 focus/input/view session | lifecycle과 임시 상태가 응집되므로 신규 Component |
| `UComputerSampleScreenWidget` | 없음 | 클릭 연결과 표시용 클릭 확인 상태 | Native Widget policy에 따라 신규 C++ base |
| `ABathhouseComputerActor.ManagedBathPlacementZone` | 없음 | 관리 화면 context인 Zone instance reference | Actor가 context를 resolve해 root widget에 주입 |
| 욕탕 관리 Widget 계층 | 없음 | snapshot 표시와 setting intent 전달 | UI 계층만 소유하고 domain mutation은 subsystem에 위임 |

범용 program manager, computer app base와 별도 screen state UObject는 현재 단일 샘플 화면에 필요하지 않아 만들지 않는다. 실제 gameplay 데이터가 추가되면 Widget 밖의 Computer domain owner로 분리한다.

## `ABathhouseComputerActor`

native default subobject:

- `ComputerMesh: UStaticMeshComponent`
- `ScreenWidget: UWidgetComponent`
- `FocusCamera: UCameraComponent`
- `FocusExitPoint: USceneComponent`(신규, root 하위): 정상 포커스아웃 때 캐릭터 **발바닥** 위치와 바라볼 방향. 위치는 컴퓨터 local cm이며 월드 rotation의 yaw·pitch만 쓰고 roll은 무시한다.
- `FocusExitArrow: UArrowComponent`(신규, editor-only, `FocusExitPoint` 하위): Editor viewport에서 이탈 자리와 시선 방향을 보여 주는 표시 전용 component. game에서는 존재하지 않으며 runtime은 이 component를 읽지 않는다.

Actor는 `IPlayerInteractable`을 직접 구현한다.

- `BeginPlay`에서 `ScreenWidget->InitWidget()`을 한 번 보장하고 실제 user widget 생성 여부를 사용 가능 조건에 포함한다.
- 2026-09-27 상점: screen root user widget이 `IComputerScreenContextReceiver`(Computer, C++ 전용)를 구현하면 BeginPlay에 `InitializeComputerScreen(Context{Computer, OperationsSubsystem, ManagedBathPlacementZone})`를 호출한다. 관리 화면을 직접 cast하던 경로는 이 interface로 바꾸고 `UBathWaterManagementScreenWidget`도 구현해 기존 WBP 직접 사용을 호환한다.
- reservation과 session 시작이 성공하면 `NotifyComputerUserChanged(PlayerState)`를 호출한다. 사용 종료 때 사용자를 지우지 않아 화면이 마지막 사용자 기준을 유지한다. Computer는 Shop을 알지 못한다.
- `QueryInteraction`은 carry context가 유효하고 `IsHandEmpty()`이며 screen/focus camera가 사용 가능하고 다른 user가 없을 때 `컴퓨터 사용`을 허용한다.
- 손에 key, wet mop 또는 towel basket이 있으면 `손에 든 물건을 내려놓아야 합니다`를 반환한다.
- `ExecuteInteraction`은 같은 조건을 다시 검증하고 interactor의 `UPlayerComputerUseComponent`에 시작을 요청한다.
- Actor reservation과 player session 시작은 한 transaction처럼 처리한다. session 시작 실패 시 reservation을 원복한다.
- current user identity를 검증한 release만 허용해 중복 종료나 다른 player의 해제를 막는다.
- `EndPlay`는 current user component에 actor unavailable을 통지하고 reservation을 지운다.
- `ManagedBathPlacementZone`은 `EditInstanceOnly`로 authoring하며 management root에는 이 Zone과 `UBathWaterOperationsSubsystem`을 명시적으로 주입한다. Widget이 world scan으로 대상을 정하지 않는다.

`FocusExitSearchRadiusCm`(신규, EditAnywhere, 기본 `100`, finite ≥ 0)은 고정 위치가 막혔을 때 빈자리를 찾는 최대 수평 거리다. `FocusExitPoint` transform은 Blueprint class 기본값과 Level instance override 모두 허용한다. native 기본 위치는 실제 자리가 아니므로 Editor가 반드시 정한다. `GetFocusExitFootTransform()`은 발바닥 위치와 `(Pitch, Yaw, 0)` 회전을 반환한다.

`FocusBlendInSeconds`, `FocusBlendOutSeconds`는 음수가 아닌 Editor authoring 값이며 native 기본값은 각각 `0.35`, `0.25`초다. `FocusCamera`의 relative transform과 FOV는 monitor를 정면 중앙에 두되 주변 world가 보이도록 Blueprint class/instance에서 조정한다.

## `UPlayerComputerUseComponent`

player별 authoritative transient session owner다. 내부 phase는 `Inactive`, `FocusingIn`, `Active`, `FocusingOut`을 사용한다.

시작 전 검증:

- owner가 locally controlled pawn이고 유효한 player controller를 가진다.
- active computer/session이 없다.
- `UPlayerCarryComponent`가 빈손이다.
- target reservation이 성공한다.

진입 시 다음을 한 commit/rollback 경계에서 수행한다.

1. 이전 view target, movement mode/custom mode와 cursor 표시값을 저장한다.
2. sprint와 jump를 끝내고 movement를 즉시 정지한 뒤 `MOVE_None`으로 전환한다.
3. `UPlayerInteractionComponent`를 suppress해 active hold를 조용히 한 번 cancel하고 prompt/query를 지운다.
4. computer Actor를 `SetViewTargetWithBlend` 대상으로 설정한다.
5. blend가 끝나면 `Active`로 전환하고 다음 순서로 조작 가능 상태를 만든다.
   1. mouse cursor 표시.
   2. `FInputModeGameAndUI`를 **widget focus 없이**(`SetWidgetToFocus` 미지정) 적용하고 기존 `DoNotLock`, `SetHideCursorDuringCapture(false)`를 유지한다. 이어서 UMG `UWidgetBlueprintLibrary::SetFocusToGameViewport()`로 실제 사용자의 keyboard focus를 game viewport에 둔다.
   3. `GetViewportSize`의 중앙으로 `SetMouseLocation`한다. 매 진입마다 적용하며 이전 커서 위치를 기억하지 않는다.
   4. 마지막에 widget pointer hit testing을 켠다. 커서 배치만으로 press/click이 생기지 않는다.
   - 이전 구현은 `SetWidgetToFocus(ScreenWidget->TakeWidget())`로 keyboard focus를 world-space screen의 Slate widget에 줬다. 그 widget은 viewport 계층 밖이라 E·ESC가 Enhanced Input에 닿지 않았고, 화면 클릭으로 viewport가 focus를 되찾은 뒤에야 E가 동작했다. world screen은 pointer 입력만 `UWidgetInteractionComponent` virtual user로 받으므로 keyboard focus가 필요 없다.
6. UE 5.8의 CameraComponent에는 view별 AA method override가 없으므로, active computer focus 동안 태그가 붙은 `r.AntiAliasingMethod=FXAA` override를 적용해 world-widget의 TSR history 잔상을 피한다.

focus-in 중 E나 ESC를 누르면 focus-out으로 전환한다. pointer click은 `Active`에서만 허용한다. focus-out 중 추가 E·ESC는 무시한다.

정상 종료(`RequestEndComputerUse`, `FocusingIn`·`Active`에서만):

1. pointer press가 남았다면 release하고 hit testing과 cursor를 끈다. 슬라이더 drag는 이 release로 끝나며 이후 값이 바뀌지 않는다. focus용 AA override를 태그로 unset한다. `FInputModeGameOnly`를 적용한다.
2. 아래 Focus Exit Placement로 캐릭터 capsule 중심 위치를 정하고, 같은 frame에 pawn을 sweep 없이 teleport한다(`ETeleportType::TeleportPhysics`). actor rotation은 yaw만, controller control rotation은 `(Pitch, Yaw, 0)`으로 둔다. camera manager의 pitch 제한은 그대로 적용된다. movement는 아직 `MOVE_None`이다.
3. 옮긴 pawn을 view target으로 `FocusBlendOutSeconds` 동안 blend한다. pawn이 먼저 이동했으므로 blend 중 진입 직전 시점이 보이지 않고, 끝난 뒤 추가 순간이동이 없다.
4. blend 완료 시 저장한 movement mode, 1인칭 입력과 interaction query를 복구하고 reservation을 해제한다. authoring 위치가 바닥보다 높으면 movement 복구 뒤 일반 낙하한다. blend가 0이면 같은 frame에 완료한다.

`PreviousViewTarget`은 비정상 복구 경로에서만 쓴다. 정상 종료의 view target은 항상 owner pawn이다.

컴퓨터 또는 player/controller가 유효하지 않게 되면 고정 위치 이동 없이 timer와 pointer state를 정리하고 가능한 player pawn/view target, `FInputModeGameOnly`, 저장된 movement mode와 interaction을 즉시 복구한다. begin/end/unavailable은 반복 호출해도 상태를 복제하거나 잠금을 남기지 않는다.

## Focus Exit Placement

private 비-UObject helper `FComputerFocusExitPlacement`(`Private/Computer/ComputerFocusExitPlacement.*`)가 world query만으로 이탈 위치를 계산한다. 상태를 갖지 않으며 session component가 결과로 pawn을 옮긴다.

입력: world, player capsule(scaled radius·half height, collision object type, channel responses), 무시할 player actor, 발바닥 위치 F, 고정 yaw, 탐색 반경 R.

1. 기준 중심 `C0 = F + Up * ScaledHalfHeight`. 바닥 높이 보정은 하지 않는다(Q68 B).
2. `IsFree(C)`: player capsule shape·object type·response로 `OverlapBlockingTestByChannel`, player는 무시. player 이동을 막는 것(벽, 설비, 손님 capsule)만 장애물이다. Pawn을 무시하는 free-world item은 장애물이 아니다.
3. `IsFree(C0)`면 C0.
4. 아니면 `C0`에서 겹치는 blocking component 집합 `B0`를 구한다. 반경 `r = 10, 20, …`cm(마지막은 R)인 수평 고리를 가까운 순으로 돈다. 각 고리는 `max(8, ceil(2πr / 10))`개 각도를 고정 yaw부터 균등 배치한다. 후보 `C = C0 + r·(cosθ, sinθ, 0)`, 높이는 C0와 같다.
5. 후보는 `IsFree(C)`이고, `C0 → C` player capsule sweep이 `B0`와 player를 무시한 채 blocking hit가 없을 때만 채택한다. sweep으로 벽 너머·다른 방·좁은 틈 너머를 제외한다. 처음 채택한 후보가 가장 가까운 자리다.
6. 후보가 없으면 C0로 강제 이탈한다. 어떤 경우에도 다른 actor를 옮기거나 밀지 않는다. 강제 이탈 뒤 빠져나오는 것은 CharacterMovement의 일반 penetration 처리와 이동에 맡긴다.
7. `R = 0`이면 탐색 없이 1·6만 적용한다.

결과: capsule 중심, 사용한 경로(고정·탐색·강제), 탐색 거리. 경로가 고정이 아니면 `LogBathhouseComputer` Verbose로 남긴다.

## Input Routing

`AFirstPersonCharacter`는 `UPlayerComputerUseComponent`와 `UWidgetInteractionComponent`를 default subobject로 조립한다. Widget interaction은 평상시에 hit testing을 하지 않는다.

- E Started: computer phase가 `Inactive`가 아니면 focus-out intent로 소비하고, 아니면 기존 primary begin으로 전달한다.
- E Completed/Canceled: computer가 현재 press를 소유하면 소비한다. 진입에 사용한 E release가 즉시 focus-out을 일으키지 않는다.
- `CancelAction` Started(신규, 기본 ESC): computer phase가 `FocusingIn`·`Active`면 focus-out을 요청하고, `FocusingOut`이면 무시한다. computer를 쓰지 않으면 아무것도 하지 않으며 placement·레버·회수 등 다른 동작에 전달하지 않는다. Started 한 번만 쓰므로 누르고 있어도 전환은 한 번이다.
- `PrimaryUseAction` Started/Triggered/Completed/Canceled: input owner priority `Computer > Placement > Equipment`를 유지한다. Computer가 `Active`일 때 left pointer press/release를 소유한다.
- deprecated `ComputerClickAction`은 `PrimaryUseAction`이 비어 있는 기존 Blueprint를 위한 fallback으로만 유지한다.
- Move/Look/Jump/Sprint/F/G: computer session이 input을 capture하는 동안 Character의 input-facing handler와 공개 `DoMove`/`DoLook`/`DoJumpStart` 경로에서 domain 호출 전에 차단한다.

전체 `DisableInput()`은 종료 E와 pointer 입력까지 막으므로 사용하지 않는다. 기존 mapping context는 교체하지 않는다. ESC는 computer 전용이 아닌 범용 `취소/뒤로` intent `IA_Cancel`로 추가하고, E를 매핑한 기존 IMC에 Escape를 매핑한다. 이후 게임 메뉴·범용 포커스아웃이 같은 action을 재사용하며 이번에는 computer 이탈만 연결한다.

## Interaction Suppression

`UPlayerInteractionComponent`는 C++ 전용 `SetInteractionSuppressed(bool)`과 조회 API를 제공한다.

- suppress 시작은 active hold를 transient failure 없이 정확히 한 번 cancel하고 focus/current query를 empty로 commit한다.
- suppress 중 Tick trace와 E/F/G public attempt는 target/domain mutation을 하지 않는다.
- suppress 해제는 즉시 `RefreshInteractionQuery()`를 실행한다.
- Interaction은 Computer concrete class를 include하거나 active computer state를 직접 판정하지 않는다.

Character의 정상 input gate가 1차 경계이고 suppression은 stale prompt, active hold와 외부 API 호출을 막는 2차 경계다.

## Screen And UI Contract

`ScreenWidget`은 World Space로 표시되고 `UWidgetInteractionComponent`의 mouse source/hit test를 받는다. `bReceiveHardwareInput` 방식과 동시에 사용하지 않아 중복 클릭을 막는다.

`UComputerSampleScreenWidget`은 다음 필수 `BindWidget`만 가진다.

- `TestButton: UButton`
- `ClickResultText: UTextBlock`

native C++은 construct/destruct의 대칭 delegate 연결, 클릭 여부와 text 갱신을 소유한다. Widget Blueprint는 hierarchy, layout, style와 선택적 animation만 소유한다. 초기 표시는 `버튼을 클릭하세요`, 클릭 뒤 표시는 `클릭 확인`이다.

focus-out은 `ScreenWidget`이나 user widget을 remove/recreate하지 않는다. 따라서 마지막 클릭 상태는 같은 computer Actor lifetime 동안 유지되고 재진입 시 그대로 보인다. Actor 파괴 또는 level reload 뒤의 영속 저장은 현재 범위 밖이다.

욕탕 관리 화면의 native hierarchy, 지도 투영, slider request와 refresh 정책은 [BathWaterManagementUISystem.md](BathWaterManagementUISystem.md)가 정본이다. 기존 sample widget은 삭제하지 않고 회귀와 asset 호환을 위해 보존한다.

## Dependencies

- Character -> Computer
- Computer -> Interaction public query/carry 계약
- Computer -> Engine Camera/CharacterMovement/PlayerController
- Computer -> UMG `UWidgetComponent`, `UWidgetInteractionComponent`
- UI sample widget -> UMG
- Computer -> UMG `UWidgetBlueprintLibrary::SetFocusToGameViewport`(기존 `UMG` dependency, Slate 직접 사용 없음)
- Computer -> Engine collision query(overlap·sweep) for focus exit placement
- Computer/UI management screen -> Bath Water Operations snapshot/request API
- Interaction은 Computer concrete type에 의존하지 않는다.

현재 `UMG`, `InputCore`와 `EnhancedInput` module dependency로 구현한다. direct Slate API 사용처가 없으므로 `Slate`, `SlateCore`를 추가하지 않는다.

## Blueprint/API Contracts

신규 reflected 계약:

- `ABathhouseComputerActor` native parent와 `ComputerMesh`, `ScreenWidget`, `FocusCamera`
- `AFirstPersonCharacter::PlayerComputerUse`, `ComputerWidgetInteraction`, `ComputerClickAction`
- `AFirstPersonCharacter::PrimaryUseAction`을 canonical LMB로 사용하고 `ComputerClickAction`은 deprecated fallback으로 유지
- `ABathhouseComputerActor::ManagedBathPlacementZone` instance reference
- `IComputerScreenContextReceiver`와 `FComputerScreenContext`(2026-09-27). `BP_BathhouseComputer.ScreenWidget.WidgetClass`는 탭 root WBP로 교체한다([ShopSystem.md](ShopSystem.md))
- `UComputerSampleScreenWidget` native parent와 `TestButton`, `ClickResultText` BindWidget
- computer focus camera/blend와 widget interaction distance/debug authoring 값
- 2026-09-26 신규: `ABathhouseComputerActor::FocusExitPoint`, editor-only `FocusExitArrow`, `FocusExitSearchRadiusCm`, `AFirstPersonCharacter::CancelAction`, Content `IA_Cancel`

기존 reflected symbol을 rename/delete하지 않으므로 Core Redirect는 필요하지 않다. 신규 default subobject 추가는 기존 `BP_BathhouseComputer`·DefaultMap instance export와 이름이 겹치지 않는다. 그래도 CoreSystem 규칙에 따라 Editor 작업 전 `BP_BathhouseComputer` 복사본 비저장 로드를 먼저 확인한다. Content asset 생성과 assignment는 Unreal 단계에서 수행한다. 읽기 전용 조사상 `IA_ComputerClick`은 없고 `IA_PrimaryUse`가 LMB canonical mapping이다.

## Out Of Scope

- fullscreen viewport UI와 game pause
- 범용 desktop, app window, keyboard text input와 scroll
- 계정, 결제, 경제 또는 저장 데이터
- 여러 player의 network replication
- level reload를 넘는 monitor 상태 저장
- 컴퓨터 사용 중 held item 자동 drop 또는 보관

## Manual Review Points

- 빈손일 때만 진입하며 실패 query와 execute가 동일한 이유를 반환하는지 확인한다.
- Active 전환이 screen widget에 keyboard focus를 주지 않고 viewport focus·커서 중앙·hit testing 순서를 지키는지 확인한다.
- 정상 이탈이 진입 위치와 무관하게 고정 위치·방향으로 teleport한 뒤 blend하고, 비정상 복구는 teleport하지 않는지 확인한다.
- 막힘 탐색이 다른 actor를 옮기지 않고 벽 너머 후보를 고르지 않는지 확인한다.
- 진입에 사용한 E release는 유지되고 새로운 E Started만 focus-out을 시작하는지 확인한다.
- focus-in/out과 반복 E에서도 reservation, timer와 input lock이 정확히 한 번 정리되는지 확인한다.
- 사용 중 Move/Look/Jump/Sprint/F/G와 world trace가 mutation을 만들지 않는지 확인한다.
- mouse press/release와 focus-out 강제 release가 button stuck 또는 double click을 만들지 않는지 확인한다.
- monitor가 정면으로 보이면서 주변 목욕탕이 viewport에 남는지 플레이 테스트한다.
- focus-out/re-entry 뒤 `클릭 확인` 상태가 유지되고 widget construct가 반복되지 않는지 확인한다.
- active focus에서는 FXAA가 적용되고 정상 종료와 강제 cleanup 뒤에는 진입 전 AA method가 복구되는지 확인한다.
- computer/player EndPlay 뒤 view target, cursor, input mode, movement와 interaction prompt가 복구되는지 확인한다.
- world/NPC simulation이 computer 사용 중 pause되지 않는지 확인한다.
- 여러 computer가 같은 `ManagedBathPlacementZone`을 참조할 때 같은 욕탕 설정/용량을 표시하고, Zone 누락 시 안전한 unavailable 화면을 표시하는지 확인한다.
