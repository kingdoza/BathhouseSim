# 구현 프롬프트 — 급수·배수 시스템과 욕탕 물 수직 구현

## 입력과 현재 단계

- 승인 기능 계약: `.md/PROMPT_ARCHITECTURE.md`의 `BH-WATER-001`~`BH-WATER-029`
- 기능 답변: `.md/QNA_FEATURE_SPEC.md` Q1~Q27
- Unreal 사전 조사: `.md/REPORT_UNREAL_DISCOVERY.md`
- 기술 정본: `.md/Architecture/BathWaterSystem.md`
- 연계 정본: `.md/Architecture/FacilitySystem.md`, `CustomerSystem.md`, `PlacementSystem.md`, `InteractionSystem.md`, `CustomerRecoverySystem.md`, `CoreSystem.md`
- 현재 단계는 `/Game/Bathhouse/Blueprints/Facility/BP_Bath` 첫 욕탕 타입 하나의 C++ 수직 구현이다.
- 이번 단계는 Source, 필요한 runtime module dependency와 native automation만 변경한다.
- Content, Level, StateTree asset, Project Settings 저장값과 `.md/Unreal/*`은 수정하지 않는다.

사용자가 기능 명세 이후 확정한 다음 사항도 승인 입력으로 포함한다.

- 입욕 임계 수위는 모든 욕탕이 공유하되 Project Settings에서 조정 가능하고 기본값은 `80%`다.
- 급수/배수 속도는 각각 `6.666667%/s`, `10%/s`를 native 기본값으로 하며 욕탕 Blueprint 기본값과 Level instance override를 허용한다.
- 밸브·레버 interaction은 별도 collision component 없이 각각의 실제 Static Mesh collision만 사용한다.
- local rotation axis는 음수 벡터를 포함하며 정규화 시 부호를 보존한다.
- 수면은 cube/volume이 아닌 평면 Static Mesh이고 normalized amount를 alpha로 Empty/Full 위치 사이에서 선형 보간한다.
- 첫 욕탕 수면 기준 asset은 `SM_Bath_01_Water`지만 C++에서 asset 경로를 hard reference하지 않는다.
- `ST_CustomerRoutine` BathLoop는 버그 분석을 위한 공통 native category와 phase/reason/correlation 로그를 제공한다.

## 목표

1. 기존 `UBathWaterStateComponent`를 욕탕 물 양, 두 control 논리 상태, 유량과 recovery freeze의 authoritative owner로 확장한다.
2. 실제 Static Mesh collision로 E interaction하고 local axis 기준으로 회전하는 범용 `UBathWaterControlComponent`를 구현한다.
3. 범용 Facility Actor를 비대하게 만들지 않고 새 `ABathhouseBathFacilityActor`에서 물 control, 평면 수면과 Niagara 표현을 조립한다.
4. 전역 입욕 threshold를 `UBathWaterSettings` 하나에 노출한다.
5. Facility reservation, Customer search/move/entry/dwell과 현재 사용자 전부에 동일한 수위 유효성을 적용한다.
6. 전체 60초 timer, 각 10초 search window와 실제 입욕 누적시간을 분리한다.
7. Q Hold 동안 물·control 표현을 동결하고 모든 cancel/failure 경로에서 정확히 복원한다.
8. 기존 placement, facility slot, physical carry, interaction prompt와 knockdown soft-interruption 계약을 보존한다.

## 비목표와 변경 금지

- Content/Blueprint/StateTree/DataAsset/Level/Material/Niagara asset 저장 또는 resave
- 실제 밸브·레버 mesh 생성·분리와 pivot 편집
- `NS_HoneyBeam`을 물줄기로 자동 선택
- 물 온도, 오염, 수질, 넘침, 누수, 증발과 fluid simulation
- 수위 숫자 HUD와 신규 입력
- 손님 수에 따른 물 감소와 만족도 변경
- gameplay SaveGame persistence와 replication
- Bath 이외 시설로 물 시스템 일반화
- generic interaction trace, E/F/G/Q/LMB key와 우선순위 변경
- 기존 Facility slot transform/snap, queue, key, towel와 placement 후보 계산 변경
- 물 표현이나 StateTree Blueprint graph에서 domain state mutation
- 수면 scale/WPO/volume 표현 또는 별도 water collision
- control용 Box/proxy collision과 actor bounds fallback

## Module과 파일 경계

`BathhouseSim.Build.cs`에 native `UNiagaraComponent` 사용을 위한 `Niagara` runtime dependency만 추가한다. 기존 `DeveloperSettings`, StateTree, GameplayStateTree와 Engine dependency를 재사용한다. `.uproject` plugin, Config와 신규 module을 추가하지 않는다.

신규 파일:

```text
Source/BathhouseSim/Public/Facility/BathWaterSettings.h
Source/BathhouseSim/Private/Facility/BathWaterSettings.cpp
Source/BathhouseSim/Public/Facility/BathWaterControlComponent.h
Source/BathhouseSim/Private/Facility/BathWaterControlComponent.cpp
Source/BathhouseSim/Public/Facility/BathhouseBathFacilityActor.h
Source/BathhouseSim/Private/Facility/BathhouseBathFacilityActor.cpp
Source/BathhouseSim/Public/Customer/CustomerBathLoopLog.h
Source/BathhouseSim/Private/Customer/CustomerBathLoopLog.cpp
Source/BathhouseSim/Private/Customer/CustomerSessionBath.cpp
Source/BathhouseSim/Private/Tests/BathWaterAutomationTests.cpp
```

기존 변경 후보:

```text
Source/BathhouseSim/BathhouseSim.Build.cs
Source/BathhouseSim/Public|Private/Facility/BathWaterStateComponent.*
Source/BathhouseSim/Public|Private/Facility/BathhouseFacilityActor.*
Source/BathhouseSim/Public|Private/Facility/BathhouseFacilitySubsystem.*
Source/BathhouseSim/Public/Placement/PlaceableFacility.h
Source/BathhouseSim/Public|Private/Placement/PlayerFacilityPlacementComponent.*
Source/BathhouseSim/Public|Private/Customer/CustomerRoutineDefinition.*
Source/BathhouseSim/Public|Private/Customer/CustomerSessionComponent.*
Source/BathhouseSim/Public|Private/Customer/BathhouseCustomerTypes.*
Source/BathhouseSim/Public|Private/Customer/StateTree/CustomerStateTreeTasks.*
Source/BathhouseSim/Public|Private/Customer/StateTree/CustomerStateTreeConditions.*
Source/BathhouseSim/Private/Tests/BathhouseDomainTests.cpp
```

Bath 관련 session 구현은 이미 큰 `CustomerSessionComponent.cpp`에 누적하지 않고 같은 class의 `CustomerSessionBath.cpp`로 분리한다. Bath 진단 helper/category도 montage 로그와 분리한다. 단순 파일 분리를 위해 새 UObject owner를 만들지 않는다.

## `UBathWaterSettings`

`UDeveloperSettings`, `Config=Game`, `DefaultConfig`, DisplayName `Bath Water`로 구현한다. Project Settings의 Game/Bath Water section에 다음 값 하나만 둔다.

```cpp
UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Customer Use",
    meta=(ClampMin="0.0", ClampMax="100.0"))
float CustomerUsableThresholdPercent = 80.0f;
```

`GetCategoryName()`은 `Game`, `GetSectionName()`은 `Bath Water`를 반환해 노출 경로를 구현 기본값에 맡기지 않는다.

요구사항:

- 모든 Bath가 `GetDefault<UBathWaterSettings>()`의 동일 값을 읽는다.
- finite가 아니면 default 80으로 방어하고 getter 결과를 `0~100`으로 clamp한다.
- normalized getter는 percent에 `0.01`을 곱한다.
- 욕탕 Actor/component에 threshold 복사본을 만들지 않는다.
- runtime 변경·delegate hot reload 기능은 추가하지 않는다.

## Water Type과 reflected compatibility

필요한 enum은 기존 ordinal을 보존하며 신규 항목은 끝에 추가한다.

```text
EBathWaterState: Empty, Filling, Filled, Draining, Holding(new last)
EBathWaterControlType: FillValve, DrainLever
EBathWaterControlChangeReason: PlayerInteraction, FullAutoClose, RecoveryCommit, Reset
```

control change reason은 로그/표현용이고 gameplay 결과를 별도 owner에 복제하지 않는다.

기존 reflected 이름과 signature를 rename/delete하지 않는다.

- `BathWaterState` default subobject
- `WaterState`, `NormalizedAmount`
- `GetWaterState`, `GetNormalizedAmount`, `IsEmpty`
- `SetWaterState`, `SetNormalizedAmount`
- `OnWaterStateChanged`

`WaterState`, `NormalizedAmount`는 runtime-owned `VisibleInstanceOnly, BlueprintReadOnly, Transient`로 전환한다. 기존 두 public setter는 `DeprecatedFunction` metadata를 붙여 한 migration cycle 보존하되 canonical runtime과 신규 tests에서는 사용하지 않는다. Core Redirect는 추가하지 않는다.

## `UBathWaterStateComponent`

Authoring 값:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bath Water|Flow",
    meta=(ClampMin="0.0"))
float FillRatePercentPerSecond = 6.666667f;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bath Water|Flow",
    meta=(ClampMin="0.0"))
float DrainRatePercentPerSecond = 10.0f;
```

두 값은 Class Default와 instance override를 허용한다. runtime getter는 finite/non-negative를 방어하며 invalid authoring은 `IsDataValid()` 실패다.

State owner가 제공할 최소 query:

```text
GetNormalizedAmount()
GetWaterPercent()
GetWaterState()
IsEmpty()
IsCustomerUsable()
IsControlOpen(ControlType)
IsRecoveryFrozen()
CanSetControlOpen(ControlType, bOpen, OutFailure)
```

Mutation API는 native only로 유지한다.

```text
RequestSetControlOpen(ControlType, bOpen, Reason, OutFailure)
ResetEmptyForPlacement()
BeginRecoveryFreeze(OutFailure)
CancelRecoveryFreeze()
PrepareRecoveryCommit()
```

필요 delegate:

- amount previous/current normalized
- derived water state previous/current
- customer usability changed
- control type/open state/change reason
- recovery freeze changed

Blueprint는 delegate를 읽어 표현할 수 있지만 mutation API를 호출하지 않는다.

### 물 계산

Actor Tick을 사용하지 않고 state component Tick만 필요할 때 활성화한다.

```text
NetPercentPerSecond
  = (FillOpen ? FillRatePercentPerSecond : 0)
  - (DrainOpen ? DrainRatePercentPerSecond : 0)

NewNormalized
  = Clamp(CurrentNormalized
          + NetPercentPerSecond * 0.01 * DeltaSeconds,
          0, 1)
```

- endpoint에 도달하면 exact `0.0f`/`1.0f`로 snap한다.
- `IsEmpty()`는 derived enum이 아니라 exact normalized endpoint를 사용한다.
- 순유량 양수/음수는 `Filling/Draining`, 중간에서 0이면 `Holding`, endpoint는 `Empty/Filled`다.
- 실제 amount가 변하지 않으면 amount delegate를 발행하지 않는다.
- global threshold의 이전/현재 usable 결과가 다를 때만 usability delegate를 한 번 발행한다.
- 두 control이 닫히거나 endpoint에서 현재 open 조합으로 변할 수 없으면 Tick을 끈다. control state change가 다시 평가해 Tick을 켠다.
- drain은 0%에서 open을 유지할 수 있다.
- fill이 100%에 도달하면 같은 update에서 논리적으로 닫고 `FullAutoClose` control event를 보낸다.
- fill과 drain이 동시에 열리면 한 번의 net delta만 commit한다.

### 초기화와 freeze snapshot

`ResetEmptyForPlacement()`는 amount 0, control 둘 closed, derived Empty와 cached usability를 한 transaction으로 만든다. stale serialized state를 읽지 않는다.

recovery snapshot은 amount, 두 logical open 상태, derived/cached usability와 freeze 이전 Tick 상태를 가진다. control visual motion과 Niagara snapshot은 Bath Actor가 같은 hold lifecycle에서 보완한다.

- begin은 snapshot 하나만 허용하고 water Tick을 멈춘다.
- cancel은 exact state를 복원하고 필요한 Tick/delegate/publication을 복구한다.
- prepare commit은 control을 closed로 만들되 source Actor destroy가 확정될 때까지 snapshot을 보존한다.
- conversion rollback은 snapshot을 복원하고 source Actor destroy 성공은 EndPlay에서 snapshot을 폐기한다.
- repeated begin/end는 fail 또는 no-op로 명확히 처리하며 snapshot을 덮어쓰지 않는다.
- EndPlay는 callback 없이 snapshot을 버린다.

## `UBathWaterControlComponent`

다음 상속을 사용한다.

```cpp
UCLASS(ClassGroup=(Bathhouse), meta=(BlueprintSpawnableComponent))
class UBathWaterControlComponent
    : public UStaticMeshComponent
    , public IPlayerInteractable
```

별도 Box/Shape collision을 만들지 않는다. `FPlayerInteractionContext.HitComponent` 자체가 이 component이고 기존 `UPlayerInteractionComponent::BuildInteraction()`의 component-first 경로를 그대로 사용한다.

Constructor default:

- query collision 활성
- `Visibility=Block`
- overlap/physics 불필요
- `CanEverAffectNavigation=false`
- Tick capability는 true지만 시작 disabled이며 회전 중에만 enabled

Blueprint/instance가 collision profile을 바꿀 수 있으므로 runtime과 Data Validation에서 Static Mesh 존재, 실제 query collision과 Visibility Block을 확인한다. 기존 Player Interaction은 complex trace를 허용하므로 asset triangle collision을 그대로 사용할 수 있고 Simple Collision은 선택적인 성능 최적화다. usable collision geometry가 없으면 query를 숨기거나 실행 실패시키고 임의 proxy collision을 생성하지 않는다.

Authoring property:

```text
ControlType              Visible/read-only; native Actor가 고정
LocalRotationAxis        EditAnywhere, BlueprintReadOnly
OpenAngleDegrees         EditAnywhere, BlueprintReadOnly
RotationDurationSeconds  EditAnywhere, BlueprintReadOnly, > 0
```

component Relative Transform이 닫힌 authored pose이며 BeginPlay/PostInitialize 시 closed quaternion을 저장한다. Static Mesh pivot이 실제 회전 중심이다.

축 처리:

- local vector를 `GetSafeNormal()`로 정규화하되 음수 부호를 보존한다.
- `(1,0,0), +90`과 `(-1,0,0), +90`은 반대 회전이다.
- `(-1,0,0), +90`과 `(1,0,0), -90`은 같은 회전이다.
- zero/non-finite vector는 validation/runtime failure다.
- signed angle은 제한하지 않되 finite여야 한다.
- open target은 authored closed basis에 local-axis delta quaternion을 합성한다.

회전 runtime:

- logical toggle 승인 즉시 open state는 state component에서 바뀐다.
- visual은 current quaternion을 start, open/closed를 target으로 저장하고 fixed ease-in/out alpha + quaternion slerp로 이동한다.
- `RotationDurationSeconds`는 full traversal duration이다.
- full auto-close가 opening 중 발생하면 current quaternion에서 closed로 즉시 반전하고 남은 angle/full angle 비율로 duration을 줄인다.
- player가 회전 중 같은 component에 E를 누르면 `작동 중입니다`로 거부한다.
- 다른 control component는 독립적으로 사용할 수 있다.
- recovery freeze는 current quaternion, motion direction, elapsed/remaining duration과 tick state를 snapshot하고 정지한다. cancel은 그 위치와 남은 시간부터 복원한다.

Interaction query/action:

- owner가 `ABathhouseBathFacilityActor`이고 placed domain active일 때만 표시
- current closed/open에 따라 열기/닫기 ActionName 구분
- 100%의 closed fill은 `이미 물이 가득 차 있습니다`
- 0%의 closed drain은 정상 open 허용
- motion 중 `작동 중입니다`
- recovery freeze 중 `설비 회수 중입니다`
- carry 상태와 customer reservation/use는 거부 조건이 아님
- execute에서 같은 조건을 재검증한 뒤 owner/state API에 intent 전달

## `ABathhouseBathFacilityActor`

`ABathhouseFacilityActor`의 파생 native class로 구현한다. 범용 base에 신규 water visual/control component를 넣지 않는다. 기존 base가 생성하는 stable `BathWaterState`를 그대로 사용한다.

Native default subobject:

```text
SceneRoot
├─ FillValveControl        UBathWaterControlComponent
├─ DrainLeverControl       UBathWaterControlComponent
├─ FillFlowNiagara         UNiagaraComponent
└─ WaterPresentationRoot   USceneComponent
   ├─ WaterSurfaceMover    USceneComponent
   │  └─ WaterSurfaceMesh  UStaticMeshComponent
   ├─ WaterLevelEmptyPoint USceneComponent
   └─ WaterLevelFullPoint  USceneComponent
```

고정값:

- FillValveControl kind = FillValve
- DrainLeverControl kind = DrainLever
- WaterSurfaceMesh collision/overlap/physics/navigation off
- FillFlowNiagara AutoActivate false
- Actor Tick off

`PostInitializeComponents()` 또는 registration 이전 lifecycle에서 delegate를 idempotent하게 연결하고 empty/closed/hidden 초기 상태를 만든다. staged import와 선배치 BeginPlay 순서 모두 기존 Facility registration이 stale water usability를 publish하지 않게 한다. duplicate initialization은 no-op다.

수면 표현:

```text
Alpha = Clamp(BathWaterState.NormalizedAmount, 0, 1)
Mover.RelativeLocation = Lerp(EmptyPoint.RelativeLocation,
                              FullPoint.RelativeLocation,
                              Alpha)
WaterSurfaceMesh.Visible = Alpha > 0
```

- 세 SceneComponent는 같은 `WaterPresentationRoot` local space여야 한다.
- runtime은 mover location만 변경하고 mesh local rotation/scale/material은 건드리지 않는다.
- 0에서도 mover는 EmptyPoint에 두되 mesh만 숨긴다.
- future water asset도 평면 SM 계약이며 cube/volume 검사나 scale 표현을 만들지 않는다.
- C++은 `/Game/Bathhouse/Meshes/Bath/Bath_01/StaticMeshes/SM_Bath_01_Water`를 load하지 않는다. Editor가 WaterSurfaceMesh에 연결한다.

Niagara 표현:

- fill logical open이고 recovery freeze가 아니며 auto-close 전일 때만 active
- player close/full auto-close/recovery begin에서 즉시 deactivate
- recovery cancel에서 snapshot이 fill open이면 다시 activate
- drain state는 직접 Niagara를 바꾸지 않음

BlueprintImplementableEvent는 amount/usability/fill-flow 결과를 notification할 수 있지만 C++ 기본 표현을 대체하지 않는다.

Actor는 `IsAvailableForReservation()`을 override해 enabled/placed-domain, global threshold와 recovery freeze를 모두 확인한다. usability crossing마다 Facility Subsystem에 Bath availability를 정확히 한 번 publish한다.

`IsDataValid()`은 component 존재/종류, control axis/angle/duration/query collision, shared presentation parent, finite marker 위치, Empty/Full 구분, water plane collision/Nav off와 Niagara auto-activate off를 검사한다. mesh/Niagara asset 자체는 Source CDO에서 비어 있을 수 있지만 concrete `BP_Bath` Editor 통합에서는 필수다.

## Facility Reservation 변경

`ABathhouseFacilityActor`에 side-effect-free virtual `IsAvailableForReservation() const`를 추가한다. base는 기존 enabled/operational 조건만 반환하고 Bath subclass는 water 조건을 추가한다.

`UBathhouseFacilitySubsystem::TryReserveRandomSlot()`은 다음 두 시점에 query한다.

1. weighted candidate 수집
2. `TryReserve()` 직전

Bath의 last-used exclusion/fallback 양쪽이 같은 filter를 사용한다. `GetFacilitiesOfType()`처럼 topology 조회용 API 의미를 불필요하게 변경하지 않는다.

water threshold crossing은 existing `NotifyFacilityAvailabilityChanged(Bath)`를 사용한다. 새 Subsystem과 반복 world scan을 만들지 않는다.

## Placement Recovery Hold 확장

`IPlaceableFacility`에 default no-op인 native begin/cancel hook을 추가한다.

```text
TryBeginFacilityRecoveryHold(FText& OutFailureReason) -> bool
CancelFacilityRecoveryHold()
```

`UPlayerFacilityPlacementComponent` lifecycle:

1. `BeginRecoveryHold()`의 side-effect-free query 성공
2. target hook begin 성공
3. target/elapsed를 commit하고 Tick 시작
4. 기존처럼 매 Tick trace와 recovery query 재검증
5. release/gaze/condition/suppression에서 cancel exactly once
6. full hold에서 conversion transaction 실행
7. Bath domain-unregistration stage가 closed/flow-off commit-pending을 적용하되 snapshot 유지
8. source destroy 성공이면 EndPlay에서 snapshot 폐기
9. conversion 실패는 Bath rollback이 domain/collision과 snapshot을 복원하고 Player의 후속 cancel은 no-op

Bath begin은 모든 slot Available/normalized 0을 다시 확인하고 water, control motion, Niagara를 한 snapshot으로 묶어 동결한다. hold 동안 `IsAvailableForReservation=false`로 새 reservation을 막는다. cancel은 원래 water/control/flow/tick과 availability를 복원한다. Bath의 `StagePlacedDomainUnregistration`/rollback override가 success-pending/복원을 기존 Actor conversion과 원자적으로 연결하고 water state는 payload에 넣지 않는다.

generic placement transform, item spawn, collision/domain rollback과 publication 순서는 변경하지 않는다. hold hook 실패는 item/Actor mutation 전에 사용자 실패 결과를 반환한다.

## Customer Routine Data

`UCustomerRoutineDefinition`에 다음 값을 추가한다.

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Bath",
    meta=(ClampMin="0.1"))
float BathSearchTimeoutSeconds = 10.0f;
```

기존 Source default `BathStayDurationSeconds=60`, `BathDwellMinSeconds=10`, `BathDwellMaxSeconds=20`을 유지한다. Data Validation은 stay/search/dwell 값의 finite/positive와 min<=max를 확인한다.

Editor 단계에서 현재 `DA_CustomerRoutine_Default.BathStayDurationSeconds=10`을 60으로 migration해야 하지만 이번 C++ 단계에서 asset을 수정하지 않는다.

## `UCustomerSessionComponent` Bath 확장

새 상태:

- bath search active/expired, end time, timer handle, paused remaining과 serial/iteration
- actual bath accumulated seconds
- active actual segment start world time와 guard
- current Bath usability delegate handle
- forced-exit pending/reason

최소 API:

```text
BeginBathSearchWindow()
CompleteBathSearchWindow()
CancelBathSearchWindow()
IsBathSearchExpired()
GetRemainingBathSearchSeconds()
GetActualBathSeconds()
IsCurrentBathUsable()
IsCurrentBathExitPending()
BeginActualBathSegment()
EndActualBathSegment(Reason)
```

Search window:

- 새 Bath가 필요한 logical 구간마다 한 번 시작
- 이미 active면 retry/StateTree reselect가 호출해도 reset하지 않음
- duration은 `min(BathSearchTimeoutSeconds, GetRemainingBathStaySeconds())`
- reservation 성공 시 complete
- timeout은 expired flag를 먼저 commit하고 `Customer.Event.BathSearchExpired`를 한 번 전송
- 새 invalidation으로 다음 search가 필요하면 이전 window를 끝낸 뒤 새 serial로 시작
- pause/resume timer 목록에 포함

Actual segment:

- valid Bath action snap + `BeginUseCurrentFacility()` 이후에만 시작
- world-time timestamp 차이를 accumulated 값에 한 번 합산
- threshold 하락, dwell/전체 stay 종료, knockdown suspend, StateTree abort, technical abort와 EndPlay에서 즉시/idempotent 종료
- movement/search/entry/exit/reservation-only와 paused knockdown 구간 제외
- 여러 Bath의 segment를 합산
- HUD/satisfaction/SaveGame에 연결하지 않음

Current Bath delegate:

- reservation 성공 시 해당 `UBathWaterStateComponent` usability delegate bind
- release/switch/cleanup/EndPlay에서 exact handle unbind
- moving/reserved 상태에서 false면 actual segment 없이 invalid event
- occupied/BathDwell에서 false면 segment를 즉시 중지하고 exit-pending commit 후 event
- same invalid state 반복 broadcast/event 방지
- approach 복귀/release 완료 전 slot은 유지
- threshold 상승은 exit-pending을 해제하지 않음

`SnapCurrentFacility(ActionPoint)`와 `BeginUseCurrentFacility()`는 Bath일 때 바로 직전 usability를 재검증한다. knockdown resume도 같은 query를 사용하며 invalid면 기존 slot을 다른 customer에게 노출하기 전에 exit/cleanup 경로로 넘긴다.

전체 BathStay expiry handler는 actual segment와 search window를 먼저 중지하고 기존 event를 보낸다. `PauseRoutineTimers/ResumeRoutineTimers`는 BathStay와 search timer를 함께 다루고 active actual segment는 pause에서 합산 종료, valid resume 후 재시작한다.

## StateTree와 BathLoop

`BathhouseCustomerTypes`에 native gameplay tag를 추가한다.

```text
Customer.Event.BathSearchExpired
Customer.Event.BathBecameUnusable
```

기존 `Customer.Event.BathStayExpired`는 유지한다. 기존 generic task 이름을 불필요하게 rename/delete하지 않는다.

Native Task 변경:

- Bath `FCustomerFacilityTask` 진입은 search window를 idempotent하게 시작하고 usable Bath만 예약한다.
- reservation 성공 시 search window를 complete한다.
- Tick은 search/전체 expiry를 검사해 더 이상 retry하지 않는다.
- task exit은 active Bath activity를 actual segment 종료 후 approach 복귀, release 순으로 정리한다.
- `FCustomerFacilityTargetTask`, move 결과와 snap/begin-use 경로는 pre-entry usability failure를 구분한다.
- BathDwell duration은 기존 random `10~20초`와 남은 전체시간 중 작은 값이다.
- generic montage task는 current activity가 BathDwell일 때 bath-stay expiry와 usability invalidation에 안전하게 종료되며 다른 activity 의미는 유지한다.
- Condition은 query-only이고 timer, reservation, actual segment를 변경하지 않는다.

Editor 인계를 위한 기대 StateTree 흐름을 `.md/PROMPT_UNREAL.md`에 기록하되 이번 단계에서 asset을 열거나 저장하지 않는다.

```text
Start Bath Stay
→ BathLoop
   → Start/Continue Search
   → Reserve Usable Bath
   → Move To Approach
   → Revalidate / Snap / Begin Use
   → BathDwell
   → Return Approach / Release
   → 남은 시간: 새 Search
   → SearchExpired 또는 BathStayExpired: Main Shower
```

`BathBecameUnusable`는 Move/PreEntry/Dwell에서 각각 cleanup/exit branch로 전이한다. Dwell exit은 actual segment가 handler에서 이미 멈췄더라도 idempotent하다. 수위가 다시 오르는 event로 진행 중 exit을 역전하지 않는다.

## BathLoop 로그

공유 category:

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogBathhouseCustomerBath, Log, All);
```

`CustomerBathLoopLog.cpp`에서 한 번 define한다. Blueprint Print String과 `LogTemp`를 신규 BathLoop 경로에 사용하지 않는다.

internal diagnostic reason은 최소 다음을 구분한다.

```text
NoBathCandidate
WaterBelowThreshold
NoAvailableSlot
ReservationLost
NavigationFailed
EntryValidationFailed
DwellCompleted
BathStayExpired
SearchExpired
KnockdownInterrupted
StateTreeExited
TechnicalFailure
```

로그 수준:

- Log: loop/search/dwell begin/end, reservation success, forced exit, timeout, final advance
- Verbose: candidate/rejection aggregate, retry, move/pre-entry validation
- Warning: recoverable navigation/recovery failure
- Error: invariant violation only

공통 필드는 가능한 경우 한 줄에 다음 순서로 쓴다.

```text
Customer Iteration Phase BathActor Slot WaterPercent ThresholdPercent
BathStayRemaining ActualBathSeconds SearchElapsed Result Reason
```

매 Tick, amount frame update, 같은 condition 반복 평가를 Log/Warning으로 출력하지 않는다. normal no-candidate retry는 Verbose다. `ST_CustomerRoutine` graph transition 자체 문제는 Editor에서 `LogStateTree VeryVerbose`와 StateTree Debugger를 함께 사용한다.

## Presentation과 Blueprint API

새 native component는 `VisibleAnywhere, BlueprintReadOnly`와 stable 이름으로 파생 Blueprint에 보인다. 조정값만 `EditAnywhere, BlueprintReadOnly`다. runtime state setter를 BlueprintCallable로 추가하지 않는다.

Editor authoring 경로:

```text
Project Settings → Game → Bath Water
  CustomerUsableThresholdPercent = 80

BP_Bath Class Defaults / Level instance
  BathWaterState.FillRatePercentPerSecond = 6.666667
  BathWaterState.DrainRatePercentPerSecond = 10
  FillValveControl.LocalRotationAxis/OpenAngleDegrees/RotationDurationSeconds
  DrainLeverControl.LocalRotationAxis/OpenAngleDegrees/RotationDurationSeconds
  WaterLevelEmptyPoint / WaterLevelFullPoint Relative Location
```

asset 연결은 후속 Unreal 단계다.

- `WaterSurfaceMesh`: `/Game/Bathhouse/Meshes/Bath/Bath_01/StaticMeshes/SM_Bath_01_Water`
- control: 서로 분리되고 올바른 pivot/query collision geometry를 가진 valve/lever SM
- `FillFlowNiagara`: 사용자가 확정한 물줄기 System, AutoActivate false

현재 분리 control mesh가 없으므로 구현자가 임의 Engine Cube, combined Hardware, proxy Box 또는 `NS_HoneyBeam`으로 기능 완료를 주장하지 않는다.

## Lifecycle과 rollback 순서

### 선배치/신규 Bath

1. native/BP components 등록
2. control authored closed pose capture와 delegate bind
3. water empty/closed, surface/flow hidden 초기화
4. existing Facility registration/placement domain 활성화
5. threshold default 기준 availability publication

### Water threshold 하락

1. amount commit
2. usability crossing commit
3. Bath Facility availability publication
4. bound session actual segment stop/exit-pending commit
5. customer gameplay event
6. StateTree approach 복귀
7. slot release와 next search/next shower

### Recovery hold cancel/failure

1. placement hold 종료 결정
2. conversion이 시작됐다면 기존 domain/collision rollback 완료
3. Bath rollback 또는 cancel hook으로 water/control/Niagara snapshot 복원
4. reservation availability 재활성화/publication
5. target/query/result refresh

### Recovery success

1. hold snapshot 유지한 채 existing conversion transaction
2. Bath domain-unregistration stage에서 controls closed/flow off를 적용하되 snapshot 유지
3. payload에 water 미포함, domain/collision 비활성 원본 마지막 제거
4. source EndPlay에서 snapshot 폐기
5. recovery item physics/publication

## Native Automation

새 focused automation은 최소 다음을 포함한다.

### Water state

- default global threshold 80%, exact boundary `0.7999` false/`0.8` true
- settings getter clamp/finite defense와 모든 Bath 동일값
- fill-only, drain-only, both-open net rate, both-closed hold
- `6.666667%/s` 약 15초 fill, `10%/s` 약 10초 drain
- exact 0/1 endpoint, drain-open-at-zero, full auto-close
- amount/state/usability/control delegate actual transition once
- staged/pre-placed reset empty and deprecated setter not used by canonical test

### Control/interaction

- hit component가 직접 `IPlayerInteractable`이고 separate Box가 없음
- component collision query/Visibility/nav defaults
- open/close action/failure, full fill rejection와 empty drain allowance
- moving re-input rejection and other control independence
- `(1,0,0)`/`(-1,0,0)` opposite, negative-axis+positive-angle equivalence
- zero/non-finite axis and duration validation
- opening 중 full auto-close current angle reversal
- authored closed pose exact return and tick disable

### Presentation

- amount `0/0.25/0.5/0.8/1`에서 exact vector Lerp
- 0 hidden, positive visible, full point exact
- WaterSurfaceMesh rotation/scale/material 불변
- collision/physics/navigation off
- fill Niagara state and recovery freeze/cancel restore

### Facility/customer

- threshold 미만 Bath/slot candidate 제외와 exact threshold 포함
- last-bath fallback도 usability filter 유지
- threshold crossing facility notification once
- movement/pre-entry invalidation cleanup
- multiple occupied sessions의 threshold 하락 event와 approach-before-release
- rising threshold가 exit-pending을 취소하지 않음
- search timer 10초/remaining-total clamp, retry no reset와 new cause new window
- actual dwell excludes search/move/entry/exit/knockdown and sums multiple segments
- total expiry/search expiry event exactly once and no satisfaction mutation
- knockdown pause/resume current water revalidation

### Recovery

- Q Hold begin at empty/open-fill state freezes amount/control motion and hides flow
- hold 동안 threshold 0 설정에서도 reservation 불가
- release/gaze/suppression/condition cancel exactly once; successful or unexpected EndPlay discards snapshot without restore callback
- cancel and injected conversion failures restore exact amount/logical state/quaternion/remaining motion/flow
- success closes controls, discards state and new placement starts empty
- repeated hook calls do not overwrite snapshot or duplicate publication

### Regression

- generic Facility reservation/slot, placement conversion/collision rollback tests
- washer/dryer/locker recovery unaffected by default no-op hook
- Interaction E/F/G/Q/LMB routing and prompt result unaffected
- existing Bath snap and customer knockdown automation

테스트 helper가 settings CDO를 일시 변경하면 scope 종료 시 원래 값을 반드시 복원해 다른 test에 누출하지 않는다. UObject lifecycle이 필요한 검증은 실제 World/component registration을 사용하고 단순 setter만 검사해 성공을 과장하지 않는다.

## 빌드와 결과물

1. `git diff --check`
2. UE 5.8 정책의 `Build.bat BathhouseSimEditor Win64 Development`
3. focused `BathhouseSim.BathWater` automation
4. 관련 Facility/Placement/Customer/Interaction/Recovery automation
5. 가능하면 전체 `BathhouseSim` automation

구현자는 사용자 소유의 기존 dirty 파일과 asset을 되돌리거나 덮어쓰지 않는다. 특히 현재 Placement grid 후속 작업을 bath 작업으로 재작성하지 않는다.

완료 후 현재 Bath Water 작업만 담은 다음 결과물을 작성한다.

- `.md/PROMPT_REVIEW.md`: 변경 파일, 설계 준수, tests/build 결과와 남은 위험
- `.md/PROMPT_UNREAL.md`: `BP_Bath` reparent, components/assets/collision/transforms, Project Settings, DataAsset와 `ST_CustomerRoutine` migration/검증

Unreal 인계에는 최소 다음을 명시한다.

- `BP_Bath` parent를 `ABathhouseBathFacilityActor`로 변경하고 compile/resave
- inherited control component에 분리 valve/lever mesh, query collision, Visibility Block, local axis/signed angle/duration 지정
- `WaterSurfaceMesh`에 `SM_Bath_01_Water`, Empty/Full marker와 plane local transform 지정
- 최종 물줄기 Niagara와 transform 지정; `NS_HoneyBeam`을 근거 없이 재사용하지 않음
- global threshold 80, rates 6.666667/10 확인
- `DA_CustomerRoutine_Default` BathStay 60, BathSearch 10, dwell 10~20 저장
- 실제 `ST_CustomerRoutine.BathLoop` hierarchy/task/binding을 열어 두 신규 event와 search/revalidation/exit flow 연결
- BathLoop category와 `LogStateTree VeryVerbose`를 사용한 PIE 로그 검증
- `Bath`, `Bath2` instance의 missing visual/Approach override를 의도와 migration 오류로 구분
- 분리 control mesh나 StateTree 편집이 MCP로 불가능하면 우회하지 않고 `.md/USER_UNREAL.md`에 exact blocker 기록
