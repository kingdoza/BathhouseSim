# PROMPT_UNREAL — EXP-U1 공간 건물 Editor 작업

- 작업 ID: `EXP-U1`
- 단계: 구현
- 상태: 완료

## 0. 상태·모드·기준선

- 상태: **작업 필요**(Content·Level 변경이 있다. Editor 작업 단계를 생략할 수 없다).
- 입력 근거: [PROMPT_IMPLEMENTATION.md](PROMPT_IMPLEMENTATION.md) 0.2·0.6·10절, [../PROMPT_ARCHITECTURE.md](../PROMPT_ARCHITECTURE.md) 4.1~4.7, 구조 정본 [BuildingSystem.md](../../../Architecture/BuildingSystem.md), 구현 결과 [PROMPT_REVIEW.md](PROMPT_REVIEW.md). Editor 사실 [../REPORT_UNREAL_DISCOVERY_2.md](../REPORT_UNREAL_DISCOVERY_2.md)(actor 이름·위치 표).
- 모드: Editor 작업([AGENT_UNREAL_EDITOR.md](../../../AGENT_UNREAL_EDITOR.md)). 코드 리뷰 승인 뒤에만 시작한다. PIE는 하지 않는다.
- 기준선: 작업 시작 때 `git status`(구현 단계 종료 시 `Content/` 변경 없음)와 Editor dirty package를 기록한다. 사용자 Editor가 같은 프로젝트에 열려 있으면 그 세션을 쓴다. 없으면 `.md/UNREAL_MCP_CONNECTION.md` 2절대로 작업용 숨김 Editor 한 개를 `-ModelContextProtocolStartServer`와 함께 띄운다(빌드는 이미 끝나 있어야 하며 C++ 모듈이 로드돼야 한다).
- 전제 확인(읽기만): Project Settings > Game > Bathhouse Building 값이 `Config/DefaultGame.ini` `[/Script/BathhouseSim.BathhouseBuildingSettings]`와 같고, GameplayTags에 `Facility.Type.*` 14개(ClothesLocker, DrinkFridge, MassageChair, RestBench, Television, Vanity, Bath, Shower, ScrubTable, Boiler, Cooler, Circulator, Washer, Dryer)가 있으며, `ABathhouseSpaceActor`가 클래스 목록에 있는지 본다. 하나라도 없으면 멈추고 보고한다(C++ 구현 단계 문제).
- 수치 원칙: 이 문서의 수치는 사용자 판단 제안값(상위 계약 4.7)과 계산식이다. 확정 값의 정본은 Level instance·asset이다. Unreal 정본 문서에는 수치를 복제하지 않고 원본 위치만 적는다.

## 1. 작업 순서(대표 먼저)

1. 대표 하나: `BP_BathhouseSpace` 생성·Compile·저장 → `Space_Hall` 하나만 DefaultMap에 놓고 저장·재로드해 `ABathhouseSpaceActor`가 Level에서 형상을 만드는지(뷰포트 벽·바닥·조명, Data Validation) 확인한다. World Partition external actor 저장 경로를 이 대표 actor로 먼저 검증한다.
2. 나머지 공간 2개, 재질, 설비 정의 태그, Level 정리, Nav, 지형을 같은 방식으로 확장한다.
3. 공간 3개 Data Validation 오류 0 → Unreal 정본 갱신.

## 2. allowlist

생성·수정·저장할 수 있는 대상(개별 Save만, `Save All` 금지):

| 구분 | 대상 |
|---|---|
| 새 Blueprint | `/Game/Bathhouse/Blueprints/Building/BP_BathhouseSpace` |
| 새 재질 | `/Game/Bathhouse/Materials/Building/` 아래 공간 재질(5절), `/Game/Bathhouse/Materials/World/` 아래 지형 재질(9절). 이름은 5·9절 제안을 따르되 바꾸면 보고에 적는다 |
| 새 Level actor(DefaultMap) | `Space_Hall`, `Space_Bath`, `Space_Work`(class `BP_BathhouseSpace_C`) |
| 수정 asset | `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_*` 활성 16개(6절) |
| 수정 Level actor | 7절 표의 이동·삭제·연결 대상, `NavMeshBounds`, `RecastNavMesh-Default`, Landscape 액터들(9절) |
| 쓰기 문서 | `.md/Unreal/BuildingSystem.md`(새), `0_UNREAL.md`, `WorldSystem.md`, `PlacementSystem.md`, `CleaningSystem.md`, `FacilitySystem.md`, `InteractionUISystem.md`(13절), `REPORT_UNREAL_EDITOR.md`, 필요 시 `USER_UNREAL.md` 항목 |

금지: `Source/`, `Config/`, Architecture 정본, `Content/` 중 allowlist 밖 asset, Blueprint graph로 형상·조명·조각을 만드는 것, 확장 정의 `DA_BathhouseExpansion_Default`·`DA_ShopCatalog`·`BP_BathhouseKeyRack`·`BP_BathhouseExpansionAuthority` 수정(U2·U3). 이미 놓인 청소 도구(집게·걸레·때수건)·`BP_CleaningDirector` Blueprint 값은 바꾸지 않는다(위치만 이동).

## 3. `BP_BathhouseSpace`

- Asset: `/Game/Bathhouse/Blueprints/Building/BP_BathhouseSpace`, Parent Class `/Script/BathhouseSim.BathhouseSpaceActor`.
- 상속 component만 쓰고 새 component·graph·변수를 만들지 않는다: `SpaceRoot`(root, Static), `ZoneBounds`(그 아래), `PlacementFloor`, `GridVisual`, `Shell`.
- `GridVisual`: Static Mesh `/Engine/BasicShapes/Plane.Plane`, Material element 0 `/Game/Bathhouse/Materials/Placement/MI_FacilityPlacementGrid`(기존 `BP_FacilityPlacementZone`과 같다).
- `ZoneBounds`: Z extent와 상대 위치(Z)는 기존 `/Game/Bathhouse/Blueprints/Placement/BP_FacilityPlacementZone`의 `ZoneBounds`와 같게 맞춘다(그 BP는 `ZoneBounds` Extent Z = 10, `PlacementFloor` identity — 읽어서 확인). XY extent는 C++가 `FloorSizeCm`로 덮으므로 값을 정하지 않는다.
- Class Default: `GridLineThicknessCm`·`GridZOffsetCm`·`MajorGridIntervalCells`는 `BP_FacilityPlacementZone` Class Default와 같게. `AllowedFacilityTags`는 비워 둔다(공간 instance가 채운다).
- Compile(`warnings_as_errors`)·개별 Save·재로드 후 Parent, `GridVisual` mesh·material 유지를 확인한다.
- 실행 경로 힌트: MCP·Python 모두 가능한 단순 BP 작업. 상속 component의 mesh·material 지정은 기존 `BP_FacilityPlacementZone`이 같은 방식으로 만들어졌다.

## 4. 공간 Actor 3개(DefaultMap)

모든 값의 정본은 각 Level instance Details의 `Bathhouse Space` 묶음이다. Rotation은 0, Scale은 1이다. 아래는 상위 계약 4.7 제안과 계산식이다. `T`는 Project Settings `WallThicknessCm`, `S`는 `SlabThicknessCm`(둘 다 읽어서 쓰고 코드·문서에 수치를 복제하지 않는다).

| 값 | `Space_Hall` | `Space_Bath` | `Space_Work` |
|---|---|---|---|
| `Space Kind` | 홀(Hall) | 목욕공간(Bath) | 작업공간(Work) |
| 안쪽 바닥 크기 `Floor Size Cm` | 21m×14m | 10m×13m | 14m×9m |
| 안쪽 바닥 위치 | X −800~1300(서쪽 끝 −8m, 동쪽 13m), Y −700~700 | 홀 동쪽에 맞닿음 | 홀 아래, 계단이 들어가는 위치(아래 참고) |
| Location XY | 안쪽 중심 `(250, 0)` | X = 홀 안쪽 동쪽 끝 + 2T + 안쪽 X/2 (= `1300 + 2T + 500`), Y 0 | 아래 참고 |
| Location Z(바닥 윗면) | `Zg` | `Zg`(홀과 같다) | `Zg − 4m` |
| `Ceiling Height Cm` | 3.5m | 3.5m | 3m |
| `Cleaning Chunk Kind` | 쓰레기 조각(Litter) | 물 얼룩 조각(Stain) | 없음(None) |
| `Allowed Facility Tags` | `Facility.Type.ClothesLocker`, `DrinkFridge`, `MassageChair`, `RestBench`, `Television`, `Vanity` | `Facility.Type.Bath`, `Shower`, `ScrubTable` | `Facility.Type.Boiler`, `Cooler`, `Circulator`, `Washer`, `Dryer` |

- `Zg`(지상 바닥 윗면 높이): 지형 면(world Z=0)보다 판 아랫면이 위에 오도록 `Zg − S > 0`이어야 한다(같으면 바닥과 지형이 겹쳐 깜빡인다). 출입구 문턱 높이도 `Zg`이므로 RecastNavMesh `AgentMaxStepHeight`와 캐릭터 `MaxStepHeight`를 읽어 `Zg`가 둘 이하인지 확인한다. 둘 다 만족하는 값이 없으면 멈추고 보고한다(설계 재검토 사항: 출입구 경사로·바닥 높이).
- 맞닿음 확인: 목욕공간 Location을 입력한 뒤 홀 통로 항목을 넣고 Data Validation을 돌린다. 안 맞닿으면 오류 문구에 맞닿게 하는 Location 값이 나온다. 그 값을 쓴다.
- 작업공간(지하) 위치: 바닥 깊이 4m(제안), 홀 아래. 계단이 홀과 작업공간 안쪽에 들어가고 계단 아래 출구 앞 공간이 있도록 정한다. 제안 구성: 작업공간 안쪽을 홀 안쪽 안에 두고(예: Location `(250, 200)` → 안쪽 X −450~950, Y −250~650), 계단을 북쪽 레인에 둔다(아래 계단 항목). 설비가 계단 레인과 겹치지 않으면 다른 값도 가능하다. 최종 위치는 Data Validation 오류 0으로 판정한다.

개구부·통로·계단(해당 공간 Actor `Openings`, `Stairs`):

| 공간 | 항목 | 값 |
|---|---|---|
| 홀 `Openings[0]` | 바깥 출입구 | `Side` 서(−X), `Connected Space` 비움, `Center Offset Cm` = 출입구 중심 Y − 홀 Location Y(제안: 출구 자리였던 Y≈300 부근), 폭 2m, 높이 2.6m |
| 홀 `Openings[1]` | 홀→목욕공간 통로 | `Side` 동(+X), `Connected Space` = `Space_Bath`, 중심 Y 0 부근(양쪽 벽 안쪽 길이 안), 폭 2m, 높이 2.6m. 통로는 홀에만 적는다(목욕공간 Openings는 비워 둔다) |
| 홀 `Stairs[0]` | 홀→작업공간 계단 | `Lower Space` = `Space_Work`, 내려가는 방향 `DownSide`와 `TopEdgeCenterOffsetCm`은 위 레인 제안에 맞춘다(제안: 동쪽으로 내려가고 맨 위 가장자리를 홀 북쪽 레인에 둠). `Width Cm`·`Run Cm`·`Step Count`·`Guard Height Cm`: 폭은 캐릭터 캡슐이 오르내릴 크기, 길이는 `Rise = Zg − Space_Work Z`에서 걸을 수 있는 최대 각도(캐릭터 이동 설정 `Walkable Floor Angle`)를 넘지 않게(Data Validation이 가파름을 알린다). 판 수는 한 단이 보행에 자연스러운 높이가 되도록, 난간은 허리 높이 정도 |
| 작업공간·목욕공간 `Stairs` | — | 비운다 |

계단 발자국 규칙(검증이 판정): 계단 벽까지 포함한 발자국이 홀·작업공간 안쪽 안에 들어오고, 위 입구 앞·아래 출구 앞에 계단 폭만큼 빈 바닥이 있어야 한다. 계단이 놓이는 홀 바닥에는 설비·카운터·컴퓨터·열쇠걸이를 두지 않는다(7절 이동 시 계단 레인을 피한다).

조명(`Lighting`): 공간마다 값을 정한다. 제안 출발점: 간격 5m 안팎, 밝기·닿는 거리는 천장 높이에서 바닥이 어둡지 않은 값, 그림자 끔, 천장에서 내린 높이는 천장 판 아래. 값은 사용자가 PIE에서 밝기를 보고 조정하므로 정한 값을 보고서에 적는다.

## 5. 재질

- 공간별 `Surfaces`(벽·바닥·천장)와 `Stairs[0]`의 `Step Material`·`Stair Wall Material`을 채운다. 비우면 Data Validation 경고(엔진 기본 격자 재질)다.
- 생성 형상은 하나의 상자를 늘려 쓰므로 UV가 늘어난다. world 위치 기준 투영 재질(WorldAlignedTexture)을 권장한다. 마스터 재질 하나(예: `/Game/Bathhouse/Materials/Building/M_Building_WorldAligned`)와 공간별 Material Instance로 만든다.
- 기본 제안(상위 계약 4.1): 홀 = 벽지 + 나무 바닥, 목욕공간 = 욕실 타일, 작업공간 = 거친 회색 마감. 텍스처 원본은 `/Game/StylizedKitchen`의 기존 재질·텍스처 중 가까운 것을 먼저 찾는다(사전 조사: 건물용 mesh는 쓸 수 없지만 재질은 재사용 후보). 맞는 것이 없으면 단색 임시 재질로 두고 보고서에 임시임을 적는다.
- 재질 asset은 Compile·저장·재로드를 확인한다. 재질이 비어 있는 Space는 Validation 경고로 남기지 말고 모두 채운다.

## 6. 설비 정의 16개 종류 태그

각 정의의 `Facility Tags`에 종류 태그 하나를 **추가**한다(기존 태그는 유지). 개별 Save·재로드.

| Definition(`/Game/Bathhouse/Data/Placement/`) | 추가 태그 |
|---|---|
| `DA_FacilityPlacement_ClothesLocker_1`, `_4`, `_8` | `Facility.Type.ClothesLocker` (세 정의가 같은 태그) |
| `DA_FacilityPlacement_DrinkFridge` | `Facility.Type.DrinkFridge` |
| `DA_FacilityPlacement_MassageChair` | `Facility.Type.MassageChair` |
| `DA_FacilityPlacement_RestBench` | `Facility.Type.RestBench` |
| `DA_FacilityPlacement_Television` | `Facility.Type.Television` |
| `DA_FacilityPlacement_Vanity` | `Facility.Type.Vanity` |
| `DA_FacilityPlacement_Bath` | `Facility.Type.Bath` |
| `DA_FacilityPlacement_Shower` | `Facility.Type.Shower` |
| `DA_FacilityPlacement_ScrubTable` | `Facility.Type.ScrubTable` |
| `DA_FacilityPlacement_Boiler` | `Facility.Type.Boiler` |
| `DA_FacilityPlacement_Cooler` | `Facility.Type.Cooler` |
| `DA_FacilityPlacement_Circulator` | `Facility.Type.Circulator` |
| `DA_FacilityPlacement_Washer` | `Facility.Type.Washer` |
| `DA_FacilityPlacement_Dryer` | `Facility.Type.Dryer` |

- `DA_FacilityPlacement_CleanTowelStack`, `DA_FacilityPlacement_UsedTowelBin`은 placement opt-out이라 태그를 추가하지 않는다.
- 저장 후 각 정의 Data Validation. 이 태그가 없으면 공간의 `IsDefinitionAllowed`가 거부한다.

## 7. Level 정리(DefaultMap, World Partition external actor)

저장 경로: 먼저 대표 actor(`Space_Hall`)로 검증한다. MCP `save_actor`의 external package 저장 실패가 이미 기록돼 있다(`.md/Unreal/WorldSystem.md`·`CleaningSystem.md`). Python `EditorLoadingAndSavingUtils.save_packages`가 그 경로의 검증된 선례다. 같은 `save_actor` 실패를 반복하지 않는다. `DefaultMap.umap` 자체 저장은 필요한 경우에만 확인 후 한다.

**삭제**(Blueprint asset은 유지): `PlacementZone`(`BP_FacilityPlacementZone_C_UAID_F02F7433CA36D1FF02_1155169559`), `LitterSpawnZone_Dressing`, `BathCleaningZone`, `DressingCleaningZone`, `Wall`(북쪽 임시 벽), `SM_Bath_old`, `SM_Bath_01_Body`, `Plane`, `SM_Kitchen_Countertop_C`, `SM_Kitchen_Countertop_C2`, `SM_Kitchen_Countertop_C3`, `SM_Kitchen_Countertop_C4`, `Studio_floor`, `Cooler_Lid`, `Cooler_Needle`, `Cooler_Needle_Mesh`, `RootNode`.

**이동**(현재 위치는 사전 조사 2차 보고서 표, 높이가 `Zg` 기준이 되도록 모든 Z에 `Zg`를 더한다 — 바닥에 닿는 pivot은 `Zg`, 선반 위 물건·컴퓨터는 기존 높이 + `Zg`). 설비·물건의 footprint가 소속 공간 안쪽 안에 완전히 들어가야 하고(`ContainsFootprint`), 서로·벽·계단 레인과 겹치지 않아야 한다. 같은 군집의 상대 배열은 가능한 유지한다.

| 소속 | actor(label) |
|---|---|
| 홀 | `Counter`, `Computer`, `KeyRack`, `BP_ShopDeliveryPoint`, `BP_DrinkCollectionBox`, `ShoeLocker_1`, `ShoeLocker_2`, `ClothesLocker_1`, `ClothesLocker_2`, `MonkeyWrenchFixedSlot`+`BP_MonkeyWrench`, `WetMopFixedSlot`+`WetMop`, `LitterTongsSlot`+`LitterTongs`, `PlayerStart`(카운터 근처), 장식 `SM_Fridge`·`cleaner`(홀 안으로), `CustomerQueueOverflowWanderVolume_Checkout_0`(카운터 옆, 크기·상대 위치 유지) |
| 목욕공간 | `Shower`, `Bath`, `Bath2`(`BP_Bath`는 footprint가 크니 공간 안쪽 안에 들어오게), `CleanTowelStack`(수건 공급대), `UsedTowelBin`(사용 수건함), `ScrubTowelSlot`+`ScrubTowel`(때수건 거치대). 장식 `boiler` mesh(`/Game/Bathhouse/Meshes/boiler`)는 계약 목록에 없어 그대로 두되, 공간 벽·설비와 겹치면 멈추고 보고한다(설계 S2) |
| 작업공간(지하) | `BP_Circulator`, `BP_Cooler`, `BP_Boiler`, `CoalSupply`, `BP_DryIceSupply`, `ShovelSlot`+`BP_UtilityShovel`, `Washer`, `Dryer`, `DryingSpot`, `TowelBasketFixedSlot`+`TowelBasketCart` |
| 출입구 밖 마당(홀 서쪽 출입구 정면 바깥) | `Spawner`(손님 생성기, `BP_BathhouseCustomerSpawner_C`), `Exit`(`BP_BathhouseExit_C`, 퇴장 지점), `TrashCollectionZone`(`BP_TrashCollectionZone_C`, 쓰레기 수거 구역). 마당 바닥은 지형(Z 0)이다. 이 셋은 Nav 범위 안에 있어야 한다 |
| 위치 무관(그대로) | `ExpansionAuthority`, `CleaningDirector`, `RecastNavMesh-Default`, 조명 6개(`Lighting` 폴더) |

- 확인(이동 뒤): ① 각 설비 Approach point가 Nav 위에 남는다(Nav 재생성 뒤 투영 가능), ② 고정 거치대(`*FixedSlot`)와 그 위 물건이 함께 이동했다(`AssignedItem`·`ItemAnchor` 상대 0 유지), ③ 침대형 큰 footprint가 벽·계단 구멍과 겹치지 않는다, ④ `ClothesLocker_1/2`의 `RegistrationId` 등 instance 값은 건드리지 않는다.
- 컴퓨터 연결: `Computer`(`BP_BathhouseComputer_C_UAID_F02F7433CA3690F802_2051456727`)의 `Managed Bath Placement Zone`을 `Space_Bath`로 다시 연결한다(삭제된 `PlacementZone` 참조 제거).
- 위치 이동 전 각 actor의 위치·회전·scale·고유 override를 기준선으로 저장하고(백업 스크립트·JSON), 이동 뒤 같은 항목을 대조한다. 한 번에 한 군집씩 처리하고 군집마다 위치·Z·footprint를 읽어 확인한다.

## 8. 손님 길(Nav)

- `NavMeshBounds`(`NavMeshBoundsVolume`)를 조정한다. 요구(검증이 판정): 홀·목욕공간 바닥 직사각형 전체와, 홀 바깥 출입구 앞 바깥 지점(출입구 폭만큼 바깥)을 한 volume이 덮는다. Z 범위는 지상(마당 지형 Z와 `Zg` 바닥을 포함)만 덮고 작업공간 바닥을 넣지 않는다. 제안: X −1600~2500, Y −1000~1000, Z −100~600 정도(마당·출입구 앞·목욕공간 동쪽 끝과 앞으로의 넓힘 여유를 포함). brush scale·위치로 이 world 범위를 맞춘다.
- `RecastNavMesh-Default`(`RecastNavMesh_UAID_F02F7433CA3615F402-Default`): `Runtime Generation`을 `Dynamic`으로 바꾼다(이전 `USER_UNREAL.md` 2항 항목 해소; external actor 저장은 Python `save_packages` 경로 확인). 자동화로 저장이 안 되면 그 항목을 유지한다.
- 저장 뒤 재로드해 `Dynamic`이 유지되는지 확인한다.

## 9. 지형(Q1 A)

- 현재 지형 재질은 `/Engine/OpenWorldTemplate/LandscapeMaterial/M_ProcGrid`다(`Landscape` 1개와 `LandscapeStreamingProxy` 63개, 놀이 영역 아래 4개 proxy의 bounds Z −100~100). 읽어서 확인한다.
- 같은 모습에 지형 구멍을 지원하는 프로젝트 재질을 만든다(제안: `/Game/Bathhouse/Materials/World/M_Landscape_ProcGridHole`): `M_ProcGrid` 그래프를 복제하고 Blend Mode Masked, `Landscape Visibility Mask` 노드를 Opacity Mask에 연결한다. 모습이 바뀌지 않아야 한다(바깥 지형 색·격자·밝기 대조).
- Landscape 액터들의 재질을 새 재질로 지정한다(proxy가 부모 재질을 따르는지 확인).
- 구멍 칠하기: **0회 홀 안쪽 바닥 아래**(홀 안쪽 직사각형, 홀 바닥 판이 가리는 범위)를 지형 구멍으로 칠한다. 계단을 0회 홀 안쪽에서 옮겨도 다시 칠하지 않게 홀 안쪽 전체다. 칠할 때 Data Validation이 알리는 계단 통로 장애물 오류가 사라지는지 확인한다. 바깥 지형은 바뀌지 않아야 한다.
- 실행 경로 힌트: 재질 생성·그래프 연결·재질 지정은 Python으로 가능할 가능성이 높다. 구멍 칠하기(Landscape Visibility 레이어)는 공식 Python API로 되는지 알려져 있지 않다. 자동화가 안 되면 칠하기만 `USER_UNREAL.md`에 남기고(영역: 홀 안쪽 XY 직사각형, 영향 proxy 목록 포함) 나머지는 끝낸다. 이 경우 계단 통로 오류가 남으므로 보고에 명시한다.

## 10. 검증

- `Space_Hall`, `Space_Bath`, `Space_Work` 각각 Data Validation 오류 0. 경고는 사유를 보고서에 적는다(예: 장식 `boiler` 관련, 임시 재질).
- Blueprint Compile(`warnings_as_errors`), 재질 Compile, 정의 asset 저장 확인, 각 package 개별 Save, 디스크 재로드 후 값·연결 재검사.
- 저장 후 dirty package와 `git status`를 기준선과 비교한다. 예상 밖 dirty는 저장하지 않는다.
- 실행하지 못한 검증(PIE, 렌더 확인)을 통과로 쓰지 않는다.

## 11. 멈춰야 하는 조건

- 전제 확인 실패(Settings·태그·클래스), `Zg`가 문턱 높이 조건을 만족하지 못함, 장식 `boiler`가 공간 형상과 겹침, 설비 footprint를 공간 안에 넣을 수 없음, 계단이 어떤 위치에서도 규칙을 만족하지 못함 → 영향받는 asset만 저장하지 않고 멈춰 보고한다(상위 결함은 Blueprint·값으로 우회하지 않는다).
- `save_actor` external package 저장 실패를 같은 방식으로 반복하지 않는다. 원인 확인 포함 두 번까지.
- 코드·설계 문제(예: 형상·검증 오류가 계약과 다름)는 소유 단계(구현·아키텍처)로 반환한다.

## 12. Blueprint에서 구현하면 안 되는 것

벽·바닥·천장·조명·계단·개구부·청소 조각 생성, 공간 크기 동기화, 설비 허용 판정, 검증은 모두 C++(`ABathhouseSpaceActor`, `UBathhouseSpaceShellComponent`, `FBathhouseSpaceLayout`, `FBathhouseSpaceValidation`)이 소유한다. `BP_BathhouseSpace`에 graph·구성 스크립트·추가 component를 두지 않는다. 설비 허용 표는 공간 instance `Allowed Facility Tags`와 정의의 종류 태그만으로 표현한다.

## 13. 갱신할 Unreal 정본(재로드로 확인된 상태만, 수치 복제 없음)

- 새 `.md/Unreal/BuildingSystem.md`와 `0_UNREAL.md` 라우팅 표 링크: `BP_BathhouseSpace` 구조, 공간 3개 Level instance(이름·값 원본 위치), Project Settings 값 위치, 재질 asset 목록, 지형 재질·구멍 상태, 설비 정의 종류 태그 표.
- `WorldSystem.md`: PlacementZone 단일 기록 대체(공간 3개), Recast `Dynamic` 상태, `NavMeshBounds` 범위 계약, DefaultMap 정리 결과.
- `PlacementSystem.md`(Unreal): Placement Zone 절에 공간 Actor가 구역임을, 정의 종류 태그를 반영.
- `CleaningSystem.md`: DefaultMap 구역 instance 제거(`LitterSpawnZone_Dressing`, `BathCleaningZone`, `DressingCleaningZone`), 바닥이 `Studio_floor`라는 낡은 기록 정정(바닥은 공간 바닥 ISM과 지형).
- `FacilitySystem.md`: 정의 종류 태그, 설비 위치 이동.
- `InteractionUISystem.md`: 컴퓨터 `ScreenWidget`의 WidgetClass 낡은 기록을 실제 값 `WBP_ComputerScreenRoot_C`로 정정(상위 계약 6절).

## 14. 사용자 PIE에서 관찰할 항목(시나리오 ID별)

| ID | 관찰 | 기대 결과 |
|---|---|---|
| EXP-001 | 홀·목욕공간·지하를 둘러본다 | 직사각형 벽·바닥·천장으로 닫혀 있고 틈·구멍·깜빡임이 없고 실내가 밝다. 바닥·지형이 깜빡이지 않고 계단 구멍 안에 지형이 보이지 않는다 |
| EXP-002 | 서쪽 출입구로 나갔다 들어온다. 묶은 봉투를 마당 수거 구역에 둔다 | 문짝 없이 지나가고, 봉투가 지금처럼 수거된다 |
| EXP-003 | 홀↔목욕공간 통로를 지난다 | 막힘 없이 오간다 |
| EXP-004(대표) | 석탄·수건 바구니·설비 아이템을 들고 계단으로 지하를 오르내린다 | 계속 들고 있고 떨어뜨리거나 끼이지 않는다 |
| EXP-005 | 물건을 벽·천장 쪽으로 던지거나 떨어뜨린다 | 벽·천장을 뚫지 않는다 |
| EXP-006 | 공간별 설비·물건 위치, 장식·임시 벽 | 7절 표대로이고 건물 밖 장식·북쪽 임시 벽이 없다. 시작 위치는 카운터 근처다 |
| EXP-007(대표) | 손님 한 명을 처음부터 끝까지 지켜본다 | 출입구 밖에서 생겨 체크인 → 락커 → 샤워·입욕 → 계산 → 출입구로 퇴장한다. 지하에 가지 않고 끼이지 않는다 |
| EXP-008 | 욕탕·샤워기·보일러·락커·음료 냉장고 아이템으로 홀·목욕공간·지하를 조준한다 | 맞는 공간만 설치되고 아니면 `이 공간에는 놓을 수 없는 설비입니다`. 벽 너머 공간의 배치 격자가 조준되지 않는다 |
| EXP-009 | 지하 세탁기를 회수해 홀에 놓으려 한다 | 불가 문구, 지하에는 다시 놓인다 |
| EXP-010 | 손님이 머무는 동안 시간이 지난다 | 홀에는 쓰레기만, 목욕공간에는 물 얼룩만 생기고 벽에 묻히지 않는다. 지하에는 생기지 않는다 |
| EXP-011 | 지하 보일러·쿨러·순환기에 삽으로 석탄·드라이아이스를 넣고 레버를 돌린다 | 지금처럼 작동한다 |
| EXP-012 | 사용 수건함의 수건을 바구니로 지하 세탁·건조를 거쳐 공급대에 채운다 | 수건 순환이 끝난다 |
| EXP-013 | 컴퓨터 관리 탭의 욕탕 지도를 본다 | 목욕공간 기준으로 욕탕이 표시되고 설정이 동작한다 |
| EXP-014 | 계산대 줄이 넘치게 한다 | 카운터 옆 대기 배회 구역에서 서성인다 |
| EXP-015 | 주문·배송·개봉 | 배송 지점(홀)에서 지금처럼 동작한다 |

## 15. 결과 보고

`REPORT_UNREAL_EDITOR.md`(Editor 워커는 첫머리를 갖춘 전문을 보고 뒤에 붙이고 마스터가 저장)에 완료/부분 완료/중단, exact 변경 대상, 정한 값(공간 크기·위치·조명·재질·`Zg`·계단·Nav 범위), 실행 경로·스크립트·로그·백업, Compile·Validation·Save·재로드 결과, 남은 `USER_UNREAL.md` 항목을 쓴다.
