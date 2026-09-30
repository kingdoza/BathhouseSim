# Unreal 작업 프롬프트 — 서비스 2단위

## 현재 단계 — 수건 cue 갱신 재작업

현재 인계는 수건 cue 재작업의 코드 리뷰 뒤 PIE 확인이다. AGENT_WORKFLOW.md → AGENT_UNREAL_MCP.md와 현재 PROMPT_REVIEW.md를 읽는다. 코드 리뷰 승인 자체를 뜻하지 않는다.
입력: [재작업](PROMPT_IMPLEMENTATION_R.md), [구현 프롬프트 맨 앞 재작업 절](PROMPT_IMPLEMENTATION.md), [수건 cue 재계산 정본](Architecture/TowelSystem.md), [Interaction query 정본](Architecture/InteractionSystem.md).
기존 2단위 C++와 Editor 작업물을 유지했다. 이번 C++ 재작업에서는 Content·Config·Level·external actor를 수정·저장하지 않았다. 시작 시 Content 변경 13개의 status·SHA256 기록은 Saved/ImplementationUnit2/CueRefresh/content_before.json이며 끝난 뒤 동일함을 확인한다.
이번 저장 allowlist는 **비어 있다**. 새 component·BP 연결·asset migration은 없다. PresentationRevision은 Blueprint 비노출 query 값이며 asset 저장 대상이 아니다. 최신 DLL을 읽는 Editor에서 아래 cue PIE만 추가 확인한다. 빌드·DefaultMap 로드·자동화 결과는 PROMPT_REVIEW.md를 따른다.
아래 기존 2단위 authoring 계약과 PIE 항목은 인계 기록으로 보존한다. 이미 존재하는 DA·BP·상품을 다시 생성하거나 기존 작업물을 원복하지 않는다. 현재 Editor 상태는 Unreal/0_UNREAL.md와 연결된 정본을 따른다.

## 이번 재작업 PIE 관찰

- 선반·사용 수건통·대기 세탁기·대기 건조기 각각에서 같은 대상을 계속 조준한다. 바구니와 대상이 빈 상태·가득 참 경계에 닿지 않도록 수량을 준비한다.
- 선반·세탁기·건조기에 맞는 상태의 바구니를 들고 LMB를 누른 채 연속 넣는다. 매 1장 직후 프리뷰는 다음 자리, 외곽선은 새 맨 위로 따라가야 한다. 실제 수건과 프리뷰가 같은 자리에 남아 겹치면 실패다.
- 네 대상에서 RMB를 누른 채 연속 뺀다. 매 1장 직후 외곽선은 새 맨 위, 프리뷰는 방금 빠진 자리로 따라가야 한다. 사용 수건통은 Apply가 없으므로 프리뷰 없이 Take 외곽선만 관찰한다.
- 플레이어가 선반을 조준한 채 실제 손님이 수건을 가져가게 한다. 늦어도 다음 query tick에 외곽선이 새 맨 위, 프리뷰가 새 Count의 다음 자리로 이동해야 한다.
- 조준 종료·suppression·기계 작동 시작의 숨김/닫힘, 처리 중 표시 없음, 기존 TargetName·행동명·이유가 유지되는지도 확인한다. query revision 때문에 연속 입력이 중단되거나 뚜껑이 매 장 다시 처음부터 열리면 실패다.
- headless는 실제 trace/held-use 경로의 transform을 검사했다. 반투명·외곽선 화면 모양, lid animation, authored mesh bounds, 실제 손님 이동은 PIE 관찰값을 별도로 기록한다. PIE 뒤 패키지를 저장하지 않는다.

## 기존 2단위 authoring 계약(보존 기록)

정본: [ServiceFacilityDisplaySystem.md](Architecture/ServiceFacilityDisplaySystem.md), [TowelSystem.md](Architecture/TowelSystem.md), [TowelPresentationSystem.md](Architecture/TowelPresentationSystem.md), [서비스 기능 계약](PROMPT_ARCHITECTURE.md).
최초 asset 백업과 F1~F4 검증은 Saved/MigrationBackup/20260930_service_unit2/ 및 Saved/ImplementationUnit2/Rework/에 유지돼 있다. 아래 저장 범위·authoring 순서는 당시 계약이며 이번 cue 재작업의 저장 권한을 추가하지 않는다.

## 기존 authoring 저장 allowlist와 parent

아래 패키지만 개별 저장한다. 필요 없는 기존 asset은 상속 확인만 하고 저장하지 않는다. 새 Material·Config·Level·external actor package는 allowlist에 없다.

| 정확한 package path | 생성/수정 계약 |
|---|---|
| /Game/Bathhouse/Data/Service/DA_ServiceItem_HairDryer | 신규 ServiceItemDefinition, 헤어드라이기, 정원 2·횟수 0 |
| /Game/Bathhouse/Data/Service/DA_ServiceItem_SkinLotion | 신규 ServiceItemDefinition, 스킨로션, 정원 6·횟수 10 |
| /Game/Bathhouse/Data/Service/DA_ServiceItem_CottonSwab | 신규 ServiceItemDefinition, 면봉, 정원 6·횟수 30 |
| /Game/Bathhouse/Data/Service/DA_ServiceItem_Comb | 신규 ServiceItemDefinition, 빗, 정원 12·횟수 0 |
| /Game/Bathhouse/Data/Service/DA_ServiceItem_Shampoo | 신규 ServiceItemDefinition, 샴푸, 정원 6·횟수 30 |
| /Game/Bathhouse/Data/Service/DA_ServiceItem_BodyWash | 신규 ServiceItemDefinition, 바디워시, 정원 6·횟수 30 |
| /Game/Bathhouse/Blueprints/Service/BP_Vanity | 신규 parent /Script/BathhouseSim.BathhouseFacilityActor |
| /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Vanity | 신규 FacilityPlacementDefinition |
| /Game/Bathhouse/Blueprints/Facility/BP_Shower | 기존 parent /Script/BathhouseSim.BathhouseFacilityActor 유지 |
| /Game/Bathhouse/Blueprints/Towel/BP_Washer | 기존 parent /Script/BathhouseSim.TowelProcessingMachineActor 유지 |
| /Game/Bathhouse/Blueprints/Towel/BP_Dryer | 기존 parent /Script/BathhouseSim.TowelProcessingMachineActor 유지 |
| /Game/Bathhouse/Blueprints/Towel/BP_CleanTowelStack | 기존 parent /Script/BathhouseSim.CleanTowelStackActor, native cue 상속 확인 |
| /Game/Bathhouse/Blueprints/Towel/BP_UsedTowelBin | 기존 parent /Script/BathhouseSim.UsedTowelBinActor, native cue 상속 확인 |
| /Game/Bathhouse/Blueprints/Service/BP_DrinkFridge | 기존 parent /Script/BathhouseSim.DrinkFridgeActor, native manager 상속 확인 |
| /Game/Bathhouse/Blueprints/Service/BP_ItemBox | 기존 parent /Script/BathhouseSim.ItemBoxActor, 종류별 preview 값 확인·원복 |
| /Game/Bathhouse/Data/Shop/DA_ShopCatalog | 기존 9상품 유지 + 이번 7상품 추가 |

## 품목 여섯 종과 박스

ItemId는 각각 HairDryer, SkinLotion, CottonSwab, Comb, Shampoo, BodyWash이며 DisplayName은 위 표의 한글 이름을 쓴다. DisplayCategories는 비워 두고 SaleValue=0으로 둔다.
임시 mesh 출처는 `/Engine/BasicShapes/Cube.Cube`, `/Engine/BasicShapes/Cylinder.Cylinder`, `/Engine/BasicShapes/Plane.Plane` 세 개뿐이다. 새 Mesh·Material을 만들지 않으며 material은 해당 mesh 기본값 또는 기존 프로젝트 material만 쓴다.
HairDryer·Comb ItemMesh는 Cube, SkinLotion·CottonSwab·Shampoo·BodyWash ItemMesh는 Cylinder로 지정한다. DisplayMesh는 비워 ItemMesh를 재사용한다. DisplayOffset은 균일하지 않은 placeholder 보정이 필요한 경우 양수 scale로 맞추며 BoxItemOffset과 설비 자리 bounds를 각각 검증한다. 기존 BananaMilk 정의는 수정하지 않는다.
BoxItemOffset은 모든 박스 자리에 공통 적용하며 기본 Identity다. 회전·피벗·크기 보정은 이 값에, 개별 자리 위치는 BoxSlotTransforms에 둔다. 최종 instance는 BoxItemOffset * BoxSlotTransforms[i]다. finite·양수 scale을 지키고 공통 scale은 균일값을 우선한다.

| 품목 | BoxSlotTransforms 수·배열 순서 | 검사 |
|---|---|---|
| HairDryer | 2, 2열×1행 | 두 개가 겹치지 않게 놓이고 끝부터 빠짐 |
| SkinLotion | 6, 3열×2행 | 병 여섯 개·공통 보정 적용 |
| CottonSwab | 6, 3열×2행 | 통 여섯 개·공통 보정 적용 |
| Comb | 12, 4열×3행 | 빗 열두 개·회전 보정 적용 |
| Shampoo | 6, 3열×2행 | 병 여섯 개·공통 보정 적용 |
| BodyWash | 6, 3열×2행 | 병 여섯 개·공통 보정 적용 |

BP_ItemBox의 기존 크기와 ContentsVisual의 부모 scale 상쇄를 유지한다. 현재 정본의 BoxMesh scale=(0.4,0.3,0.1), ContentsVisual scale=(2.5,3.3333,10)을 실제 readback과 대조한다.
각 DA를 BP_ItemBox.EditorPreviewKind에 지정하고 EditorPreviewCount를 0·1·정원으로 바꿔 OnConstruction preview와 mesh bounds를 확인한다. 여섯 품목 모두 실제 박스 크기 안에 들어가야 한다. 들어가지 않으면 박스를 확대하거나 Content 수정을 계속하지 말고 기능 명세로 보고한다. preview 값은 원래 값으로 되돌린다.

## 화장대 구성

FacilityType=Vanity. SceneRoot 자식 `VanityBody`는 Cube.Cube(상대 위치 `(0,0,50)`, 회전 0, scale `(0.6,1.2,1)`), `MirrorVisual`은 Plane.Plane(위치 `(-25,0,140)`, Pitch 90/Yaw 0/Roll 0, scale `(0.8,1.2,1)`)를 시작값으로 쓴다. 이는 임시 도형이며 새 반사 material은 만들지 않는다.
VanityBody는 BlockAllDynamic·Navigation 활성, MirrorVisual은 NoCollision·Navigation off다. footprint 위치 `(0,0,90)`, extent `(40,70,90)`로 grid 20cm 배수를 맞추고 Navigation/충돌을 readback한다. 품목 자리 외형까지 footprint 안에 두며 임시 도형 bounds가 초과하면 저장 전 조정한다.
ServiceDisplayManagerComponent 하나를 Actor component로 추가한다(attachment·SceneRoot 부모 설정 없음): RequiredCustomerSlotCount=1, bConsumeOnCustomerUseStart=true, CustomerUseSeconds=20.
DisplayFacilityTargetComponent 하나: FacilityDisplayName=`화장대`(현지화 가능한 FText 필수), QueryOnly, Visibility Block, 다른 trace 응답 Ignore, Navigation off. 시작 위치 `(0,0,100)`, BoxExtent `(45,75,100)`에서 거울·상판·몸체·품목 전체 외형의 실제 bounds를 덮는지 확인한다. 몸체 mesh의 앞면 조준이 router보다 먼저 막히지 않는지 여러 방향에서 확인한다.

| DisplaySpaceComponent | SpaceIndex | FixedKind | SlotTransforms 수 |
|---|---:|---|---:|
| DryerGroup | 0 | DA_ServiceItem_HairDryer | 2 |
| LotionGroup | 1 | DA_ServiceItem_SkinLotion | 4 |
| SwabGroup | 2 | DA_ServiceItem_CottonSwab | 4 |
| CombGroup | 3 | DA_ServiceItem_Comb | 6 |

네 공간은 SceneRoot 자식이며 TargetMode=FacilityRouted, NoCollision, Navigation off다. 시작 위치는 SpaceIndex 순 `(0,-45,105)`, `(0,-15,105)`, `(0,15,105)`, `(0,45,105)`이다. 묶음 중심을 서로 떨어뜨려 빈 박스 조준으로 네 그룹을 구별할 수 있게 배치한다. 자리 배열은 앞부터 채우며 물품이 겹치지 않는다. FixedKind 중복·index 누락을 금지한다.
BathhouseFacilitySlotComponent는 정확히 하나다. ApproachOffset·FacingRotation을 서서 이용하는 위치와 거울 방향에 맞춘다.
DA_FacilityPlacement_Vanity: StableId=Facility.Vanity, Facility.Placeable·Facility.Discardable 태그, PlacedFacilityClass=BP_Vanity_C, RecoveryItemClass=/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem.BP_PlaceableFacilityItem_C, LockerSlotCount=0. BP의 FacilityPlacement.Definition에 연결한다.
화장대 손님 루틴은 추가하지 않는다. 20초 값은 이후 루틴이 읽을 설정이며 이번 PIE는 console BeginUse/EndUse로 소모를 확인한다.

## 기존 샤워기

BP_Shower의 parent·slot 두 개·Definition·footprint·몸체를 유지한다. Manager 하나(RequiredCustomerSlotCount=2, bConsumeOnCustomerUseStart=true, CustomerUseSeconds=0), 전체 외형을 덮는 router 하나(FacilityDisplayName=`샤워기`, 현지화 가능한 FText 필수)를 추가한다. manager는 Actor component이며 attachment를 설정하지 않는다. 두 router 모두 이름이 비거나 공백뿐이면 Validation 오류이므로 BP 기본값으로 채운다.
샴푸 SpaceIndex=0, 바디워시=1, 각각 FixedKind 연결·SlotTransforms 2개·FacilityRouted·NoCollision·Navigation off. 두 그룹 중심을 구별 가능한 위치에 놓는다.
DefaultMap의 기존 샤워기 instance가 새 BP component를 상속했는지 read-only로 확인한다. 레벨 instance override로 재고를 설정하지 않는다. PIE 시작 시 샴푸 0/2·바디워시 0/2여야 한다. 저장 대상은 BP_Shower뿐이며 Level·external actor는 저장하지 않는다.

## 수건과 냉장고 상속

Washer/Dryer의 기존 Inventory·TransferPort·MachineControl·TowelPresentationVisual·MachineKind·처리 시간·Definition은 유지한다.
추가 native subobject: DisplayCue는 TowelPresentationVisual 자식이다. LidPivot은 SceneRoot 자식, LidMesh는 LidPivot 자식이며 NoCollision·Navigation off다. LidPresentation은 OpeningPresentationComponent이고 native Configure가 pivot을 연결한다. 같은 이름의 SCS component를 중복 생성하지 않는다.
Washer/Dryer 모두 `LidMesh`에 `/Engine/BasicShapes/Cube.Cube`를 지정한다. 기존 `MachineVisual`의 asset은 교체하지 않는다. Pile/프리뷰를 가리면 기존 Cube의 transform만 조정한다. 임시 관찰용 시작값: MachineVisual 위치 `(0,0,4)`/scale `(0.6,0.5,0.08)`/회전 0(바닥 도형), LidPivot 위치 `(-30,0,80)`/닫힌 상대 회전 0/scale 1, LidMesh pivot 상대 위치 `(30,0,0)`/회전 0/scale `(0.6,0.5,0.02)`이다.
이 시작값에서 LidPresentation은 LocalRotationAxis=`(0,1,0)`, OpenAngleDegrees=`-90`, OpenSeconds/CloseSeconds=`0.2/0.2`다. 후면 경첩에서 위로 열리는지 확인하고 native Configure를 유지한다. PlacementFootprint와 TransferPort·MachineControl 기존 위치/충돌/Navigation은 유지하며 MachineVisual의 기존 BlockAllDynamic·Navigation 활성도 유지한다. 뚜껑·Pile helper는 NoCollision·Navigation off다.
LidPresentation.PreviewOpenPose → RestoreClosedPose로 움직임·끼임을 확인하고 닫힘 baseline으로 저장한다.
TowelPresentationVisual의 기존 MeshProfile은 유지한다. 시작 상대 위치 `(0,0,12)`, PileHalfExtent=`(22,18)`, BaseLocalOffset=`(0,0,0)`, ItemsPerLayer=5, LayerSpacing=8, MaxZJitter=2에서 실제 profile mesh bounds·정원 10이 footprint 안에 있고 열린 뚜껑에서 보이는지 확인한다. profile 외형이 넘치면 Pile 범위/mesh 표현 scale을 현재 허용된 component authoring 안에서 조정하고 정원·상태는 바꾸지 않는다. 회전 범위와 RandomSeed는 기존값을 readback 후 유지한다. PreviewState/PreviewCount 및 RebuildPreview/ClearPreview로 0·1·정원을 확인한다. 같은 RandomSeed 재빌드는 같은 배치여야 한다. 열린 뚜껑에서 내부 insert preview가 보이는지 PIE로 확인한다.
CleanTowelStack·UsedTowelBin의 native DisplayCue 한 개와 기존 TowelPresentationVisual을 확인한다. 사용 수건통은 Take 강조만 있으며 Apply 프리뷰는 없다.
DrinkFridge의 native DisplayManager 하나(RequiredCustomerSlotCount=1, 소모 false)와 기존 SCS DisplaySpace 네 개·slot 한 개를 확인한다. 공간은 SelfAim·Display.Fridge를 유지한다.
기존 preview Material·카메라 외곽선 연결·ServiceDisplaySettings를 유지한다. 새로운 Widget 계산·Blueprint inventory mutation·Customer/StateTree 로직은 추가하지 않는다.

## 상점

Products 기존 9개를 보존하고 아래 일곱 개를 추가한다. 각 상품은 definition 둘 중 정확히 하나만 지정한다. bForSale=true, ProductId 고유, DisplayName 한글이다.

| ProductId | 가격 | definition |
|---|---:|---|
| Vanity | 20000 | PlacementDefinition=DA_FacilityPlacement_Vanity |
| HairDryerBox | 10000 | ItemBoxDefinition=DA_ServiceItem_HairDryer |
| SkinLotionBox | 6000 | ItemBoxDefinition=DA_ServiceItem_SkinLotion |
| CottonSwabBox | 3000 | ItemBoxDefinition=DA_ServiceItem_CottonSwab |
| CombBox | 3000 | ItemBoxDefinition=DA_ServiceItem_Comb |
| ShampooBox | 6000 | ItemBoxDefinition=DA_ServiceItem_Shampoo |
| BodyWashBox | 6000 | ItemBoxDefinition=DA_ServiceItem_BodyWash |

## 검증 순서와 대표 PIE

1. 변경 전 exact path·parent·기존 components/defaults를 readback한다. runtime Editor가 이전 DLL이면 종료 후 새 빌드를 읽도록 재시작한다.
2. 모든 구성과 연결을 끝낸 뒤 각 BP를 warnings_as_errors로 Compile한다. DA·BP Data Validation과 SCS template readback(공간·router·slot 수)을 함께 확인한다.
3. allowlist asset만 개별 Save한다. dirty package 목록에 Level·external actor·Config가 있으면 저장하지 않는다.
4. 새 Editor 프로세스에서 디스크 재로드해 parent·native/SCS component·DA 값·상품 16개·닫힌 뚜껑 baseline을 다시 확인한다.
5. 다음 PIE를 수행하고 수치·HUD·관찰 결과와 실패를 기록한다. PIE 종료 후 Level을 저장하지 않는다.

| 시나리오 | 키·조준과 기대 결과 |
|---|---|
| 대표 SHWR→TOWL | 상점 샴푸·바디워시 박스 구입→배송 상자 E로 들고 LMB 개봉→박스 E 들기→기존 샤워기 몸체 조준 LMB 유지. 각 2병에서 멈춘다. 실제 손님 씻기 시작마다 두 품목 29/30회, 두 slot 시작 시 합계 2회 감소. 사용한 수건 바구니로 대기 세탁기 투입구 조준→뚜껑/프리뷰→LMB 한 장→그 자리에 실제 수건→RMB 한 장으로 되돌림 |
| DISP-015~017·023·024 | 빈 박스로 드라이기 쪽 RMB 유지→첫 개 후 헤어드라이기 박스→빗 쪽 조준해도 드라이기만 빠짐, 빗 수량 불변. router 밖으로 조준하면 멈추며 버튼을 떼기 전 재개 없음. 다른 품목 박스는 양 방향 “여기에 넣을 수 없는 물건” |
| VANI-001~005·011~013 | 품목 박스로 몸체 조준 LMB/RMB, 빈 박스는 가까운 그룹 강조. 정원 “가득 참”; 빈 그룹 “꺼낼 물건 없음”; 사용 중 하나만 “사용 중인 것은 꺼낼 수 없음”. 박스 없이도 네 그룹 HUD 표시, E 행동 없음 |
| VANI-006~010·014~017 | idle 화장대 조준 console bathhouse.Debug.Facility.BeginUse → 로션 첫 병 9/10. .EndUse 후 재시작마다 1회. 열 번째 시작에 앞 병 소진·새것 당김, 비품 불변. 이용 중 LMB/RMB 가능·Q 회수 불가. idle Q Hold→아이템 요약→재설치 남은 횟수 보존; 쓰레기통 E로 내용물 함께 폐기. Hold 동안 신규 예약 불가·취소 뒤 복구 |
| SHWR-001~011 | 기존 instance와 신규 설치 모두 빈 상태. 빈 박스로 바디워시 쪽 RMB→새것 LIFO. 손님 한 명 목욕 전·후 각 한 번, 두 손님 두 slot 시작 합계 두 번 소모. 비품 없어도 기존 샤워 루틴 진행. 이용 중 Q 거부, idle 회수·재설치 남은 횟수 보존 |
| F1 예약 주기·knockdown | 손님이 샤워기를 Reserve→BeginUse한 직후 샴푸·바디워시 29/30회를 기록한다. 같은 손님을 기존 전투 입력으로 knockdown→기립→샤워 재개시켜 두 값이 그대로 29인지 관찰한다. Release 뒤 목욕 후 샤워의 새 예약/시작은 28회, 다른 slot 손님 시작은 두 품목 각각 추가 1회다. StateTree·Customer 로직은 편집하지 않는다 |
| F2 보류 취소·손님 재시도 | 사용 가능한 샤워기가 하나인 상황에서 그 설비를 조준 Q Hold→손님 샤워 예약 실패·WaitForFacility→Q 해제로 취소한다. 다음 샤워기 타입 availability 알림에서 대기 손님이 실제 예약/이동을 재개하는지 관찰한다. 취소 반복은 새 알림이 없고 성공 회수는 기존 알림 1회다. 시간 경과/다른 설비 알림 때문에 우연히 깨어나는 결과와 구별해 로그를 함께 기록한다 |
| TOWL-001~010 | 깨끗한 바구니/선반 다음 자리 프리뷰. 사용한 바구니/대기 세탁기, 젖은 바구니/대기 건조기 조준 시 열림·다음 자리 프리뷰. LMB 실제 자리 일치; RMB/LMB 재삽입 자리 동일; 조준 종료·suppression·작동 시작 시 닫힘. Processing에는 프리뷰/열림 없음. 모두 비운 다음 배치는 달라짐. 완료 기계는 Take 강조·열림 |
| TOWL-011~018 | 대기 기계·선반 RMB 유지→한 장씩, 빈 대상/바구니 정원에서 멈춤. 상태 다르면 “다른 상태의 수건”, 빈 선반/Waiting “꺼낼 수건 없음”. 강조 설정 off는 표시만 제거. 손님 선반 획득과 플레이어 RMB 동시에도 총량 보존. 수건통 Apply 없음 |
| SHOP-S03·S04 | 상점 카드 일곱 개와 가격, 화장대+로션 혼합 주문 배송 상자를 E로 들고 LMB 개봉→설비 아이템 1개와 새것 6병 박스. 이번 상품마다 박스 1개=주문 수량 1 |

Q Hold 시간, LMB/RMB 0.15초 감각, 반투명/외곽선 화면 모양·경계, 닫힘/열림 간섭, 손님 실제 루틴은 headless로 확인되지 않았다.
F2 headless는 실제 CustomerSession의 예약 실패→WaitForFacility 구독과 취소 알림 1회→명시적 TryReserveFacility 재시도 성공을 확인한다. 이 fixture는 실행 중 authored StateTree/controller를 구성하지 않으므로 Customer.Event.FacilityAvailable의 실제 StateTree 전이·자동 재시도는 위 PIE에서 확인해야 한다. 현재 Unreal Customer 정본에도 ST_CustomerRoutine BathLoop migration이 미완료로 기록되어 있어 그 asset을 headless 성공의 근거로 가정하지 않았다.
MCP 지원 여부는 이 단계에서 확인한다. 지원하지 않는 작업은 정확한 값·path·절차와 함께 USER_UNREAL.md로 인계하며 Computer Use는 별도 호출 없이는 사용하지 않는다.

## 저장·재로드 뒤 정본 갱신

Unreal/ServiceSystem.md(6 DA·Vanity·manager·상품), FacilitySystem.md(Shower/Vanity/native lid·cue), PlacementSystem.md(Vanity definition), ShopSystem.md(추가 상품), InteractionUISystem.md(새 HUD 실제 표시가 바뀐 경우), 0_UNREAL.md의 관련 링크를 실제 확인 상태로 갱신한다. 수건 표현 별도 정본을 만들면 지도에 링크를 추가한다. 구현 단계에서는 이들 현재 상태 문서를 변경하지 않았다.
