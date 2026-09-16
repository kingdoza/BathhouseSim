# BathhouseSim Architecture Map

## 문서 기준

- 기준일: 2026-09-14(KST) 급수·배수와 욕탕 물 수직 구현 Source 기준
- 상태: PlacementZone grid와 Bath Water는 Source 구현 후 각각 코드 리뷰·Editor 통합 대기
- 정본 문서: `.md/0_ARCHITECTURE.md`와 `.md/Architecture/*.md`
- legacy 문서: 현재 별도 legacy architecture 문서는 없다.

## 분석 범위

- 주 분석 범위:
  - `Source/BathhouseSim/Public`
  - `Source/BathhouseSim/Private`
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
- `Content`는 Blueprint 참조 검증 범위로만 다룬다. C++ 시스템 책임의 정본은 Source 하위 문서에 둔다.
- `Config/DefaultEngine.ini`는 GameMode/Pawn/Controller 연결 또는 Core Redirect가 필요한 rename 호환 경로로만 문서화한다.

## 시스템 문서

- [CharacterSystem.md](Architecture/CharacterSystem.md): 1인칭 입력, 컨트롤러 입력 매핑, 이동, 점프, sprint, 캐릭터 조립
- [CameraSystem.md](Architecture/CameraSystem.md): 이동/착지 기반 카메라 셰이크, camera manager 기반 pitch limit
- [InteractionSystem.md](Architecture/InteractionSystem.md): camera trace, primary/secondary intent와 equipment-use routing
- [PhysicalCarrySystem.md](Architecture/PhysicalCarrySystem.md): exact fixed slot, held-position free drop와 fixed-slot 미지원 설비 아이템
- [FacilitySystem.md](Architecture/FacilitySystem.md): 다중 facility slot, transform 기반 counter queue assignment, checkout overflow와 key drop point
- [BathWaterSystem.md](Architecture/BathWaterSystem.md): 욕탕 급수·배수, 평면 수면 표현, 공통 입욕 임계치와 Customer BathLoop 연계
- [PlacementSystem.md](Architecture/PlacementSystem.md): 배치 설비와 전용 회수 아이템 변환, preview/Q 회수와 locker capacity lease
- [EconomySystem.md](Architecture/EconomySystem.md): PlayerState wallet과 일회성 cash 획득
- [CustomerSystem.md](Architecture/CustomerSystem.md): UE 5.8 StateTree customer routine, session과 cleanup
- [UISystem.md](Architecture/UISystem.md): native Widget/Widget Blueprint 경계와 E/F/LMB interaction prompt 계약
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
    Economy/
    Customer/
    UI/
    Cleaning/
    Towel/
    Combat/
  Private/
    Character/
    Camera/
    Interaction/
    Facility/
    Economy/
    Customer/
    UI/
    Cleaning/
    Towel/
    Combat/
    Tests/
```

Computer 구현은 `Public/Computer`, `Private/Computer`와 기존 `Public/UI`, `Private/UI` 확장을 사용한다.

- `Core`는 소스 폴더가 아니라 문서상 공통 경계다.
- 시스템 하위 폴더명은 include 경로의 1차 네임스페이스 역할을 한다.
- 현재 파일 분포는 하위 시스템 문서를 기준으로 확인한다.
  - Character: 1인칭 캐릭터, 플레이어 컨트롤러, movement/sprint 책임
  - Camera: 이동 상태와 착지 상태 기반 camera shake, camera manager 기반 pitch limit 책임
  - Interaction: player focus, equipment use와 single physical carry transaction 책임
  - Facility: facility/action slot, generic lookup와 counter queue 책임
  - Bath Water: 욕탕 물 양, 조작부, 수면/Niagara 표현과 입욕 가능성 책임
  - Placement: 설비 mode/preview/placement/recovery, 확장 단계와 locker capacity 책임
  - Economy: player money와 cash claim 책임
  - Customer: StateTree routine과 customer session 책임
  - UI: interaction query의 local HUD 표현 책임
  - Cleaning: zone/stain registry, 물 얼룩 spawn/equipment-use cleaning과 wet mop 책임
  - Towel: 수건 재고/전송, 설비, overflow, 처리 기계와 recovery ledger 책임
  - Computer: 월드 monitor, focus camera, 사용권과 player computer-use session 책임
  - Combat: 몽키스패너, camera-based melee attack과 공용 health 책임
  - Customer Recovery: Customer Source 내 knockdown, soft interruption과 restartable Task 책임
  - Core: 모듈/redirect/문서 경계 책임

## 시스템 간 책임 흐름

- Character는 로컬 플레이어 입력과 1인칭 캐릭터 조립 지점이다.
- Character는 Enhanced Input action을 이동, 시점, 점프, sprint 의도로 변환한다.
- Character는 sprint 상태와 속도 변경을 movement 책임으로 위임한다.
- Character는 1인칭 카메라와 camera shake component를 조립하지만, camera shake 상태 계산은 Camera가 담당한다.
- Camera는 owner의 이동 상태, sprint 상태, falling/landing 상태를 읽고 camera shake 재생/중단을 결정한다.
- Camera는 player camera manager를 통해 상하 시야각 제한 기본값을 제공하고, Blueprint 파생 class에서 값을 조정할 수 있게 한다.
- Camera는 비로컬 플레이어에서 Tick interval 조정과 shake 중단으로 비용을 줄인다.
- Character는 E/F/G/Q, LCtrl/MouseWheel과 범용 LMB를 의도로 전달한다. LMB owner는 `Computer > Placement > Equipment` 순서이며 Character가 domain 상태를 변경하지 않는다.
- Interaction은 camera trace, equipment/placement/recovery prompt 합성과 held motion 표현을 소유한다. Physical Carry는 key/wet mop/towel basket/monkey wrench/전용 설비 아이템 중 하나의 held state, exact fixed slot과 free-drop transaction을 소유한다.
- 모든 일반 carryable은 별도 예외가 없으면 G free drop과 exact assigned fixed slot을 지원한다. 전용 설비 아이템은 명시적 `FreeDrop` 전용 예외이며, free drop은 actual held pose에서 질량 무시 약한 velocity change로 시작하고 free-world item은 Pawn을 무시하며 CCD를 사용한다.
- Facility는 다중 use slot, check-in/checkout 독립 FIFO와 revision을 소유한다. queue point는 Location/Yaw 전체를 사용하고 checkout visible capacity를 넘은 entry는 같은 FIFO 순번을 유지한 채 전용 NavMesh volume assignment를 받는다.
- Placement는 placed Actor↔전용 item transaction, 설비 preview, 명시적 zone floor/footprint 파생 snap과 Q Hold 회수를 조율한다. preview 세션은 호환 Zone 전체의 native 중립 grid를 표시하며 전역 셀·Held·preview material은 Settings, Zone별 선 두께·Z offset·강조 간격과 DMI는 PlacementZone이 소유한다.
- Economy는 PlayerState wallet을 소유하고 cash claim을 한 번만 반영한다.
- Customer StateTree는 routine을 조율하고 session/queue/facility/key/wallet API에 실행을 위임한다. 신발 단계와 key-locker 대응은 제거하고 탈의·착의마다 임의의 unnumbered locker action slot을 잠시 사용한다.
- Customer bath stay는 pre-shower 완료부터 고정 60초다. 각 탐색 구간은 최대 10초이며 전역 설정 임계 수위 이상 Bath만 예약·이동·입욕하고 실제 입욕 시간만 별도 누적한다.
- Bath Water는 욕탕별 순유량, control mesh 자체 E interaction, 평면 수면의 `Lerp(Empty, Full, Amount)` 위치와 회수 홀드 동결을 소유한다. 수위 임계 하락은 현재 이용 Customer 전원에게 퇴장·재탐색 event를 보낸다.
- Bath의 `ApproachPoint`와 `ActionPoint`는 모두 캐릭터 발바닥 transform으로 authoring하며, Customer Session이 scaled capsule half height를 한 번 더해 실제 actor/capsule-center transform으로 변환한다. 고객은 NavMesh 위 `ApproachPoint`까지 이동한 뒤 blocking collision 사전 검사 없이 `ActionPoint`로 unswept snap하고, 퇴탕 시 같은 방식으로 `ApproachPoint`에 복귀한 뒤 navigation을 재개한다.
- Customer 행동 montage는 native StateTree Task가 유효 후보 중 하나를 EnterState에서 선택하며 one-shot 종료 또는 선택된 한 montage의 duration loop를 완료 기준으로 사용한다.
- UI는 Interaction query를 표시하고 domain 상태를 직접 판단하거나 변경하지 않는다.
- Computer는 빈손 primary interaction으로 진입하고 world-space monitor를 유지한 채 player별 camera/input session만 전환한다. 포커스아웃은 widget을 파괴하지 않아 actor lifetime 동안 마지막 화면 상태를 유지한다.
- Cleaning은 zone 기반 water stain spawn, spawn별 material/yaw/XY scale variation과 wet mop hold-cleaning state를 소유한다.
- Combat은 LMB Started 단발 몽키스패너 swing, camera-based multi shape trace와 공용 health/depleted event를 소유한다. 무기 World Mesh는 authoritative 피격 판정이 아니다.
- Cleaning의 wet mop은 LMB Hold중 target 유무와 관계없이 mopping state/motion을 유지하고 유효한 정면 water stain에만 제거 progress를 commit한다.
- Customer Recovery는 health 0을 death가 아닌 일시 래그돌로 처리한다. session 타이머·자원·예약과 StateTree hierarchy를 보존하고, 기립 후 queue member는 최신 visible point 위치·Yaw 복귀 gate를 완료한 다음 미완료 국소 행동을 재시작한다.
- Checkout key return은 새 Actor나 점유 슬롯을 만들지 않는다. 손님에게 할당된 동일 key instance를 Counter drop point 주변의 충돌 없는 후보에서 공통 free-world physics transaction으로 `OnCounter` 전환한다.
- 구조적 customer capacity는 설치된 locker action slot 총수이고 check-in key 전달과 capacity lease를 함께 commit한다. 물리 key 수는 expansion tier에만 종속되며 locker 배치·회수로 번호나 수량을 바꾸지 않는다.
- Towel은 homogeneous count, atomic transfer, used-bin overflow와 washer/dryer state를 소유한다.
- Towel Presentation은 inventory snapshot을 읽어 clean stack/used bin/basket의 Stack과 기존 washer/dryer의 Pile을 표시한다. Stack/Pile/Slot은 transient CallInEditor preview를 제공하며 Slot은 gameplay actor에 연결하지 않는다.
- 사용 수건통 내부는 container 단위 E/F interaction이고, overflow world towel만 개별 E interaction이다.
- Customer는 clean towel token과 satisfaction을 session에 보관하고, used bin full이면 floor overflow로 반납한다.
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
- Bath Water -> Facility/Interaction/Placement/Niagara
- Customer -> Bath Water public query/delegate
- Customer -> UE GameplayStateTree/AI/Navigation
- Facility overflow volume -> UE NavigationSystem
- UI -> Interaction
- Computer -> Interaction
- Computer -> UMG/Engine Camera/PlayerController
- Cleaning -> Interaction
- Towel -> Interaction
- Towel -> Facility
- Towel Presentation -> Towel
- Customer -> Towel
- Customer -> Combat
- Customer Recovery -> Combat
- Customer Recovery -> Facility
- Customer Recovery -> UE GameplayStateTree/AI/Navigation/Physics
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
- E는 world primary/fixed slot, F는 world secondary, G는 free drop, Q Hold는 facility recovery, LCtrl/휠/LMB는 placement snap/rotation/confirm이다. Character는 intent만 routing한다.
- 모든 towel endpoint 이동은 source 감소와 destination 증가를 단일 native transaction으로 commit한다.
- Customer routine의 gameplay 상태 변경은 native C++ API를 통해 수행하고 StateTree/Blueprint asset에 domain mutation을 두지 않는다.
- Bath 물 양·조작부 상태와 threshold 판정은 native C++만 변경한다. 임계 수위는 Project Settings 한 곳, 유량과 control/수면 transform은 욕탕 Blueprint 및 허용된 Level override만 정본으로 사용한다.
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
- Bath Water 수직 구현은 Source와 focused automation까지 구현됐다. `BP_Bath` native parent/분리 control mesh/평면 수면/Niagara, 전역 threshold, routine Data Asset와 `ST_CustomerRoutine` BathLoop 연결은 코드 리뷰 후 Editor 단계에서 통합한다.
