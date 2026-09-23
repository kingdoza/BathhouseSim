# 코드 리뷰 프롬프트 — Bath Water Operations 재작업

## 리뷰 범위

`.md/PROMPT_IMPLEMENTATION_R.md`의 deficit transaction, 지도 투영, native 관리 UI, registry revision과 실제 facility transaction 보강을 리뷰한다.

- Source와 architecture/review/Unreal 인계 문서만 수정했다.
- `Content/`, `Config/`, Level, StateTree, Blueprint/WBP와 `.md/Unreal/*`은 수정하지 않았다.
- 기존 reflected 이름과 enum ordinal을 유지했고 Core Redirect를 추가하지 않았다.

## 구현 결과

### Request atomicity

- heating 방향 증가는 현재 target을 하한, cooling 방향 증가는 현재 target을 상한으로 보존한다.
- deficit 상태에서도 ambient 방향 감소와 ambient 횡단은 계속 허용한다.
- 제한 부족량은 `max(RequestedCandidateAggregateUsed - TotalCapacity, 0)`이다.
- limited/no-op의 committed 값이 current와 같으면 setter, `DataRevision`, delegate publication을 모두 생략한다.

### Registry와 snapshot

- provider register/unregister는 capacity/data revision만 변경하고 bath topology revision을 변경하지 않는다.
- bath identity 변경만 topology/data revision을 변경한다.
- invalid provider/bath weak entry prune는 대응 revision과 data revision을 올리고 reentrant-safe publication을 수행한다.
- bath snapshot은 demand가 있는 종류에 한해 `bCirculationCapacityDeficit`, `bHeatingCapacityDeficit`, `bCoolingCapacityDeficit`을 독립적으로 채운다.

### 지도 투영

- Zone world X:Y 비율을 보존하는 중앙 letterbox content rect를 사용한다.
- footprint 네 corner를 `TransformPosition()`으로 world에 옮겨 Actor/root/component scale을 포함한다.
- Zone world 축에 투영해 `+X=화면 위`, `+Y=화면 오른쪽`을 유지한다.
- position은 네 projected corner 평균, size/angle은 인접 edge에서 한 번만 계산한다. screen AABB 뒤 재회전하지 않는다.
- 네 corner 중 하나라도 Zone 밖이면 tile을 제외한다.
- bath topology 또는 Zone geometry가 바뀔 때만 tile set을 rebuild하며 provider-only 변화는 tile identity를 유지한다.

### Native UI 계약

- capacity summary는 종류별 text, progress와 `Normal/Full/Deficit` 상태를 C++에서 적용한다.
- tile/detail은 욕탕별 복수 deficit 이유를 동시에 표시하며 thermal status와 합치지 않는다.
- detail은 bath 이름, `°C`, empty-selection clear, limited kind/points feedback과 selection/context/revision 기반 stale cleanup을 구현한다.
- root의 사용하지 않던 `bSnapshotDirty`를 제거하고 delegate refresh + polling, child별 value cache 정책으로 통일했다.
- 동일 snapshot은 text/progress/slider/layout/hook을 다시 쓰지 않는다.

필수 `BindWidget`:

| Native Widget | 필수 이름 |
|---|---|
| `UBathWaterManagementScreenWidget` | `CapacitySummary`, `BathMap`, `BathDetail` |
| `UBathWaterCapacitySummaryWidget` | `CirculationCapacityText`, `HeatingCapacityText`, `CoolingCapacityText`, `CirculationCapacityBar`, `HeatingCapacityBar`, `CoolingCapacityBar`, `CirculationCapacityStatusText`, `HeatingCapacityStatusText`, `CoolingCapacityStatusText` |
| `UBathWaterMapWidget` | `BathTileCanvas`, `EmptyStateText` |
| `UBathWaterBathTileWidget` | `SelectButton`, `BathNameText`, `ActualTemperatureText`, `ContaminationText`, `ThermalStatusText`, `CapacityStatusText` |
| `UBathWaterDetailWidget` | `BathNameText`, `WaterAmountText`, `ActualTemperatureText`, `TargetTemperatureText`, `ContaminationText`, `CirculationText`, `CirculationDemandText`, `HeatingDemandText`, `CoolingDemandText`, `ThermalStatusText`, `ThermalThresholdText`, `CapacityStatusText`, `FeedbackText`, `CirculationSlider`, `TargetTemperatureSlider` |

Optional presentation hook은 `OnCapacityDisplayStateChanged`, `OnBathTileStateChanged`다. Blueprint 구현이 없어도 값과 입력은 정상 동작해야 한다.

### Facility transaction 검증

- `ABathWaterUtilityFacilityActor` 실제 instance로 pre-placed registration, demand-blocked recovery, silent stage/success publication, injected activation failure rollback, unexpected Destroy deficit와 replacement resume을 실행한다.
- 실제 typed payload의 kind/points round trip과 mismatched kind/NaN import fail-closed를 실행한다.
- `ABathhouseBathFacilityActor` 실제 instance로 empty-bath recovery hold, staged unregister와 condition/water/registry rollback을 실행한다.
- utility native footprint 기본은 프로젝트 20cm grid에서도 유효한 60×60cm로 정리했다.
- 기존 generic ActorReplacement 실패는 Content fixture의 현재 grid/Blueprint footprint 불일치와 함께 별도 남아 있으며, 이번 domain은 독립 실제 utility/bath integration으로 success/failure/rollback을 검증한다.

## 자동화 근거

`BathhouseSim.BathWater`의 다음 10개 테스트가 성공해야 한다.

- `Operations.CapacityDemandAndRequests`
- `Operations.FacilityTransactionAtomicity`
- `Operations.FlowConditionAndBathers`
- `Operations.MapProjection`
- `Operations.NativeWidgetPresentation`
- `Operations.PayloadAndUIContracts`
- `Operations.RequestAtomicityAndRevision`
- `ControlAxisMotionAndPlanePresentation`
- `CustomerSearchActualTimeAndInvalidation`
- `StateThresholdFlowAndFreeze`

추가 확인:

- UE 5.8 `BathhouseSimEditor Win64 Development` 실제 compile
- `BathhouseSim.Customer` 회귀
- 전체 `BathhouseSim`에서 신규 실패 여부
- `git diff --check`

2026-09-22 실제 실행 결과:

- `git diff --check`: 성공(기존 LF→CRLF 경고만 존재)
- UE 5.8 `BathhouseSimEditor Win64 Development`: 실제 compile 성공
- `BathhouseSim.BathWater`: 10/10 성공
- `BathhouseSim.Customer`: 8/8 성공
- 전체 `BathhouseSim`: 47개 중 44 성공, 아래 기존 Placement 3 실패

현재 Placement 분리 기준:

- `Placement.ActorReplacementTransaction`, `Placement.ActorReplacementFailureAtomicity`: 기존 Content/fixture의 global 20cm grid와 authored footprint 불일치가 남아 있다. 이번 실제 utility/bath focused transaction은 성공해야 한다.
- `Placement.SettingsZoneLeaseAndCompatibility`: test의 10cm 기대와 현재 Config 20cm, 기존 grid/material fixture 문제로 이번 완료 조건에서 분리한다.

## 집중 리뷰 항목

1. deficit outward request가 current target을 낮추지 않으며 no-op publication이 없는가.
2. provider 변화가 bath topology를 올리지 않고 invalid weak prune가 revision/publication을 남기는가.
3. projection이 Zone/footprint scale과 임의 yaw를 한 좌표계에서 처리하고 letterbox 비율을 보존하는가.
4. child cache가 continuous state 갱신을 누락하지 않으면서 동일 snapshot write를 억제하는가.
5. 복수 deficit flag가 demand 0인 Bath에 오표시되지 않는가.
6. slider limited feedback이 kind/points를 유지하고 selection/context/후속 mutation에서 정리되는가.
7. utility/bath staged rollback이 identity, state, capacity와 publication을 중복시키지 않는가.
8. Content/Config/기존 sample computer와 input owner 계약에 파장이 없는가.

## Editor 전 남은 작업

- 새 WBP hierarchy와 모든 필수 BindWidget compile
- utility 3종 Blueprint/Definition과 승인된 mesh/footprint authoring
- computer Widget Class 및 exact `ManagedBathPlacementZone` instance reference
- PIE에서 letterbox/yaw/selection/deficit/feedback/회수 시각 확인
