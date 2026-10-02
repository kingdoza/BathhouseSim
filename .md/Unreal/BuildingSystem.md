# Building Editor Authoring

공간 건물(홀·목욕공간·작업공간)의 Editor 구조다. C++ 책임·형상 규칙·검증 계약은 [Architecture/BuildingSystem.md](../Architecture/BuildingSystem.md)가 정본이다. 조정값 수치는 적지 않고 원본 위치만 적는다. 저장·새 프로세스 재로드로 확인한 상태다(`EXP-U1`, 넓힘 목록은 `EXP-U2`).

## 공용 값 원본

- Project Settings > Game > Bathhouse Building(`Config/DefaultGame.ini` `[/Script/BathhouseSim.BathhouseBuildingSettings]`): 벽 두께, 바닥·천장 판 두께, 형상 상자 mesh(`/Engine/BasicShapes/Cube`), 조각 최대 크기, 쓰레기 조각 class `BP_LitterSpawnZone_C`, 물 얼룩 조각 class `BP_StainSpawnZone_C`, 넓힘 미리보기 글자 높이 여유·크기·해상도 `EditorPreviewLabelHeightCm`·`EditorPreviewLabelWorldSizeCm`·`EditorPreviewLabelFontSize`(원본은 이 ini 같은 섹션, 편집 화면 전용).
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
- `Space_Hall` `Stairs[0]`: `LowerSpace` = `Space_Work`, 동쪽으로 내려가며 맨 위 가장자리가 홀 남쪽 레인에 있다. 위치(`TopEdgeCenterOffsetCm`)·폭·길이·판 수·재질의 정본은 이 항목이다(2026-10-02 사용자 결정으로 칠한 지형 구멍 안에 들도록 북쪽으로 옮김). 다른 공간 `Stairs`는 비어 있다.
- `AllowedFacilityTags`: 홀 `ClothesLocker`·`DrinkFridge`·`MassageChair`·`RestBench`·`Television`·`Vanity`, 목욕공간 `Bath`·`Shower`·`ScrubTable`, 작업공간 `Boiler`·`Cooler`·`Circulator`·`Washer`·`Dryer`(모두 `Facility.Type.` 접두).
- `Lighting`: 원본은 각 공간 instance `Lighting`이다(그림자 끔, 밝기는 2026-10-02 사용자가 PIE에서 조정). 화면 밝기는 Project Settings 자동 노출 끔(`Config/DefaultEngine.ini` `r.DefaultFeature.AutoExposure`)을 전제로 맞춘 값이다.
- 컴퓨터 `ManagedBathPlacementZone`은 `Space_Bath`다([InteractionUISystem.md](InteractionUISystem.md)).

### 넓힘 목록(`Expansion Steps`)

- 원본: 각 instance Details `Bathhouse Space › Expansion`의 `Expansion Steps`(줄 = 그 공간의 몇 번째 넓힘, `Side`·`Amount Cm`). 넓힘 양 값은 이 instance 값이 정본이다. 줄 수 = 공간별 넓힘 횟수 상한이고 세 공간 합이 확장 정의 `Max Purchase Count` 이상이다([FacilitySystem.md](FacilitySystem.md)).
- 현재 방향(줄 순서): `Space_Hall` 남 → 북(동쪽 목욕공간 맞닿음·서쪽 출입구 제외), `Space_Bath` 북 → 남(서쪽 홀 맞닿음 제외), `Space_Work` 서 → 동(지하, 서쪽 띠는 홀·마당 지형 아래, 동쪽 띠는 목욕공간 바닥 아래에 묻힌다).
- 끝 모습(모든 줄 적용)의 지상 넓힘 띠에는 Level Actor(편집 전용 sprite·frustum 제외)가 없고 지형 면은 지상 바닥 판 아래다(편집 world object trace).
- `Editor Preview Expansion Count`(Transient, 편집 전용)는 넓힌 모습 미리보기용이며 저장 상태는 세 공간 모두 0이다. 1 이상이면 벽·바닥·천장·조명·`ZoneBounds`가 넓힌 모습을 따르고, 편집 전용 Transient `UWidgetComponent` 글자 `넓힘 미리보기 N회`가 모든 공간 중 가장 높은 천장 판 윗면 + 높이 여유에 뜬다. 아래층(지하) 글자는 안쪽 중심 남쪽, 나머지는 북쪽에 붙고 위에서 읽힌다(위쪽 북). 사용자가 보이는 Editor에서 확인했다(`EXP-U2`).
- 판정 경로 주의: 작업용 숨김 Editor에서는 위젯 component가 그려지지 않으니(render target 미생성) 미리보기 글자의 화면 판정은 사용자 확인으로 한다(FBK-003).

## 재질

| Asset | 용도 |
|---|---|
| `/Game/Bathhouse/Materials/Building/M_Building_WorldAligned` | world 기준 3면 투영(`WorldAlignedTexture`) 마스터. parameter `BaseTexture`, `TileSizeCm`, `Tint`, `Desaturation`, `Roughness`. Used with Instanced Static Meshes |
| `MI_Building_Hall_Wall` / `_Floor` / `_Ceiling` | 홀 `Surfaces`(벽지·나무 바닥·천장, StylizedKitchen 텍스처) |
| `MI_Building_Bath_Wall` / `_Floor` / `_Ceiling` | 목욕공간 `Surfaces`(욕실 타일·석고 천장) |
| `MI_Building_Work_Wall` / `_Floor` / `_Ceiling` | 작업공간 `Surfaces`(회색 거친 마감, desaturate+tint). 계단 `StepMaterial` = `MI_Building_Work_Floor`, `StairWallMaterial` = `MI_Building_Work_Wall` |

텍스처 원본은 `/Game/StylizedKitchen/Textures/`이며 MI parameter가 정본이다. 무늬 크기·색은 MI에서 조정한다.

## 지형(Q1 A)

- `Landscape`와 `LandscapeStreamingProxy` 63개(전체 64)의 Landscape Material은 `/Game/Bathhouse/Materials/World/M_Landscape_ProcGridHole`이다. `/Engine/OpenWorldTemplate/LandscapeMaterial/M_ProcGrid` 그래프 복제 + Blend Mode Masked다. 이 재질은 Material Attributes를 쓰므로 Opacity Mask 핀이 무시된다. 그래서 기존 attribute 출력 → `BreakMaterialAttributes` → `MakeMaterialAttributes`(OpacityMask = `LandscapeVisibilityMask`, 나머지 attribute는 그대로 전달) → 출력으로 연결했다(같은 세션에서 칠한 Visibility 자리가 화면에서 뚫리는 것 확인, 새 프로세스 재로드 VALID). 바깥 지형 모습은 같다.
- 지형 구멍 레이어는 엔진 공식 `/Engine/EngineResources/LandscapeVisibilityLayerInfo`가 자동 지정된다(별도 layer info asset 없음).
- 지형 구멍(Visibility)은 사용자가 Landscape 모드로 칠했다(저장 proxy `LandscapeStreamingProxy_3_3_0` `/Game/__ExternalActors__/Maps/DefaultMap/E/5Y/MJQIJ8RYADZHO7ZS6IE5NM`, `_4_3_0` `…/B/NX/PF1O5HWD53VE14YXX78A8H`). 구멍은 홀 계단 구멍과 그 주변을 덮고 홀 벽 밖에는 없다. 홀 안쪽 나머지는 칠하지 않았다(비고: 홀 바닥 판이 가려 보이지 않음). 계단을 옮기면 새 계단 통로가 구멍 안에 드는지 Data Validation이 알린다.
- Editor 확인 방법: 편집 world에서는 Landscape에 구멍 없는 Editor 전용 heightfield가 Visibility 채널로 붙고(엔진 `LandscapeCollision.cpp`), 로드된 `WorldPartitionHLOD` 지형 mesh가 complex 충돌로 맞는다. 게임 충돌 확인은 WorldStatic·WorldDynamic object type의 단순 충돌 trace로 한다(공간 Validation 계단 통로 검사와 같은 방식).

## 설비 정의 종류 태그

활성 Definition 16개(`/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_*`)의 `FacilityTags`에 기존 태그를 유지한 채 종류 태그 하나가 있다: ClothesLocker_1/_4/_8 → `Facility.Type.ClothesLocker`, 나머지 13개는 이름과 같은 `Facility.Type.<이름>`. opt-out `CleanTowelStack`·`UsedTowelBin`에는 종류 태그가 없다.

## Data Validation 현재 상태

넓힘 검사(목록 끝 모습 포함)까지 적용된 상태다.

- `Space_Bath`: 오류 0. 경고 2개(배치된 `UsedTowelBin`·`CleanTowelStack`이 허용 종류가 아님 — opt-out 정의에 종류 태그가 없어서다).
- `Space_Hall`·`Space_Work`: 오류 0, 경고 0.
- 끝 모습 손님 길 범위 검사를 위해 `NavMeshBounds` Y 범위를 넓혔다([WorldSystem.md](WorldSystem.md) Navigation).
