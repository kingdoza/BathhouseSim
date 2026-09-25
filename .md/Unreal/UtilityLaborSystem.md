# Utility Labor Editor Authoring

## 저장된 Blueprint

| Asset | Parent Class | 표현과 핵심 계약 |
|---|---|---|
| `/Game/Bathhouse/Blueprints/Facility/BP_Boiler` | `/Script/BathhouseSim.BathWaterBoilerFacilityActor` | 본체·풋프린트·Definition은 [BathWaterSystem.md](BathWaterSystem.md) 참조. 투입구·계기는 native inherited component |
| `/Game/Bathhouse/Blueprints/Utility/BP_UtilityShovel` | `/Script/BathhouseSim.UtilityShovelActor` | `WorldMesh` 물리 root, `LoadVisual` child |
| `/Game/Bathhouse/Blueprints/Utility/BP_CoalSupply` | `/Script/BathhouseSim.UtilityFuelSupplyActor` | `SupplyMesh` 상호작용 trace target |

세 Blueprint는 warnings-as-errors Compile, 개별 Save와 새 Editor 재로드에서 parent·mesh 연결을 확인했다. 별도 Blueprint gameplay graph는 없다.

## 임시 Cube 설정

- `BP_Boiler.FuelIntake`: `/Engine/BasicShapes/Cube`, local Location `(0,-33,42)`, Scale `(0.25,0.06,0.18)`. QueryOnly/Visibility Block/Navigation off인 투입 판정용 component다.
- `BP_Boiler.GaugeFace`: 같은 Cube, Location `(0,-32,90)`, Scale `(0.30,0.02,0.30)`. `GaugeNeedlePivot`은 `(0,-34,90)`이며 child `GaugeNeedleMesh`는 같은 Cube, pivot 상대 Location `(7,0,0)`, Scale `(0.14,0.02,0.015)`다. 둘 다 NoCollision/Navigation off다.
- `BP_UtilityShovel.WorldMesh`: `/Engine/BasicShapes/Cube`, Scale `(0.25,0.06,0.03)`. 물리 root의 native QueryAndPhysics, CCD와 Pawn Ignore 계약을 유지한다.
- `BP_UtilityShovel.LoadVisual`: 같은 Cube, `WorldMesh` child, relative Location `(20,0,90)`, Scale `(0.5,0.8,1)`. NoCollision/Navigation off이며 비적재 상태에서는 숨긴다.
- `BP_CoalSupply.SupplyMesh`: 바닥 pivot `/Game/Bathhouse/Meshes/SM_Facility_sample`, Scale `(0.5,0.5,0.5)`. native 기본값 `FuelKind=Coal`, `ScoopPoints=25`다.

`BP_Boiler.GaugePresentation`은 Y local 회전축, `-90°~90°`, 시작 비율 `1/3`이며 `Operation`은 최대 100·초당 1 감소다. 최종 본체는 투입구·계기판 통합 모델로 교체하고 가동 바늘만 별도 움직임으로 남긴다.

## Level과 UI 경계

`DefaultMap`에는 석탄 공급함·삽·전용 fixed slot이 아직 저장되지 않았다. MCP에서 외부 액터의 개별 Save가 패키지를 찾지 못해 임시 배치를 제거했다. 기존 보일러 instance는 Blueprint 재로드 뒤 투입구·계기 설정을 상속한다. 정확한 배치·AssignedItem 연결과 1024×576 Capacity Summary의 실제 플레이 화면 검증은 [USER_UNREAL.md](../USER_UNREAL.md)에 있다. Capacity Summary의 저장된 layout은 [InteractionUISystem.md](InteractionUISystem.md)에 기록한다.
