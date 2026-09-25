# Unreal MCP 읽기 전용 사전 조사 — 설비 노동 가동

## 범위와 판정 기준

- 2026-09-23 KST, `C:/UnrealProjects/BathhouseSim/BathhouseSim.uproject`, 열린 `/Game/Maps/DefaultMap` 대상. 이번 문서는 동작명세용 **현재 상태 조사**이며 설계·구현·실행 검증이 아니다.
- `.md/QNA_FEATURE_SPEC.md` Q29~Q47을 이번 확장 사용자 선택의 정본으로 읽었다. `.md/PROMPT_ARCHITECTURE.md`의 항시 가동 계약은 기존 구현을 설명할 때만 사용했다.
- `AGENTS.md`, `.md/AGENT_WORKFLOW.md`, `.md/AGENT_UNREAL_MCP.md`, `.md/AGENT_FEATURE_SPEC.md`, `.md/0_ARCHITECTURE.md`, 기존 보고서, `.md/Architecture/BathWaterOperationsSystem.md`, `BathWaterManagementUISystem.md`, `PhysicalCarrySystem.md`, `PlacementSystem.md`, `InteractionSystem.md`, `ComputerSystem.md`, `UISystem.md`, `.md/Unreal/0_UNREAL.md`, `BathWaterSystem.md`, `PlacementSystem.md`, `InteractionUISystem.md`를 조사 기준으로 삼았다.
- 실제 Source와 현재 MCP 응답을 다시 확인했다. 이전 보고서의 2026-09-22 값(설비 Blueprint 부재 등)은 현재 사실로 재사용하지 않았다. 문서값은 serialized asset이나 Source와 다르면 아래에 별도로 표시한다.
- 프로젝트 `EngineAssociation=5.8`, 사용자 BathhouseSim Editor PID 22060의 startup log는 UE `5.8.3-58210709+++UE5+Release-5.8`, MCP `127.0.0.1:8000/mcp` 응답 확인. Actor/Asset/Blueprint/Object/Scene/StaticMesh/Programmatic 등 19 toolset. 사전 확인 PIE=false, Git clean. `DefaultMap`과 조회한 설비 BP·컴퓨터 BP·관리 WBP는 MCP dirty=false. Startup log의 `ST_CustomerRoutine` 자동 compile 성공은 Editor 로드 결과이며 조사에서 Compile을 호출하지 않았다.

## 1. 설비 Blueprint, 용량, 배치

| Serialized asset | Native parent / CDO `Capacity` | CDO `VisualMesh` / `PlacementFootprint` | `DefaultMap` 인스턴스 |
|---|---|---|---|
| `/Game/Bathhouse/Blueprints/Facility/BP_Boiler` | `/Script/BathhouseSim.BathWaterUtilityFacilityActor`; Heating 100 | `/Game/Bathhouse/Meshes/SM_Facility_sample`, scale (1,0.6,1.2); extent (50,30,60), Z 60 | 1개, (299,-885,0), Heating 100 |
| `/Game/Bathhouse/Blueprints/Facility/BP_Cooler` | 동일; Cooling 100 | 동일 mesh·scale·footprint | 1개, (125,-885,0), Cooling 100 |
| `/Game/Bathhouse/Blueprints/Facility/BP_Circulator` | 동일; Circulation 100 | 동일 mesh, scale (1.2,0.8,1.2); extent (60,40,60), Z 60 | 1개, (-179,-908,0), Circulation 100 |

- MCP `BlueprintTools.get_parent/get_default_object`, `ActorTools.get_components`, `ObjectTools.get_properties`, `SceneTools.find_actors` 확인. 세 CDO 모두 Blueprint 자체 변수 없음; graph는 `UserConstructionScript`와 `EventGraph`만 열거됐다. 실제 component는 native `PackagePhysicalRoot → SceneRoot → VisualMesh/PlacementFootprint`, 비공간 `FacilityPlacement`·`Capacity` 6개다. 계기·바늘·레버·투입구 component는 없다. Hierarchy의 parent 관계는 Source의 `SetupAttachment`로 확인했으며 MCP 자체 attachment 조회는 불가했다.
- 세 Level 인스턴스의 `CapacityKind/CapacityPoints`와 `VisualMesh` 값은 CDO와 같아 **이 조회 범위에는 값 차이 없음**. 이것만으로 serialized override 플래그 자체가 없다고 단정하지 않는다. `FacilityPlacement.Definition`은 세 CDO와 세 Level 인스턴스에서 모두 `None`, `Mode=Placed`다. `.md/Unreal/BathWaterSystem.md`·`PlacementSystem.md`의 관련 Definition 연결/배치 활성 설명과 실제 조회가 다르다.
- 대응 Definition asset `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_{Boiler,Cooler,Circulator}`은 존재하고 각각 대응 `PlacedFacilityClass`, 공통 `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem` 회수 class를 가리킨다. `RecoveryItemMesh=None`이다. Definition asset **존재**와 BP component에 **연결됨**은 별개다. `UFacilityPlacementComponent::IsOperational`은 Definition이 없으면 실패하므로, 현재 preplaced 설비의 정상 Q 회수 가능성에는 이 연결 누락이 직접 걸린다. PIE로 실행 판정하지는 않았다.
- Source 근거: `Source/BathhouseSim/Private/Facility/BathWaterUtilityFacilityActor.cpp`는 설치 상태 BeginPlay·staged 설치 시 `Capacity`를 provider로 등록하고, 회수 stage/EndPlay에 해제한다. `Source/BathhouseSim/Private/Facility/BathWaterOperationsSubsystem.cpp`의 `CalculateTotalCapacity`는 등록된 provider의 `GetCapacityPoints()` 전부를 합산한다. `Source/BathhouseSim/Private/Facility/BathWaterUtilityCapacityComponent.cpp`는 정격의 유효성/복원만 처리하고 Tick하지 않는다. 현재 설치용량과 가동용량의 별도 원장·가동수치·자연 감소·석탄/드라이아이스 상태는 없다. 현재 provider는 등록되면 사용률과 무관하게 정격 전부를 제공한다.
- `Source/BathhouseSim/Private/Facility/BathWaterOperationsSubsystem.cpp`의 설정 요청은 현재 단일 `CalculateTotalCapacity`를 상한으로 예약하고, `CanRemoveProvider`는 제거 후 그 총량이 예약량보다 작은지 검사한다. `Source/BathhouseSim/Private/Facility/BathWaterConditionComponent.cpp`는 종류별 deficit이면 정화/능동 수온 조절을 중단하고 설정을 보존하며, 용량 회복 시 별도 입력 없이 다시 효과를 계산한다. 순환 부족은 정화와 능동 수온 조절 모두를 막는다. **정지·보존·재개 gate는 재사용 가능하나**, 그 판정이 현재 설치 정격 총량 하나에 묶여 있다.
- `Source/BathhouseSim/Public/Facility/BathWaterOperationsTypes.h`의 `FBathWaterCapacitySnapshot`은 kind/used/total/deficit/revision만 갖는다. Q32의 예약량·가동용량·설치용량 3종 표시나 0/양수 경계는 현재 표현되지 않는다. Q31·Q39·Q40의 0 신규 시작, 양수 시 정격 전부, 초당 1 감소도 현재 미지원이다.

## 2. 삽·재료·공급과 재사용 후보

- MCP Asset Registry를 `/Game` 전체에서 `Shovel, Scoop, Coal, DryIce, Dry_Ice, Supply, Hopper, Fuel, Intake, Chute`로 재검색했으나 해당 이름의 asset은 0개였다. `DefaultMap`에서도 `Supply`, `Shovel` 이름의 Actor는 0개. 이는 **검색 범위·이름에 해당하는 후보가 없음**이지, 임의 이름의 mesh를 시각적으로 검사해 용도를 배제한 결론은 아니다.
- `/Game/Bathhouse/Meshes/boiler`은 실제 StaticMesh, `/Game/Bathhouse/Meshes/M_Boiler`은 그 의존 Material. Mesh local bounds는 min (-28.335,-20.406,0), max (32.463,22.431,102.717)cm, 슬롯 `Material` 하나다. 현재 `BP_Boiler.VisualMesh`는 이 mesh가 아니라 공통 `SM_Facility_sample`이다. `DefaultMap`의 별도 `StaticMeshActor_UAID_F02F7433CA36460103_1976889207`이 (1528,-532,0)에서 `boiler`를 사용한다. 따라서 보일러 **외형 후보**는 확인됐지만 투입구·독립 바늘·내부 연료 표시·상호작용 가능 구조라는 근거는 없다.
- `/Game/Bathhouse/Blueprints/Cleaning/BP_WetMop`, `/Game/Bathhouse/Blueprints/Combat/BP_MonkeyWrench`, `/Game/Bathhouse/Blueprints/Towel/BP_TowelBasket`, `/Game/Bathhouse/Blueprints/Interaction/BP_PhysicalCarryFixedSlot`은 실제 asset이다. `Source/BathhouseSim/Private/Interaction/PlayerCarryComponent.cpp`와 `Source/BathhouseSim/Private/Interaction/PlayerEquipmentUseComponent.cpp`에 pickup/held, G free drop, 지정 슬롯 E 거치/회수, 보유 도구 LMB 사용 경로가 있다. 삽 자체 및 한 종류 한 회분 내용물·재료별 외형 상태는 확인되지 않아 기존 도구가 삽 요구를 이미 만족한다고 볼 수 없다.
- `Source/BathhouseSim/Private/Interaction/PlayerEquipmentUseComponent.cpp`의 held-use context에는 camera 방향과 현재 focus hit가 전달된다. 사용 시작은 query 후 `BeginEquipmentUse`, Hold일 때만 `UpdateEquipmentUse`가 반복된다. Q35 즉시 클릭의 대상·거리·재료 검증을 수행하는 현재 삽 구현은 없다. 공급함 F 반환도 현재 전용 동작이 아니다.

## 3. 순환기 레버·원형 계기

- `/Game` Asset Registry의 `Gauge, Needle, Dial, Lever, Pump, Pressure, Meter` 이름 검색은 0개. 세 utility BP는 공통 단일 cube mesh와 위 6개 inherited component뿐이다. 현재 구성에서 바늘/레버만 독립 회전시키는 component, 계기 값·시작 비율·회전 범위 설정은 확인되지 않았다.
- `SM_Facility_sample`의 local bounds는 (−50,−50,0)~(50,50,100)cm이고 세 설비의 상대 위치·회전은 (0,0,0), 위 표의 scale이다. `boiler` mesh bounds는 위와 같다. MCP는 bounds/슬롯은 조회하지만 mesh의 실제 조작부 분리, 바늘/레버 pivot, 로컬 회전축, 계기와 손잡이의 시야 동시 가시성을 판정하지 못한다. 현 BP에 해당 부품 자체가 없으므로 축·각도·화면 크기를 추정하지 않는다.
- 욕탕의 기존 `FillValveControl`·`DrainLeverControl`은 분리 component를 회전시키는 Source 사례(`Source/BathhouseSim/Private/Facility/BathWaterControlComponent.cpp`)이지만, 순환기 펌프 asset이나 한 왕복 작업 구현으로 확인된 것은 아니다.

## 4. 입력·작업 취소와 회수

- Editor serialized `/Game/FirstPersonCharacter/BP_FirstPersonCharacter` CDO는 `IA_Interact`, `IA_SecondaryInteract`, `IA_DropCarry`, `IA_RecoverFacility`, `IA_PrimaryUse`를 연결했고 `ComputerClickAction=None`. `/Game/Input/IMC_FirstPerson`의 `Mappings`는 각각 E/F/G/Q/LMB다. 이 부분은 **Editor 연결 확인**이다.
- **Source 동작**: `Source/BathhouseSim/Private/Character/FirstPersonCharacter.cpp`에서 E Started/Completed/Canceled, F Started, Q Started/Completed/Canceled, LMB Started/Triggered/Completed/Canceled를 받는다. LMB 소유 우선순위는 Computer → Placement → Equipment. 컴퓨터 입력 포획 중 E는 컴퓨터 종료, F는 일반 보조 상호작용으로 보내지지 않는다. E의 기존 Hold 경로(`Source/BathhouseSim/Private/Interaction/PlayerInteractionComponent.cpp`)는 입력 해제·시선/거리 이탈·대상 소실·조건 변경·interaction suppression에서 취소된다. Q30·Q36의 **E 클릭 후 손을 떼도 1초 한 왕복 진행**은 이 Hold 계약과 다르며 현재 순환기 작업은 없다.
- 한 번 시작한 시간 기반 동작 사례는 `Source/BathhouseSim/Private/Combat/MeleeAttackComponent.cpp`의 공격 duration/명중 시점, `Source/BathhouseSim/Private/Towel/TowelProcessingMachineActor.cpp`의 처리 timer다. 둘 다 순환기 E 왕복의 현재 연결은 아니다. `Source/BathhouseSim/Private/Interaction/PlayerEquipmentUseComponent.cpp`의 Hold 사용은 LMB가 유지될 때만 갱신하며 도구 상실·drop·computer suppression에서 취소된다.
- `Source/BathhouseSim/Private/Placement/PlayerFacilityPlacementComponent.cpp`는 Q Hold 시작 후 매 Tick 대상 trace/조건을 재확인하고 시간 충족 시 다시 조회해 회수 transaction을 수행한다. 해제·시선/거리 이탈·대상 소실·컴퓨터 진입 시 취소한다. `Source/BathhouseSim/Private/Facility/BathWaterUtilityFacilityActor.cpp`의 설비 Hold 상태는 단순 bool이며 현재 노동·투입 차단 로직과 자연 감소가 없다.
- 회수 시 `Source/BathhouseSim/Public/Facility/BathWaterUtilityPlacementInstanceData.h`와 utility Actor의 `ExportPlacementPayload/ImportPlacementPayload`는 **CapacityKind, CapacityPoints, Definition만** 보존·복원한다. `Source/BathhouseSim/Private/Placement/FacilityActorConversionTransaction.cpp`가 회수 item 생성→payload→domain unregister→원본 제거, 재설치 staged actor→payload import→등록 transaction을 담당한다. 가동 잔량은 payload에 없고 포장 중 감소 정지/재설치 잔량 복원/회수 Hold 동안 계속 감소/취소 후 비복원은 현재 지원되지 않는다. 현재 Definition=None이므로 정상 회수 통과는 별도 제약이다.

## 5. 컴퓨터와 HUD

- Exact WBP/native parent: `/Game/Bathhouse/UI/WBP_BathWaterManagementScreen` → `/Script/BathhouseSim.BathWaterManagementScreenWidget`; `WBP_BathWaterCapacitySummary` → `BathWaterCapacitySummaryWidget`; `WBP_BathWaterMap` → `BathWaterMapWidget`; `WBP_BathWaterBathTile` → `BathWaterBathTileWidget`; `WBP_BathWaterDetail` → `BathWaterDetailWidget`; `WBP_InteractionPrompt` → `InteractionPromptWidget`; `WBP_ComputerSampleScreen` → `ComputerSampleScreenWidget`. 모두 `/Game/Bathhouse/UI/` 아래이며 MCP parent/dependency로 재확인했다.
- `BP_BathhouseComputer`의 `DefaultMap` 인스턴스는 (-470,0,160), `ScreenWidget.WidgetClass=WBP_BathWaterManagementScreen_C`, DrawSize (1024,576)이다. `ManagedBathPlacementZone`은 `BP_FacilityPlacementZone_C_UAID_F02F7433CA36D1FF02_1155169559`를 참조한다(MCP instance 조회). `Source/BathhouseSim/Public/UI/BathWaterManagementScreenWidget.h`는 CapacitySummary/ BathMap/ BathDetail 3영역을 요구한다. `Source/BathhouseSim/Private/UI/BathWaterCapacitySummaryWidget.cpp`는 종류별 현재 `UsedPoints / TotalPoints`와 deficit/status/bar만 표시한다. 설치·가동 두 총량의 별도 행은 Source 계약에 없다. `Source/BathhouseSim/Private/UI/BathWaterMapWidget.cpp`는 Bath snapshot으로 tile만 만들어 설비를 지도에 추가하지 않는다.
- `Source/BathhouseSim/Public/UI/InteractionPromptWidget.h` 및 `.cpp`에는 작업명/오류와 primary·equipment·recovery progress 슬롯이 있다. 전용 순환기 왕복 진행 상태나 삽 재료 오류를 공급하는 현재 utility 경로는 없다. `WBP_InteractionPrompt`의 실제 child 연결/여백은 별도 확인 불가.
- MCP의 Object/Blueprint 도구는 WBP `WidgetTree` 읽기를 노출하지 않았다(`ObjectTools.get_properties`에서 `WidgetTree` 조회 실패). 실제 1024×576 화면에서 3종 용량 추가 공간, child 위치·크기·BindWidget 연결의 serialized 상세는 **조회 불가**다. Source의 필드 및 `.md/Unreal/InteractionUISystem.md` 기록을 실제 화면 치수 근거로 대체하지 않는다.

## 6. 첫 보일러 수직 검증 준비와 남은 확인

- 현재 `DefaultMap`은 보일러 1, 순환기 1, 욕탕 2, 관리 컴퓨터 1 인스턴스를 갖는다. 컴퓨터의 Zone 연결도 확인됐다. 두 욕탕 `BathWaterCondition` 인스턴스는 각각 최대 순환 요구 100, 가열/냉각 요구 5 points/°C다. 이는 테스트 **대상 배치** 확인일 뿐 작동 검증 완료가 아니다. 석탄 공급함·삽·투입구·계기/바늘 asset 및 레벨 배치는 확인되지 않았고, `BP_Boiler`는 공통 cube, `boiler` 외형은 별도 StaticMeshActor다. Definition 누락도 설치/회수 경로 확인에 걸린다.
- 현재 Source에서는 preplaced 순환기 정격 100이 등록되면 항시 공급되는 경로가 있어 보일러 가열 흐름의 순환 전제와 구분해 기록한다. Q29~Q47 최종 계약에서는 순환기도 신규 설치 가동수치 0이므로 **첫 보일러 검증을 위한 순환 공급 상태**를 검증 시점에 별도로 확보해야 한다. 이번 조사는 테스트용 변경/PIE를 수행하지 않아 실제 공급·목표 수온·정화·소진 전이는 미확인이다.
- 동작명세 단계의 추가 **사용자 선택**은 Q29~Q47 범위에서는 발견되지 않았다. 남은 것은 asset 시각 적합성(삽 적재 외형, 분리 투입구, 레버/바늘 pivot·축·시야), WidgetTree 공간/연결, Definition 누락의 실제 런타임 영향, PIE 수용 결과 등 **기술·Editor 검증 사항**이다. 조회 불가는 asset 부재로 분류하지 않는다.
- 이번 조사에서 Source/Config/Content/Level/Blueprint/Project Settings 수정, asset 생성·Compile·Save·재저장·PIE·시뮬레이션·Computer Use·Editor 종료를 하지 않았다. 본 작업이 갱신한 것은 이 보고서뿐이다. 시작 Git은 clean이었으나 종료 점검에서 예상치 못한 미추적 `.md/BATH_WATER_SLIDER_ISSUE.md`가 별도로 나타났다. 본 작업은 이 파일을 생성·수정하지 않았으며 보존했다. 대상 asset의 MCP dirty=false와 `git diff --check`를 확인했다.
