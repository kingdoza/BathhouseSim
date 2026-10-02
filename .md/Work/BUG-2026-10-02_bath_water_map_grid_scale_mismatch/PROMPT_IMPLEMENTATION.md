# PROMPT_IMPLEMENTATION — 욕탕 관리 지도 그리드 선 누락·욕탕 타일 크기

- 작업 ID: `BUG-2026-10-02_bath_water_map_grid_scale_mismatch`
- 단계: 아키텍처
- 상태: 완료

## 1. 기능 계약과 단계

- 기능 계약: 버그 리포트 [2026-10-02_bath_water_map_grid_scale_mismatch.md](../../BugReports/2026-10-02_bath_water_map_grid_scale_mismatch.md)의 기대 동작(기능 명세 생략, 근거는 `CONTEXT.md`). 사용자 결정 2026-10-02 "둘 다 수정", "값은 기본적으로 원본참조로 작성임".
- 입력: `REPORT_UNREAL_DIAGNOSIS.md`(완료). 시나리오 ID는 이 작업에서 다음처럼 정한다.
  - `MAPG-001` 지도 그리드 선이 빠짐없이 고른 간격으로 보인다.
  - `MAPG-002` 욕탕 타일이 실제 욕탕 footprint가 바닥에서 차지하는 범위와 같은 비율·위치로 지도를 덮고, 그 전체가 클릭 영역이다.
  - `MAPG-003` 공간 넓힘 뒤에도 `MAPG-001`·`MAPG-002`가 유지된다.
- 단계 구분: 단순 버그 수정(수직 구현·전체 확장 구분 없음).
- 아키텍처 정본 반영: [BathWaterManagementUISystem.md](../../Architecture/BathWaterManagementUISystem.md) `Map Grid Lines`·`Bath Tile Fill`, [UISystem.md](../../Architecture/UISystem.md) Bath Water Management Screen, [CoreSystem.md](../../Architecture/CoreSystem.md) Module Rules(SlateCore 사용처).

## 2. 원인 요약과 책임

| 현상 | 원인(진단 확정·추정) | 책임 단계 | 이 설계의 처리 |
|---|---|---|---|
| 그리드 선 일부 누락 | 선 두께가 레이아웃 1px 상수다. 컴퓨터 화면 root의 `ManagementScale`(ScaleBox) 배율이 1보다 작아 render target에서 1px 미만 사각형이 되고, 시작 위치 소수부에 따라 칠해지는 픽셀이 0개가 된다 | 구현 | 선 두께를 render px 단위 authoring 값으로 바꾸고 누적 배율로 레이아웃 두께를 역산한다(3절) |
| 선 두께·색이 코드 상수 | 조정값 원본 원칙 위반 | 구현 | `WBP_BathWaterMap` Class Defaults로 이동(4절) |
| 욕탕 타일이 footprint보다 작음 | `WBP_BathWaterBathTile` `SelectButton` OverlaySlot이 Left/Top이라 버튼이 글자 크기로만 그려짐. C++ slot 위치·크기는 정상 | Editor asset | C++ 변경 없음. Editor 계약만 확정(6절) |
| 정본 수치 복제 | `.md/Unreal/PlacementSystem.md` 등이 asset 수치를 복제 | Editor 작업 | 원본 위치 참조로 정리(7절) |

## 3. 그리드 선 그리기 설계 (`MAPG-001`, `MAPG-003`)

### 3.1 방식 결정

- 채택: 선마다 `UBorder` 하나를 `GridCanvas`에 두는 현재 방식을 유지한다. 선 두께만 "화면에 그려지는 render target 픽셀" 단위로 authoring하고, `GridCanvas`의 누적 배율(레이아웃 1 단위가 render target 몇 px인지)로 나눠 레이아웃 두께를 정한다.
  - 레이아웃 두께 = `max(1, 두께Px) / 축 배율`. 세로선은 X축 배율, 가로선은 Y축 배율을 쓴다.
  - render 두께가 1px 이상이면 Slate 정점 반올림(정수 평행이동에 대해 단조)에서도 `round(x + w) ≥ round(x) + 1`이고, 반올림이 없는 GPU 래스터화에서도 폭 1 이상 사각형은 픽셀 중심을 반드시 하나 덮는다. 따라서 시작 위치 소수부와 관계없이 선이 사라지지 않는다.
  - 두께가 정확히 1 render px이면 모든 선이 같은 1px 폭으로 그려진다. 부동소수 오차로 0.99999px가 되는 경우는 판정 구간이 1e-5px 이하라 무시한다. epsilon을 더하지 않는다(정수 두께가 1px/2px로 흔들리는 것을 막기 위해).
- 거부한 대안
  - `NativePaint`에서 `FSlateDrawElement::MakeBox/MakeLines`로 직접 그리기: production Slate draw API 의존과 매 frame paint 코드가 생긴다. 현재 문제는 두께 하나로 해결된다.
  - `ManagementScale`을 없애거나 관리 화면을 1024×532로 줄여 배율 1로 만들기(Editor): 관리 화면 전체 layout·글자 크기가 바뀌고, 이후 배율이 다시 1 미만이 되면 재발한다.
  - 레이아웃 단위 두께에 `max(1, 1/scale)`만 곱하기: 결과는 같지만 authoring 단위(레이아웃 px인지 화면 px인지)가 모호하다. 사용자가 보는 화면 px 단위로 정한다.
  - 선 위치를 render 픽셀 경계에 맞추는 정렬: 누적 평행이동까지 추적해야 하고, 두께 보정만으로 가시성이 보장된다. 선 간격이 1px씩 다른 것은 칸 간격 자체가 정수 px가 아니어서 생기는 정상 결과다.

### 3.2 누적 배율 읽기

- 출처: `GridCanvas->GetPaintSpaceGeometry().GetAccumulatedRenderTransform()`. 단위 벡터 `(1,0)`, `(0,1)`을 `TransformVector`한 길이를 각각 X·Y 배율로 쓴다. ScaleBox(layout transform)와 조상 render transform, window root 배율이 모두 포함된다. world-space `UWidgetComponent`의 window 공간은 render target 픽셀 공간이다.
- geometry가 아직 없거나(paint 전) 배율이 유한한 양수가 아니면 `(1,1)`로 본다. 이 경우 첫 polling에서 배율 1로 그리고, 실제 배율이 잡힌 다음 polling에서 아래 rebuild key 변화로 다시 그린다.
- `ResolveCanvasSize`의 cached geometry 기반 크기 계산은 바꾸지 않는다.

### 3.3 Rebuild key

- 기존 key(canvas 크기, Zone transform, Zone extent, 굵은 칸 간격)에 `LastGridRenderScale`(FVector2D)을 더한다. 배율 비교는 기존 `Equals` 기본 허용오차를 쓴다.
- `ClearGrid`는 `LastGridRenderScale`도 초기화한다.
- 두께·색 프로퍼티는 `EditDefaultsOnly`라 runtime에 바뀌지 않으므로 key에 넣지 않는다.

### 3.4 선 배치 규칙 (현재 규칙 유지 + 두 가지 정리)

- 유지: 간격 = `UFacilityPlacementSettings::GetGridSizeCm()` × `PlacementZone->GetMajorGridIntervalCells()`. 선은 Zone 중심에서 `Cell × 간격` 위치. world `+X` 위, `+Y` 오른쪽. letterbox 식은 `ProjectFootprint`와 같다. 축별 선 수 guard(아래 예외 목록)도 유지한다.
- 정리 1 — 선 중심 정렬: 칸 경계 좌표가 선의 중심이 되게 `좌표 − 두께/2`에 놓는다(현재는 선의 왼쪽·위 모서리가 좌표). 두께가 바뀌어도 칸 경계가 선 가운데에 있게 하기 위해서다.
- 정리 2 — 경계와 겹치는 칸 선 생략: 좌표가 Zone 경계와 같은(허용오차 `KINDA_SMALL_NUMBER`×간격 이내) 칸 선은 만들지 않는다. 그 자리는 경계선이 그린다. 현재는 이 선이 경계선과 겹치거나 content 밖 1px로 삐져나가 clip된다.
- 경계선 네 개는 content rect 안쪽에 붙는다(현재와 같다). 두께만 3.1 규칙으로 바꾼다.

### 3.5 공유 계산 helper (클래스 성장)

`BathWaterMapWidget.cpp`는 350줄이다. 선 배치 계산을 위젯 밖 순수 helper로 옮겨 자동화 테스트가 위젯·Slate 없이 검사하게 한다.

- 새 파일: `Source/BathhouseSim/Private/UI/BathWaterMapGridLayout.h`, `BathWaterMapGridLayout.cpp`(비 UObject, `ExpansionScreenModel.h` 선례). 이름은 아래 의미를 지키면 구현이 조정할 수 있다.
  - `struct FBathWaterMapLetterbox { float PixelsPerCm; FVector2D Origin; FVector2D ContentSize; }`와 `bool ComputeBathWaterMapLetterbox(const FVector2D& CanvasSize, float ZoneHalfXCm, float ZoneHalfYCm, FBathWaterMapLetterbox& Out)` — 유한·양수 검사 포함. `UBathWaterMapWidget::ProjectFootprint`와 그리드가 같이 쓴다(현재 두 곳에 복제된 식을 하나로). `ProjectFootprint`의 public static 시그니처와 결과는 바꾸지 않는다.
  - `FVector2D ResolveRenderPixelsPerLayoutUnit(const FGeometry& PaintSpaceGeometry)` — 3.2 규칙, 무효면 `(1,1)`.
  - `float ToLayoutLineThickness(float RenderThicknessPx, float RenderPixelsPerLayoutUnit)` — `max(1, RenderThicknessPx) / 배율`, 배율 무효면 1로 본다.
  - `struct FBathWaterMapGridLine { FVector2D Position; FVector2D Size; bool bBoundary; }`와 `bool BuildBathWaterMapGridLines(const FBathWaterMapGridInput& Input, TArray<FBathWaterMapGridLine>& OutLines)`. 입력은 canvas 크기, Zone 반 extent(cm, scale 적용 뒤), 굵은 칸 간격(cm), 축별 render 배율, 두 두께(render px). 색은 위젯이 붙인다.
- `UBathWaterMapWidget::RebuildGrid`는 key 비교 → helper 호출 → 줄마다 `UBorder` 생성·`SetBrushColor`(bBoundary로 색 선택)·slot 위치/크기 적용만 한다.
- `SlateCore` 사용: `FGeometry`·`FSlateRenderTransform`의 inline 접근만 쓴다. `SlateCore`는 이미 `PrivateDependencyModuleNames`에 있어 Build.cs 변경은 없다. production 사용처가 생기므로 [CoreSystem.md](../../Architecture/CoreSystem.md) Module Rules에 반영했다.

## 4. 조정값 원본 (authoring owner)

### 4.1 지도 선 — `UBathWaterMapWidget` → `WBP_BathWaterMap` Class Defaults

| 프로퍼티 | 타입·meta | 의미 | C++ 초기값 |
|---|---|---|---|
| `GridLineThicknessPx` | float, `EditDefaultsOnly`, `ClampMin=1`, Category `Bath Water Map\|Grid` | 굵은 칸 선의 render target px 두께 | 현재 코드 상수와 같은 값(현재 화면에서 두께 차이 최소화) |
| `BoundaryLineThicknessPx` | 같음 | Zone 경계선 render px 두께 | 현재 코드 상수와 같은 값 |
| `GridLineColor` | FLinearColor, `EditDefaultsOnly` | 칸 선 색 | 현재 `GridColor` 값 |
| `BoundaryLineColor` | FLinearColor, `EditDefaultsOnly` | 경계선 색 | 현재 `BoundaryColor` 값 |

- 정본 위치는 `/Game/Bathhouse/UI/WBP_BathWaterMap` Class Defaults다. C++ 초기값은 WBP가 상속하는 시작값일 뿐이며 문서·테스트는 이 수치를 복제하지 않는다.
- `ClampMin=1`은 조정값이 아니라 3.1의 가시성 보장 하한이다(아래 예외).
- 새 프로퍼티라 `WBP_BathWaterManagementScreen`의 중첩 `BathMap` 템플릿에 기존 override가 있을 수 없다. Content 저장은 필요 없다.

### 4.2 타일 상태 표현 — `UBathWaterBathTileWidget` → `WBP_BathWaterBathTile` Class Defaults

같은 UI의 코드 상수 정리 범위에 포함한다. 버그와 같은 지도 표현이고, Editor 단계가 이 WBP를 어차피 수정하기 때문이다.

| 프로퍼티 | 타입·meta | 현재 코드 상수 |
|---|---|---|
| `DeficitTileColor` | FLinearColor, `EditDefaultsOnly`, Category `Bath Water Tile` | 용량 부족 배경색 |
| `SelectedTileColor` | 같음 | 선택 배경색 |
| `NormalTileColor` | 같음 | 기본 배경색 |
| `SelectedRenderOpacity` | float, `EditDefaultsOnly`, `ClampMin=0`, `ClampMax=1` | 선택 시 opacity |
| `UnselectedRenderOpacity` | 같음 | 비선택 opacity |

- C++ 초기값은 현재 상수 그대로 둔다(화면 변화 없음). `ApplyBathSnapshot`의 분기·우선순위(부족 > 선택 > 기본)는 바꾸지 않는다.

### 4.3 코드에 남기는 상수(예외 목록과 근거)

| 위치 | 상수 | 근거 |
|---|---|---|
| 3.1 레이아웃 두께 식, `ClampMin=1` | 최소 render 두께 1px | 래스터화 의미상 상수. 1px 미만이면 픽셀 중심을 덮지 못할 수 있다(이번 버그) |
| `ResolveCanvasSize` | cached 크기 `> 2`, canvas `> 1` | geometry 미배치 판정(엔진 의미) |
| 축별 선 수 guard | `200` | 비정상 geometry에서 widget 폭증을 막는 안전 상한. 사용자 감각값이 아니다 |
| letterbox·slot | `0.5` 중앙 정렬, alignment `0.5`, pivot `0.5` | 기하 의미(중앙) |
| 경계 겹침·유한성 판정 | `KINDA_SMALL_NUMBER` | 부동소수 0 판정 |
| 3.2 무효 배율 대체 | `(1,1)` | 단위 변환 항등값 |
| 타일 문자열 | `%.1f`, `°C`, `%` | 표시 형식 계약(Architecture Refresh And Mutation) |

## 5. Lifecycle·rollback·전역 영향

- lifecycle 변화 없음. `SetPlacementZone`·`ApplySnapshot`·`NativeDestruct`의 clear 순서 유지. 배율 key는 polling 안에서만 갱신된다.
- polling마다 `GetPaintSpaceGeometry` 한 번과 vector 비교만 추가된다. rebuild는 key가 바뀔 때만.
- 전역 설정(Project/Input/Collision/Nav)·Config 변경 없음. Core Redirect 불필요(rename·삭제 없음, UPROPERTY 추가만).
- Blueprint API: 새 프로퍼티는 `EditDefaultsOnly`만(BlueprintReadOnly 불필요). `BindWidget` 계약 변화 없음.
- 탭 root·`ManagementScale`·관리 화면 layout·타일 투영(`ProjectFootprint`, `LayoutTile`) 결과는 바꾸지 않는다.

## 6. 욕탕 타일 크기 (`MAPG-002`) — Editor 계약, C++ 변경 없음

- 계약: 타일 위젯의 보이는 영역과 클릭 영역은 `LayoutTile`이 넣은 canvas slot 사각형(footprint 투영) 전체다. `WBP_BathWaterBathTile`의 `RootOverlay` 직속 `SelectButton` OverlaySlot은 H=Fill, V=Fill이어야 한다. 내부 글자 묶음 `TileColumn`은 Center/Center를 유지한다.
- Editor 단계 범위(구현 단계는 asset을 건드리지 않는다): 위 slot 변경, [QNA_ARCHITECTURE.md](QNA_ARCHITECTURE.md) Q1 답(기본안: `TileColumn`을 `SelectButton` 아래 ScaleBox(Stretch=ScaleToFit, StretchDirection=DownOnly)로 감싸 타일보다 큰 글자만 축소)에 따른 hierarchy 변경, Compile·개별 Save·새 프로세스 재로드. `BindWidget` 이름·타입은 유지.
- C++ 쪽 영향 검토 결과(코드 수정 불필요)
  - 위치·크기·yaw: `LayoutTile`은 slot에 footprint 투영 크기와 중심(alignment 0.5)을 넣고 render pivot 0.5로 회전한다. 지금은 버튼이 slot 왼쪽 위에 붙어 회전 타일이 pivot 밖에서 돌아 위치까지 어긋난다. Fill이 되면 회전 타일도 footprint 위에 정확히 겹친다.
  - 작은 타일: 넓힘으로 Zone이 커지면 px/cm가 줄어 타일이 글자 묶음보다 작아질 수 있다(진단 값으로 추산하면 넓힘 방향에 따라 1~2회 넓힘에서 타일 가로가 글자 묶음 폭보다 작아질 수 있다). Overlay·Center 정렬은 자식을 slot 크기로 자르지 않으므로 글자가 타일 밖으로 넘친다. 처리 방식은 사용자 결과라 Q1로 분리했고 어느 답이든 WBP layout만으로 해결된다.
  - 클릭: `SelectButton`이 footprint 전체를 덮어 hit 영역이 넓어진다. 욕탕 footprint는 배치 규칙상 겹치지 않으므로 타일 간 hit 충돌은 없다. `GridCanvas`는 `HitTestInvisible`이라 가리지 않는다.
  - 표시: 상태색 배경이 footprint 전체를 칠한다. 비선택 opacity로 아래 격자가 비쳐 보이는 것은 정상이다.

## 7. 정본 정리 범위 (Editor 작업 단계 몫)

사용자 지시 "값은 기본적으로 원본참조로 작성임"과 [AGENT_WORKFLOW.md](../../AGENT_WORKFLOW.md) 조정값 원본 원칙에 따라 Editor 작업 단계가 다음을 처리한다. 수치를 고쳐 적는 것이 아니라 수치를 지우고 원본 위치(asset·component·프로퍼티, Config 파일·섹션)를 적는다. 구조 사실(Parent Class, component 이름·종류, 연결 asset)은 값이 아니라 계약이므로 실제와 다르면 정정한다.

- `.md/Unreal/PlacementSystem.md` — 불일치 근거는 [../PLACEMENT-FOOTPRINT-PREVIEW/REPORT_UNREAL_DISCOVERY.md](../PLACEMENT-FOOTPRINT-PREVIEW/REPORT_UNREAL_DISCOVERY.md) 6절.
  - `FacilityItemHeldTransform` 저장값 줄 → `Config/DefaultGame.ini`의 `UFacilityPlacementSettings` 섹션 참조.
  - Preview MI 두 개의 Emissive·Opacity 값 → 각 MI asset 파라미터 참조(실제 사용 asset이 Config 경로 쪽임을 함께 정리).
  - Definition 표 `Locker Slots` 열 → 각 Definition asset의 해당 프로퍼티 참조로 바꾸거나 열 삭제.
  - footprint 표의 `Footprint Extent`·`Footprint Relative Z`·body mesh scale 수치 → 각 Blueprint `PlacementFootprint`(BoxExtent·Relative Location)·body component 참조. 표에는 Blueprint path, Parent Class, footprint·body component 이름과 collision/Navigation 계약만 남긴다.
  - Parent Class 정정(구조 사실): `BP_Bath`, `BP_Circulator`, `BP_Cooler`(6절). 6절의 body component 구성 차이(Cooler body, Washer·Dryer `LidMesh`)도 구조 사실로 정정한다.
  - 표에 없는 6종(DrinkFridge, Vanity, MassageChair, RestBench, Television, ScrubTable)은 같은 형식(수치 없이 path·parent·component)으로 추가할지 Editor 단계가 판단한다. 값은 적지 않는다.
  - `MI_FacilityPlacementGrid` 표현값 줄 → MI asset 파라미터 이름만 남기고 값 삭제.
  - `BP_ClothesLocker`는 사용자 작업 트리 변경 중이다. asset을 열거나 저장하지 않고 문서도 원본 위치만 적는다.
  - `SceneRoot` yaw(Circulator·Boiler −1°, Cooler −90°)와 Cooler root scale은 별도 판단 예정이라 이번 작업에서 계약으로 기록하지 않는다(값 복제도 하지 않는다).
- `.md/Unreal/InteractionUISystem.md` `Bath Water 관리 화면` 절
  - 타일 `SelectButton` Fill과 Q1 결과 hierarchy를 현재 상태로 기록.
  - 지도 선 두께·색, 타일 상태색·opacity의 원본 위치(4절 WBP Class Defaults 프로퍼티)를 적는다. 수치는 적지 않는다.
  - 같은 절의 기존 수치 서술(글자 pt, 줄바꿈 폭, 행 여백 등)은 해당 WBP·widget 프로퍼티 참조로 바꾼다. 다른 절은 이번 범위 밖이다.
- 쓰기 경로: 이전 진단에서 Editor 워커의 정본 쓰기가 권한 분류기에 거부됐다. [AGENT_WORKFLOW.md](../../AGENT_WORKFLOW.md) "Editor 보고서 저장" 방식(전문을 보고 뒤에 붙이고 마스터가 저장)을 정본에도 적용하는지 마스터가 Editor 인계에서 정한다.

## 8. 구현 범위와 금지

- 수정 대상
  - `Source/BathhouseSim/Public/UI/BathWaterMapWidget.h`, `Private/UI/BathWaterMapWidget.cpp`
  - `Source/BathhouseSim/Public/UI/BathWaterBathTileWidget.h`, `Private/UI/BathWaterBathTileWidget.cpp`
  - 신규 `Source/BathhouseSim/Private/UI/BathWaterMapGridLayout.h/.cpp`
  - 신규 테스트 `Source/BathhouseSim/Private/Tests/BathWaterMapGridAutomationTests.cpp`(기존 `BathWaterOperationsAutomationTests.cpp`는 754줄이라 추가하지 않는다. 기존 `MapProjection` 테스트는 그대로 통과해야 한다)
- 금지
  - Content·Config 수정, Editor 실행, PIE. `Content/Bathhouse/Blueprints/Facility/BP_ClothesLocker.uasset`(사용자 변경)에 손대지 않는다.
  - `ProjectFootprint` 시그니처·결과, `LayoutTile`, tile rebuild 조건, `BindWidget` 계약, 컴퓨터 탭 root·ScaleBox 변경.
  - Slate draw API(`NativePaint`, `FSlateDrawElement`) 도입.
  - 진단 8절(지도·바닥 격자 50cm 어긋남 의심), Cooler 비정수 칸·Circulator/Boiler −1° 문제.
- `PROMPT_UNREAL.md`(구현 단계 작성)에는 6절 tile 작업, 4절 새 프로퍼티의 WBP 컴파일 확인(저장 불필요), 7절 정본 정리를 넘긴다. Content 변경이 있으므로 Editor 단계는 생략할 수 없다.

## 9. 자동화·빌드·리뷰 기준

빌드와 headless 실행은 [UE_BUILD_POLICY.md](../../UE_BUILD_POLICY.md)를 따른다.

- 새 테스트 `BathhouseSim.BathWater.Operations.MapGridLineLayout`(EditorContext | EngineFilter). 두께 입력은 `GetDefault<UBathWaterMapWidget>()`의 두 두께 프로퍼티에서 읽는다(접근은 기존 패턴처럼 `friend` 또는 테스트 전용 접근). 기대값은 리터럴로 복제하지 않고 입력에서 계산한다.
  - 배율 1 미만 비정수(예: 탭 바를 뺀 높이 / 관리 화면 높이 같은 fixture 입력), 1, 1 초과 세 경우에 대해: 모든 선의 render 두께(레이아웃 두께 × 해당 축 배율)가 `max(1, 입력 두께)`와 같다(허용오차 1e-4).
  - 입력 두께가 1 미만(fixture 0.5)이어도 render 두께가 1이다.
  - 가시성 회귀: 각 선의 render 시작 위치에 0~1px 소수 오프셋을 촘촘히(예: 0.01 간격) 더해 `round(start + w) − round(start) ≥ 1`이 모두 성립한다. 같은 검사를 옛 규칙(레이아웃 두께 1, 배율 < 1)에 적용하면 실패하는 오프셋이 존재함도 확인해 테스트가 버그를 잡는지 보인다.
  - 칸 선 수 = `|k × 간격| < 반 extent − 허용오차`인 정수 k 개수(축별, 루프로 계산). 경계와 같은 좌표의 칸 선은 없다. 경계선은 정확히 4개(bBoundary).
  - 모든 선이 content rect 안이고, 칸 선 중심이 `Origin + (k × 간격 + 반 extent) × PixelsPerCm` 좌표와 일치(Y축은 위 방향 반전 포함).
  - `ResolveRenderPixelsPerLayoutUnit`: `FGeometry::MakeRoot(크기, FSlateLayoutTransform(s))`에서 s를 돌려주고, 기본 geometry·0·NaN 배율은 `(1,1)`.
  - `ComputeBathWaterMapLetterbox`를 쓰도록 바꾼 뒤 기존 `MapProjection` 테스트가 수정 없이 통과.
- 타일: 기존 `NativeWidgetPresentation` 테스트 또는 새 테스트에서 `SelectButton`이 있는 타일에 선택·부족 snapshot을 적용해 배경색·opacity가 같은 타일 객체의 프로퍼티 값과 같음을 확인한다(값 리터럴 비교 금지).
- 빌드: Editor target 빌드 성공. 실행: `BathhouseSim.BathWater.Operations` 필터 전체.
- 코드 리뷰 확인점
  - `RebuildGrid`가 helper 결과만 적용하고 배율 key를 비교하는지, `ClearGrid`가 key를 모두 초기화하는지.
  - 두께·색·타일 색이 코드 리터럴로 남지 않았는지(4.3 예외만 허용).
  - `ProjectFootprint`와 그리드가 같은 letterbox helper를 쓰는지.
  - production에 Slate draw API나 새 module 의존이 추가되지 않았는지.

## 10. 사용자 PIE 관찰 시나리오 (수정과 Editor 작업 뒤)

현재 저장 상태(넓힘 0회) 기준 기대 수는 진단 2절의 원본 값에서 계산한 것이다. 원본이 바뀌면 같은 식으로 다시 계산한다.

- `MAPG-001`: 컴퓨터 `관리` 탭 지도에서 세로 칸 선(Zone Y 폭 안의 칸 경계, 현재 13개)과 가로 칸 선(경계 제외, 현재 9개)이 모두 같은 굵기·밝기로 보이고 간격이 고르다. 네 경계선이 보인다. 화면을 나갔다 다시 들어와도 같다.
- `MAPG-002`: `Bath`·`Bath2` 타일이 각각 footprint 크기(가로 = footprint Y 폭 / 굵은 칸 간격, 세로 = footprint X 폭 / 굵은 칸 간격, 현재 약 2.4×3칸)를 덮고, 위치가 실제 목욕 공간 배치와 같다. 타일 안 어디를 눌러도 해당 욕탕이 선택된다. 글자가 타일 밖으로 넘치지 않는다. yaw 90°로 설치한 욕탕이 있으면 타일도 돌아간 footprint와 겹친다.
- `MAPG-003`: 목욕 공간을 한 번 넓힌 뒤 지도 칸 수가 넓어진 Zone 크기 / 굵은 칸 간격에 맞게 늘고(진단 7절의 17×14 예상은 넓힘 방향 해석이 확인되지 않아 기대값으로 쓰지 않는다. 넓힌 뒤 Zone `FloorSizeCm`에서 계산한다) 모든 칸 선이 보이며, 타일은 줄어든 비율로 footprint를 덮는다. 타일보다 글자가 클 때는 Q1 답대로(기본안: 글자 축소) 표시된다.

## 11. 복귀 정보

복귀 재설계가 아니다. 유지되는 완료 범위: 진단 결론(지도 Zone 대응, 투영 계약, footprint와 메시 차이 정상).
