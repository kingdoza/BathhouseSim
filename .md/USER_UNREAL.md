# 사용자 Unreal 후속 작업

허용된 자동화와 승인된 화면 작업으로 끝낼 수 없는 실제 Editor 수정 작업의 큐다. 형식과 처리 규칙은 [AGENT_WORKFLOW.md](AGENT_WORKFLOW.md)의 `USER_UNREAL.md` 절을 따른다.

- 아래 기존 항목의 `PROMPT_UNREAL.md`, `PROMPT_INTEGRATION_REVIEW.md` 참조는 2026-10-01 삭제된 루트 결과물이며 Git 이력에서 읽는다.
- 기존 항목에 섞인 PIE·플레이 검증은 해당 작업을 이어갈 때 마스터가 작업 폴더의 `PIE_CHECKLIST.md`로 옮긴다.

## 서비스 4단위 (안마의자·평상·TV·세신) — PIE 수용 대기

남은 실제 Editor 수정 작업 없음. PIE·화면 판정 항목은 `.md/Work/SERVICE/SERVICE-U4/PIE_CHECKLIST.md`로 옮겼다(2026-10-01).

---

## 서비스 3단위 (쓰레기·수거) — PIE 수용 대기

MCP·Python으로 authoring·Compile·Data Validation·개별 Save·재로드 대조는 끝났다(`.md/Unreal/CleaningSystem.md`, `PROMPT_INTEGRATION_REVIEW.md`). MCP로 입력·console을 실행할 수 없어 아래는 사용자 Editor에서 확인해야 한다. 완료 전에는 이 작업을 완료 처리하지 않는다.

1. **대표 PIE 전체** (`.md/PROMPT_UNREAL.md`의 "대표 PIE 검증"): TRSH-001~030, COLL-001~009. 특히 ① 빈손 쓰레기 조준 HUD·Visibility trace, ② 거치대 집게 E→쓰레기 LMB 3회 `봉투 3/20`, ③ RMB 봉투 묶기·실패 문구, ④ 집게 거치/G/낙하 복구, ⑤ 봉투 들기·수거(`bathhouse.Debug.TrashCollection.CollectNow`)·제외 대상, ⑥ 손님 체류 시 쓰레기·얼룩 발생(탈의 구역), ⑦ 새 RMB 행(`WBP_InteractionPrompt`) 표시. 실패하면 조준 위치와 Output Log를 남긴다.
2. **배치·임시 도형 화면 확인**: 제안 위치가 맞는지(쓰레기 구역 `(1050,-280)` 탈의 구역 하나만 덮음, 집게 거치대 `(650,750,43)`, 수거 구역 `(-800,450)` 출입구 밖, 겹침 없음), 집게(긴 Cube)·봉투(Cube)·쓰레기(병/면봉/휴지 대용)가 보이는 크기, 수거 구역 바닥 표시(초록 plane)가 경계를 설명하는지, RMB 행이 다른 행과 겹치지 않는지(위쪽 여백 640/680 제안값). 욕탕 바닥·다른 체류 영역이 필요하면 구역 추가를 요청한다. 어긋나면 값을 알려 주면 재작업한다.
3. **바닥 mesh 조건**: `Studio_floor`는 Static·`WorldStatic`으로 확인했다. 다른 바닥 mesh를 추가하면 같은 조건을 만족해야 쓰레기가 생성된다.

---

## 서비스 2단위 (화장대·비품 여섯 종) — PIE 수용 대기

MCP로 authoring·Compile·개별 Save·재로드 대조는 끝났다(`.md/Unreal/ServiceSystem.md`, `PROMPT_INTEGRATION_REVIEW.md`). MCP는 입력·console 명령을 실행할 수 없어 아래는 사용자 Editor에서 확인해야 한다. 완료 전에는 이 작업을 완료 처리하지 않는다.

1. **PIE 대표 시나리오와 표 전체** (`.md/PROMPT_UNREAL.md`의 "검증 순서와 대표 PIE" 이하): SHWR→TOWL, DISP-015~017·023·024, VANI-001~017, SHWR-001~011, TOWL-001~018, SHOP-S03·S04, F1(knockdown 재개 후 소모 불변), F2(보류 취소 뒤 대기 손님 자동 재시도, 실제 StateTree 전이). 실패하면 조준 위치와 Output Log를 남긴다. 콘솔: `bathhouse.Debug.Facility.BeginUse` / `.EndUse`.
2. **임시 도형 화면 확인**: 박스 안 품목 배치(각 품목 정원 개수)와 트레이 안 수용, 화장대 네 그룹 자리·조준 범위(몸체 앞면 조준이 router보다 먼저 막히는지), 샤워기 샴푸/바디워시 자리, Washer/Dryer 뚜껑 열림 축·끼임과 pile 가시성(pile 범위는 컴포넌트 scale을 고려해 월드 약 ±10cm로 환산해 두었다). 어긋나면 값을 알려 주면 재작업한다.
3. **Data Validation 명시 실행 (선택)**: 이번 작업엔 MCP tool이 없고 Python API는 승인되지 않았다. 저장 시 AssetCheck는 오류 0이었다. 원하면 Python API 사용을 승인해 6개 DA·`BP_Vanity`·`DA_FacilityPlacement_Vanity`·`BP_Shower`·`BP_Washer`·`BP_Dryer`·`DA_ShopCatalog`를 일괄 검증할 수 있다.
4. **연결 절차 갱신 (문서)**: MCP 서버는 `-ModelContextProtocolStartServer` 인자(또는 설정 `bAutoStartServer`)가 있어야 포트가 열린다. `.md/UNREAL_MCP_CONNECTION.md`에 반영할지 결정한다.

---

## 서비스 1단위 수직 (품목 박스·진열·음료 냉장고·수거함) — MCP 불가 항목

MCP로 신규 asset 7개와 `DA_ShopCatalog`, `BP_FirstPersonCharacter` 카메라 blendable은 저장·재로드했다(`.md/Unreal/ServiceSystem.md`). Config(`ItemBoxClass`, `InsertPreviewMaterial`, `r.CustomDepth=3`)는 사용자 승인으로 직접 반영했고 새 Editor에서 값을 확인했다. 아래는 남은 항목이다. 완료 확인 전에는 이 작업을 완료 처리하지 않는다.

1. **DefaultMap에 `BP_DrinkCollectionBox` 배치·저장 (사용자 직접).** `SceneTools.save_actor`가 external actor 경로 오류(`Asset does not exist: /Game/__ExternalActors__/Maps/DefaultMap/1/P1/UWXIDD9LM1ZURKQSERVEKZ`)로 실패해 MCP로는 저장할 수 없다.
   - 배치할 액터: `/Game/Bathhouse/Blueprints/Service/BP_DrinkCollectionBox`(1개, 레벨 액터 이름 `DrinkCollectionBox`). 카운터(`BP_BathhouseCounter`, 원점) 상판 위 world `(-50,-20,90)`에 두면 된다(카운터 bounds x∈[-80,80], y∈[-50,50], 상판 z=75; 손님 서비스 지점 x=220 반대편, `ReturnedKeyDropPoint (20,20,90)`과 겹치지 않음). 회전·scale 기본값.
   - 저장 후 `git status`에서 `DefaultMap.umap`이 아닌 `__ExternalActors__` package만 변경됐는지 확인한다. `Save All`은 사용하지 않는다.
2. **직접 화면·입력 검증** (`.md/PROMPT_UNREAL.md`의 PIE 절차 1~9와 DISP 표). 특히 `M_PP_TakeHighlightOutline`이 실제로 마지막 자리 외곽선을 그리는지(셰이더 컴파일 오류는 로그에 없으나 화면은 미확인), 화면 네 테두리에 선이 없는지, `BP_ItemBox` Blueprint 뷰포트 미리보기(`EditorPreviewCount` 7)에서 병 7개가 트레이 위에 보이는지, 병(Cylinder)·트레이(Cube)·냉장고 모양이 임시 mesh로 충분한지 확인한다. 보이지 않으면 stencil 판독 채널(`.r`)과 `DepthBias`를 먼저 의심한다.
3. **`WBP_InteractionPrompt` 보완 (선택, 이번 승인 범위 밖).** `HeldSummaryText`는 Python API로 추가·저장했다(`PromptRoot` Overlay 자식, 위쪽 여백 600). 다만 저장된 트리에는 held-use 단계 항목인 `HeldTakeActionNameText`, `HeldTakeFailureReasonText`, `PrimaryKeyText`, `LmbKeyText`, `RmbKeyText`(모두 `BindWidgetOptional`)가 아직 없다. RMB 행·키 라벨을 화면에 쓰려면 별도 승인으로 추가한다. 위치(위쪽 여백 600)는 화면에서 보고 조정한다.
4. 임시 art 교체(선택): 위가 열린 박스 mesh, 바나나우유 mesh, 냉장고 mesh. 교체하면 `BoxSlotTransforms`와 `DisplayOffset`을 다시 맞춘다.

---

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

위 항목과 Data Validation이 통과해야 Editor 단계를 완료 처리할 수 있다. `Save All`은 사용하지 않는다.

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

## 3. Definition Data Validation 네이티브 차단점

다음 두 opt-out Definition은 의도대로 `PlacedFacilityClass=None`, `RecoveryItemClass=None` 상태지만 현재 native `UFacilityPlacementDefinition::IsDataValid()`가 opt-out을 구분하지 않아 각각 세 개의 오류를 낸다.

- `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_CleanTowelStack`
- `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_UsedTowelBin`

오류는 `Placed Facility Class must implement IPlaceableFacility`, `Recovery Item Class must derive from APlaceableFacilityItemActor`, `Placed and recovery item classes must be different`다.

Editor에서 class를 임의로 채우지 않는다. 그러면 Stack/Bin의 placement opt-out 계약이 깨진다. 구현 단계로 되돌려 `IsDataValid()`가 conversion opt-out Definition에는 placement/recovery class 검사를 적용하지 않도록 수정하고 코드 리뷰를 다시 받아야 한다. 그 뒤 Definition 9개를 Data Validation한다.

활성 Definition 7개는 helper collision/Navigation 오류 없이 검증됐고, `RecoveryItemMesh=None`에 대한 native Cube fallback 경고만 남았다.

## 4. 직접 플레이 검증

1·3번을 마친 뒤 PIE에서 Bath, Shower, Locker 1/4/8, Washer, Dryer를 각각 확인한다.

1. 설비를 들었을 때 class-default의 모든 body mesh가 preview에 나타나는지 확인한다.
2. 설치 가능 위치는 초록, 불가 위치는 빨강 반투명 재질이 모든 mesh slot에 적용되는지 확인한다.
3. 모든 footprint bottom이 Zone의 `PlacementFloor`와 일치하고 부양·매몰되지 않는지 확인한다.
4. LCtrl snap, wheel yaw, containment, blocking overlap, 네 corner floor support를 확인한다.
5. confirm 전 preview가 collision/NavMesh를 만들지 않고, confirm 뒤 body collision에 따라 NavMesh가 갱신되는지 확인한다.
6. 빈 설비 회수 시 source collision/NavMesh가 사라지고, rollback이면 복구되며, 회수 item 재배치 뒤 NavMesh가 다시 생성되는지 확인한다.
7. Clean Towel Stack과 Used Towel Bin에 placement/recovery prompt가 생기지 않는지 확인한다.
8. 설비 회수 아이템이 공통 `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem`의 `ItemRoot`에 지정한 Scale(asset 값) 그대로 생성되고 E pickup/G drop 뒤에도 같은 크기를 유지하는지 확인한다.

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
3. minor line이 Project Settings grid 간격의 실제 snap과 일치하고, 10칸마다 major line이 나타나는지 확인한다. grid 한 장이 `ZoneBounds` 전체를 덮고 셀 내부에는 line과 분리된 fill이 보여야 한다.
4. confirm 성공, cancel, preview 실패, held item 교체, interaction suppression과 PIE 종료 각각에서 모든 grid가 즉시 숨는지 확인한다.
5. 기존 초록/빨강 preview, wheel yaw, LCtrl snap, LMB confirm, E/G/Q 입력과 prompt가 그대로 동작하는지 확인한다.
6. grid가 collision/overlap/physics/NavMesh를 만들지 않고 벽과 설비 뒤에서는 가려지는지 확인한다.
7. 같은 placement session을 유지해도 DMI 생성 또는 visibility notification 경고가 매 Tick 반복되지 않는지 Output Log를 확인한다.

`DefaultMap`에는 현재 PlacementZone이 한 개뿐이므로 서로 다른 allowed tag를 가진 여러 Zone의 동시 표시(`FP-GRID-01`)는 이 Map만으로 완전 검증할 수 없다. 별도 테스트 Level 또는 저장하지 않을 임시 Zone 구성이 준비되면 compatible/incompatible Zone을 함께 두고 확인한다.

## 재개 조건

- Project Settings의 두 Material reference가 재시작 후 유지된다.
- opt-out Definition 두 개의 native Data Validation 오류가 수정된다.
- Definition 9개와 pre-placed Locker 두 개의 Data Validation이 통과한다.
- 4번 직접 플레이 검증 결과를 기록한다.
- 5번 native grid 입력·시각 검증 결과를 기록한다.

완료 후 당시 Editor 보고(`PROMPT_INTEGRATION_REVIEW.md`, Git 이력)의 미완료 항목을 다시 판정한다.

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

# 컴퓨터 포커스 CMP-001~020 — MCP 작업 결과 및 남은 수동 수용 (2026-09-27)

## MCP 작업 완료

- CDO FocusExitPoint와 Escape 취소 입력을 authoring했다. 저장·새 프로세스 재로드로 확인된 위치는 (1500,0,-228.5714285714), 회전 yaw 180이며 SearchRadius는 100cm다. FocusExitArrow는 FocusExitPoint의 자식이고 local origin/zero rotation, editor-only, 길이 80cm다. Level 인스턴스도 동일 transform을 읽었다. actor transform (-470,0,160), scale (0.12,1.2,0.7) 기준 세계 발 위치는 (-290,0,0), 바라보는 방향은 -X다. 이전 인계의 X=1000 값은 현재 fresh-process readback과 달라 최신 저장 상태로 정정한다.
- BP_FirstPersonCharacter.CancelAction은 IA_Cancel, 기존 InteractAction은 IA_Interact다. IA_Cancel ValueType은 Boolean이다. IMC_FirstPerson의 기존 7개 mapping을 보존하고 Escape → IA_Cancel을 추가해 8개이며, BP_FirstPersonController.DefaultMappingContext는 IMC_FirstPerson이다.
- BP_BathhouseComputer, BP_FirstPersonCharacter, BP_FirstPersonController는 warnings-as-errors Compile을 통과했다. 네 변경 에셋(BP_BathhouseComputer, BP_FirstPersonCharacter, IMC_FirstPerson, IA_Cancel)은 개별 Save 후 fresh-process reload에서 모두 dirty=false였다. Controller는 변경하지 않았다.
- 현재 Level은 /Game/Maps/DefaultMap이다. exact computer actor의 ScreenWidget과 ManagedBathPlacementZone 참조가 유지되고 FocusExitPoint가 CDO와 같은 값으로 재로드됐다. Map dirty=false이며 성공한 map/external actor Save는 없었다. 앞선 SceneTools.save_actor 호출은 external actor asset registry 경로 오류로 실패했다.
- Focus foot point에서 아래로 한 trace는 바닥 Z=0을 가리켰다. 반경 30cm, 반높이 96cm capsule의 보수적 AABB query는 landscape tiles와 ManagedPlacementZone만 돌려줬고 컴퓨터 mesh/다른 blocker는 반환하지 않았다. 이 AABB 조회는 Pawn capsule의 실제 collision response test가 아니므로 blocker 수용 통과로 간주하지 않는다.
- MCP StartPIE는 in-viewport PIE를 시작했고 PIE server login 및 월드 초기화가 로그에 남았다. StopPIE 뒤 IsPIERunning=false였다. task 관련 로그에서 gameplay error/ensure는 확인되지 않았다. Editor MapCheck의 0 error/0 warning 로그는 있었지만 Data Validation 실행 결과는 아니다. PIE에서 키보드/마우스 입력 시나리오는 수행하지 않았다.

## 연결과 종료 기록

- 제한된 최초 실행에서 AutomationTool이 사용자 AppData 로그 경로에 쓸 때 UnauthorizedAccessException이 발생했다. 승인된 elevated 실행 후 Turnkey와 MCP가 정상 동작했다.
- 그 뒤 PID 20416/4284 재로드 실행은 MCP 리스너까지 열거나 TCP 연결을 맺었지만 Python 초기화/에디터 toolset 응답에 도달하지 못했다. 이 정지 원인은 확인되지 않았다.
- 최신 PID 22480에서는 에디터 toolset 19개가 등록됐다. 현재 대화의 도구 catalog에는 Unreal wrapper가 노출되지 않았지만, 저장소 지침에 따른 직접 MCP initialize → tools/list → list_toolsets와 read-only 조회가 성공했다. project MCP 설정은 바꾸지 않았다.
- MCP 작업 후 PID 22480의 정상 창 종료 요청은 실패해 작업 소유 PID만 종료했다. 확인 결과 UnrealEditor 프로세스와 8000 listener가 없다.

## 남은 Editor 수용

1. Editor의 Asset Actions에서 allowlist 에셋 Data Validation을 실행하고 결과를 기록한다. 현재 MCP toolset에는 Data Validation 호출 도구가 없다.
2. 실제 플레이 입력·시각 수용은 직접 수행한다: E 진입·release, Escape 이탈, 화면 버튼/slider drag 중 LMB release, 커서 중앙 복귀, 서로 다른 진입 위치에서 고정 발 위치·방향, blocker/forced 경로, 바닥 낙하 조건, placement/lever/recovery 입력과 재진입. MCP에는 keyboard/mouse 입력 주입 기능이 없다.
3. FocusExitArrow 및 실제 화면 배치는 Editor viewport에서 육안 확인한다. CaptureEditorImage 요청은 프로젝트 UI/asset 정보가 MCP로 전송될 수 있다는 사유로 automatic approval review에서 거부됐다. 캡처 우회는 하지 않는다.

위 직접 입력·시각 수용과 Data Validation이 끝나기 전까지 전체 CMP-001~020 통합 완료로 판정하지 않는다. MCP로 가능한 authoring, compile, save, reload, PIE start/stop은 수행했다.

# 상점 Editor 미지원 작업 — 현재 인계

## 남은 MCP 미지원 authoring

1. `/Game/Bathhouse/UI/WBP_ComputerScreenRoot`, `/Game/Bathhouse/UI/Shop/WBP_ShopProductCard`, `/Game/Bathhouse/UI/Shop/WBP_ShopCartLine`, `/Game/Bathhouse/UI/Shop/WBP_ShopOrderLine`, `/Game/Bathhouse/UI/WBP_MoneyHud`, `/Game/Bathhouse/UI/WBP_ShopNotice`의 Widget hierarchy와 BindWidget 이름을 해당 구현 프롬프트대로 authoring한다. 현재 MCP에 Widget tree 편집 tool이 없다. 필요한 class 연결 후 변경 asset만 Compile·개별 Save한다.
2. `/Game/Bathhouse/UI/Shop/WBP_ShopScreen`을 열어 `ProductScroll`이 상품 목록만 포함하고 cart/order panel은 바깥 형제로 남는지 확인한다. MCP는 `WidgetTree`를 조회·편집하지 못한다. 계층이 이미 맞으면 수정·저장하지 않고, 다르면 승인된 layout만 조정해 WBP를 저장한다.
3. `/Game/Maps/DefaultMap`에 `BP_ShopDeliveryPoint` 한 개와 `BP_TrashBin` 한 개 이상을 authoring하고 바닥, 상자 stack 공간, 낮은 천장 공간을 구성한다. DeliveryPoint 후보는 `(1800,650,0)`이었다. 이전 임시 actor의 World Partition external package 저장은 `/Game/__ExternalActors__/Maps/DefaultMap/C/FD/PZ6HQFYUX7L4RVM1PXG25B` 부재 오류로 실패했다. 현재 actor package 개별 저장 지원을 확인한 뒤에만 배치를 확대한다.
4. Catalog, Shop Blueprint, 관련 Definition에 Data Validation을 실행한다. 현재 MCP toolset에는 Data Validation 실행 tool이 없다. 이전 Catalog save-time 로그에는 “Every shop product requires a valid placement definition.”가 남았다. 명시적 검증은 별도 승인 뒤 수행한다.
5. 실제 keyboard/mouse 입력과 시각 수용은 MCP 입력 주입 tool이 없어 자동 수행할 수 없다. 상점 tab·상품 추가·수량 상한·주문·FIFO 알림, 배송 상자 pickup/drop/re-pick, LMB 개봉, 막힌 구석, 쓰레기통 discard, 잔액/HUD 및 화면 layout을 직접 확인한다. PIE 수용은 사용자 승인 후 진행한다.

Project Settings의 Shop Catalog, DeliveryBoxClass 참조와 `UnboxOverlapDepthCm=8`은 이번 MCP 작업에서 CDO 및 Config 기준으로 이미 일치함을 확인했으므로 미완료 작업 큐에서 제외했다.

# Held target use — MCP 미지원 authoring 인계 (2026-09-28)

## 1. Input Mapping Context

- 대상: `/Game/Input/IMC_FirstPerson`
- MCP readback 기준 기존 유효 binding 8개는 IA_Interact/E, IA_SecondaryInteract/F, IA_DropCarry/G, IA_PrimaryUse/LeftMouseButton, IA_RecoverFacility/Q, IA_PlacementSnap/LeftControl, IA_PlacementRotate/MouseWheelAxis, IA_Cancel/Escape다. 모두 보존한다.
- `RightMouseButton → /Game/Input/Actions/IA_SecondaryUse` binding을 추가한다. `IA_SecondaryInteract/F`는 이동·삭제하지 않는다.
- `defaultKeyMappings` readback의 기존 유효 row 15개를 보존하고 끝의 빈 None/None row만 제거한다. 앞선 저장 로그에 빈 Input Action mapping 오류가 남아 있다. 현재 에디터 표시가 다르면 전체를 덮어쓰지 말고 실제 rows를 먼저 비교한다.
- 현 MCP ObjectTools 배열 편집은 삽입 지점을 특정할 수 없다며 추가·삭제를 모두 거부했다. MCP 실패 뒤 package는 dirty=false였고 이번 MCP 작업에서는 수정·저장하지 않았다. Editor에서 변경 후 이 에셋만 개별 Save한다.

## 2. Interaction Prompt Widget hierarchy

- 대상: `/Game/Bathhouse/UI/WBP_InteractionPrompt`
- 기존 native parent와 다음 15개 필수 BindWidget을 보존한다: `PromptRoot`, `TargetNameText`, `ActionNameText`, `FailureReasonText`, `SecondaryActionNameText`, `SecondaryFailureReasonText`, `InteractionProgressBar`, `EquipmentActionNameText`, `EquipmentFailureReasonText`, `EquipmentProgressBar`, `PlacementActionNameText`, `PlacementFailureReasonText`, `RecoveryActionNameText`, `RecoveryFailureReasonText`, `RecoveryProgressBar`.
- 다음 다섯 TextBlock을 정확한 이름으로 적절한 키 행에 추가한다: `HeldTakeActionNameText`, `HeldTakeFailureReasonText`, `PrimaryKeyText`, `LmbKeyText`, `RmbKeyText`. HeldTake 텍스트는 RMB 행에 두고, key label은 E/LMB/RMB 행에 배치한다.
- Event Graph gameplay logic은 추가하지 않는다. 고정 키 글자가 native key label과 중복되면 정리한다. 변경된 경우에만 이 WBP를 개별 Save한다.
- 현 MCP toolset에는 WidgetTree/hierarchy authoring 기능이 없어서 미완료다. WBP는 이번 작업에서 수정·저장하지 않았다.

## 재개 및 범위

위 두 에셋의 MCP 미지원 authoring이 실제 Editor에서 완료됐다고 사용자가 알리면, 이후 명시된 MCP 작업에서 실제 asset 상태를 읽어 인계 항목을 확인·정리한다. 이번 요청에서 제외된 Compile, Data Validation, 새 Editor reload와 PIE는 수행하지 않았다. 해당 검증 범위는 사용자가 다시 승인하기 전까지 제외한다.
