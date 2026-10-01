# Building Editor Authoring

공간 건물(홀·목욕공간·작업공간)의 Editor 구조다. C++ 책임·형상 규칙·검증 계약은 [Architecture/BuildingSystem.md](../Architecture/BuildingSystem.md)가 정본이다. 조정값 수치는 적지 않고 원본 위치만 적는다. 저장·새 프로세스 재로드로 확인한 상태다(`EXP-U1`).

## 공용 값 원본

- Project Settings > Game > Bathhouse Building(`Config/DefaultGame.ini` `[/Script/BathhouseSim.BathhouseBuildingSettings]`): 벽 두께, 바닥·천장 판 두께, 형상 상자 mesh(`/Engine/BasicShapes/Cube`), 조각 최대 크기, 쓰레기 조각 class `BP_LitterSpawnZone_C`, 물 얼룩 조각 class `BP_StainSpawnZone_C`.
- 설비 종류 태그 `Facility.Type.*` 14개는 `Config/DefaultGameplayTags.ini`에 있다.

## Blueprint

| Asset | Parent | 내용 |
|---|---|---|
| `/Game/Bathhouse/Blueprints/Building/BP_BathhouseSpace` | `/Script/BathhouseSim.BathhouseSpaceActor` | graph·추가 component·변수 없음. 상속 `GridVisual`에 `/Engine/BasicShapes/Plane`과 `/Game/Bathhouse/Materials/Placement/MI_FacilityPlacementGrid`(element 0). 상속 `ZoneBounds` Z extent는 `BP_FacilityPlacementZone`과 같고 상대 위치·`PlacementFloor`는 identity라 설치 바닥 = 공간 바닥 윗면이다. Class Default `GridLineThicknessCm`·`GridZOffsetCm`·`MajorGridIntervalCells`는 `BP_FacilityPlacementZone`과 같은 값, `AllowedFacilityTags`는 비어 있다(instance가 채움) |

Compile 결과 `BS_UP_TO_DATE`(경고 없음). component 계층: `SpaceRoot`(root, Static) > `ZoneBounds` > `PlacementFloor` > `GridVisual`, `SpaceRoot` > `Shell`. 벽·바닥·천장·조명·계단·조각 미리보기는 `Shell`이 만드는 Transient 생성물이며 저장되지 않는다.

## DefaultMap 공간 Actor (World Partition external actor)

모든 공간 값의 정본은 각 instance Details `Bathhouse Space` 묶음과 Actor Location이다(Rotation 0, Scale 1).

| Label | Actor | external package | Space Kind | Cleaning Chunk Kind |
|---|---|---|---|---|
| `Space_Hall` | `BP_BathhouseSpace_C_UAID_F02F7433CA362F0703_1270228495` | `/Game/__ExternalActors__/Maps/DefaultMap/8/7N/V36YOPHA8C46E77EZIX78C` | 홀 | 쓰레기 조각 |
| `Space_Bath` | `BP_BathhouseSpace_C_UAID_F02F7433CA362F0703_1645196495` | `/Game/__ExternalActors__/Maps/DefaultMap/6/85/2UD4T92UQSBNFKC3N8BZFK` | 목욕공간 | 물 얼룩 조각 |
| `Space_Work` | `BP_BathhouseSpace_C_UAID_F02F7433CA362F0703_1645216496` | `/Game/__ExternalActors__/Maps/DefaultMap/9/0G/5Z3D27MZMK7HDNFHFAEYU2` | 작업공간 | 없음 |

- 지상 바닥 높이(`Space_Hall`·`Space_Bath` Location Z)는 지형 면보다 판 두께 이상 위이고 Recast `AgentMaxStepHeight`와 캐릭터 `MaxStepHeight` 이하다(출입구 문턱). 지하 깊이는 `Space_Work` Location Z와 홀 Z의 차다.
- `Space_Hall` `Openings`: 서쪽 바깥 출입구 1개(`ConnectedSpace` 없음), 동쪽 통로 1개(`ConnectedSpace` = `Space_Bath`). `Space_Bath`·`Space_Work` `Openings`는 비어 있다.
- `Space_Hall` `Stairs[0]`: `LowerSpace` = `Space_Work`, 동쪽으로 내려가며 맨 위 가장자리가 홀 남쪽 레인에 있다. 계단 재질은 이 항목 안에 있다. 다른 공간 `Stairs`는 비어 있다.
- `AllowedFacilityTags`: 홀 `ClothesLocker`·`DrinkFridge`·`MassageChair`·`RestBench`·`Television`·`Vanity`, 목욕공간 `Bath`·`Shower`·`ScrubTable`, 작업공간 `Boiler`·`Cooler`·`Circulator`·`Washer`·`Dryer`(모두 `Facility.Type.` 접두).
- `Lighting`: 세 공간 같은 값으로 시작했다(그림자 끔). 밝기 조정은 instance 값에서 한다.
- 컴퓨터 `ManagedBathPlacementZone`은 `Space_Bath`다([InteractionUISystem.md](InteractionUISystem.md)).

## 재질

| Asset | 용도 |
|---|---|
| `/Game/Bathhouse/Materials/Building/M_Building_WorldAligned` | world 기준 3면 투영(`WorldAlignedTexture`) 마스터. parameter `BaseTexture`, `TileSizeCm`, `Tint`, `Desaturation`, `Roughness`. Used with Instanced Static Meshes |
| `MI_Building_Hall_Wall` / `_Floor` / `_Ceiling` | 홀 `Surfaces`(벽지·나무 바닥·천장, StylizedKitchen 텍스처) |
| `MI_Building_Bath_Wall` / `_Floor` / `_Ceiling` | 목욕공간 `Surfaces`(욕실 타일·석고 천장) |
| `MI_Building_Work_Wall` / `_Floor` / `_Ceiling` | 작업공간 `Surfaces`(회색 거친 마감, desaturate+tint). 계단 `StepMaterial` = `MI_Building_Work_Floor`, `StairWallMaterial` = `MI_Building_Work_Wall` |

텍스처 원본은 `/Game/StylizedKitchen/Textures/`이며 MI parameter가 정본이다. 무늬 크기·색은 MI에서 조정한다.

## 지형(Q1 A)

- `Landscape`와 `LandscapeStreamingProxy` 63개(전체 64)의 Landscape Material은 `/Game/Bathhouse/Materials/World/M_Landscape_ProcGridHole`이다. `/Engine/OpenWorldTemplate/LandscapeMaterial/M_ProcGrid` 그래프 복제 + Blend Mode Masked + `LandscapeVisibilityMask` → Opacity Mask이며 바깥 지형 모습은 같다.
- 0회 홀 안쪽 바닥 아래 지형 구멍은 아직 칠해지지 않았다. 남은 칠하기는 [USER_UNREAL.md](../USER_UNREAL.md) `EXP-U1` 항목이며 그 전까지 계단 통로 장애물 Data Validation 오류가 남는다.

## 설비 정의 종류 태그

활성 Definition 16개(`/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_*`)의 `FacilityTags`에 기존 태그를 유지한 채 종류 태그 하나가 있다: ClothesLocker_1/_4/_8 → `Facility.Type.ClothesLocker`, 나머지 13개는 이름과 같은 `Facility.Type.<이름>`. opt-out `CleanTowelStack`·`UsedTowelBin`에는 종류 태그가 없다.

## Data Validation 현재 상태

- `Space_Bath`: 오류 0. 경고 2개(배치된 `UsedTowelBin`·`CleanTowelStack`이 허용 종류가 아님 — opt-out 정의에 종류 태그가 없어서다).
- `Space_Hall`·`Space_Work`: 오류 1개(계단 통로를 지형이 막음, 지형 구멍 미칠). 그 밖의 오류·경고 없음.
