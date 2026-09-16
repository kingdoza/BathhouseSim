# Bath Water System

## Implementation Status

이 문서는 승인된 `BH-WATER-001`~`BH-WATER-029`의 첫 욕탕 타입 수직 구현 아키텍처를 정의한다. 연속 수위, 실제 mesh 조작부, 욕탕 전용 표현 Actor, Facility 가용성, 회수 동결과 Customer BathLoop timer/delegate/log는 Source와 focused native automation까지 구현됐다. `BP_Bath`, Project Settings, Data Asset와 `ST_CustomerRoutine` 연결은 코드 리뷰 뒤 Editor 통합 대기 상태다.

Facility slot과 등록은 [FacilitySystem.md](FacilitySystem.md), Q Hold 회수 transaction은 [PlacementSystem.md](PlacementSystem.md), 손님 timer·StateTree·knockdown은 [CustomerSystem.md](CustomerSystem.md)와 [CustomerRecoverySystem.md](CustomerRecoverySystem.md)를 따른다.

## Target Source Scope

```text
Source/BathhouseSim/Public/Facility/
  BathWaterSettings.h
  BathWaterStateComponent.h
  BathWaterControlComponent.h
  BathhouseBathFacilityActor.h

Source/BathhouseSim/Private/Facility/
  BathWaterSettings.cpp
  BathWaterStateComponent.cpp
  BathWaterControlComponent.cpp
  BathhouseBathFacilityActor.cpp

Source/BathhouseSim/Public|Private/Customer/
  CustomerRoutineDefinition.*
  CustomerSessionComponent.*
  CustomerSessionBath.cpp
  BathhouseCustomerTypes.*
  StateTree/CustomerStateTreeTasks.*
  StateTree/CustomerStateTreeConditions.*
  CustomerBathLoopLog.*
  CustomerBathLoopLog.*

Source/BathhouseSim/Public|Private/Placement/
  PlaceableFacility.h
  PlayerFacilityPlacementComponent.*

Source/BathhouseSim/Private/Tests/
  BathWaterAutomationTests.cpp
  BathhouseDomainTests.cpp
  CombatRecoveryAutomationTests.cpp
  FacilityPlacementAutomationTests.cpp
```

## Responsibilities

- 욕탕별 `0~1` authoritative 물 양과 급수·배수 순변화
- 급수밸브·배수레버의 독립적인 논리 상태와 회전 표현
- 만수 급수 자동 폐쇄와 급수 Niagara 활성 상태
- 평면 수면 Static Mesh의 0%~100% 위치 보간
- 모든 욕탕이 공유하는 조정 가능한 손님 입욕 임계 수위
- 임계치 횡단에 따른 Facility 후보 publication과 현재 이용 Customer 통지
- Q Hold 회수 중 물·조작부 동결과 cancel/실패 시 정확 복원
- 물 관련 C++ 상태를 위한 Blueprint 표현 event와 authoring 검증

Bath Water는 facility slot reservation, StateTree hierarchy, Customer 전체 입욕 timer, satisfaction, placement Actor 교체와 UI hierarchy를 소유하지 않는다.

## State And Execution Owners

| 책임 | Owner |
|---|---|
| 전역 손님 입욕 임계 수위 | `UBathWaterSettings` |
| 물 양, 급수·배수 개방 상태, 유량, 파생 상태와 회수 동결 snapshot | `UBathWaterStateComponent` |
| 한 조작부의 메시 충돌, interaction query와 회전 재생 | `UBathWaterControlComponent` |
| 욕탕 전용 component 조립, 수면/Niagara 표현과 Facility publication | `ABathhouseBathFacilityActor` |
| 욕탕 이용 슬롯 | `UBathhouseFacilitySlotComponent` |
| 욕탕 후보 수집·random reservation | `UBathhouseFacilitySubsystem` |
| 전체 입욕·탐색 timer, 실제 체류 누적과 현재 욕탕 delegate | `UCustomerSessionComponent` |
| BathLoop state/transition | `ST_CustomerRoutine` |
| Q Hold 경과와 Actor 변환 transaction | `UPlayerFacilityPlacementComponent` |

범용 `ABathhouseFacilityActor`는 기존 `BathWaterState` default subobject와 회수 gate를 호환용으로 유지한다. 물 조작부와 표현을 범용 Actor에 추가하지 않고 `ABathhouseBathFacilityActor`가 기존 component를 배선한다.

## Global Threshold Settings

`UBathWaterSettings : UDeveloperSettings`는 `Config=Game`, `DefaultConfig`로 만들고 Project Settings의 다음 경로에 노출한다.

```text
Project Settings
→ Game
→ Bath Water
→ Customer Usable Threshold Percent
```

```cpp
UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Customer Use",
    meta=(ClampMin="0.0", ClampMax="100.0"))
float CustomerUsableThresholdPercent = 80.0f;
```

- `80.0`은 프로젝트 기본값이며 하드코딩된 판정값이 아니다.
- getter는 유한성 및 `0~100`을 방어하고 `0~1`로 정규화한다.
- 욕탕 Blueprint 종류와 Level instance에는 같은 threshold property를 만들지 않는다.
- 런타임 도중 설정 변경을 gameplay 기능으로 지원하지 않는다. 다음 World/PIE 시작부터 모든 욕탕이 같은 값을 읽는다.
- `NormalizedAmount >= ThresholdNormalized`일 때 정확히 이용 가능하다.

기존 `DeveloperSettings` module 의존성을 재사용하며 새 plugin은 필요 없다.

## `UBathWaterStateComponent`

기존 reflected component 이름 `BathWaterState`와 `GetWaterState`, `GetNormalizedAmount`, `IsEmpty`, `OnWaterStateChanged`를 보존하면서 authoritative 실행 owner로 확장한다.

Authoring 값:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bath Water|Flow",
    meta=(ClampMin="0.0"))
float FillRatePercentPerSecond = 6.666667f;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bath Water|Flow",
    meta=(ClampMin="0.0"))
float DrainRatePercentPerSecond = 10.0f;
```

- 단위는 수위 퍼센트 포인트/초다.
- `BP_Bath` Class Default와 Level 개별 instance override를 모두 허용한다.
- runtime 계산에서 각각 `0.01`을 곱해 normalized rate로 변환한다.
- rate는 finite/non-negative여야 하며 invalid authoring은 Data Validation 실패다.

Runtime state:

- `NormalizedAmount`
- `bFillValveOpen`, `bDrainLeverOpen`
- 파생 `EBathWaterState`
- 이전 손님 이용 가능 여부
- 회수 동결 여부와 단일 snapshot

`EBathWaterState`의 기존 ordinal은 유지하고 중간 수위에서 순변화가 0인 상태를 나타내는 `Holding`을 enum 끝에 추가한다. `Empty/Filled`는 endpoint, `Filling/Draining`은 순유량 부호, `Holding`은 그 밖의 중간 수위다. 회수 gate는 enum이 아니라 endpoint로 snap된 `NormalizedAmount == 0`을 정본으로 사용한다.

기존 `SetWaterState`, `SetNormalizedAmount` reflected signature는 한 migration cycle 동안 보존하되 deprecated compatibility API로 표시한다. 신규 runtime, Blueprint와 StateTree는 이를 호출하지 않고 native request/commit API만 사용한다.

주요 native query/mutation 경계:

```text
IsCustomerUsable()
IsControlOpen(ControlType)
CanSetControlOpen(ControlType, bOpen, OutFailure)
RequestSetControlOpen(ControlType, bOpen, Reason, OutFailure)
BeginRecoveryFreeze(OutFailure)
CancelRecoveryFreeze()
PrepareRecoveryCommit()
ResetEmptyForPlacement()
```

`RequestSetControlOpen`은 같은 상태 요청을 idempotent하게 처리하며 논리 상태를 먼저 commit하고 control/presentation delegate를 발생시킨다. 닫힌 급수밸브는 100%에서 열 수 없고 정확한 실패 이유를 제공한다. 0%의 닫힌 배수레버는 열 수 있다.

## Water Integration

component Tick은 물 양이 변할 가능성이 있을 때만 활성화한다. Actor Tick은 사용하지 않는다.

```text
NetPercentPerSecond
  = (FillOpen ? FillRatePercentPerSecond : 0)
  - (DrainOpen ? DrainRatePercentPerSecond : 0)

NewAmount
  = Clamp(CurrentAmount + NetPercentPerSecond * 0.01 * DeltaSeconds, 0, 1)
```

- endpoint 도달 시 값을 정확히 `0` 또는 `1`로 snap한다.
- 두 장치가 모두 열리고 순유량이 0이면 현재 수위를 유지한다.
- 0%에서 배수만 열려 있으면 레버 논리 상태를 유지하되 불필요한 Tick은 중지한다. 급수 상태 변경이 Tick을 다시 깨운다.
- 100% 도달 frame에 급수 논리 상태를 즉시 닫고 Niagara를 끈다.
- 급수밸브가 열리는 중이었다면 현재 보이는 회전에서 닫힌 자세로 방향을 바꾼다.
- amount 변경 delegate는 실제 값 변경에만, usability delegate는 전역 threshold를 횡단할 때만 한 번 발생한다.

## `UBathWaterControlComponent`

`UBathWaterControlComponent`는 별도 상호작용 Box가 아닌 실제 밸브·레버 `UStaticMeshComponent`이자 `IPlayerInteractable`이다.

```text
UBathWaterControlComponent
  : UStaticMeshComponent
  : IPlayerInteractable
```

Player Interaction은 hit component에서 interface를 먼저 찾으므로 밸브와 레버를 actor fallback 없이 구분한다. component는 자신의 욕탕 상태에 toggle intent만 전달하고 물 양을 직접 변경하지 않는다.

Collision 계약:

- 지정된 Static Mesh asset의 collision geometry를 interaction trace 형상으로 사용한다. 기존 Player Interaction trace는 complex trace를 허용하므로 asset의 triangle collision도 사용할 수 있고 Simple Collision은 선택적인 성능 최적화다.
- 추가 `UBoxComponent`, proxy collision 또는 actor-wide fallback collision을 만들지 않는다.
- component는 query collision이 활성화되고 `Visibility`를 Block해야 한다.
- 물리 충돌은 요구하지 않으며 `CanEverAffectNavigation=false`다.
- Static Mesh가 없거나 query collision/`Visibility` Block이 아니면 Data Validation 실패다. collision 형상이 없는 경우 임의 bounds fallback을 만들지 않는다.

Authoring 값:

- `LocalRotationAxis`
- signed `OpenAngleDegrees`
- `RotationDurationSeconds`
- component의 authored Relative Transform과 Static Mesh/Collision 설정

`LocalRotationAxis`는 닫힌 자세 기준 component-local 축이며 정규화할 때 부호를 보존한다. `(1,0,0)`과 `(-1,0,0)`은 같은 각도에서 반대 방향이고, `Axis=(-1,0,0), Angle=90`은 `Axis=(1,0,0), Angle=-90`과 같다. zero/non-finite 축은 validation 실패다. 가독성을 위해 단위 양의 축과 signed angle 사용을 권장하지만 음의 축도 완전히 지원한다.

회전 중심은 축 변수가 아니라 Static Mesh pivot이다. 분리 asset의 pivot이 실제 밸브 축·레버 hinge와 일치해야 한다. 필요한 경우 비충돌 SceneComponent를 회전 pivot 부모로 둘 수 있지만 interaction collision은 계속 mesh component만 사용한다.

BeginPlay에 authored closed relative quaternion을 저장하고 다음 목표를 계산한다.

```text
OpenTarget = ClosedBasis와 LocalAxis/OpenAngle의 local quaternion 합성
ClosedTarget = authored ClosedRotation
```

회전은 고정 ease-in/out alpha와 quaternion slerp를 사용한다. `RotationDurationSeconds`는 닫힘↔완전 열림 전체 각도의 시간이며 만수 자동 닫힘이 중간 각도에서 시작되면 남은 각도 비율만큼 시간을 줄여 같은 체감 각속도를 유지한다. player 입력은 회전 중 거부하지만 만수 자동 닫힘은 현재 각도에서 즉시 반전할 수 있다.

Interaction query:

- 닫힘: `급수밸브 열기` 또는 `배수레버 열기`
- 열림: `급수밸브 닫기` 또는 `배수레버 닫기`
- 회전 중: 실행 불가, `작동 중입니다`
- 100%에서 닫힌 급수밸브: 실행 불가, `이미 물이 가득 차 있습니다`
- 회수 동결 중: 실행 불가, `설비 회수 중입니다`
- staged/비활성 placement domain: query 숨김
- `LocalRotationAxis`가 finite/non-zero가 아니거나 angle이 non-finite이거나 duration이 finite/양수가 아니면 명시적 authoring 실패로 실행 불가

들고 있는 item 종류는 query 조건에 포함하지 않는다.

Data Validation과 runtime interaction query는 같은 side-effect-free motion authoring helper를 사용한다. `ExecuteInteraction()`은 이 query가 성공한 뒤에만 water request를 보낸다. `CalculateOpenRotation()`, motion start와 Tick도 독립적으로 quaternion·duration 유한성을 확인하여 잘못된 직접 호출이나 runtime override가 component transform에 non-finite 값을 기록하지 못하게 한다.

## `ABathhouseBathFacilityActor`

`ABathhouseBathFacilityActor : ABathhouseFacilityActor`는 첫 욕탕 Blueprint의 새 native parent이자 composition root다. Actor Tick은 사용하지 않는다.

Native component 계약:

```text
SceneRoot
├─ FillValveControl        (UBathWaterControlComponent)
├─ DrainLeverControl       (UBathWaterControlComponent)
├─ FillFlowNiagara         (UNiagaraComponent)
└─ WaterPresentationRoot   (USceneComponent)
   ├─ WaterSurfaceMover    (USceneComponent)
   │  └─ WaterSurfaceMesh  (UStaticMeshComponent)
   ├─ WaterLevelEmptyPoint (USceneComponent)
   └─ WaterLevelFullPoint  (USceneComponent)

BathWaterState             (inherited UBathWaterStateComponent)
```

두 control kind는 native constructor가 고정하고 Blueprint가 바꾸지 않는다. Actor는 water-state delegate를 control, 수면, Niagara와 Facility publication에 배선한다.

Blueprint 표현 notification:

- `OnBathWaterAmountChanged(PreviousPercent, CurrentPercent)`
- `OnBathCustomerUsabilityChanged(bUsable)`
- `OnFillFlowPresentationChanged(bActive)`
- 필요 시 기존 control mesh 자체 회전 외의 부가 표현

위 event가 비어 있어도 C++이 수면 위치/가시성, 조작부 회전과 Niagara 활성 상태를 완성한다. Blueprint는 water amount, open state, usability와 회수 상태를 변경하지 않는다.

`UNiagaraComponent`를 사용하므로 runtime module에 `Niagara` dependency를 추가한다. Niagara System과 nozzle Relative Transform은 욕탕 Blueprint authoring 값이고 `AutoActivate=false`다. 조사된 `/Game/Niagaras/NS_HoneyBeam`은 물줄기로 확정된 asset이 아니므로 자동 지정하지 않는다.

## Plane Water Surface Presentation

수면 asset 계약은 cube나 volume mesh가 아닌 수평 평면 Static Mesh다. 첫 수직 구현의 기준 asset은 다음이다.

```text
/Game/Bathhouse/Meshes/Bath/Bath_01/StaticMeshes/SM_Bath_01_Water
```

이 asset은 C++에서 hard reference하지 않고 `BP_Bath`의 inherited `WaterSurfaceMesh`에 지정한다. 미래 욕탕도 각 형태에 맞는 평면 수면 SM을 같은 component에 지정한다.

`WaterSurfaceMesh`의 local transform은 imported pivot, 방향과 크기를 한 번 보정한다. runtime은 mesh transform을 변형하지 않고 부모 `WaterSurfaceMover`의 위치만 바꾼다. `WaterLevelEmptyPoint`, `WaterLevelFullPoint`와 mover는 같은 `WaterPresentationRoot` local space를 사용한다.

```text
Alpha = Clamp(NormalizedAmount, 0, 1)
MoverLocation = Lerp(EmptyPoint.RelativeLocation,
                     FullPoint.RelativeLocation,
                     Alpha)
```

- `0`: EmptyPoint에 두고 수면 mesh를 숨긴다.
- `0 < Alpha < 1`: mesh를 표시하고 두 지점 사이를 연속 선형 보간한다.
- `1`: FullPoint에 둔다.
- 보간은 XYZ를 지원하지만 일반 authoring은 X/Y를 같게 하고 Z만 다르게 둔다.
- 수위에 따라 scale, material WPO, mesh 재생성 또는 volume simulation을 사용하지 않는다.
- marker는 mover pivot endpoint다. 평면 asset pivot 차이는 `WaterSurfaceMesh` child local transform으로 보정한다.

수면 component는 collision/overlap/physics/Navigation이 모두 꺼져 있고 generic facility placement preview의 runtime water 표현 수집 대상이 아니다.

## Facility Availability And Customer Boundary

`ABathhouseFacilityActor`에 side-effect-free virtual `IsAvailableForReservation()`을 추가한다. 기본 구현은 기존 enabled/domain 조건을 유지하고 Bath subclass는 `BathWaterState.IsCustomerUsable()` 및 `!IsRecoveryFrozen()`을 추가한다.

`UBathhouseFacilitySubsystem::TryReserveRandomSlot()`은 candidate 수집과 실제 reserve 직전에 이 query를 사용한다. 따라서 threshold 미만 Bath의 빈 slot은 후보가 아니다. last-bath exclusion 후 fallback도 같은 filter를 통과한다.

threshold 횡단 시 Bath Actor는 `NotifyFacilityAvailabilityChanged(Bath)`를 한 번 호출해 기다리는 Customer를 깨운다. 현재 Bath를 예약·이동·사용 중인 `UCustomerSessionComponent`는 그 Bath의 usability delegate를 reservation 수명 동안 구독한다.

- 이동/입장 전 하락: StateTree에 `Customer.Event.BathBecameUnusable`을 보내 이동을 끝내고 reservation을 정리한다.
- 실제 체류 중 하락: 실제 체류 누적을 그 순간 중지하고 exit-pending을 표시한 뒤 같은 event를 보낸다.
- 퇴장 transition은 cached ApproachPoint 복귀를 완료한 다음 slot을 release한다.
- 퇴장 중 다시 threshold 이상이 되어도 현재 퇴장을 취소하지 않는다.
- release 후 남은 전체 시간이 있으면 새 탐색에서 같은 Bath도 다시 선택할 수 있다.
- 한 Bath의 모든 current session이 각각 delegate를 받으므로 사용 중인 손님 전원이 같은 규칙으로 나온다.

## Recovery Hold Freeze

`IPlaceableFacility`에 기본 no-op인 recovery-hold begin/cancel hook을 추가하고 `UPlayerFacilityPlacementComponent`가 Q Hold target에 전달한다. Query는 계속 side-effect free다.

```text
TryBeginFacilityRecoveryHold(OutFailure)
CancelFacilityRecoveryHold()
```

Bath는 모든 slot이 Available이고 수위가 정확히 0일 때 snapshot을 하나 생성한다.

Snapshot 범위:

- normalized amount와 두 control logical state
- 각 control의 현재 relative rotation, motion 방향과 남은 시간
- Niagara 활성 여부
- 이전 usability/facility availability 상태

Hold begin commit 뒤에는 water Tick과 control rotation Tick을 멈추고 Niagara를 숨긴다. Bath reservation query도 false가 되어 threshold가 0으로 설정돼 있어도 새 Customer가 들어오지 않는다.

- input release, gaze/target 변경, 조건 변경 또는 suppression: cancel hook이 snapshot을 정확히 복원하고 원래 motion/flow를 남은 지점부터 재개한다.
- conversion의 Bath domain-unregistration stage는 두 control을 닫고 Niagara를 끄는 commit-pending 상태를 만들되 snapshot을 아직 폐기하지 않는다.
- source Actor destroy 성공은 EndPlay에서 snapshot을 폐기한다. destroy 이전 conversion 실패는 Bath rollback이 기존 domain/collision 복원 뒤 snapshot까지 복구하며 Player의 뒤이은 cancel은 no-op다.
- cancel/commit/EndPlay의 중복 호출은 no-op이고 snapshot은 한 개만 허용한다.
- hold 중 slot 상태가 예기치 않게 바뀌면 기존 매 Tick recovery 재검증이 hold를 취소하고 snapshot을 복원한다.

## Initialization, Placement And EndPlay

- 선배치 Bath BeginPlay와 새 staged placement import는 모두 `0`, 두 control 닫힘, 수면/Niagara 숨김으로 초기화한다.
- 기존 CDO/instance에 저장된 `WaterState`와 `NormalizedAmount` override는 시작 상태 정본이 아니며 migration 후 제거한다.
- placement staged/domain inactive 상태에서는 water/control Tick과 interaction을 사용하지 않는다.
- placement commit으로 domain이 활성화된 뒤 빈 상태 publication을 허용한다.
- 회수 payload는 water/control snapshot을 저장하지 않는다.
- EndPlay는 water/control Tick, delegates, Niagara와 recovery snapshot을 idempotent하게 정리한다.
- 이번 범위에서 SaveGame persistence를 추가하지 않는다.

## Blueprint And Editor Authoring

| 값 | 정본 경로 | 기본값/계약 | Level override |
|---|---|---|---|
| 입욕 임계 수위 | Project Settings → Game → Bath Water | `80%` | 불가 |
| 급수 속도 | `BP_Bath.BathWaterState` → Flow | `6.666667%/s` | 허용 |
| 배수 속도 | `BP_Bath.BathWaterState` → Flow | `10%/s` | 허용 |
| control 축/각도/시간 | 각 inherited control component | 메시별 | 허용 |
| control collision | 각 control Static Mesh + component | Simple Collision, Visibility Block | 허용하되 검증 필수 |
| 0%/100% 위치 | Empty/Full Point Relative Location | 욕조 내부 형태 기준 | 허용 |
| 수면 평면 SM/material | `WaterSurfaceMesh` | 첫 타입 `SM_Bath_01_Water` | 종류 기본값 권장 |
| 물줄기 System/위치 | `FillFlowNiagara` | AutoActivate false | 위치 override 허용 |
| 전체 입욕 시간 | `DA_CustomerRoutine_Default` | `60s` | Data Asset |
| 탐색 제한시간 | 같은 Data Asset | `10s` | Data Asset |
| dwell 범위 | 같은 Data Asset | `10~20s` | Data Asset |

초기 수위, initial control state, 0/100 endpoint, 수면 표시 규칙, auto-close, recovery freeze, Tick 간격과 numeric epsilon은 authoring하지 않는다.

현재 `/Game/Bathhouse/Blueprints/Facility/BP_Bath`는 `ABathhouseFacilityActor` parent이므로 새 subclass로 reparent하고 compile/resave해야 한다. 기존 combined `SM_Bath_01_Hardware` 하나는 두 독립 control의 mesh/pivot/collision source가 될 수 없다. 분리 밸브·레버 Static Mesh와 query 가능한 collision geometry가 준비되지 않으면 Editor 단계에서 추측하거나 proxy Box로 우회하지 않고 `USER_UNREAL.md`에 남긴다.

`BP_Bath`의 현재 `SM_Bath_old`, `Bath2`의 missing visual과 Approach override는 현재 결함을 목표 계약으로 승격하지 않고 Editor migration에서 직접 검증한다. reparent는 기존 class의 자식으로 이동하므로 Core Redirect가 필요하지 않다. reflected 이름을 이번 단계에서 rename/delete하지 않는다.

## Customer BathLoop Diagnostics

공유 `LogBathhouseCustomerBath` category를 session과 native StateTree Task가 사용한다. search는 시작 시 clamp가 적용된 resolved duration을 보존하고 `resolved duration - remaining`으로 실제 active elapsed를 계산하므로 10초 기본 window, 남은 stay에 clamp된 window와 pause/resume을 같은 규칙으로 진단한다. native Task는 Bath 이동 결과, action snap/begin-use validation, dwell/search/stay advance와 forced-exit cleanup의 terminal 결정만 기록한다. Tick·water frame·Condition 반복은 기본 로그에 남기지 않는다. 수준별 항목과 공통 필드는 [CustomerSystem.md](CustomerSystem.md)의 `BathLoop Diagnostics`가 정본이다. Blueprint `Print String`은 사용하지 않고 graph 자체 추적은 `LogStateTree VeryVerbose`와 StateTree Debugger를 병행한다.

## Dependencies

- Bath Water -> Facility slot/subsystem public 계약
- Bath Water -> Interaction `IPlayerInteractable` query/result 계약
- Bath Water -> Placement recovery-hold lifecycle 계약
- Bath Water -> `Niagara` runtime module
- Customer -> Bath Water public query/delegate
- StateTree/Blueprint는 Bath Water 상태를 직접 mutate하지 않는다.

## Verification

- water amount endpoint, 독립/동시 유량과 15초/10초 기본 결과
- 전역 threshold 기본 80%와 설정 변경 시 모든 Bath의 동일 경계
- threshold 횡단 publication once와 exact 80% 후보 포함
- 100% auto-close, opening 중 reversal과 Niagara 즉시 종료
- positive/negative local axis, signed angle, zero-axis validation과 authored closed pose 복귀
- control mesh 자체 Visibility collision만 hit되고 별도 interaction Box가 없는지 확인
- 평면 수면 mesh가 0에서 숨고 `Lerp(Empty, Full, Amount)` 위치만 사용하는지 확인
- search window가 retry로 reset되지 않고 60초 남은 시간으로 clamp되는지 확인
- 실제 dwell만 누적되고 이동/퇴장/search/knockdown은 제외되는지 확인
- 사용 중 threshold 하락 시 모든 Customer가 approach 복귀 뒤 release하고 상승으로 퇴장을 취소하지 않는지 확인
- Q Hold freeze의 cancel/transaction-failure exact restore와 successful state discard
- pre-placed/new placement/replacement가 모두 empty/closed/hidden으로 시작하는지 확인
- 로그가 주요 phase/reason/correlation field를 제공하고 Tick spam을 만들지 않는지 확인
