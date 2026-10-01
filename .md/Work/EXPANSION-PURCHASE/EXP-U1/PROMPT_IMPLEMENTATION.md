# PROMPT_IMPLEMENTATION — EXP-U1 공간 건물

- 작업 ID: `EXP-U1`
- 단계: 아키텍처
- 상태: 보류 — 사용자 특별 지시(구현 전 승인)에 따라 0절 Editor 데이터 관리 개요 승인 대기, 책임 단계: 아키텍처(사용자 확인), 재개 조건: 사용자 승인. 승인되면 내용 변경 없이 상태만 `완료`로 바꾼다
- 재작업 기록(2026-10-02 사용자 검토 1회차): Q1 A 확정, 사후 결정 D1(홀=쓰레기 조각만, 목욕공간=물 얼룩 조각만, 작업공간=없음) 반영, 바깥 출입구 필수 규칙을 홀 기준으로 명확화, 공간별 넓힘 목록과 Editor 전용 넓힘 미리보기의 형태를 U2 예정 형태로 확정(U1 구현 금지). 통로 위치 지정 방식과 공용 값의 Project Settings 위치는 유지

- 상위 계약: [../PROMPT_ARCHITECTURE.md](../PROMPT_ARCHITECTURE.md)(상태 완료). 이번 범위는 EXP-001~015, 대표 시나리오는 EXP-007과 EXP-004다.
- Editor 사실: [../REPORT_UNREAL_DISCOVERY.md](../REPORT_UNREAL_DISCOVERY.md), [../REPORT_UNREAL_DISCOVERY_2.md](../REPORT_UNREAL_DISCOVERY_2.md)
- 구조 정본: [../../../Architecture/BuildingSystem.md](../../../Architecture/BuildingSystem.md). 형상·검증 규칙의 상세는 그 문서가 정본이고 이 문서는 반복하지 않는다.

---

## 0. 사용자 확인용 — Editor 데이터 관리 개요

이 절은 사용자가 Editor에서 공간과 확장 데이터를 어디서 어떻게 관리하게 되는지 정리한 것이다. U1에서 만드는 것과 U2·U3에서 만들 예정인 것을 구분해 적는다. 수치는 적지 않는다. 기본 제안값은 상위 계약 4.7 표에 있다.

### 0.1 한눈에 보기

값은 세 곳에만 있다.

1. **Level의 공간 Actor 3개** (`Space_Hall`, `Space_Bath`, `Space_Work`, Blueprint `BP_BathhouseSpace`)
   - 공간 하나에 관한 값은 전부 그 Actor의 Details 패널 `Bathhouse Space` 묶음에 있다. 크기·위치·천장 높이·재질·조명·출입구/통로·계단·놓을 수 있는 설비가 여기 있다.
   - 뷰포트에서 홀 벽을 클릭하면 홀 Actor가 선택된다. 공간을 옮기려면 Actor를 끌면 된다.
2. **Project Settings > Game > Bathhouse Building** (파일 `Config/DefaultGame.ini`)
   - 모든 공간이 함께 쓰는 값이다. 벽 두께, 바닥·천장 판 두께, 벽·바닥을 만드는 상자 모양, 생성 조각의 최대 크기, 그리고 "쓰레기 조각"·"물 얼룩 조각"으로 쓸 구역 Blueprint가 여기 있다. 어느 공간에 어떤 조각을 깔지는 공간 Actor 값이다(0.2 `Cleaning Chunk Kind`).
3. **기존 asset** (대부분 U2·U3에서 씀)
   - 확장 정의 `DA_BathhouseExpansion_Default`: 가격·전체 구입 상한·홀 넓힘 효과 표(U2·U3 예정)
   - 상점 목록 `DA_ShopCatalog`: 락커 상품 가격(U2·U3 예정)
   - 조각 Blueprint `BP_LitterSpawnZone`·`BP_StainSpawnZone`: 조각마다 동시 최대 개수(기존 값 그대로)
   - 청소 관리자 `BP_CleaningDirector`: 맵 전체 상한(기존 값 그대로)

### 0.2 값별 위치

| 값 | 원본(어디에) | Editor에서 바꾸는 법 | 단위·기준 | 단위 작업 |
|---|---|---|---|---|
| 공간 종류 | 공간 Actor `Space Kind`(홀/목욕공간/작업공간) | Actor 선택 → Details | 종류마다 Actor 하나 | U1 |
| 0회 바닥 위치 | 공간 Actor의 **Location**. XY = 바닥 직사각형 중심, Z = 바닥 윗면 높이 | 뷰포트에서 끌거나 Location 입력 | cm, world. Rotation은 0, Scale은 1로 둔다 | U1 |
| 0회 바닥 크기 | 공간 Actor `Floor Size Cm`(X, Y) | Details 입력 | cm, 벽 안쪽 면 사이 바닥 | U1 |
| 천장 높이 | 공간 Actor `Ceiling Height Cm` | Details 입력 | cm, 바닥 윗면 → 천장 아랫면 | U1 |
| 지하 바닥 깊이 | 작업공간 Actor의 Location Z(홀 Actor Z와의 차가 깊이) | Location Z 입력 | cm | U1 |
| 출입구·통로 | 해당 공간 Actor `Openings` 목록. 항목마다 벽 방향(동·서·남·북), 중심 위치(Actor 기준), 폭, 높이, 연결 공간 | 항목 추가·수정 | cm. 연결 공간이 비면 바깥 출입구, 지정하면 통로 | U1 |
| 계단 | 홀 Actor `Stairs` 목록. 아래층 공간, 맨 위 가장자리 위치(홀 Actor 기준), 내려가는 방향, 폭, 길이, 판 수, 난간 높이, 재질 | 항목 수정 | cm | U1 |
| 벽·바닥·천장 재질 | 공간 Actor `Surfaces`(벽·바닥·천장 3칸). 계단 재질은 `Stairs` 항목 안 | 재질 asset 선택 | 공간 종류마다 다르게 | U1 |
| 실내 조명 | 공간 Actor `Lighting`(간격, 밝기, 닿는 거리, 색, 그림자, 천장에서 내린 높이) | Details 입력 | cm, cd | U1 |
| 놓을 수 있는 설비 | 공간 Actor `Allowed Facility Tags`(설비 종류 태그 목록) | 태그 추가·삭제 | 태그 `Facility.Type.<종류>` | U1 |
| 설비 종류 표시 | 설비 정의 `DA_FacilityPlacement_*`의 `Facility Tags`에 종류 태그 하나 | 정의 asset 열기 | 한 번 정하면 바꿀 일 없음. 1·4·8칸 락커는 같은 태그 | U1 |
| 벽 두께, 바닥·천장 판 두께, 형상 상자 mesh | Project Settings > Bathhouse Building | 설정 창 | cm | U1 |
| 생성 조각 종류(공간별) | 공간 Actor `Cleaning Chunk Kind`(없음/쓰레기/물 얼룩 중 하나) | Details 선택 | 기능 계약 D1: 홀=쓰레기, 목욕공간=물 얼룩, 작업공간=없음. 넓힐 때 추가되는 조각도 같은 종류 | U1 |
| 쓰레기·물 얼룩 생성 조각 최대 크기 | Project Settings > Bathhouse Building `Cleaning Chunk Max Size Cm` | 설정 창 | cm(X, Y). 바닥을 이 크기 이하의 같은 칸으로 나눔 | U1 |
| 조각 종류별 구역 Blueprint | Project Settings > Bathhouse Building(쓰레기 조각 class, 물 얼룩 조각 class) | 설정 창 | 기본 `BP_LitterSpawnZone`, `BP_StainSpawnZone`. 공간 Actor가 고른 종류에 맞는 class를 여기서 찾는다 | U1 |
| 조각마다 동시 최대 개수 | `BP_LitterSpawnZone`·`BP_StainSpawnZone` Class Defaults | Blueprint 열기 | 개, 기존 값 | 기존 |
| 맵 전체 쓰레기·물 얼룩 상한 | `BP_CleaningDirector` | 기존과 같음 | 개 | 기존 |
| 손님 길 범위 | Level의 `NavMeshBoundsVolume` 1개 | 상자 크기·위치 조정 | 홀·목욕공간(앞으로 넓어질 자리 포함)과 출입구 밖 마당을 넓게, 높이는 지상만 | U1 |
| 열쇠걸이 자리 수 | `BP_BathhouseKeyRack` `Pair Transforms` | 기존과 같음 | 자리 | 기존 |
| 공간별 넓힘 방향·양 | 공간 Actor `Expansion Steps` 목록. 1번째 줄 = 그 공간의 1번째 넓힘, 2번째 줄 = 2번째 넓힘. 줄마다 `Side`(동·서·남·북)와 `Amount Cm` | 줄 추가·수정·삭제 | cm. 그 방향 벽 한 면만 바깥으로 그만큼 물러난다. 형태 확정, 구현은 U2 | U2·U3 예정 |
| 공간별 넓힘 횟수 상한 | 같은 `Expansion Steps`의 줄 수(따로 적는 값 없음) | 줄을 지우거나 더함 | 회. 줄이 2개면 그 공간은 2번까지 넓힐 수 있다 | U2·U3 예정 |
| 넓힘 미리보기 횟수 | 공간 Actor `Editor Preview Expansion Count`(편집 화면 전용, 저장 안 됨) | Details 숫자 입력 | 회, 0~줄 수. 편집 뷰포트에서만 그만큼 넓힌 모습을 보여 준다. 게임은 항상 실제 넓힘 횟수(시작 0)로 짓는다 | U2 예정 |
| 확장 구입 가격, 전체 구입 상한, 홀 넓힘 횟수별 열쇠 수·락커 칸 한도 | `DA_BathhouseExpansion_Default`(기존 확장 정의). 지금의 단계 표를 "홀 넓힘 횟수별 효과 표"로 쓰고 가격 목록·전체 상한을 더할 예정(세부 필드는 U2 설계) | DataAsset 열기 | 원, 회, 개·칸. 전체 상한은 공간별 줄 수와 별개다. 어느 쪽이든 먼저 닿으면 막힌다 | U2·U3 예정 |
| 락커 상품 가격 | `DA_ShopCatalog`에 락커 상품 3개 추가 예정 | DataAsset 열기 | 원, 상품별 | U2(1칸)·U3(4·8칸) 예정 |

### 0.3 작업 흐름 예

- **홀을 더 크게 시작하고 싶다:** `Space_Hall` 선택 → `Floor Size Cm` 수정 → 벽·바닥·천장·조명·배치 격자 범위·조각 미리보기가 그 자리에서 다시 생긴다. 목욕공간과 겹치면 저장할 때 오류가 뜬다. 목욕공간도 옮겨야 하면 `Space_Bath`를 끈다. 통로는 두 공간이 맞닿아 있어야 뚫린다.
- **출입구를 옮기고 싶다:** `Space_Hall` → `Openings`의 서쪽 항목 `Center Offset Cm` 수정 → 서쪽 벽 구멍이 옮겨진다. 손님 생성기·퇴장 지점·쓰레기 수거 구역은 따라가지 않으므로 출입구 밖으로 직접 옮긴다.
- **통로 폭을 바꾸고 싶다:** 통로는 한쪽 공간(홀)의 `Openings`에만 있다. 그 항목의 `Width Cm`을 바꾸면 홀 동쪽 벽과 목욕공간 서쪽 벽이 같이 다시 뚫린다.
- **계단을 옮기고 싶다:** `Space_Hall` → `Stairs` 항목의 위치·방향 수정 → 홀 바닥 구멍, 지하 천장 구멍, 계단, 계단 벽이 같이 옮겨진다. 계단이 지하 공간 밖으로 나가거나 너무 가파르면 오류가 뜬다. 0회 홀 안쪽에서 옮기면 지형 구멍은 다시 칠할 필요가 없다(Q1 A). 0회 홀 밖으로 옮기면 구멍을 넓혀야 하고, 빠뜨리면 검사가 알린다.
- **지하를 더 깊게:** `Space_Work`의 Location Z를 낮춘다 → 계단 길이는 그대로이므로 경사가 가팔라진다. 너무 가파르면 오류가 뜨므로 계단 `Run Cm`을 늘린다.
- **목욕공간에 놓을 수 있는 설비를 바꾸고 싶다:** `Space_Bath` → `Allowed Facility Tags`에 태그를 더하거나 뺀다. 새 설비 종류라면 그 설비 정의 asset에 종류 태그를 먼저 붙인다(태그 목록은 Project Settings > GameplayTags).
- **재질·조명:** 공간 Actor의 `Surfaces`, `Lighting`을 바꾸면 즉시 반영된다.
- **어느 공간에 어떤 조각을 깔지 바꾸고 싶다:** 그 공간 Actor의 `Cleaning Chunk Kind`를 바꾼다. 미리보기 선이 바로 바뀐다. 한 공간에는 한 종류만 깔린다.
- **(U2부터) 홀 2번째 넓힘을 북쪽 4m로 바꾸고 싶다:** `Space_Hall` → `Expansion Steps`의 2번째 줄 → `Side`를 `북(+Y)`, `Amount Cm`을 400으로. 홀을 2번 넓히면 북쪽 벽이 그만큼 물러난다. 넓힌 끝 모습이 목욕공간이나 다른 공간의 넓힌 끝 모습과 겹치면 저장할 때 오류가 뜬다.
- **(U2부터) 2번 넓힌 홀이 어떻게 보이는지 미리 보고 싶다:** `Space_Hall` → `Editor Preview Expansion Count`를 2로. 편집 뷰포트에서 홀이 목록 1·2번째 줄대로 넓어지고 벽·바닥·천장·조명·통로·배치 격자 범위·조각 미리보기 선이 따라간다. 공간 위에 `넓힘 미리보기 2회` 표시가 뜬다. 0으로 돌리거나 레벨을 다시 열면 0회 모습으로 돌아온다. PIE를 눌러도 게임은 0회에서 시작한다.
- **(U2부터) 홀을 1번만 넓힐 수 있게:** `Space_Hall` → `Expansion Steps`에서 2번째 줄을 지운다. 전체 구입 횟수 상한은 `DA_BathhouseExpansion_Default`에서 따로 바꾼다.
- **조각을 더 잘게:** Project Settings의 조각 최대 크기를 바꾼다. 편집 화면의 조각 미리보기 선이 바로 바뀐다(공간 Actor를 한 번 건드리거나 레벨을 다시 열면 갱신). 게임 시작 때 그 크기로 조각이 생긴다.

### 0.4 값을 바꾸면 자동으로 따라오는 것

| 바꾼 값 | 자동으로 따라오는 것 | 따라오지 않는 것(직접 옮김) |
|---|---|---|
| 공간 위치·크기·천장 높이 | 그 공간의 벽·바닥·천장·조명, 설비 배치 구역과 격자, 생성 조각(미리보기 선, 게임 시작 때 실제 조각), 맞닿은 공간의 통로 구멍, 계단 구멍, 컴퓨터 관리 탭 욕탕 지도(목욕공간) | 이미 놓인 설비·고정 물건, 손님 생성기·퇴장 지점, 계산대 대기 배회 구역, 쓰레기 수거 구역, 플레이어 시작 위치, 손님 길 범위 상자(벗어나면 오류로 알림), 지형 구멍 |
| 출입구·통로 | 해당 벽 구멍(통로는 양쪽 벽) | 출입구 밖 물건 |
| 계단 | 홀 바닥 구멍, 지하 천장 구멍, 계단 판·경사로·계단 벽·구멍 막이 | 지형 구멍(0회 홀 밖으로 옮길 때만, Q1 A) |
| 조각 종류 | 그 공간의 미리보기 선과 게임 시작 때 생기는 조각 종류 | — |
| (U2) 넓힘 목록 | 넓힐 때 물러나는 벽과 그 뒤 형상·구역·조각(넓힌 띠만 새 조각 추가) | 출입구 밖 물건(출입구가 있는 벽을 넓히면 출입구만 벽과 함께 바깥으로 이동) |
| (U2) 넓힘 미리보기 횟수 | 편집 뷰포트의 그 공간 형상·조명·통로·배치 구역·조각 미리보기 선, 이웃 공간의 통로 구멍 | 게임 시작 상태(항상 실제 넓힘 횟수), 저장 내용, 이미 놓인 설비 |
| 재질·조명 | 즉시 반영 | — |
| 허용 설비 태그 | 다음 배치 미리보기부터 반영(격자도 허용 공간에만 보임) | 이미 놓인 설비(놓인 채 유지, 검사가 경고) |
| Project Settings 값 | 공간 Actor가 다시 지어질 때(편집·레벨 열기·게임 시작) 반영 | — |

- "자동"은 공간 Actor의 생성 스크립트(C++ `OnConstruction`)다. Details 값을 바꾸거나 Actor를 끌면 그 공간이 다시 지어지고, 다음 프레임에 나머지 공간도 다시 지어져 통로·계단 구멍이 맞춰진다.
- 벽·바닥·천장·조명·조각은 저장하지 않는 생성물이다. 레벨을 열 때와 게임을 시작할 때 다시 만든다. 그래서 벽 하나를 손으로 고칠 수 없고, 항상 공간 Actor 값으로 바꾼다.

### 0.5 잘못된 설정은 어떻게 알려 주나

- 공간 Actor를 저장하면 Data Validation이 실행되고 결과가 Message Log의 Data Validation 탭에 뜬다. 오류가 있으면 저장 경고가 나온다.
- 게임(PIE)을 시작하면 같은 오류가 Output Log에 `LogBathhouseBuilding` Error로 한 번 찍힌다.
- 오류로 알리는 설정(전체 목록은 BuildingSystem.md Validation):
  - 공간 종류가 빠졌거나 두 번 있음, 공간 Actor를 돌리거나 늘림
  - 두 공간이 겹침(지하는 위층과 높이로 떨어져 있으면 괜찮음), 지하가 너무 얕아 홀 바닥과 지하 천장이 겹침
  - 출입구·통로가 벽 밖으로 나감, 같은 벽에서 겹침, 통로인데 두 공간이 맞닿지 않음, 바깥 출입구 너머에 다른 공간이 붙어 있음, 홀에 바깥 출입구가 하나도 없음(손님은 홀 출입구로만 드나든다. 목욕공간에는 요구하지 않음)
  - 계단이 지하 공간 밖으로 나감, 계단 위·아래 출입 자리가 없음, 너무 가파름, 지하로 가는 계단이 없음, 계단 통로를 지형 등이 막음
  - 손님 길 범위가 홀·목욕공간·출입구 밖을 덮지 않거나 지하를 덮음
  - 놓을 수 있는 설비 목록이 빔, Project Settings의 상자 mesh가 빔, 공간이 고른 조각 종류의 구역 Blueprint가 Project Settings에 없음
- 경고: 재질이 비었음(엔진 기본 격자 재질로 보임), 이미 놓인 설비가 어느 공간에도 없거나 그 공간에 맞지 않는 종류, 작업공간에 조각 종류를 고름(손님이 없어 아무것도 생기지 않음)
- U2·U3 예정: 모든 공간을 목록 끝까지 넓힌 모습끼리 겹침, 다른 공간과 맞닿은 벽(통로 벽 포함)을 넓히는 줄, 넓힘 양이 0 이하, 넓힌 손님 공간이 손님 길 범위 밖, 열쇠 수가 열쇠걸이 자리 수를 넘음, 가격·효과 표가 전체 상한보다 짧음. 경고: 바깥 출입구가 있는 벽을 넓히는 줄

### 0.6 Level Actor와 asset 목록

| 구분 | 대상 | 이번 단위에서 |
|---|---|---|
| 새 Blueprint | `/Game/Bathhouse/Blueprints/Building/BP_BathhouseSpace`(parent `ABathhouseSpaceActor`) | 만든다. 배치 격자 Plane과 격자 재질만 지정 |
| 새 Level Actor | `Space_Hall`, `Space_Bath`, `Space_Work`(위 Blueprint 3개) | 놓고 값 설정 |
| 새 재질 | 공간별 벽·바닥·천장 재질과 계단 재질 | 기존 자원 중 가까운 것으로 만든다. 상자를 늘려 쓰므로 무늬가 늘어나지 않는 world 기준 재질을 권장 |
| 새 재질(Q1 A) | 지금 격자 재질과 똑같이 보이면서 구멍을 지원하는 프로젝트 지형 재질 | 만들어 지형에 지정, 0회 홀 안쪽 바닥 아래 지형에 구멍 칠하기 |
| Project Settings | Bathhouse Building 묶음 | 구현 단계가 기본값을 넣고 Editor에서 확인 |
| GameplayTags | `Facility.Type.*` 14개 | 구현 단계가 등록 |
| 설비 정의 16개 | `DA_FacilityPlacement_*`(배치 활성 16개) | 종류 태그 하나씩 추가 |
| 기존 Level Actor 이동 | 4.1 표의 모든 설비·물건, 플레이어 시작, 손님 생성기와 퇴장 지점(출입구 밖), 쓰레기 수거 구역(출입구 밖 마당), 계산대 대기 배회 구역(카운터 옆), 손님 길 범위 상자 | 이동·크기 조정 |
| 기존 Level Actor 연결 | 컴퓨터 `Managed Bath Placement Zone` → `Space_Bath` | 다시 연결 |
| Level 설정 | Recast NavMesh `Runtime Generation = Dynamic` | 변경 |
| 삭제 | 기존 설비 배치 구역 1개, 쓰레기 생성 구역 1개, 물 얼룩 생성 구역 2개, 북쪽 임시 벽 `Wall`, 건물 밖 장식(옛 욕조 2, 수면 판, 조리대 4), 빈 import actor(`Studio_floor`, `Cooler_Lid`, `Cooler_Needle`, `Cooler_Needle_Mesh`, `RootNode`) | Level에서 삭제(Blueprint asset은 유지) |
| 유지 | 홀 안 장식 냉장고·청소기 mesh(홀 안으로 정리), 확장 관리자·확장 정의·열쇠걸이 | 그대로 |

### 0.7 핵심 결정과 대안

| 결정 | 채택 | 대안과 버린 이유 |
|---|---|---|
| 공간 값을 어디에 | Level의 공간 Actor 인스턴스 값 | DataAsset 하나에 세 공간: 위치가 Level 좌표라 DataAsset에 어울리지 않고, DataAsset을 고쳐도 Level 형상이 바로 다시 지어지지 않아 별도 동기화가 필요하다. 벽을 클릭해도 값이 있는 곳으로 가지 않는다 |
| 배치 구역 | 공간 Actor가 곧 그 공간의 배치 구역 | 공간과 구역을 따로 두고 연결: 크기를 바꿀 때마다 구역도 맞춰야 해서 어긋나기 쉽다 |
| 출입구·통로·계단 | 공간 Actor의 목록. 통로는 한쪽에만 적고 양쪽 벽이 같이 뚫린다 | 문 자리·계단을 별도 Actor로 배치: 눈으로 끌기는 쉽지만 벽 위에 정확히 맞춰야 하고 Actor 종류가 늘어난다 |
| 모두 같이 쓰는 값 | Project Settings 한 곳 | 공간마다 벽 두께 등을 둠: 같은 뜻의 값이 세 군데 생겨 어긋날 수 있다 |
| 손님 길 | 넓게 깐 고정 Nav 범위(지하 높이 제외) + 실시간 Nav 재생성 | 공간이 Nav 범위를 직접 움직임: 다른 Actor를 생성 스크립트에서 고치게 되어 불안정하다. 범위가 모자라면 검사가 알린다 |
| 생성 조각 | 게임 시작 때 공간이 자동 생성, 편집 화면은 선으로 미리보기 | Level에 조각을 직접 배치: 크기를 바꿀 때마다 다시 깔아야 하고 U2 넓힘 때 조각을 추가할 수 없다 |
| 공간별 조각 종류(D1) | 공간 Actor 값 `Cleaning Chunk Kind`(없음/쓰레기/물 얼룩) + 종류별 구역 Blueprint는 Project Settings | 공간 종류로 코드에 고정: Editor에서 보이지 않고 바꿀 수 없다. 공간 Actor에 Blueprint를 직접 지정: 같은 Blueprint를 공간마다 다시 고르게 되어 공용 값 원칙과 어긋난다 |
| 지형 구멍(Q1) | A: 구멍 지원 프로젝트 지형 재질로 바꾸고 0회 홀 안쪽 아래에 구멍 | B: 계단 자리만 작게 칠함. 계단을 옮길 때마다 다시 칠해야 한다 |
| 넓힘 미리보기 소속 | U2(넓힘 목록과 함께) | U1: 미리볼 넓힘 목록이 U2 데이터라 U1에 넣으려면 목록까지 앞당겨야 한다(수직 단위 선행 일반화). 비용은 편집 전용 숫자 하나와 형상 계산 입력 하나라 U2에 얹어도 작다 |
| 넓힘 데이터 형태(U2 예정) | 공간 Actor `Expansion Steps` 목록, 줄 = 몇 번째 넓힘(방향·양), 공간별 상한 = 줄 수 | 상한을 별도 숫자로 둠: 줄 수와 숫자가 어긋날 수 있다(숫자 > 줄 수면 넓힐 양이 없음). DataAsset에 공간별 표: 공간 값이 공간 Actor와 DataAsset 두 곳으로 갈린다. 방향 하나 + 횟수별 양만: "2번째는 북쪽" 같은 횟수별 방향을 못 정한다 |
| 공간별 허용 설비 | 공간의 허용 태그 목록 + 설비 정의마다 종류 태그 | 공간마다 설비 정의 목록: 태그를 쓰는 기존 배치 구역 규칙을 그대로 쓰지 못한다. 정의마다 공간 태그: 표를 바꾸려면 정의 16개를 고쳐야 한다 |
| 벽·바닥 모양 | 상자 하나를 늘려 쓰는 생성 형상 | 집 mesh(StylizedKitchen): 방 배치가 구워진 통짜라 크기를 바꿀 수 없다(사전 조사) |

---

## 1. 기능 계약과 범위

- 시나리오: EXP-001~015(상위 계약 5절 "공간 건물 [U1]"). 대표: EXP-007(손님 한 명이 출입구로 들어와 전 루틴을 마치고 출입구로 나감), EXP-004(물건을 들고 계단 왕복).
- 현재 단계: 수직 구현 첫 단위. 확장 구입 없음. 공간 크기가 Editor 값대로 바뀌면 건물·구역이 따라간다(계약 4.7, 10절).
- 승인 결정: 상위 계약 3절(Q1~Q15, P1~P37, D1, S2 A, S3 A). 아키텍처 질문은 `QNA_ARCHITECTURE.md` Q1 A(지형 구멍). 설계에 맡김(12절) 중 U1 해당 항목의 결정은 3절과 BuildingSystem.md다.

## 2. 목적·수용 기준·비목표

수용 기준(Editor 작업까지 끝난 상태에서 사용자 PIE로 확인):

- 세 공간이 각자 직사각형 벽·바닥·천장으로 닫혀 있고, 틈·구멍·겹쳐 깜빡이는 면이 없으며 실내가 밝다(EXP-001).
- 홀 서쪽 출입구, 홀↔목욕공간 통로, 홀↔지하 계단으로만 오간다. 물건을 든 채 계단을 오르내린다(EXP-002~005).
- 설비·물건이 4.1 표대로 있고 장식·임시 벽이 없다. 플레이어는 카운터 근처에서 시작한다(EXP-006).
- 손님이 출입구 밖에서 생겨 전 루틴 뒤 출입구로 나가고, 지하에 가지 않는다(EXP-007, 014).
- 공간별 허용 설비만 놓이고, 아니면 `이 공간에는 놓을 수 없는 설비입니다`(EXP-008, 009).
- 쓰레기·물 얼룩은 홀·목욕공간에만 손님 수에 비례해 생긴다(EXP-010).
- 설비 노동·수건 순환·관리 탭 지도·상점 배송이 그대로다(EXP-011~013, 015).
- 공간 값을 바꾸면 형상·구역·조각이 따라오고, 잘못된 설정은 Editor가 알린다(0.4·0.5).

비목표: 상위 계약 11절 전체, 확장 탭·구입·넓힘·열쇠/한도 변경·락커 판매(U2·U3), 문짝·창문, 2층, 손님 지하 출입.

## 3. 대상 시스템·파일과 책임 변화

| 항목 | 판단 |
|---|---|
| 기존 책임 | 배치 구역은 레벨에 하나인 `AFacilityPlacementZoneActor`(전 공간 덮음). 쓰레기·물 얼룩 구역은 레벨에 직접 둔 3개. 건물 형상은 없음 |
| 신규 책임 | 공간 단위 형상·조명·개구부·계단 생성, 공간별 배치 구역·허용 설비, 공간 생성 조각, layout 검증 |
| 상태 owner | 공간 Level instance의 authored 값(저장). 생성물은 Transient |
| 실행 owner | 공간 Actor(OnConstruction·BeginPlay·EndPlay), shell component(생성물 수명), 편집 동기화 helper |
| authoring owner | 0.2 표 |
| 의존 방향 | Building → Placement·Cleaning·NavigationSystem·DeveloperSettings. 역방향 없음 |
| 분리 | Actor(조립·흐름), `UBathhouseSpaceShellComponent`(생성 component 수명), 순수 layout·검증 helper, 조각 spawn helper, WITH_EDITOR 동기화 helper |
| 최종 판단 | 공간 = 배치 구역 subclass(새 타입). 대안 거부 이유는 0.7 |

신규 파일: BuildingSystem.md Source Scope 표 그대로.

수정 파일:

| 파일 | 변경 |
|---|---|
| `Public·Private/Placement/FacilityPlacementZoneActor.*` | `static FText GetDefinitionNotAllowedReason()`(문구 `이 공간에는 놓을 수 없는 설비입니다`) 추가. 다른 동작 변경 없음 |
| `Private/Placement/PlayerFacilityPlacementValidation.cpp` | 허용 거부 문구를 위 getter로. `ValidateWorldPlacement`의 `Params.AddIgnoredActor(&Zone)` → `Params.AddIgnoredComponent(Zone.GetZoneBounds())` |
| `Private/Facility/BathhouseFacilityActor.cpp`, `Private/Facility/BathWaterUtilityFacilityActor.cpp`, `Private/Towel/TowelProcessingMachineActor.cpp` | `QueryFacilityPlacement`의 구역 거부 문구를 getter로 |
| `Public·Private/Cleaning/LitterSpawnZoneActor.*`, `StainSpawnZoneActor.*` | `USceneComponent* GetSpawnFloor() const`, `void SetSpawnAreaHalfSizeXY(const FVector2D&)`(SpawnBounds X·Y extent만, Z와 `SpawnFloor` 상대 위치 유지, BeginPlay 전 deferred spawn 중 호출 계약) |
| `Config/DefaultGameplayTags.ini` | `Facility.Type.ClothesLocker`, `DrinkFridge`, `MassageChair`, `RestBench`, `Television`, `Vanity`, `Bath`, `Shower`, `ScrubTable`, `Boiler`, `Cooler`, `Circulator`, `Washer`, `Dryer` 등록(DevComment에 공간 허용 판정용임을 적음) |
| `Config/DefaultGame.ini` | `[/Script/BathhouseSim.BathhouseBuildingSettings]` 섹션: 상자 mesh `/Engine/BasicShapes/Cube`, 종류별 조각 class 두 개(쓰레기 `BP_LitterSpawnZone_C`, 물 얼룩 `BP_StainSpawnZone_C`), 조각 최대 크기(상위 계약 4.7 제안), 벽·판 두께(구현이 정한 기본값). C++ 초기값은 설정이 없을 때의 예비값 |
| `BathhouseSim.Build.cs` | 변경 없음(NavigationSystem, DeveloperSettings, GameplayTags 기존 의존) |

## 4. 공간 Actor 구현

BuildingSystem.md Space Actor 절의 class·component·property 계약을 그대로 구현한다. 추가 지시:

- property tooltip은 한국어로 0.2 표의 의미·단위를 적는다(사용자가 Details에서 읽는다). `EBathhouseSpaceKind`·`EBathhouseSpaceSide`는 `UMETA(DisplayName)`에 한국어 이름(`홀`, `동(+X)` 등)을 둔다.
- `ConnectedSpace`, `LowerSpace`는 `TObjectPtr<ABathhouseSpaceActor>`(EditAnywhere, 같은 레벨 Actor 참조)다.
- `ZoneBounds` 갱신 함수 하나(`ApplyZoneGeometry`)가 extent XY와 상대 위치를 설정한다. OnConstruction에서는 `Super::OnConstruction` 전에 호출해 grid가 새 extent를 쓰게 한다.
- `GetInteriorRect()`는 U1에서 Actor XY ± `FloorSizeCm`/2다. 형상·조각·검증은 반드시 이 함수(와 snapshot)를 거쳐 크기를 얻는다(U2 확장 지점).
- 공간 Actor는 `IPlayerInteractable`·carry 계약을 구현하지 않는다.

`UBathhouseSpaceShellComponent`:

- `Rebuild(const FBathhouseSpacePlan& Plan, const FBathhouseShellVisualInputs& Inputs)`와 `ClearGenerated()`만 public이다.
- part별 ISM 하나와 조명 component, 편집용 조각 미리보기 box를 `RF_Transient`로 만들고 Transient 배열로 소유한다. 재생성 시작 때 이전 생성물을 모두 `DestroyComponent`한다. Engine construction 재실행과 겹쳐 중복 생성되지 않게 CreationMethod·Transient 처리를 구현이 정하고, 두 번 연속 재생성 뒤 component 수가 같음을 자동화로 증명한다.
- part별 표시·충돌·object type·Nav·배치 trace 채널·재질은 BuildingSystem.md 표다. 배치 trace 채널은 `BathhousePlacementCollision::ZoneTraceChannel`을 쓴다. `StairKeepClear`는 response를 전부 Ignore로 시작해 WorldStatic·WorldDynamic·PhysicsBody만 Block한다.
- ISM instance transform은 상자 mesh local bounds에서 scale·offset을 파생한다(mesh가 원점 중심 100cm 큐브라고 가정하지 않음). mesh가 없거나 bounds가 0이면 생성하지 않고 오류를 기록한다.
- 조명: `UPointLightComponent`, Movable, `IntensityUnits = Candelas`, 값은 `Lighting`.
- 편집용 조각 미리보기: editor world에서만, `bIsEditorOnly`, 게임에서 숨김, 충돌·Nav 없음.

## 5. 형상 계산

- `FBathhouseSpaceLayout`(Private, 순수)이 snapshot 배열과 Settings 값(벽 두께, 판 두께, 조각 최대 크기)을 인자로 받아 공간별 `FBathhouseSpacePlan`(part별 상자 목록, 조명 위치, 조각 직사각형)과 검증 문제 목록을 만든다. world·Settings를 직접 읽지 않는다.
- snapshot: 공간 Actor 포인터, 종류, 안쪽 직사각형, 바닥 Z, 천장 높이, 개구부(world 구간으로 변환), 계단 spec. 같은 world의 `TActorIterator<ABathhouseSpaceActor>`로 모으며 다른 공간의 생성 component는 읽지 않는다.
- 규칙은 BuildingSystem.md Geometry Rules가 정본이다. 직사각형 빼기와 조각 분할은 별도 순수 함수로 두고 단위 테스트한다.
- 계단 판 윗면 배치는 "경사로와 한 단 높이 이내"를 만족하는 한 구현이 정한다.

## 6. 생성 조각

BuildingSystem.md Cleaning Chunks 절대로 `FBathhouseCleaningChunkSpawner`가 공간 BeginPlay에서 실행한다.

- Owner = 공간 Actor, spawn은 `SpawnActorDeferred` → `SetSpawnAreaHalfSizeXY` → `GetSpawnFloor()` 상대 Z로 Actor Z 보정 → `FinishSpawning`. class가 없거나 load 실패면 그 종류만 건너뛰고 오류를 한 번 기록한다.
- 공간 Actor `CleaningChunkKind`가 정한 한 종류만 spawn한다. `Litter`면 Settings 쓰레기 조각 class, `Stain`이면 물 얼룩 조각 class, `None`이면 아무것도 만들지 않는다. 기능 계약 D1의 값(홀 `Litter`, 목욕공간 `Stain`, 작업공간 `None`)은 Editor 단계가 Level instance에 설정한다. C++ 기본값은 `None`이다.
- 생성된 조각은 공간 Actor의 Transient weak 배열에 두고 EndPlay에서 파괴한다.
- 공간 종류로 조각 종류를 추론하지 않는다(작업공간에 `None` 외의 값은 검증 경고만).
- 조각은 BeginPlay에서 등록되므로 `ACleaningDirectorActor`의 기존 구역 등록 경로가 그대로 clock을 만든다. director·Floor Rule은 바꾸지 않는다.

## 7. 배치 변경

- PlacementSystem.md Space Zones 절이 정본이다.
- `AddIgnoredComponent(Zone.GetZoneBounds())` 변경으로 공간 바닥 ISM이 바닥 지지 trace에 잡히고, 벽·계단 벽이 overlap에 걸린다. 기존 단일 Zone 자동화가 모두 통과해야 한다.
- grid 표시(`PlayerFacilityPlacementGrid`)는 수정하지 않는다. 공간이 zone subclass이고 net startup Actor라 허용 공간에만 격자가 보인다.

## 8. 편집 동기화와 lifecycle

- BuildingSystem.md Lifecycle 절대로 구현한다. `FBathhouseSpaceEditorSync::RequestRebuild(UWorld*)`는 `EWorldType::Editor` world에서만 동작하고 world weak 키로 다음 tick에 한 번 모든 공간의 shell만 재생성한다(`FTSTicker`). 재생성 중 OnConstruction을 부르지 않고 transaction·`Modify`를 하지 않는다.
- runtime 정본은 BeginPlay 재생성이다. PIE 복제나 직렬화된 생성물에 기대지 않는다.
- 실패: Rotation·Scale 위반과 상자 mesh 누락은 그 공간의 형상을 만들지 않는다(구역 extent는 갱신). 그 밖의 검증 오류는 계산 가능한 형상을 만들고 owner 공간이 `LogBathhouseBuilding` Error를 한 번 남긴다. 되돌림이 필요한 transaction은 없다.
- 전역 설정 영향: 새 DeveloperSettings 하나, GameplayTags 14개 추가. 기존 Project/Collision/Nav 전역값은 Source에서 바꾸지 않는다. Recast `Dynamic`은 Level 설정이며 Editor 단계가 바꾼다(전 레벨 Nav 동작이 실시간 생성으로 바뀜. 설비 배치용으로 이미 요구된 값이다).

## 9. 검증

BuildingSystem.md Validation 표 전부를 `FBathhouseSpaceValidation`에 구현한다.

- 순수 규칙은 layout 결과로 검사하고, world 검사(Nav 범위, 계단 통로 장애물 trace, 배치된 설비 소속)는 world를 받는 별도 함수로 둔다.
- `ABathhouseSpaceActor::IsDataValid`는 Super 결과에 이 Actor가 관련된 문제를 더한다. 문구는 0.5의 사용자 표현을 따른 한국어다.
- 계단 경사 상한은 `GetDefault<UCharacterMovementComponent>()->GetWalkableFloorAngle()`을 읽는다. 문구에 "캐릭터 이동 설정 기준"임을 적는다.
- 계단 통로 장애물 trace는 구멍 안 여러 점에서 수직 line trace(WorldStatic·WorldDynamic object)로 공간 Actor가 아닌 blocking hit를 찾는다. Landscape module 의존을 추가하지 않는다.

## 10. Blueprint/API, Core Redirect와 Editor migration

- 신규 reflected 계약: BuildingSystem.md Blueprint/API Contracts. 기존 class·property 이름 변경·삭제가 없어 Core Redirect와 copy-first load gate가 필요 없다(Placement·Cleaning 변경은 non-reflected 함수 추가와 문구 변경뿐).
- 구현 단계가 `PROMPT_UNREAL.md`에 넣을 Editor 범위(0.6 표를 exact하게):
  1. `BP_BathhouseSpace` 생성(GridVisual Plane·`MI_FacilityPlacementGrid`, ZoneBounds Z extent는 기존 `BP_FacilityPlacementZone`과 같게).
  2. Recast `RuntimeGeneration = Dynamic`(기존 `USER_UNREAL.md` 항목 해소. 자동화 실패 시 그 항목 유지).
  3. 공간 3개 배치와 값 설정(`CleaningChunkKind`: 홀 쓰레기, 목욕공간 물 얼룩, 작업공간 없음). 기본 제안은 상위 계약 4.7(홀·목욕공간 맞닿음, 지하는 홀 아래). 지상 공간 바닥 Z는 지형 면보다 위로 둔다(같은 높이면 바닥과 지형이 겹쳐 깜빡임). 개구부: 홀 서쪽 출입구, 홀→목욕공간 통로. 계단: 홀→작업공간. 허용 태그는 계약 4.2 표.
  4. 공간 재질(world 기준 재질 권장)과 조명 값.
  5. 설비 정의 16개에 종류 태그.
  6. 기존 Level instance 정리: 0.6 표의 삭제·이동·연결. 설비는 공간 바닥 Z로 내리고 footprint가 공간 안쪽에 들어가야 한다. Approach point가 Nav 위에 남는지, 고정 거치대·운반 물건이 같이 옮겨지는지 확인한다. `boiler` 장식 mesh(목욕공간 안)는 계약 목록에 없어 그대로 두고, 공간 형상과 겹치면 멈추고 묻는다(S2).
  7. 손님 생성기·퇴장 지점·쓰레기 수거 구역을 출입구 밖 마당으로, 손님 길 범위 상자를 0.2 기준으로 조정.
  8. 지형 재질·구멍(Q1 A): 지금 `/Engine/OpenWorldTemplate/LandscapeMaterial/M_ProcGrid`와 같게 보이면서 Landscape Visibility Mask를 지원하는 프로젝트 지형 재질을 만들어 지형에 지정하고, 0회 홀 안쪽 바닥 아래(홀 바닥에 가려지는 범위)에 지형 구멍을 칠한다. 바깥 지형 모습이 바뀌지 않았는지 확인한다. 칠하기 자동화가 안 되면 `USER_UNREAL.md`.
  9. 공간 Actor Data Validation 오류 0, 경고는 사유 기록.
  10. Unreal 정본: 새 `.md/Unreal/BuildingSystem.md`와 `0_UNREAL.md` 링크, `WorldSystem.md`(PlacementZone 단일 기록 대체, Recast), `PlacementSystem.md`(Placement Zone 절), `CleaningSystem.md`(DefaultMap 구역 instance 제거, 바닥이 `Studio_floor`라는 낡은 기록 정정), `FacilitySystem.md`(정의 종류 태그), `InteractionUISystem.md`의 컴퓨터 WidgetClass 낡은 기록(상위 계약 6절).
- Editor 단계는 Blueprint graph로 형상·조명·조각을 만들지 않는다. C++ 계약이 Level·asset과 맞지 않으면 아키텍처로 복귀한다.

## 11. 조정값 원본과 상수 예외

- 원본: 0.2 표. 코드는 Level instance property, `UBathhouseBuildingSettings`, 조각 Blueprint Class Defaults, `UCharacterMovementComponent` CDO만 읽는다. 순수 helper는 값을 인자로 받는다.
- 상수 예외(엔진 의미상 상수): 절반(0.5)·축 부호·축마다 최소 1개(조명·조각 개수), 부동소수 비교 허용 오차 `UE_KINDA_SMALL_NUMBER`. 계단 출입 자리 크기(계단 폭)와 출입구 밖 Nav 확인 거리(개구부 폭)는 다른 원본 값에서 파생하며 새 수치를 만들지 않는다.
- 정본·자동화는 수치를 복제하지 않는다. 테스트는 fixture 값을 입력하고 기대값을 같은 입력에서 계산한다.

## 12. 구현 금지 범위

- Content·Level 수정 금지(Editor 단계 몫). Recast·지형·Project Settings asset 저장 금지(Config 파일 기본값 작성만 허용).
- U2·U3 금지: 넓힘 목록(`ExpansionSteps`, 형태는 BuildingSystem.md U2 Expansion Authoring Contract로 확정만)·현재 넓힘 횟수·확장 탭·구입·가격·열쇠/한도 변경·락커 판매·상점 규칙 변경. `UBathhouseExpansionDefinition`·`ABathhouseExpansionAuthority` 변경 금지.
- Customer StateTree·Spawner·Counter·Cleaning director·Floor Rule·Placement 후보 계산·grid 표시 로직 변경 금지(3절 표의 변경만 허용).
- Landscape module 의존 추가 금지. 다른 Actor를 OnConstruction에서 수정하지 않는다(편집 동기화 helper의 shell 재생성만 예외).
- Blueprint로 우회할 Content 계약을 만들지 않는다.

## 13. 자동화·빌드·코드 리뷰 기준

빌드·headless 실행은 [UE_BUILD_POLICY.md](../../../UE_BUILD_POLICY.md). 새 테스트는 `BathhouseSim.Building.*`.

| 시나리오 | 자동화 |
|---|---|
| EXP-001 | 단일 공간: 바닥·천장 Z 범위와 바깥 직사각형, 벽 4개의 모서리 비겹침·합집합이 바깥 띠를 덮음. 맞닿은 두 공간: 바닥·천장이 변만 닿고 겹치지 않음 |
| EXP-002, 003 | 개구부 분할(옆 조각·인방), 통로가 두 공간 벽에서 같은 world 구간, 바깥 출입구 검증 |
| EXP-004 | 계단: 위층 바닥·아래층 천장 구멍 = 구멍 `R`, 경사로 윗면 양 끝점, 계단 벽 열림 구간(위 입구·아래 출구), 구멍 막이 위치. world: 경사로 BlockAll·Nav 비관련, 계단 판 충돌 없음 |
| EXP-005 | world: 벽·천장 ISM이 PhysicsBody·Pawn·Visibility Block |
| EXP-008, 009 | 공간 Actor를 zone으로 한 실제 배치 검증: 허용 종류 성공, 비허용 종류 실패 문구 = getter, 벽과 겹친 후보 Blocked, 계단 구멍 위 Blocked, 공간 바닥 ISM 위 바닥 지지 성공 |
| EXP-010 | 조각 분할(축별 개수·균등 크기·전체 덮음), world: `Litter` 공간은 쓰레기 조각만·`Stain` 공간은 물 얼룩 조각만 생기고 수·`SpawnFloor` Z = 바닥 Z·크기가 맞음, `None` 공간 조각 0, 다른 종류 조각 0, EndPlay 파괴 |
| EXP-013 | zone subclass `ZoneBounds` transform·extent = 안쪽 직사각형(지도 투영 입력) |
| 0.5 | 검증: 종류 중복·누락, 회전, 겹침/닿음/높이 분리, 개구부 범위·겹침, 통로 비인접, 출입구 막힘, 홀 바깥 출입구 없음(목욕공간만 바깥 출입구가 없어도 오류 아님), 고른 조각 종류의 Settings class 없음, 작업공간 조각 종류 경고, 계단 범위 밖·출입 자리·경사, Nav 범위(fixture `NavMeshBoundsVolume`), 빈 허용 태그 |
| lifecycle | 두 번 재생성 뒤 component 수 불변, 공간 하나의 값 변경 뒤 이웃 공간 통로 구멍 갱신(동기화 helper 직접 호출) |
| 회귀 | Placement·Cleaning(Litter 포함)·Facility·Customer·Computer·BathWater 관리 UI 기존 automation 전체 |

리뷰 기준:

- 형상 계산이 다른 공간의 authored 값만 읽고 결과가 순서와 무관한가. 생성물이 Transient이고 저장 package를 dirty로 만들지 않는가.
- 바닥 ISM이 Static·WorldStatic이라 청소 Floor Rule을 통과하고, 벽은 Floor Rule clearance를 막는가.
- `AddIgnoredActor(&Zone)` 제거로 기존 Zone 동작이 바뀌지 않는가.
- 수치 상수가 11절 예외 밖에 없는가.
- 클래스 성장: 공간 Actor가 조립·흐름만 맡고 계산·검증·spawn·동기화가 분리됐는가.

## 14. 사용자 PIE에서 관찰할 시나리오

- 대표: EXP-007(손님 한 명이 출입구 밖에서 생겨 체크인 → 락커 → 샤워·입욕 → 계산 → 출입구로 퇴장, 지하 미출입, 끼임 없음), EXP-004(석탄·수건 바구니·설비 아이템을 들고 계단 왕복).
- 나머지: EXP-001~003, 005, 006, 008~015.
- 마스터가 `PIE_CHECKLIST.md`에 옮길 관찰 포인트: 바닥·지형 깜빡임 없음, 계단 구멍 안에 지형이 보이지 않음, 벽 너머로 배치 격자가 조준되지 않음, 쓰레기가 벽에 묻히지 않음, 관리 탭 지도가 목욕공간 기준.

## 15. 복귀 재설계 여부

해당 없음(첫 설계).
