# Interaction System

2026-09-28 Held Target Use: 들고 있는 물건으로 대상에 하는 일은 LMB(Apply)·RMB(Take) held-use다. 삽 퍼담기·반환·투입과 수건 이동이 E/F에서 옮겨졌고 F는 예약으로 비워 둔다. 계약·입력 소유·연속 실행·HUD 데이터는 [HeldTargetUseSystem.md](HeldTargetUseSystem.md)가 정본이다.

2026-09-26 Utility Labor(2026-09-28 대체): 삽 퍼담기·반환·투입은 공급함·투입 판정 Volume target의 E primary였다. 보일러·쿨러 문은 generic `IPlayerInteractionFocusObserver`를 쓴다. 순환기 레버 왕복은 Instant E로 시작하는 시간 작업이며, 진행 표시를 위해 query에 `bPrimaryProgressVisible`을 추가한다. generic router에는 concrete 설비 분기를 넣지 않는다.

## Implementation Status

이 문서는 현재 구현된 primary/secondary/hold interaction, Computer suppression과 LMB equipment-use를 정의하고 Placement target의 LMB confirm/Q Hold prompt 합성 경계를 추가한다. carry 상세는 [PhysicalCarrySystem.md](PhysicalCarrySystem.md), placement 실행은 [PlacementSystem.md](PlacementSystem.md)를 따른다.

## Source Scope

```text
Source/BathhouseSim/Public/Interaction/
  InteractionTypes.h
  PlayerInteractable.h
  PlayerInteractionFocusObserver.h
  PhysicalCarryable.h
  PhysicalCarryDiscardable.h
  PhysicalCarryFixedSlot.h
  PhysicalCarryFixedSlotActor.h
  HeldEquipmentUsable.h
  PlayerInteractionComponent.h
  PlayerCarryComponent.h
  PlayerEquipmentUseComponent.h
  PlayerHeldTargetUseComponent.h
  HeldEquipmentMotionComponent.h
  BathhouseKeyActor.h
  BathhouseKeyHookActor.h

Source/BathhouseSim/Private/Interaction/
  PlayerInteractionComponent.cpp
  PhysicalCarryFixedSlotActor.cpp
  PhysicalCarryPlacementTransaction.h
  PhysicalCarryPlacementTransaction.cpp
  PlayerCarryComponent.cpp
  PlayerEquipmentUseComponent.cpp
  PlayerHeldTargetUseComponent.cpp
  HeldEquipmentMotionComponent.cpp
  BathhouseKeyActor.cpp
  BathhouseKeyHookActor.cpp
  PlayerViewFrontPlacement.h/.cpp  # 카메라 시선 앞 생성 순수 기하(Shop 개봉·Cleaning 봉투 공유)

Source/BathhouseSim/Private/Tests/
  BathhouseDomainTests.cpp  # single-key carry와 interaction attempt result coverage
  CleaningTowelAutomationTests.cpp  # mop/basket carry, hold cleaning과 physical placement coverage
  ComputerAutomationTests.cpp  # suppression, computer focus/input/pointer/cleanup coverage
  CombatRecoveryAutomationTests.cpp  # equipment routing, health, melee와 soft interruption coverage
```

## Responsibilities

- first-person camera 중앙 line trace와 primary/secondary interaction 실행
- instant/hold primary lifecycle과 target/context 재검증
- side-effect 없는 상호작용 조회와 실행 직전 재검증
- query 상태와 별개인 interaction 실행 결과의 일회성 native notification
- inventory/hotbar 없는 single physical carry의 입력·query 연결
- 공용 held anchor, item별 local held transform, fixed-slot/free-drop 계약 연결
- LMB 장비 사용의 side-effect-free query, Begin/Update/End/Cancel routing
- 장비가 아닌 들고 있는 물건의 LMB Apply·RMB Take held-use routing과 연속 실행([HeldTargetUseSystem.md](HeldTargetUseSystem.md))
- one-shot/hold equipment use 공통 lifecycle와 held Actor transform 표현
- E/F world target과 별도인 LMB equipment-use prompt/result 표시 데이터
- key token/hook lifecycle과 physical placement 연결; locker 번호 topology에는 의존하지 않음
- focus와 held key 변화의 UI용 delegate
- 외부 focus mode가 활성화된 동안 active hold, trace와 prompt를 중단하는 C++ suppression 경계
- focus target 교체와 target query 변화를 optional target 표현에 알리는 C++ focus 알림

Interaction은 cleaning progress, attack/damage/health, towel count/machine, customer routine, facility slot, queue와 player money를 소유하지 않는다.

## Interaction Contract

`IPlayerInteractable`은 다음 계약을 제공한다.

- `QueryInteraction(Context)`: 표시 여부, 실행 가능 여부, 대상명, 행동명과 실패 이유를 반환하며 상태를 바꾸지 않는다.
- `ExecuteInteraction(Context)`: 실행 직전에 조건을 다시 검증하고 성공/실패 결과를 반환한다.
- 기존 `bCanInteract`, `ActionName`, `FailureReason`은 primary 의미를 유지한다.
- target은 optional secondary 표시/가능 여부, action name과 failure reason을 추가로 반환할 수 있다.
- primary는 `Instant` 또는 `Hold` activation mode를 선언할 수 있다. 기존 target의 default는 `Instant`다.
- secondary는 현재 계약에서 Started 한 번의 instant 실행만 사용한다. 2026-09-28부터 secondary를 제공하는 대상은 없고 계약은 예약으로 유지한다.
- target은 optional held-use Apply·Take 필드와 `ExecuteHeldTargetUse(Context, Direction)`을 제공할 수 있다. 상세는 [HeldTargetUseSystem.md](HeldTargetUseSystem.md) Contract Types다.
- 기존 `ExecuteInteraction(Context)`는 primary API로 유지하고 optional secondary execute와 hold begin/update/cancel 계약을 추가한다.

`IPlayerInteractionFocusObserver`는 target이 선택적으로 구현하는 C++ 전용 표현 계약이다(`CannotImplementInterfaceInBlueprint`).

- `NotifyInteractionFocusChanged(Source, Query)`: 이 target이 source의 현재 focus이고 commit된 query가 처음 설정되거나 바뀌었을 때 호출된다.
- `NotifyInteractionFocusEnded(Source)`: 이 target이 더 이상 source의 focus가 아닐 때 한 번 호출된다.
- 알림은 표현 전용이다. observer는 query 결과를 읽기만 하고 domain mutation, interaction 실행이나 source 상태 변경을 하지 않는다. 실행 가능 여부의 정본은 여전히 `QueryInteraction`/`ExecuteInteraction`이다.

`FPlayerInteractionContext`는 interactor, `UPlayerCarryComponent`, hit actor/component와 hit 정보를 가진다. `FPlayerInteractionQuery`와 결과 문구는 localization 가능한 `FText`를 사용한다.

`FPlayerInteractionQuery`는 기존 E primary/F secondary/LMB equipment 필드를 유지하고 optional LMB placement와 Q recovery의 visibility/can-use/action/failure/progress를 추가한다. `EPlayerInteractionIntent::PlacementConfirm`, `FacilityRecovery`와 `EPhysicalCarryKind::Facility`는 기존 ordinal을 보존하도록 각 enum 끝에 추가한다.

`FPlayerInteractionQuery.bPrimaryProgressVisible`(신규, 기본 false)은 E row 진행 막대를 Hold mode가 아니어도 보이게 한다. 값은 기존 `HoldProgress`를 쓴다. Instant로 시작한 뒤 target이 소유한 시간 작업의 진행을 보여 주는 용도이며, Interaction은 이 값을 해석하거나 갱신하지 않는다. `ActiveHoldTarget`이 아닐 때 target query의 `HoldProgress`를 덮어쓰지 않는 기존 규칙을 유지한다. `Equals`에 포함한다.

## Held Equipment Use Contract

`IHeldEquipmentUsable`은 concrete type을 Character/Interaction에 결합하지 않고 다음을 제공한다.

- `QueryEquipmentUse(Context)`: 실행 가능, action name, failure, mode를 반환하고 상태를 변경하지 않음
- `BeginEquipmentUse(Context)`: 입력 시작 owner를 commit
- `UpdateEquipmentUse(Context, DeltaTime)`: Hold use와 progress 갱신
- `EndEquipmentUse(Context)`: 정상 release
- `CancelEquipmentUse(Context)`: drop, target/tool invalidation, focus mode/EndPlay cleanup

context는 user/carry, camera origin/forward과 현재 focus hit를 제공한다. held equipment가 실제 domain owner API를 호출하며 `UPlayerEquipmentUseComponent`는 concrete wrench/mop을 cast하지 않는다.

Equipment row 합성은 현재 held Actor가 `IHeldEquipmentUsable`이면 해당 query를 authoritative하게 사용하고, 대상이 채운 held-use Apply·Take 필드를 비운다. `HasUsableHeldEquipment()`는 Character owner 선택용 C++ 조회다. 사용 가능한 held equipment가 없을 때만 focus target이 `물걸레가 필요합니다` 같은 disabled equipment action/failure를 광고할 수 있다. 두 source를 두 LMB row로 동시 표시하지 않으며 focus target은 target name/hit context를 제공한다.

## Player View-Front Spawn Geometry

2026-10-01 UNBOX-SPAWN-VIEW. `Private/Interaction/PlayerViewFrontPlacement.h/.cpp`의 namespace `PlayerViewFrontPlacement`는 world·UObject에 접근하지 않는 순수 계산이다. 상자 개봉([ShopSystem.md](ShopSystem.md) World Placement)과 봉투 묶기([CleaningLitterSystem.md](CleaningLitterSystem.md) Front Drop Placement)가 공유한다. 카메라 입력은 `FHeldEquipmentUseContext::CameraOrigin/CameraDirection`이다.

- `FViewFrontBox {Center, YawDegrees, HalfExtent}`: 선 자세(yaw만) box 하나.
- `GetProjectionRange(Boxes, Origin, Axis)`: support function(`Center·Axis ± Σ e_k|Axis·local_k|`)으로 구한 정확한 투영 [Min, Max].
- `ComputeViewFrontTranslation(Boxes, CameraOrigin, ViewDirection, DistanceCm)`: 축은 V와 `FRotationMatrix(V.Rotation())`의 Y·Z다. 결과에서 V 투영 최솟값 = DistanceCm("카메라에서 시선 방향으로 잰 가장 가까운 부분"), right·up 투영 범위 중앙 = 카메라. pitch ±90에서도 정의된다.
- `GetCameraClearancePushCm(Boxes, CameraOrigin, HorizontalForward, CameraClearanceCm)`: 윗면 ≤ 카메라 Z − 여유 또는 수평 전방 투영 최솟값 ≥ 여유면 0, 아니면 투영 최솟값을 여유로 만드는 추가 거리. 카메라와 무리를 평면 하나로 분리해 감싸지 않게 한다.
- `BuildPullDistances(Start, Min, Step)`: max(Start, Min)부터 Step씩 줄이고 마지막은 Min 한 번. 비유한·Step ≤ 0이면 빈 결과.
- 조정값을 소유하지 않는다. 여유·간격·거리는 모두 호출자가 자기 원본(개봉 `UShopSettings`, 봉투 `ALitterTongsActor`)에서 읽어 인자로 넘긴다. helper 안의 수는 부동소수 판정 epsilon과 기하 정의(중앙 = 범위 합의 1/2)뿐이다.
- 단계 순서, 충돌·시야·손님 검사와 실패 계약은 호출 시스템이 소유한다. 이 helper에 world query를 넣지 않는다.

## `UPlayerInteractionComponent`

- local player camera 기준 configurable distance/channel line trace를 수행한다.
- actor 또는 hit component에서 `IPlayerInteractable`을 찾는다.
- target 또는 query 결과가 바뀔 때만 `OnInteractionQueryChanged`를 방송한다.
- 같은 commit 지점에서 target이 바뀌면 이전 target observer에 종료를 먼저 보내고 새 target observer에 변화 알림을 보낸다. 같은 target의 query만 바뀌면 변화 알림만 보낸다. query가 같으면 알림하지 않는다.
- clear, suppression, 로컬 제어 상실과 EndPlay의 focus 해제는 기존 target에 종료를 정확히 한 번 보낸다. 파괴된 target에는 알림하지 않는다. 알림 callback 중 재진입 commit은 이전 알림 순서를 뒤섞지 않는다.
- `TryInteract()`에서 대상을 다시 trace/query한 뒤 execute한다.
- 기존 `TryInteract()`는 instant primary 호환 wrapper로 유지한다.
- E Started/Completed/Canceled를 primary begin/end로 받고 active hold 동안 같은 target, focus, carry와 query 조건을 매 Tick 재검증한다.
- F Started는 secondary query/execute를 호출하고 target에 secondary가 없으면 mutation하지 않는다.
- C++ 전용 `ResolveFocusedInteraction`은 기존 `BuildInteraction`에 위임해 held-use component에 fresh focus를 제공한다. 이 component는 held-use 상태·Tick을 갖지 않는다.
- G Started는 view intent를 carry component에 전달하고 반환 결과를 동일 attempt notification으로 방송한다.
- `TryInteract()`의 대상 없음, query 실행 불가, execute 성공·실패는 모두 `FPlayerInteractionResult` 하나를 반환하고 `OnInteractionAttemptFinishedNative`를 정확히 한 번 방송한다.
- execute 뒤에는 query를 먼저 refresh한 다음 attempt result를 방송하므로 UI는 최신 지속 상태 위에 일시 실행 피드백을 표시할 수 있다.
- key, mop, basket, towel, stain, customer, cash 같은 구체 domain type을 직접 판별하지 않는다.
- focus target의 world query와 held Actor의 equipment query를 합성해 E/F/LMB row의 단일 `FPlayerInteractionQuery`를 방송한다.
- Interaction package가 소유한 supplemental intent-source interface를 통해 포커스 Actor의 recovery row를 일반 target query에 합성한다. player-global supplemental source는 placement row와 활성 recovery progress만 보충한다. Interaction은 concrete Placement component에 의존하거나 설비 mode/progress를 직접 변경하지 않는다.
- equipment-use attempt result를 `EquipmentUse` intent로 받아 기존 query/result delegate에 합성하되 domain mutation을 대행하지 않는다.
- active hold는 target/input/focus/carry/EndPlay invalidation에서 정확히 한 번 cancel한다.
- pawn 종료·교체 시 focus를 지우고 query/result delegate를 정리한다.
- `SetInteractionSuppressed(true)`는 active hold를 transient failure 없이 한 번 cancel하고 current target/query를 지우며 suppress 중 trace와 public attempt의 mutation을 막는다.
- `SetInteractionSuppressed(false)`는 즉시 query를 refresh한다. 같은 값의 반복 설정은 lifecycle을 중복 실행하지 않는다.
- suppression은 generic 외부 focus 계약이며 Computer concrete type이나 computer session 상태를 판별하지 않는다.

### Interaction Trace Debug

개발 빌드에서 `bathhouse.Debug.InteractionTrace 1`은 authoritative camera `Visibility` line trace를 매 frame 표시한다. cyan은 no-hit, green은 component 또는 Actor가 `IPlayerInteractable`인 첫 Hit, red는 상호작용 불가능한 첫 blocker이며 yellow point와 문자열은 실제 impact, Actor, Component와 거리를 나타낸다. 이 기능은 기존 single-hit 선택, query, execute와 collision response를 변경하지 않는다. `0`으로 끄며 shipping debug draw에는 포함되지 않는다.

PlacementZone의 `ZoneBounds`는 placement 전용 Trace Channel만 차단하고 `Visibility`를 무시한다. placement surface 탐색을 위해 generic interaction single-hit 규칙에 예외나 blocker-skip을 추가하지 않는다.

## `UPlayerCarryComponent`

- generic held `AActor` 하나와 호환 key getter/delegate를 authoritative하게 소유한다.
- 빈손 pickup만 허용하며 inventory array, hotbar, item swap을 만들지 않는다.
- E fixed-slot take/store와 G free drop을 concrete item cast 없이 public interface로 조율한다.
- active equipment use를 placement 전에 한 번 cancel하고 성공 후에만 held reference를 해제한다.
- attach, slot, collision, CCD 또는 physics 실패 시 snapshot 전체를 rollback한다.
- `HeldKeyAnchor`와 기존 key commit API를 rename/delete하지 않는다.
- detailed state, release physics, key extension과 recovery는 [PhysicalCarrySystem.md](PhysicalCarrySystem.md)를 따른다.

Cash는 carry 대상이 아니며 Economy System의 즉시 획득 interaction으로 처리한다.

## Generic Carryable Contract

`IPhysicalCarryable`은 concrete domain type을 Interaction에 결합하지 않는 native 계약이다. 기본 `FreeDrop|FixedSlot` capability, exact-slot binding, actual-held-pose release, free-world CCD, key state 확장과 reflected compatibility는 [PhysicalCarrySystem.md](PhysicalCarrySystem.md)를 따른다.

`GetHeldTransform()`의 기본은 Identity이며 location/rotation만 사용하고 scale은 `(1,1,1)`로 유지한다. 새 공통 carry Actor 또는 `UPhysicalCarryableComponent`를 만들지 않는다.

`UHeldEquipmentMotionComponent`는 carryable 공통화가 아닌 표현 도우미다. current relative transform을 baseline으로 저장하고 one-shot/hold-loop curve offset을 적용하며 end/cancel/drop에서 baseline을 복구한다. carry reference, pickup/drop transaction와 domain progress를 소유하지 않는다.

## Key And Fixed-Slot Target

`ABathhouseKeyActor`의 number/customer/counter transaction과 exact `ABathhouseKeyHookActor` binding은 유지한다. key-hook validation은 같은 key instance/number만 확인하고 shoe/clothes locker 존재를 검사하지 않는다. 상세 계약은 [PhysicalCarrySystem.md](PhysicalCarrySystem.md)를 따른다.

## Character Integration

`AFirstPersonCharacter`는 composition root로 다음을 추가한다.

- `UPlayerInteractionComponent`
- `UPlayerCarryComponent`
- `UPlayerEquipmentUseComponent`
- `UPlayerHeldTargetUseComponent`
- target `UPlayerFacilityPlacementComponent`
- first-person camera 하위 `HeldKeyAnchor`
- E Started/Completed/Canceled, F/G Started, Q Started/Completed/Canceled, LCtrl Started/Completed, MouseWheel axis와 LMB lifecycle을 Interaction/Carry/Placement/Equipment에 의도로 전달한다. Q hold elapsed와 자동 commit은 입력 반복 이벤트가 아니라 Placement Component Tick이 소유한다.
- LMB owner는 `Computer > Placement > Equipment(장비를 든 경우) > HeldTargetUse` 순서로 하나만 선택한다. RMB(`SecondaryUseAction`)는 장비 보조 사용이 있으면 Equipment secondary, 장비가 없으면 HeldTargetUse Take를 쓴다. 상세 순서는 [HeldTargetUseSystem.md](HeldTargetUseSystem.md) Input Ownership이다.
- computer session이 input을 capture하면 해당 session이 E lifecycle을 소비하고 Interaction에는 전달하지 않는다.

Character는 focus 규칙과 key transaction을 직접 구현하지 않는다. PlayerController는 mapping context 등록·해제 책임을 유지한다.

## Blueprint/API Contracts

Blueprint 조회·표현 API:

- `UPlayerInteractionComponent::GetCurrentInteractionQuery`
- `UPlayerInteractionComponent::TryInteract`
- primary begin/end, secondary attempt와 equipment drop attempt API
- equipment-use begin/update/end/cancel API와 C++ result integration
- `UPlayerInteractionComponent::OnInteractionQueryChanged`는 Blueprint 표시 갱신 계약이다.
- `UPlayerInteractionComponent::OnInteractionAttemptFinishedNative`는 C++ 전용 실행 결과 계약이며 BlueprintAssignable로 노출하지 않는다.
- `UPlayerInteractionComponent::SetInteractionSuppressed`, `IsInteractionSuppressed`는 외부 focus owner가 사용하는 C++ 전용 계약이며 Blueprint에 노출하지 않는다.
- `IPlayerInteractionFocusObserver`는 target 표현용 C++ 전용 계약이며 Blueprint에 노출하지 않는다.
- `IPhysicalCarryDiscardable`은 쓰레기통(held)과 수거 구역(world) 판정용 C++ 전용 선택 계약이다([PhysicalCarrySystem.md](PhysicalCarrySystem.md)).
- `IHeldEquipmentSecondaryUsable`은 장비의 RMB 보조 사용(press당 한 번, 조준 무관) C++ 전용 계약이다. `UPlayerEquipmentUseComponent`가 RMB 행 query와 실행을 맡는다(2026-10-01, [CleaningLitterSystem.md](CleaningLitterSystem.md) Input Routing)
- `UPlayerCarryComponent::IsHandEmpty`
- `UPlayerCarryComponent::GetHeldKey`
- generic held object와 held kind 조회, `OnHeldObjectChanged`
- exact fixed-slot take/store와 actual-held-pose free-drop result
- combined equipment-use query/result의 optional LMB action/failure/mode/progress
- held-use Apply·Take의 visible/can/action/failure/mode 필드와 `HeldApply`·`HeldTake` result intent. Take 필드는 대상 → 물건 방향 신호이며 target focus observer가 읽는다
- 장비 보조 사용 RMB 행 `bEquipmentSecondaryVisible`, `bCanEquipmentSecondary`, `EquipmentSecondaryActionName`, `EquipmentSecondaryFailureReason`(BlueprintReadOnly, `Equals` 포함)와 `EquipmentSecondaryUse` result intent(2026-10-01)
- `HeldObjectSummary`: 들고 있는 carryable의 `GetHeldSummaryText()`. `UPlayerEquipmentUseComponent::MergeEquipmentQuery`가 채운다(2026-09-30, [ServiceSystem.md](ServiceSystem.md) HUD)
- `HeldUseTargetKey`(int32, 기본 `INDEX_NONE`): 같은 target 안의 하위 대상 key. held-use 반복은 key가 바뀌면 멈춘다(2026-09-30, [ServiceFacilityDisplaySystem.md](ServiceFacilityDisplaySystem.md) Held-Use Extension)
- `PresentationRevision`(int64, 기본 0, `UPROPERTY()` Blueprint 비노출, `Equals` 포함): target 표현 상태의 revision. HUD 문구가 같아도 target 표현이 바뀌면 query를 달라지게 해 focus observer가 다시 알림을 받게 한다. 동작 판정·held-use 반복 조건에는 쓰지 않는다(2026-09-30, [TowelSystem.md](TowelSystem.md) Service Unit 2 Display Changes cue 재계산 경로)
- combined placement LMB action/failure와 recovery Q action/failure/hold progress
- `ABathhouseKeyActor::OnKeyStateChanged`
- `ABathhouseKeyActor::OnHeldPresentationChanged`
- `APhysicalCarryFixedSlotActor::OnSlotOccupancyChanged`

Editor authoring 값:

- `AFirstPersonCharacter::InteractAction`
- `AFirstPersonCharacter::SecondaryInteractAction`
- `AFirstPersonCharacter::DropCarryAction`
- `AFirstPersonCharacter::PrimaryUseAction`
- `AFirstPersonCharacter::SecondaryUseAction`(RMB)
- `UPlayerHeldTargetUseComponent::RepeatIntervalSeconds`(기본 0.15)
- `AFirstPersonCharacter::RecoverFacilityAction`
- `AFirstPersonCharacter::PlacementSnapAction`
- `AFirstPersonCharacter::PlacementRotateAction`
- trace 거리와 collision channel
- `HeldKeyAnchor` transform
- key, wet mop, towel basket, monkey wrench Blueprint class default의 개별 `HeldTransform`
- item별 약한 forward/upward velocity change, 기본 `120/15 cm/s`
- deprecated `ThrowSpawnDistance`, `DropSweepChannel`, `DropSweepClearance`는 호환용으로만 보존
- equipment fixed slot의 exact `AssignedItem`, `bStartOccupied`, `ItemAnchor`
- key `KeyPhysicsRoot` bounds와 collision
- key actor mesh/number presentation
- key hook의 번호와 정확한 key actor 연결; locker reference 없음

## Dependencies

- Interaction -> Engine actor/component/collision
- Interaction -> Facility의 generic key-hook/facility query
- Placement -> Interaction의 supplemental intent-source/query-result 계약
- Cleaning -> Interaction public query/equipment-use/motion/carry 계약
- Combat -> Interaction public carry/equipment-use/motion 계약
- Towel -> Interaction public intent/carry/held-use target 계약
- Shop -> Interaction public interactable/carry/equipment-use/discardable 계약
- Shop, Cleaning -> Interaction private `PlayerViewFrontPlacement` 순수 기하(UNBOX-SPAWN-VIEW)
- Utility -> Interaction public interactable/carry/focus-observer/held-use target 계약
- Character -> Interaction
- Computer -> Interaction public query/carry/suppression 계약
- UI -> Interaction
- Interaction은 Computer concrete class에 의존하지 않는다.
- Interaction은 Combat/Cleaning/Customer/UI concrete class에 의존하지 않는다.

## Manual Review Points

- 어떤 경로에서도 player가 두 key를 동시에 들지 않는지 확인한다.
- 어떤 경로에서도 key/mop/basket/monkey wrench/전용 facility item을 둘 이상 동시에 들지 않는지 확인한다.
- E hold cancel과 F/G attempt가 기존 primary result를 중복 방송하지 않는지 확인한다.
- key의 기존 state transition과 GetHeldKey/OnHeldKeyChanged 계약이 generic carry 확장 뒤에도 유지되는지 확인한다.
- query가 상태를 바꾸지 않고 execute가 조건을 재검증하는지 확인한다.
- focus observer 알림이 query 변화 때만 발생하고 target 교체·clear·suppress·EndPlay에서 종료가 한 번만 전달되는지 확인한다.
- 같은 query에서 UI delegate가 매 Tick 반복되지 않는지 확인한다.
- 한 번의 `TryInteract()`에서 result delegate가 중복 방송되지 않고 반환값과 동일한 성공·실패 이유를 전달하는지 확인한다.
- dropped key가 복제·소실되거나 자신의 exact hook 이외에 반환되지 않는지 확인한다.
- player/customer 비정상 종료 시 key가 원래 hook으로 복구되는지 확인한다.
- key number가 HUD text가 아니라 first-person 3D key에 표시되는지 확인한다.
- 모든 carryable의 slot/store/free-drop이 같은 carry component commit 경로를 사용하고 concrete actor가 transaction을 복제하지 않는지 확인한다.
- Identity `HeldTransform`이 기존 anchor 부착을 보존하고 item별 location/rotation이 player-held 상태에만 적용되는지 확인한다.
- held transform이 hook/counter/world drop transform과 physical bounds scale을 오염시키지 않는지 확인한다.
- suppression 시작이 hold를 한 번만 cancel하고 query/prompt를 지우며, 해제 직후 최신 target을 다시 조회하는지 확인한다.
- LMB use press owner가 Computer/Equipment 사이에서 섞이지 않고 active use가 release/cancel/drop/EndPlay에 한 번만 종료되는지 확인한다.
- equipment query/result가 E/F row를 덮어쓰지 않고 empty hand/invalid target의 정확한 실패 이유를 제공하는지 확인한다.
- suppress 중 E/F/G 직접 호출도 domain mutation이나 stale attempt feedback을 만들지 않는지 확인한다.
- placement가 active면 LMB equipment use가 시작되지 않고 Computer focus가 둘 모두보다 우선하는지 확인한다.
- Q recovery hold가 release/gaze/조건 변경에서 한 번만 cancel되고 Interaction은 progress를 복제하지 않는지 확인한다.
- free drop이 camera target으로 teleport하지 않고 actual held pose에서 시작하는지 확인한다.
- held pose world overlap 실패가 attachment, carrier와 presentation을 보존하는지 확인한다.
- free-world item이 Pawn을 영구 무시하고 질량과 무관한 약한 velocity change 및 CCD를 받는지 확인한다.
