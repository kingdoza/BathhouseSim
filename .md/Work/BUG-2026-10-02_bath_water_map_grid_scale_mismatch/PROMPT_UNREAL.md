# PROMPT_UNREAL — 욕탕 타일 Fill·정본 원본 참조 정리

- 작업 ID: `BUG-2026-10-02_bath_water_map_grid_scale_mismatch`
- 단계: 구현
- 상태: 완료

## 1. 상태

- 작업 필요(Content 변경 있음). Editor 단계를 생략할 수 없다. 현재 단계: 코드 리뷰 승인 뒤 Editor 작업.
- 작업 모드: 읽기 확인 + 승인된 asset 수정·저장 + 정본 문서 수정. PIE는 사용자가 한다.
- 기존 사용자 변경: `Content/Bathhouse/Blueprints/Facility/BP_ClothesLocker.uasset`(건드리지 않는다, 열거나 저장하지 않는다). 다른 worktree `C:\UnrealProjects\BathhouseSim-FPV`도 건드리지 않는다.

## 2. 수정·저장 allowlist

| asset | 작업 | 저장 |
|---|---|---|
| `/Game/Bathhouse/UI/WBP_BathWaterBathTile`(Parent `UBathWaterBathTileWidget`) | 3절 hierarchy 변경 | Compile + 개별 Save + 새 프로세스 재로드 |
| `/Game/Bathhouse/UI/WBP_BathWaterMap`(Parent `UBathWaterMapWidget`) | Compile 확인만. 새 프로퍼티 4개 노출 확인. 값 변경·저장 불필요 | 저장 안 함 |
| `/Game/Bathhouse/UI/WBP_BathWaterManagementScreen` | 중첩 `BathMap` Compile 오류 없음 확인만 | 저장 안 함 |

- 그 밖 asset은 수정·저장하지 않는다. 3절 변경은 `BindWidget` 이름·타입(`SelectButton` Button, `BathNameText`·`ActualTemperatureText`·`ContaminationText`·`ThermalStatusText`·`CapacityStatusText` TextBlock)을 바꾸지 않는다.

## 3. `WBP_BathWaterBathTile` 계약 (MAPG-002)

- `RootOverlay` 직속 `SelectButton`의 OverlaySlot H=Fill, V=Fill(현재 Left/Top). 타일 위젯의 보이는 영역과 클릭 영역이 `UBathWaterMapWidget::LayoutTile`이 canvas slot에 넣는 footprint 사각형 전체가 되어야 한다. 현재 `TileColumn`은 Center/Center이며 유지한다.
- Q1(타일이 글자 묶음보다 작아질 때)은 [QNA_ARCHITECTURE.md](QNA_ARCHITECTURE.md)에서 사용자 답 A로 확정(마스터 확인 필요 시 해당 파일을 다시 읽는다). 답에 따라 갈리는 항목:
  - 답 A(기본안, 현재 적용): `TileColumn`을 `SelectButton` 아래 ScaleBox(Stretch=ScaleToFit, StretchDirection=DownOnly)로 감싼다. 타일보다 큰 글자만 축소, 충분하면 크기 변화 없음. 새 ScaleBox 이름은 Editor 단계가 정한다(BindWidget 아님).
  - 답 B(참고): `SelectButton` Clipping=ClipToBounds, ScaleBox 없음.
  - 답 C(참고): 변경 없음(Fill만).
  - Q1 답이 A가 아니면 이 항목만 위 대안으로 바꾼다. Fill 변경과 나머지 계약은 같다.
- 확인 지점: ScaleBox가 `SelectButton`의 클릭을 가리지 않는다(버튼 자식이므로 영향 없음을 확인). 상태색은 `SelectButton` BackgroundColor, 선택 표현은 C++가 쓴다(`DeficitTileColor` 등 새 프로퍼티는 이 WBP Class Defaults에서 조정; 값은 이번에 바꾸지 않는다).
- 실행 경로 힌트: 진단 때 WidgetTree 읽기는 MCP·Python으로 가능했다. Overlay slot 속성(`HorizontalAlignment`, `VerticalAlignment`)과 ScaleBox 삽입(WidgetTree 부모 교체)은 Editor Python/MCP 가능 여부를 실제 capability로 판정한다(미확인 항목, 불가하면 `USER_UNREAL.md`로 인계).
- 멈춰야 하는 조건: 실제 hierarchy가 진단 보고(RootOverlay > SelectButton > TileColumn)와 다르면 저장하지 말고 보고. `BindWidget` 이름이 바뀌는 경우 저장 금지.

## 4. 새 프로퍼티 노출 확인 (저장 없음)

- `WBP_BathWaterMap` Class Defaults: `GridLineThicknessPx`, `BoundaryLineThicknessPx`, `GridLineColor`, `BoundaryLineColor`(Category `Bath Water Map|Grid`). `WBP_BathWaterBathTile` Class Defaults: `DeficitTileColor`, `SelectedTileColor`, `NormalTileColor`, `SelectedRenderOpacity`, `UnselectedRenderOpacity`(Category `Bath Water Tile`). C++ 초기값이 이전 코드 상수와 같아 화면은 변하지 않는다. 값을 문서에 복제하지 않는다.
- 새 프로세스 재로드 뒤 위 프로퍼티를 읽을 수 있는지와 Compile 오류가 없는지만 확인한다.

## 5. 정본 정리 범위 (PROMPT_IMPLEMENTATION 7절 그대로)

수치를 고쳐 적는 것이 아니라 지우고 원본 위치(asset·component·프로퍼티, Config 파일·섹션)를 적는다. 구조 사실(Parent Class, component 이름·종류, 연결 asset)은 실제와 다르면 정정한다. 쓰기 경로: 정본 쓰기가 하네스에 거부되면 [AGENT_WORKFLOW.md](../../AGENT_WORKFLOW.md) "Editor 보고서 저장" 방식(전문을 보고 뒤에 붙이고 마스터가 저장)을 쓰는지 마스터가 정한다.

- `.md/Unreal/PlacementSystem.md` — 불일치 근거는 [../PLACEMENT-FOOTPRINT-PREVIEW/REPORT_UNREAL_DISCOVERY.md](../PLACEMENT-FOOTPRINT-PREVIEW/REPORT_UNREAL_DISCOVERY.md) 6절.
  - `FacilityItemHeldTransform` 저장값 줄 → `Config/DefaultGame.ini`의 `UFacilityPlacementSettings` 섹션 참조.
  - Preview MI 두 개의 Emissive·Opacity 값 → 각 MI asset 파라미터 참조(실제 사용 asset이 Config 경로 쪽임을 함께 정리).
  - Definition 표 `Locker Slots` 열 → 각 Definition asset 프로퍼티 참조로 바꾸거나 열 삭제.
  - footprint 표의 `Footprint Extent`·`Footprint Relative Z`·body mesh scale 수치 → 각 Blueprint `PlacementFootprint`(BoxExtent·Relative Location)·body component 참조. 표에는 Blueprint path, Parent Class, footprint·body component 이름과 collision/Navigation 계약만 남긴다.
  - Parent Class 정정(구조 사실): `BP_Bath`, `BP_Circulator`, `BP_Cooler`(위 근거 6절). Cooler body, Washer·Dryer `LidMesh` 등 body component 구성 차이도 정정.
  - 표에 없는 6종(DrinkFridge, Vanity, MassageChair, RestBench, Television, ScrubTable)은 같은 형식(수치 없이 path·parent·component)으로 추가할지 Editor 단계가 판단. 값은 적지 않는다.
  - `MI_FacilityPlacementGrid` 표현값 줄 → MI asset 파라미터 이름만 남기고 값 삭제.
  - `BP_ClothesLocker`는 사용자 작업 트리 변경 중이다. asset을 열거나 저장하지 않고 문서도 원본 위치만 적는다.
  - `SceneRoot` yaw(Circulator·Boiler·Cooler)와 Cooler root scale은 별도 판단 예정이라 계약으로 기록하지 않는다(값 복제도 하지 않는다).
- `.md/Unreal/InteractionUISystem.md` `Bath Water 관리 화면` 절
  - 타일 `SelectButton` Fill과 Q1 결과 hierarchy를 현재 상태로 기록.
  - 지도 선 두께·색(`WBP_BathWaterMap` Class Defaults 4개), 타일 상태색·opacity(`WBP_BathWaterBathTile` Class Defaults 5개)의 원본 위치를 적는다. 수치는 적지 않는다.
  - 같은 절의 기존 수치 서술(글자 pt, 줄바꿈 폭, 행 여백 등)은 해당 WBP·widget 프로퍼티 참조로 바꾼다. 다른 절은 범위 밖.
- 갱신 대상 정본은 위 두 파일이다. 아키텍처 정본(`BathWaterManagementUISystem.md`, `UISystem.md`, `CoreSystem.md`)은 아키텍처 단계에서 이미 반영됐다.

## 6. 사용자 PIE 관찰 항목 (시나리오별)

현재 저장 상태 기준 칸 수는 원본 값(Zone `FloorSizeCm`·굵은 칸 간격)에서 계산한다.

- `MAPG-001`: 컴퓨터 `관리` 탭 지도의 세로·가로 칸 선이 모두 같은 굵기·밝기로 보이고 간격이 고르다. 네 경계선이 보인다. 화면을 나갔다 들어와도 같다.
- `MAPG-002`: `Bath`·`Bath2` 타일이 각각 footprint 크기를 덮고 위치가 실제 목욕 공간 배치와 같다. 타일 안 어디를 눌러도 해당 욕탕이 선택된다. 글자가 타일 밖으로 넘치지 않는다. yaw 90° 욕탕이 있으면 타일도 돌아간 footprint와 겹친다.
- `MAPG-003`: 목욕 공간을 한 번 넓힌 뒤 지도 칸 수가 넓어진 Zone 크기 / 굵은 칸 간격에 맞게 늘고 모든 칸 선이 보이며, 타일은 줄어든 비율로 footprint를 덮는다. 타일보다 글자가 클 때는 Q1 답(A: 글자 축소)대로 표시된다.

## 7. Blueprint에 구현하면 안 되는 것

- 선 두께 보정, 배율 읽기, 선 배치, 타일 투영, 상태 분기(부족 > 선택 > 기본)는 모두 C++가 소유한다. WBP는 hierarchy·layout·값 authoring만 한다. Blueprint Event Graph로 우회 로직을 만들지 않는다.
