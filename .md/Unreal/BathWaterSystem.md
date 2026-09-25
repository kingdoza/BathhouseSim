# Bath Water Editor Authoring

## 욕탕 Blueprint

| Asset | Parent Class | 저장 상태 |
|---|---|---|
| `/Game/Bathhouse/Blueprints/Facility/BP_Bath` | `/Script/BathhouseSim.BathhouseBathFacilityActor` | Compile·개별 Save·새 Editor 재로드 확인 완료 |

기존 `FacilityVisual`, `PlacementFootprint`, `FacilityPlacement`, `FacilitySlotA~C`와 슬롯 transform은 유지한다. Level instance에는 이번 작업의 별도 override를 저장하지 않았으며 `/Game/Maps/DefaultMap`은 재로드 뒤 clean 상태다.

## 유틸리티 Blueprint

`BP_Circulator`와 `BP_Cooler`는 `/Script/BathhouseSim.BathWaterUtilityFacilityActor`를, `BP_Boiler`는 그 native 자식 `/Script/BathhouseSim.BathWaterBoilerFacilityActor`를 상속한다. 공통 본체 시각 mesh는 바닥 pivot 100cm cube인 `/Game/Bathhouse/Meshes/SM_Facility_sample`이다. Blueprint graph나 중복 gameplay component는 없다.

`BP_Boiler`의 `FacilityPlacement.Definition`은 `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler`다. Reparent와 아래 임시 Cube 표현을 개별 저장하고 새 Editor에서 재로드했다. 기존 `DefaultMap` 보일러 인스턴스도 재시작 후 투입구 mesh와 transform을 상속했으며, 그 외부 액터의 별도 override는 저장하지 않았다.

| Asset | Capacity | Visual scale | PlacementFootprint Extent / Relative Z |
|---|---|---|---|
| `/Game/Bathhouse/Blueprints/Facility/BP_Circulator` | Circulation `100` | `(1.2,0.8,1.2)` | `(60,40,60)` / `60` |
| `/Game/Bathhouse/Blueprints/Facility/BP_Boiler` | Heating `100` | `(1.0,0.6,1.2)` | `(50,30,60)` / `60` |
| `/Game/Bathhouse/Blueprints/Facility/BP_Cooler` | Cooling `100` | `(1.0,0.6,1.2)` | `(50,30,60)` / `60` |

대응 `DA_FacilityPlacement_*`의 StableId는 `Facility.Circulator`, `Facility.Boiler`, `Facility.Cooler`이며 모두 `Facility.Placeable` 태그와 공통 `BP_PlaceableFacilityItem` recovery class를 사용한다. `RecoveryItemMesh=None`으로 기존 설비와 같은 native Cube fallback을 사용한다. `BP_Boiler`의 warnings-as-errors Compile·개별 Save·새 Editor 재로드를 확인했다. 새 세션 기본 PIE에서 보일러 필수 투입구 누락 경고는 재발하지 않았다. 실제 연료 투입과 바늘 회전의 입력·시각 수용 검증은 [USER_UNREAL.md](../USER_UNREAL.md)에 남아 있다.

### Boiler 노동 가동 임시 표현

| inherited component | mesh / local transform | 역할 |
|---|---|---|
| `FuelIntake` | Engine Cube, Location `(0,-33,42)`, Scale `(0.25,0.06,0.18)` | 별도 Visibility hit target, QueryOnly, Navigation off |
| `GaugeFace` | Engine Cube, Location `(0,-32,90)`, Scale `(0.30,0.02,0.30)` | 고정 계기판, NoCollision/Navigation off |
| `GaugeNeedlePivot` | Location `(0,-34,90)` | 바늘 회전 중심 |
| `GaugeNeedleMesh` | Engine Cube, pivot child, Location `(7,0,0)`, Scale `(0.14,0.02,0.015)` | 두 축을 축소한 얇은 바늘, NoCollision/Navigation off |

`GaugePresentation.LocalRotationAxis=(0,1,0)`, `ZeroAngleDegrees=-90`, `MaxAngleDegrees=90`, `ActiveStartRatio=1/3`이다. `Operation`의 최대 잔량은 100, 감소량은 초당 1이다. 최종 보일러 모델은 투입구·계기판을 본체에 통합하고 움직이는 바늘만 별도 mesh로 교체할 예정이다.

## Native component hierarchy

```text
SceneRoot
├─ FillValveControl
├─ DrainLeverControl
├─ FillFlowNiagara
└─ WaterPresentationRoot
   ├─ WaterSurfaceMover
   │  └─ WaterSurfaceMesh
   ├─ WaterLevelEmptyPoint
   └─ WaterLevelFullPoint

Actor Components
├─ BathWaterState
└─ BathWaterCondition
```

`BathWaterState`와 `BathWaterCondition`은 각각 하나만 존재한다. `WaterSurfaceMesh`는 `NoCollision`, overlap·physics·Navigation off 계약을 따르고, 두 control component는 query-only interaction 대상으로 `Visibility=Block`, overlap·physics·Navigation off인 native 기본 계약을 따른다.

## 저장된 Class Default

| 대상 | 값 |
|---|---|
| `BathWaterState` | `FillRatePercentPerSecond=6.666667`, `DrainRatePercentPerSecond=10.0` |
| `BathWaterCondition` capacity | Max circulation `100`, Heating/Cooling `5 points/°C` |
| `BathWaterCondition` rates | Cleaning `1 point/s`, contamination `0.1 point/s/actual bather`, target control `0.5°C/s`, natural return `0.05°C/s` |
| `WaterSurfaceMesh.StaticMesh` | `/Game/Bathhouse/Meshes/Bath/Bath_01/StaticMeshes/SM_Bath_01_Water` |
| `WaterSurfaceMesh` transform | Location `(0,0,0)`, Rotation `(0,0,0)`, Scale `(0.65,0.65,1)` |
| 수위 marker | Empty `(0,0,-70)`, Full `(0,0,-15)`; `WaterPresentationRoot` local space |
| `FillValveControl.StaticMesh` | `/Engine/BasicShapes/Cube`; Location `(-55,105,68)`, Scale `(0.30,0.08,0.08)` |
| `FillValveControl` motion | local axis `(0,1,0)`, open angle `90°`, duration `0.5s` |
| `DrainLeverControl.StaticMesh` | `/Engine/BasicShapes/Cube`; Location `(55,105,65)`, Scale `(0.06,0.08,0.30)` |
| `DrainLeverControl` motion | local axis `(0,1,0)`, open angle `45°`, duration `0.5s` |
| `FillFlowNiagara` | `/Game/Niagaras/NS_HoneyBeam`; Location `(-55,80,68)`, Rotation `(-90,0,0)`, `AutoActivate=false` |

두 Cube와 `NS_HoneyBeam`은 사용자가 승인한 임시 표현이다. 실제 전용 mesh/Niagara로 교체할 때 component 이름과 native 역할은 유지하고 mesh pivot, control 회전축, nozzle 위치만 재저작한다. 현재 DrainLever Cube의 pivot은 중앙이므로 실제 끝단 hinge 표현은 전용 mesh 교체 시 맞춘다.

## 알려진 표현 경고

`SM_Bath_01_Water`의 translucent material `M_Water_Turquoise`가 Nanite mesh에 사용되어 Editor 로드 시 경고가 발생한다. 이번 작업에서는 mesh asset이나 material을 변경하지 않았다. 물이 렌더링되지 않거나 잘못 보이면 `WaterSurfaceMesh` component에서 Nanite 사용 금지 또는 비-Nanite 수면 mesh 사용을 별도 검토한다.

StateTree BathLoop authoring과 실제 급수·배수/상호작용 PIE 수용 검증은 [USER_UNREAL.md](../USER_UNREAL.md)에 남아 있다.
