# Unreal 작업 프롬프트 — Bath Water Operations 관리 화면

## 전제와 경계

- `.md/PROMPT_IMPLEMENTATION_R.md` 재작업과 코드 리뷰 승인 뒤 시작한다.
- Content/Level 변경은 아래 allowlist만 개별 Compile/Save한다. `Save All`을 사용하지 않는다.
- Widget Blueprint는 hierarchy/layout/style만 소유하며 domain 계산, actor 검색, slider mutation과 tile lifecycle을 Event Graph에 구현하지 않는다.
- utility 최종 visual mesh/footprint가 승인되지 않았다면 임의 placeholder를 저장하지 말고 보류 사유를 기록한다.

## 1. Utility Blueprint와 Definition

`/Script/BathhouseSim.BathWaterUtilityFacilityActor` 자식:

- `/Game/Bathhouse/Blueprints/Facility/BP_Circulator`: `CapacityKind=Circulation`, `CapacityPoints=100`
- `/Game/Bathhouse/Blueprints/Facility/BP_Boiler`: `CapacityKind=Heating`, `CapacityPoints=100`
- `/Game/Bathhouse/Blueprints/Facility/BP_Cooler`: `CapacityKind=Cooling`, `CapacityPoints=100`

각 Blueprint는 inherited `PackagePhysicalRoot`, `SceneRoot`, `VisualMesh`, `PlacementFootprint`, `FacilityPlacement`, `Capacity`를 사용한다. 중복 component, customer slot, direct primary-use와 On/Off graph를 만들지 않는다. native fallback footprint는 60×60cm지만 최종 footprint는 승인된 mesh에 맞고 20cm global grid의 정수 셀이어야 한다.

Definition:

- `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Circulator`
- `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler`
- `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Cooler`

`PlacedFacilityClass`는 대응 BP, `RecoveryItemClass`는 `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem`이다. approved `RecoveryItemMesh`를 명시하고 Definition별 runtime/Data Validation을 독립 실행한다. preview/staged actor가 capacity total에 들어가지 않는지 확인한다.

## 2. 기존 BP_Bath 확인

`/Game/Bathhouse/Blueprints/Facility/BP_Bath`에서 inherited `BathWaterCondition`을 확인하고 중복 추가/rename하지 않는다. 기존 balance override 의도가 없으면 native 기본을 유지한다.

- Max circulation demand 100
- Heating/Cooling demand 5 points/°C
- Cleaning 1 point/s at full circulation
- Contamination 0.1 point/s per actual bather
- Max target control 0.5°C/s
- Natural return 0.05°C/s

Project Settings의 ambient 20°C, target 10~50°C, step 1°C를 확인한다. 별도 변경 의도가 없으면 Config를 저장하지 않는다.

## 3. Widget Blueprint 생성

| Asset | Native parent |
|---|---|
| `/Game/Bathhouse/UI/WBP_BathWaterManagementScreen` | `UBathWaterManagementScreenWidget` |
| `/Game/Bathhouse/UI/WBP_BathWaterCapacitySummary` | `UBathWaterCapacitySummaryWidget` |
| `/Game/Bathhouse/UI/WBP_BathWaterMap` | `UBathWaterMapWidget` |
| `/Game/Bathhouse/UI/WBP_BathWaterBathTile` | `UBathWaterBathTileWidget` |
| `/Game/Bathhouse/UI/WBP_BathWaterDetail` | `UBathWaterDetailWidget` |

모든 아래 항목은 정확한 이름/타입으로 만들고 `Is Variable`을 켠다.

### Management root

- `CapacitySummary`: `WBP_BathWaterCapacitySummary`
- `BathMap`: `WBP_BathWaterMap`
- `BathDetail`: `WBP_BathWaterDetail`

### Capacity summary

- TextBlock: `CirculationCapacityText`, `HeatingCapacityText`, `CoolingCapacityText`
- ProgressBar: `CirculationCapacityBar`, `HeatingCapacityBar`, `CoolingCapacityBar`
- TextBlock: `CirculationCapacityStatusText`, `HeatingCapacityStatusText`, `CoolingCapacityStatusText`

Native가 `Normal`, `Full`, `Deficit`과 progress를 적용한다. 필요하면 `OnCapacityDisplayStateChanged`에서 색/animation만 표현하고 수치를 재계산하지 않는다. deficit에서는 bar가 1이어도 상태/부족 text를 숨기지 않는다.

### Map

- `BathTileCanvas`: CanvasPanel
- `EmptyStateText`: TextBlock
- class default `BathTileWidgetClass=WBP_BathWaterBathTile`

Canvas에 clipping을 켠다. Native가 Zone aspect-preserving letterbox, four-corner position/size/yaw와 empty state를 적용하므로 graph에서 좌표/회전을 다시 계산하지 않는다.

### Bath tile

- `SelectButton`: Button
- TextBlock: `BathNameText`, `ActualTemperatureText`, `ContaminationText`, `ThermalStatusText`, `CapacityStatusText`

Button은 타일 전체 hit target이다. `OnBathTileStateChanged`는 선택/세 deficit flag의 style만 처리할 수 있다. native OnClicked와 경쟁하는 graph를 만들지 않는다.

### Bath detail

- TextBlock: `BathNameText`, `WaterAmountText`, `ActualTemperatureText`, `TargetTemperatureText`, `ContaminationText`, `CirculationText`, `CirculationDemandText`, `HeatingDemandText`, `CoolingDemandText`, `ThermalStatusText`, `ThermalThresholdText`, `CapacityStatusText`, `FeedbackText`
- Slider: `CirculationSlider`, `TargetTemperatureSlider`

Slider 범위는 0~1이다. 온도/순환 값을 graph에서 직접 set, quantize하거나 capacity를 계산하지 않는다. `CapacityStatusText`는 복수 부족 종류의 지속 상태, `FeedbackText`는 마지막 제한 종류와 부족 포인트를 보여 줄 공간이다.

1024×576 world screen에서 summary/map/detail이 겹치지 않게 배치하고 WBP 5개 모두 BindWidget error 0건으로 Compile/Save한다.

## 4. Computer와 Level reference

- `/Game/Bathhouse/Blueprints/Computer/BP_BathhouseComputer`의 inherited `ScreenWidget.WidgetClass`를 `WBP_BathWaterManagementScreen`으로 지정한다.
- World Space, Draw Size 1024×576, Receive Hardware Input=false와 기존 transform/focus 설정은 유지한다.
- `/Game/Maps/DefaultMap`의 computer instance `BP_BathhouseComputer_C_UAID_F02F7433CA3690F802_2051456727`에서 `ManagedBathPlacementZone`을 exact actor `BP_FacilityPlacementZone_C_UAID_F02F7433CA36D1FF02_1155169559`로 지정한다.
- CDO에 Level actor reference를 넣거나 다른 Zone을 추정하지 않는다.

## 5. Validation

- utility kind/points, placed/recovery class, recovery mesh와 footprint grid 정합성
- BP_Bath에 native condition 중복 없음
- WBP parent와 모든 required BindWidget 이름/타입
- `BathTileWidgetClass` 지정
- removed property/component, duplicate native component와 Blueprint compile error 없음
- 변경 asset과 exact DefaultMap external actor만 개별 저장 후 Editor 재시작/재로드

기존 `Placement.SettingsZoneLeaseAndCompatibility`와 generic ActorReplacement Content fixture의 10/20cm grid 불일치는 이번 utility asset 오류와 섞지 말고 별도 보고한다.

## 6. PIE 수용 검증

1. 설비 미설치에서 capacity 0/0, 빈 지도/선택 해제 상세가 stale text 없이 표시된다.
2. utility 배치 성공만 해당 total을 정확히 한 번 증가시키고 preview/cancel/failure는 바꾸지 않는다.
3. provider-only 배치/회수에서 기존 Bath tile instance와 선택이 유지된다.
4. managed Zone 안 Bath만 표시되고 Zone 종횡비가 다른 Canvas에서도 중앙 letterbox와 실제 footprint 비율이 유지된다.
5. yaw 0/90/임의 각도, non-unit scale에서 `+X=위`, `+Y=오른쪽`, 위치/크기/yaw가 맞다.
6. 선택 Bath 이름, 수위, 실제/목표 수온 `0.0 °C`, 오염, 순환, 세 demand, thermal/deficit 상태가 표시된다.
7. circulation/heating/cooling deficit이 독립적으로, 복수 deficit이면 동시에 표시된다.
8. slider 증가 제한은 기존 승인값을 반대로 낮추지 않고 kind/부족 포인트를 표시한다. 감소와 ambient 횡단은 허용된다.
9. 선택 변경, context 제거와 후속 정상 mutation에서 이전 `FeedbackText`가 남지 않는다.
10. demand를 깨는 utility recovery는 hold 전 거부되고 actor/provider/item이 불변이다.
11. utility 회수 성공/rollback/예상 밖 파괴/대체 설치가 total과 deficit을 정확히 한 번 갱신한다.
12. empty Bath 회수 hold/cancel/stage/rollback에서 water/control/condition과 registry가 복원된다.
13. focus-out/re-entry에서 Widget instance와 선택이 유지되고 sample computer 및 input owner 계약이 보존된다.
14. Output Log에 Tick/UI polling spam, Ensure, NaN transform과 duplicate registration이 없다.

## 완료 보고

- 생성/변경/저장한 exact asset path
- utility mesh, footprint, Definition 값과 validation 결과
- WBP parent, exact BindWidget compile 결과
- computer exact Zone reference 재로드 결과
- PIE 시나리오별 결과와 로그
- 미승인 visual, 기존 Placement fixture 또는 unsupported Editor 작업 blocker
