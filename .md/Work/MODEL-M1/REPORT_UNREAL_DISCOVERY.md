# REPORT_UNREAL_DISCOVERY — MODEL-M1 설비 스태틱 메시 모델링용 Blueprint 실측
- 작업 ID: `MODEL-M1`
- 단계: Editor 사전 조사
- 상태: 완료

(Editor 워커 전문을 마스터가 저장)

## 범위·방법·기준선
- 시작 커밋 `7c53d88`(work/EXP-U1). 시작 시 미추적 파일은 `.md/Work/MODEL-M1/`, `ArtSource/Bathhouse/_Pipeline/`(이번 작업과 무관)이고 Content 변경은 없었다.
- 사용자 허용 범위는 Editor 읽기 전용 조사다. Content 수정·import·Level 변경과 PIE는 하지 않았다.
- Editor 실행: 대상 Editor가 없어서 작업용 숨김 Editor 1개(PID 29352)를 아래 인자로 띄웠다.
  - 인자: `-ModelContextProtocolStartServer -NoSplash -log -ExecCmds="py m1_harness.py"`
  - `m1_harness.py`는 파일 큐 실행기로 EXP-U1 harness의 사본이다. MCP 도구는 쓰지 않았다(Python으로 충분).
  - 종료는 `QUIT_EDITOR`로 정상 종료했고 PID 소멸을 확인했다.
- API: SubobjectDataSubsystem(BP template), CDO/DataAsset `get_editor_property`, `LineTraceSingle`(Visibility, 조회만), `ProceduralMeshLibrary.get_section_from_static_mesh`(section bounds), `StateTreeEditorData`.
- 각 스크립트 끝 dirty content/map은 0이었다.
- raw 값은 `Saved/Claude/MODEL-M1/01_bp.json`, `02_level.json`, `03_data.json`, `04_mesh.json`이고, 스크립트와 순서는 같은 폴더 `README.md`에 있다.
- 좌표 기준: 아래 값은 모두 cm, **SceneRoot 로컬**(= 설비 actor 로컬, 바닥 Z=0)이다. 회전은 (Pitch, Yaw, Roll)로 적는다.
  - `BP_Boiler`·`BP_Circulator`는 SceneRoot 자체가 yaw −1°라서 actor 로컬과 1° 어긋난다. footprint도 SceneRoot 자식이므로 mesh는 SceneRoot 기준으로 만들면 된다.
- C++ 회전 규약: `FUtilityPivotRotation`은 `Baseline * Quat(축, 각)`이다. 축은 pivot 자신의 로컬축이다.
  - +Y축으로 θ만큼 돌리면 +X → (cosθ, 0, −sinθ), +Z → (sinθ, 0, cosθ)가 된다.
  - +Z축으로 +90° 돌리면 +X → +Y가 된다.
  - UE는 왼손 좌표계라서 **−Y 쪽에 서서 +Y를 보면 +X가 화면 왼쪽**이다.

## 1. 좌표축 판정 (Blender → 이 프로젝트 Unreal)
| 근거 mesh | 원본 | import 옵션(읽은 값) | 판정 |
|---|---|---|---|
| `SM_Bath_01_Body/Details/Hardware/Water` (`/Game/Bathhouse/Meshes/Bath/Bath_01/StaticMeshes/`) | `ArtSource/Bathhouse/Bath_01/Bath_01.glb` (Interchange glTF) | GenericAssetsPipeline offset rot (0,0,0)·trans 0·uniform scale 1, bake pivot off | Blender 배지 (0.96, −1.741, 0.665)m → UE Details section x 67~124, y 51~178, z 33~81. Blender 앞 계단(y −2.64) → UE Body max Y +264. **UE = (Bx, −By, Bz)×100** |
| `Cooler_Body/Lid/Needle` | `C:/Users/kdowo/Downloads/cooler-sky-blue-parts.fbx`(저장소 밖) | Interchange FBX, offset 0/1 | 투입 뚜껑·바늘이 mesh **+Y** 면(y≈53)에 있다. Blender 관례(정면 −Y)와 위 변환에 맞는다(원본 방향은 미확인) |
| `/Game/Bathhouse/Meshes/boiler`(장식) | `C:/Users/kdowo/Downloads/boiler.fbx` | Interchange FBX, offset 0/1 | bounds (−28.3,−20.4,0)~(32.5,22.4,102.7), section 1개라 정면 판정 불가 |
| `SM_Bath_old` | `C:/Users/kdowo/3D Objects/Bathhouse/Bath/Bath_old.fbx` | 같은 값 | bounds (−47.7,−38.5,−0.3)~(46.1,40.9,25.2), 바닥 pivot, convex 1 |
| `SM_Towel_clean/used/wet_common` | `.../Towel/towel_clean_bakset.fbx` | 같은 값 | 190×177.1×24.8, 중심 pivot |
| `SM_Towel_clean_good` | `.../Towel/towel_clean_shelf.fbx` | 같은 값 | 189.8×190×35, 중심 pivot |
| `SM_Facility_sample` | 원본 경로 없음 | 구형 FbxStaticMeshImportData: ConvertScene true, ForceFrontXAxis **false**, ConvertSceneUnit false, rot 0, scale 1 | 100³, 바닥 pivot |

- Interchange FBX translator 설정(Force Front X Axis, Convert Scene)은 asset에서 읽히지 않았다(`get_translator_settings` 반환 None). 엔진 기본값은 Force Front X off다.
- `ArtSource`의 `SM_Boiler_01.fbx`(axis_forward −Y, up Z)와 Bath_01 FBX 3개는 프로젝트에 import돼 있지 않다. 그래서 **FBX 경로의 축은 실측 근거가 없고** glb 실측과 표준 동작으로 추정했다.
  - 검증하려면 Blender에서 비대칭 테스트 상자를 FBX(axis_forward −Y, up Z, apply unit)로 내보내 import해 봐야 하는데, 이번 허용 범위 밖이다.
- 모델링 규칙: 아래 표의 UE 좌표 (x, y, z)를 Blender (x/100, −y/100, z/100)m에 놓는다.
  - UE 정면 −Y(보일러·순환기)는 Blender **+Y**이고, UE 정면 +X(화장대·샤워기·락커)는 Blender **+X**다.
- 쿨러 pivot:
  - `Cooler_Lid`: X 0~75.5, Y ±8.17, Z ±31.7. pivot은 경첩 모서리(X=0)이고 두께·높이의 중앙이며, 판은 +X 쪽으로 뻗는다.
  - `Cooler_Needle`: X −4.0~9.0, Y ±1.05, Z −4.1~9.7. pivot은 허브 중심(section 1이 ±2.1인 허브)이고 바늘은 +X+Z 대각선을 향한다. 두께 방향(법선)은 Y다.
  - `Cooler_Body`: (−69,−51.5,0)~(97.4,57.5,273.7), 재질 슬롯 8개.

## 2. BP_Boiler (`/Game/Bathhouse/Blueprints/Facility/BP_Boiler`, parent `BathWaterBoilerFacilityActor`)
트리: PackagePhysicalRoot(Box, root) > SceneRoot(yaw −1°) > 아래 component들. 모두 native component다(GaugeFacePlate만 SCS).
| component | 부모 | class | rel loc | rel scale | mesh·실크기 | collision / hidden |
|---|---|---|---|---|---|---|
| VisualMesh | SceneRoot | StaticMesh | (0,0,0) | (1,0.6,1.2) | SM_Facility_sample → 100×60×120 (X±50,Y±30,Z0~120) | BlockAllDynamic, Nav on |
| PlacementFootprint | SceneRoot | Box ext (50,30,60) | (0,0,60) | 1 | 100×60×120 | NoCollision, hidden |
| FuelIntake | SceneRoot | UtilityFuelIntakeComponent | (0,−33,42) | (0.25,0.06,0.18) | Cube 25×6×18 (y −36~−30) | NoCollision |
| FuelIntakeVolume | SceneRoot | UtilityFuelIntakeVolume(Box) | (0,−42,42) | 1 | ext (15,4,11) → x±15, y −46~−38, z 31~53 | QueryOnly, hidden |
| FuelDoorPivot | SceneRoot | Scene | (12.5,−37,42) | 1 | 경첩 = 판의 +X 모서리(정면에서 보면 **왼쪽**) | — |
| FuelDoorMesh | FuelDoorPivot | StaticMesh | pivot 상대 (−12.5,0,0) | (0.25,0.02,0.18) | Cube 25×2×18, 판이 pivot에서 −X로 뻗음 | NoCollision |
| GaugeFacePlate | SceneRoot | StaticMesh | (0,−32,90) | (0.3,0.02,0.3) | Cube 30×2×30 (y −33~−31, z 75~105) | NoCollision |
| GaugeNeedlePivot | SceneRoot | Scene | **(67.77,−34,129.13)** | 1 | 본체(X±50, Z≤120) 밖. 문판(0,−32,90)과 떨어져 있음 | — |
| GaugeNeedleMesh | GaugeNeedlePivot | StaticMesh | pivot 상대 (7,0,0) | (0.14,0.02,0.015) | Cube 14×2×1.5, pivot에서 +X로 뻗음 | NoCollision |
- 문(`FuelDoorPresentation`): 축은 pivot 로컬 Z, 열림 +90°, 열기·닫기 각 0.2초다. 판 방향 (−1,0,0)이 (0,−1,0)으로 바뀌므로 **정면(−Y) 쪽으로 바깥 열림**이다.
- 바늘(`GaugePresentation`): 축은 pivot 로컬 **Y**(문판 법선)이고 Zero −30°, Max −150°, ActiveStartRatio 1/3이다.
  - 계산식: 각 = Lerp(Zero, Max, f). 운전량이 0이면 f=0이고, 0보다 크면 f = 1/3 + 2/3×잔량비다.
  - 바늘 방향: −30° = (+X, 위 30°), −90° = 정위, −150° = (−X, 위 30°).
  - 정면에서 보면 비었을 때 10시, 가동을 시작하면 약 11시 20분(−70°), 가득 차면 2시 방향이다. 위쪽 120° 호를 시계 방향으로 돈다.
- 운전: Max 100점, 초당 1점 감소. Capacity HEATING 100.
- 재질: VisualMesh 슬롯 1개(`WorldGridMaterial`, override 없음). Cube 부품도 모두 슬롯 1개 WorldGridMaterial.
- 정면: 투입구·문·계기가 모두 **−Y** 면(y −30)에 있어 플레이어는 −Y 쪽에서 조작한다.
- Level 배치: `BP_Boiler` (299,115,−375), yaw 0, scale 1, Space_Work.
  - +Y 벽까지 135(등면 y+30에서 105 떨어짐), −Y는 400 안에 막힘 없음.
  - 왼쪽(−X) 236에 쿨러, +X 323에 드라이아이스 공급함이 있다.
- 배치 정의 `DA_FacilityPlacement_Boiler`: extent·오프셋 필드는 없고 footprint는 BP component가 정한다. Grid 20cm, 회전 단위 15°.

## 3. BP_Circulator (`.../Facility/BP_Circulator`, parent `BathWaterUtilityFacilityActor`)
트리: PackagePhysicalRoot > SceneRoot(yaw −1°) > 아래. 모두 native다.
| component | 부모 | rel loc | scale / ext | 실크기·비고 |
|---|---|---|---|---|
| VisualMesh | SceneRoot | (0,0,0) | (1.2,0.8,1.2) | SM_Facility_sample → 120×80×120 (X±60,Y±40,Z0~120), BlockAllDynamic·Nav on |
| PlacementFootprint | SceneRoot | (0,0,60) | ext (60,40,60) | 120×80×120, NoCollision |
| LeverPivot | SceneRoot | **(30,−44,30)** | 1 | 정면(−Y) 면에서 4cm 앞, 바닥 위 30 |
| LeverMesh | LeverPivot | pivot 상대 (0,0,25) | (0.04,0.02,0.25) | Cube 4×2×25, pivot 위 12.5~37.5에 떠 있음(축과 붙어 있지 않음). NoCollision |
| LeverOperatingVolume | SceneRoot | (19,−44,42) | ext (16,6,16) | x 3~35, y −50~−38, z 26~58, QueryOnly, hidden |
| GaugeNeedlePivot | SceneRoot | (34.49,−44,109.92) | 1 | 정면에서 4cm 앞, 상단 아래 10. 계기판 mesh는 없음 |
| GaugeNeedleMesh | GaugeNeedlePivot | 상대 (7,0,0) | (0.14,0.02,0.015) | Cube 14×2×1.5 |
- **레버(`LeverLabor`)**: 축은 LeverPivot 로컬 **Y(0,1,0)**(정면 법선 방향)이고 DownAngle **−60°**다.
  - +Z(레버 위쪽)가 (−0.87,0,0.5)로 기운다. 정면(−Y)에서 보면 **똑바로 위에서 오른쪽(−X)으로 60° 내려간다**(수평 위 30°에서 멈춤).
  - 1회 1초 동안 0→−60°(0.5초)→0(0.5초)으로 삼각 왕복하고, 1회에 10점을 준다. 취소 복귀 시간(CancelReturnSeconds)은 0초(즉시)다.
  - 회전 반경: 레버 끝은 pivot에서 37.5다. −X로 32cm까지 흔들리므로 레버 앞·오른쪽을 비워 둔다.
- 바늘: 축 Y, Zero −30°, Max −150°, start 1/3. 보일러와 같은 호를 그린다.
- 재질: VisualMesh 슬롯 1개 WorldGridMaterial.
- Level 배치: `BP_Circulator` (−179,92,−375), yaw 0, Space_Work. +Y 벽까지 158, −X 벽까지 271, +X 187에 쿨러, 정면 −Y는 열려 있다.

## 4. BP_Vanity (`/Game/Bathhouse/Blueprints/Service/BP_Vanity`, parent `BathhouseFacilityActor`, FacilityType Vanity)
트리: PackagePhysicalRoot > SceneRoot(회전 0) > 아래. 화장대 고유 component는 SCS다.
| component | rel loc / rot | scale·ext | 실크기·비고 |
|---|---|---|---|
| VanityBody | (0,0,50) | (0.6,1.2,1) | Cube 60×120×100 (X±30, Y±60, 상판 Z=100), BlockAllDynamic·Nav on, 슬롯 1개 WorldGridMaterial |
| MirrorVisual | (−25,0,140) / Pitch 90 | (0.8,1.2,1) | Plane → x=−25, Y±60, Z 100~180 (80 높이×120 폭). 슬롯 `lambert1`=WorldGridMaterial. **법선이 −X(뒤쪽)** 이라 단면 재질이면 손님(+X) 쪽에서 안 보일 수 있음 |
| PlacementFootprint | (0,0,90) | ext (40,70,90) | 80×140×180 |
| DisplayTarget | (0,0,100) | ext (45,75,100) | z 0~200, QueryOnly |
| CustomerSlot | (70,0,0), facing yaw 180 | approach (30,0,0) | 손님은 (70,0,0)에서 −X(화장대)를 보고 선다. 접근점 (100,0,0) |
| DisplayManager | — | — | 손님 1명, 사용 20초, 사용 시작 때 소모 |
- 진열 그룹(모두 SceneRoot 자식, NoCollision, hidden, FacilityRouted). 품목 mesh는 중심 pivot이라 slot 점이 물건 중심이다.
  - 실크기 = 엔진 mesh 100 × DisplayOffset scale이다. 슬롯 회전·scale은 1이다.

| 그룹(index) | 중심 / ext | 칸 수·slot 중심(x,y,z) | 품목(정의) | 물건 실크기 X×Y×Z | 바닥 Z |
|---|---|---|---|---|---|
| DryerGroup(0) | (0,−45,105) / (15,15,10) | 2: (0,−54,107), (0,−36,107), Y 간격 18 | HairDryer: Cube, scale (0.18,0.08,0.14) | 18×8×14 | 100 |
| LotionGroup(1) | (0,−15,105) | 4: x ±5 × y −21/−9, z 106 | SkinLotion: Cylinder (0.05,0.05,0.12) | Ø5×12 | 100 |
| SwabGroup(2) | (0,15,105) | 4: x ±5 × y 8/22, z 104 | CottonSwab: Cylinder (0.06,0.06,0.08) | Ø6×8 | 100 |
| CombGroup(3) | (0,45,105) | 6: x −8/0/8 × y 36/54, z 101 | Comb: Cube (0.08,0.03,0.02), yaw 90 | 3(X)×8(Y)×2 | 100 |
- 진열품 전체가 차지하는 범위는 X −9~9, Y −58~58, Z 100~114다. 거울면(x −25)과 진열품 뒷면 사이는 16cm다.
  - 상판은 최소 X −10~10을 덮어야 하고 높이 Z=100을 유지해야 한다(slot Z가 여기에 맞춰져 있다).
- Level instance 없음. 정면은 **+X**다.
- StateTree `ST_CustomerRoutine`에는 화장대 사용 state가 없다(손님 사용은 DisplayManager 경로).

## 5. BP_Shower (`.../Facility/BP_Shower`, parent `BathhouseFacilityActor`)
| component | rel loc | scale·ext | 실크기·비고 |
|---|---|---|---|
| FacilityVisual | (0,0,50) | (0.8,1.1,0.12) | Cube 판 80×110×12 (X±40, Y±55, Z 44~56), BlockAllDynamic·Nav on, 슬롯 1개 WorldGridMaterial |
| PackagePhysicalRoot(root Box) | (0,0,0) | ext (40,55,28) | hidden |
| PlacementFootprint | (0,0,28) | ext **(40,60,28)** | 80×**120**×56. 정본 `PlacementSystem.md`에는 (40,55,28)로 적혀 있음 |
| ShowerDisplayTarget | (0,0,50) | ext (45,60,30) | z 20~80, QueryOnly |
| FacilitySlotA / B | (120,−55,0) / (120,55,0), facing yaw 180 | approach (40,0,0) | 손님 2명이 x=120에서 −X를 보고 선다. 접근점 (160,±55) |
| ShampooGroup(0) | (0,−25,58) ext (15,15,10) | 2칸 (0,−32,63), (0,−18,63) | Shampoo Cylinder (0.06,0.06,0.14) → Ø6×14, 바닥 Z 56 |
| BodyWashGroup(1) | (0,25,58) | 2칸 (0,18,62), (0,32,62) | BodyWash Cylinder (0.06,0.06,0.12) → Ø6×12, 바닥 Z 56 |
- 병 진열 칸은 각 그룹 안에서 Y 간격 14다. 두 그룹 중심은 ±25이고 병 중심 X는 0이다(판 깊이의 가운데).
  - 진열면은 Z=56(현재 판 윗면)이다. 선반 높이를 바꾸면 slot Z도 함께 맞춰야 한다.
- DisplayManager: 손님 2명, 사용 시작 때 소모, 0초.
- Level 배치: `Shower` (1420,−400,25), yaw 0, Space_Bath.
  - **−X 벽까지 80**(등면 x −40에서 40 떨어짐) → 등은 −X 벽 쪽이고 정면은 **+X**다.
  - −Y 벽 250, +X 190에 욕탕 Bath2가 있다.
  - instance에는 빈 StaticMeshComponent 4개가 그룹 위치에 runtime 진열 표현용으로 붙어 있다.

## 6. BP_ClothesLocker / _4 / _8 (`.../Facility/`, parent `BathhouseFacilityActor`)
| BP | 칸 visual(Cube 60×35×100, scale (0.6,0.35,1), 중심 Z 50) | footprint ext (Z 50) | slot |
|---|---|---|---|
| `BP_ClothesLocker` | FacilityVisual (0,0,50): X±30, Y±17.5, Z0~100 | (30,20,50) = 60×**40** (칸 폭 35보다 5 넓음) | FacilitySlot (120,0,0) yaw180 approach (40,0,0). LockerSlot01 (120,0,0) yaw0 approach 0, ID `Locker_1_01`, 부모 PackagePhysicalRoot |
| `_4` | FacilityVisual y −52.5, LockerVisual02~04 y −17.5/17.5/52.5 (Y 간격 35, 140 폭) | (30,70,50) | FacilitySlot (120,0,0). LockerSlot01~04 (120, 칸 y, 0), approach (−100,0,0) |
| `_8` | FacilityVisual y −122.5, LockerVisual02~08 y −87.5 … 122.5 (280 폭) | (30,140,50) | LockerSlot01~08 (120, 칸 y, 0) |
- `LockerVisual*`는 PackagePhysicalRoot 자식이고 FacilityVisual은 SceneRoot 자식이다. 회전은 모두 0이다.
- 재질은 칸마다 슬롯 1개 WorldGridMaterial이고, 문 열림 component는 없다.
- 정면 **+X**(손님은 x=120에서 −X를 본다).
- Level 배치: `ClothesLocker_1` (1150,−380,25), `ClothesLocker_2` (1150,−180,25), 둘 다 yaw 0, Space_Hall.
  - 정면 +X 벽까지 150(손님 서는 자리 x120 → 벽까지 30). 등 −X 220에 신발장이 있어 등은 벽에 붙어 있지 않다.
  - 두 락커 중심 간격은 Y 200이다.

## 7. 손님·플레이어 크기와 자세
- 손님 `BP_BathhouseCustomer`: Capsule 반지름 25, 반높이 90(키 180).
  - Mesh는 `SKM_Manny_Simple`, 상대 (0,0,−90), yaw −90이다.
  - 배치 규약: 발 = slot 점, 몸 앞 방향 = slot 회전·FacingRotation의 +X(C++ `MakeCharacterTransformAtFacilityPoint`).
- 자세는 모두 **선 자세**다. StateTree에서 쓰는 montage는 두 개뿐이다.
  - Undress·Dress·StoreShoes·ReturnTowel: `AM_Customer_Action_Once`(MM_Pistol_Fire·MM_Land)
  - Preshower·MainShower·BathUse·Drying: `AM_Customer_Bath_Loop`(MM_Idle)
  - 앉는 모션은 없다.
- 플레이어 `BP_FirstPersonCharacter`: Capsule 반지름 30, 반높이 96. FirstPersonCamera는 capsule 중심 위 70 → **눈높이는 바닥에서 166**이다. BaseEyeHeight 64는 쓰지 않는다(camera가 정함).

## 8. 정본·목록과 다른 실제값(정정은 소유 단계 몫, 이번에 정본 미수정)
| 항목 | 정본/목록 기록 | 디스크 asset 실제값 |
|---|---|---|
| `BP_Boiler.GaugeNeedlePivot` | (0,−34,90) (`UtilityLaborSystem.md`, `MODELING_STATIC_MESH_LIST.md`) | (67.77,−34,129.13). 본체·문판 밖이라 의도 확인 필요 |
| `BP_Boiler`·`BP_Circulator` 바늘 각도 | Y축 −90°~90° | Zero −30°, Max −150°(축 Y) |
| `BP_Shower` footprint extent | (40,55,28) (`PlacementSystem.md`) | (40,60,28) |
| `BP_ClothesLocker` footprint | 목록 "60×40×100" | 동일(정본 (30,20,50)과 일치), 칸 mesh는 35 폭 |
| `BP_Boiler`·`BP_Circulator` SceneRoot | 회전 언급 없음 | yaw −1°(Cooler는 −90°) |
| `BP_Cooler` GaugePresentation | — | 축 Y, Zero 180°, Max 450° |

## 9. 미확정·한계
- Interchange FBX translator 옵션(ForceFrontXAxis 등)은 Python에서 읽지 못했다. FBX import 축은 glb 실측과 표준 동작으로 추정한 것이며, 실제 FBX 시험 import는 하지 않았다(허용 범위 밖).
- 장식 `boiler`·쿨러 원본 FBX는 저장소 밖(Downloads)에 있어 원본 방향과 대조하지 않았다.
- 화장대·4칸·8칸 락커는 Level instance가 없어 벽 관계를 확인할 수 없다. 화장대 손님 사용 state는 StateTree에 없다.
- 벽 판정은 Editor world의 Visibility trace(400cm, 높이 50·120)로 했다. runtime 생성 형상과 다를 수 있다.
