# 구현 프롬프트 — 욕탕 물 순환·가열·냉각과 컴퓨터 제어

## 입력과 현재 단계

이 프롬프트는 다음 승인·설계 정본을 Source에 구현하기 위한 지시다.

- 기능 계약: `.md/PROMPT_ARCHITECTURE.md`
- 중심 설계: `.md/Architecture/BathWaterOperationsSystem.md`
- 관리 화면 설계: `.md/Architecture/BathWaterManagementUISystem.md`
- 기존 물 시스템: `.md/Architecture/BathWaterSystem.md`
- 관련 경계:
  - `.md/Architecture/PlacementSystem.md`
  - `.md/Architecture/ComputerSystem.md`
  - `.md/Architecture/UISystem.md`
  - `.md/Architecture/CustomerSystem.md`
  - `.md/Architecture/FacilitySystem.md`
  - `.md/Architecture/CoreSystem.md`
- Editor 사실: `.md/REPORT_UNREAL_DISCOVERY.md`
- 구현 규칙: `.md/AGENT_WORKFLOW.md`, `.md/AGENT_IMPLEMENTATION.md`

아키텍처 단계는 완료됐다. 이 단계는 C++ Source와 native automation만 구현한다. `Content/`, Level, Blueprint, Widget Blueprint, StateTree asset과 실제 Editor property assignment는 수정하지 않는다.

## 목표

1. 설치된 순환기·보일러·쿨러의 종류별 총용량과 설치 욕탕의 요구량을 authoritative하게 관리한다.
2. 욕탕별 순환도, 목표/실제 수온, 오염도와 실제 입욕자 수를 native 상태로 구현한다.
3. 기존 급수·배수의 실제 유입량을 사용해 ambient/clean water mixing을 구현한다.
4. 정상 회수, staged rollback과 예기치 않은 소실에서 용량·요구량이 손실·중복되지 않게 한다.
5. world-space computer에 연결할 native 욕탕 관리 Widget 계층과 request/snapshot 경계를 구현한다.
6. 기존 interaction, single physical carry, placement, customer routine, bath water threshold와 computer focus 계약을 회귀 없이 보존한다.

## 변경 금지와 비목표

- `Content/`, `Config/`, Level과 serialized asset을 생성·수정·resave하지 않는다.
- `ST_CustomerRoutine` graph를 변경하지 않는다.
- 새로운 Input Action이나 Mapping Context를 만들지 않는다.
- 기존 reflected type/property/component를 rename/delete하지 않는다.
- `UComputerSampleScreenWidget`, `WBP_ComputerSampleScreen` 호환 경로와 deprecated `ComputerClickAction` fallback을 제거하지 않는다.
- 기존 물 양, 밸브·레버 상호작용, 수면 보간, 임계 수위와 BathLoop 시간을 다시 설계하지 않는다.
- SaveGame, replication, 연료·전원·고장, utility On/Off, 오염도 customer 효과와 최종 art를 추가하지 않는다.
- utility를 `EBathhouseFacilityType` 또는 customer facility registry에 억지로 추가하지 않는다.
- `ABathhouseFacilityActor`에 utility 종류 분기와 전역 원장을 누적하지 않는다.
- `UBathWaterStateComponent`에 수온·오염도·utility capacity를 누적하지 않는다.
- UI, Character와 Blueprint event가 domain 상태를 직접 set하게 하지 않는다.
- 하드코딩된 `10%` thermal threshold를 gameplay 판정에 사용하지 않는다.

## 구현 전 확인

실제 Source를 다시 읽고 설계와 이름이 충돌하면 기존 public/reflected 계약을 우선 보존한다. 특히 다음을 확인한다.

- `UBathWaterStateComponent` Tick gate와 amount/usability delegate
- `ABathhouseBathFacilityActor` recovery freeze/stage/rollback/EndPlay 순서
- `IPlaceableFacility`, `UFacilityPlacementComponent`, `FFacilityPlacementPayload`와 typed instance data
- `ATowelProcessingMachineActor`의 독립 placeable actor 패턴
- `ABathhouseComputerActor::BeginPlay()`의 `InitWidget()`과 focus 계약
- `AFirstPersonCharacter`의 `PrimaryUseAction` 및 `Computer > Placement > Equipment` owner 우선순위
- `UCustomerSessionComponent::BeginActualBathSegment()`와 `EndActualBathSegment()`의 모든 호출 경로

사용자 변경이 섞여 있으면 관련되지 않은 diff를 되돌리거나 포맷하지 않는다.

## 파일 계획

신규 파일:

```text
Source/BathhouseSim/Public/Facility/BathWaterOperationsTypes.h
Source/BathhouseSim/Public/Facility/BathWaterOperationsSubsystem.h
Source/BathhouseSim/Private/Facility/BathWaterOperationsSubsystem.cpp
Source/BathhouseSim/Public/Facility/BathWaterUtilityCapacityComponent.h
Source/BathhouseSim/Private/Facility/BathWaterUtilityCapacityComponent.cpp
Source/BathhouseSim/Public/Facility/BathWaterUtilityFacilityActor.h
Source/BathhouseSim/Private/Facility/BathWaterUtilityFacilityActor.cpp
Source/BathhouseSim/Public/Facility/BathWaterUtilityPlacementInstanceData.h
Source/BathhouseSim/Private/Facility/BathWaterUtilityPlacementInstanceData.cpp
Source/BathhouseSim/Public/Facility/BathWaterConditionComponent.h
Source/BathhouseSim/Private/Facility/BathWaterConditionComponent.cpp

Source/BathhouseSim/Public/UI/BathWaterManagementScreenWidget.h
Source/BathhouseSim/Private/UI/BathWaterManagementScreenWidget.cpp
Source/BathhouseSim/Public/UI/BathWaterCapacitySummaryWidget.h
Source/BathhouseSim/Private/UI/BathWaterCapacitySummaryWidget.cpp
Source/BathhouseSim/Public/UI/BathWaterMapWidget.h
Source/BathhouseSim/Private/UI/BathWaterMapWidget.cpp
Source/BathhouseSim/Public/UI/BathWaterBathTileWidget.h
Source/BathhouseSim/Private/UI/BathWaterBathTileWidget.cpp
Source/BathhouseSim/Public/UI/BathWaterDetailWidget.h
Source/BathhouseSim/Private/UI/BathWaterDetailWidget.cpp

Source/BathhouseSim/Private/Tests/BathWaterOperationsAutomationTests.cpp
```

기존 파일 수정은 아키텍처 문서에 적힌 연결 지점에 한정한다. 실제 응집도를 위해 private helper cpp를 추가할 수 있지만 새 subsystem이나 범용 framework를 임의로 만들지 않는다.

## 공용 타입

`BathWaterOperationsTypes.h`에 필요한 reflected/native 타입을 정의한다.

필수 의미:

- `EBathWaterCapacityKind`: Circulation, Heating, Cooling
- `EBathWaterThermalStatus`: Empty, ReturningToAmbient, Stalled, MovingToTarget, MaintainingTarget, SuspendedByCapacity
- setting request failure/reason enum
- 종류별 used/total/deficit snapshot
- 욕탕 표시 snapshot
- setting request result

조건:

- snapshot은 UI가 domain object 내부 상태를 재계산하지 않게 필요한 수치를 포함한다.
- bath identity는 weak object reference를 사용한다.
- FText/한국어 문구는 domain 타입에 넣지 않는다.
- float는 외부 입력 경계에서 finite 검증한다.
- 종류 enum ordinal은 처음 추가되는 값이므로 명시적으로 안정적인 순서를 사용한다.

## `UBathWaterSettings` 확장

기존 Developer Settings에 다음 값을 추가한다.

```text
AmbientTemperatureC = 20.0
MinTargetTemperatureC = 10.0
MaxTargetTemperatureC = 50.0
TargetTemperatureStepC = 1.0
```

요구사항:

- `Config`, `EditAnywhere`, 적절한 category/meta와 Blueprint read-only getter를 제공한다.
- 기존 bath usability threshold property와 config key를 유지한다.
- finite, min<=ambient<=max, step>0을 검증한다.
- invalid config는 안전한 기본값으로 정규화하고 로그/ensure를 남기되 gameplay state에 NaN을 넣지 않는다.
- target quantization은 ambient를 anchor로 한다.
- runtime hot reload는 구현하지 않는다.

## `UBathWaterOperationsSubsystem`

`UWorldSubsystem`으로 구현하고 Tick하지 않는다.

### Registry

- utility capacity component와 bath condition component를 weak identity로 등록한다.
- 같은 object의 중복 register/unregister가 합계나 revision을 중복 변경하지 않게 한다.
- invalid weak entries를 안전하게 prune한다.
- capacity total, bath demand와 topology/capacity revision을 제공한다.
- silent stage/rollback과 최종 publication을 구분해 placement transaction에서 delegate storm이 생기지 않게 한다.
- delegate callback 재진입을 고려해 publication 중 registry 변경이 유실되지 않게 한다.

### Demand

각 bath의 수요는 component authoring 값을 사용한다.

```text
Circulation = MaxCirculationDemandPoints * CirculationPercent / 100
Heating = max(TargetC - AmbientC, 0) * HeatingDemandPointsPerC
Cooling = max(AmbientC - TargetC, 0) * CoolingDemandPointsPerC
```

- water amount와 actual temperature는 demand에 영향을 주지 않는다.
- heating과 cooling을 동시에 예약하지 않는다.
- aggregate는 bath identity별 정확히 한 항목을 가진다.

### Setting Request

순환도/목표 온도 변경은 subsystem request 하나를 통해 처리한다.

1. target bath와 proposed 값을 검증한다.
2. 범위/step으로 정규화한다.
3. 현재 bath demand를 빼고 candidate demand를 더한 aggregate를 계산한다.
4. capacity 이하인 최대 committed 값을 결정한다.
5. bath component state와 ledger를 한 commit 경계에서 갱신한다.
6. result에 requested, committed, 제한 종류와 부족 포인트를 반환한다.

필수 정책:

- 값을 낮추는 방향은 world가 이미 deficit이어도 허용한다.
- target이 ambient를 건너면 기존 종류 수요를 0으로 만든 뒤 반대 종류 capacity로 제한한다.
- 다른 bath 설정을 변경하거나 비례 배분하지 않는다.
- 실패 시 bath/ledger를 원상 유지한다.
- total이 0이면 circulation은 0을 넘지 못하고 target은 해당 방향으로 ambient를 벗어나지 못한다.

### Availability And Removal

- kind별 `IsDemandSatisfied`와 bath effect availability를 제공한다.
- circulation deficit은 해당 circulation을 요구하는 bath의 cleaning과 thermal을 모두 중지한다.
- heating/cooling deficit은 해당 thermal 방향만 중지한다.
- 정상 provider 회수 전 post-removal total이 current used 이상인지 검사한다.
- 부족하면 kind, 부족 포인트와 interaction 실패 이유를 반환한다.
- 예상치 않은 provider 소실은 bath 설정을 바꾸지 않고 deficit을 publish한다.
- provider가 돌아와 aggregate가 다시 충분하면 effect가 자동 재개된다.

## `UBathWaterUtilityCapacityComponent`

필수 property:

```text
CapacityKind: EditDefaultsOnly
CapacityPoints: EditAnywhere, default 100
```

- 종류는 Blueprint class 단위로 고정하고 instance에서 바꾸지 않는다.
- 용량은 Blueprint default/Level instance override를 허용한다.
- 음수·NaN을 등록하지 않는다.
- registration state를 idempotent하게 추적하되 aggregate 정본은 subsystem이다.
- component가 임의 BeginPlay world scan이나 Tick을 하지 않는다.

## `ABathWaterUtilityFacilityActor`

독립 actor로 구현한다. `ATowelProcessingMachineActor`와 기존 placeable actors의 계약을 실제 Source에서 대조해 필요한 interface를 동일하게 구현한다.

필수 composition:

- scene root
- package/free-world physics primitive
- `VisualMesh`
- `PlacementFootprint`
- `UFacilityPlacementComponent`
- `UBathWaterUtilityCapacityComponent`

필수 동작:

- placed 상태에서만 provider를 등록한다.
- packaged item/preview/staged 상태는 total에 포함하지 않는다.
- customer facility subsystem과 use slot에 등록하지 않는다.
- primary direct action이나 On/Off를 제공하지 않는다.
- 기존 supplemental Q recovery prompt/hold/transaction을 사용한다.
- recovery query는 post-removal capacity를 먼저 확인한다.
- staged unregister, rollback re-register, commit publication이 정확히 한 번 일어난다.
- pre-placed BeginPlay와 unexpected EndPlay를 idempotent하게 처리한다.
- collision/navigation/held/drop/preview는 Placement와 Physical Carry 계약을 그대로 사용한다.

세 설비 종류를 native subclass 세 개로 만들 필요는 없다. Editor에서 같은 native parent의 세 Blueprint가 `CapacityKind`와 visuals/definition을 authoring할 수 있게 한다.

## Utility Typed Payload

`UBathWaterUtilityPlacementInstanceData`를 `UFacilityPlacementInstanceData`에서 파생한다.

- per-instance capacity를 export/import한다.
- payload kind와 target class-default kind가 다르면 fail closed한다.
- invalid capacity는 restore하지 않는다.
- install transaction이 실패하면 item/payload를 보존한다.
- placed actor domain/collision commit 뒤에만 held item을 제거한다.
- actual temperature, contamination이나 bath setting은 이 payload에 넣지 않는다.

## `UBathWaterConditionComponent`

`ABathhouseBathFacilityActor`의 native default subobject로 생성한다.

### Authoring Properties

```text
MaxCirculationDemandPoints = 100
HeatingDemandPointsPerC = 5
CoolingDemandPointsPerC = 5
CleaningRateAtFullCirculationPercentPointsPerSecond = 1
ContaminationPerBatherPercentPointsPerSecond = 0.1
MaxTargetControlRateCPerSecond = 0.5
NaturalReturnRateCPerSecond = 0.05
TemperatureEpsilonC = small positive default
```

- balance 값은 `EditAnywhere`로 bath Blueprint default와 Level instance override를 허용한다.
- epsilon은 `EditDefaultsOnly`로 둔다.
- 모든 demand/rate는 finite, nonnegative validation을 한다.
- thermal threshold percent는 저장 property가 아니라 `NaturalReturnRate / MaxTargetControlRate * 100` 파생 getter다.
- max rate가 0인 경우 divide-by-zero 없이 target movement impossible 상태를 반환한다.

### Runtime State

- circulation 0%
- target ambient
- actual ambient
- contamination 0%
- weak active-bather identity set
- recovery freeze snapshot/guard

초기화 규칙:

- pre-placed bath와 새 placement 모두 global ambient/0/clean으로 시작한다.
- 완전 배수 시 actual ambient, contamination 0으로 reset한다.
- 회수 payload에 runtime condition state를 저장하지 않는다.
- preview/staged/domain-inactive bath는 register/Tick하지 않는다.

### Tick

- water amount>0이고 recovery freeze가 아닐 때만 Tick한다.
- `UBathWaterStateComponent` 뒤에 실행되도록 tick prerequisite를 설정한다.
- flow mixing을 먼저 반영한 뒤 contamination과 thermal update를 수행한다.
- snapshot/result delegate는 값이 실제 변할 때만 발생시킨다.
- Actor Tick을 새로 켜지 않는다.

## `UBathWaterStateComponent` Flow Sample

기존 water amount owner를 유지하고 새 native flow-step delegate만 추가한다.

sample 최소 필드:

```text
PreviousAmount
IncomingAmount
OutgoingAmount
CurrentAmount
DeltaSeconds
```

정확성 조건:

- incoming은 1.0 clamp 뒤 실제로 받아들인 normalized water amount다.
- outgoing은 0 clamp 뒤 실제로 제거된 amount다.
- fill/drain이 동시에 같아 net amount가 0이어도 두 값과 sample을 보존한다.
- 이 경우 mixing을 위해 water Tick을 중지하지 않는다.
- 기존 amount-changed delegate는 amount가 실제 달라질 때만 발생한다.
- endpoint auto-close와 control/Niagara 의미를 바꾸지 않는다.
- freeze 중에는 flow sample과 변화가 없다.

## Mixing

급수는 global ambient, contamination 0인 물이다.

flow step에서 retained old amount와 accepted incoming amount로 mass-weighted mixture를 계산한다.

```text
RetainedOld = max(PreviousAmount - OutgoingAmount, 0)
NewAmount = RetainedOld + IncomingAmount
Temperature =
  (RetainedOld * OldTemperature + IncomingAmount * Ambient) / NewAmount
Contamination =
  (RetainedOld * OldContamination) / NewAmount
```

- 실제 state component의 simultaneous integration order와 clamp 결과에 맞춰 amount conservation을 보장한다.
- drain-only는 remaining water의 temperature/concentration을 바꾸지 않는다.
- empty 결과는 ambient/0으로 reset한다.
- net-zero exchange도 incoming>0이면 temperature/contamination이 ambient/clean 방향으로 변한다.
- 0 division, negative retained amount와 accumulated float drift를 방어한다.

## Temperature Solver

### Effect Gate

active target force는 다음이 모두 참일 때만 계산한다.

- water amount>0
- circulation percent>0
- aggregate circulation capacity가 circulation demand를 충족
- target>ambient이면 aggregate heating capacity가 heating demand를 충족
- target<ambient이면 aggregate cooling capacity가 cooling demand를 충족

natural return은 water amount>0, not frozen이면 항상 존재한다.

### Force Rules

```text
ActiveRate = MaxTargetControlRateCPerSecond * CirculationPercent / 100
NaturalRate = NaturalReturnRateCPerSecond
```

- active는 target 방향, natural은 ambient 방향이다.
- 같은 방향이면 합하고 반대면 뺀다.
- exact ambient에서 natural alone은 위치를 바꾸지 않지만 ambient 밖으로 출발하는 active에 저항한다.
- exact ambient에서 active<=natural이면 ambient에 고정하고, active>natural일 때 차이만큼 target 방향으로 이동한다.
- exact target에서 active>=natural이면 target을 유지하고, 부족하면 ambient 쪽으로 이탈한다.
- target==ambient이면 active demand 0, natural로 actual을 ambient에 수렴시킨다.
- active가 capacity 때문에 suspended면 natural만 적용한다.

### Numeric Integration

- target/ambient 경계를 overshoot하지 않는다.
- 한 frame에 경계를 통과할 수 있으면 경계까지 소비한 시간과 남은 시간을 나눠 힘 방향을 다시 계산한다.
- epsilon 안에서는 해당 경계에 snap한다.
- exact boundary에서 frame마다 부호가 반전하는 jitter를 만들지 않는다.
- 큰 DeltaSeconds에서도 bounded loop 또는 닫힌 형태로 안정적으로 끝낸다. 무한 while을 만들지 않는다.
- 기본값의 파생 threshold가 10%인지 test하되 판정식은 authoring 값을 사용한다.

thermal status를 snapshot에 계산해 UI가 같은 규칙을 재구현하지 않게 한다.

## Contamination Solver

```text
IncreaseRate = ActiveBatherCount * ContaminationPerBatherPercentPointsPerSecond
CleaningRate = CleaningRateAtFullCirculationPercentPointsPerSecond * CirculationPercent / 100
Delta = (IncreaseRate - EffectiveCleaningRate) * DeltaSeconds
```

- 0~100으로 clamp한다.
- circulation deficit이면 effective cleaning은 0이다.
- heating/cooling deficit은 cleaning에 영향이 없다.
- active-bather weak set에서 invalid entries를 prune한다.
- 예약, 이동, 입장/퇴장, 탐색, knockdown은 active count에 포함하지 않는다.
- contamination이 customer behavior를 바꾸지 않는다.

## Customer Actual-Bather Integration

기존 `UCustomerSessionComponent` actual segment 경계만 확장한다.

- `BeginActualBathSegment()`가 성공하는 transaction 안에서 현재 bath condition에 session identity를 등록한다.
- condition 등록 실패 시 actual segment를 active로 commit하지 않는다.
- `EndActualBathSegment()`는 reason과 관계없이 identity를 idempotent하게 unregister한다.
- dwell complete, threshold invalidation, stay timeout, knockdown, StateTree exit, technical abort와 EndPlay 모든 기존 경로를 보존한다.
- 중복 begin/end가 active count를 복제하거나 음수로 만들지 않는다.
- StateTree asset과 task topology는 변경하지 않는다.

## Bath Actor Placement/Recovery Integration

`ABathhouseBathFacilityActor`가 condition component를 조립한다.

- placed domain 활성화 시 condition/demand를 subsystem에 등록한다.
- recovery hold begin은 water/control freeze와 condition freeze를 같은 guard로 commit한다.
- hold 동안 demand reservation은 제거하지 않는다.
- cancel은 water/control/condition snapshot과 Tick을 정확히 복원한다.
- actual recovery stage에서 condition registry/demand를 silent unregister한다.
- rollback은 condition registry와 snapshot을 정확히 한 번 복원한다.
- success/EndPlay는 stale delegate, Tick과 active-bather reference를 정리한다.
- recovery 조건 자체는 기존 all slots available + exact water 0을 유지한다.

## Computer Actor Integration

`ABathhouseComputerActor`에 다음을 추가한다.

- `ManagedBathPlacementZone`: `EditInstanceOnly`, Blueprint read-only reference
- management root context initialization helper

`BeginPlay()`에서 `ScreenWidget->InitWidget()` 뒤 실제 widget이 management root인지 검사하고 world subsystem과 Zone을 주입한다. sample widget이면 기존 동작을 그대로 유지한다.

- Widget이 world를 검색해 Zone을 고르지 않는다.
- invalid Zone이면 crash하지 않고 unavailable presentation을 설정한다.
- focus/session/pointer/view/AA lifecycle은 변경하지 않는다.
- `PrimaryUseAction` canonical LMB와 owner priority를 변경하지 않는다.
- `ComputerClickAction` fallback을 제거하지 않는다.

## Native Management Widget Hierarchy

다음 native base를 구현한다.

```text
UBathWaterManagementScreenWidget
  UBathWaterCapacitySummaryWidget
  UBathWaterMapWidget
    dynamic UBathWaterBathTileWidget
  UBathWaterDetailWidget
```

### Root

- subsystem/Zone context를 명시적으로 받는다.
- child Widget을 최소 필수 `BindWidget`으로 가진다.
- selected bath는 weak reference다.
- construct/destruct/context 교체에서 delegate를 대칭 구독/해제한다.
- lightweight snapshot은 `NativeTick` 또는 기존 lifecycle에 맞는 polling으로 갱신할 수 있다.
- topology revision이 바뀔 때만 map tile set을 rebuild한다.
- 연속 값은 cached presentation과 다를 때만 child에 적용한다.
- selected bath가 제거되면 selection/detail을 지운다.
- focus-out은 widget을 파괴하지 않으므로 state를 유지한다.

### Capacity Summary

- circulation/heating/cooling별 used, total, deficit만 표시한다.
- formatting/visibility는 C++이 적용하고 WBP는 layout/style만 담당한다.

### Map

- injected PlacementZone world bounds를 지도 rect로 사용한다.
- world +X -> screen up, world +Y -> screen right다.
- Zone 안에 있는 registered bath만 표시한다.
- bath footprint 네 모서리를 투영해 size와 yaw를 표시한다.
- utility와 다른 facility는 표시하지 않는다.
- dynamic tile class를 Editor에서 지정할 수 있게 하되 invalid class를 안전 처리한다.

### Tile

- bath 이름, actual temperature, contamination, thermal status를 표시한다.
- click은 bath selection intent를 root에 전달한다.
- actor를 직접 mutate하거나 subsystem을 world에서 찾지 않는다.

### Detail

- water amount, actual/target temperature, contamination, circulation, kind별 demand, thermal status와 derived threshold를 표시한다.
- circulation slider와 target slider intent를 root/subsystem request로 전달한다.
- committed value와 failure/limited result를 화면에 반영한다.
- slider가 condition property를 직접 set하지 않는다.
- target slider는 global min/max/step, circulation은 0~100 연속 범위를 사용한다.

### BindWidget Naming

명확하고 안정적인 필수 이름을 header에 정의한다. 기존 sample widget의 `TestButton`, `ClickResultText`는 변경하지 않는다. 구현 완료 뒤 정확한 새 이름·타입을 결과 보고와 `.md/USER_UNREAL.md`에 넘길 수 있게 목록화한다.

Widget Blueprint event 없이 native 로직이 동작해야 한다. Blueprint hook은 선택적 시각 효과만 허용한다.

## Logging

새 log category를 하나 두거나 기존 bath log 체계와 충돌하지 않는 category를 사용한다.

- Log: provider/bath structural register/unregister, unexpected capacity deficit/resume
- Verbose: setting request 제한, demand/total 변화, selection/topology refresh
- Warning: invalid authoring, missing Zone, recoverable registration/import failure
- Error: duplicate identity invariant, mismatched payload kind, rollback 실패

매 Tick 수온/오염도와 UI polling을 기본 Log에 출력하지 않는다. 테스트가 로그 문자열 자체에 의존하지 않게 한다.

## Native Automation

기존 test helper 패턴을 재사용하고 가능한 순수 계산은 world 없는 testable helper로 분리한다. 최소 검증 목록:

### Settings And Demand

- default ambient/range/step와 invalid 값 방어
- target step quantization이 ambient anchor를 사용
- circulation/heating/cooling demand 공식
- empty bath도 demand를 예약
- heating/cooling 상호 배제

### Registry And Requests

- provider 중복 등록/해제 no-op
- 종류별 total 독립 합산
- selected bath replacement aggregate
- capacity 이하 request commit
- 초과 request 최대 허용값 clamp와 failure details
- decrease always allowed
- ambient cross-over에서 old kind release 후 new kind 제한
- invalid/NaN request rollback

### Recovery And Payload

- post-removal capacity 허용/거부
- 거부 시 actor/provider/item 상태 불변
- silent stage + rollback exact once
- unexpected provider loss preserves settings and suspends effects
- capacity restore auto resumes
- payload capacity round trip
- mismatched kind/invalid capacity fail closed

### Water Mixing

- fill only ambient/clean weighted mixture
- drain only temperature/concentration preservation
- simultaneous equal fill/drain에서 amount 불변 + mixing 발생
- endpoint accepted inflow/outflow clamp
- complete drain ambient/clean reset
- recovery freeze 동안 no flow/no condition change

### Temperature

- 기본 derived threshold 10%
- exact ambient에서 10% 이하 유지, 초과 출발
- ambient와 target 사이에서 below/at/above threshold 방향
- exact target에서 충분/부족 active force 유지/이탈
- target==ambient natural return
- same-direction active+natural 합산
- circulation/heating/cooling deficit gate
- 큰 DeltaSeconds 경계 통과와 epsilon non-jitter
- max active rate 0과 natural rate 0 경계

### Contamination And Customer

- active-bather 수만 증가율에 포함
- weak identity 중복 begin/end no-op
- cleaning rate circulation 비례
- circulation deficit은 cleaning 0, heat/cool deficit은 cleaning 유지
- 0~100 clamp
- knockdown/abort/EndPlay cleanup

### Computer UI Contract

- computer management root context injection
- sample widget compatibility
- missing Zone unavailable state
- world +X up/+Y right projection
- footprint size/yaw projection
- non-bath filtering
- topology revision에서만 tile rebuild
- removed selected bath clears detail
- slider가 subsystem transaction 결과만 반영
- focus-out/re-entry가 widget instance state를 유지

### Regression

- 기존 Bath Water automation
- Placement/recovery automation
- Computer automation
- Combat/PrimaryUse routing automation
- Customer BathLoop automation

새 테스트를 통과시키기 위해 기존 테스트의 유효 계약을 약화하거나 삭제하지 않는다.

## 빌드와 검증 순서

1. 신규/수정 Source를 정적 검토한다.
2. Unreal Editor가 실행 중인지 확인하고 실행 중이면 Live Coding/파일 잠금 위험을 보고한 뒤 안전한 경로를 사용한다.
3. 프로젝트가 정한 `Build.bat`/UBT 경로로 `BathhouseSimEditor Win64 Development`를 빌드한다.
4. 새 `BathWaterOperations` focused automation을 실행한다.
5. 관련 BathWater, Placement, Computer, Customer와 PrimaryUse regression automation을 실행한다.
6. 실패는 로그와 source cause를 분석해 수정하고 재실행한다.
7. Source 구현 뒤 `.md/USER_UNREAL.md`의 기존 관련 지시를 삭제하지 말고, Editor 단계에 필요한 신규 Blueprint/WBP/Level authoring과 정확한 reflected 이름을 추가한다.

문서-only architecture diff를 구현 단계에서 임의로 되돌리지 않는다. 구현으로 설계의 실제 이름이나 경계가 달라졌다면 관련 architecture 문서와 구현 프롬프트를 최소 범위에서 동기화한다.

## 완료 보고

다음을 간결하게 보고한다.

- 추가·수정한 Source 파일과 책임
- capacity/setting/recovery transaction의 실제 구현 경계
- temperature exact-ambient 저항과 파생 threshold 구현 방식
- net-zero exchange mixing 처리
- actual-bather cleanup 처리
- management Widget의 정확한 `BindWidget` 이름·타입
- 실행한 build/test 명령과 결과
- 남은 코드 위험 또는 미검증 사항
- Unreal Editor 단계에서 필요한 asset, Blueprint parent/component/값, WBP hierarchy와 Level reference

Source, build와 native automation이 끝났더라도 Content 작업은 완료로 간주하지 않는다. Editor 변경은 별도 코드 리뷰와 Unreal 단계에서 수행한다.
