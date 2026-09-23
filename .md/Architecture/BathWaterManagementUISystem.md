# Bath Water Management UI System

## Implementation Status

- 상태: native Widget Source와 Widget Blueprint 5개 authoring·컴퓨터 연결 완료; 직접 PIE 수용 검증 대기
- domain 정본: [BathWaterOperationsSystem.md](BathWaterOperationsSystem.md)
- computer session 정본: [ComputerSystem.md](ComputerSystem.md)
- 공통 native Widget 정책: [UISystem.md](UISystem.md)

## Target Source Scope

```text
Source/BathhouseSim/Public/UI/
  BathWaterManagementScreenWidget.h
  BathWaterCapacitySummaryWidget.h
  BathWaterMapWidget.h
  BathWaterBathTileWidget.h
  BathWaterDetailWidget.h

Source/BathhouseSim/Private/UI/
  BathWaterManagementScreenWidget.cpp
  BathWaterCapacitySummaryWidget.cpp
  BathWaterMapWidget.cpp
  BathWaterBathTileWidget.cpp
  BathWaterDetailWidget.cpp
```

`BathhouseComputerActor.h/.cpp`에는 관리 Zone context 주입만 추가한다. focus/input session은 변경하지 않는다.

## Responsibilities

- 종류별 utility used/total/deficit 표시
- 관리 PlacementZone과 설치 Bath footprint의 world-to-screen 투영
- 선택 Bath 상태 표시와 circulation/target setting intent 전달
- topology revision과 연속 presentation 값의 효율적인 refresh
- selected Bath 또는 Zone 소멸 시 안전한 selection/context 정리

이 UI는 capacity, demand, setting, 수온, 오염도와 물 양의 정본이 아니다.

## Computer Management Context

`ABathhouseComputerActor`에 `ManagedBathPlacementZone`을 `EditInstanceOnly`로 추가한다.

- `ScreenWidget->InitWidget()` 뒤 management root이면 operations subsystem과 Zone을 명시적으로 주입한다.
- Widget이 world에서 Zone이나 Bath를 반복 검색하지 않는다.
- 같은 관리 화면을 보여야 하는 여러 computer는 Editor에서 같은 Zone instance를 참조한다.
- 참조가 없거나 Zone이 제거되면 빈 지도와 unavailable 상태를 표시하고 mutation을 보내지 않는다.
- 기존 focus camera, world-space widget lifetime, pointer와 focus-out 복원 계약은 바꾸지 않는다.
- LMB는 기존 `PrimaryUseAction`과 `Computer > Placement > Equipment` owner priority를 사용한다. 새 Input Action을 만들지 않는다.
- deprecated `ComputerClickAction` fallback은 asset migration 호환을 위해 유지한다.

## Native Widget Hierarchy

```text
UBathWaterManagementScreenWidget
  ├─ UBathWaterCapacitySummaryWidget
  ├─ UBathWaterMapWidget
  │    └─ dynamic UBathWaterBathTileWidget instances
  └─ UBathWaterDetailWidget
```

- root: subsystem/Zone context, selected Bath weak reference, snapshot refresh와 child 조립
- capacity summary: 종류별 used/total/deficit 표시
- map: Zone과 footprint의 화면 투영과 tile lifecycle
- tile: 이름, 실제 수온, 오염도, 상태색과 선택 의도
- detail: 수위, 실제/목표 수온, 오염도, 순환도, 요구량, 임계치와 slider intent

각 native Widget은 로직에 필수인 최소 child만 `BindWidget`으로 요구한다. WBP는 hierarchy, layout, style, font/material과 animation만 소유한다.

## Map Projection

- world `+X`는 화면 위, world `+Y`는 화면 오른쪽이다.
- `ManagedBathPlacementZone`의 실제 world bounds를 지도 영역으로 정규화한다.
- operations registry의 Bath 중 footprint가 해당 Zone에 포함된 대상만 표시한다.
- Bath footprint 네 모서리를 동일 투영해 크기와 yaw를 표현한다. Actor center 점만 표시하지 않는다.
- 순환기·보일러·쿨러와 기타 설비는 지도에 표시하지 않고 capacity summary에만 반영한다.
- Zone world X:Y 비율을 보존하는 중앙 letterbox content rect를 사용하고 uniform pixels-per-world-unit 하나만 적용한다.
- footprint의 네 world corner에 Actor/component scale을 포함해 한 번 투영하고, 인접 edge에서 tile size와 yaw를 계산한다.
- Bath topology 또는 Zone geometry가 바뀔 때만 tile set을 rebuild한다. provider-only capacity 변화는 tile identity를 바꾸지 않는다.

## Refresh And Mutation

- root는 constructed 상태에서 lightweight snapshot을 polling할 수 있다.
- Bath topology/Zone geometry가 변할 때만 map 구조를 rebuild하고 capacity summary는 종류별 presentation cache로 갱신한다.
- 연속 수위·수온·오염 값은 presentation cache와 달라질 때만 bound Widget과 Blueprint hook에 적용한다.
- slider callback은 operations subsystem request API를 호출하고 committed 값과 failure result를 다시 표시한다.
- slider가 condition property를 직접 set하지 않는다.
- 선택된 Bath가 회수/소멸하면 selection을 지우고 detail을 unavailable로 전환한다.
- focus-out은 Widget을 파괴하지 않으므로 마지막 선택과 화면 상태가 Actor lifetime 동안 유지된다.
- thermal threshold는 snapshot의 authoring 기반 파생값을 표시하고 `10%`를 UI 상수로 저장하지 않는다.
- bath snapshot은 circulation/heating/cooling deficit을 독립 flag로 제공하며 tile/detail은 이를 동시에 표시한다.
- 온도 문자열은 소수점 한 자리와 `°C`를 사용한다. selection/context 변경과 후속 revision에서는 transient request feedback을 정리한다.

### Required BindWidget Contract

- root: `CapacitySummary`, `BathMap`, `BathDetail`
- capacity: `CirculationCapacityText`, `HeatingCapacityText`, `CoolingCapacityText`, `CirculationCapacityBar`, `HeatingCapacityBar`, `CoolingCapacityBar`, `CirculationCapacityStatusText`, `HeatingCapacityStatusText`, `CoolingCapacityStatusText`
- map: `BathTileCanvas`, `EmptyStateText`
- tile: `SelectButton`, `BathNameText`, `ActualTemperatureText`, `ContaminationText`, `ThermalStatusText`, `CapacityStatusText`
- detail: `BathNameText`, `WaterAmountText`, `ActualTemperatureText`, `TargetTemperatureText`, `ContaminationText`, `CirculationText`, `CirculationDemandText`, `HeatingDemandText`, `CoolingDemandText`, `ThermalStatusText`, `ThermalThresholdText`, `CapacityStatusText`, `FeedbackText`, `CirculationSlider`, `TargetTemperatureSlider`

Optional native presentation hooks are `OnCapacityDisplayStateChanged` and `OnBathTileStateChanged`; gameplay correctness does not depend on Blueprint implementation.

## Compatibility

- 기존 `UComputerSampleScreenWidget`과 `WBP_ComputerSampleScreen`은 삭제하거나 rename하지 않는다.
- 새 management root가 아니면 `ABathhouseComputerActor`는 sample 화면 동작을 그대로 유지한다.
- existing monitor focus, FXAA override, pointer release와 EndPlay cleanup을 변경하지 않는다.
- SaveGame, fullscreen UI, hardware-input 병행과 keyboard desktop은 범위 밖이다.

## Blueprint And Editor Contracts

Editor 단계의 최소 신규 WBP:

- `WBP_BathWaterManagementScreen`
- `WBP_BathWaterCapacitySummary`
- `WBP_BathWaterMap`
- `WBP_BathWaterBathTile`
- `WBP_BathWaterDetail`

`BP_BathhouseComputer`는 새 root Widget Class를 지정하고 각 Level instance의 `ManagedBathPlacementZone`에 같은 욕탕용 Zone을 연결한다.

WidgetTree의 현재 hierarchy, 필수 `BindWidget` 이름·타입과 컴퓨터/Level 연결은 [InteractionUISystem.md](../Unreal/InteractionUISystem.md)에 기록한다. 다섯 WBP는 Editor API/Python fallback으로 저장·재로드 후 검사했으며 직접 PIE 수용 검증은 `USER_UNREAL.md`에 남아 있다.

## Dependencies

- Management UI -> Bath Water Operations snapshot/request API
- Computer -> Management UI context initialization
- Management UI -> UMG
- Gameplay domain -> concrete Widget Blueprint 의존 금지
- Interaction/Placement/Equipment input routing은 Management UI concrete type에 의존하지 않는다.

## Verification

- invalid/missing Zone이 mutation 없이 unavailable 상태가 되는지 확인한다.
- 여러 computer가 같은 Zone을 참조하면 같은 domain 값을 표시하는지 확인한다.
- world `+X` up, `+Y` right와 footprint 크기/yaw 투영을 확인한다.
- Zone 밖 Bath와 non-Bath utility가 지도에서 제외되는지 확인한다.
- topology revision에서만 tile set을 rebuild하는지 확인한다.
- slider가 subsystem transaction을 거쳐 committed/limited result를 표시하고 다른 Bath 설정을 바꾸지 않는지 확인한다.
- selected Bath 회수/EndPlay에서 stale reference가 남지 않는지 확인한다.
- focus-out/re-entry에서 Widget instance와 선택이 유지되는지 확인한다.
- sample Widget compatibility와 existing computer automation을 회귀 검증한다.
