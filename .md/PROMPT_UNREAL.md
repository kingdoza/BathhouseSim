# Unreal 작업 프롬프트 — 서비스 4단위

## 단계와 정본

- C++ 구현 다음의 Editor authoring 단계다. `.md/AGENT_WORKFLOW.md` → `.md/AGENT_UNREAL_MCP.md`를 읽는다.
- 기능: `.md/PROMPT_ARCHITECTURE.md` MASS-001~015, REST-001~006, SCRB-001~020, SVC4-001~003.
- 설계: `.md/Architecture/ServiceAmenitySystem.md` 전체. 코드/검증 인계는 `.md/PROMPT_REVIEW.md`.
- 기존 authoring: `.md/Unreal/0_UNREAL.md`, Service·Facility·Placement·InteractionUI·Shop·World·Cleaning 문서를 읽고 실제 asset과 대조한다.
- 신규 Blueprint/UI/Definition/Level 연결이 필요하다. C++ headless 통과를 PIE 통과로 간주하지 않는다.
- Source·Config·Architecture·입력 프롬프트는 변경하지 않는다. MCP 미지원 작업은 `USER_UNREAL.md`에 exact 작업으로 남긴다. Computer Use로 우회하지 않는다.

## 저장 allowlist

아래만 개별 저장한다. Content 전체 Save/Save All, 무관 asset 재저장은 금지한다.

| exact asset path | Parent Class / 역할 |
|---|---|
| `/Game/Bathhouse/Blueprints/Service/BP_MassageChair` | `/Script/BathhouseSim.MassageChairActor` |
| `/Game/Bathhouse/Blueprints/Service/BP_RestBench` | `/Script/BathhouseSim.BathhouseFacilityActor` |
| `/Game/Bathhouse/Blueprints/Service/BP_Television` | `/Script/BathhouseSim.TelevisionActor` |
| `/Game/Bathhouse/Blueprints/Service/BP_ScrubTable` | `/Script/BathhouseSim.ScrubTableActor` |
| `/Game/Bathhouse/Blueprints/Service/BP_ScrubTowel` | `/Script/BathhouseSim.ScrubTowelActor` |
| `/Game/Bathhouse/Blueprints/Service/BP_ServiceTestUser` | `/Script/BathhouseSim.ServiceTestUserActor` |
| `/Game/Bathhouse/UI/WBP_ScrubFocusHud` | `/Script/BathhouseSim.ScrubFocusHudWidget` |
| `/Game/Bathhouse/Blueprints/Game/BP_BathhouseHUD` | `/Script/BathhouseSim.BathhouseHUD`; widget class 연결 |
| `/Game/Bathhouse/Blueprints/Combat/BP_MonkeyWrench` | `/Script/BathhouseSim.MonkeyWrenchActor`; 선택적 수리 표현 |
| `/Game/FirstPersonCharacter/BP_FirstPersonCharacter` | `/Script/BathhouseSim.FirstPersonCharacter`; native subobject 확인 후 필요 시 compile/save |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_MassageChair` | `/Script/BathhouseSim.FacilityPlacementDefinition` |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_RestBench` | 같은 class |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Television` | 같은 class |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ScrubTable` | 같은 class |
| `/Game/Bathhouse/Data/Shop/DA_ShopCatalog` | `/Script/BathhouseSim.ShopCatalog`; 기존 상품 유지 + 4개 |

- `/Game/Maps/DefaultMap`: 신규 `BP_ScrubTowel` 1개와 기존 `/Game/Bathhouse/Blueprints/Interaction/BP_PhysicalCarryFixedSlot` 1개만 배치한다. fixed-slot asset exact path는 Asset Registry로 먼저 확인한다.
- 신규 두 instance의 **실제 World Partition external actor package 경로**를 조사·기록한 뒤 그 두 package만 저장한다. `DefaultMap.umap`·기존 external actor는 저장하지 않는다. 도구가 정확히 저장하지 못하면 미완료 인계한다.
- `/Game/Bathhouse/Blueprints/Facility/BP_Shower`는 기존 슬롯/비품 component 검증만 하며 저장 대상이 아니다.
- 대표 설비 설치와 테스트 인형·현금은 PIE transient 경로다. 레벨에 테스트 설비/인형/현금을 영구 저장하지 않는다.
- 신규 재질은 요구하지 않는다. 임시 Engine mesh와 Blueprint component Visibility로 고장/TV 표현을 만든다.

## 공통 설비 authoring

- 네 설비 Actor scale은 `(1,1,1)`. `SceneRoot` 아래 임시 body StaticMesh를 만들고 `/Engine/BasicShapes/Cube.Cube`를 지정한다.
- body만 BlockAllDynamic·Navigation on이다. footprint·PackagePhysicalRoot·slot·action/approach·커서/표현 helper는 Navigation off를 유지한다.
- `PlacementFootprint`는 NoCollision, actor local 바닥 Z=0에서 시작한다. full X/Y는 **실제 Project Settings grid**의 양의 정수배다. 아래 초기 치수가 grid와 안 맞으면 body와 함께 조정한다.
- 설비 Blueprint에는 각 Definition을 inherited `FacilityPlacement.Definition`으로 지정한다. 모든 slot은 `BathhouseFacilitySlotComponent`, enabled이며 SCS에서 만든다.
- 바닥 primitive를 추가하거나 preview/slot helper를 collision body로 쓰지 않는다. 기존 설치·회수 payload/physics는 C++가 맡는다.

| BP | FacilityType / slot | 임시 body / footprint 초기값(cm) | Class Default |
|---|---|---|---|
| MassageChair | MassageChair / `CustomerSlot` 1개 | body loc `(0,0,50)`, scale `(.8,.8,1)`; extent `(40,40,50)`, loc Z50 | UseSeconds60, UseFee3000, BreakChancePercent10, RepairSeconds3 |
| RestBench | RestBench / `SeatSlot0~2` 3개 | body loc `(0,0,25)`, scale `(3.6,1,.5)`; extent `(180,50,25)`, loc Z25 | 기존 bEnabled/SelectionWeight 계약 유지 |
| Television | Television / **0개** | body loc `(0,0,90)`, scale `(.8,.4,1.8)`; extent `(40,20,90)`, loc Z90 | 시작/재설치 전원 false(native transient) |
| ScrubTable | ScrubTable / `CustomerSlot` **1개** | body loc `(0,0,45)`, scale `(2,.8,.9)`; extent `(100,40,45)`, loc Z45 | ScrubFee20000, WaitLimitSeconds90, RequiredRubDistanceCm3000, RubCmPerInputUnit1, ExitSearchRadiusCm100, BlendIn .35, BlendOut .25 |

- 의자 slot action은 앉는 위치, 평상 action 3개는 서로 다른 좌석(예: X=-120/0/120), 세신 action은 누움 위치다. approach는 body 밖 바닥에서 접근하도록 정한다. 모든 slot 수를 Data Validation으로 확인한다.
- `BP_MassageChair`: NoCollision·Navigation off `BrokenLabel`(TextRender, “고장”) 또는 임시 표시 mesh를 추가한다. 기본 hidden. `Event BeginPlay`에서 native `IsBroken()`을 읽어 초기 Visibility를 적용하고(회수 후 고장 재설치 포함), `OnBrokenStateChanged(Broken)`에서 Visibility만 반전한다. `OnCoinBalanceChanged`는 선택적 표현이며 돈/고장 판정은 graph에 쓰지 않는다.
- `BP_Television`: NoCollision·Navigation off `ScreenOnVisual`을 기본 hidden으로 추가한다. `OnPowerChanged(PoweredOn)`에서 화면 표현 Visibility만 변경한다. 기본 꺼진 화면과 켜진 화면을 PIE에서 구별할 수 있게 한다.
- 렌치 `OnRepairActiveChanged(Active)`는 선택적 표현만 연결한다. 기존 WorldMesh·MeleeAttack·공격 curve·피격 timing은 유지한다. 수리 진행/commit은 Blueprint에 두지 않는다.

## 세신대 여섯 native component

이름과 class를 유지하고 새 동명 SCS component를 만들지 않는다.

| component | class / authoring |
|---|---|
| `ScrubCamera` | CameraComponent. 세신 표면을 위에서 비스듬히 본다. 초기 loc `(0,-170,240)`, pitch -45/yaw90/roll0; 실제 화면을 보고 조정 |
| `ScrubArea` | BoxComponent. loc `(0,0,95)`, extent `(90,35,1)`, rotation0; local XY가 이동 범위, +Z가 표면 법선. NoCollision·Navigation off |
| `ScrubCursor` | StaticMeshComponent. 임시 Cube, scale `(.18,.12,.03)`, NoCollision·Navigation off·기본 HiddenInGame. native가 area 표면에 이동시키므로 graph로 transform을 덮어쓰지 않는다 |
| `ScrubExitPoint` | SceneComponent. 초기 loc `(0,-120,0)`, yaw90. **발바닥 위치**다. 바닥 위 body 밖 빈 위치로 정한다 |
| `CashOfferPoint` | SceneComponent. 초기 loc `(0,-160,110)`; 현금이 바닥·body에 박히지 않는 곳 |
| `CashStandPoint` | SceneComponent. 초기 loc `(0,-180,0)`, yaw90; 인형이 옆에서 내미는 위치 |

- `CashOfferClass = /Game/Bathhouse/Blueprints/Economy/BP_BathhouseCashPayment.BP_BathhouseCashPayment_C`.
- camera가 `ScrubArea` 네 모서리와 `ScrubCursor`를 모두 보는지 실제 포커스 화면으로 확인한다. cursor가 몸체에 가려지면 area/camera 위치를 authoring으로 조정한다.
- 이탈점의 world 발 Z가 DefaultMap 바닥 Z=0이고 capsule 공간이 비어 있는지 확인한다. 높은 이탈점은 떨어질 수 있으며 native가 바닥 높이를 보정하지 않는다.
- 현금/인형 두 점이 플레이어 이탈 공간과 겹치지 않고 E 조준 가능한 위치인지 확인한다.
- 대기·게이지·사용자·예약·현금 spawn·수금·이탈 탐색을 graph로 재구현하지 않는다.

## 때수건·테스트 인형·Level

- `BP_ScrubTowel.WorldMesh`: 임시 Cube, root Relative Scale `(.25,.18,.03)`(25×18×3cm). PhysicsActor, QueryAndPhysics, Pawn Ignore, CCD. HeldTransform scale1, location/rotation만 조정한다. 시작 physics는 free-world 또는 고정 슬롯 native가 적용한다.
- `BP_ServiceTestUser.WorldMesh`: 임시 Cube, NoCollision·Navigation off, 물리 false·Tick 없음. 테스트 명령은 이 BP class를 사용하며 Editor authoring 전에는 native Cube 인형으로 대체한다.
- DefaultMap 기존 손님/상점/청소/거치대와 충돌하지 않는 빈 위치를 조사한 뒤 때수건·전용 fixed slot 한 쌍을 둔다. slot `AssignedItem`은 새 **그 인스턴스**를 참조하고 `bStartOccupied=true`다. identical towel을 복제 연결하지 않는다.
- fixed slot 및 때수건 Actor의 scale과 anchor를 readback한다. 거치·take 후 원래 크기가 유지돼야 한다.
- 플레이 중 내려놓기·레벨 밖 복구·수거 제외는 native다. 버리기 tag/interface를 때수건에 추가하지 않는다.

## HUD·상점

- `WBP_ScrubFocusHud`: root panel 아래 필수 `ScrubGaugeBar`(ProgressBar), `ScrubWaitText`(TextBlock). 화면 하단 중앙에 게이지와 “대기 72초”가 기존 HUD와 겹치지 않도록 둔다.
- WBP는 layout/style만 한다. native NativeTick이 ratio·남은 시간을 읽는다. `TickFrequency=Auto`, widget host Visibility=HitTestInvisible을 유지하고 graph에서 Collapsed/Hidden으로 tick을 끄지 않는다. native가 Active 밖에서 RenderOpacity0으로 표시를 지운다.
- `BP_BathhouseHUD.ScrubFocusHudWidgetClass = WBP_ScrubFocusHud_C`. 기존 InteractionPrompt/Money/ShopNotice class는 유지한다.
- Character의 inherited `PlayerScrubFocus`가 정확히 하나인지 확인한다. 기존 모든 input action/default subobject·컴퓨터 연결은 유지한다. 새 Input Action/Mapping은 없다.
- 네 Definition: `StableId=Facility.MassageChair/RestBench/Television/ScrubTable`, 양쪽 태그 `Facility.Placeable`·`Facility.Discardable`, `LockerSlotCount=0`, 각 신규 `PlacedFacilityClass`.
- `RecoveryItemClass=/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem.BP_PlaceableFacilityItem_C`, `RecoveryItemMesh=None`(기존 fallback). 기존 Definition의 recovery/drop/회전 기본값을 확인해 동일 계약으로 지정한다.
- `DA_ShopCatalog`: 기존 상품을 보존한 채 ProductId/DisplayName/Price를 `MassageChair/안마의자/60000`, `RestBench/평상/15000`, `Television/TV/25000`, `ScrubTable/세신대/20000`으로 append한다. bForSale=true, PlacementDefinition은 해당 신규 정의, ItemBoxDefinition=None.

## Compile·저장·재로드

1. 신규 설비는 slot/mesh/Definition 연결을 모두 만든 뒤 compile한다(미완성 구조는 native validation 실패가 정상). compile warnings_as_errors와 Data Validation을 통과시킨다.
2. allowlist asset만 개별 저장한다. 실패/예상 밖 dirty package는 원인을 먼저 보고한다.
3. 새 Editor 프로세스로 디스크 재로드한다. Parent Class, SCS/native component 수·이름, defaults, BindWidget, catalog 4개, fixed-slot AssignedItem, external package 경로를 다시 확인한다.
4. 기존 BP_Shower 2slot/2display space/manager, BP_MonkeyWrench 공격 component, Character computer component·입력, HUD 기존 widget 연결을 비교한다.
5. 아래 PIE를 수행하고 실행하지 못한 항목을 미검증으로 남긴다. test용 BreakChance=100을 저장 상태에 남기지 않는다.

## 대표 PIE 절차와 관찰

- SVC4-001/002: 컴퓨터 상점에서 네 상품/가격 확인 → 구매·배송·LMB 개봉 → 각 아이템 E 들기·배치 LMB·Q 회수·재배치. 별도 구매 아이템을 수거 구역에 놓고 다음 수거 tick에 제거되는지 확인한다.
- TV REST-001~003/006: 빈손 body 조준 E로 켜기/끄기. 때수건이나 다른 물건을 들면 “빈손으로 켜고 끌 수 있음”. 켜진 채 Q 취소는 유지, Q 확정→재설치는 꺼짐.
- 평상 REST-004/005: body를 조준해 `bathhouse.Debug.Service.SpawnTestUser` 세 번. 좌석 3개가 점유되고 네 번째는 실패, Q 회수도 실패. `RemoveTestUser`로 비운다.
- 안마의자 MASS-001/010/011: 조준 E에 “모인 돈 없음”. `SpawnTestUser` → 60초 후 +3,000. 중간 30초에 `KnockdownTestUser` → Reserved·요금 없음; `StandUpTestUser` 후 다시 60초다. `RemoveTestUser`는 요금 없이 비운다.
- MASS-002/009/014: 여러 정상 이용으로 9,000을 모으고 새 인형 이용 중 E 수금한다. 들고 있는 물건에 상관없이 +9,000 한 번, 빠른 반복 E는 추가 지급이 없다. 이용 중 Q는 불가다.
- MASS-003~008/013: 테스트 세션의 BreakChancePercent만 100으로 조정해 항상 고장 관찰. 새 이용 실패, 렌치 조준 LMB “수리” Hold 3초·피격 없음. 1.5초에 release/조준 이탈은 초기화. 정상 의자 LMB는 휘두르기. 고장+3,000 의자 Q 회수는 돈 지급, 아이템 “고장” 요약, 재설치 고장 유지. E 수금으로 먼저 비운 의자도 회수 가능하다.
- MASS-012/015·Q67: 고장 의자에 빈손/품목 박스/집게+봉투/때수건 조준 → LMB “몽키스패너가 필요합니다”. 집게로 선반/세탁기 조준 시 이유만 보이고 강조·프리뷰·뚜껑은 꺼짐. 집게 쓰레기는 “줍기”, RMB “봉투 묶기” 유지.
- SCRB-001~004: 세신대 body 조준 `SpawnTestUser`. 빈손/다른 물건은 E “때수건이 필요합니다”; 때수건 들면 “세신대 · 대기 …초 · 0% / E 세신”. 빈 세신대는 “세신할 손님 없음”.
- SCRB-005~007/011/016: 때수건 들고 E → camera blend·cursor 중심. entry E release/계속 누름으로 이탈하지 않음. 마우스만/LMB 정지는 gauge 불변, LMB+이동만 증가, 경계 밀기는 불변. 이동·점프·sprint·F·G·Q·RMB 차단, E/ESC만 이탈. 40% 이탈·재진입 시 이어지고 때수건은 손에 남음.
- SCRB-008/014/015: 완료 자동 이탈 → 인형은 현금 점으로 이동, 세신대는 비어 새 인형 사용 가능. 현금 조준 E로 +20,000 한 번·인형 제거. 앞 현금은 세신대 회수 뒤에도 남음.
- SCRB-009/018/020: 진입 안 하고 90초 또는 진행 60%에서 90초 만료 → 무요금·인형 제거·자리 비움/자동 이탈. `KnockdownTestUser` → gauge0·Reserved, `StandUpTestUser` → 90초 재시작.
- SCRB-010: 이탈점에 player-blocking 물체를 두고 E/ESC → 근처 빈 자리 탐색. 테스트 장애물은 영구 저장하지 않는다.
- SCRB-013/017/019: 전용 거치대 E take/store·wrong-slot 거부·G held-pose drop·레벨 밖 복구. 수거 구역에 둬도 때수건은 남음. 때수건으로 쓰레기 조준 시 LMB “집게가 필요합니다”.
- SVC4-003: 네 명령은 non-shipping에서만 등록된다. Shipping에서 테스트 인형 호출 경로가 없는지는 소스 guard를 확인하고, 이 단계에서 Shipping 실행을 하지 않았다면 미검증으로 기록한다.
- 감도 결정: 기본 RequiredRubDistanceCm3000/RubCmPerInputUnit1로 실제 마우스 움직임과 완료 시간을 관찰한다. 작은/큰 화면에서도 경계 clamp와 LMB 조건을 유지하며 거리 임계값을 조정하고 최종 PIE 체감값을 readback·정본에 기록한다. 감도 조정 때문에 fee/wait/input 규칙을 바꾸지 않는다.
- 기존 회귀: 컴퓨터 E/ESC/포인터·이탈, 집게 줍기/묶기, 대걸레·렌치 공격, 수건 이동/뚜껑·진열 cue, placement/recovery·상점·money HUD.

## 정본 갱신과 결과

- 저장·fresh-process 재로드로 확인한 내용만 `.md/Unreal/ServiceSystem.md`, `FacilitySystem.md`, `PlacementSystem.md`, `InteractionUISystem.md`, `ShopSystem.md`, `WorldSystem.md`에 반영한다. 서로 링크하고 기존 시스템의 책임은 유지한다.
- 새로운 별도 Unreal 문서로 분리한다면 `.md/Unreal/0_UNREAL.md`를 routing entry로 갱신한다. 크기 규칙을 따른다.
- `.md/PROMPT_INTEGRATION_REVIEW.md`: 저장 allowlist와 실제 external package, validation/compile/reload 결과, 시나리오별 PIE 결과, 결정된 거리·카메라/커서·이탈 authoring 값, 미검증을 적는다.
- native 버그/설계 불일치면 Blueprint 우회 대신 구현/아키텍처 단계로 돌려보낸다.
