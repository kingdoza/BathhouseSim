# 사용자 Unreal 후속 작업

## FuelIntake migration — 직접 Editor 검증 대기

### 현재 상태

- 사용자가 이번 대화에서 코드 리뷰 승인 관문을 건너뛰도록 지시했다. Unreal MCP 서버에 실제 연결해 `/Game/Bathhouse/Blueprints/Facility/BP_Boiler`만 Compile·개별 Save했다. 기존 `FuelIntake` class와 Definition 참조는 유지했고 새 판정 Volume·문 Cube를 설정했다. 정확한 값은 `.md/Unreal/UtilityLaborSystem.md`에 있다.
- 같은 Editor에서 `/Game/Maps/DefaultMap`을 다시 로드하자 기존 보일러 instance가 새 Class Default를 상속했다. Definition과 Map은 저장하지 않았고 BP·Definition·Map의 dirty는 false, PIE는 종료 상태였다. 짧은 자동 PIE에서 보일러 provider 등록 오류는 없었으나 직접 입력·시각 수용은 하지 않았다.
- 이후 새 Editor 프로세스로 재로드하려 했으나 MCP 포트는 열려도 `initialize` 응답이 시간 초과됐고 시작 로그는 Slate 초기화 뒤 진행되지 않았다. 이 재시작 검증은 미완료다. 작업 소유 백그라운드 Editor는 종료했다. 복구 패키지 창 등 UI 상태를 MCP로 확인할 수 없어 원인은 확정하지 않았다.

### 필요한 직접 조작과 확인

1. UE 5.8 Editor에서 `/Game/Bathhouse/Blueprints/Facility/BP_Boiler`를 열고 `Data Validation`을 실행한다. `FuelIntake` 외형은 NoCollision, `FuelIntakeVolume`은 QueryOnly·Visibility Block, BoxExtent `(15,4,11)` cm·Scale `(1,1,1)`, 문 mesh와 바늘 mesh는 필수·NoCollision인지 확인한다. 오류가 나면 임의 저장하지 말고 오류 내용을 남긴다. 현재 MCP에는 Data Validation 실행 tool이 없다.
2. 새 Editor 세션에서 BP와 `/Game/Maps/DefaultMap` 보일러 instance를 다시 열어 새 Volume·문 설정, 기존 Definition 연결과 instance override가 유지되는지 확인한다. 복구 패키지 창이 뜨면 기존 작업과 autosave 내용을 비교한 뒤 선택하고, 무조건 복구·건너뛰지 않는다. 이 항목은 백그라운드 재시작이 초기화 단계에서 멈춰 확인하지 못했다.
3. BP 컴포넌트 미리보기에서 `FuelDoorPresentation` preview-open/restore-closed와 `GaugePresentation` construction preview/restore를 실행해 피벗 축, 문 닫힘 자세, 바늘 기준 회전이 유지되는지 확인한다. 현재 MCP는 이 native 호출과 시각 판정을 제공하지 않는다.
4. PIE에서 `.md/PROMPT_UNREAL.md`의 E 퍼담기·반환·투입, LMB/F 비변경, 투입 Volume 단독 판정, 문 자동 열림·닫힘/중간 반전·두 보일러 독립, 가열·소진·재투입·회수 rollback을 실제 입력과 화면으로 확인한다. 실패하면 조준 위치와 Output Log를 기록한다.

위 항목과 Data Validation이 통과해야 통합 리뷰에서 Editor 단계를 승인할 수 있다. `Save All`은 사용하지 않는다.

# 보일러 노동 가동 수직 구현 — MCP 미지원 작업

## 현재 확인 상태

`/Game/Bathhouse/Blueprints/Facility/BP_Boiler`는 `/Script/BathhouseSim.BathWaterBoilerFacilityActor`로 reparent했고 `FacilityPlacement.Definition=/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler`와 임시 Cube 투입구·계기·바늘을 저장·재로드했다. `/Game/Bathhouse/Blueprints/Utility/BP_UtilityShovel`과 `BP_CoalSupply`도 임시 Cube/`SM_Facility_sample`로 생성·컴파일·저장·재로드했다. 새 Editor에서 기존 보일러 instance는 새 component 값을 상속했고 기본 PIE에서 필수 투입구 누락 경고가 재발하지 않았다.

이전 MCP 세션의 개별 `save_actor`는 World Partition 외부 액터에 `Asset does not exist: /Game/__ExternalActors__/...`를 반환했다. 이번 새 배경 Editor에서 `DefaultMap`을 로드했을 때 석탄 공급함·삽·전용 거치대 actor 세 개가 다시 발견됐다. 다만 각 external actor의 디스크 저장 경로와 `AssignedItem`의 exact 참조는 이번 작업에서 확인하지 않았으므로 영속 상태를 단정하지 않는다.

## 1. DefaultMap 액터 배치·외부 액터 저장

`/Game/Maps/DefaultMap`에서 아래 세 Blueprint instance가 이미 있는지 먼저 확인한다. 없을 때만 재배치한다. 이전 배치에서 바닥 trace는 두 위치 모두 Z=0이었다.

| Actor | Blueprint | 제안 world Location |
|---|---|---|
| `CoalSupply` | `/Game/Bathhouse/Blueprints/Utility/BP_CoalSupply` | `(550,-850,0)` |
| `UtilityShovel` | `/Game/Bathhouse/Blueprints/Utility/BP_UtilityShovel` | `(425,-800,45)` |
| `ShovelSlot` | `/Game/Bathhouse/Blueprints/Interaction/BP_PhysicalCarryFixedSlot` | `(425,-800,45)` |

`ShovelSlot.AssignedItem`이 **그 레벨의 `UtilityShovel` 인스턴스**를 가리키는지, `bStartOccupied=true`, `SlotDisplayName=삽 거치대`인지 확인한다. `ItemAnchor`와 삽의 시작 world transform은 둘 다 `(425,-800,45)`/회전 0이 목표다. 필요한 변경이 있을 때만 `DefaultMap`과 해당 World Partition 외부 액터를 개별 저장하고, 다시 열어 세 actor와 exact 참조가 유지되는지 확인한다. `Save All`은 사용하지 않는다.

## 2. Capacity Summary 실제 화면 검증

`/Game/Bathhouse/UI/WBP_BathWaterCapacitySummary`의 기존 9개 BindWidget을 유지한 상태로 제목·3열 레이블, 글자 크기, 줄바꿈과 여백을 Editor Python API로 개별 저장했다. 새 프로세스 재로드와 Blueprint Compile `BS_UP_TO_DATE`를 확인했다. 이제 1024×576 컴퓨터 화면에서 세 종류의 `예약 Used / 가동 Active / 설치 Total` 및 설치 부족·가동 부족 동시 상태가 잘리지 않고 아래 지도·상세 영역을 누르지 않는지 직접 확인한다. 자산을 다시 편집할 필요는 없으며, 잘림이 보일 때만 해당 화면을 캡처해 재작업 대상으로 돌린다.

## 3. 검증

세 Blueprint의 Data Validation, 실제 LMB 삽·투입구 판정, G/E/F/Q, 석탄 잔량·가동·바늘 회전 및 1024×576 화면 판독성을 `.md/PROMPT_UNREAL.md`의 LAB 시나리오대로 확인한다. 자동 기본 PIE 시작만으로 실제 입력/시각 수용을 대신하지 않는다.

# 욕탕 물 순환·가열·냉각과 컴퓨터 제어

## 관리 화면 직접 플레이 검증

관리 WBP 5개의 대비·배치와 지도 `GridCanvas`, 중첩 `BathMap.BathTileWidgetClass`가 저장됐다. native 격자·욕탕명과 종료 중 갱신 방지 코드는 UE 5.8 Editor DLL로 링크됐다. 자동 PIE의 1024×576 RenderTarget에서 Zone 격자·경계와 욕탕 타일 2개, 전체 폭 capacity summary, detail 패널이 보였다. 타일 Button `OnClicked` 이벤트 호출로 Bath 선택과 두 slider 활성화도 확인했다. 이는 실제 마우스 hit test를 대신하지 않는다.

사용자 Editor를 새로 열고 PIE에서 컴퓨터 Focus 후 두 타일을 각각 **실제 LMB로** 클릭해 선택 강조, 상세 패널, 두 slider가 갱신되는지 확인한다. 이어 slider를 움직여 feedback과 욕탕 값 변화를 확인한다. 클릭되지 않으면 조준 위치와 타일 hit test를 보고한다. 기존 sample 화면에서 LMB 클릭이 성공했으므로 입력 매핑은 임의 변경하지 않았다.

## 현재 상태

C++ Source와 코드 단계 검증은 완료됐다. Unreal Editor API/Python으로 다음 Content/Level 저장 상태를 확인했다.

- `BP_Circulator`, `BP_Boiler`, `BP_Cooler`와 대응 Placement Definition 3개, 관리 WBP 5개는 생성·Compile·개별 Save됐다.
- utility visual은 모두 `/Game/Bathhouse/Meshes/SM_Facility_sample`, recovery mesh는 `None`으로 native Cube fallback을 사용한다. Boiler/Cooler footprint는 100×60cm, Circulator는 120×80cm이며 높이는 모두 120cm다.
- `BP_Bath`에는 inherited `BathWaterCondition`이 정확히 하나 있으며 demand/rate 7개가 모두 native 기본값과 일치한다.
- Bath Water Project Settings는 ambient `20°C`, target `10~50°C`, step `1°C`로 일치하므로 Config 변경이 필요 없다.
- `BP_BathhouseComputer.ScreenWidget.WidgetClass`는 `/Game/Bathhouse/UI/WBP_BathWaterManagementScreen`을 사용하며 World Space, Draw Size `1024×576`, Hardware Input `false`를 유지한다.
- DefaultMap exact computer instance의 `ManagedBathPlacementZone`은 지정된 `BP_FacilityPlacementZone` actor를 참조한다. 해당 World Partition external actor package만 저장하고 재로드로 참조 유지 확인했다.
- WBP 5개는 native parent, 필수 `BindWidget` 이름·타입, Map CDO와 중첩 템플릿의 tile class, Canvas clipping을 검사했다. 여섯 Blueprint의 Data Validation은 `VALID`다. PIE에서 타일 2개 생성과 Button 이벤트 이후 상세·slider 갱신을 확인했다.

## 남은 Editor 작업

1. `.md/PROMPT_UNREAL.md`의 PIE 14개 시나리오로 capacity/demand clamp, 지도 projection, 실제 LMB 선택, slider/feedback, deficit, utility와 Bath recovery transaction을 직접 검증한다. 자동 PIE는 RenderTarget과 Button 이벤트까지만 확인했으며 물 제어 전체 수용을 선언하지 않는다.

Utility Definition 3개는 새 Editor에서 Data Validation `VALID`였다. 각각 recovery mesh `None`에 대한 native Cube fallback 경고만 있다. 새 Editor에서 컴퓨터의 Level Zone 참조와 화면 Widget 연결도 재확인했다.

## 유틸리티 임시 표현 결정

사용자 승인에 따라 세 utility 모두 `/Game/Bathhouse/Meshes/SM_Facility_sample`을 임시 visual로 사용하고, recovery mesh는 비워 native Cube fallback을 사용한다. Boiler/Cooler는 100×60×120cm, Circulator는 120×80×120cm로 저장했다. 최종 art 교체 시 Blueprint 클래스·Capacity 종류·StableId는 유지하고 VisualMesh scale과 PlacementFootprint만 실제 mesh에 맞춰 다시 authoring한다.

# 급수·배수 시스템과 욕탕 물 수직 구현

## 현재 상태

Unreal MCP로 `/Game/Bathhouse/Data/DA_CustomerRoutine_Default`의 욕탕 체류·탐색·dwell 값을 저장하고 새 Editor 프로세스에서 재로드 확인했다. `CustomerUsableThresholdPercent`는 native 기본값이 이미 `80.0`이고 `DefaultGame.ini` override가 없어 Project Settings 변경은 필요 없다.

사용자 승인에 따라 `/Game/Bathhouse/Blueprints/Facility/BP_Bath`는 새 native parent로 전환하고 임시 Cube 밸브·레버, `SM_Bath_01_Water`, `NS_HoneyBeam`을 연결했다. Compile·개별 Save·새 Editor 재로드와 기존 Level instance 반영까지 확인했다. 상세 저장값은 `.md/Unreal/BathWaterSystem.md`가 정본이다. `/Game/Bathhouse/AI/ST_CustomerRoutine`은 변경하지 않았다.

## 1. `ST_CustomerRoutine` BathLoop migration

현재 Unreal MCP에는 StateTree 내부 state/task/transition/binding을 읽거나 쓰는 toolset이 없다. Blueprint 우회 graph를 만들지 말고 `/Game/Bathhouse/AI/ST_CustomerRoutine`에서 다음 BathLoop flow를 직접 구성한다.

- PreShower 뒤 `Start Customer Bath Stay`는 한 번만 실행한다.
- Search → usable Bath 예약 → Approach 이동 → 재검증/Snap/Begin Use → BathDwell → Approach 복귀/Release를 구성한다.
- 남은 전체 stay가 있으면 마지막 Bath 제외 우선으로 다시 Search하고, search/stay 만료면 Main Shower로 진행한다.
- `Customer.Event.BathSearchExpired`, `Customer.Event.BathStayExpired`, `Customer.Event.BathBecameUnusable` 전이와 cleanup 경로를 연결한다.
- `BathSearchExpired`, `CurrentBathUsable`, `CurrentBathExitPending`은 query-only condition으로 사용한다.
- 물 양, control 상태, actual seconds, exit-pending을 StateTree에서 직접 set하지 않는다.

완료 뒤 StateTree Compile과 binding 오류 0건을 확인하고 해당 StateTree만 개별 저장·재로드한다.

## 2. 급수·배수 PIE 수용 검증

StateTree 작업 뒤 선배치/재설치 Bath, 충전·배수 시간, 동시 개방 순유량, 80% 예약 임계값, 수위 하락 cleanup, 반복 BathLoop, knockdown timer, Q Hold recovery/rollback을 검증한다. Cube 조작부 회전, 수면 높이·가시성, `NS_HoneyBeam` 방향과 크기도 실제 플레이 화면에서 확인한다. `LogBathhouseCustomerBath Log`와 필요 시 `LogStateTree VeryVerbose`를 사용하고 Tick별 로그나 Blueprint Print String은 추가하지 않는다.

# 기존 설비 배치 후속 작업

## 상태

Definition/Blueprint migration, footprint·body Navigation authoring, preview Material, native PlacementZone grid Material/MI와 Blueprint 연결은 저장·재시작 확인까지 완료됐다. 아래 항목은 현재 Unreal MCP가 Project Settings 또는 World Partition external actor를 디스크에 저장하지 못하거나, 실제 플레이어 입력·시각 판정이 필요해 남아 있다.

`Save All`은 사용하지 않는다. 아래에서 지정한 설정과 `/Game/Maps/DefaultMap` 관련 external actor만 저장한다.

## 1. Project Settings 연결

1. **Edit > Project Settings > Game > Facility Placement**를 연다.
2. `Facility Item Held Transform`을 다음 값으로 둔다.
   - Location `(0,0,0)`
   - Rotation `(0,0,0)`
   - Scale `(1,1,1)`
3. `Valid Preview Material`에 `/Game/Bathhouse/Materials/Placement/MI_FacilityPreview_Valid`을 지정한다.
4. `Invalid Preview Material`에 `/Game/Bathhouse/Materials/Placement/MI_FacilityPreview_Invalid`를 지정한다.
5. Editor를 닫았다가 다시 열어 두 reference가 유지되는지 확인한다.

현재 확인 상태: 두 Material은 Translucent/Unlit로 저장돼 있지만 Project Settings의 두 reference는 재시작 후 `None`으로 돌아왔다. MCP property setter는 메모리만 바꾸고 config를 저장하지 않았다.

## 2. RecastNavMesh Dynamic 저장

1. PIE를 중지하고 `/Game/Maps/DefaultMap`을 연다.
2. Outliner에서 `RecastNavMesh`를 선택한다.
3. **Runtime Generation**을 `Dynamic`으로 바꾼다. `Dynamic Modifiers Only`가 아니다.
4. 해당 Recast external actor와 필요한 Map 패키지만 저장한다.
5. `DefaultMap`을 닫았다가 다시 열고 값이 `Dynamic`으로 유지되는지 확인한다.

현재 확인 상태: MCP 메모리에서는 `Dynamic` 적용이 됐지만 World Partition actor 저장 호출이 external actor 패키지를 에셋으로 찾지 못했다. 재시작 후 저장값은 다시 `Dynamic Modifiers Only`였다.

## 3. Definition Data Validation 네이티브 차단점

다음 두 opt-out Definition은 의도대로 `PlacedFacilityClass=None`, `RecoveryItemClass=None` 상태지만 현재 native `UFacilityPlacementDefinition::IsDataValid()`가 opt-out을 구분하지 않아 각각 세 개의 오류를 낸다.

- `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_CleanTowelStack`
- `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_UsedTowelBin`

오류는 `Placed Facility Class must implement IPlaceableFacility`, `Recovery Item Class must derive from APlaceableFacilityItemActor`, `Placed and recovery item classes must be different`다.

Editor에서 class를 임의로 채우지 않는다. 그러면 Stack/Bin의 placement opt-out 계약이 깨진다. 구현 단계로 되돌려 `IsDataValid()`가 conversion opt-out Definition에는 placement/recovery class 검사를 적용하지 않도록 수정하고 코드 리뷰를 다시 받아야 한다. 그 뒤 Definition 9개를 Data Validation한다.

활성 Definition 7개는 helper collision/Navigation 오류 없이 검증됐고, `RecoveryItemMesh=None`에 대한 native Cube fallback 경고만 남았다.

## 4. 직접 플레이 검증

1~3번을 마친 뒤 PIE에서 Bath, Shower, Locker 1/4/8, Washer, Dryer를 각각 확인한다.

1. 설비를 들었을 때 class-default의 모든 body mesh가 preview에 나타나는지 확인한다.
2. 설치 가능 위치는 초록, 불가 위치는 빨강 반투명 재질이 모든 mesh slot에 적용되는지 확인한다.
3. 모든 footprint bottom이 Zone의 `PlacementFloor`와 일치하고 부양·매몰되지 않는지 확인한다.
4. LCtrl snap, wheel yaw, containment, blocking overlap, 네 corner floor support를 확인한다.
5. confirm 전 preview가 collision/NavMesh를 만들지 않고, confirm 뒤 body collision에 따라 NavMesh가 갱신되는지 확인한다.
6. 빈 설비 회수 시 source collision/NavMesh가 사라지고, rollback이면 복구되며, 회수 item 재배치 뒤 NavMesh가 다시 생성되는지 확인한다.
7. Clean Towel Stack과 Used Towel Bin에 placement/recovery prompt가 생기지 않는지 확인한다.
8. 설비 회수 아이템이 공통 `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem`의 `ItemRoot` Scale `(0.3,0.3,0.3)`로 생성되고 E pickup/G drop 뒤에도 같은 크기를 유지하는지 확인한다.

깨끗한 재시작 상태에서 기본 PIE 2회는 이미 통과했으며 duplicate RegistrationId, locker capacity/expansion 오류, removed property/component, Blueprint compile 오류와 Ensure는 없었다.

## 5. Native PlacementZone Grid 직접 수용 검증

현재 저장·재시작 확인 상태:

- `/Game/Bathhouse/Materials/Placement/M_FacilityPlacementGrid`와 `/Game/Bathhouse/Materials/Placement/MI_FacilityPlacementGrid`가 생성돼 있다.
- MI에서 `CellFillColor=(0.08,0.10,0.12,1)`, `CellFillOpacity=0.08`을 조정할 수 있으며 line mask 바깥의 셀 내부에만 적용된다.
- `/Game/Bathhouse/Blueprints/Placement/BP_FacilityPlacementZone`의 inherited `GridVisual`에는 `/Engine/BasicShapes/Plane.Plane`과 위 MI가 연결돼 있다.
- `DefaultMap`의 exact PlacementZone은 Bounds 2800×1800cm에 대해 GridVisual Scale `(28,18,1)`, Relative Z `0.5`로 재구성되며 기본 hidden, NoCollision, Navigation 비활성이다.
- 자동 PIE 시작·종료에서는 grid가 숨겨진 상태를 유지했고 grid 관련 runtime Error/Ensure가 없었다.

다음은 실제 입력과 화면 판정이 필요하다.

1. placement 가능한 설비 아이템을 들고 Zone을 조준한다. 현재 Definition과 compatible인 Zone만 표시되는지 확인한다.
2. 조준 Zone을 바꾸고 LCtrl을 누르고 떼는 동안 compatible grid 집합이 유지되는지 확인한다.
3. minor line이 실제 10cm snap과 일치하고, 10칸마다 major line이 나타나는지 확인한다. grid 한 장이 `ZoneBounds` 전체를 덮고 셀 내부에는 line과 분리된 fill이 보여야 한다.
4. confirm 성공, cancel, preview 실패, held item 교체, interaction suppression과 PIE 종료 각각에서 모든 grid가 즉시 숨는지 확인한다.
5. 기존 초록/빨강 preview, wheel yaw, LCtrl snap, LMB confirm, E/G/Q 입력과 prompt가 그대로 동작하는지 확인한다.
6. grid가 collision/overlap/physics/NavMesh를 만들지 않고 벽과 설비 뒤에서는 가려지는지 확인한다.
7. 같은 placement session을 유지해도 DMI 생성 또는 visibility notification 경고가 매 Tick 반복되지 않는지 Output Log를 확인한다.

`DefaultMap`에는 현재 PlacementZone이 한 개뿐이므로 서로 다른 allowed tag를 가진 여러 Zone의 동시 표시(`FP-GRID-01`)는 이 Map만으로 완전 검증할 수 없다. 별도 테스트 Level 또는 저장하지 않을 임시 Zone 구성이 준비되면 compatible/incompatible Zone을 함께 두고 확인한다.

## 재개 조건

- Project Settings의 두 Material reference가 재시작 후 유지된다.
- Recast `RuntimeGeneration=Dynamic`이 재시작 후 유지된다.
- opt-out Definition 두 개의 native Data Validation 오류가 수정된다.
- Definition 9개와 pre-placed Locker 두 개의 Data Validation이 통과한다.
- 4번 직접 플레이 검증 결과를 기록한다.
- 5번 native grid 입력·시각 검증 결과를 기록한다.

완료 후 `.md/PROMPT_INTEGRATION_REVIEW.md`의 미완료 항목을 최종 통합 리뷰에서 다시 판정한다.

# 쿨러 전체 확장과 순환기 레버 — 2026-09-26 Unreal MCP 인계

## 현재 확인 상태

- 로컬 Unreal MCP 127.0.0.1:8000 연결 후 DefaultMap에서 작업했다. 다섯 Blueprint의 warnings-as-errors Compile이 모두 통과했고 BP_Boiler, BP_Cooler, BP_Circulator, BP_UtilityShovel, 신규 BP_DryIceSupply를 각각 Save했다. 현재 세션에서 CDO 변경값을 읽어 확인했지만 새 Editor 프로세스 재로드는 하지 못했다.
- BP_Boiler CDO에 GaugeFacePlate StaticMeshComponent를 추가해 Cube, Location (0,-32,90), Scale (0.30,0.02,0.30), NoCollision, navigation/overlap off로 authoring했다. 기존 FuelIntake와 Definition은 유지했다.
- BP_Cooler parent는 /Script/BathhouseSim.BathWaterCoolerFacilityActor다. 본체·footprint·Capacity=Cooling/100, Operation=100/1은 보존했다. CDO FuelIntakeVolume은 (0,-42,42), extent (15,4,11), unit scale, native QueryOnly/Visibility Block이다. Cube needle/door, gauge/door presentation, FacilityPlacement.Definition=DA_FacilityPlacement_Cooler를 설정했다.
- BP_Circulator parent는 /Script/BathhouseSim.BathWaterCirculatorFacilityActor다. 본체·footprint·Capacity=Circulation/100, Operation=100/1은 보존했다. CDO에는 Cube gauge needle, unit-scale QueryOnly lever volume Location=(19,-44,42)/extent=(16,6,16), pivot=(30,-44,30), Cube lever mesh, Stroke=1s/10점, CancelReturnSeconds=0, axis=(0,1,0), down angle=-60도, FacilityPlacement.Definition=DA_FacilityPlacement_Circulator를 설정했다.
- BP_UtilityShovel CDO LoadAppearances는 Coal=Cube, DryIce=Sphere+M_Glaze_Celadon_1로 구성했다. 전용 coal/dry-ice art가 없어 기존 assets로 구분한 임시 표현이다.
- BP_DryIceSupply는 /Script/BathhouseSim.UtilityFuelSupplyActor 자식이며 CDO FuelKind=DryIce, ScoopPoints=25, SupplyMesh=SM_Facility_sample, Scale=(0.5,0.5,0.5)다.
- 기존 DefaultMap instance는 class defaults를 완전히 상속하지 않았다. Boiler actor의 새 GaugeFacePlate는 instance에서 Mesh=None, 기본 transform/BlockAllDynamic/nav on이다. Cooler instance는 FacilityPlacement.Definition=None, needle/door Mesh=None, gauge 기본 축, intake extent=(32,32,32)다. Circulator instance는 Definition=None, gauge/lever Mesh=None, lever extent=(32,32,32), LeverLabor.CancelReturnSeconds=0.2다. 기존 Shovel instance LoadAppearances는 빈 맵이다. 따라서 위 CDO 저장만으로 기존 WP actor authoring이 갱신됐다고 보지 않는다.
- Cooler/Circulator instance FacilityPlacement.Definition에 set_properties 및 reset_properties를 호출했으나 둘 다 definition read-only 오류로 거부됐다. 현재 actor 경로는 /Game/Maps/DefaultMap.DefaultMap:PersistentLevel.BP_Cooler_C_UAID_F02F7433CA366F0403_1976197592 및 /Game/Maps/DefaultMap.DefaultMap:PersistentLevel.BP_Circulator_C_UAID_F02F7433CA366F0403_1986151593다.
- 새 DryIceSupply는 (450,-900,0)에 배치했다. PlacementZone 안이고 stain-spawn 영역 밖이며 기존 공급함/설비와 bounds가 겹치지 않는 위치다. 현재 세션에서 actor와 CDO 값은 읽었으나 저장되지 않았다. SceneTools.save_actor는 두 번 모두 Asset does not exist: /Game/__ExternalActors__/Maps/DefaultMap/B/E4/YYD7GB5C924IGGI6PAL42T로 실패했다. /Game/Maps/DefaultMap 개별 Save는 성공 응답을 반환했지만 map은 clean 상태였고 actor external package는 여전히 없었다. 이 배치의 디스크 영속 상태를 단정하지 않는다.
- PIE는 한 번 시작·종료됐고 종료 뒤 IsPIERunning=false였다. Data Validation 실행, PreviewDownPose/RestoreUpPose 호출, 실제 E 입력 주입 도구는 현재 MCP tool registry에 없었다. 직접 조작 수용은 수행하지 않았다.
- 저장 후 새 Editor 재로드를 위해 작업 소유 PID 25024를 정상 종료하려 했지만 숨김 창 MainWindowHandle=0이라 CloseMainWindow()가 false를 반환했다. Editor MCP에도 종료 tool이 없어 강제 종료하지 않았다. 현재 작업 Editor/MCP listener는 살아 있으며 깨끗한 새 프로세스 reload 검증은 미완료다.

## 직접 Editor 인계

1. 새 Editor에서 위 다섯 저장 Blueprint의 parent/CDO 값을 재로드 확인한다. 기존 WP actor component override도 Blueprint defaults와 맞춘다. Boiler의 새 GaugeFacePlate는 기존 actor에서 빠져 있고, Cooler/Circulator의 component Mesh/Volume/Gauge/Lever 값과 Shovel의 LoadAppearances가 stale하므로 actor별 reset/reinstance 상태를 먼저 확인한다. 기존 world transform은 보존한다.
2. Cooler/Circulator actor의 FacilityPlacement.Definition이 각각 DA_FacilityPlacement_Cooler, DA_FacilityPlacement_Circulator를 가리키는지 확인한다. 현재 MCP instance setter/reset은 read-only 오류로 막혔다. 변경이 필요하면 해당 WP external actor만 개별 저장하고 재로드해 확인한다.
3. DefaultMap에서 BP_DryIceSupply를 (450,-900,0)에 배치하고 해당 external actor package를 개별 저장한다. actor package 생성/저장 및 DefaultMap 재로드 뒤 class와 transform이 유지되는지 확인한다. 현재 MCP save_actor 실패를 재현하면 디스크에 저장된 것으로 처리하지 않는다.
4. Boiler/Cooler/Circulator 및 Shovel의 PIE 결과를 .md/PROMPT_UNREAL.md의 E 입력 시나리오로 직접 수용한다. Circulator에서 CancelReturnSeconds=0이 cancel 시 즉시 up/Idle로 돌아오는지 확인한다.
5. 저장된 Blueprint와 세 Placement Definition에 Data Validation을 실행하고, Circulator의 PreviewDownPose 후 RestoreUpPose를 Details 버튼으로 확인한다. 오류와 화면 결과를 기록한다.

## 재개 조건

- 기존 Level instance가 필요한 CDO/Definition 값을 상속하거나 해당 WP actor에 저장된 값으로 갱신된다.
- DryIceSupply external actor package가 생성·저장되고 새 Editor 재로드 뒤 배치가 남는다.
- Data Validation, lever preview 복원, 직접 PIE 입력·시각 수용이 완료된다.
- 재로드 뒤에만 .md/Unreal/UtilityLaborSystem.md, .md/Unreal/PlacementSystem.md, .md/Unreal/BathWaterSystem.md의 authoring 정본을 갱신한다.

### MCP 속성 설정·WP 저장 재시도 — 2026-09-26

- 현재 MCP registry에는 ActorTools.get_components와 ObjectTools.set_properties/reset_properties가 있다. 그러므로 모든 instance authoring이 API에 없는 것은 아니다.
- Boiler level instance의 GaugeFacePlate에서 staticMesh 설정은 반영됐지만 relativeLocation, relativeScale3D, bCanEverAffectNavigation, bGenerateOverlapEvents 설정은 성공 응답 뒤에도 읽기 값이 바뀌지 않았다. 시험 중 바꾼 staticMesh는 원래 None으로 되돌려 이 진단으로 남은 변경은 없다.
- Circulator level instance의 LeverLabor.cancelReturnSeconds=0 설정은 MCP가 해당 property를 설정할 수 없다고 명시적으로 거부했다.
- 기존 Circulator actor에 SceneTools.save_actor를 실행했으나 /Game/__ExternalActors__/Maps/DefaultMap/5/YZ/KRUXF2RNGDHA3WNR999GN3 패키지가 없다는 오류로 실패했다. 신규 DryIceSupply 외부 actor 저장 오류와 같은 종류의 blocker다.
- 따라서 현재 MCP toolset으로는 모든 stale WP component override를 instance에서 복구·저장할 수 있다고 확인되지 않았다. BP CDO authoring은 가능하며, actor component별 property는 개별 편집 가능 여부가 다르다. 이 단계는 수동 Editor authoring 또는 WP external actor package 생성·저장 지원이 필요하다.

# 컴퓨터 포커스 CMP-001~020 — Unreal MCP 재시도 인계 (2026-09-26)

## 현재 상태

- 코드 단계 승인을 받은 현재 작업은 .md/PROMPT_UNREAL.md의 컴퓨터 포커스 진입·이탈 authoring이다. 이번 시도에서는 Content, Config, Level asset을 수정하거나 저장하지 않았다.
- 대상 프로젝트에 실행 중인 Editor가 없어 UE 5.8 작업용 백그라운드 Editor PID 33284를 시작했다. Saved/Logs/BathhouseSim.log는 14.26.08 UTC의 LogTurnkeySupport VerifySdk 호출 뒤 진행 로그가 없었고 Intermediate/TurnkeyLog_0.log 및 TurnkeyReport_0.log도 생성되지 않았다. 8000 포트는 열리지 않았다.
- Unreal MCP list_toolsets transport 호출이 http://127.0.0.1:8000/mcp 연결 실패를 반환했다. 실제 Unreal tool 호출 성공/세션 초기화에 도달하지 못했다. Turnkey 정지 원인은 확인되지 않았다.
- 해당 Editor는 MainWindowHandle=0이라 정상 종료와 CloseMainWindow가 실패했다. 소유권을 확인한 PID와 그 실행으로 생성된 cmd/dotnet 자식만 종료했으며 현재 Editor와 8000 listener는 없다.

## 재개할 Editor 작업

MCP 연결이 실제 읽기 전용 조회까지 성공하면 .md/PROMPT_UNREAL.md allowlist 안에서 다음 authoring을 진행한다.

1. /Game/Bathhouse/Blueprints/Computer/BP_BathhouseComputer의 FocusExitPoint와 FocusExitSearchRadiusCm class default를 정하고 Editor-only Arrow 방향을 확인한다. 고정 exit 위치는 player capsule이 주변 구조물과 겹치지 않아야 한다.
2. 필요하면 exact Level actor /Game/Maps/DefaultMap.DefaultMap:PersistentLevel.BP_BathhouseComputer_C_UAID_F02F7433CA3690F802_2051456727에만 instance override를 authoring하고, 기존 ScreenWidget, ManagedBathPlacementZone과 blend 값 0.35/0.25초를 보존한다.
3. /Game/Input/Actions/IA_Cancel이 없으면 Digital bool Input Action으로 만들고, /Game/FirstPersonCharacter/BP_FirstPersonCharacter의 CancelAction과 /Game/Input/IMC_FirstPerson의 Escape mapping에 연결한다. /Game/FirstPersonCharacter/BP_FirstPersonController의 DefaultMappingContext가 IMC_FirstPerson인지 확인한다.
4. 대상 Blueprint만 Compile하고 allowlist asset만 개별 Save한다. Data Validation, 새 Editor session 재로드와 CMP-001~020 중 PIE 수용 항목은 현재 미실행 상태다.

## 재개 조건

- Turnkey VerifySdk가 반환하고 Editor startup log가 Slate/asset load까지 진행한다.
- MCP 서버가 127.0.0.1:8000에서 대상 PID 소유로 listen하며 Unreal read-only 조회가 성공한다.
- authoring 후 allowlist 개별 Save와 새 세션 재로드 결과를 기록한다. 작업 종료 시 에이전트가 시작한 백그라운드 Editor와 MCP 하위 프로세스를 종료한다.
### 연결 실패 원인 비교 — 2026-09-27

- 이전 성공 로그와 실패 시도의 실행 인자는 둘 다 `-NoSplash -log`이며 Turnkey VerifySdk 호출도 같은 명령이다. 이전 Editor는 Turnkey 호출(09:40:18 UTC) 후 11초 안에 MCP listener를 127.0.0.1:8000에 열고 세션 초기화 및 tool 목록 조회까지 진행했다.
- 이번 재시도는 Turnkey 호출(14:57:30 UTC) 이후 60초 넘게 로그가 갱신되지 않았고 `Intermediate/TurnkeyLog_0.log`, `TurnkeyReport_0.log`, MCP listener가 생성되지 않았다. 따라서 관찰된 연결 실패는 MCP plugin 통신보다 앞선 Editor/Turnkey startup 정지의 결과다.
- `-WaitForUATMutex` 또는 다른 Turnkey 단계 중 무엇이 대기 원인인지는 아직 증명되지 않았다. 작업용 Editor PID 27704와 이번 실행에서 시작된 UE 5.8 UAT 하위 프로세스를 종료했다. 현재 Editor/listener는 없고 에셋 변경도 없다. 이전 로그의 `resources/templates/list` 미지원 응답은 MCP 세션 초기화 후 나온 별도 프로토콜 요청 오류다.
# 컴퓨터 포커스 CMP-001~020 — authoring 및 재로드 재개 상태 (2026-09-27)

## 이전 인계 갱신

위의 2026-09-26 MCP 실패 기록은 당시 상태의 이력이다. 이후 최초 authoring 세션은 AutomationTool 로그 경로에 대한 sandbox UnauthorizedAccessException 때문에 시작하지 못했으나, 승인된 권한으로 재실행한 Turnkey는 ExitCode=0을 반환했고 MCP 연결·편집이 진행됐다. 따라서 이 작업의 최신 차단점은 Turnkey 권한이나 포트 개방 실패가 아니다.

## 저장된 MCP authoring 상태

- /Game/Bathhouse/Blueprints/Computer/BP_BathhouseComputer CDO의 FocusExitPoint.relativeLocation을 (1000, 0, -228.5714285714)로 설정했다. 기존 SearchRadius 100cm, editor-only Arrow의 +X 방향을 유지했다. 수치 bounds와 floor trace에서는 고정 발 위치 (-350, 0, 0)가 컴퓨터 mesh와 capsule을 겹치지 않는 후보로 계산됐다. 화면 캡처는 승인 검토에서 거부되어 시각 배치는 확인하지 못했다.
- /Game/Input/Actions/IA_Cancel이 없어서 ValueType Boolean인 action을 생성했다.
- /Game/FirstPersonCharacter/BP_FirstPersonCharacter의 CancelAction을 IA_Cancel로 설정했고 기존 InteractAction은 유지했다.
- /Game/Input/IMC_FirstPerson에 Escape → IA_Cancel을 추가했다. 기존 7개 mapping을 보존해 총 8개다.
- /Game/FirstPersonCharacter/BP_FirstPersonController의 DefaultMappingContext가 IMC_FirstPerson을 참조하는 것을 확인했다.
- BP_BathhouseComputer, BP_FirstPersonCharacter, BP_FirstPersonController를 warnings-as-errors Compile했다. Compile 결과 오류를 받지 않았고 LogBlueprint 조회에서 새 항목이 없었다.
- allowlist 네 asset인 BP_BathhouseComputer, BP_FirstPersonCharacter, IMC_FirstPerson, IA_Cancel을 각각 Save해 성공 응답과 dirty=false를 확인했다. Controller는 변경하지 않아 저장하지 않았다.
- DefaultMap exact computer actor 저장은 SceneTools.save_actor가 external actor asset registry path를 찾지 못해 실패했다. 대응하는 .uasset 파일은 디스크에 있었지만 MCP registry에서는 external package가 확인되지 않았다. Map/actor를 저장하지 않았다. Blueprint Compile 뒤 같은 세션의 actor는 CDO FocusExitPoint를 상속했지만 새 프로세스에서 재로드된 Level 인스턴스 결과는 아직 모른다.

## 새 프로세스 재로드 시도와 연결 원인

- 작업용 UE 5.8 Editor PID 4284는 Turnkey ExitCode=0, Engine initialized, 127.0.0.1:8000 listener 시작 및 MCP client 연결까지 진행했다.
- 로그에는 MCP 메타 도구 검색 가능 표시가 최대 3개뿐이고 Python init_unreal.py 실행이나 Editor 작업 toolset 등록이 없었다. DDC maintenance 종료 후 로그가 더 진행하지 않았다.
- 해당 상태에서 list_toolsets 단일 호출은 시간 초과했다. 포트와 client TCP 연결은 있었으므로 이는 이전의 포트 연결 실패와 다른 증상이다. 현재 근거만으로 초기화가 멈춘 구체 원인은 확정할 수 없다.
- 동일 작업 재로드 확인의 두 번째 시도도 성공하지 않아 추가 재시도는 중단했다. CloseMainWindow는 false였고 이 작업이 시작한 정확한 PID 4284를 종료했다. 종료 확인 시 UnrealEditor 프로세스와 8000 listener가 없었다.

## 남은 Editor 수용

1. 정상 초기화되어 Editor toolset이 완전히 등록된 새 MCP 세션에서 저장된 네 asset을 읽기 전용 재로드 확인한다. 특히 BP_BathhouseComputer CDO와 DefaultMap exact actor의 FocusExitPoint 상속을 구분해 기록한다. 새 세션에 작업용 Editor를 다시 시작했다면 마무리 후 종료한다.
2. 현재 MCP registry에는 Data Validation 호출 tool이 없어 assets의 Data Validation을 실행하지 못했다. Editor에서 allowlist assets의 Validate Assets 결과와 오류를 기록한다.
3. MCP는 PIE 시작/종료를 제공하지만 게임 입력을 주입할 수 없다. 화면·키보드·마우스가 필요한 수용은 별도 Editor 플레이에서 .md/PROMPT_UNREAL.md의 PIE 수용 절차를 수행한다. E/ESC 이탈과 진입 E release, 버튼/slider drag 중 LMB release, cursor 중앙 복귀, 고정 exit 위치/방향, blocker 및 forced 경로, 바닥 낙하 조건, placement/lever/recovery ESC 비간섭과 재진입을 기록한다.
4. Editor screenshot capture 호출은 automatic approval review에서 프로젝트 UI/asset 정보를 MCP로 전송할 수 있다는 사유로 거부됐다. 시각 기준은 Editor에서 직접 확인한다.

재로드, Data Validation, 직접 PIE 및 시각 수용이 끝나기 전까지 완료로 판정하지 않는다. 다음 재개는 우선 새 Editor의 MCP Python/toolset 초기화 및 read-only 호출 성공을 확인한 뒤 진행한다.
