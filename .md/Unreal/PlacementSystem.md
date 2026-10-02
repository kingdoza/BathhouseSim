# Placement Editor Authoring

## 전역 설정과 Preview 재질

- 전역 grid 간격은 Project Settings `Facility Placement > Grid Size Cm`가 정본이며 이 문서에 수치를 기록하지 않는다.
- `UFacilityPlacementSettings.FacilityItemHeldTransform`은 위치·회전만 담당하며 런타임은 scale을 1로 정규화한다. 값 원본은 `Config/DefaultGame.ini` `[/Script/BathhouseSim.FacilityPlacementSettings]`의 `FacilityItemHeldTransform`이다.
- 배치 미리보기 재질의 원본은 같은 Config 섹션의 `ValidPreviewMaterial`·`InvalidPreviewMaterial`이며 현재 `/Game/Material/MI_Preview_Valid`·`/Game/Material/MI_Preview_Invalid`(부모 `/Game/Material/M_Preview`)를 가리킨다. 색은 각 MI의 vector parameter `Param`이 원본이다. `M_Preview`는 Translucent·Unlit·one-sided이고 Opacity 입력이 연결되어 있지 않다(`PLACEMENT-FOOTPRINT-PREVIEW` 사전 조사 읽기 확인).
- `/Game/Bathhouse/Materials/Placement/MI_FacilityPreview_Valid`·`_Invalid`는 이름과 달리 class가 Material이며 배치 미리보기에는 쓰이지 않는다. 표현값은 각 Material asset의 Emissive·Opacity 입력이 원본이다. `_Valid`는 `BP_TrashCollectionZone`·`BP_Television`이 참조하고 `_Invalid`는 참조자가 없다.

## Definition 계약

다음 Definition에는 `PreviewActorClass`, `FootprintCellsX`, `FootprintCellsY`가 없으며 저장·재로드가 완료됐다.

| Definition | Placed Class | 상태 |
|---|---|---|
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Bath` | `BP_Bath` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Shower` | `BP_Shower` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_1` | `BP_ClothesLocker` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_4` | `BP_ClothesLocker_4` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_8` | `BP_ClothesLocker_8` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Washer` | `BP_Washer` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Dryer` | `BP_Dryer` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Circulator` | `BP_Circulator` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler` | `BP_Boiler` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Cooler` | `BP_Cooler` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_DrinkFridge` | `BP_DrinkFridge` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Vanity` | `BP_Vanity` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_MassageChair` | `BP_MassageChair` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_RestBench` | `BP_RestBench` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Television` | `BP_Television` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ScrubTable` | `BP_ScrubTable` | placement 활성 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_CleanTowelStack` | `None` | placement opt-out |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_UsedTowelBin` | `None` | placement opt-out |

락커 칸 수의 원본은 각 Definition asset의 `LockerSlotCount`다.

활성 16개 Definition의 `FacilityTags`에는 기존 태그와 함께 설비 종류 태그 `Facility.Type.*` 하나가 있다(1·4·8칸 락커는 같은 `Facility.Type.ClothesLocker`, 표는 [BuildingSystem.md](BuildingSystem.md)). 공간의 `IsDefinitionAllowed`가 이 태그로 공간별 허용을 판정한다. opt-out Stack·Bin에는 종류 태그가 없다.

활성 16개 Definition의 `RecoveryItemClass`는 공통 `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem.BP_PlaceableFacilityItem_C`, `RecoveryItemMesh`는 `None`이며 native Cube fallback을 사용한다. Stack과 Bin은 `PlacedFacilityClass`, `RecoveryItemClass`, `RecoveryItemMesh`가 모두 `None`인 opt-out 상태다.

`BP_Boiler`의 inherited `FacilityPlacement.Definition`은 `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler`로 저장됐다. `DefaultMap`의 기존 보일러 인스턴스는 별도 override 없이 재시작 후 같은 Definition을 상속한다. 다른 utility Blueprint의 Definition 연결 여부는 이 변경으로 보장하지 않는다.

공통 설비 아이템 Blueprint의 parent는 `/Script/BathhouseSim.PlaceableFacilityItemActor`다. 설비 회수·신규 설치 아이템의 공통 크기는 상속된 `ItemRoot`의 Relative Scale에서 조정한다. 이 값은 사용자 조정값이며 정본은 이 Blueprint asset 자체다. 문서에 수치를 기록하지 않고, 각 축은 유한한 양수여야 한다. Project Settings의 `FacilityItemHeldTransform.Scale`로는 크기를 조정하지 않는다.

구형 `/Game/Bathhouse/Blueprints/Placement/Preview/BP_FacilityPreview_*` 9개는 Definition 재저장 뒤 참조가 0임을 확인하고 삭제됐다. 현재 preview는 native `AFacilityPlacementPreviewActor` 경로만 사용한다.

## Blueprint footprint와 body

모든 footprint는 actor local install floor `Z=0`을 기준으로 한다. `PlacementFootprint`는 Navigation 비활성, Collision `NoCollision`이다. scaled full X/Y는 Project Settings grid 간격의 양의 정수배여야 하며 Data Validation이 검사한다.

footprint 크기·높이의 원본은 각 Blueprint의 `PlacementFootprint` component(Box Extent, Relative Location)이고, body 크기·mesh의 원본은 아래 body component의 Static Mesh·Relative Transform이다. 이 문서는 수치를 복제하지 않는다. Parent Class는 `/Script/BathhouseSim.` 접두를 생략했다.

| Blueprint | Parent Class | footprint component | body component | body 충돌·Navigation 계약 |
|---|---|---|---|---|
| `/Game/Bathhouse/Blueprints/Facility/BP_Bath` | `BathhouseBathFacilityActor` | `PlacementFootprint` | `FacilityVisual`(+ `DrainLeverControl`, `FillValveControl`; `WaterSurfaceMesh` 제외) | `FacilityVisual` BlockAllDynamic, Navigation 활성 |
| `/Game/Bathhouse/Blueprints/Facility/BP_Shower` | `BathhouseFacilityActor` | `PlacementFootprint` | `FacilityVisual` | BlockAllDynamic, Navigation 활성 |
| `/Game/Bathhouse/Blueprints/Facility/BP_ClothesLocker` | `BathhouseFacilityActor` | `PlacementFootprint` | `FacilityVisual` | BlockAllDynamic, Navigation 활성 |
| `/Game/Bathhouse/Blueprints/Facility/BP_ClothesLocker_4` | `BathhouseFacilityActor` | `PlacementFootprint` | `FacilityVisual`, `LockerVisual02~04` | BlockAllDynamic, Navigation 활성 |
| `/Game/Bathhouse/Blueprints/Facility/BP_ClothesLocker_8` | `BathhouseFacilityActor` | `PlacementFootprint` | `FacilityVisual`, `LockerVisual02~08` | BlockAllDynamic, Navigation 활성 |
| `/Game/Bathhouse/Blueprints/Towel/BP_Washer` | `TowelProcessingMachineActor` | `PlacementFootprint` | `MachineVisual`, `LidMesh` | `MachineVisual` BlockAllDynamic, Navigation 활성 |
| `/Game/Bathhouse/Blueprints/Towel/BP_Dryer` | `TowelProcessingMachineActor` | `PlacementFootprint` | `MachineVisual`, `LidMesh` | `MachineVisual` BlockAllDynamic, Navigation 활성 |
| `/Game/Bathhouse/Blueprints/Facility/BP_Circulator` | `BathWaterCirculatorFacilityActor` | `PlacementFootprint` | `VisualMesh`, `LeverMesh`, `GaugeNeedleMesh` | `VisualMesh` BlockAllDynamic, Navigation 활성 |
| `/Game/Bathhouse/Blueprints/Facility/BP_Boiler` | `BathWaterBoilerFacilityActor` | `PlacementFootprint` | `VisualMesh`, `GaugeFacePlate`, `FuelIntake`, `FuelDoorMesh`, `GaugeNeedleMesh` | `VisualMesh` BlockAllDynamic, Navigation 활성 |
| `/Game/Bathhouse/Blueprints/Facility/BP_Cooler` | `BathWaterCoolerFacilityActor` | `PlacementFootprint` | `VisualMesh`(`Cooler_Body`), `FuelDoorMesh`(`Cooler_Lid`), `GaugeNeedleMesh`(`Cooler_Needle`) | `VisualMesh` BlockAllDynamic, Navigation 활성 |
| `/Game/Bathhouse/Blueprints/Service/BP_DrinkFridge` | `DrinkFridgeActor` | `PlacementFootprint` | `FridgeBody`, `ShelfVisual0~3` | 미기록 |
| `/Game/Bathhouse/Blueprints/Service/BP_Vanity` | `BathhouseFacilityActor` | `PlacementFootprint` | `VanityBody`, `MirrorVisual` | 미기록 |
| `/Game/Bathhouse/Blueprints/Service/BP_MassageChair` | `MassageChairActor` | `PlacementFootprint` | `ChairBody` | 미기록 |
| `/Game/Bathhouse/Blueprints/Service/BP_RestBench` | `BathhouseFacilityActor` | `PlacementFootprint` | `BenchBody` | 미기록 |
| `/Game/Bathhouse/Blueprints/Service/BP_Television` | `TelevisionActor` | `PlacementFootprint` | `TvBody`(`ScreenOnVisual`은 기본 비가시) | 미기록 |
| `/Game/Bathhouse/Blueprints/Service/BP_ScrubTable` | `ScrubTableActor` | `PlacementFootprint` | `TableBody`(`ScrubCursor`는 HiddenInGame) | 미기록 |

- body component 목록은 배치 미리보기 수집 규칙(`FacilityPlacementPreviewSource::Collect`)이 고르는 mesh와 같다(`PLACEMENT-FOOTPRINT-PREVIEW` 사전 조사, 2026-10-02). `BP_ClothesLocker`는 사용자가 작업 트리에서 수정 중이라 이 문서는 구조만 적는다.
- "미기록"은 충돌·Navigation 설정을 이 문서가 아직 확인하지 않았다는 뜻이다. 추가 body component의 충돌 설정도 확인 범위 밖이다.
- `BP_Circulator`·`BP_Boiler`·`BP_Cooler`의 `SceneRoot` 회전과 `BP_Cooler` root scale은 별도 판단 대기라 계약으로 적지 않는다.

`PackagePhysicalRoot`, slot, interaction, action/approach point, towel presentation, water/contents helper는 Navigation에 관여하지 않는다. Bath의 `FacilityVisual` mesh(`SM_Bath_old`)에는 generated convex collision과 NavCollision이 존재한다.

## Placement Zone

- `/Game/Bathhouse/Blueprints/Placement/BP_FacilityPlacementZone`
- Parent Class: `/Script/BathhouseSim.FacilityPlacementZoneActor`
- inherited `ZoneBounds`, `PlacementFloor`, `GridVisual`을 사용한다.
- `PlacementFloor`의 ZoneBounds 상대 transform은 identity이며 Navigation 비활성이다.
- 실제 Level instance의 floor plane은 world `Z=0`이다. Zone Bounds의 두께는 설치 높이 계산의 기준이 아니다.
- `GridVisual`은 `PlacementFloor`의 child다. Static Mesh는 `/Engine/BasicShapes/Plane.Plane`, Material Element 0은 `/Game/Bathhouse/Materials/Placement/MI_FacilityPlacementGrid`다.
- `GridVisual`은 기본 hidden, Collision `NoCollision`, overlap/physics/Tick/Navigation 비활성이다. transform과 visibility는 native가 파생·관리하며 Blueprint graph는 관여하지 않는다.
- Grid 표현 Class Default(`GridLineThicknessCm`, `GridZOffsetCm`, `MajorGridIntervalCells`)의 정본은 이 Blueprint Class Defaults다. `BP_BathhouseSpace`는 같은 값을 가진다.

### Native grid Material

- `/Game/Bathhouse/Materials/Placement/M_FacilityPlacementGrid`: Surface, Translucent, Unlit, normal depth test, one-sided plane Material
- `/Game/Bathhouse/Materials/Placement/MI_FacilityPlacementGrid`: 위 Material의 공통 표현 instance
- Material graph는 plane UV에 `ZoneSizeXCm/Ycm ÷ GridSizeCm` cell 수를 적용한다. `LineThicknessCm ÷ GridSizeCm`의 절반 폭으로 minor line을 만들고, `MajorGridEveryNCells` 주기와 `MajorLineThicknessMultiplier`로 major line을 합성한다. 두 line mask의 역영역에는 별도 cell fill 색과 opacity를 적용한다.
- native DMI 입력은 `GridSizeCm`, `ZoneSizeXCm`, `ZoneSizeYCm`, `LineThicknessCm`, `MajorGridEveryNCells` 다섯 scalar다. MI는 이 다섯 값을 override하지 않는다.
- MI 표현 parameter는 `MinorLineColor`, `MajorLineColor`, `GridOpacity`, `MajorLineThicknessMultiplier`, `CellFillColor`, `CellFillOpacity`다. 값 원본은 `MI_FacilityPlacementGrid`의 이 parameter override이며 이 문서에 수치를 적지 않는다.

`DefaultMap`에는 `BP_FacilityPlacementZone` instance가 없다(`EXP-U1`에서 삭제). 배치 구역은 공간 Actor 3개(`BP_BathhouseSpace`, parent `BathhouseSpaceActor` ← `FacilityPlacementZoneActor`)이며 각 공간 바닥 전체가 그 공간의 구역이다. `ZoneBounds` XY extent는 공간 `FloorSizeCm`에서 native가 파생하고, 공간별 허용 설비는 instance `AllowedFacilityTags`(`Facility.Type.*`)다. 구조·값 원본은 [BuildingSystem.md](BuildingSystem.md)에 있다.

Recast와 Project Settings의 미저장 전역값, 실제 입력 기반 preview/배치/회수 판정은 [USER_UNREAL.md](../USER_UNREAL.md)를 따른다.
