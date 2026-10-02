# PROMPT_UNREAL — PLACEMENT-FOOTPRINT-PREVIEW 배치 미리보기의 footprint 공간 표시

- 작업 ID: `PLACEMENT-FOOTPRINT-PREVIEW`
- 단계: 구현
- 상태: 완료

## 상태와 모드

- **작업 필요**(Content 변경 있음). Editor 단계를 생략할 수 없다.
- 구현(C++, Config, 테스트)은 끝났고 Content는 건드리지 않았다. 아래가 Editor 단계 몫이다.
- 작업 모드: 새 asset 생성, 기존 Blueprint 속성 수정, Project Settings 지정, Data Validation·content 테스트 실행, Unreal 정본 갱신.
- 기존 사용자 변경: `Content/Bathhouse/Blueprints/Facility/BP_ClothesLocker.uasset`이 작업 트리에서 수정된 상태다. 저장·되돌리기·커밋 모두 하지 않는다.
- S2: 아래 목록 밖 asset을 바꿔야 하면 멈추지 않고 진행하되 보고에 asset·값·이유를 적는다(`BP_ClothesLocker` 제외).
- 현재 동작 영향(`PROMPT_IMPLEMENTATION.md` 7절): BP 수정 전까지 쿨러는 CDO footprint가 비정수라 배치 초기화가 실패한다. 순환기·보일러는 Data Validation 오류만 난다. 이 작업이 해소한다.

## allowlist

생성: `/Game/Bathhouse/Materials/Placement/M_FacilityPlacementFootprint`, `/Game/Bathhouse/Materials/Placement/MI_FacilityPlacementFootprint`.

수정·저장: `BP_Cooler`, `BP_Circulator`, `BP_Boiler`(exact 경로는 Asset Registry로 찾는다), `Config/DefaultGame.ini`(Project Settings 저장), 필요하면 `DefaultMap`의 세 설비 instance.

`.md/Unreal/PlacementSystem.md`는 8항 갱신 대상이다. 이 외 asset은 읽기 전용이다.

## 항목

### 1. `M_FacilityPlacementFootprint` (Material)

- 값: Surface Translucent, Unlit, Two Sided 끔(one-sided), depth test 켬. translucency pass와 After DOF 여부는 `/Game/Material/M_Preview`(메시 미리보기 재질의 부모)와 `M_FacilityPlacementGrid`가 같은 값이면 그대로 맞춘다. 둘이 다르면 `M_FacilityPlacementGrid` 기준으로 하고 보고한다. 이유: 반투명 sort priority는 같은 pass 안에서만 비교된다.
- parameter(이름은 C++ 계약이다. 바꾸지 않는다):
  - `PreviewColor`(vector). native가 유효/무효 색으로 설정한다.
  - `FootprintSizeXCm`, `FootprintSizeYCm`(scalar). native가 footprint world 크기(cm)로 설정한다. X가 plane local X다.
  - `FillOpacity`, `OutlineOpacity`, `OutlineThicknessCm`(scalar). MI 기본값이 원본이다.
- graph: plane UV에서 가장 가까운 변까지의 거리를 cm로 구한다. UV 축과 plane local X/Y 대응은 같은 Engine plane을 쓰는 `M_FacilityPlacementGrid`의 `ZoneSizeXCm/YCm` 연결을 따른다. 그 거리가 `OutlineThicknessCm`보다 작으면 외곽선(사각형 안쪽)이다. Opacity는 외곽선이면 `OutlineOpacity`, 아니면 `FillOpacity`다. Emissive는 `PreviewColor` RGB다.
- Compile·Save 후 재로드로 확인한다.

### 2. `MI_FacilityPlacementFootprint` (Material Instance, 부모 위 Material)

- `FillOpacity`, `OutlineOpacity`, `OutlineThicknessCm`만 `QNA_FEATURE_SPEC.md` 기본값 표의 제안값(채움·외곽선 불투명도, 외곽선 두께)으로 설정한다. 이 값은 사용자 PIE에서 조정될 수 있으며 원본은 이 MI다.
- native parameter 세 개(`PreviewColor`, `FootprintSizeXCm`, `FootprintSizeYCm`)는 override하지 않는다.

### 3. Project Settings `Facility Placement`

- `FootprintPreviewMaterial` = 위 MI. `Config/DefaultGame.ini` `[/Script/BathhouseSim.FacilityPlacementSettings]` 저장을 확인한다.
- 구현이 이미 넣은 값(확인만, 바꾸지 않는다): `FootprintPreviewMesh`(Engine `Plane`), `FootprintPreviewFloorOffsetCm`, `FootprintPreviewTranslucencySortPriority`, `PreviewMeshTranslucencySortPriority`. 우선순위는 grid `GridVisual`(Zone Blueprint 값) < footprint < 메시 미리보기 관계를 만족해야 한다. 만족하지 않으면 보고한다(사용자 PIE 조정 대상).

### 4. `BP_Cooler` (FPV-016)

- `PlacementFootprint` BoxExtent X/Y만 바꿔 world footprint를 Actor 축 X 2칸, Y 3칸으로 만든다.
- `SceneRoot` Yaw −90° 때문에 footprint local X가 Actor Y다. 각 local 축 extent = 그 축 칸 수 × `GridSizeCm`(Project Settings) ÷ (2 × |root scale × 하위 합성 scale|)이다. footprint local X는 Actor Y 3칸, local Y는 Actor X 2칸이다.
- 바꾸지 않는다: Z extent, relative location·rotation, root scale, 메시.
- 확인: Data Validation 비정수 오류가 사라지고 `DeriveFootprintCells`가 Actor 축 X 2칸·Y 3칸에 해당하는 cell을 낸다.

### 5. `BP_Circulator`, `BP_Boiler`

- native `SceneRoot` relative rotation을 0으로 맞춘다(현재 Yaw −1°). 하위 component의 relative 값은 그대로 둔다.
- 수정 전에 Data Validation이 축 정렬 오류를 내는지 먼저 확인해 보고하고, 수정 뒤 사라지는지 확인한다.

### 6. `DefaultMap`의 세 설비 instance

- 세 설비의 `SceneRoot` rotation, `PlacementFootprint` extent의 instance override가 있는지 확인한다.
- 있으면 Class Default로 되돌리고 보고한다. 없으면 Map을 저장하지 않는다.
- 쿨러 footprint가 넓어져 인접 물체와 겹치면 사실만 보고한다. 위치는 옮기지 않는다.

### 7. 검증

- Data Validation: 활성 Placement Definition 16개 모두 오류 없음(Cube fallback 경고는 허용).
- content 테스트: `-ExecCmds="Automation RunTests BathhouseSim.PlacementContent.FootprintPreview; Quit"` 형식(UE_BUILD_POLICY headless 명령)으로 실행한다. 이 테스트는 3번 지정 전에는 실패하는 것이 정상이다. 실패하면 Definition 이름이 메시지에 나온다.
- 회귀: `BathhouseSim.Placement.`를 다시 실행한다.
- 쿨러·MI 저장(3·4·5항) 뒤 `BathhouseSim.Shop.` 3개(`SevenDefinitionSpawn`, `UnboxingPhysics`, `UnboxViewFront.RoomPhysics`)를 실행해 통과를 확인한다. 구현 단계 전체 실행에서 쿨러 비정수 cell(`FootprintGridMismatch`)로 실패했던 것들이다.
- 마지막에 전체 `BathhouseSim`을 실행해 실패 0을 확인한다. 기준선은 작업 전 전체 177개 실패 0(`Saved/Automation/Reports/20261002/u3_editor3`)이다.
- 실패가 남으면 테스트 이름과 메시지를 보고하고 멈춘다(asset 우회 금지).
- 남는 Warning 중 표시 준비 Warning은 transient 재질 fixture(`Param` 없음)가 원인이면 정상이다. 실제 Definition preview에서 나오면 보고한다.
- 화면 판정(가림·비침·순서·깜빡임)은 하지 않는다. `PIE_CHECKLIST.md` 대상이다.

### 8. 정본 갱신 `.md/Unreal/PlacementSystem.md`

- 새 재질·MI의 위치, Project Settings 새 키 위치, 세 Blueprint의 footprint·`SceneRoot` 현재 상태를 적는다.
- 미리보기 재질 실제 속성(`REPORT_UNREAL_DISCOVERY.md` 6절의 불일치 포함)을 반영한다.
- 수치는 복제하지 않고 원본 위치(asset·property, Config 키)를 참조한다.

## 시나리오별 사용자 PIE 관찰

- FPV-001~009(Vanity, 홀): 설비를 들면 바닥에 사각형이 보이고 메시와 같이 움직임, 유효·무효 색이 메시와 같음, 숨김 때 함께 사라짐.
- FPV-002: 반투명 메시 아래 footprint 외곽선이 알아볼 만하게 보임. 부족하면 MI 불투명도나 두 우선순위 설정으로 조정.
- FPV-003: snap 때 사각형 네 변이 grid 선과 맞음. FPV-005: 벽·설치 설비 발밑까지 닿고 불투명 물체에 가림.
- FPV-007: 숨김에서 footprint만 남지 않음.
- FPV-010~015: DrinkFridge·Boiler, Shower·Bath(바닥면과 깜빡임 없음), 세 공간 높이, 판정 회귀 없음, 16종, 초기화 실패 시 footprint도 없음.
- FPV-016: 쿨러 2×3칸·snap 정렬(좁은 자리 무효 가능), 순환기·보일러가 반듯함, Level 기존 instance 회수·재배치·잔량.

## Blueprint에서 구현하면 안 되는 것

- footprint 표시 생성, 위치·회전, 색 선택, 판정 연동, 숨김 처리는 모두 C++가 한다. 새 Blueprint 로직·`BP_FacilityPreview_*` 재도입 금지.
- 색을 MI에 별도 값으로 두지 않는다(원본은 메시 미리보기 재질 `Param`).
- Blueprint 우회로 C++ 결함(계약 불일치)을 가리지 않는다. 계약과 실제 asset이 맞지 않으면 저장하지 않고 멈춰 아키텍처로 복귀를 요청한다.
