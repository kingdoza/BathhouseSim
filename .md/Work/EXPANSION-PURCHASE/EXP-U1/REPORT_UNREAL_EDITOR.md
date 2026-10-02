# REPORT_UNREAL_EDITOR — EXP-U1 공간 건물 Editor 작업

- 작업 ID: `EXP-U1`
- 단계: Editor 작업
- 상태: 완료

(Editor 워커 전문을 마스터가 저장)

## 1. 범위·기준선·실행 경로

- 입력: `PROMPT_UNREAL.md`(구현, 완료), 단계 시작 커밋 `2917a41`(브랜치 `work/EXP-U1`). C++ DLL(04:58)이 Source보다 새로웠다.
- 기준선: `git status` clean, 사용자 Editor 없음, 포트 8000 없음, dirty 0. 백업은 `Saved/MigrationBackup/20261002_EXP-U1/`(DefaultMap external actor와 Placement 정의, `manifest.json` SHA-256).
- 전제 확인(읽기): Project Settings Bathhouse Building 값이 `DefaultGame.ini`와 같다. `Facility.Type.*` 14개는 `DefaultGameplayTags.ini`에 있다. `/Script/BathhouseSim.BathhouseSpaceActor`가 있다.
- 실행:
  - 작업용 숨김 Editor를 `-ModelContextProtocolStartServer -NoSplash -log -ExecCmds="py exp_harness.py"`로 띄웠다(PID 31332 → 23248 → 20716 → 22888 → 32372, 각각 정상 종료).
  - `exp_harness.py`는 `Saved/Claude/EXP-U1/queue/`의 스크립트를 같은 Editor에서 실행하는 파일 큐다(공식 `unreal` API만 사용, 네트워크 endpoint·설정 변경 없음).
  - MCP는 `CaptureViewport` 화면 확인에만 썼다. MCP에는 일반 Python 실행 도구가 없다.
  - 스크립트·결과: `Saved/Claude/EXP-U1/01~39_*.py`, `*.json`, 캡처 `cap_*.png`. 로그는 `Saved/Logs/BathhouseSim.log`.

## 2. 정한 값

정본은 Level instance·asset이다. 아래는 보고용 기록이다.

| 항목 | 값 |
|---|---|
| `Zg`(지상 바닥 윗면) | 25cm. 판 20 → 판 아랫면 5 > 지형 0. Recast `AgentMaxStepHeight` 35(해상도 3종 모두), 캐릭터 `MaxStepHeight` 45(플레이어·손님) 이하 |
| `Space_Hall` | Loc (250,0,25), 바닥 2100×1400, 천장 350, 쓰레기 조각. 안쪽 X −800~1300, Y −700~700 |
| `Space_Bath` | Loc (1840,0,25)(= 1300+2T+500, Validation에서 맞닿음 확인), 1000×1300, 천장 350, 물 얼룩 조각 |
| `Space_Work` | Loc (250,−200,−375)(깊이 400), 1400×900, 천장 300, 조각 없음 |
| 출입구 | 홀 서쪽, CenterOffset 300, 폭 200, 높이 260 |
| 통로 | 홀 동쪽 → `Space_Bath`, CenterOffset 0, 폭 200, 높이 260 |
| 계단 | 홀 `Stairs[0]`: TopEdgeCenterOffset (−150,−540)(world 맨 위 가장자리 (100,−540)), DownSide 동, 폭 150, Run 640, 20판, 난간 100. 경사 약 32°(걸을 수 있는 최대 44.77° 이하). 남쪽 레인(북쪽 레인은 카운터 옆 대기 배회 구역과 겹쳐 피함) |
| 조명(세 공간 공통 시작값) | 간격 500, 400cd, 닿는 거리 800, 색 (1,0.95,0.85), 그림자 끔, 천장에서 30 아래. 생성 수: 홀 15, 목욕 6, 작업 6 |
| NavMeshBounds | Loc (450,0,250), Scale (20.5,10,3.5) → X −1600~2500, Y −1000~1000, Z −100~600 |
| 재질 MI(텍스처 / TileSizeCm / Desat / Tint / Rough) | Hall_Wall T_Wallpaper_A_D/150, Hall_Floor T_Wood_Floor_A_D/300, Hall_Ceiling T_Popcorn_Ceiling_D/300, Bath_Wall T_Bathroom_Walls_Tile_D/150, Bath_Floor T_Bathroom_Floor_Tile_D/150, Bath_Ceiling T_Plaster_Wall_01_D/300, Work_Wall T_Stucco_Wall_01_D/300/1/0.55, Work_Floor T_Plaster_Wall_01_D/300/1/0.35, Work_Ceiling T_Stucco_Wall_01_D/300/1/0.5. 계단 판 = Work_Floor, 계단 벽 = Work_Wall. 임시 단색이 아니라 StylizedKitchen 텍스처를 world 기준 투영으로 썼다 |

## 3. 변경 대상과 결과

| 대상 | 결과 |
|---|---|
| `/Game/Bathhouse/Blueprints/Building/BP_BathhouseSpace`(신규) | parent `BathhouseSpaceActor`. 상속 GridVisual = Plane + `MI_FacilityPlacementGrid`. ZoneBounds Z extent 10(=`BP_FacilityPlacementZone`). ZoneBounds 상대 위치 0(zone BP에서 ZoneBounds는 root이므로 Actor 기준 유효 상대값 0). PlacementFloor identity. Class Default 1.0/0.5/5는 zone BP CDO 값 복사(정본 문서의 "10"은 낡은 값). graph·component·변수 추가 없음. Compile `BS_UP_TO_DATE` |
| 공간 Actor 3개(신규 external package `8/7N/V36YOPHA8C46E77EZIX78C`, `6/85/2UD4T92UQSBNFKC3N8BZFK`, `9/0G/5Z3D27MZMK7HDNFHFAEYU2`) | 2절 값, 재질·태그·조명 지정. 대표 `Space_Hall`로 저장 경로를 먼저 검증한 뒤 확장 |
| `/Game/Bathhouse/Materials/Building/M_Building_WorldAligned` + MI 9개(신규) | `WorldAlignedTexture` XYZ → Desaturation → ×Tint → BaseColor, Roughness param. Used with ISM 켬. 모두 VALID |
| `DA_FacilityPlacement_*` 16개 | `Facility.Type.*` 하나 추가(기존 태그 유지). 각 VALID(기존 RecoveryItemMesh fallback 경고만). Stack·Bin은 그대로 |
| Level 삭제 17개 | PlacementZone, LitterSpawnZone_Dressing, BathCleaningZone, DressingCleaningZone, Wall, SM_Bath_old, SM_Bath_01_Body, Plane, Countertop C/C2/C3/C4, Studio_floor, Cooler_Lid, Cooler_Needle, Cooler_Needle_Mesh, RootNode. package 파일 삭제 확인 |
| Level 이동 41개 | 홀: Z+25(XY 유지), 걸레·집게 거치대만 Y −150. 작업공간: 보일러 띠 dY+1000, 세탁 군 (−1400,−950), Z → −375. 목욕공간: Shower (1420,−400), Bath (1842,0), Bath2 (1750,−480), Stack (1500,300), Bin (1500,580)(사용 슬롯이 벽 안에 들어가 1차 위치에서 다시 옮김), ScrubTowel+Slot (2250,450,68). 마당: Spawner (−1150,0,0), Exit (−1100,300,0), TrashCollectionZone (−1150,700,0). 회전·scale·`AssignedItem`·`ItemAnchor` 상대 0 유지(전후 `19_before/after.json`) |
| Computer `ManagedBathPlacementZone` | → `Space_Bath`(`..._1645196495`) |
| `NavMeshBounds` | 2절 범위 |
| `/Game/Bathhouse/Materials/World/M_Landscape_ProcGridHole`(신규) + Landscape 64개 | M_ProcGrid 복제(식 47개 동일) + Masked + `LandscapeVisibilityMask` → OpacityMask. 64개 모두 지정·저장. 지정 전후 캡처(`cap_land_before/after.png`)에서 바깥 지형 색·격자가 같다 |
| Recast | 디스크가 이미 `Dynamic`(package 문자열·새 프로세스 readback). 변경·저장 없음 |

## 4. 검증 결과

- Compile: BP `BS_UP_TO_DATE`, 재질 recompile 오류 로그 없음.
- Save: 대상 package만 개별 저장. 각 단계 뒤 dirty와 예상 집합을 대조해 불일치 0이었다(예상 밖 dirty 없음).
- 새 프로세스 재로드(`38_verify.py`, `39_guid.py`):
  - BP parent·grid가 맞다.
  - 공간 값·Openings·Stairs·Surfaces·Lighting이 맞다. ISM·조명이 생성된다.
  - 정의 태그 16개가 있다.
  - 이동 위치와 거치대 연결이 맞다. 삭제 actor는 없다.
  - Computer 연결, Nav 범위, Recast Dynamic, Landscape 재질 64/64가 맞고 재질 11개 VALID, dirty 0이다.
- Data Validation:
  - `Space_Bath`: VALID. 경고 2개(opt-out Bin·Stack 종류 태그 없음, PROMPT 6절과 C++ 경고 규칙의 결과. 구현이 opt-out을 제외하도록 다듬을 수 있음, 비차단).
  - `Space_Hall`·`Space_Work`: 오류 1개(계단 통로 지형). 재질·Nav·출입구·통로·계단 형상 오류는 없다.
- Nav(편집 world, Dynamic): Spawner에서 카운터·락커·목욕공간까지 경로가 있다. 작업공간·계단에는 Nav가 없다.
  - 카운터 서비스·대기 지점, 락커·샤워·Stack·Bin(이동 뒤) 슬롯, Exit 슬롯은 Nav 위에 있다.
  - `Bath`/`Bath2`의 `FacilitySlotC`는 투영되지 않는다(욕탕 BP 상대 위치는 그대로, 이전 상태와 같은지는 미확인).
- 화면 캡처(MCP): `cap_overview.png`, `cap_hall_in.png`, `cap_work_in.png`, `cap_stair.png`(계단 구멍 안에 지형 격자가 보임 = 구멍 미칠).
- 미검증: PIE·렌더 수용, 손님·플레이어 실제 이동.

## 5. 지형 구멍 시도(실패)

- 경로: 임시 render target 2017×2017에 홀 안쪽 꼭짓점(X −700~1200, Y −600~600, 충돌 구멍이 안쪽 −800~1300 × −700~700에 머물게)만 흰색으로 그렸다(픽셀 확인). 그 뒤 `landscape_import_weightmap_from_render_target(rt, "__LANDSCAPE_VISIBILITY__", 0)` 결과 True, proxy 4개 weightmap이 다시 만들어졌다.
- 결과: `force_layers_full_update` 뒤에도 trace가 지형에 맞고 화면에도 구멍이 없다. Visibility target layer의 `LayerInfoObj=None`이다. 같은 원인 2회 제한으로 중단했다.
- 복구: `reload_packages`가 메모리 값을 되돌리지 못한 채 dirty만 지운 것을 확인했다. 그래서 Editor를 정상 종료·재시작해 디스크 상태(M_ProcGrid, 구멍 없음)를 확인한 뒤 재질 지정만 다시 적용·저장했다.
- 남은 작업: `USER_UNREAL.md` "EXP-U1 공간 건물 — 지형 구멍 칠하기". 화면 작업(Computer Use) 후보: Landscape 모드 Paint > Visibility로 홀 안쪽 직사각형 칠하기, 저장은 Landscape와 바뀐 proxy만.

## 6. 멈춤·관찰·복귀 후보

- 멈춤(11절, S2): 장식 `boiler`(StaticMeshActor, (1528,−532,0), bounds Z 0~103)가 목욕공간 바닥 판(Z 5~25)과 겹친다. 목록상 "그대로 둠"이라 이동·저장하지 않았다. Z +25 이동 또는 삭제 중 사용자 결정이 필요하다. 주변 설비·슬롯과는 겹치지 않는다(샤워 슬롯 A와 약 55cm).
- 같은 세션 재로드 이상(진단 후보, 구현): `LevelEditorSubsystem.load_level`로 같은 Editor에서 DefaultMap을 다시 열자 `Space_Hall`의 `SpaceRoot` 위치가 (0,0,0)으로 읽혔다. 파일에는 (250,0,25)가 저장돼 있고 새 프로세스에서는 정상이다. 사용자가 같은 세션에서 맵을 다시 열 때 재현되는지는 확인하지 않았다.
- 조명 누출(PIE 관찰): 그림자를 끈 실내 점광원이 벽을 지나 출입구 밖 마당 지면을 밝게 비춘다(`cap_overview.png`). `Lighting` 그림자 켜기나 닿는 거리 줄이기로 조정할 수 있다.
- 락커 ID: 이전 package에 `RegistrationId`가 없었고(바이트 검사), 이동 저장 때 로드 중 native가 만든 유효하고 서로 다른 ID가 저장됐다. 값을 직접 쓰지 않았다. `FacilitySystem.md`를 현재 값으로 갱신했다.
- 권한 거부: 저장·재로드 대조용으로 Spawner를 먼저 옮겨 저장하려던 시험이 auto mode 분류기에 막혀 실행하지 않았다. 대신 새 프로세스 재로드로 판정했다.
- C++ 계약과 asset 불일치로 생긴 상위 결함은 없다. 지형 구멍은 Editor 조작 문제다.

## 7. Unreal 정본 갱신

- 신규 `.md/Unreal/BuildingSystem.md`(공간 BP·instance·재질·지형·태그·Validation 상태). `0_UNREAL.md` 라우팅에 Building 행을 넣었다.
- `WorldSystem.md`: 공간 3개, 배치 결과, 삭제 목록, Recast Dynamic, Nav 범위 계약, boiler 주의.
- `PlacementSystem.md`: 단일 PlacementZone 기록을 공간 Actor로 대체, 정의 종류 태그, grid Class Default 원본 위치.
- `CleaningSystem.md`: 구역 instance 제거, 바닥 기록 정정(`Studio_floor`는 빈 actor), 조각 생성 경로.
- `FacilitySystem.md`: 락커 ID, 태그·배치 링크.
- `InteractionUISystem.md`: WidgetClass는 이미 `WBP_ComputerScreenRoot_C`로 맞음. ManagedBathPlacementZone → `Space_Bath`, 컴퓨터 위치 기록 정정.
- `USER_UNREAL.md`: EXP-U1 칠하기 항목 추가, 기존 "2. RecastNavMesh Dynamic 저장"과 재개 조건 한 줄 제거(새 프로세스 확인).

## 8. 사용자 PIE 관찰 항목

| ID | 관찰 | 기대 / 이번 상태 |
|---|---|---|
| EXP-001 | 세 공간 둘러보기 | 벽·바닥·천장 닫힘, 실내가 밝음. 칠하기 전에는 계단 구멍에 지형 면이 보임 |
| EXP-002 | 서쪽 출입구 왕복, 마당 수거 구역에 봉투 | 문턱 25cm(스텝 이하), 수거 |
| EXP-003 | 홀↔목욕 통로 | 막힘 없음(샤워기를 통로 앞에서 치움) |
| EXP-004 | 석탄·바구니·설비 아이템 들고 계단 왕복 | 칠하기 뒤 확인. 지금은 지형 충돌이 계단 중간을 막음 |
| EXP-005 | 벽·천장으로 던지기 | 뚫지 않음 |
| EXP-006 | 설비 위치, 장식·임시 벽 없음, 시작 위치 | WorldSystem.md 표대로. 장식 boiler는 결정 대기 |
| EXP-007 | 손님 한 명 전체 루틴 | 마당 생성 → 체크인 → 락커 → 샤워·입욕 → 계산 → 퇴장, 지하에 가지 않음. 욕탕 슬롯 C 사용 여부 관찰 |
| EXP-008/009 | 공간별 설치 허용·거부 문구 | 태그 표대로 |
| EXP-010 | 조각 | 홀 쓰레기만, 목욕공간 물 얼룩만, 지하 없음 |
| EXP-011/012 | 지하 설비 노동, 수건 순환(계단 경유) | 칠하기 뒤 확인 |
| EXP-013 | 컴퓨터 욕탕 지도 | `Space_Bath` 기준 |
| EXP-014 | 계산대 줄 넘침 | 카운터 옆 배회 구역 |
| EXP-015 | 주문·배송 | 홀 배송 지점 |

## 9. 종료 상태

- 작업용 Editor PID 32372를 정상 종료했다. UnrealEditor 프로세스 0개, 포트 8000 리스너 0개를 확인했다. PIE는 실행하지 않았고 커밋도 하지 않았다.
- `git status`: Content 신규(BP 1, 재질 11, 공간 actor 3), 수정(정의 16, external actor 106 = 이동·연결·Nav 42 + Landscape 64), 삭제 17. `.md/Unreal` 7개 파일과 `USER_UNREAL.md`가 바뀌었다.

## 10. 추가 작업 (2026-10-02)

- 입력: 마스터 추가 지시(사용자 결정 2026-10-02). 장식 `boiler`는 Z +25, 지형 구멍은 사용자가 나중에 직접 칠함(이번에 확인하지 않음, USER_UNREAL 항목 유지). 시작 HEAD `a59628b`.
- 기준선: UnrealEditor 프로세스 0, 포트 8000 리스너 0, Content 변경 없음, dirty 0.
- 실행: 작업용 Editor PID 5428(harness 큐) → 재로드 확인용 PID 32736.
  - PID 32736은 `CloseMainWindow`가 False를 반환해 닫히지 않았다. dirty 0을 확인한 뒤 `QUIT_EDITOR` 콘솔 명령으로 정상 종료했다.
  - 종료 후 프로세스 0, 포트 리스너 0.
- 스크립트: `Saved/Claude/EXP-U1/40_boiler.py`(변경), `43_boiler_verify.py`(새 프로세스 확인), `44_quit.py`.

### 6절 멈춤 해소 — 장식 boiler

| 항목 | 결과 |
|---|---|
| 대상 | Level `boiler` StaticMeshActor(`/Game/Bathhouse/Meshes/boiler`), package `/Game/__ExternalActors__/Maps/DefaultMap/D/8Z/B7P48YHJ18CIPB7PW9OBU2` |
| 변경 | Location (1528,−532,0) → (1528,−532,25). XY·회전·scale 그대로. Z만 지상 바닥 높이(`Zg`)만큼 올림 |
| 저장 | 이 package만 Python `save_packages`로 저장. 저장 전후 dirty 집합이 이 package 하나와 일치 |
| 새 프로세스 재로드 | 위치 (1528,−532,25), bounds Z 25~127.7(바닥 판 윗면 위), dirty 0 |
| Validation | `Space_Bath` VALID, 오류 0, 경고 2(opt-out Bin·Stack, 기존과 같음) |
| 정본 | `.md/Unreal/WorldSystem.md` boiler 주의 기록을 현재 상태로 교체 |

- 6절의 boiler 멈춤 항목은 해소됐다. 남은 보류 사유는 지형 구멍 칠하기(USER_UNREAL.md `EXP-U1` 항목, 사용자 직접)뿐이다. `Space_Hall`·`Space_Work`의 계단 통로 지형 오류 1개는 그 작업 뒤 확인한다.
- 같은 세션에서 사용자 요청(스태틱 메시 모델링 목록) 읽기 전용 조사를 했다. 결과는 `.md/MODELING_STATIC_MESH_LIST.md`(마스터 저장)이고 근거는 `Saved/Claude/EXP-U1/41_mesh_probe.py`·`.json`, `42_mesh_extra.py`다. 조사 중 modify·저장은 없었다.

## 11. 지형 구멍 재시도 (2026-10-02)

- 입력: 마스터 추가 지시(사용자 승인 "레이어 정보를 만들고 자동화 재시도 진행해"). 시작 HEAD `7c53d88`.
- 기준선: UnrealEditor 프로세스 0, 포트 8000 리스너 0, Content 변경 없음.
- 작업 트리의 사용자 미추적 파일(`.md/Work/MODEL-M1/`, `ArtSource/Bathhouse/*`, `.md/MODELING_STYLE_GUIDE.md`)은 건드리지 않았다.
- 실행: 작업용 Editor PID 27084(harness 큐). 스크립트는 `Saved/Claude/EXP-U1/45_land_probe3.py`, `46_land_hole2.py`, `47_layers2.py`, `48_dirty.py`, 캡처는 `cap_stair2.png`다.

### 레이어 정보

- UE 5.8 엔진 소스 확인 결과 공식 Visibility 레이어 정보는 `/Engine/EngineResources/LandscapeVisibilityLayerInfo`(`ALandscapeProxy::VisibilityLayer`, LayerName `__LANDSCAPE_VISIBILITY__`)다.
  - `Landscape.cpp`: proxy PostLoad가 Visibility target layer를 넣는다. `ULandscapeInfo::UpdateLayerInfoMapInternal`은 `LayerInfoObj=None`이면 그 자리를 공식 객체로 채운다.
  - `UE::Landscape::IsVisibilityLayer`는 이 정적 객체와 같은지만 비교한다.
- 그래서 새 `ULandscapeLayerInfoObject`는 구멍 레이어로 인정되지 않는다. 지시의 "엔진 공식 레이어 정보 우선"에 따라 새 asset을 만들지 않았고 Landscape 연결도 바꾸지 않았다. 남기거나 지울 asset은 없다.
- edit layer: `Base Landscape`(index 0), `Flat Middle`(1). 둘 다 visible, weightmap alpha 1이다. merge shader(`LandscapeEditLayersWeightmaps.usf`)는 Visibility를 weight blending 없이 더한다.

### 재시도(2회째, 같은 원인 제한 도달)

- 방법: `LandscapeProxy.landscape_import_weightmap_from_render_target(rt, "__LANDSCAPE_VISIBILITY__", 0)`.
  - 임시 render target 2017×2017에 홀 안쪽 꼭짓점 X −700~1200, Y −600~600만 흰색으로 칠했다(픽셀 확인, 충돌 구멍이 홀 안쪽 −800~1300 × −700~700에 머물게).
  - `LogLandscape` verbose를 켜고 import했다. 반환값 True.
- 관찰:
  - 로그에 홀이 걸친 proxy 4개의 `M_Landscape_ProcGridHole___LANDSCAPE_VISIBILITY__0` 재질 조합 생성이 남았다. Visibility allocation은 최종 merge까지 왔다.
  - 약 3분(수백 frame) 뒤에도 충돌 trace 8지점(홀 안 (0,0), (500,−540) 포함)이 모두 지형 Z 0에 맞았다.
  - 계단 구멍을 내려다본 캡처에 지형 격자가 보인다. `Space_Hall`·`Space_Work`의 계단 통로 지형 오류도 그대로다.
- 판정: 실패. 1차(같은 API·같은 layer)와 같은 결과라 두 번 제한으로 중단했다. 원인은 미확정이다. Visibility 데이터가 충돌·렌더 갱신(edit layer readback 뒤 collision layer data·재질 반영)으로 이어지지 않는 것으로 추정한다. Landscape 모드 도구 없이 이 경로를 더 밀려면 새 도구가 필요해 승인 범위 밖이다.
- 되돌림:
  - import는 `SetData`가 전체 영역 component에 갱신을 요청해 Landscape proxy 63개 package를 dirty로 만들었다. 그 밖의 dirty는 없었다.
  - 앞 세션에서 `reload_packages`가 메모리 값을 되돌리지 못하고 dirty만 지운 전례가 있다. 그래서 저장하지 않고 작업용 Editor PID 27084만 종료해 미저장 변경을 버렸다.
  - 종료 뒤 UnrealEditor 프로세스 0, 포트 리스너 0. `git status`상 Landscape package를 포함한 Content 변경 없음.

### 남은 작업과 정본

- `USER_UNREAL.md` EXP-U1 칠하기 항목에 이번 재시도(공식 레이어 정보 확인, 결과, 버림)를 덧붙이고 항목은 유지했다. 사용자가 Landscape 모드 Paint > Visibility로 홀 안쪽을 칠한 뒤 Editor 역할이 저장·재로드·Validation을 확인한다.
- `BuildingSystem.md`·`WorldSystem.md`는 저장 상태가 바뀌지 않아 수정하지 않았다(지형 구멍 미칠 기록 유지).
- 남은 보류 사유는 지형 구멍 칠하기뿐이다.

## 12. 지형 구멍 보이는 Editor 재시도 (2026-10-02)

- 입력: 마스터 추가 지시(사용자 승인 "2번으로 진행해"). 시작 HEAD `ba7f500`.
- 기준선: UnrealEditor 0, 포트 리스너 0, Content 변경 없음. 사용자 미추적 파일(`.md/Work/MODEL-M1/`, `ArtSource/Bathhouse/*`, `.md/MODELING_STYLE_GUIDE.md`)은 건드리지 않았다.
- 실행:
  - 보이는 창 작업용 Editor PID 28408(숨김 옵션 없음, harness 큐). 화면 조작은 하지 않았다.
  - 확인용 숨김 Editor PID 9380은 스크립트 끝에서 `QUIT_EDITOR`로 정상 종료했다.
- 스크립트: `Saved/Claude/EXP-U1/49_land_diag.py`(+`.json`), `50_mic.py`, `51_lmic.py`, `52_land_hole3.py`, `53_mat_probe.py`, `54_mat_fix.py`, `56_trace_complex.py`, `57_hlod.py`, `58_save_mat.py`, `59_mat_verify.py`.
- 캡처: `cap_stair3.png`(수정 전), `cap_stair4.png`·`cap_stair5.png`(수정 후), `cap_land_fix.png`.

### 충돌 구멍(실패, 저장 안 함)

- 방법: §11과 같은 공식 API(`landscape_import_weightmap_from_render_target`, `__LANDSCAPE_VISIBILITY__`, edit layer 0)로 홀 안쪽 꼭짓점 X −700~1200, Y −600~600에 import했다. 약 2분 기다린 뒤 `ForceLayersFullUpdate`(동기 갱신)를 불렀다.
- 관찰:
  - Visibility 재질 조합 4개가 생겼고, 아래 재질 수정 뒤 화면에서 계단 구멍이 뚫렸다(최종 데이터에 Visibility 있음).
  - 단순 충돌 trace 8지점(홀 안 (0,0)·(500,−540) 포함)은 모두 지형 Z 0에 맞았다.
  - `Space_Hall`·`Space_Work`의 계단 통로 지형 오류도 그대로였다.
  - 사용자의 Landscape 모드 칠하기는 충돌을 뚫었으므로, Python import 경로에서만 충돌 layer data 갱신이 일어나지 않는 것으로 본다.
- 판정: 실패(지시상 마지막 시도). Landscape proxy 63개 dirty를 저장하지 않았다. 작업용 Editor PID 28408만 종료해 버렸고 `git status`상 Landscape package 변경은 없다.

### 화면 구멍 원인 진단과 수정(성공, 저장)

- 진단:
  - 홀 아래 proxy 4개와 Landscape: `LandscapeMaterial`은 `M_Landscape_ProcGridHole`, `LandscapeHoleMaterial`은 None, `bEnableNanite=False`, override 없음.
  - 메모리의 `LandscapeMaterialInstanceConstant` 378개(조합 126 + component 252)는 모두 `M_Landscape_ProcGridHole`을 부모로 한다.
  - package 이름 표의 `M_ProcGrid` 참조는 저장된 MIC import 잔재로, 렌더 부모 체인에는 쓰이지 않는다.
  - 원인: `M_Landscape_ProcGridHole`(M_ProcGrid 복제)은 `bUseMaterialAttributes=True`다. 따라서 §3에서 연결한 Opacity Mask 핀(`LandscapeVisibilityMask`)이 무시돼 구멍 자리도 그려졌다.
- 수정(허용 범위: 이 재질 자체): 기존 attribute 출력(`SetMaterialAttributes_0`) → `BreakMaterialAttributes` → `MakeMaterialAttributes` → 재질 출력.
  - Make의 OpacityMask에 `LandscapeVisibilityMask`를 연결하고 나머지 18개 attribute는 Break에서 그대로 전달했다.
  - CustomizedUVs는 재질에서 쓰지 않는다(`NumCustomizedUVs=0`).
- 확인:
  - 같은 세션에서 계단 구멍을 내려다보면 지형 격자 대신 계단 판이 보인다(`cap_stair4/5.png`).
  - 마당 개관(`cap_land_fix.png`)의 지형 격자·색은 그대로다(밝기 차이는 노출 차이).
  - 재질만 개별 저장, Data Validation VALID.
  - 새 프로세스 재로드에서 출력 노드 `MakeMaterialAttributes`, Masked, VALID, Landscape 64개 재질 지정 유지, dirty 0.
- 디스크 변경: `/Game/Bathhouse/Materials/World/M_Landscape_ProcGridHole` 1개.

### 기타 관찰

- 편집 world에 로드된 `WorldPartitionHLOD` 64개(HLOD0_Instancing)의 `LandscapeMeshProxyComponent`는 보이지 않지만 QueryOnly 충돌을 가진다. 그래서 complex trace가 Z 0에서 이 mesh에 맞는다. 공간 Validation의 단순 trace는 Landscape proxy에 맞으므로 이번 판정과는 무관하다. 지형 구멍 뒤에도 편집 world complex trace가 막히면 이 HLOD가 원인 후보다.

### 남은 작업과 정본

- `USER_UNREAL.md` EXP-U1 칠하기 항목에 3차 시도와 재질 수정을 덧붙였고 항목은 유지했다. 사용자가 Landscape 모드 Visibility로 홀 안쪽을 칠하면 충돌(사용자 실험으로 확인)과 화면(재질 수정)이 함께 반영될 것으로 본다. 칠한 뒤 Editor 역할이 저장·재로드·Validation·화면을 확인한다.
- `.md/Unreal/BuildingSystem.md` 지형 절에 재질 구조(Break/Make OpacityMask)와 공식 Visibility layer info를 반영했다. `WorldSystem.md`는 바뀐 사실이 없어 수정하지 않았다.
- 작업용 Editor 종료 확인: UnrealEditor 0, 포트 8000 리스너 0.

## 13. 사용자 칠하기 확인 (2026-10-02)

- 입력: 마스터 마무리 지시. HEAD `4e695af`.
  - 사용자 칠하기 commit `8960140`: proxy `_3_3_0`(`E/5Y/MJQIJ8RYADZHO7ZS6IE5NM`), `_4_3_0`(`B/NX/PF1O5HWD53VE14YXX78A8H`).
  - 사용자 PIE 승인 2026-10-02.
  - 사용자 변경 commit `4e695af`: `r.DefaultFeature.AutoExposure=False`, 공간 3개 조명 값.
- 실행: 새 프로세스 숨김 작업용 Editor PID 14984, 읽기 전용이다. 끝에서 dirty 0 확인 뒤 `QUIT_EDITOR`로 정상 종료했고 프로세스 0, 포트 리스너 0이다.
  - 스크립트: `Saved/Claude/EXP-U1/60_paint_verify.py`(+`.json`), `61_obj_trace.py`, `62_hole_extent.py`(+`.json`).
  - 캡처: `cap_stair_user.png`.
- 사용자 미추적 파일은 건드리지 않았고 저장도 하지 않았다.

### 판정 기준 정정 — Editor 전용 heightfield

- 편집 world(게임 world 아님)의 Landscape 충돌에는 구멍이 없는 Editor 전용 heightfield가 하나 더 있다. 이 shape는 Visibility 채널을 Block한다(엔진 `LandscapeCollision.cpp` 462~488행, Landscape 편집 도구용). 그래서 편집 world의 Visibility 채널 trace는 구멍과 무관하게 항상 지형에 맞는다.
- §5·§11·§12의 "충돌 trace가 지형에 맞음" 판정은 이 채널을 썼으므로 근거가 아니었다. 게임 충돌은 공간 Validation처럼 WorldStatic·WorldDynamic object type trace로 봐야 한다.
- §12 import는 게임 충돌에 반영됐을 가능성이 있다. 다만 꼭짓점을 Y −600까지만 칠해, 아래 같은 가장자리 문제로 Validation은 실패했을 것이다.

### 게임 충돌 확인(object trace, 단순 충돌)

| 항목 | 결과 |
|---|---|
| 구멍 범위(홀 주변 25cm 격자) | 구멍 표본 224개, bbox X 100~775, Y −600~−425. 홀 안쪽 밖으로 벗어난 구멍 0 |
| 계단 구멍 R(world X 100~740, Y −615~−465) | 중앙·북쪽 표본 통과. 남쪽 가장자리 띠 Y −615~−600은 지형 남음 |
| Validation 계단 trace 9지점 | Y −540·−465 6곳 통과, Y −615 3곳(X 100·420·740) `LandscapeStreamingProxy_4_3_0` 지형 hit |
| `Space_Hall`·`Space_Work` Validation | 계단 통로 지형 오류 1개 남음. `Space_Bath`는 오류 0(경고 2, 기존과 같음) |
| 홀 안쪽 나머지 | 지형 남음(표본 4,341개). Q1 A의 "0회 홀 안쪽 전체" 미충족 |
| 마당·목욕공간 아래 | 지형 hit(정상) |
| 화면 | 계단 구멍을 내려다보면 계단 판이 보이고 지형 격자는 거의 없음(`cap_stair_user.png`) |
| dirty | 0 |

- 편집 world에 로드된 `WorldPartitionHLOD` landscape mesh(QueryOnly)는 complex trace에서만 맞는다. 단순 trace를 쓰는 Validation에는 영향이 없다.

### 판정과 남은 작업

- 지시 1항의 "계단 자리를 다 덮지 못함"에 해당해 수정·정본 갱신 없이 보고한다. 사용자 PIE는 통과했다(남쪽 15cm 띠는 계단 옆 벽 아래라 걷기에 영향이 적은 것으로 보인다). 그러나 Validation 오류와 Q1 A 범위가 남는다.
- 재개 조건: 사용자가 Landscape 모드 Visibility로 홀 안쪽 전체를 칠한다. 최소한으로는 계단 구멍보다 사방 1칸(100cm, 지형 꼭짓점 간격) 넓게, 남쪽은 Y −700까지 칠한다. 그 뒤 Editor 역할이 같은 스크립트(`60`·`62`)로 게임 충돌·Validation·화면을 다시 확인하고 `USER_UNREAL.md` 항목 제거와 정본 갱신을 한다.
- `USER_UNREAL.md` EXP-U1 항목은 그대로 둔다(이번 확인에서 문서 수정 없음). 사용자 조명 값(공간 instance `Lighting`)과 Auto Exposure(`Config/DefaultEngine.ini` `r.DefaultFeature.AutoExposure`) 반영은 위 확인 완료 뒤 정본 갱신 때 함께 한다.

## 14. 사용자 재칠하기 확인 (2026-10-02)

- 입력: 마스터 마무리 재지시. 브랜치 `work/EXP-U1`, HEAD `9522be3`(사용자 재칠하기 proxy `B/NX/PF1O5HWD53VE14YXX78A8H`·`E/5Y/MJQIJ8RYADZHO7ZS6IE5NM`).
- 실행: 새 프로세스 숨김 작업용 Editor PID 5800, 읽기 전용이다. dirty 0 확인 뒤 `QUIT_EDITOR`로 정상 종료했고 프로세스 0, 포트 리스너 0이다.
  - 스크립트: §13과 같은 `Saved/Claude/EXP-U1/62_hole_extent.py`(+`.json`)·`60_paint_verify.py`(+`.json`). 게임 충돌은 WorldStatic·WorldDynamic object trace로 봤다.
  - 캡처: `cap_stair_user2.png`.
  - 저장·브랜치 변경·stash 접근은 없었다.

| 항목 | 결과 |
|---|---|
| 구멍 범위(25cm 격자) | 표본 408개, bbox X 25~975, Y −600~−325(§13: X 100~775, Y −600~−425) |
| 홀 벽 밖 구멍 | 0 |
| Validation 계단 trace 9지점 | Y −540·−465 6곳 통과, Y −615 3곳(X 100·420·740) `LandscapeStreamingProxy_4_3_0` 지형 hit |
| `Space_Hall`·`Space_Work` Validation | 계단 통로 지형 오류 1개 남음. `Space_Bath` 오류 0(경고 2, 기존과 같음) |
| 마당·목욕공간 아래 | 지형(정상) |
| 화면 | 계단 판이 보이고 지형은 거의 보이지 않음 |
| 홀 안쪽 나머지 | 미칠(표본 4,157개, 비고) |
| dirty | 0 |

- 판정: 계단 통로 오류가 남아 지시 3항대로 수정·정본 갱신 없이 보고한다. `USER_UNREAL.md` EXP-U1 항목은 유지한다.
- 원인 추정: 두 번 칠한 결과 모두 남쪽 경계가 지형 꼭짓점 Y −600에서 멈췄다. 충돌 구멍은 꼭짓점 간격(100cm) 단위다. 계단 구멍 남쪽 가장자리(Y −615)가 든 Y −700~−600 칸을 뚫으려면 Y −700 꼭짓점(홀 안쪽 남쪽 끝)까지 칠해야 하는 것으로 보인다.
- 해결안(사용자 결정):
  - (가) Landscape 모드 Visibility로 남쪽을 홀 안쪽 끝(Y −700)까지 칠한다. 화면 구멍은 꼭짓점 사이를 보간하므로 벽 바깥면(Y −720) 근처 지면에 작은 틈이 보이는지 확인이 필요하다.
  - (나) `Space_Hall` `Stairs[0]`의 `TopEdgeCenterOffsetCm` Y를 북쪽으로 15cm 이상 옮긴다(Level instance 1개 변경, 지형 무변경). 현재 배치에서 계단 발자국·출입 자리·작업공간 안쪽 조건은 여유가 있다(작업공간 안쪽 북쪽 끝까지 수백 cm).
- 재개: 결정 뒤 Editor 역할이 같은 스크립트로 게임 충돌·Validation·화면을 다시 확인한다. 통과하면 `USER_UNREAL.md` 항목 제거와 `BuildingSystem.md`·`WorldSystem.md`(지형 구멍 상태, 사용자 조명 원본 = 공간 instance `Lighting`, Auto Exposure 원본 = `Config/DefaultEngine.ini` `r.DefaultFeature.AutoExposure`)를 갱신한다.

## 15. 계단 북쪽 이동과 최종 확인 (2026-10-02)

- 입력: 마스터 지시, 사용자 결정 (나) "계단을 북쪽으로 40cm". 브랜치 `work/EXP-U1`(시작 HEAD는 §14 저장 커밋 `8971aa0` 이후).
- 실행:
  - 변경용 숨김 작업용 Editor PID 31628, 새 프로세스 확인용 PID 27840. 둘 다 dirty 0 확인 뒤 `QUIT_EDITOR`로 정상 종료했고 프로세스 0, 포트 리스너 0이다.
  - 스크립트: `Saved/Claude/EXP-U1/63_stair_move.py`, `64_stair9.py`, `65_revalidate.py`, `66_stair9_complex.py`, `67_save_hall.py`, `62_hole_extent.py`. 캡처: `cap_stair_final.png`.
- stash, `main`, 브랜치, 사용자 미추적 파일은 건드리지 않았다.

### 변경

| 항목 | 결과 |
|---|---|
| 대상 | `Space_Hall`(`BP_BathhouseSpace_C_UAID_F02F7433CA362F0703_1270228495`) `Stairs[0]` |
| 값 | `TopEdgeCenterOffsetCm` (−150,−540) → (−150,−500). 전후 export 비교에서 이 값 외 변화 없음 |
| 저장 | `/Game/__ExternalActors__/Maps/DefaultMap/8/7N/V36YOPHA8C46E77EZIX78C`만 개별 저장. 지형·다른 actor 변경 없음 |
| 새 계단 범위(world) | 발자국 X 80~760, Y −595~−405. 구멍 R X 100~740, Y −575~−425. 위 출입 자리 X −70~80, 아래 출구 X 760~910(Y −575~−425) |
| 겹침 | 대기 배회 구역(X 320~680, Y 280~520)·카운터·열쇠걸이·배송 지점·락커와 겹침 없음 |
| 겹침 오탐 | 2D 검사에서 `DryingSpot`(지하 Z −380~−275)이 위 출입 자리(홀 바닥)와 XY로만 겹침. 다른 층이라 실제 겹침 아님. 지하에서는 계단 위쪽 끝 벽(X 80~100)과 떨어져 있음 |
| Validation 기타 계단 규칙 | 발자국·출입 자리·작업공간 안쪽·경사 오류 없음 |

### 새 프로세스 확인

| 항목 | 결과 |
|---|---|
| 계단 구멍 R 9지점(Validation과 같은 위치, 단순 object trace, Z 25→−75) | 9곳 모두 공간 외 blocking 없음(지형 통과) |
| 구멍 범위 | 표본 408개, X 25~975, Y −600~−325. 벽 밖 구멍 0. 홀 안쪽 나머지 미칠 표본 4,157개(비고) |
| 화면 | 계단 구멍에 계단 판이 보이고 지형 없음(`cap_stair_final.png`) |
| `Space_Hall`·`Space_Work` Validation | 계단 통로 오류 1개 남음 |
| `Space_Bath` Validation | 오류 0(경고 2, 기존과 같음) |
| dirty | 0 |

### 남은 오류 원인(구현 복귀 대상)

- `Source/BathhouseSim/Private/Building/BathhouseSpaceWorldValidation.cpp`의 계단 통로 검사는 `World.LineTraceMultiByObjectType(Hits, Start, End, ObjectParams)`를 query param 없이 부른다. 기본값 `FCollisionQueryParams::DefaultQueryParam`은 엔진 `Engine/Private/Collision/WorldCollision.cpp` 50행에서 `bTraceComplex=true`다.
- 같은 9지점을 complex object trace로 찍으면 모두 `HLOD0_Instancing/OpenWorld_MainGrid_L0_X0_Y-1_Z0`(`WorldPartitionHLOD`)의 `LandscapeMeshProxyComponent`에 Z 0에서 맞는다. 이 HLOD 지형 mesh는 편집 world에 로드돼 있고 보이지 않으며 QueryOnly 충돌을 가진다. 지형 구멍이 반영되지 않은 HLOD 생성물이다.
- 단순 충돌(게임 Landscape heightfield)로는 9곳 모두 통과하므로 Content 측 작업은 끝났다. 검사가 계약("공간이 아닌 blocking 물체(지형 등)")과 다르게 편집 전용 HLOD를 막힘으로 판정하는 구현 문제다.
- 수정 방향(구현 단계 판단): 계단 통로 trace를 단순 충돌(`bTraceComplex=false`)로 하거나 `AWorldPartitionHLOD` actor를 제외한다. 참고로 §5·§11·§12 판정은 편집 world의 Visibility 채널(구멍 없는 Editor 전용 heightfield)을 썼으므로 근거가 아니었다(§13).

### 정본·큐

- 통과 조건(Validation 0)을 채우지 못해 `USER_UNREAL.md` EXP-U1 항목 제거, `BuildingSystem.md`·`WorldSystem.md` 갱신은 하지 않았다.
- 구현 수정·리뷰 뒤 Editor 역할이 Validation 0을 확인하면 다음을 갱신한다: 항목 제거, 지형 구멍 상태(계단 통로 + 주변, 홀 안쪽 일부 미칠 비고), 계단 위치는 `Space_Hall` `Stairs` 원본 참조, 사용자 조명 원본 = 공간 instance `Lighting`, Auto Exposure 원본 = `Config/DefaultEngine.ini` `r.DefaultFeature.AutoExposure`.

## 16. 검사 수정 뒤 최종 확인 (2026-10-02)

- 입력: 마스터 최종 확인 지시. 구현 수정 `2c170b9`(계단 통로 trace `bTraceComplex=false`, 코드 리뷰 3회차 승인), HEAD `aa85c4f`, 브랜치 `work/EXP-U1`.
  - 정규 빌드 바이너리 `UnrealEditor-BathhouseSim.dll`(16:44:51)이 수정 Source(`BathhouseSpaceWorldValidation.cpp` 16:41:00)보다 새롭다. Source에 `FCollisionQueryParams StairQueryParams(..., false)`가 있다.
- 환경: 다른 프로젝트 Editor(PID 30900, `BeekeepingSim.uproject`)가 떠 있었다. 실행 인자로 구분했고 건드리지 않았다. 포트 8000 리스너는 없었다. 충돌을 피하려고 작업용 Editor는 `-ModelContextProtocolStartServer` 없이 Python harness만으로 띄웠다(숨김, PID 30048).
- 스크립트: `Saved/Claude/EXP-U1/68_final.py`, `64_stair9.py`, `48_dirty.py`, `44_quit.py`. 읽기 전용이며 저장은 없다.

| 항목 | 결과 |
|---|---|
| `Space_Hall` Data Validation | VALID, 오류 0, 경고 0 |
| `Space_Work` Data Validation | VALID, 오류 0, 경고 0 |
| `Space_Bath` Data Validation | VALID, 오류 0, 경고 2(배치된 `UsedTowelBin`·`CleanTowelStack` 종류 태그 없음, 기존과 같음) |
| 계단 구멍 9지점(단순 WorldStatic·WorldDynamic object trace, Z 25→−75) | 9곳 모두 공간 외 blocking 없음 |
| dirty | 0 |

- 종료: `QUIT_EDITOR`로 정상 종료하고 PID 30048 소멸을 확인했다. 남은 Editor는 BeekeepingSim PID 30900뿐이고 포트 8000 리스너 0이다.
- 정본·큐 갱신:
  - `USER_UNREAL.md`: EXP-U1 지형 구멍 칠하기 항목 제거.
  - `.md/Unreal/BuildingSystem.md`:
    - 계단 위치는 `Space_Hall` `Stairs[0]` 원본 참조(사용자 결정으로 북쪽 이동).
    - 조명 원본은 공간 instance `Lighting`, 자동 노출 원본은 `Config/DefaultEngine.ini` `r.DefaultFeature.AutoExposure`.
    - 지형 구멍은 계단 통로와 주변을 사용자가 칠함(저장 proxy `_3_3_0`·`_4_3_0`). 홀 안쪽 나머지 미칠은 비고.
    - Editor 확인 방법: 편집 world Visibility 채널 Editor 전용 heightfield, HLOD complex 충돌 주의. 게임 충돌은 단순 object trace로 본다.
    - Data Validation 현재 상태 갱신.
  - `.md/Unreal/WorldSystem.md`: 지형 구멍·자동 노출·조명 원본 참조 추가.
- 금지 항목(저장, PIE, 커밋, 화면 작업, Source 수정, stash·`main`·브랜치 변경)은 지켰다.
- 판정: EXP-U1 Editor 작업의 남은 보류 사유가 모두 해소돼 완료다.
