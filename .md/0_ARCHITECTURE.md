# BathhouseSim Architecture Map

## 문서 기준

- 기준일: 2026-10-01(KST), 서비스 2단위 완료(`c9a1150`)와 3단위(쓰레기·수거) 설계 반영
- 상태: 보일러 노동 수직은 완료·승인됐다. 쿨러·순환기 확장은 Source 구현 뒤 통합 승인 전이다. 컴퓨터 포커스 수정은 구현됐다. 상점 주문·배송 상자·쓰레기통 수직(샤워기)은 2026-09-28 구현됐다(`94f0f11`). 같은 날 상점 확장(7종 판매·개봉 물품 흩어짐·배송 상자 scale 정책)이 구현되고 사용자가 통합 승인했다. 설비 회수 아이템 scale 읽기 수정(Placement)은 구현됐다. 들고 있는 물건 조작 LMB·RMB 통일은 구현됐다(`3934d09`). 2026-09-30 서비스 1단위 수직(음료 냉장고)은 아키텍처 재검토(construction 뒤 payload 적용) 재작업을 거쳐 완료됐다(사용자 확인). 같은 날 2단위(진열 확장)가 완료됐고(`c9a1150`), 2026-10-01 3단위(쓰레기·수거)를 설계했다.
- 정본 문서: `.md/0_ARCHITECTURE.md`와 `.md/Architecture/*.md`

## 분석 범위

- 주 분석 범위: `Source/BathhouseSim/Public`, `Source/BathhouseSim/Private`
- 현재 구현된 C++ 하위 시스템:
  - Core
  - Character
  - Camera
  - Interaction
  - Facility
  - Economy
  - Customer
  - UI
  - Cleaning
  - Towel
  - Computer
  - Combat
  - Customer Recovery
  - Utility Labor
  - Shop
  - Service
- `Content`는 Blueprint 참조 검증 범위로만 다룬다. C++ 시스템 책임의 정본은 Source 하위 문서에 둔다.
- `Config/DefaultEngine.ini`는 GameMode/Pawn/Controller 연결 또는 Core Redirect가 필요한 rename 호환 경로로만 문서화한다.

## 시스템 문서

- [UtilityLaborSystem.md](Architecture/UtilityLaborSystem.md): 설비 노동 hub — class 계층, Operation·시간, 설치/가동 용량, 바늘 계기, 회수 payload
- [UtilityFuelSystem.md](Architecture/UtilityFuelSystem.md): 석탄·드라이아이스, 공급함, 삽, 보일러·쿨러 투입 Volume·자동 열림 문
- [UtilityLeverSystem.md](Architecture/UtilityLeverSystem.md): 순환기 조작부, 레버 왕복·취소·복귀와 E 진행 표시
- [ServiceSystem.md](Architecture/ServiceSystem.md): 품목 정의·품목 박스, 진열 공간 넣기·빼기·프리뷰·외곽선 강조, 음료 냉장고, 판매 적립과 공용 수거함
- [ServiceAmenitySystem.md](Architecture/ServiceAmenitySystem.md): 안마의자(동전함·고장·렌치 수리), 평상, TV, 세신대·때수건·세신 포커스, 테스트 인형과 검증 명령
- [CleaningLitterSystem.md](Architecture/CleaningLitterSystem.md): 쓰레기·물 얼룩 인원 기반 생성, 집게+봉투, 묶은 봉투, 수거 구역·world 버리기, 배치 확정 시 발밑 정리, RMB 장비 보조 사용
- [ServiceFacilityDisplaySystem.md](Architecture/ServiceFacilityDisplaySystem.md): 설비 전체 조준 진열, 소모품, 공용 화장대, 샤워 비품, 진열 payload 일반화, 공용 표시 도구
- [ShopSystem.md](Architecture/ShopSystem.md): 상품 목록, 장바구니, 주문·배송 FIFO, 배송 지점·상자, LMB 개봉 무리 배치, 쓰레기통
- [CharacterSystem.md](Architecture/CharacterSystem.md): 1인칭 입력, 컨트롤러 입력 매핑, 이동, 점프, sprint, 캐릭터 조립
- [CameraSystem.md](Architecture/CameraSystem.md): 이동/착지 기반 카메라 셰이크, camera manager 기반 pitch limit
- [InteractionSystem.md](Architecture/InteractionSystem.md): camera trace, primary/secondary intent와 equipment-use routing
- [HeldTargetUseSystem.md](Architecture/HeldTargetUseSystem.md): 들고 있는 물건의 LMB Apply·RMB Take, 입력 소유, 연속 실행과 HUD 데이터
- [PhysicalCarrySystem.md](Architecture/PhysicalCarrySystem.md): exact fixed slot, held-position free drop와 fixed-slot 미지원 설비 아이템
- [FacilitySystem.md](Architecture/FacilitySystem.md): 다중 facility slot, transform 기반 counter queue assignment, checkout overflow와 key drop point
- [BathWaterSystem.md](Architecture/BathWaterSystem.md): 욕탕 급수·배수, 평면 수면 표현, 공통 입욕 임계치와 Customer BathLoop 연계
- [BathWaterOperationsSystem.md](Architecture/BathWaterOperationsSystem.md) / [BathWaterManagementUISystem.md](Architecture/BathWaterManagementUISystem.md): 순환·가열·냉각 용량과 욕탕 condition domain / world-space 관리 화면
- [PlacementSystem.md](Architecture/PlacementSystem.md): 배치 설비와 전용 회수 아이템 변환, preview/Q 회수와 locker capacity lease
- [PlacementPreviewSystem.md](Architecture/PlacementPreviewSystem.md): 배치 미리보기 표현 — 메시 미리보기, footprint 표시, 조준 없음 숨김, 호환 Zone grid, 반투명 그리기 순서
- [BuildingSystem.md](Architecture/BuildingSystem.md): 공간(홀·목욕공간·지하 작업공간) 벽·바닥·천장·조명·출입구·통로·계단 생성, 공간 = 배치 구역, 쓰레기·물 얼룩 생성 조각(EXP-U1), 공간 넓힘 적용·미리보기·넓힘 검증(EXP-U2 설계), 넓힘 한 번에 여러 벽·줄마다 가격(EXP-U3 설계)
- [ExpansionPurchaseSystem.md](Architecture/ExpansionPurchaseSystem.md): 컴퓨터 확장 탭, 구입 transaction(돈·공간 넓힘·홀 효과 tier), 공간별·넓힘별 가격과 홀 효과 표, 락커 상품 허용(EXP-U2 설계, EXP-U3 설계로 전체 상한 삭제)
- [EconomySystem.md](Architecture/EconomySystem.md): PlayerState wallet과 일회성 cash 획득
- [CustomerSystem.md](Architecture/CustomerSystem.md): UE 5.8 StateTree customer routine, session과 cleanup
- [UISystem.md](Architecture/UISystem.md): native Widget/Widget Blueprint 경계와 E/LMB/RMB interaction prompt 계약
- [CleaningSystem.md](Architecture/CleaningSystem.md): water stain spawn, wet mop hold cleaning과 presentation
- [TowelSystem.md](Architecture/TowelSystem.md): towel circulation, atomic transfer, overflow와 processing machine
- [TowelPresentationSystem.md](Architecture/TowelPresentationSystem.md): towel quantity mesh profile과 Stack/Pile/Slot world presentation
- [ComputerSystem.md](Architecture/ComputerSystem.md): world-space monitor, player focus/input session과 sample click UI
- [CombatSystem.md](Architecture/CombatSystem.md): 범용 LMB 장비 사용, 몽키스패너 공격과 health
- [CustomerRecoverySystem.md](Architecture/CustomerRecoverySystem.md): customer 래그돌, routine soft interruption과 native Task restart
- [CoreSystem.md](Architecture/CoreSystem.md): 공통 문서 규칙, 모듈 경계, Source/Content/Config 경계, Core Redirect

## Source 구조

```text
Source/BathhouseSim/
  Public/
    Character/
    Camera/
    Interaction/
    Facility/
    Utility/
    Shop/
    Economy/
    Customer/
    UI/
    Cleaning/
    Towel/
    Combat/
    Building/
  Private/
    Character/
    Camera/
    Interaction/
    Facility/
    Utility/
    Shop/
    Economy/
    Customer/
    UI/
    Cleaning/
    Towel/
    Combat/
    Building/
    Tests/
```

Computer 구현은 `Public/Computer`, `Private/Computer`와 기존 `Public/UI`, `Private/UI` 확장을 사용한다.
Utility Labor target은 `Public/Utility`, `Private/Utility`와 기존 Facility/Interaction/UI의 최소 확장이다. 잔량은 Operation, 적재는 삽, 레버 왕복은 레버 노동 component, 전역 용량은 Operations가 소유한다. 모든 utility는 Operation이 있어야 가동용량을 제공한다.

- `Core`는 소스 폴더가 아니라 문서상 공통 경계다.
- 시스템 하위 폴더명은 include 경로의 1차 네임스페이스 역할을 한다.
- 현재 파일 분포는 하위 시스템 문서를 기준으로 확인한다.
  - Character: 1인칭 캐릭터, 플레이어 컨트롤러, movement/sprint 책임
  - Camera: 이동 상태와 착지 상태 기반 camera shake, camera manager 기반 pitch limit 책임
  - Interaction: player focus, equipment use와 single physical carry transaction 책임
  - Facility: facility/action slot, generic lookup와 counter queue 책임
  - Bath Water / Operations: 물 양·조작부·수면/Niagara·입욕 가능성과 utility 용량·욕탕 condition·관리 snapshot 책임
  - Placement: 설비 mode/preview/placement/recovery, 확장 단계와 locker capacity 책임
  - Economy: player money와 cash claim 책임
  - Customer: StateTree routine과 customer session 책임
  - UI: interaction query의 local HUD 표현 책임
  - Cleaning: zone/stain·쓰레기 registry, 구역별 인원 기반 spawn, equipment-use cleaning, wet mop·집게, 묶은 봉투와 수거 구역 책임
  - Towel: 수건 재고/전송, 설비, overflow, 처리 기계와 recovery ledger 책임
  - Computer: 월드 monitor, focus camera, 사용권과 player computer-use session 책임
  - Combat: 몽키스패너, camera-based melee attack과 공용 health 책임
  - Customer Recovery: Customer Source 내 knockdown, soft interruption과 restartable Task 책임
  - Shop: cart, 주문·배송, 배송 상자·개봉과 쓰레기통 책임
  - Service: 품목 박스·진열 공간·음료 냉장고·판매 적립·수거함, 안마의자·TV·세신대·때수건·세신 포커스 책임
  - Building: 공간 Actor(배치 구역 subclass)와 생성 형상·조명·계단, 공간별 허용 설비와 생성 조각 spawn 책임(EXP-U1), 공간 넓힘과 확장 구입 transaction 책임(EXP-U2 설계)
  - Core: 모듈/redirect/문서 경계 책임

## 시스템 간 책임 흐름

- Character는 로컬 플레이어 입력과 1인칭 캐릭터 조립 지점이다.
- Character는 Enhanced Input action을 이동, 시점, 점프, sprint 의도로 변환한다.
- Character는 sprint 상태와 속도 변경을 movement 책임으로 위임한다.
- Character는 1인칭 카메라와 camera shake component를 조립하지만, camera shake 상태 계산은 Camera가 담당한다.
- Camera는 owner의 이동 상태, sprint 상태, falling/landing 상태를 읽고 camera shake 재생/중단을 결정한다.
- Camera는 player camera manager를 통해 상하 시야각 제한 기본값을 제공하고, Blueprint 파생 class에서 값을 조정할 수 있게 한다.
- Camera는 비로컬 플레이어에서 Tick interval 조정과 shake 중단으로 비용을 줄인다.
- Character는 E/F/G/Q, LCtrl/MouseWheel과 범용 LMB·RMB를 의도로 전달한다. MouseWheel은 computer capture 중 computer 화면 스크롤, 그 밖에는 배치 회전으로 간다. LMB owner는 `Computer > Placement > Equipment(장비) > HeldTargetUse` 순서, RMB는 HeldTargetUse Take이며 Character가 domain 상태를 변경하지 않는다. 들고 있는 물건으로 대상에 하는 일은 LMB(물건 → 대상)·RMB(대상 → 물건), 손으로 하는 일은 E, 내려놓기는 G다. F는 예약이다.
- Interaction은 camera trace, equipment/placement/recovery prompt 합성과 held motion 표현을 소유한다. Physical Carry는 key/wet mop/towel basket/monkey wrench/전용 설비 아이템 중 하나의 held state, exact fixed slot과 free-drop transaction을 소유한다.
- 모든 일반 carryable은 별도 예외가 없으면 G free drop과 exact assigned fixed slot을 지원한다. 전용 설비 아이템은 명시적 `FreeDrop` 전용 예외이며, free drop은 actual held pose에서 질량 무시 약한 velocity change로 시작하고 free-world item은 Pawn을 무시하며 CCD를 사용한다.
- Facility는 다중 use slot, check-in/checkout 독립 FIFO와 revision을 소유한다. queue point는 Location/Yaw 전체를 사용하고 checkout visible capacity를 넘은 entry는 같은 FIFO 순번을 유지한 채 전용 NavMesh volume assignment를 받는다.
- Placement는 placed Actor↔전용 item transaction, 설비 preview, 명시적 zone floor/footprint 파생 snap과 Q Hold 회수를 조율한다. preview 세션은 호환 Zone 전체의 native 중립 grid를 표시하며 전역 셀·Held·preview material은 Settings, Zone별 선 두께·Z offset·강조 간격과 DMI는 PlacementZone이 소유한다. (2026-10-02 설계) preview Actor가 들고 있는 설비 footprint의 XY 사각형을 메시 미리보기와 같은 transform·숨김·유효 색으로 함께 표시하며 판정은 바꾸지 않는다.
- Economy는 PlayerState wallet을 소유하고 cash claim을 한 번만 반영한다.
- Shop은 PlayerState cart와 world 주문 subsystem을 소유한다. 주문은 wallet 차감·주문 생성·cart 비우기를 한 transaction으로 처리하고, 게임시간 딜레이 뒤 배송 지점에 상자를 FIFO로 쌓는다. 상자 LMB 개봉은 Placement factory로 신규 설치 설비 아이템을 카메라 시선 앞 무작위 무리(물품끼리만 겹침, 엔진 충돌 해소로 튐, 막히면 바닥 정면·머리 위)로 만들고, 쓰레기통은 `IPhysicalCarryDiscardable`로 판정해 carry consume으로 제거한다. 컴퓨터 화면은 관리·상점 탭 root widget이다.
- Customer StateTree는 routine을 조율하고 session/queue/facility/key/wallet API에 실행을 위임한다. 신발 단계와 key-locker 대응은 제거하고 탈의·착의마다 임의의 unnumbered locker action slot을 잠시 사용한다.
- Customer bath stay는 pre-shower 완료부터 고정 60초다. 각 탐색 구간은 최대 10초이며 전역 설정 임계 수위 이상 Bath만 예약·이동·입욕하고 실제 입욕 시간만 별도 누적한다.
- Bath Water는 욕탕별 순유량, control mesh E interaction, 수면 위치와 회수 동결을 소유하고 임계 하락을 이용 Customer에게 알린다. Operations는 utility 공용 용량과 욕탕 예약 요구량을 원자적으로 관리하며 condition은 수온·오염도·실제 입욕자 identity, UI는 snapshot/request만 사용한다.
- Bath의 `ApproachPoint`와 `ActionPoint`는 모두 캐릭터 발바닥 transform으로 authoring하며, Customer Session이 scaled capsule half height를 한 번 더해 실제 actor/capsule-center transform으로 변환한다. 고객은 NavMesh 위 `ApproachPoint`까지 이동한 뒤 blocking collision 사전 검사 없이 `ActionPoint`로 unswept snap하고, 퇴탕 시 같은 방식으로 `ApproachPoint`에 복귀한 뒤 navigation을 재개한다.
- Customer 행동 montage는 native StateTree Task가 유효 후보 중 하나를 EnterState에서 선택하며 one-shot 종료 또는 선택된 한 montage의 duration loop를 완료 기준으로 사용한다.
- UI는 Interaction query를 표시하고 domain 상태를 직접 판단하거나 변경하지 않는다.
- Computer는 빈손 primary interaction으로 진입하고 world-space monitor를 유지한 채 player별 camera/input session만 전환한다. 포커스아웃은 widget을 파괴하지 않아 actor lifetime 동안 마지막 화면 상태를 유지한다. 이탈은 E·ESC 한 번이며, 정상 이탈은 컴퓨터별 고정 발바닥 위치·방향(막히면 근처 도달 가능한 빈자리)으로 캐릭터를 옮긴 뒤 시점을 blend한다.
- Cleaning은 zone 기반 water stain spawn, spawn별 material/yaw/XY scale variation과 wet mop hold-cleaning state를 소유한다.
- (3단위 설계) Cleaning은 쓰레기·물 얼룩을 구역 안 손님 수에 비례한 무작위 시점에 바닥에만 만든다. 집게 LMB 줍기·RMB 봉투 묶기, 묶은 봉투 휴대물, 주기 수거 구역을 소유한다. 수거는 `IPhysicalCarryDiscardable` world 버리기로 판정하며 쓰레기통은 레벨에서 빠진다. 배치 확정 이벤트를 구독해 설치 자리와 겹치는 쓰레기·얼룩을 지운다.
- Combat은 LMB Started 단발 몽키스패너 swing, camera-based multi shape trace와 공용 health/depleted event를 소유한다. 무기 World Mesh는 authoritative 피격 판정이 아니다.
- Cleaning의 wet mop은 LMB Hold중 target 유무와 관계없이 mopping state/motion을 유지하고 유효한 정면 water stain에만 제거 progress를 commit한다.
- Customer Recovery는 health 0을 death가 아닌 일시 래그돌로 처리한다. session 타이머·자원·예약과 StateTree hierarchy를 보존하고, 기립 후 queue member는 최신 visible point 위치·Yaw 복귀 gate를 완료한 다음 미완료 국소 행동을 재시작한다.
- Checkout key return은 새 Actor나 점유 슬롯을 만들지 않는다. 손님에게 할당된 동일 key instance를 Counter drop point 주변의 충돌 없는 후보에서 공통 free-world physics transaction으로 `OnCounter` 전환한다.
- 구조적 customer capacity는 설치된 locker action slot 총수이고 check-in key 전달과 capacity lease를 함께 commit한다. 물리 key 수는 expansion tier에만 종속되며 locker 배치·회수로 번호나 수량을 바꾸지 않는다.
- Towel은 homogeneous count, atomic transfer, used-bin overflow와 washer/dryer state를 소유한다.
- Towel Presentation은 inventory snapshot을 읽어 clean stack/used bin/basket의 Stack과 기존 washer/dryer의 Pile을 표시한다. Stack/Pile/Slot은 transient CallInEditor preview를 제공하며 Slot은 gameplay actor에 연결하지 않는다.
- 사용 수건통 내부는 container 단위 E/F interaction이고, overflow world towel만 개별 E interaction이다.
- Customer는 clean towel token과 satisfaction을 session에 보관하고, used bin full이면 floor overflow로 반납한다.
- (EXP-U1 설계) Building은 공간마다 Level 공간 Actor 하나를 둔다. 공간 Actor가 그 바닥의 설비 배치 구역이며, 모든 공간의 authored 값으로 벽·바닥·천장·개구부·계단을 계산해 자기 몫을 Transient component로 만든다(편집 world는 OnConstruction과 지연 일괄 재생성, runtime은 BeginPlay). 공간은 BeginPlay에 자기 조각 종류(홀 쓰레기, 목욕공간 물 얼룩, 작업공간 없음)의 생성 조각 Actor를 spawn한다. 손님 길은 넓은 Nav 범위와 Recast Dynamic 재생성이 맡는다.
- (EXP-U2 설계) 확장 구입은 Building의 `UBathhouseExpansionPurchaseSubsystem`이 조율한다. 공간 Actor가 자기 넓힌 횟수를 소유하고 전체 구입 횟수는 그 합이다. 구입은 사전 검사 → 공간 넓힘(되돌림 가능) → wallet 차감 → 홀이면 확장 관리자 tier 상승(열쇠 생성·락커 한도) 순서의 한 transaction이다. 기존 tier는 홀 넓힘 횟수별 효과 표가 된다. 컴퓨터 화면은 `관리·상점·확장` 탭이고 확장 화면은 view를 매번 subsystem에서 받는다. 상점은 `Facility.Discardable` 없는 락커 상품을 허용한다. (EXP-U3 설계, Source 미반영) 사용자 결정 D2로 전체 구입 횟수·상한이 없어지고 가격은 공간 넓힘 줄마다이며 확장 탭은 선택지별 단계·가격을 보인다. D3로 넓힘 줄 하나가 여러 벽(벽별 양)을 한 번에 물러나게 한다. D4로 설비 배치 미리보기는 후보 위치를 계산하지 못하면 숨는다([ExpansionPurchaseSystem.md](Architecture/ExpansionPurchaseSystem.md), [BuildingSystem.md](Architecture/BuildingSystem.md) Expansion, [PlacementPreviewSystem.md](Architecture/PlacementPreviewSystem.md) Preview Without Aim).
- Core는 런타임 gameplay 상태를 소유하지 않고 모듈 의존성, Source 경계, Content/Config 정책, Core Redirect 기준을 문서화한다.

## 주요 의존 방향

- Character -> Camera/Interaction/Placement/Computer
- Character -> EnhancedInput/Engine Character/Movement
- Camera -> Character/Engine Camera/CameraShake/PlayerCameraManager
- Interaction -> Facility public query
- Placement -> Interaction public carry/prompt-provider contract와 CoreUObject 기반 typed payload
- Facility/Towel -> Placement placeable-facility contract
- Combat -> Interaction
- Economy -> Interaction
- Customer -> Facility/Interaction/Economy
- Bath Water/Operations -> Facility/Interaction/Placement/Niagara/UI
- Customer -> Bath Water public query/delegate와 Operations actual-bather registration API
- Customer -> UE GameplayStateTree/AI/Navigation
- Facility overflow volume -> UE NavigationSystem
- UI -> Interaction
- Computer/UI -> Interaction과 Bath Water Operations snapshot/request API
- Utility -> Interaction/Placement 계약, Facility utility base. Facility Capacity는 주입된 Operation 가동 query만 사용하며 전역 합계는 Operations에 둔다.
- Shop -> Economy/Placement/Interaction 계약, DeveloperSettings, Service 품목 정의·박스 class
- Service -> Interaction/Facility/Placement/Economy 계약, GameplayTags, DeveloperSettings, Combat `IWrenchRepairable`, Computer 이탈 위치 helper(4단위)
- Character -> Service 세신 포커스 component(4단위)
- Towel -> Service 표시 도구(`UDisplayCueComponent`)·Interaction 열림 표현
- UI -> Shop, Computer screen context interface
- Computer -> UMG/Engine Camera/PlayerController
- Cleaning -> Interaction
- Cleaning -> Customer(손님 위치 읽기), Placement(배치 확정 이벤트 subsystem·collision helper)
- Towel -> Interaction
- Towel -> Facility
- Towel Presentation -> Towel
- Customer -> Towel
- Customer -> Combat
- Customer Recovery -> Combat
- Customer Recovery -> Facility
- Customer Recovery -> UE GameplayStateTree/AI/Navigation/Physics
- Building -> Placement(zone base·placeable 계약), Cleaning(조각 zone class), NavigationSystem, DeveloperSettings(EXP-U1)
- Building -> Facility(확장 관리자·정의·락커 용량), Economy(wallet)(EXP-U2)
- UI -> Building 확장 구입 subsystem(EXP-U2)
- Core -> Engine module boundary

## Blueprint/API 변경 주의

- Blueprint native parent, BlueprintCallable API와 serialized reflected/component 이름은 Content 계약이며 rename/delete를 Source와 분리해 판단하지 않는다.
- rename/delete는 [CoreSystem.md](Architecture/CoreSystem.md)의 Core Redirect, Editor 재시작, compile/save와 post-migration scan을 함께 계획한다.
- 상세 API와 이전 이식 호환은 관련 시스템 문서의 `Blueprint/API Contracts`를 우선한다.

## 현재 설계 원칙

- Actor는 composition root에 가깝게 유지하고 상태·반복 기능은 Component 또는 명확한 owner로 분리한다.
- 입력·표시는 의도와 결과만 전달하고 runtime mutation은 상태 owner가 수행한다. 새 기능은 상태/실행/표시/authoring owner를 먼저 정한다.
- 의존은 필요한 방향으로만 추가하고 순환 concrete 참조 전에 interface, event와 subsystem 경계를 검토한다.
- 새 시스템·Blueprint 계약은 관련 정본을 함께 갱신하고 Content 변경은 승인된 Editor 단계에서만 수행한다.
- Player carry는 inventory/hotbar가 아닌 key/wet mop/towel basket/monkey wrench/전용 설비 아이템 중 physical actor 하나만 허용한다. key/equipment의 exact slot, 모든 free-world item의 CCD와 cash 비소지 계약을 유지한다.
- 모든 소지품을 통합하는 공통 Actor/Component는 만들지 않고 `IPhysicalCarryable`을 유지한다. Placement 전용 item과 placed Actor는 새 Actor stage/원본 마지막 제거 transaction을 사용한다. 후보 Z는 explicit zone floor에 footprint bottom offset을 한 번 역산하고, staged/recovery는 Actor collision을 끈 뒤 성공/rollback에서 authored 상태를 복원한다. pre-placed locker는 stable runtime ID 순서의 단일 subsystem reconciliation 후 facility/capacity/Nav를 함께 활성화한다.
- E는 world primary/fixed slot, F는 world secondary, G는 free drop, Q Hold는 facility recovery, LCtrl/휠/LMB는 placement snap/rotation/confirm이다. RMB는 장비 보조 사용(집게 봉투 묶기)이 있으면 그것, 없으면 held-use Take다. Character는 intent만 routing한다.
- 모든 towel endpoint 이동은 source 감소와 destination 증가를 단일 native transaction으로 commit한다.
- Customer routine의 gameplay 상태 변경은 native C++ API를 통해 수행하고 StateTree/Blueprint asset에 domain mutation을 두지 않는다.
- Bath 물 양·조작부·threshold는 native C++만 변경하고 임계 수위는 Project Settings, 유량/control/수면 transform은 Bath authoring이 정본이다. 순환·목표 수온과 utility 회수는 Operations candidate transaction만 commit하며 UI/Actor가 전역 합계나 다른 Bath를 직접 변경하지 않는다.
- Counter는 FIFO와 assignment만 소유하고 Customer Queue Navigation이 AI request·도착 Yaw·overflow wander·knockdown recovery gate를 소유한다. checkout overflow를 별도 queue로 복제하지 않는다.
- 컴퓨터 사용은 game을 pause하거나 fullscreen viewport UI를 열지 않는다. Character는 입력 의도만 분기하고 Computer component가 view/input session lifecycle을 소유하며 screen Widget은 domain gameplay 상태를 소유하지 않는다.

## Implementation Boundary

- 현재 Source에는 Core, Character, Camera, Interaction, Facility, Economy, Customer, UI, Cleaning, Towel, Computer와 Combat native class가 존재한다.
- `ST_CustomerRoutine`, Data Asset, Blueprint facility/key/customer/cash/UI와 Level 배치는 C++ 코드 리뷰 승인 후 Unreal 단계 target이다.
- 현재 Source는 customer-owned montage playback component, Bath action/approach snap과 두 native montage StateTree Task까지 구현한다. AnimNotify, Motion Warping, 신발·의상 전환은 포함하지 않는다.
- Cleaning/Towel Source, secondary/drop input, native prompt 확장과 customer towel StateTree Task/Condition은 구현되었다. InputAction/IMC, WBP hierarchy, Blueprint actor, facility 배치와 `ST_CustomerRoutine` asset 연결은 Unreal 후속 단계다.
- Towel Stack/Pile/Slot native presentation은 collision-free `TowelPresentationVisual` reflected 계약으로 구현되었다. 기존 `BP_Washer`/`BP_Dryer`의 inherited Pile authoring과 profile 지정은 Editor 후속 단계이며 신규 machine이나 drying-rack gameplay actor는 만들지 않는다.
- per-item `HeldTransform`, seeded water-stain visual variation, Stack/Pile/Slot Editor preview와 collision-independent Bath snap은 Source와 native automation까지 구현되었다. `HeldTransform`/stain Blueprint authoring, inherited towel preview와 blocked Bath Editor 통합 검증은 후속 단계다.
- `ABathhouseComputerActor`, `UPlayerComputerUseComponent`, `UComputerSampleScreenWidget`, interaction suppression과 click input은 Source와 focused automation까지 구현되었다. Computer Blueprint/WBP, click InputAction/IMC assignment와 level 배치는 Editor 후속 단계다.
- Combat Source, `PrimaryUseAction` 호환 이관, LMB mop use, equipment prompt row, customer knockdown/soft interruption과 restartable MoveTo는 Source와 native automation까지 구현되었다. `IA_PrimaryUse`, `IMC_FirstPerson`, wrench/customer Blueprint, `WBP_InteractionPrompt`와 `ST_CustomerRoutine` 교체는 코드 리뷰 후 Editor 단계로 인계한다.
- exact equipment slot, key free drop과 actual-held-pose weak release는 Source와 native automation까지 구현되었다. equipment slot Blueprint/instance, exact item/anchor, key physics bounds와 기존 Blueprint release velocity 값은 코드 리뷰 후 Editor 단계로 인계한다.
- counter queue transform/overflow, shared queue navigation, recovery pose gate와 physical checkout key drop은 Source와 native automation까지 구현되었다. StateTree/Counter/overflow volume/Blueprint authoring과 PIE 통합은 후속 Editor 단계이며 기존 queue target Task와 returned-key reflected symbol은 asset migration 동안 deprecated compatibility로 보존한다.
- placed facility↔전용 item 교체, global Held/material, derived footprint, explicit floor, generic preview, collision snapshot, locker reconciliation과 native `GridVisual`/DMI·호환 Zone grid session은 구현됐다. 기존 migration과 전용 grid material·Plane authoring은 Unreal 단계에서 함께 검증한다.
- Bath Water 수직과 Operations 용량 설비, condition, flow mixing, actual-bather 연계, management Widget은 Source/focused automation까지 구현돼 Editor 통합 대기다. deficit no-op transaction, 실제 네-corner 지도 투영, child presentation cache와 utility/bath actor transaction 회귀는 `.md/PROMPT_IMPLEMENTATION_R.md` 기준으로 보강됐다.
- 보일러 노동 수직(Operation, 삽·석탄 공급함, 투입 Volume·문, 계기, 설치/가동 용량, 회수 잔량 보존)은 구현·승인됐다. 2026-09-26 설계로 labor·fuel intermediate class, 쿨러·드라이아이스, 순환기 레버, native `GaugeFace` 삭제와 legacy 항상 공급 제거가 추가되며 `.md/PROMPT_IMPLEMENTATION.md`가 다음 Source 입력이다.
- 상점(ShopSystem): 수직(`94f0f11`)과 확장 모두 구현·통합 승인(2026-09-28).
- 설비 회수 아이템 scale 읽기 수정(PlacementSystem): 2026-09-28 구현(`72f85ec`).
- 들고 있는 물건 조작 LMB·RMB 통일(HeldTargetUseSystem): 구현(`3934d09`).
- 서비스 1단위(ServiceSystem): 2026-09-30 완료(`5b42a47`).
- 서비스 2단위(ServiceFacilityDisplaySystem·Towel 2단위 절): 2026-09-30 완료(`c9a1150`).
- 서비스 3단위(CleaningLitterSystem): 2026-10-01 완료(`2c14d8f`). 같은 날 버그(집게를 들면 빼기 강조 표시) 수정 설계로 장비 보조 사용을 별도 query 필드로 분리했다. 버그 수정은 `4111e20`로 완료됐다.
- 조정값 원본 참조 정리(DOC-TUNING-REFS, Shop·CleaningLitter·Interaction): 2026-10-01 정본 숫자를 원본 위치 참조로 바꿨다. 남은 코드 상수의 데이터 이전도 Source 반영·사용자 PIE 통과·main 병합을 마쳤다(원본: `UShopSettings`, `UShopScreenWidget`, `ACleaningDirectorActor`, 종류별 정리 Actor, 각 carryable).
- 서비스 4단위(ServiceAmenitySystem): 2026-10-01 설계, Source 미반영. 다음 Source 입력은 `.md/PROMPT_IMPLEMENTATION.md`다.
- 버그 `BUG-2026-09-25_bath_water_slider_overrun_and_computer_focus_out`: 2026-10-01 Source 반영, 사용자 PIE 통과, main 병합. 관리 화면 slider 확정값 동기화([BathWaterManagementUISystem.md](Architecture/BathWaterManagementUISystem.md))와 컴퓨터 키보드 focus 불변식 회귀 자동화([ComputerSystem.md](Architecture/ComputerSystem.md))다.
- `COMPUTER-WHEEL-SCROLL`: 2026-10-01 Source 반영, 사용자 PIE 통과, main 병합. 컴퓨터 Active 동안 휠을 virtual pointer로 화면에 주입해 커서 아래 스크롤 영역을 스크롤한다. Content 변경은 없다([ComputerSystem.md](Architecture/ComputerSystem.md) Screen Wheel Scroll, [CharacterSystem.md](Architecture/CharacterSystem.md)).
- 빈 박스 빼기 대상 선택 통일(ServiceFacilityDisplaySystem, `EMPTY-BOX-TAKE-TARGET`): 2026-10-01 Source 반영, 사용자 PIE 통과, main 병합.
- 플레이어 앞 생성 위치 카메라 시선 기준(UNBOX-SPAWN-VIEW, Shop·CleaningLitter·Interaction): 2026-10-01 Source 반영, 사용자 PIE 통과, main 병합.
- 확장 구입·공간 건물(`EXPANSION-PURCHASE`): 2026-10-02 단위 `EXP-U1`(공간 건물) 설계·구현([BuildingSystem.md](Architecture/BuildingSystem.md), Placement Space Zones, CleaningLitter 공간 생성 조각). 2026-10-02 사용자 PIE 통과·병합(`8ca6a24`). 단위 결과물은 Git 이력(병합 커밋의 `.md/Work/EXPANSION-PURCHASE/EXP-U1/`)에 있다. 다음 단위 `EXP-U2`(확장 구입 수직): 2026-10-02 설계([ExpansionPurchaseSystem.md](Architecture/ExpansionPurchaseSystem.md), BuildingSystem Expansion, Shop 락커 규칙, Computer 탭 3개), 2026-10-02 사용자 PIE 통과·병합(`3c17e41`, 결과물은 병합 커밋의 `.md/Work/EXPANSION-PURCHASE/EXP-U2/`). 사용자 결정 D2(공간별 상한·가격)는 설계만 있고 다음 단위(U3 묶음)에서 구현한다. `EXP-U3`(전체 확장 묶음: 목욕·작업공간 넓힘, D2 공간별 상한·가격, D3 한 넓힘 여러 방향, D4 배치 미리보기 숨김, 4·8칸 락커) 2026-10-02 사용자 PIE 통과·병합(`201b2d0`). `EXPANSION-PURCHASE` 완료, 작업 폴더 제거 — 결과물은 각 병합 커밋 이력에서 읽는다.
- 배치 미리보기 footprint 표시(`PLACEMENT-FOOTPRINT-PREVIEW`, [PlacementPreviewSystem.md](Architecture/PlacementPreviewSystem.md)): 2026-10-02 설계, Source 미반영. 쿨러·순환기·보일러 footprint 수정과 root scale 포함 cell 파생·Yaw 정렬 Data Validation([PlacementSystem.md](Architecture/PlacementSystem.md) Definition And Footprint)을 함께 다룬다.
