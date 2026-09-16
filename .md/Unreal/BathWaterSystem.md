# Bath Water Editor Authoring

## 욕탕 Blueprint

| Asset | Parent Class | 저장 상태 |
|---|---|---|
| `/Game/Bathhouse/Blueprints/Facility/BP_Bath` | `/Script/BathhouseSim.BathhouseBathFacilityActor` | Compile·개별 Save·새 Editor 재로드 확인 완료 |

기존 `FacilityVisual`, `PlacementFootprint`, `FacilityPlacement`, `FacilitySlotA~C`와 슬롯 transform은 유지한다. Level instance에는 이번 작업의 별도 override를 저장하지 않았으며 `/Game/Maps/DefaultMap`은 재로드 뒤 clean 상태다.

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
```

`BathWaterState`는 Actor component이며 하나만 존재한다. `WaterSurfaceMesh`는 `NoCollision`, overlap·physics·Navigation off 계약을 따르고, 두 control component는 query-only interaction 대상으로 `Visibility=Block`, overlap·physics·Navigation off인 native 기본 계약을 따른다.

## 저장된 Class Default

| 대상 | 값 |
|---|---|
| `BathWaterState` | `FillRatePercentPerSecond=6.666667`, `DrainRatePercentPerSecond=10.0` |
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
