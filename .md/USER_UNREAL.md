# 사용자 Unreal 후속 작업

# 보일러 노동 가동 수직 구현 — MCP 미지원 작업

## 현재 확인 상태

`/Game/Bathhouse/Blueprints/Facility/BP_Boiler`는 `/Script/BathhouseSim.BathWaterBoilerFacilityActor`로 reparent했고 `FacilityPlacement.Definition=/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler`와 임시 Cube 투입구·계기·바늘을 저장·재로드했다. `/Game/Bathhouse/Blueprints/Utility/BP_UtilityShovel`과 `BP_CoalSupply`도 임시 Cube/`SM_Facility_sample`로 생성·컴파일·저장·재로드했다. 새 Editor에서 기존 보일러 instance는 새 component 값을 상속했고 기본 PIE에서 필수 투입구 누락 경고가 재발하지 않았다.

`DefaultMap`의 신규 석탄 공급함·삽·전용 거치대는 **디스크에 저장되지 않았다.** 이전 MCP 세션의 개별 `save_actor`는 신규/기존 World Partition 외부 액터 모두에 `Asset does not exist: /Game/__ExternalActors__/...`를 반환했다. 현재 사용자가 열어 둔 Editor에는 아래 세 actor를 다시 배치하고 exact 삽 참조를 연결했으나, Editor를 닫으면 사라질 수 있다. MCP 저장 실패를 반복하지 않았다.

## 1. DefaultMap 액터 배치·외부 액터 저장

현재 열린 `/Game/Maps/DefaultMap`에는 다음 세 Blueprint instance가 **미저장 상태로 배치되어 있고 선택되어 있다.** 재생성하지 말고 위치가 괜찮은지 확인한다. 바닥 trace는 두 위치 모두 Z=0이었다.

| Actor | Blueprint | 제안 world Location |
|---|---|---|
| `CoalSupply` | `/Game/Bathhouse/Blueprints/Utility/BP_CoalSupply` | `(550,-850,0)` |
| `UtilityShovel` | `/Game/Bathhouse/Blueprints/Utility/BP_UtilityShovel` | `(425,-800,45)` |
| `ShovelSlot` | `/Game/Bathhouse/Blueprints/Interaction/BP_PhysicalCarryFixedSlot` | `(425,-800,45)` |

`ShovelSlot.AssignedItem`은 **그 레벨의 `UtilityShovel` 인스턴스**를 가리키고, `bStartOccupied=true`, `SlotDisplayName=삽 거치대`다. `ItemAnchor`와 삽의 시작 world transform은 둘 다 `(425,-800,45)`/회전 0이다. 현재 열린 Editor에서 `DefaultMap`과 해당 세 World Partition 외부 액터만 저장한 뒤, 다시 열어 세 actor와 exact 참조가 유지되는지 확인한다. 이번 MCP는 외부 액터의 디스크 저장을 완료하지 못했다. 저장 전 Editor를 종료하지 않는다.

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
