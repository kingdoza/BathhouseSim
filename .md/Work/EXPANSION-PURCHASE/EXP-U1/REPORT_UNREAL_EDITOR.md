# REPORT_UNREAL_EDITOR — EXP-U1 공간 건물 Editor 작업

- 작업 ID: `EXP-U1`
- 단계: Editor 작업
- 상태: 보류 — 지형 구멍 칠하기 자동화 실패(USER_UNREAL.md, 화면 작업 후보)와 장식 `boiler` 바닥 겹침(설계 S2, 사용자 결정), 책임: 사용자 Editor 조작·사용자 결정, 재개 조건: 칠하기 완료 알림 뒤 Editor 역할이 저장·재로드·Validation 확인, boiler 처리 결정 전달

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
