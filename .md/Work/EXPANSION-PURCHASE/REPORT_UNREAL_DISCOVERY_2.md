# REPORT_UNREAL_DISCOVERY_2 — EXPANSION-PURCHASE 공간·건물 구조
- 작업 ID: `EXPANSION-PURCHASE`
- 단계: Editor 사전 조사
- 상태: 완료

(Editor 워커 전문을 기능 명세 일반 세션이 저장 — 하위 에이전트 보고서 파일 쓰기가 하네스에 막힘, 회고 대상)

## 범위·방법·기준선

- 대상: `/Game/Maps/DefaultMap`(World Partition, actor desc 199개, 199개 모두 로드됨), StylizedKitchen·Bathhouse StaticMesh asset. 읽기 전용이며 modify·Compile·Save·PIE는 하지 않았다.
- 2026-10-02 사용자가 Editor를 닫았다고 알림. 실행 전 `tasklist` 결과 `UnrealEditor.exe` 0개, 포트 8000 리스너 0개였다.
- 숨김 작업 Editor(`UnrealEditor.exe ... -ModelContextProtocolStartServer -NoSplash -log -ExecCmds="py <script>"`)를 두 번 띄웠다. 두 번 모두 스크립트 끝의 `QUIT_EDITOR`로 스스로 정상 종료했고 PID 소멸을 확인했다. 강제 종료는 없다.
  - PID 16276(01:41:00): `Saved/Claude/ExpansionDiscovery/exp_03_probe2.py` → `probe_2.json`(전 actor·구역·Data Layer·mesh), 로그 `editor_probe2.log`
  - PID 6900(01:43:20): `exp_04_trace.py` → `probe_2_trace.json`(100cm 격자 바닥 line trace와 NavMesh 투영), 로그 `editor_probe2_trace.log`
- 두 실행 모두 dirty package 전후 0개. `git status`는 전후 `?? .md/Work/`뿐이다.
- 사람이 읽기 쉬운 actor 표: `Saved/Claude/ExpansionDiscovery/actors_2.tsv`(Landscape·HLOD 제외 71행). 평면도 원본: `floorplan_2.txt`.
- 이전 보고의 "파일 문자열 기반 잠정값"은 아래 Editor 실측으로 대체한다. 다른 점: 벽 1개(`Wall`)가 실제로 있고, `Studio_floor`는 mesh가 비어 바닥이 아니다.

## 1. 전 actor 요약 (199개)

- Landscape 64개(`Landscape` 1 + `LandscapeStreamingProxy` 63). 전체 bounds는 X·Y -100800~100800, Z -100~14989.8이다.
  - 놀이 영역 아래 4개 proxy(`_3_3_0`, `_4_3_0`, `_3_4_0`, `_4_4_0`)의 bounds Z는 -100~100이다.
  - X -2100~2700, Y -1800~1300을 100cm 간격으로 trace한 결과 Landscape 표면은 전부 Z=0.0으로 평평하다.
- `WorldPartitionHLOD` 64개(Outliner 폴더 `HLOD/HLOD0_Instancing`). 나머지 71개 중 Outliner 폴더가 있는 것은 `Lighting` 6개(DirectionalLight, SkyLight, SkyAtmosphere, VolumetricCloud, ExponentialHeightFog, SM_SkySphere)뿐이다. 게임 actor는 모두 폴더 없음(None).
- 숨김: actor 단위로는 `WorldDataLayers-1`, `WorldPartitionMiniMap`만 bHidden=True(엔진 기본)다. editor hidden인 actor는 없다.
  - 숨김 component: PlacementZone `GridVisual`(visible=False, hidden in game), Bath `WaterSurfaceMesh`(hidden in game), Computer 카메라 프록시.
- 실내 조명(Point·Spot·Rect Light)은 0개다. 조명은 DirectionalLight와 SkyLight뿐이다.
- 설비별 위치·bounds·mesh 전체는 `actors_2.tsv`와 `probe_2.json` `1_actors`에 있다.

## 2. 바닥·건물 외피

StaticMeshActor 16개의 역할 추정(근거는 mesh·크기·위치):

| label | 위치 (X,Y,Z) | mesh·scale | 실제 크기·범위 | 역할 추정 |
|---|---|---|---|---|
| Wall | (1162,1075,250) | Cube ×(50,1,5) | 5000×100×500cm, X -1338~3662, Y 1025~1125, Z 0~500 | **유일한 벽**(북쪽 +Y 한 면) |
| SM_SkySphere | (0,0,0) | SkySphere ×400 | 하늘 | 환경 |
| SM_Bath_old | (-1727,-169,0) | SM_Bath_old ×4 | 375×318×102 | 장식 욕조(NavMesh 밖) |
| SM_Bath_01_Body | (-1263,-219,0) | SM_Bath_01_Body ×0.8 | 365×354×129 | 장식 욕조(NavMesh 밖) |
| Plane | (-1258,-221,77) | Plane ×(3,2.2,0.8) | 300×220 수평판 | 위 욕조의 수면 판으로 추정 |
| SM_Kitchen_Countertop_C/C2/C3/C4 | X 163~397, Y -1384~-1586, Z 31 | Countertop | 각 약 215×19×64 | 장식(작업 설비 남쪽, NavMesh 밖) |
| SM_Fridge | (509,367,20) | SM_Fridge ×0.5 | 76×61×118 | 장식(대기 구역 옆) |
| cleaner | (595,385,0) | Cleaner/cleaner | 66×61×93 | 장식(대기 구역 옆) |
| boiler | (1528,-532,0) | Meshes/boiler | 61×43×103 | 장식(Bath2 옆, BP_Boiler와 별개) |
| Studio_floor | (0,0,-2.2) | **mesh 없음** ×100 | bounds 0 | 빈 actor(바닥 아님) |
| Cooler_Lid, Cooler_Needle, Cooler_Needle_Mesh | (-41,53,104)/(0,57,193)/(87,-778,0) | **mesh 없음** | bounds 0 | 빈 import 잔재 |

- `RootNode`는 StaticMeshActor가 아니라 component 없는 `Actor`다(bounds 0, import 잔재).
- 바닥판은 없다. 바닥은 Landscape(Z=0 평면)뿐이다. trace hit은 Landscape 1481칸, Wall 41칸, 그 밖은 설비·장식이다.
- 천장은 없다. 벽은 북쪽 `Wall` 하나뿐이고 남·동·서쪽 벽과 문은 없다. `SM_House_*`·Door·Window mesh는 Level에서 참조하지 않는다.
- 건물 외곽 범위(바닥판이 없으므로 아래로 대신 정의):
  - NavMeshBoundsVolume: X -750~2250, Y -1000~1000
  - 실제 NavMesh 범위: X -700~2200, Y -900~900
  - FacilityPlacementZone: X -800~2000, Y -1000~800
  - 게임 설비의 실제 분포: X -800~2200, Y -1000~900
  - 장식까지 넣은 분포: X -1914~2200, Y -1596~1125

## 3. 공간 구분 (설비 위치, 모두 Z=0 지면)

| 군집(제안) | 대략 범위 X / Y | actor (X,Y[,Z]) |
|---|---|---|
| 홀: 입구·카운터 | -800~550 / -550~550 | Exit(-500,300) 얇은 판 12×120×120, PlayerStart(-200,0,92) yaw 180, Computer(-470,0,160), Counter(0,0), KeyRack(0,-260,50), DrinkCollectionBox(-240,310,20), ShopDeliveryPoint(-490,-534,1), MonkeyWrenchFixedSlot(5,140,42)+MonkeyWrench(5,246,9), CleaningDirector(0,700) |
| 홀: 락커(탈의) | 750~1200 / -400~-160 | ShoeLocker_1(900,-380), ShoeLocker_2(900,-180), ClothesLocker_1(1150,-380), ClothesLocker_2(1150,-180), Spawner(700,-600) |
| 목욕공간 | 1350~2200 / -760~120 | Shower(1400,-100), Bath(1842,0), Bath2(1750,-638), 장식 boiler(1528,-532) |
| 작업공간: 보일러실 | -250~700 / -1000~-750 | Circulator(-179,-908), BP_Cooler(34,-885, scale 0.5), BP_Boiler(299,-885), ShovelSlot(425,-800,45)+UtilityShovel(444,-808), CoalSupply(550,-850), DryIceSupply(647,-861) |
| 작업공간: 세탁·수건 | 1100~1900 / 100~800 | CleanTowelStack(1250,120), UsedTowelBin(1150,420), DryingSpot(1400,420), TowelBasketFixedSlot+TowelBasketCart(1150,750,10), Washer(1500,750), Dryer(1850,750) |
| 청소 도구 거치 | 300~750 / 750~900 | ScrubTowelSlot+ScrubTowel(300,900,43), LitterTongsSlot+LitterTongs(650,750,43), WetMopFixedSlot+WetMop(750,750,43) |

- 고정 거치대 6개: LitterTongsSlot, MonkeyWrenchFixedSlot, ScrubTowelSlot, ShovelSlot, TowelBasketFixedSlot, WetMopFixedSlot. 위치는 위 표와 같다.
- 실제로 나뉜 공간은 없다. 벽·바닥 구분 없이 같은 Z=0 평면에 위치로만 모여 있다.
  - 홀은 서쪽(-X), 목욕은 동쪽(+X), 보일러류는 남쪽 띠(Y≈-900), 세탁은 북동쪽(Y 120~750)이다.
  - 세탁·수건 설비와 도구 거치대가 홀과 작업공간 중 어디에 속하는지는 위치만으로 정할 수 없다. 사용자 질문 대상이다.
- world Z<0 영역은 없다. Z<0인 것은 ExponentialHeightFog(-5600,-50,-6850, 조명)와 빈 `Studio_floor`(-2.2)뿐이다.
  - Landscape 전체 최저 Z는 -100이지만 놀이 영역은 Z=0이다. 지하 바닥·계단·엘리베이터는 없다.
  - 사용자가 말한 "작업공간(지하)"는 현재 Level에 대응물이 없다. 보일러류는 지상 남쪽 띠에 있다.

## 4. 범위 구역 actor 전부

| actor (class) | 중심 | world 범위 X / Y / Z | 핵심 설정 | 공간 소속 |
|---|---|---|---|---|
| PlacementZone (BP_FacilityPlacementZone) | (600,-100,0) | -800~2000 / -1000~800 / -10~10 | AllowedFacilityTags=`Facility.Placeable`, MajorGridIntervalCells=5, GridZOffset 0.5, PlacementFloor Z 0 | **전 공간에 걸침**(홀·목욕·보일러·세탁 모두 포함, 하나뿐). Computer의 `ManagedBathPlacementZone`도 이 actor |
| LitterSpawnZone_Dressing (BP_LitterSpawnZone, scale 2,2,1) | (1380,-190,100) | 780~1980 / -790~410 / 0~200 | MaxActiveLitterInZone=5, FloorTraceDistance 300, MaxSlope 25, Spacing override 0, RequiredFloorComponentTag None | 락커 + 샤워 + Bath 서쪽 + 세탁 남쪽에 **걸침** |
| BathCleaningZone (BP_StainSpawnZone) | (2050,-300,0) | 1900~2200 / -550~-50 / -100~100 | MaxActiveStainsInZone=4, MaxSlope 10, Spacing override 0 | 목욕공간(Bath 동쪽) |
| DressingCleaningZone (BP_StainSpawnZone) | (1000,-700,0) | 600~1400 / -900~-500 / -100~100 | MaxActiveStainsInZone=4, MaxSlope 10 | 락커 남쪽. Spawner(700,-600)와 DryIceSupply(647,-861)가 범위 안에 있어 보일러실 띠와 **겹침** |
| TrashCollectionZone (BP_TrashCollectionZone) | (-800,450,0) | -950~-650 / 300~600 / -100~100 | CollectionIntervalSeconds=300, 바닥 표시 Plane 300×300 | 홀 서쪽(입구 옆). 중심이 NavMesh(X≥-700)·PlacementZone(X≥-800) 경계에 있음 |
| CustomerQueueOverflowWanderVolume_Checkout_0 | (500,400,100) | 320~680 / 280~520 / 0~200 | NavigationProjectionExtent (75,75,150), SampleAttemptCount 12 | 홀(카운터 동쪽, 장식 Fridge·cleaner 옆) |
| NavMeshBounds (NavMeshBoundsVolume, scale 15,10,5) | (750,0,150) | -750~2250 / -1000~1000 / -350~650 | — | 전 공간 |

- 전역 설정: CleaningDirector(0,700)는 MaxActiveLitter 100, MaxActiveStains 20, DefaultLitterSpacing 1.0이다.
- "BoundZone"이라는 asset·class는 없다. 위 Zone/Volume 7개를 가리키는 말로 본다(이전 보고와 같음).

## 5. Data Layer

- `DataLayerEditorSubsystem.get_all_data_layers()` 결과는 빈 목록이다. Data Layer는 0개이고 `WorldDataLayers-1` actor만 있다.
- Level Instance·sub-level도 없다(이전 파일 조사).

## 6. 건물 mesh 자원 (`/Game/StylizedKitchen/Meshes/`, asset bounds, cm)

| mesh | 크기 X×Y×Z | min 좌표 | 섹션(LOD0) | 정점 | 판정 |
|---|---|---|---|---|---|
| SM_House_Walls | 1980×1883×348 | (-1067,-1208,-16) | 6 (Bathroom_Walls, Bathroom_Wall_Tiles, Wall_Paper_Walls, Kitchen_Wall_01, Backsplash, Molding) | 1506 | **집 한 채 통짜** |
| SM_House_Outer_Walls | 1970×1894×347 | (-1054,-1212,-14) | 1 (Cabinets) | 1587 | 통짜 외벽 |
| SM_House_Floor | 1964×1871×0 | (-1051,-1192,1.3) | 2 (Bathroom_Floor_Tiles, WorldGridMaterial) | 267 | 통짜 바닥 |
| SM_House_Ceiling | 1970×1874×0 | (-1054,-1192,333) | 1 | 243 | 통짜 천장(높이 333) |
| SM_Walls_Trim | 551×663×19 | (-547,-1190,180) | 1 | 94 | 집 일부에 맞춘 띠 |
| SM_Door | 10×116×260 | (-10,-116,0) | 1 | 479 | 모듈(문짝, pivot 경첩 쪽) |
| SM_Door_Frame | 4×135×271 | (-4,-125,0) | 1 | 180 | 모듈 |
| SM_Door_Frame_A | 17×162×298 | (-8,-83,0) | 1 | 360 | 모듈 |
| SM_Window | 32×177×205 | (-16,-89,-103) | 1 | 240 | 모듈 |
| SM_BTWindow / SM_Kitchen_Window | 104×37×114 / 23×203×124 | — | 2 / 1 | 377 / 206 | 모듈 |

- 판정 근거: `SM_House_*` 4개가 모두 약 19.6×18.8m로 bounds가 거의 같고 pivot이 중심에서 벗어나 있다(min ≈ (-1054,-1200)).
  - 벽 mesh 한 개에 욕실·주방·벽지 재질 섹션이 함께 들어 있다.
  - 바닥·천장은 두께 0인 한 장이다.
  - 따라서 한 집의 방 배치가 구워진 통짜 외피이고 판(모듈) 형태가 아니다. 길이·높이를 바꿔 이어 붙일 수 없다.
  - 문·창문 mesh는 단품 모듈이다.
- 그 밖에 이름에 wall/floor/ceil/tile 등이 들어간 StylizedKitchen mesh는 `SM_Frames_01~08`(액자, 최대 71×44cm)뿐이다.
- `/Game/Bathhouse` 아래 StaticMesh는 16개다. 벽·바닥·천장으로 쓸 수 있는 것은 없다.
  - 목록: SM_Facility_sample 100cm 큐브, Cooler 3종, Bath 계열 6종, boiler, cleaner, 수건 4종, Equips 1종
  - 현재 벽(`Wall`)과 각종 판은 `/Engine/BasicShapes/Cube`·`Plane`을 scale한 것이다.

## 7. 시작·입출구·배달·NavMesh

| actor | location | rotation (P,Y,R) | 비고 |
|---|---|---|---|
| PlayerStart | (-200,0,92) | (0,180,0) | -X(입구 쪽)를 바라봄 |
| Exit (BP_BathhouseExit) | (-500,300,0) | (0,0,0) | Cube 판 bounds 12×120×120 |
| Spawner (BP_BathhouseCustomerSpawner) | (700,-600,0) | (0,0,0) | 락커 남쪽, DressingCleaningZone 안 |
| BP_ShopDeliveryPoint | (-489.92,-533.9,1) | (0,0,0) | 홀 남서쪽 |

- NavMesh(`RecastNavMesh-Default`, bounds 중심 (988,0,45), extent (1976,988,35))
  - 100cm 격자를 투영한 결과 덮는 범위는 X -700~2200, Y -900~900, 높이 Z≈10이다(543칸).
  - 설비 footprint 자리만 빠진다.
  - Wall(Y≥1025), 장식 욕조(X<-1000), Countertop(Y<-1380)은 NavMesh 밖이다.

## 8. top-down 평면도 (1칸 = 100cm, 위쪽 +Y, 오른쪽 +X)

`|`는 X = -1500, -1000, -500, 0, 500, 1000, 1500, 2000이다. `.`은 NavMesh가 덮는 칸, 빈칸은 덮지 않는 칸이다.

```text
   y/x     |    |    |    |    |    |    |    |
  1100       =====================================            <- Wall (유일한 벽)
   900             ..........m...................
   800            +..............m...k...w...y+..   +=PlacementZone 모서리
   700             .......A......................
   500             ..................... ........   [작업?: 세탁·수건]
   400            T............Qf....u .d........
   300             ..E..$........................
   100             ...... m............t...  ....
     0             ..C..P K ............... B....   [목욕공간 X1350~2200]
  -100             .....................H..  ....
  -200   x   x     ....... ........S.L ..........
  -300             .......R....................z.
  -400             ....... ........S.L ..........   [홀: 락커 X750~1200]
  -500             ..D...................o.......
  -600             ..............G.........B.....
  -800             ...........vF.................
  -900             .....I.U..O..Y................   [작업공간: 보일러실 띠 Y-1000~-750]
 -1000            +                           +
 -1400                      c                      (Countertop 장식 3~4개)
 -1600                        c
   [홀: 입구·카운터 X-800~550]
```

- 범례
  - 홀: E Exit, P PlayerStart, C Computer, K Counter, R KeyRack, $ DrinkCollectionBox, D DeliveryPoint, T TrashCollectionZone, Q QueueOverflowVolume, A CleaningDirector
  - 락커와 손님 생성: S ShoeLocker, L ClothesLocker, G Spawner
  - 목욕: H Shower, B Bath/Bath2, z BathCleaningZone 중심
  - 보일러실: I Circulator, U Cooler, O Boiler, F Coal, Y DryIce, v ShovelSlot
  - 세탁·수건: w Washer, y Dryer, k TowelBasket, d DryingSpot, u UsedTowelBin, t CleanTowelStack
  - 도구 거치대: m
  - 장식: x 욕조, c Countertop, f Fridge/cleaner, o boiler mesh
- 빈 행(1200, 1000, 600, 200, -700, -1100~-1300, -1500)은 생략했다. 원본은 `Saved/Claude/ExpansionDiscovery/floorplan_2.txt`다.

## 미확정·주의

- 공간 군집 경계는 위치로만 추정한 제안이다. Level에는 공간을 나누는 데이터(벽, 볼륨, 태그, Data Layer)가 없다.
- 세탁·수건 설비와 도구 거치대의 공간 소속, 보일러실 띠를 "지하"로 옮길지는 사용자 결정 사항이다.
- LitterSpawnZone_Dressing과 DressingCleaningZone은 제안 군집 경계를 넘는다. PlacementZone은 하나가 전 공간을 덮는다. 공간별로 나눌지는 질문 대상이다.
- NavMesh 범위는 editor world의 저장된 nav data를 투영한 값이다. runtime 재생성 결과와는 다를 수 있다.
