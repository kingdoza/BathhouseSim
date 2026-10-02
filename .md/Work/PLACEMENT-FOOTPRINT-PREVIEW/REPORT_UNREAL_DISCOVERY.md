# REPORT_UNREAL_DISCOVERY — PLACEMENT-FOOTPRINT-PREVIEW 배치 미리보기의 footprint 공간 표시

- 작업 ID: `PLACEMENT-FOOTPRINT-PREVIEW`
- 단계: Editor 사전 조사
- 상태: 완료

(Editor 워커 전문을 마스터가 저장)

## 1. 범위와 방법

- 범위: `PROMPT_ARCHITECTURE.md` 10절 1~4항목. 읽기 전용이며 저장·Compile·resave·PIE·화면 작업을 하지 않았다. 시작 커밋은 `main` `b6ae16e`다.
- 실행 경로: Editor가 꺼져 있어 작업용 숨김 Editor 1개를 띄웠다(PID 9440, `-ModelContextProtocolStartServer -NoSplash -log -ExecCmds="py …/Saved/Claude/FPV/harness.py"`, DefaultMap). 공식 `unreal` Python을 queue 방식으로 실행했다. 앞선 BUG 진단의 harness를 `Saved/Claude/FPV/`에 복사해 썼다. MCP는 쓰지 않았다(Python 조회로 충분).
- 스크립트·결과: `Saved/Claude/FPV/01_defs.py`(→`01_defs.json`), `02_rest.py`(→`02_rest.json`), `03_extra.py`(→`03_extra.json`), 로그 `editor_9440.log`. `01_defs`의 첫 실행은 Blueprint `parent_class` 미노출 예외였고, Asset Registry tag로 바꿔 재실행해 PASS했다.
- 메시 수집은 `FacilityPlacementPreviewSource::Collect`와 같은 규칙을 따랐다. native와 SCS 전체(`SubobjectDataSubsystem`, inherited override template)를 모으고 ISM·editor-only·비가시·HiddenInGame·이름 필터(footprint/packagephysicalroot/interaction/helper/slot/pile/water/contents)를 제외했다. 좌표는 parent 체인을 합성한 root-relative transform에 root scale을 적용한 값이고 Actor Yaw는 0이다. 메시 bounds는 `StaticMesh.get_bounding_box()`의 8 모서리 AABB다.
- footprint 좌표도 같은 root 기준 합성값이다. 런타임 `GetFootprintRelativeToRoot × Actor(root scale)`와 같다.
- 기준선: dirty는 시작 `content=[] map=[]`, 각 스크립트 끝과 종료 직전에도 0이었다. 종료 후 프로세스가 사라졌다. `git status`는 시작 때와 같다.
- `Content/Bathhouse/Blueprints/Facility/BP_ClothesLocker.uasset`은 작업 트리에서 사용자가 수정 중이다(SHA-256 `6c81ae31d759aa4b97442a9201c9969b7a9874dfc32ef578ac8488322cde5967`, 조사 전후 같음). 현재 디스크 상태를 읽기만 했다. 아래 ClothesLocker_1 값은 HEAD가 아니라 이 작업 트리 값이다.

## 2. 항목 1 — footprint와 미리보기 메시 비교 (단위 cm, grid 20)

모든 footprint 중심 XY는 Actor root 원점 `(0,0)`이고 바닥은 local Z=0이다. 여백은 footprint 변에서 메시 외곽까지의 거리이며 음수면 메시가 footprint 밖으로 나간 양이다(−X/+X/−Y/+Y).

| 설비 | footprint full XY (칸) | 메시 AABB XY | footprint−메시 | 여백 −X/+X/−Y/+Y | 메시 Z 최저~최고 | 미리보기 메시(제외) |
|---|---|---|---|---|---|---|
| Bath | 300×240 (15×12) | 281.4×238.4 | +18.6/+1.6 | 6.8/11.8/4.4/**−2.8** | **−0.98**~83.0 | FacilityVisual(SM_Bath_old ×3), DrainLeverControl, FillValveControl (WaterSurfaceMesh 제외) |
| Shower | 80×120 (4×6) | 80×110 | 0/+10 | 0/0/5/5 | 44~56 (바닥에서 뜬 판) | FacilityVisual(Cube) |
| ClothesLocker_1 (작업 트리) | 240×160 (12×8) | 60×35 | +180/+125 | 90/90/62.5/62.5 | 0~100 | FacilityVisual(Cube) |
| ClothesLocker_4 | 60×140 (3×7) | 60×140 | 0/0 | 0 | 0~100 | 4개 |
| ClothesLocker_8 | 60×280 (3×14) | 60×280 | 0/0 | 0 | 0~100 | 8개 |
| Washer | 60×60 (3×3) | 60×50 | 0/+10 | 0/0/5/5 | 0~81 | MachineVisual, LidMesh |
| Dryer | 60×60 (3×3) | 60×50 | 0/+10 | 0/0/5/5 | 0~81 | MachineVisual, LidMesh |
| Circulator | 120×80 (6×4)¹ | 121.4×86.9 | −/−4.8 | 0/0/**−4.8**/0 | 0~120 | VisualMesh, LeverMesh, GaugeNeedleMesh |
| Boiler | 100×60 (5×3)¹ | 131.7×69.1 | −30.7/−7.3 | 0/**−30.7**/**−7.3**/0 | 0~129.9 | VisualMesh, GaugeFacePlate, FuelIntake, FuelDoorMesh, GaugeNeedleMesh |
| Cooler | 30×50 (**1.5×2.5**)² | 56.3×83.2 | −26.3/−33.2 | **−10.7/−15.6/−23.7/−9.5** | 0~136.8 | VisualMesh(Cooler_Body), FuelDoorMesh(Cooler_Lid), GaugeNeedleMesh(Cooler_Needle) |
| DrinkFridge | 60×60 (3×3) | 60×60 | 0/0 | 0 | 0~180 | 5개 |
| Vanity | 80×140 (4×7) | 60×120 | +20/+20 | 10/10/10/10 | 0~180 | VanityBody, MirrorVisual |
| MassageChair | 80×80 (4×4) | 80×80 | 0/0 | 0 | 0~100 | 1개 |
| RestBench | 360×100 (18×5) | 360×100 | 0/0 | 0 | 0~50 | 1개 |
| Television | 80×40 (4×2) | 80×40 | 0/0 | 0 | 0~180 | 1개 (ScreenOnVisual 비가시 제외) |
| ScrubTable | 200×80 (10×4) | 200×80 | 0/0 | 0 | 0~90 | 1개 (ScrubCursor HiddenInGame 제외) |

- ¹ Circulator·Boiler는 native `SceneRoot` relative Yaw가 **−1°**다(`PackagePhysicalRoot` > `SceneRoot` > `PlacementFootprint`·메시). footprint local 크기는 표의 정수 칸이다. root 축 기준 AABB는 121.4×82.1과 101.0×61.7이다. DefaultMap instance의 footprint world Yaw도 −1°다.
- ² Cooler는 root `PackagePhysicalRoot` scale **0.5**와 `SceneRoot` Yaw **−90°** 조합이다. extent (50,30,60)이 실제 world 크기 50×30(footprint 축)이 되고, Actor 축으로는 X 30·Y 50이다. 둘 다 20의 정수배가 아니다. DefaultMap `BP_Cooler` instance에서도 footprint world scale 0.5, Yaw −90, 2.5×1.5칸을 확인했다.
- 상세 transform·메시별 AABB: `Saved/Claude/FPV/01_defs.json`, 노드별 relative 값: `02_rest.json` `nodes`

### 대표 설비 선정용 관찰

- footprint가 메시보다 눈에 띄게 넓음(커밋 상태): **Vanity**(사방 10cm, 4×7칸, 홀), Bath(X 합 18.6cm, 15×12칸, 목욕공간), Shower·Washer·Dryer(Y 양쪽 5cm). Shower 메시는 Z 44~56의 뜬 판이라 바닥에는 메시가 없다.
- 작업 트리의 ClothesLocker_1은 footprint가 메시의 약 18배 면적(240×160 대 60×35)이다. 사용자 수정 중이라 커밋 기준값은 확인하지 않았다(정본 기록은 (30,20,50)).
- footprint와 메시 외곽이 같음: Locker_4/8, DrinkFridge, MassageChair, RestBench, Television, ScrubTable(8종 중 7종, 나머지는 위 Locker_1의 HEAD 값으로 추정되나 미확인). Q4(작거나 같을 때 표시)가 활성 설비 절반에 해당한다.
- 메시가 footprint보다 큼: Cooler(사방), Boiler(+X 게이지 바늘 30.7cm, −Y 연료문 7.3cm), Circulator(−Y 레버·게이지 4.8cm). 8절의 조사 전 후보(Circulator·Boiler·Cooler)는 "footprint가 넓은 설비"에 해당하지 않는다.
- 추천: 커밋 상태 기준 대표 설비는 **Vanity**다(모든 변에 같은 여백이 있고 회전 확인에도 쓸 수 있는 4×7 비정사각형). 차이가 아주 큰 사례가 필요하면 사용자 작업 트리의 ClothesLocker_1을 쓸 수 있으나, 사용자가 확정할 값이다.

## 3. 항목 2 — 실제 사용 미리보기 재질

- Project Settings 저장값(CDO)과 `Config/DefaultGame.ini` `[/Script/BathhouseSim.FacilityPlacementSettings]`가 같다: `ValidPreviewMaterial=/Game/Material/MI_Preview_Valid`, `InvalidPreviewMaterial=/Game/Material/MI_Preview_Invalid`. **실제 사용 asset은 `/Game/Material/MI_Preview_*`다.**
- `/Game/Material/MI_Preview_Valid`·`_Invalid`: MaterialInstanceConstant, 부모 `/Game/Material/M_Preview`, base property override 없음.
  - `M_Preview`는 Translucent, Unlit, **Two Sided 꺼짐(one-sided)**, Surface, Translucency Pass After DOF, lighting mode Volumetric NonDirectional이다.
  - graph는 표현식 1개(VectorParameter `Param` → Emissive)이고 **Opacity 입력이 연결되어 있지 않다**(엔진 기본 Opacity 1.0, 반투명 blend지만 화면상 불투명에 가까움).
  - MI의 `Param`: Valid `(0,0.8,0,1)`, Invalid `(1,0,0,1)`이다.
  - Asset Registry 참조자는 없다(Config soft 참조만).
- `/Game/Bathhouse/Materials/Placement/MI_FacilityPreview_Valid`·`_Invalid`는 이름과 달리 class가 **Material**이다. Translucent, Unlit, **Two Sided**, Opacity 상수 0.35, Emissive 상수 `(0.05,1,0.05)`/`(1,0.03,0.03)`이다.
  - 배치 미리보기에는 쓰이지 않는다.
  - `MI_FacilityPreview_Valid`의 참조자는 `/Game/Bathhouse/Blueprints/Cleaning/BP_TrashCollectionZone`과 `/Game/Bathhouse/Blueprints/Service/BP_Television`이고 Invalid는 참조자가 없다.
- 명세 영향: FPV-002의 "자기 반투명 메시 아래에 놓인 부분도 메시를 통해 보인다"는 현재 재질(Opacity 1, one-sided)로는 보장되지 않는다. 설계 단계에서 그리기 순서나 재질 결정이 필요하다.

## 4. 항목 3 — 구역 grid 표시값

| 대상 | GridZOffsetCm | GridLineThicknessCm | MajorGridIntervalCells |
|---|---:|---:|---:|
| native `FacilityPlacementZoneActor` 기본값 | 0.5 | 1.0 | 10 |
| `BP_FacilityPlacementZone` Class Default | 0.5 | 1.0 | 5 |
| `BP_BathhouseSpace` Class Default | 0.5 | 1.0 | 5 |
| `Space_Hall` / `Space_Bath` / `Space_Work` instance | 0.5 | 1.0 | 5 |

- instance 값은 세 공간 모두 BP CDO와 같다. 실질 override 차이는 없다. override 플래그 자체는 `is_editor_property_overridden`이 `NOT_FOUND`를 반환해 확인하지 못했다.
- Project Settings `Grid Size Cm` = 20(CDO와 `DefaultGame.ini` 일치). major 선 간격은 5칸 = 100cm다.
- 설치 바닥 world Z(`PlacementFloor`): Hall 25, Bath 25, Work −375다. `GridVisual` relative Z 0.5(world 25.5 / 25.5 / −374.5).
- `GridVisual`: Translucency Sort Priority 0, Material `MI_FacilityPlacementGrid`.
  - 부모 `M_FacilityPlacementGrid`는 Translucent, Unlit, one-sided, depth test 정상, After DOF다.
  - MI 값은 GridOpacity 0.35, CellFillOpacity **0.30**, MajorLineThicknessMultiplier 2.0이다.
  - 선 색은 Minor `(0.35,0.38,0.40)`, Major `(0.75,0.78,0.80)`, CellFill `(0.08,0.10,0.12)`다.
- 제안값 참고: footprint 표시 높이는 grid 0.5cm보다 위여야 한다. 외곽선 두께는 grid minor 1cm, major 2cm와 구별되어야 한다.

## 5. 항목 4 — 설치 바닥 아래로 내려가는 미리보기 메시

- **Bath만 해당한다**: `FacilityVisual`(SM_Bath_old ×3) 최저 Z **−0.98cm**다. grid(+0.5)와 그 위 몇 cm의 바닥 표시는 Bath 메시 바닥면 안쪽에 놓인다.
- Cooler 최저 Z는 −0.0이다(부동소수 반올림, 사실상 0). 나머지 14종은 최저 Z ≥ 0이다.
- 바닥에 닿지 않는 메시: Shower(최저 44cm)다. Washer·Dryer `MachineVisual`은 0~8cm 얇은 판이다.

## 6. 정본 불일치 목록 (정본은 수정하지 않음)

`.md/Unreal/PlacementSystem.md` 기준이다.

- BP_Bath: footprint (145,120,38) → 실제 (150,120,38), Parent `BathhouseFacilityActor` → 실제 `BathhouseBathFacilityActor`(기존 보고와 같음)
- BP_Shower: footprint (40,55,28) → 실제 (40,60,28)
- BP_ClothesLocker: 정본 (30,20,50), 작업 트리 디스크 (120,80,50)다. 사용자가 수정 중이라 불일치로 판정하지 않고 사실만 기록한다.
- BP_Washer·BP_Dryer: footprint (30,25,40) → 실제 (30,30,40)다. body는 `MachineVisual` 외에 `LidMesh`도 미리보기에 포함된다.
- BP_Circulator: Parent `BathWaterUtilityFacilityActor` → 실제 `BathWaterCirculatorFacilityActor`. `SceneRoot` Yaw −1°는 기록되어 있지 않다.
- BP_Boiler: `SceneRoot` Yaw −1°는 기록되어 있지 않다(Parent·extent는 일치).
- BP_Cooler: Parent `BathWaterUtilityFacilityActor` → 실제 `BathWaterCoolerFacilityActor`. body `SM_Facility_sample` scale (1.0,0.6,1.2)는 실제로 `Cooler_Body`·`Cooler_Lid`·`Cooler_Needle`(scale 1)이다. root scale 0.5와 `SceneRoot` Yaw −90°는 기록되어 있지 않다.
- 미리보기 재질: 정본은 `MI_FacilityPreview_*`를 "Material 두 개"로 적고 Config는 `/Game/Material/MI_Preview_*`라고 적어 사실 자체는 맞다. 다만 실제 사용 asset의 속성(one-sided, Opacity 미연결)은 기록되어 있지 않다.
- `MI_FacilityPlacementGrid` `CellFillOpacity`: 정본 0.08 → 실제 0.30
- `FacilityItemHeldTransform`: 정본 Location (30,−50,0) → Config Translation (10,50,−60)(Definition 밖이지만 같은 문서라 기록)
- 정본 표에 없는 6종(DrinkFridge, Vanity, MassageChair, RestBench, Television, ScrubTable)의 값은 2절 표에 있다.

## 7. 명세·설계에 영향이 있는 추가 관찰 (판정은 소유 단계)

- **Cooler footprint가 grid 정수배가 아니다.** 런타임 world footprint는 1.5×2.5칸이다. 그런데 Data Validation은 16개 Definition·BP 모두 VALID다(Definition 경고는 공통 "Recovery Item Mesh unset"뿐).
  - `DeriveFootprintCells`가 `PlacementFootprint->GetComponentTransform().GetScale3D()`를 쓰고, CDO component의 이 scale은 1이라 root scale 0.5가 반영되지 않는다.
  - 결과적으로 Cooler는 FPV-003(snap 시 네 변이 grid 선과 겹침)과 FPV-013(칸 수와 같게)을 만족하지 못한다. 반 칸 어긋난다.
  - 판정·Validation 계약 문제이므로 아키텍처 또는 별도 버그로 넘길 후보다.
- **Circulator·Boiler footprint가 Actor 축에서 −1° 돌아 있다.** snap해도 footprint 변이 grid 선과 평행하지 않고 모서리에서 최대 약 1cm 어긋난다. 의도된 authoring인지 확인이 필요하다.
- Cooler는 footprint 축이 Actor 축과 90° 다르다. 표시는 실제 footprint world transform을 따라야 판정 영역과 일치한다(명세 3절 2항과 같은 결론).
- DefaultMap에는 Definition 밖 Actor(`BathhouseExit`, `DryingSpot`, `ShoeLocker_*`, opt-out `CleanTowelStack`·`UsedTowelBin`)에도 `PlacementFootprint`가 있다. 일부는 0.5·2.5칸이다. 배치 대상이 아니라 이번 범위 밖이다(Q6 B로 설치된 설비 표시를 할 때만 관련).

## 8. 미확정·한계

- ClothesLocker_1의 HEAD footprint 값은 읽지 않았다(작업 트리 파일만 로드 가능, 정본 기록 (30,20,50)).
- 화면에서의 실제 가림·그리기 순서는 숨김 Editor로 판정할 수 없다(FBK-003). 재질 속성만 기록했다.
- 메시 bounds는 StaticMesh bounding box(렌더 bounds) 기준이며 collision 형상과 다를 수 있다.
