# REPORT_UNREAL_DIAGNOSIS — 욕탕 관리 지도 그리드 선 누락·욕탕 타일 비율

- 작업 ID: `BUG-2026-10-02_bath_water_map_grid_scale_mismatch`
- 단계: Editor 진단
- 상태: 완료

(Editor 워커 전문을 마스터가 저장)

## 1. 범위와 방법

- 모드: 읽기 전용 진단(코드 계약 대조 포함). modify·Compile·Save·asset 생성·PIE·화면 작업 없음.
- 기준선: `main` `b6ae16e`. 작업 트리의 사용자 변경 `Content/Bathhouse/Blueprints/Facility/BP_ClothesLocker.uasset`(SHA-256 `6C81AE31…5967`)에는 손대지 않음.
- 실행 경로: 작업용 숨김 Editor 1개(PID 17244, `-ModelContextProtocolStartServer -NoSplash -log -ExecCmds="py harness.py"`). 공식 `unreal` Python API를 queue 방식으로 실행. 사용자 Editor는 없었음.
- dirty: 시작·각 스크립트·종료 시점 모두 content/map 0. 종료는 `quit_editor`로 했고 PID 소멸까지 확인함.
- 스크립트·상세 결과: `Saved/Claude/BUG-MapGrid/` (`01_inspect.py/.json`, `02_widgets.py/.json`, `03_scale.py`, `04_bathcdo.py`, `05_quit.py`).
- 스크린샷 측정: Windows WIC로 `user_map_screen.webp`(1363×784)를 디코딩해 행·열 밝기 프로파일로 선과 패널 경계를 측정함(PowerShell, 결과는 아래 수치).
- 사용 피드백: 색인의 해당 항목(FBK-002, FBK-003)이 모두 Experimental이라 의무 적용 대상이 없음. FBK-003 취지에 따라 화면 판정은 수치 계약과 스크린샷 측정으로 대신함.

## 2. 실제 값 (디스크에서 로드한 Editor 상태)

| 항목 | 값 | 출처 |
|---|---|---|
| 전역 grid 간격 | `GridSizeCm=20` | `Config/DefaultGame.ini` `[/Script/BathhouseSim.FacilityPlacementSettings]` |
| `Space_Bath` `MajorGridIntervalCells` | 5 → 지도 굵은 칸 간격 100cm | Level instance(`BP_BathhouseSpace` 상속) |
| 컴퓨터 `ManagedBathPlacementZone` | `Space_Bath` (`BP_BathhouseSpace_C_UAID_F02F7433CA362F0703_1645196495`) | `Computer` instance |
| `Space_Bath` | Location (1840,0,25), Rot 0, Scale 1, `FloorSizeCm` (1000,1300), `AppliedExpansionCount`=0, 미리보기 0 | instance |
| `ZoneBounds` | world 중심 (1840,0,25), unscaled extent (500,650,10), scale 1 → 지도 대상 1000(X)×1300(Y)cm | component |
| 넓힘 줄 | 2줄, 줄마다 남·북·동 400cm(가격 80000/240000) | instance `ExpansionSteps` |
| `Bath` | `BP_Bath_C`, Location (1842,0,25), yaw 0 | Level |
| `Bath2` | `BP_Bath_C`, Location (1750,-480,25), yaw 0 | Level |
| `PlacementFootprint`(CDO=instance) | extent (150,120,38) → 300(X)×240(Y)cm | BP_Bath CDO 재로드 |
| `FacilityVisual` `SM_Bath_old` ×3 | local X −143.2~138.2(281cm), Y −115.6~122.8(238cm) | mesh bounds × scale 3 |
| 컴퓨터 `ScreenWidget` | `WBP_ComputerScreenRoot_C`, DrawSize 1024×576, DrawAtDesiredSize=false, Plane | instance |
| `WBP_ComputerScreenRoot` | `TabBar`(padding 12/6, 탭 SizeBox 132×32) → 높이 44; `ScreenSwitcher` > `ManagementScale`(ScaleBox, Stretch=ScaleToFit, Both) > `ManagementScreen`(slot Center/Center) | WidgetTree |
| `WBP_BathWaterManagementScreen` | `ManagementSize` SizeBox 1024×576, `ManagementFrame` padding (18,12,18,14), Body Left 0.62 / Detail 0.38 | WidgetTree |
| `WBP_BathWaterMap` | `MapSize` min 500×350, `MapFrame` padding 8, `MapStack` > `GridCanvas`·`BathTileCanvas`(Fill) | WidgetTree |
| `WBP_BathWaterBathTile` | `RootOverlay` > `SelectButton` **OverlaySlot H=Left, V=Top** > `TileColumn`(Center/Center) > 글자 5개(5.25~6.75pt) | WidgetTree |

이번 진단 범위 밖의 정본 불일치: `Unreal/PlacementSystem.md`의 BP_Bath 줄에 footprint `(145,120,38)`, 부모 `BathhouseFacilityActor`로 적혀 있다. 실제 CDO는 `(150,120,38)`, 부모는 `/Script/BathhouseSim.BathhouseBathFacilityActor`다. 300/20=15칸이라 grid 정수배 규칙에도 맞는다. 에이전트 쓰기가 권한 분류기에 거부되어 고치지 못했다(마스터 반영 필요).

## 3. 화면 배율 (계산과 스크린샷 대조)

- 화면 위젯은 1024×576 render target이다. 탭 바 높이는 6+32+6=44이므로 관리 화면이 들어갈 영역은 1024×532다.
- `ManagementScale`(ScaleToFit) 배율 s = min(1024/1024, 532/576) = **0.9236**. 관리 화면 레이아웃 1px가 render target에서 0.92px가 된다.
- 스크린샷 배율 k: 탭 폭 132 RT px가 166 스크린샷 px → k = 1.2588(화면 원점은 스크린샷 x≈27, y≈24).
- 검증: 용량 패널 폭은 스크린샷 1147px = RT 911.2 = 레이아웃 988×s(=1024−18−18)이고, 왼쪽 시작은 RT 56.4 = 39.1+18×s다. s=0.9236과 맞는다.
- 지도 칸 간격: 스크린샷 43.3px/100cm = RT 34.4 = 레이아웃 37.24 → 레이아웃 기준 0.3724 px/cm. 지도는 세로가 먼저 차는 배치다(1000cm ↔ canvas 높이 약 372, 가로 1300cm = 484 < canvas 폭 약 587이라 좌우에 여백).
- 스크린샷 측정 결과: 지도 영역 x 167.5~727.5, y 291.5~724.5. 가로/세로 = 560/433 ≈ 1.30이고 Zone 비율 1300/1000과 같다.

## 4. 질문 1 — 그리드 선을 그리는 주체와 선이 빠지는 원인

- 그리는 주체: native `UBathWaterMapWidget::RebuildGrid`(`Source/BathhouseSim/Private/UI/BathWaterMapWidget.cpp` 125~176행). WBP 이미지·재질·배치 구역의 `GridVisual`은 관여하지 않는다.
  - 간격은 `UFacilityPlacementSettings::GetGridSizeCm() × Zone->GetMajorGridIntervalCells()`(=100cm)로 world 값에 연동된다. 고정 텍스처 타일링이 아니다.
  - 선은 Zone 중심에서 시작해 `Cell × 간격` 위치에 놓인다. 세로선은 Cell −6~6(13개, 가장자리는 50cm 반 칸), 가로선은 Cell −5~5(11개, ±5는 경계와 겹침)다.
  - 선마다 `UBorder` 하나를 쓰고 `SetSize(1.0, 길이)`로 두께를 레이아웃 1px로 고정한다. 경계선만 2px다.
- 코드 판정: 반복문이 연속된 정수 칸을 돌고 건너뛰는 조건이 없으므로 위젯 트리에는 선이 모두 만들어진다. 따라서 누락은 그리는 단계에서 생긴다.
- 원인(추정, 근거 강함):
  - 선 두께 1 레이아웃 px × s 0.9236 = **0.92 render target px**인 사각형이 된다.
  - Slate 정점 반올림이나 GPU 픽셀 중심 판정에서, 시작 위치의 소수 부분이 약 [0.5, 0.576) 구간에 들면 칠해지는 픽셀이 0개가 되어 선이 통째로 사라진다. 한 줄이 이렇게 될 확률은 약 7.6%다.
  - 내부 선 22개 기준 기대 누락 수는 약 1.7개다.
- 스크린샷 대조:
  - 세로선 피크는 188, 233, 275, 318, 361, 405, 447(중심 Cell 0), 490, **533 없음**(Cell +2, world Y=+200cm), 577, 620, 663, 707이다.
  - 가로선 피크는 335, 378, 422, 465, 508(중심), **551 없음**(Cell −1, world X=−100cm), 594, 637, 680이다.
  - 누락 2개 모두 칸 경계 위치이고 나머지 선 간격은 고르다. 즉 선이 다른 곳으로 밀린 것이 아니라 그 자리에서 사라진 것이다.
- 확정하지 못한 부분: 측정 오차(±0.4 RT px)가 판정 구간(0.08px)보다 커서, 어느 선이 빠질지를 계산만으로 지목하지는 못했다. 다른 원인(가림 위젯, 코드 누락)은 위젯 트리와 코드에서 배제했다.

## 5. 질문 2 — 지도가 대응하는 world 범위

- `ManagedBathPlacementZone` = `Space_Bath`. `ZoneBounds` 1000(X)×1300(Y)cm는 `FloorSizeCm`, 즉 벽 안쪽 바닥 전체와 같다(`ABathhouseSpaceActor::ApplyZoneGeometry`가 `GetInteriorRect()`에서 extent와 상대 위치를 파생). 스크린샷의 13×10 칸이 이 값과 정확히 맞으므로 사용자 화면은 넓힘 0회 상태다.
- 넓힘 뒤 갱신 경로(코드 대조, runtime 미검증):
  - 넓힘은 `ApplyZoneGeometry`로 `ZoneBounds` extent와 상대 위치를 바꾼다.
  - `UBathWaterMapWidget::ApplySnapshot`은 polling마다 `GetComponentTransform()`/`GetUnscaledBoxExtent()`를 다시 읽는다. `RebuildGrid`는 값이 바뀌면 격자를 다시 만들고, `bZoneChanged`이면 타일도 다시 만든다.
  - 계약 "Zone geometry가 바뀔 때만 rebuild"와 일치한다.
- 투영 계약 대조: world +X는 위, +Y는 오른쪽이고 Zone 비율을 유지하는 letterbox와 단일 px/cm 배율을 쓴다. 구현이 Architecture `Map Projection` 절과 일치한다.

## 6. 질문 3 — 욕탕 타일 크기는 어디서 정해지는가

| 대상 | footprint world | 기대 투영(레이아웃 / 스크린샷 px) | 기대 왼쪽 위(스크린샷) | 관찰 |
|---|---|---|---|---|
| Bath | 중심 (1842,0), 300(X)×240(Y) | 89.4×111.7 / **104×130** | (395.5, 442.1) | 보이는 상자 (396,442)부터 약 60×31 |
| Bath2 | 중심 (1750,−480), 300×240 | 같음 / **104×130** | (187.6, 482.0) | 보이는 상자 (188,482)부터 약 60×31 |

- footprint와 실제 메시: 300×240 vs 281×238cm로 X축만 19cm 크다. 지도 크기를 실제와 다르게 만들 정도의 차이가 아니다.
- 투영: `ProjectFootprint`가 footprint 네 모서리를 투영한다. `LayoutTile`이 CanvasPanelSlot에 Alignment 0.5, Position=중심, Size=투영 크기(104×130 상당)를 넣는다. 두 타일의 왼쪽 위 좌표가 ±1px로 맞으므로 계산·slot 크기 모두 정상이다.
- 원인(확정): `WBP_BathWaterBathTile`의 `SelectButton`이 `RootOverlay` 안에서 **HAlign Left / VAlign Top**이다.
  - 그래서 slot이 footprint 크기여도 버튼은 글자 내용 크기(레이아웃 약 52×27)로만 그려지고, footprint 사각형의 왼쪽 위에 붙는다.
  - 마스터 관찰 "칸 1.5×0.7개"는 이 버튼 크기다. 기대값은 칸 2.4(가로)×3.0(세로)개다.
  - 클릭 영역도 이 작은 버튼으로 줄어 있다.
- 사용자가 본 "비율 불일치"는 실제 현상이다. 다만 원인은 투영이나 footprint가 아니라 타일 WBP의 정렬 하나다.

## 7. 결론과 권장 수정

| 현상 | 판정 | 책임 단계 | 권장 방향 |
|---|---|---|---|
| 그리드 선 일부 누락 | 추정(근거 강함): 레이아웃 1px 선 × ScaleBox 0.9236 → 0.92px 선이 그려지지 않음 | 구현 (C++ `RebuildGrid`의 선 두께) | 화면에 실제로 그려지는 두께가 1px 이상이 되게 한다. 예: 위젯 geometry의 누적 배율로 두께를 `max(1, 1/scale)`로 보정, 또는 선 위치·두께를 render 픽셀에 맞춤. 두께·색이 코드 상수(1px, 2px, 색 4개)라서 조정값 원본 원칙에 따라 Map WBP 기본값 등으로 옮길지는 아키텍처가 짧게 정하는 편이 좋다. 대안으로 관리 화면을 1024×532에 맞춰 배율 1로 두는 방법(Editor asset)도 있지만 다른 탭 배치에 영향이 있어 보조안이다 |
| 욕탕 타일이 실제보다 작음 | 확정: `SelectButton` slot Left/Top | Editor asset (`WBP_BathWaterBathTile`). C++ 계약은 맞으므로 복귀 없이 Editor 단계에서 수정 | `SelectButton` OverlaySlot H/V를 Fill로 바꾼다. 내부 `TileColumn` Center는 유지한다. Compile·개별 Save·재로드 후 사용자 PIE에서 타일이 footprint 크기(Bath 기준 칸 2.4×3개)인지, 글자가 잘리지 않는지, 클릭 영역이 맞는지 확인한다. 회전 타일(yaw 90) 표시도 함께 본다 |
| 지도 영역 ↔ 목욕 공간 | 정상(확정): Zone = 바닥 전체 1000×1300 | 해당 없음 | 없음. 넓힘 뒤 갱신은 코드 계약상 맞으므로 PIE에서 넓힘 1회 후 칸 수가 17×14로 바뀌는지(Y 2100/X 1400, 100cm 칸) 관찰 항목으로 둔다 |
| footprint ↔ 실제 욕탕 메시 | 정상: 300×240 vs 281×238 | 해당 없음 | 없음 |

PIE 관찰 항목(수정 후, 사용자):
- 지도 내부 세로선 13개와 가로선 9개(경계 제외)가 모두 보이고 간격이 고른지
- Bath·Bath2 타일이 각각 칸 약 2.4(가로)×3(세로)개를 덮고, 위치가 실제 공간 배치(Bath는 중앙, Bath2는 왼쪽 약 4.8칸·아래 약 0.9칸)와 같은지

## 8. 추가 관찰 (범위 밖, 미확정)

- 지도의 굵은 선은 Zone 중심에서 시작한다. 반면 `Unreal/PlacementSystem.md`의 world `GridVisual` 재질 설명(plane UV에 칸 수 적용)대로라면 world 바닥 격자는 바닥 모서리에서 시작한다. 그렇다면 Y 1300cm(굵은 칸 13개)에서 두 격자의 굵은 선이 50cm 어긋난다.
- 설치 스냅(`MakeCandidateTransform` → `QuantizeLocalCoordinate`)은 중심 기준이라 지도와는 같은 기준이다.
- 재질 그래프를 직접 읽지 않았으므로 미확정이다. 필요하면 별도 확인한다.

## 9. 미검증·잔여

- 선 누락의 정확한 선별 메커니즘(Slate 정점 반올림인지 GPU 래스터화인지)과 특정 선의 소수 부분은 측정 정밀도 한계로 확정하지 못했다. 수정 후 PIE에서 확인한다.
- `.md/Unreal/PlacementSystem.md`의 BP_Bath 값 정정(§2)은 권한 거부로 반영하지 못했다.
- 종료 뒤 `git status` 재확인도 권한 분류기 거부로 실행하지 못했다. Editor dirty 0과 저장 API 미사용으로 asset 무변경을 판단했다.
- 작업용 Editor는 종료했다(PID 17244 소멸, 포트 8000 해제).
