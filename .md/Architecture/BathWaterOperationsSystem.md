# Bath Water Operations System

## Implementation Status

- 상태: 기존 Source 수직 및 deficit/registry/actor-transaction 구현은 완료. 2026-09-24 Utility Labor 수직 변경으로 Installed/Active 집계와 operation-aware provider 연결을 Source에 반영했으며, 전체 빌드/automation과 Editor 수용 검증은 미실행이다.
- 정본: 기능 `.md/PROMPT_ARCHITECTURE.md`, Editor 조사 `.md/REPORT_UNREAL_DISCOVERY.md`, 기존 수위·급수·배수 `BathWaterSystem.md`
- 이 문서는 순환·가열·냉각 용량과 수온·오염도 domain만 소유한다.
- 2026-09-24 Utility Labor Source: `TotalPoints`는 Installed, `ActivePoints`는 실제 가동 provider, `DeficitPoints`는 Active shortfall, `InstalledDeficitPoints`는 Installed shortfall이다. setting request와 회수 검증은 기존처럼 Installed를 기준으로 하며, 효과 충족은 Active를 기준으로 한다. 세부 정본은 [UtilityLaborSystem.md](UtilityLaborSystem.md)다.

## Target Source Scope

```text
Source/BathhouseSim/Public|Private/Facility/BathWaterOperationsTypes.*, BathWaterOperationsSubsystem.*
Source/BathhouseSim/Public|Private/Facility/BathWaterUtilityCapacityComponent.*, BathWaterUtilityFacilityActor.*, BathWaterUtilityPlacementInstanceData.*
Source/BathhouseSim/Public|Private/Facility/BathWaterConditionComponent.*
Source/BathhouseSim/Private/Tests/BathWaterOperationsAutomationTests.cpp
```

기존 파일의 필요한 확장은 다음으로 제한한다.

- `BathWaterSettings.h/.cpp`: 프로젝트 전역 실온·목표 범위·목표 step
- `BathWaterStateComponent.h/.cpp`: 한 integration step의 실제 유입·유출 sample 발행
- `BathhouseBathFacilityActor.h/.cpp`: condition component 조립과 회수 freeze/rollback 연결
- `CustomerSessionComponent.h`와 `CustomerSessionBath.cpp`: 실제 입욕자 identity 등록·해제
- `FirstPersonCharacter`는 현재 `PrimaryUseAction` routing을 유지하며 변경하지 않는 것을 기본으로 한다.

컴퓨터와 native Widget Source 범위는 [BathWaterManagementUISystem.md](BathWaterManagementUISystem.md)가 소유한다.

## Responsibilities

- 설치된 순환기·보일러·쿨러의 종류별 총용량과 설치 욕탕의 종류별 요구량 원장
- 욕탕 하나의 순환도, 목표/실제 수온, 오염도와 실제 입욕자 집합
- 설정 변경과 설비 회수의 원자적 용량 검증
- 정상 회수, rollback과 예기치 않은 Actor 소실의 서로 다른 처리
- 급수 유입에 의한 온도·오염도 혼합과 시간 기반 수온·오염도 변화
- 컴퓨터에 전달할 immutable snapshot, topology/capacity revision과 실패 결과

이 시스템은 기존 물 양, 밸브·레버, 수면 높이, 임계 수위와 customer StateTree 진행을 다시 소유하지 않는다.

## State Owners

| 상태 | 정본 owner | 비고 |
|---|---|---|
| 실온·목표 범위·목표 step | `UBathWaterSettings` | Project Settings |
| 설비 종류·인스턴스 용량 | `UBathWaterUtilityCapacityComponent` | Blueprint default와 Level instance override |
| 설치 설비 registry와 총용량 | `UBathWaterOperationsSubsystem` | weak identity 기반 |
| 설치 욕탕 registry와 요구량 ledger | `UBathWaterOperationsSubsystem` | 욕탕 identity별 한 항목 |
| 순환도·목표/실제 수온·오염도 | `UBathWaterConditionComponent` | 욕탕 인스턴스 상태 |
| 실제 입욕자 | `UBathWaterConditionComponent`의 weak session set | raw count 금지 |
| 물 양·유입·유출 제어 | `UBathWaterStateComponent` | 기존 정본 유지 |
| 선택 욕탕과 화면 presentation cache | root computer widget | gameplay 정본 아님 |
| 컴퓨터가 관리할 Zone | `ABathhouseComputerActor` instance reference | world scan 대체 |

Widget, Character와 Blueprint Event Graph는 용량·수온·오염도를 직접 변경하지 않는다.

## Global Settings

기존 `UBathWaterSettings`에 다음 `Config`, `EditAnywhere` 값을 추가한다.

| 값 | 기본값 | 검증 |
|---|---:|---|
| `AmbientTemperatureC` | `20.0` | finite |
| `MinTargetTemperatureC` | `10.0` | finite, Ambient 이하 |
| `MaxTargetTemperatureC` | `50.0` | finite, Ambient 이상 |
| `TargetTemperatureStepC` | `1.0` | finite, `> 0` |

- 위치: Project Settings > Game > Bath Water
- 런타임 설정 요청은 범위를 clamp하고 실온을 기준으로 가열/냉각 방향을 계산한다.
- 목표값은 실온을 anchor로 step quantize해 실온을 지나갈 때 양쪽 방향이 대칭이 되게 한다.
- 잘못된 config는 ensure/log 뒤 안전한 기본값으로 정규화하고 NaN을 domain 상태에 저장하지 않는다.
- 전역 설정은 새 Level 시작과 재설치 초기값의 기준이다. 런타임 중 config hot reload는 범위 밖이다.

## Shared Types And Result Contract

`BathWaterOperationsTypes.h`는 최소한 다음 native/reflected 값을 제공한다.

- `EBathWaterCapacityKind`: `Circulation`, `Heating`, `Cooling`
- `EBathWaterThermalStatus`: `Empty`, `ReturningToAmbient`, `Stalled`, `MovingToTarget`, `MaintainingTarget`, `SuspendedByCapacity`
- `EBathWaterRequestFailure`: invalid target, invalid number, insufficient circulation/heating/cooling capacity, unavailable bath
- `FBathWaterCapacitySnapshot`: `UsedPoints` 예약, `TotalPoints` 설치, `ActivePoints` 가동, `InstalledDeficitPoints` 설치 부족, `DeficitPoints` 가동 부족과 revision. `IsSatisfied()`는 가동 기준이다.
- `FBathWaterBathSnapshot`: weak bath identity, world transform/footprint, water/setting/condition 값, 요구량, 상태와 revision
- `FBathWaterSettingRequestResult`: requested, committed, limited kind, required additional points와 failure

표시 문자열과 색상은 UI presentation 책임이다. Domain은 enum과 수치만 반환한다.

## `UBathWaterOperationsSubsystem`

`UWorldSubsystem`이며 Tick하지 않는다.

### Registry

- utility provider와 bath condition을 `TWeakObjectPtr` identity로 등록한다.
- 같은 identity의 중복 등록은 합산하지 않는다.
- invalid weak entry는 조회/변경 경계에서 정리한다.
- provider register/unregister와 bath register/unregister는 silent stage와 publication을 분리할 수 있어야 한다.
- 총용량과 요구량은 registry 정본에서 계산하거나 검증 가능한 cache로 유지한다. Actor와 Widget의 복제 count는 금지한다.

### Setting Transaction

순환도 또는 목표 수온 요청은 선택된 욕탕 한 항목을 후보값으로 교체한 aggregate를 계산한다.

```text
CandidateUsed(kind)
  = CurrentUsed(kind)
  - CurrentBathDemand(kind)
  + CandidateBathDemand(kind)
```

- candidate가 설치 total 이하인 범위만 commit한다. 가동 잔량 부족은 설정 상한을 낮추지 않는다.
- slider 요청이 초과하면 남은 용량으로 가능한 최대 step/percent를 계산해 제한된 값을 한 번 commit하고 결과에 제한 이유를 넣는다.
- 값을 낮추는 방향은 현재 world가 deficit 상태여도 항상 허용한다.
- 목표가 실온을 건너면 이전 종류 요구량을 먼저 0으로 만들고 반대 종류의 남은 용량으로 계속 제한한다.
- 다른 욕탕 값을 자동 변경하거나 capacity를 비례 배분하지 않는다.
- commit 전 validation 실패 시 condition과 ledger 모두 이전 값을 유지한다.
- 기존 승인 target보다 바깥 방향으로 증가하는 요청이 deficit에 막히면 heating은 현재 target을 하한, cooling은 현재 target을 상한으로 보존한다.
- `CommittedValue`가 현재값과 같으면 setter, data revision과 publication을 모두 생략한다. 제한 부족량은 요청 candidate aggregate에서 total을 뺀 정확한 값이다.

### Demand Formula

욕탕 component의 authoring 값을 사용한다.

```text
CirculationDemand = MaxCirculationDemandPoints * CirculationPercent / 100
HeatingDemand = max(TargetC - AmbientC, 0) * HeatingDemandPointsPerC
CoolingDemand = max(AmbientC - TargetC, 0) * CoolingDemandPointsPerC
```

물 양, 현재 수온과 유효 효과 여부는 예약량을 줄이지 않는다.

### Provider Removal

- 정상 Q 회수 전 `CanRemoveProvider()`가 `TotalAfterRemoval >= CurrentUsed`를 확인한다.
- 부족하면 회수 hold를 시작/완료하지 않고 종류와 부족 포인트를 interaction failure로 반환한다.
- staged recovery는 provider를 silent unregister한다. rollback은 동일 identity를 한 번 복원하고 publication은 transaction 종료에 한 번만 한다.
- 예기치 않은 EndPlay/Destroy는 설정을 보존한 채 provider를 제거하고 deficit snapshot을 publish한다.
- provider 변화는 capacity/data revision만, bath identity 변화는 topology/data revision을 변경한다. invalid weak entry prune도 같은 revision/publication 규칙을 따른다.
- deficit 종류를 요구하는 모든 욕탕은 해당 효과를 중지한다. circulation deficit은 그 욕탕의 정화와 가열·냉각 효과도 함께 중지한다.
- 용량이 다시 충분해지면 별도 사용자 입력 없이 자동 재개한다.

## Utility Facility Actors

### `UBathWaterUtilityCapacityComponent`

- `CapacityKind`: `EditDefaultsOnly`, Blueprint class별 고정
- `CapacityPoints`: `EditAnywhere`, Blueprint default와 Level instance override 허용, 기본 `100`
- 음수·NaN은 등록할 수 없고 명확한 validation error를 기록한다.
- component는 자기 값을 소유하지만 world aggregate mutation은 subsystem API로만 수행한다.
- 정격은 설치 합계에, 명시적으로 주입된 Operation이 양수인 정격은 가동 합계에 기여한다. 보일러·쿨러·순환기 모두 Operation 필수이며 Operation 없는 항상 공급 경로는 없다([UtilityLaborSystem.md](UtilityLaborSystem.md)).

### `ABathWaterUtilityFacilityActor`

기존 `ATowelProcessingMachineActor`의 placement/recovery 구조를 참고하는 독립 Actor다.

- `IPlaceableFacility`, `IPhysicalCarryable`, 기존 primary/supplemental interaction 계약 구현
- scene root, packaged item용 physics primitive, `VisualMesh`, `PlacementFootprint`, `UFacilityPlacementComponent`, capacity component 소유
- base는 공통 배치·회수를 유지한다. 보일러만 `ABathWaterBoilerFacilityActor` 자식으로 분리하여 가동·투입·계기를 조립한다. 순환기·쿨러 노동은 후속 단계다.
- customer `UBathhouseFacilitySubsystem`에 사용 슬롯 설비로 등록하지 않는다.
- 직접 On/Off 또는 E primary 행동을 제공하지 않는다. Q Hold recovery만 기존 prompt 경로로 노출한다.
- placed/preview/packaged 상태, collision/navigation과 item 전환은 `PlacementSystem.md` 계약을 그대로 따른다.

### Typed Placement Payload

`UBathWaterUtilityPlacementInstanceData : UFacilityPlacementInstanceData`는 회수 후 재설치에 필요한 인스턴스 capacity를 보존한다.

- payload의 kind와 target Actor class default kind가 다르면 설치를 거부한다.
- 용량은 유한한 음이 아닌 값만 restore한다.
- 설비 item은 기존 전용 회수 item 외형·Held 계약을 사용하며 capacity gameplay owner가 되지 않는다.
- placed Actor가 성공적으로 등록된 뒤에만 item 원본 제거를 commit한다.
- 노동 설비는 optional 가동 잔량 payload를 추가한다. Hold 동안 감소, 성공 시점 잔량 보존, packaged 감소 중지와 rollback 시 비-rewind는 `UtilityLaborSystem.md`를 따른다.

## `UBathWaterConditionComponent`

`ABathhouseBathFacilityActor`에 native default subobject로 조립한다.

### Authoring Values

| 값 | 기본값 | 노출 |
|---|---:|---|
| `MaxCirculationDemandPoints` | `100` | EditAnywhere, per bath instance |
| `HeatingDemandPointsPerC` | `5` | EditAnywhere, per bath instance |
| `CoolingDemandPointsPerC` | `5` | EditAnywhere, per bath instance |
| `CleaningRateAtFullCirculationPercentPointsPerSecond` | `1` | EditAnywhere, per bath instance |
| `ContaminationPerBatherPercentPointsPerSecond` | `0.1` | EditAnywhere, per bath instance |
| `MaxTargetControlRateCPerSecond` | `0.5` | EditAnywhere, per bath instance |
| `NaturalReturnRateCPerSecond` | `0.05` | EditAnywhere, per bath instance |
| `TemperatureEpsilonC` | small positive implementation default | EditDefaultsOnly |

- rate/demand 값은 finite이고 음이 아니어야 한다.
- 첫 수직 구현에서는 위 기본값을 사용하되 balance 변경은 Blueprint/instance authoring으로 가능하다.
- 파생 임계 순환도는 별도 저장하지 않는다.

```text
ThermalThresholdPercent
  = NaturalReturnRate / MaxTargetControlRate * 100
```

최대 조절력이 0이면 목표 방향 이동은 불가능하며 UI가 정체/불가 상태를 표시한다.

### Runtime State

- `CirculationPercent`: 초기 `0`
- `TargetTemperatureC`: 생성·재설치 시 현재 global ambient
- `ActualTemperatureC`: 물이 없을 때 ambient, 첫 물 유입 전 ambient
- `ContaminationPercent`: 생성·재설치와 완전 배수 시 `0`
- active bathers: weak `UCustomerSessionComponent` set
- recovery freeze snapshot

재설치 payload에는 수온·오염도·설정을 저장하지 않는다. 새로 설치된 욕탕은 승인 계약대로 초기화한다.

### Tick Gate And Order

- 물 양이 0보다 크고 recovery freeze가 아닐 때만 condition Tick을 사용한다.
- water state component 이후 실행되도록 tick prerequisite를 둔다.
- flow sample에 의한 혼합을 먼저 적용하고, 그 step의 실제 입욕자 오염 증가·순환 정화·온도 힘 계산을 적용한다.
- `DeltaSeconds <= 0`, invalid owner와 invalid setting은 mutation 없이 안전 종료한다.

## Water Flow Integration

`UBathWaterStateComponent`는 물 양 정본을 유지하며 매 integration step 다음 native sample을 발행한다.

```text
FBathWaterFlowStep
  PreviousAmount
  IncomingAmount
  OutgoingAmount
  CurrentAmount
  DeltaSeconds
```

- `IncomingAmount`는 clamp로 실제 용기에 받아들인 양이다. 만수 밖으로 버려진 가상 급수는 포함하지 않는다.
- `OutgoingAmount`는 실제 제거된 양이다.
- 급수와 배수가 같은 양이어도 두 값 모두 0이 아니므로 sample을 발행하고 Tick을 유지한다.
- 기존 amount changed delegate는 순수 수위 변경에만 사용하며 의미를 바꾸지 않는다.

급수는 ambient, contamination 0인 물로 비례 혼합한다.

```text
MixedTemperature =
  (RetainedOldWater * OldTemperature + IncomingWater * AmbientTemperature)
  / CurrentWaterAfterFlow

MixedContamination =
  (RetainedOldWater * OldContamination)
  / CurrentWaterAfterFlow
```

배수만 일어난 경우 남은 물의 온도·오염 농도는 바뀌지 않는다. 완전 배수는 actual을 ambient, contamination을 0으로 reset한다.

## Temperature Integration

### Effect Availability

능동 목표 조절은 다음 조건을 모두 만족할 때만 존재한다.

- 물 양 `> 0`
- circulation percent `> 0`
- 전체 circulation 가동 capacity가 전체 circulation demand 이상
- 목표가 ambient보다 높으면 전체 heating 가동 capacity가 demand 이상
- 목표가 ambient보다 낮으면 전체 cooling 가동 capacity가 demand 이상

정화는 circulation 조건만 요구한다. 자연 복귀는 물이 있고 recovery freeze가 아니면 항상 활성이다.

### Competing Forces

```text
ActiveRate = MaxTargetControlRate * CirculationPercent / 100
NaturalRate = NaturalReturnRate
```

- 실제 수온이 ambient와 target 사이면 active는 target 방향, natural은 ambient 방향이다.
- 방향이 같으면 합하고 반대면 뺀다.
- exact ambient에서 natural 단독은 이동시키지 않지만 ambient 밖으로 나가려는 active에 반대 저항으로 작용한다.
- 따라서 exact ambient에서도 `ActiveRate <= NaturalRate`면 ambient 유지, 더 클 때만 차이만큼 target 방향으로 출발한다.
- exact target에서는 active가 natural을 상쇄할 수 있다. active가 부족하면 ambient 방향으로 이탈한다.
- target이 ambient이면 active demand가 0이고 natural만 actual을 ambient로 복귀시킨다.
- ambient/target을 통과하는 큰 frame은 경계까지 시간을 소비한 뒤 남은 시간을 새 방향으로 재평가한다.
- epsilon 안에서는 경계에 snap하고 동일 frame/다음 frame에서 부호가 반복 반전하지 않게 한다.

기본값에서 파생 임계치는 `0.05 / 0.5 * 100 = 10%`다. 10%는 gameplay 상수가 아니다.

## Contamination Integration

```text
IncreaseRate = ActiveBatherCount * ContaminationPerBatherPerSecond
CleaningRate = CleaningRateAtFullCirculation * CirculationPercent / 100
Delta = (IncreaseRate - EffectiveCleaningRate) * DeltaSeconds
```

- circulation capacity가 부족하면 effective cleaning은 0이다.
- heating/cooling capacity 부족은 정화를 중지하지 않는다.
- 값은 `0~100`으로 clamp한다.
- 실제 입욕자만 증가에 포함한다. 예약·이동·입장/퇴장·탐색·knockdown은 포함하지 않는다.
- 이번 범위에서 오염도는 customer 선택·만족도·입욕 가능성을 바꾸지 않는다.

## Customer Actual-Bather Boundary

`UCustomerSessionComponent`의 기존 actual bath segment API가 연계 지점이다.

- `BeginActualBathSegment()`가 점유한 bath의 condition에 자기 session identity를 등록한 뒤 actual flag를 commit한다.
- `EndActualBathSegment()`는 reason과 관계없이 같은 identity를 unregister한다.
- knockdown, threshold invalidation, technical abort, StateTree exit와 EndPlay cleanup은 기존 공통 종료 경로를 통해 idempotent 해제한다.
- raw increment/decrement counter를 두지 않는다. 같은 session 중복 begin/end가 오염 인원을 깨뜨리지 않아야 한다.
- 이 연계 때문에 `ST_CustomerRoutine` asset graph를 새로 편집하지 않는다.

## Bath Placement And Recovery

- placed domain이 활성화된 욕탕만 subsystem에 condition/demand를 등록한다.
- 회수 hold 동안 기존 water freeze와 condition freeze를 함께 적용하지만 욕탕의 예약 demand는 유지한다.
- 회수 stage가 실제 domain unregister 지점이다. demand를 silent 제거한다.
- rollback은 water/condition snapshot과 registry를 정확히 한 번 복원한다.
- commit은 Actor 제거와 publication을 한 번 수행한다.
- 예상치 못한 bath EndPlay는 registry에서 제거한다. 다른 욕탕 설정은 변경하지 않는다.
- preview/packaged Actor는 condition registry에 들어가지 않는다.

## Blueprint And Editor Contracts

Editor 단계의 최소 신규 asset은 다음이다.

- `BP_Boiler`, `BP_Cooler`, `BP_Circulator`: 각각 native boiler·cooler·circulator 자식. 구성은 [UtilityLaborSystem.md](UtilityLaborSystem.md)
- 종류별 placement definition과 회수 item presentation 연결

기존 `BP_Bath`는 inherited condition component 값을 확인하고 필요 시 instance override한다. 관리 화면의 WBP와 computer instance reference는 [BathWaterManagementUISystem.md](BathWaterManagementUISystem.md)가 소유한다.

## Dependencies

- Bath Water Operations -> 기존 Bath Water public API
- Bath Water Operations -> Placement public interface/payload
- Bath Water Operations -> Interaction public carry/prompt 계약
- Customer -> Bath Water Operations의 condition 등록 API
- Management UI -> Bath Water Operations snapshot/request API
- 새 Engine module 의존성은 필요하지 않는다. 기존 `DeveloperSettings`, `Niagara` 범위 안에서 구현한다.

## Compatibility

- 기존 reflected 이름을 rename/delete하지 않는다. Core Redirect는 필요 없다.
- 기존 `UBathWaterStateComponent` amount delegate 의미를 바꾸지 않고 새 flow delegate를 추가한다.
- 기존 placement, recovery, water threshold, customer timer와 monitor focus 계약은 필요한 연결 외에는 유지한다.
- SaveGame, replication과 최종 art는 범위 밖이다.

## Verification

Native automation은 최소한 다음을 검증한다.

- utility 중복 등록 방지, 세 종류 독립 합산과 unexpected loss/resume
- setting candidate 계산, clamp, decrease-always-allowed와 ambient cross-over
- 정상 회수 허용/거부, stage rollback과 publication 1회
- typed capacity payload round trip과 invalid kind/value 거부
- empty bath도 demand 예약, reinstall은 ambient/0/clean 초기화
- simultaneous fill/drain net-zero에서도 ambient/clean mixing
- drain-only concentration 보존과 완전 배수 reset
- 기본 임계 10%, exact ambient 출발 저항, exact target 유지/이탈
- 큰 DeltaSeconds의 ambient/target 경계 통과와 epsilon 비진동
- circulation deficit이 정화/thermal을 함께 중지하고 heat/cool deficit은 thermal만 중지
- weak actual-bather set의 중복 begin/end, knockdown/abort/EndPlay cleanup

Source 구현 뒤에는 `BathhouseSimEditor Win64 Development` build와 관련 focused automation을 실행한다. Content 편집과 PIE 검증은 코드 리뷰 승인 뒤 Unreal 단계로 넘긴다.
