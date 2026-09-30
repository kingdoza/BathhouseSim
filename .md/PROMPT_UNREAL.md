# Unreal 작업 프롬프트 — 서비스 3단위: 쓰레기·수거

## 단계와 진입 조건

- Editor authoring 필요. 구현 기준은 `.md/PROMPT_IMPLEMENTATION.md`, `.md/PROMPT_ARCHITECTURE.md`의 TRSH-001~030·COLL-001~009다.
- `.md/AGENT_WORKFLOW.md` → `.md/AGENT_UNREAL_MCP.md`를 읽는다. C++ 사전 리뷰 승인 후 실행한다.
- 정본: `Architecture/CleaningLitterSystem.md`, `CleaningSystem.md`, `PhysicalCarrySystem.md`, `HeldTargetUseSystem.md`, `PlacementSystem.md`.
- Editor 지도: `Unreal/0_UNREAL.md`, `FacilitySystem.md`, `ShopSystem.md`, `ServiceSystem.md`, `InteractionUISystem.md`, `WorldSystem.md`, `PlacementSystem.md`.
- 새 DLL로 Editor를 재시작한다. 기존 Source·서비스 1/2단위 asset을 유지한다. 구현 단계에서는 Content·Config를 저장하지 않았다.
- 로드 검증·빌드·automation 결과와 로그는 `PROMPT_REVIEW.md`를 확인한다. Q1 A와 2026-10-01 Pawn 여유 property 즉시 삭제에는 Core Redirect가 없다.
- Blueprint graph로 개수·spawn·청소·carry·수거·RMB 분기·설비 배치 정리를 구현하지 않는다. 모두 C++ 소유다.

## 저장 allowlist

| exact asset package | Parent Class | 작업 |
|---|---|---|
| `/Game/Bathhouse/Blueprints/Cleaning/BP_Litter` | `/Script/BathhouseSim.LitterActor` | 신규 |
| `/Game/Bathhouse/Blueprints/Cleaning/BP_LitterSpawnZone` | `/Script/BathhouseSim.LitterSpawnZoneActor` | 신규 |
| `/Game/Bathhouse/Blueprints/Cleaning/BP_LitterTongs` | `/Script/BathhouseSim.LitterTongsActor` | 신규 |
| `/Game/Bathhouse/Blueprints/Cleaning/BP_TrashBag` | `/Script/BathhouseSim.TrashBagActor` | 신규 |
| `/Game/Bathhouse/Blueprints/Cleaning/BP_TrashCollectionZone` | `/Script/BathhouseSim.TrashCollectionZoneActor` | 신규 |
| `/Game/Bathhouse/Blueprints/Cleaning/BP_CleaningDirector` | `/Script/BathhouseSim.CleaningDirectorActor` | 기존 값·class 연결 |
| `/Game/Bathhouse/Blueprints/Cleaning/BP_WaterStain` | `/Script/BathhouseSim.WaterStainActor` | 실제 반경 |
| `/Game/Bathhouse/Blueprints/Cleaning/BP_StainSpawnZone` | `/Script/BathhouseSim.StainSpawnZoneActor` | Q1 A compile/resave |
| `/Game/Bathhouse/UI/WBP_InteractionPrompt` | `/Script/BathhouseSim.InteractionPromptWidget` | RMB 행 |

- DefaultMap의 아래 지정 Actor만 해당 `/Game/__ExternalActors__/Maps/DefaultMap/...` package를 개별 저장한다. 조사 후 변경 전 exact actor path·external package·transform·값 목록을 고정해 결과에 기록한다.
- 신규 instance는 생성 직후 exact external package를 allowlist에 추가해 개별 저장한다. 기존 stain zone/director, 집게·fixed slot, 쓰레기 구역·수거 구역, 제거할 trash bin 이외의 actor는 포함하지 않는다.
- `/Game/Maps/DefaultMap`의 `.umap`은 저장 금지다. Save All·Config 변경·기존 BP의 reparent·native component rename도 금지다.
- 기존 `BP_ItemBox`, `BP_ShopDeliveryBox`, `BP_PlaceableFacilityItem`, `BP_TrashBin`은 이번 저장 대상이 아니다.

현재 load gate로 확인한 기존 external package allowlist(실제 저장 전 transform·override를 다시 읽는다):

| package | 기존 Actor / 작업 |
|---|---|
| `/Game/__ExternalActors__/Maps/DefaultMap/3/O2/M90TW0CJ9DL2TVTK3X0IZH` | `BP_CleaningDirector_C_UAID_F02F7433CA36F6F602_1654552567`, SpawnIntervalSeconds 15 유지 |
| `/Game/__ExternalActors__/Maps/DefaultMap/6/FX/6ZGWI7FB2UN1ZMP6Q1YON1` | `BP_StainSpawnZone_C_UAID_F02F7433CA36F6F602_1665880569`, floor 조사·resave |
| `/Game/__ExternalActors__/Maps/DefaultMap/8/Q0/Y16QBOVVFUE29TAO7F2Z2S` | `BP_StainSpawnZone_C_UAID_F02F7433CA36F6F602_1661881568`, floor 조사·resave |
| `/Game/__ExternalActors__/Maps/DefaultMap/5/OS/BNP2Y7NJ91BPOXIJVR7FXP` | `BP_TrashBin_C_UAID_F02F7433CA36DF0503_2000037360`, instance 제거 |

## 신규 Blueprint 계약

### BP_Litter

- native `InteractionCollision` root(12cm sphere): QueryOnly, Visibility Block, 그 밖 Ignore, Navigation off. Scale 1. 물리 없음.
- native `LitterMesh` 자식: NoCollision, Navigation off. `MeshVariants`에 null 아닌 후보 3개 이상을 넣는다.
- 임시 후보: `/Engine/BasicShapes/Cylinder.Cylinder`(빈 병 대용), `/Engine/BasicShapes/Cube.Cube`(면봉 대용), `/Engine/BasicShapes/Sphere.Sphere`(휴지 뭉치 대용). 신규 mesh asset을 만들 필요는 없다.
- 기본 도형을 쓸 때 `LitterMesh` scale `(0.15,0.15,0.15)`, relative Z `7.5`로 시작해 도형 밑면을 바닥에 맞춘다. `FloorRadiusCm=15`가 모든 후보의 실제 XY 반경을 덮는지 확인한다. 축이 다른 외형도 바닥 높이·sphere 조준 범위에 맞춘다.
- 외형 후보/yaw는 BeginPlay의 native seed 적용으로 결정한다. BP에서 다시 randomize하지 않는다.

### BP_LitterSpawnZone

- native `SpawnBounds` root·`SpawnFloor` 자식 유지. Bounds NoCollision·Navigation off.
- 기본 bounds extent `(300,300,100)`, `SpawnFloor` relative Z `-100`. instance의 **world SpawnFloor Z**를 실제 바닥에 맞춘다.
- `MaxActiveLitterInZone=5`, `LitterSpacingOverride=0`, `FloorHeightToleranceCm=5`.
- 필요한 tag가 있으면 `RequiredFloorComponentTag`를 실제 floor component tag와 일치시킨다. trace channel·경사는 기존 stain zone과 동일 계약이다.
- 욕탕의 낮은 바닥을 포함하려면 그 바닥 높이에 별도 zone을 둔다. 설비·박스·카운터 윗면을 floor로 삼지 않는다.

### BP_LitterTongs

- native physical root `WorldMesh`: QueryAndPhysics, PhysicsActor, CCD, Pawn Ignore. exact fixed slot과 FreeDrop capability는 native다.
- 임시 Cube root scale `(0.04,0.04,0.8)`로 시작하고 집게 대용 외형·질량·조준을 확인한다. root를 바꾸지 않는다.
- `HeldTransform`은 location/rotation만 authoring한다. 시작 위치 `(45,15,-35)`, rotation 0, scale 1; 화면에서 조준을 가리지 않도록 조정한다.
- `BagCapacity=20`, `TieForwardDistanceCm=60`, `TieMinForwardDistanceCm=30`.
- `TiedBagClass=/Game/Bathhouse/Blueprints/Cleaning/BP_TrashBag.BP_TrashBag_C`.
- 상태는 native `BagCount`다. carry·거치·G·낙하 복구 중 개수가 유지돼야 한다. 채움 외형/graph/timer를 추가하지 않는다.

### BP_TrashBag

- native `BagMesh`가 physical root다. QueryAndPhysics·CCD·Pawn Ignore, root relative location/rotation 0, 양의 finite scale을 유지한다.
- 임시 Cube 또는 Sphere, root scale `(0.3,0.3,0.4)`로 시작해 위가 닫힌 봉투 대용으로 보이게 한다. helper mesh를 붙이면 NoCollision·Navigation off다.
- `HeldTransform` 시작 위치 `(50,15,-25)`, rotation 0, scale 1. CDO root scale은 spawn·held·drop에서 보존된다.
- count는 deferred spawn 중 native가 한 번 초기화한다. 레벨에 초기화되지 않은 봉투를 직접 배치하지 않는다.
- Data Validation은 ItemBox와 동일한 physical root 계약이다. mesh/root/collision/scale 검사를 통과시킨다.

### BP_TrashCollectionZone

- native `CollectionBounds` root: NoCollision, Navigation off. 기본 extent `(150,150,100)`.
- `CollectionIntervalSeconds=300`. 별도 검증 instance에서는 60으로 테스트할 수 있으나 대표 저장 값은 300이다.
- 바닥 표시는 decal 또는 `/Engine/BasicShapes/Plane.Plane` 자식으로 authoring한다. plane NoCollision·Navigation off, 바닥보다 약간 높게 둔다. 표시가 bounds의 XY 경계를 설명해야 한다.
- pickup/수거 이벤트·돈·시간 HUD graph는 추가하지 않는다.

## 기존 Blueprint 계약

- `BP_CleaningDirector`: `LitterClass=BP_Litter_C`, `LitterMeanIntervalPerCustomerSeconds=120`, `MaxActiveLitter=20`, `DefaultLitterSpacing=40`, `SpawnUpdateIntervalSeconds=0.25`, `SpawnClearanceHeightCm=30`.
- `SpawnIntervalSeconds`는 **손님 1명당 평균 간격**이다. class와 Level instance의 기존 저장 값을 각각 먼저 기록하고 보존한다. 이름을 바꾸거나 새 default로 덮지 않는다. 기존 stain class/최대/간격을 유지한다. 삭제된 `DefaultPawnClearance`를 BP와 레벨 director의 compile/resave로 정리한다.
- `BP_WaterStain`: `FloorRadiusCm=30`에서 시작하되 실제 visual XY 반경에 맞춘다. 런타임 반경은 이 값 × 선택된 최대 XY scale이다. 기존 material·scale/yaw 범위·청소 duration을 유지한다.
- `BP_StainSpawnZone`: `SelectionWeight`, `ZoneKind`와 enum 및 `PawnClearanceOverride` 삭제를 반영해 compile·resave한다. 신규 `BP_LitterSpawnZone`에도 Pawn 여유 property가 없어야 한다. native `SpawnFloor`는 surviving `SpawnBounds` 자식이다. 기존 tag·trace·maximum·bounds를 보존한다.
- `WBP_InteractionPrompt`: 기존 hierarchy·필수 BindWidget·`HeldSummaryText`를 유지한다. `PromptRoot` 아래 RMB 행에 `HeldTakeActionNameText`, `HeldTakeFailureReasonText`, `RmbKeyText` TextBlock을 추가한다(`BindWidgetOptional`, 이름 정확히 일치).
- RMB 행 style은 기존 E/LMB 행에 맞춘다. text/visibility/result는 C++가 제어한다. `.md/USER_UNREAL.md` 항목 3의 기존 미완료와 같은 계약이며 별도 widget graph로 해결하지 않는다.

## DefaultMap authoring

0. DefaultMap의 생성 대상 floor mesh를 읽어 collision object type `WorldStatic`·Mobility `Static`인지 PIE 전에 확인한다. 둘 중 하나라도 다르면 바닥이 기각되므로 임의 collision profile 변경으로 우회하지 않고 소유 단계로 보고한다.
1. 기존 stain zone instance 전체를 조사한다. native `SpawnFloor` world Z를 각 실제 바닥에 맞추고 욕탕 포함 의도를 확인해 external actor를 resave한다. `PawnClearanceOverride` 삭제를 포함하며 다른 override를 지우지 않아야 한다.
2. 기존 director instance의 `SpawnIntervalSeconds` override를 보존한다. `DefaultPawnClearance` 삭제를 반영해 external actor를 resave하고 신규 쓰레기 값이 BP를 상속하는지 확인한다.
3. 탈의실 등 대표 손님 체류 영역에 `BP_LitterSpawnZone`을 배치한다. bounds는 손님 capsule 중심까지 포함하고 `SpawnFloor`만 바닥에 둔다.
4. 집게 1개와 `/Game/Bathhouse/Blueprints/Interaction/BP_PhysicalCarryFixedSlot` instance 1개를 배치한다. parent `/Script/BathhouseSim.PhysicalCarryFixedSlotActor`, `AssignedItem`은 exact 집게 instance, `bStartOccupied=true`, `ItemAnchor` pose는 도구를 안정적으로 거치한다.
5. 가게 밖에 수거 구역 1개를 배치한다. 배송 지점·개봉 자리·도구 거치대·음료 수거함과 겹치지 않게 한다. CollectionBounds의 Z는 놓인 휴대물의 primitive bounds 중심을 포함한다.
6. 기존 `BP_TrashBin` level instance를 제거한다. 해당 external package 삭제만 기록하고 BP/source를 삭제하지 않는다.
7. 지정한 각 external actor package만 저장하고 DefaultMap.umap 및 allowlist 밖 package 변경이 없는지 확인한다.

## Compile·Save·재로드

- 각 신규 BP의 property/component/asset 연결을 마친 뒤 warnings-as-errors Compile·Data Validation·개별 Save를 수행한다. 기존 수정 BP/WBP도 개별 Compile·Save한다.
- Q1 A 및 Pawn 여유 삭제 property 로드 경고와 예상 밖 오류를 구분한다. Fatal·Serial size mismatch·asset Failed to load면 즉시 중단하고 구현 단계로 보고한다.
- 새 Editor 프로세스에서 부모·native subobject·TiedBagClass/LitterClass·root scale·held pose·SpawnFloor·RMB widget을 readback한다. 저장 후 package clean을 확인한다.
- unsupported MCP 조작은 `.md/USER_UNREAL.md`에 exact 작업·재개 조건을 기록한다. Unreal MCP 단계에서는 Computer Use로 우회하지 않는다.

## 대표 PIE 검증

- 빈손으로 쓰레기 조준: `쓰레기`, E 동작 없음, LMB 이유 `집게가 필요합니다`. Pawn/물건 이동을 막지 않고 Visibility trace는 쓰레기에서 멈춘다(TRSH-006/020/026).
- E로 거치대의 집게를 가져와 쓰레기에 LMB 3회: 매 press 1개, `줍기`, 조준 무관 요약 `봉투 3/20`. LMB hold는 추가 수집하지 않는다(TRSH-007/008/015/024).
- 선반·냉장고·빈 공간 각각을 조준하고 RMB: `봉투 묶기`, 앞 바닥에 봉투 1개, 집게는 손에 남고 `봉투 0/20`. 해제·Canceled는 추가 봉투를 만들지 않는다. 빈 봉투는 `봉투가 비어 있음`(TRSH-010~012/022).
- 좁은 벽 앞에서 묶기 실패: `봉투를 놓을 공간이 없음`, 기존 개수 그대로. LMB 입력 중 RMB는 무시한다(TRSH-021).
- 20개 수집 뒤 다음 쓰레기: `봉투 가득 참`, 쓰레기/개수 무변화(TRSH-009).
- 집게를 거치·G drop·낙하 복구해 개수 유지와 exact slot 복귀를 확인한다(TRSH-013/023).
- 봉투에 E: `쓰레기봉투 (n개)`와 들기. G drop 후 count/root scale 유지. 수거 구역 안에 놓고 `bathhouse.Debug.TrashCollection.CollectNow` 실행하면 제거된다(TRSH-014, COLL-001).
- 수거 구역의 배송 상자·빈/채운 품목 박스·Discardable 설비 아이템·봉투는 내용과 함께 소멸한다. 지갑 불변; held 물건·락커 아이템·열쇠·집게·걸레·배치 설비·바닥 수건·쓰레기는 남는다(COLL-002~004/008).
- bounds 바깥 중심에 걸친 물건은 남고 안쪽 primitive 중심은 제거된다. 기본 300초·instance 60초, pause 중 정지를 확인한다(COLL-005/007/009).
- 손님 0명 구역에서 발생 없음, 2명 체류 시 평균 간격 60초(쓰레기 평균 120초/인); 얼룩은 기존 SpawnIntervalSeconds/인. 구역 이탈은 다음 update부터 적용된다(TRSH-001/002/018/019).
- 배치 구역 안 탈의실·욕실 바닥에서 손님이 있을 때 쓰레기·물 얼룩이 실제로 생긴다. 겹친 두 생성 구역과 Visibility 전용 query box가 발생을 막지 않아야 한다(TRSH-001/018/019).
- 설비·놓인 물건 위나 겹치는 자리, 욕탕 안, 벽 바로 옆에는 생기지 않는다. 벽에서 종류별 반경 R 밖의 자리는 허용한다(TRSH-004/030).
- 손님·플레이어 발밑이나 바로 옆에도 생길 수 있다. 넘어진 손님의 ragdoll도 생성 자리를 막지 않는다(TRSH-029).
- 허용 경사와 같은 floor mesh의 1cm 단차에서 생성되고, 다른 mesh의 턱은 막힘으로 유지된다. kind별 최대·간격과 쓰레기/얼룩 상호 겹침 허용을 확인한다(TRSH-003~005/025).
- 쓰레기·청소 중 얼룩의 가장자리와 겹치도록 신규 배치/재배치를 확정하면 제거된다. 떨어진 것은 남고 preview·실패·취소는 제거하지 않는다. 걸레는 mopping을 유지한다(TRSH-016/017/027).
- 쓰레기가 앞에 있는 얼룩은 걸레 진행 없음, 줍기 후 얼룩 조준·진행 회복. 렌치·걸레·배송 상자 기존 자체 LMB 동작과 빈손/박스 RMB Take를 확인한다(TRSH-026/028).

## 결과 문서

- 실제 authoring을 수행한 단계만 `.md/Unreal/FacilitySystem.md`, `ShopSystem.md`, `InteractionUISystem.md`, `WorldSystem.md`와 필요 시 `0_UNREAL.md`에 저장·재로드로 확인한 현재 상태를 반영한다. Cleaning 정본을 새로 분리할지는 문서 크기 정책에 따른다.
- exact asset/actor package·parent·값·검증 결과와 미완료를 `.md/PROMPT_INTEGRATION_REVIEW.md`에 인계한다. 실행하지 않은 PIE 관찰을 통과로 쓰지 않는다.
