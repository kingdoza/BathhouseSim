# Unreal 읽기 전용 사전 조사 — 욕탕 물 순환·가열·냉각과 컴퓨터 제어

## 1. 조사 상태와 결론 요약

- 조사일: 2026-09-22 KST. 대상: `C:/UnrealProjects/BathhouseSim/BathhouseSim.uproject`, `/Game/Maps/DefaultMap`.
- 상태: **완료**. 요구 항목을 `MCP 확인`, `현재 없음`, `MCP 확인 불가`로 분류했다.
- UE 5.8.2 Editor의 MCP를 사용해 Asset Registry, Blueprint/CDO, Level actor, Object property, dependency/referencer, dirty와 PIE 상태를 읽었다.
- 순환기·쿨러용 기존 asset은 없고, 보일러는 placeable 설비가 아닌 mesh/material 및 Level `StaticMeshActor`만 있다.
- 현재 Bath CDO·인스턴스와 관련 설정에는 이번 기능의 수온·오염도·순환/열 용량·변화율 직렬화 값이 없다.
- 실제 asset과 `.md/Unreal` 정본 사이에 grid, Bath footprint, 물 표현/control 값과 Level override 불일치가 확인됐다. 수정하지 않았다.

## 2. Engine·Editor·MCP capability와 dirty 기준선

| 항목 | 확인 결과 |
|---|---|
| Engine | `.uproject` `EngineAssociation=5.8`; startup log `5.8.2-56702186+++UE5+Release-5.8` |
| Editor | 이 조사가 시작한 background `UnrealEditor` PID 10448 한 개, project `C:/UnrealProjects/BathhouseSim/BathhouseSim.uproject` |
| MCP | `127.0.0.1:8000/mcp` 응답 성공. Actor/Asset/Blueprint/Object/Scene/StaticMesh/Programmatic 등 19 toolset |
| 시작 PIE | `false` |
| 시작 dirty | `/Game/Maps/DefaultMap`, `BP_Bath`, `BP_BathhouseComputer`, `BP_FacilityPlacementZone` 모두 `false` |
| 시작 Git | 사용자 소유 `M .md/AGENT_WORKFLOW.md`, `M .md/QNA_FEATURE_SPEC.md`; 재개 시 이전 패스의 허용 결과물 `M .md/REPORT_UNREAL_DISCOVERY.md`도 존재 |
| 미지원 | StateTree 내부 state/task/transition/binding, WidgetTree/BindWidget hierarchy, component attachment parent, primitive collision profile 전용 조회 |

Startup log에서 StateTree `/Game/Bathhouse/AI/ST_CustomerRoutine` compile 성공과 MapCheck 0 error/0 warning을 확인했다. Editor 로드가 수행한 compile이며 조사에서 Compile tool을 호출하지 않았다. `AM_Customer_Bath_Loop`, `AM_Customer_Action_Once` segment 길이 불일치 경고가 있다. 조사 도중의 `LogScript`/`LogBlueprint` 경고는 잘못 지정한 읽기 property와 graph DSL 조회가 낸 도구 진단이며 asset compile 결과로 해석하지 않는다.

## 3. 컴퓨터 Blueprint·Widget·Input·Level 인스턴스

- Native-parent tag 검색 결과 `/Script/BathhouseSim.BathhouseComputerActor` 파생 Blueprint는 `/Game/Bathhouse/Blueprints/Computer/BP_BathhouseComputer` 하나다.
- CDO component inventory는 `ComputerMesh`, `ScreenWidget`, `FocusCamera`다. MCP는 세 component의 attachment parent를 노출하지 않는다.
- `ComputerMesh`: `/Engine/BasicShapes/Cube.Cube`, relative L/R/S `(0,0,0)/(0,0,0)/(0.12,1.2,0.7)`, Navigation `true`.
- `ScreenWidget`: `/Game/Bathhouse/UI/WBP_ComputerSampleScreen.WBP_ComputerSampleScreen_C`, `World`, Draw `(1024,576)`, pivot `(0.5,0.5)`, `Plane`, `bReceiveHardwareInput=false`, `bWindowFocusable=true`, Tick `Enabled`, relative L/R/S `(51.6667,0,0)/(0,0,0)/(0.833333,0.083333,0.142857)`.
- `FocusCamera`: relative L/R/S `(1333.3333,0,14.2857)/(0,180,0)/(1,1,1)`, FOV `70`; blend in/out `0.35/0.25s`.
- Level actor: `/Game/Maps/DefaultMap.DefaultMap:PersistentLevel.BP_BathhouseComputer_C_UAID_F02F7433CA3690F802_2051456727`, actor transform L/R/S `(-470,0,160)/(0,0,0)/(0.12,1.2,0.7)`. 기능 property의 별도 instance 차이는 검출되지 않았다.
- `/Script/BathhouseSim.ComputerSampleScreenWidget` 파생 Widget Blueprint는 `/Game/Bathhouse/UI/WBP_ComputerSampleScreen` 하나다. dirty `false`.
- Widget graph는 `EventGraph`, `Touch Input Check`, `Stick Input`; `EventGraph` entry point는 0개다. 뒤의 두 함수 graph에는 FunctionEntry가 있으며 모바일 thumbstick 계열 변수도 존재한다. 현재 샘플 버튼용 Blueprint Event Graph 연결은 없다.
- MCP Object/Blueprint schema는 WidgetTree와 native `TestButton`/`ClickResultText` BindWidget object를 노출하지 않아 실제 hierarchy·이름·타입은 미확정이다.
- `/Game/Input/Actions/IA_ComputerClick`은 없음. `/Game/Input/Actions/IA_PrimaryUse`는 Boolean InputAction이며 `/Game/Input/IMC_FirstPerson`에서 `LeftMouseButton`에 매핑됐다.
- `/Game/FirstPersonCharacter/BP_FirstPersonCharacter` CDO는 `PrimaryUseAction=/Game/Input/Actions/IA_PrimaryUse`, `ComputerClickAction=None`이다. Controller의 `DefaultMappingContext=/Game/Input/IMC_FirstPerson`이다.
- ScreenWidget에 class가 고정돼 있어 같은 Actor가 만든 instance를 보유할 authoring 연결은 있다. focus-out 재진입 시 실제 instance/클릭 상태 유지 여부는 PIE 금지로 검증하지 않았다.

## 4. BP_Bath CDO·슬롯·footprint·Level 인스턴스

- `/Game/Bathhouse/Blueprints/Facility/BP_Bath` parent는 `/Script/BathhouseSim.BathhouseBathFacilityActor`다.
- CDO inventory 17개: `SceneRoot`, `PackagePhysicalRoot`, `FacilityVisual`, `PlacementFootprint`, `FacilityPlacement`, `BathWaterState`, `FacilitySlotA/B/C`, `FillValveControl`, `DrainLeverControl`, `FillFlowNiagara`, `WaterPresentationRoot`, `WaterSurfaceMover`, `WaterSurfaceMesh`, `WaterLevelEmptyPoint`, `WaterLevelFullPoint`. Attachment parent는 MCP 미노출이다.
- `PlacementFootprint`: extent `(150,120,38)`, relative Z `38`, scale 1, Navigation `false`; full X/Y `300x240cm`은 10cm와 실제 설정 20cm 모두의 정수배다.
- `FacilityPlacement`: `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Bath`, mode `Placed`. `FacilityVisual`: `/Game/Bathhouse/Meshes/Bath/SM_Bath_old`, scale 3, Navigation `true`.
- 슬롯은 정확히 3개이며 모두 enabled/Available, Facing yaw `180`, ApproachOffset `(150,0,-72.5)`이다. local location은 A `(0,-60,2.5)`, B `(0,0,2.5)`, C `(0,60,2.5)`다.
- `BathWaterState`: fill `6.666667%/s`, drain `10%/s`, `Empty`, normalized `0`.
- Fill: Cube, axis `(0,-1,0)`, `180°`, `0.5s`, L/S `(-55,105,68)/(0.3,0.05,0.3)`. Drain: Cube, axis `(0,1,0)`, `90°`, `0.5s`, L/S `(55,105,65)/(0.06,0.06,0.3)`.
- Water mesh: `/Engine/BasicShapes/Plane.Plane`, L/S `(0,-14,0)/(2.5,1.7,1)`; markers Empty/Full Z `3/70`. Niagara: `/Game/Niagaras/NS_WaterStream`, L `(0,-71,85)`, AutoActivate `false`.
- 인스턴스 1: `/Game/Maps/DefaultMap.DefaultMap:PersistentLevel.BP_Bath_C_UAID_F02F7433CA3615F402_1441881862`, L/R/S `(1842,0,0)/(0,0,0)/(1,1,1)`; 조사 property는 CDO와 같다.
- 인스턴스 2: `/Game/Maps/DefaultMap.DefaultMap:PersistentLevel.BP_Bath_C_UAID_F02F7433CA367BF402_1618061807`, L/R/S `(1750,-638,0)/(0,0,0)/(1,1,1)`; `FacilityVisual.StaticMesh=None`, 세 슬롯 `ApproachOffset.Z=0`의 instance 차이가 있다.
- Bath CDO와 두 인스턴스에는 temperature/target temperature/pollution/contamination/circulation 계열 property가 없다.

## 5. 욕탕용 PlacementZone과 UI 좌표 기준

- `/Game/Bathhouse/Blueprints/Placement/BP_FacilityPlacementZone` parent는 `/Script/BathhouseSim.FacilityPlacementZoneActor`; inventory는 `ZoneBounds`, `PlacementFloor`, `GridVisual`이다.
- DefaultMap의 Zone은 정확히 하나: `/Game/Maps/DefaultMap.DefaultMap:PersistentLevel.BP_FacilityPlacementZone_C_UAID_F02F7433CA36D1FF02_1155169559`.
- Actor L/R/S `(600,-100,0)/(0,0,0)/(1,1,1)`; `ZoneBounds` extent `(1400,900,10)`; floor identity, world Z `0`; `AllowedFacilityTags=Facility.Placeable`.
- `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Bath`도 `Facility.Placeable`이므로 이 Zone에서 허용된다.
- 회전이 없으므로 영역은 world X `[-800,2000]`, Y `[-1000,800]`, 크기 `2800x1800cm`; UI의 고정 `+X=위`, `+Y=오른쪽` 변환 근거로 사용할 수 있다.
- GridVisual은 Plane, instance L/S `(0,0,0.5)/(28,18,1)`; CDO `GridLineThicknessCm=1`, `GridZOffsetCm=0.5`, `MajorGridIntervalCells=5`다.

## 6. 순환기·보일러·쿨러 기존 asset 후보

- `circulator/circulation/filter/filtration/pump`, `heater/heating`, `cooler/chiller/cooling` 이름 검색 결과 `/Game/Bathhouse` 아래 후보는 0개다.
- boiler 검색은 `/Game/Bathhouse/Meshes/boiler`(StaticMesh)와 `/Game/Bathhouse/Meshes/M_Boiler`(Material)만 반환했다. Blueprint, SkeletalMesh, DataAsset, Placement Definition은 없다.
- Mesh bounds min `(-28.3347,-20.4055,0)`, max `(32.4628,22.4314,102.717)`cm; material slot `Material -> /Game/Bathhouse/Meshes/M_Boiler`.
- Mesh registry: `CollisionComplexity=UseSimpleAndComplex`, `CollisionPrims=1`, `DefaultCollision=BlockAll`, `SectionsWithCollision=1`, NavCollision object 존재, Nanite `true`.
- referencer는 external actor 하나이며 실제 actor는 `/Game/Maps/DefaultMap.DefaultMap:PersistentLevel.StaticMeshActor_UAID_F02F7433CA36460103_1976889207`, L `(1528,-532,0)`, scale 1, Navigation `true`다. 현재 placeable 설비 연결은 아니다.

## 7. 기존 Placement Definition과 현재 serialized 입력

공통 property 종류는 `StableId`, `FacilityTags`, `PlacedFacilityClass`, `RecoveryItemClass`, `RecoveryItemMesh`, `LockerSlotCount`다. 9개 모두 tag `Facility.Placeable`, `RecoveryItemMesh=None`이다.

| Definition | Placed class | Recovery class | Slots |
|---|---|---|---:|
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Bath` | `/Game/Bathhouse/Blueprints/Facility/BP_Bath.BP_Bath_C` | `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem.BP_PlaceableFacilityItem_C` | 0 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Shower` | `/Game/Bathhouse/Blueprints/Facility/BP_Shower.BP_Shower_C` | `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem.BP_PlaceableFacilityItem_C` | 0 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_1` | `/Game/Bathhouse/Blueprints/Facility/BP_ClothesLocker.BP_ClothesLocker_C` | `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem.BP_PlaceableFacilityItem_C` | 1 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_4` | `/Game/Bathhouse/Blueprints/Facility/BP_ClothesLocker_4.BP_ClothesLocker_4_C` | `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem.BP_PlaceableFacilityItem_C` | 4 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_8` | `/Game/Bathhouse/Blueprints/Facility/BP_ClothesLocker_8.BP_ClothesLocker_8_C` | `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem.BP_PlaceableFacilityItem_C` | 8 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Washer` | `/Game/Bathhouse/Blueprints/Towel/BP_Washer.BP_Washer_C` | `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem.BP_PlaceableFacilityItem_C` | 0 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Dryer` | `/Game/Bathhouse/Blueprints/Towel/BP_Dryer.BP_Dryer_C` | `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem.BP_PlaceableFacilityItem_C` | 0 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_CleanTowelStack` | `None` | `None` | 0 |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_UsedTowelBin` | `None` | `None` | 0 |

- 현재 `FacilityPlacementSettings`: grid `20cm`, rotation `15°`, recovery hold `1s`, drop Z `100cm`, placement/recovery trace `500/300cm`, held L/R/S `(10,50,-60)/(0,0,0)/(1,1,1)`, preview materials `/Game/Material/MI_Preview_Valid`, `/Game/Material/MI_Preview_Invalid`.
- 표본 7개 placeable CDO는 `PlacementFootprint`와 Definition binding을 가진다. Bath/Shower/Locker의 body `FacilityVisual`은 Navigation `true`, footprint는 Navigation `false`와 overlap `false`; MCP는 component collision profile/response를 노출하지 않았다.

## 8. Customer 욕탕 이용과 동시 입욕자 수 관련 사실

- `/Game/Bathhouse/AI/ST_CustomerRoutine`은 `StateTree`, schema `/Script/GameplayStateTreeModule.StateTreeAIComponentSchema`; `/Game/Bathhouse/Blueprints/Customer/BP_BathhouseCustomerAIController`의 `CustomerStateTree.stateTreeRef`에 연결됐다.
- StateTree dependency에는 `/Game/Bathhouse/Animations/Customer/AM_Customer_Bath_Loop`, `/Game/Bathhouse/Animations/Customer/AM_Customer_Action_Once`, `/Game/Bathhouse/Blueprints/Customer/BP_BathhouseCustomer`가 있다.
- 현재 MCP에는 StateTree 내부 조회 toolset이 없어 예약·이동·입욕·퇴장 state/task/binding은 미확정이다.
- Editor에서 확인 가능한 Bath 측 계약은 독립된 3개 `FacilitySlot`과 각 `SlotState`/`OnSlotStateChanged`다. CDO와 두 Level 인스턴스는 조사 시 모두 `Available`이다.
- 슬롯 schema에는 serialized occupant/customer reference가 없다. 실제 입욕 중인 손님 수와 Customer session의 활성 입욕 segment는 PIE 없이 관찰하지 않았다.

## 9. 이번 기능 값의 기존 존재/부재

Bath CDO/인스턴스 property schema, `/Script/BathhouseSim.BathWaterSettings`, `/Script/BathhouseSim.FacilityPlacementSettings`, Asset Registry 이름과 project class 검색을 대조했다. `TowelCirculationSubsystem`은 수건 시스템이라 이번 물 순환 값으로 대응하지 않았다.

| 계약값 | 현재 상태 |
|---|---|
| 전역 실온 20°C | 미구현/미작성 |
| 목표 수온 10~50°C, 1°C 간격 | 미구현/미작성 |
| 오염도 0~100% | 미구현/미작성 |
| 순환기/보일러/쿨러 기본 용량 각 100 | 세 항목 모두 미구현/미작성 |
| 욕탕 최대 순환 요구량 100 | 미구현/미작성 |
| 가열·냉각 요구량 5포인트/°C | 미구현/미작성 |
| 순환 100% 오염 감소 1%p/s | 미구현/미작성 |
| 입욕자 1명 오염 증가 0.1%p/s | 미구현/미작성 |
| 순환 100% 목표 수온 변화 0.5°C/s | 미구현/미작성 |
| 자연 실온 복귀 0.05°C/s | 미구현/미작성 |

기존 `/Script/BathhouseSim.BathWaterSettings`에는 별개 값 `CustomerUsableThresholdPercent=80`만 있다.

## 10. Unreal 정본과 실제 asset 불일치

- `.md/Unreal/PlacementSystem.md:5`의 grid `10cm` ↔ 실제 `FacilityPlacementSettings.GridSizeCm=20`.
- 같은 문서 :40의 Bath parent `BathhouseFacilityActor`, extent X `145` ↔ 실제 `BathhouseBathFacilityActor`, extent X `150`.
- 같은 문서 :59의 `MajorGridIntervalCells=10` ↔ 실제 `5`.
- `.md/Unreal/BathWaterSystem.md:32-39` ↔ 실제 Water mesh `Engine Plane`, L/S `(0,-14,0)/(2.5,1.7,1)`, marker Z `3/70`, Fill axis/angle/scale `(0,-1,0)/180/(0.3,0.05,0.3)`, Drain angle/scale `90/(0.06,0.06,0.3)`, Niagara `/Game/Niagaras/NS_WaterStream` at `(0,-71,85)`.
- `.md/Unreal/BathWaterSystem.md:9`의 Level 별도 override 없음 ↔ 두 번째 Bath의 `FacilityVisual=None`, 세 슬롯 ApproachOffset.Z `0`.

## 11. MCP로 확인하지 못한 사실과 이유

- WidgetTree와 `TestButton`/`ClickResultText` 실제 BindWidget 이름·타입: 현재 Object/Blueprint schema에 미노출.
- component의 attachment parent와 exact collision profile/response: current toolset 미노출.
- StateTree 내부 state/task/transition/binding: StateTree toolset 없음.
- focus 재진입 Widget instance 유지, 실제 입욕자 수, 동적 오염/온도: PIE 금지 및 runtime 사실.
- 보일러 mesh의 기능 적합성·화면상 품질: 이름과 저장 metadata만으로 판정하지 않음. Computer Use로 우회하지 않았다.

## 12. 기능 명세가 반영해야 할 확인 사실

- 컴퓨터 화면은 기존 `BP_BathhouseComputer -> WBP_ComputerSampleScreen` 연결을 가지지만 별도 `IA_ComputerClick`과 Pawn `ComputerClickAction` assignment는 없다. 현재 LMB는 `IA_PrimaryUse`다.
- UI 대상 Zone은 DefaultMap의 단일 Zone이며 world X/Y 범위와 방향은 5절 실제값을 사용한다. UI에는 그 안의 Bath만 포함한다는 계약과 구분한다.
- Bath는 3개의 독립 슬롯을 갖지만 두 번째 Level 인스턴스의 시각 mesh와 approach Z override는 현재 결함으로 분리한다.
- 기존 boiler mesh/material/StaticMeshActor는 후보 사실일 뿐 placeable 보일러로 확정하지 않는다. 순환기·쿨러 및 세 설비 Definition은 현재 없다.
- 9절의 기능 수치는 모두 새 계약값이며 기존 serialized default로 표현하지 않는다. 기존 placement 입력 종류와 9개 Definition 현황은 7절을 사용한다.
- 문서값보다 10절의 MCP 실제값을 현재 asset 사실로 우선하되, 불일치를 목표 요구사항으로 승격하지 않는다.

## 13. 종료 검증

- 종료 직전 PIE `false`; Map, 조사한 Blueprint/Widget/Input/StateTree/mesh/material 및 Placement Definition 9개 모두 dirty `false`; open asset 0개.
- 조사에서 asset/Level/Project Settings 수정, Save, explicit Compile, PIE, Computer Use를 수행하지 않았다.
- background Editor는 main window가 없어 graceful close 요청이 불가능했다. clean/PIE false 확인 뒤 agent-owned PID 10448만 강제 종료했고 PID 소멸을 확인했다.
- 종료 Git 차이는 기존 사용자 변경 2개와 허용 결과물 `.md/REPORT_UNREAL_DISCOVERY.md`뿐이며 Content, Config, Source, Architecture/Unreal 정본, QNA를 변경하지 않았다.
