# Towel System

## Implementation Status

clean stack부터 customer 사용, used bin/바닥 overflow, player basket, washer와 dryer를 거쳐 clean stack으로 돌아오는 수건 순환 Source와 기본 Blueprint class는 구현되었다. basket exact fixed slot과 held-position free drop도 [PhysicalCarrySystem.md](PhysicalCarrySystem.md)에 따라 구현되었다. Placement는 수건 inventory/revision을 변경하지 않으며 Clean Towel Stack과 Used Towel Bin은 Actor 배치·회수에서 제외한다.

## Source Scope

```text
Source/BathhouseSim/Public/Towel/
  TowelTypes.h
  TowelInventoryComponent.h
  TowelTransferSubsystem.h
  TowelCirculationSubsystem.h
  TowelBasketActor.h
  CleanTowelStackActor.h
  UsedTowelBinActor.h
  WorldUsedTowelActor.h
  TowelProcessingMachineActor.h
  TowelTransferPortComponent.h
  TowelMachineControlComponent.h
  Presentation/
    TowelVisualTypes.h
    TowelVisualMeshProfile.h
    TowelQuantityVisualComponent.h
    TowelStackVisualComponent.h
    TowelPileVisualComponent.h
    TowelSlotVisualComponent.h

Source/BathhouseSim/Private/Towel/
  # matching implementation files

Source/BathhouseSim/Private/Tests/
  BathhouseCleaningTowelTestProbe.h/.cpp
  CleaningTowelAutomationTests.cpp
```

## Responsibilities

- homogeneous towel state, count, capacity와 revision
- 모든 towel endpoint 사이의 atomic transfer
- clean stack, used bin, carried basket와 individual overflow towel
- washer/dryer state, timer와 towel state conversion
- customer towel-use token과 overflow recovery 지원
- authoritative count와 분리된 native presentation 및 Blueprint authoring 경계

Towel은 player input mapping, customer StateTree hierarchy, facility reservation과 UI 상태를 소유하지 않는다.

## Towel State

`ETowelState`:

- `None`: count가 0인 homogeneous container
- `Used`
- `Wet`
- `Clean`

`FTowelInventorySnapshot`은 state, count, capacity와 monotonic revision을 제공한다. count가 0이면 state는 반드시 `None`이고, count가 1 이상이면 하나의 non-None state만 가진다.

`ETowelMachineState`:

- `Waiting`
- `Processing`
- `Complete`

`ETowelMachineKind`:

- `Washer`: `Used -> Wet`
- `Dryer`: `Wet -> Clean`

## State Owners

| 책임 | Owner |
|---|---|
| endpoint state/count/capacity/revision | `UTowelInventoryComponent` |
| two-endpoint atomic mutation | `UTowelTransferSubsystem` |
| customer token, spill pending/recovery ledger | `UTowelCirculationSubsystem` |
| machine state/end time/conversion | `ATowelProcessingMachineActor` |
| basket held/fixed/free physical lifecycle | `ATowelBasketActor` + `UPlayerCarryComponent` |
| customer-held towel handle | `UCustomerSessionComponent` |
| visual count convergence | `UTowelQuantityVisualComponent` |

## Inventory And Atomic Transfer

`UTowelInventoryComponent`는 외부 public setter를 제공하지 않는다. Transfer Subsystem과 machine의 validated internal transition만 state/count를 변경한다.

`FTowelTransferRequest`:

- source/destination endpoint identity
- requested count
- expected source/destination revision

`FTowelTransferResult`:

- success
- moved count
- failure reason
- committed source/destination revision

`UTowelTransferSubsystem::TryTransfer` 절차:

1. 두 endpoint의 validity, registration과 transaction guard를 확인한다.
2. revision, source count와 destination remaining capacity를 확인한다.
3. state compatibility와 endpoint/machine gate를 확인한다.
4. `Min(Requested, SourceCount, DestinationRemaining)`을 계산한다.
5. 두 endpoint를 잠그고 remove/add의 실패 불가능한 internal mutation을 연속 commit한다.
6. 두 revision을 증가시키고 잠금을 해제한다.
7. 양쪽 Blueprint/native delegate를 commit 후 방송한다.

검증 실패 시 remove/add 어느 쪽도 실행하지 않는다. Unreal game thread serialization과 revision 재검증으로 completion/interaction 동시점과 반복 입력을 처리한다.

player 수건 이동은 held-use 한 단위로 requested count 1만 요청한다. LMB(Apply)는 바구니 → 대상, RMB(Take)는 대상 → 바구니이며 누르고 있으면 간격마다 한 장씩 반복한다. E·F는 수건을 옮기지 않는다(2026-09-28, [HeldTargetUseSystem.md](HeldTargetUseSystem.md) Towel Targets). 방향별 가능 여부와 이유는 `FTowelHeldTransferRules`가 만든다.

## Physical Towel Basket

`ATowelBasketActor`는 generic physical carry 계약과 하나의 towel inventory를 가진다.

- capacity 제한
- empty면 `None`
- 첫 towel이 들어오면 해당 state로 고정
- 같은 state만 추가 가능
- count가 다시 0이면 `None`
- exact assigned slot에 내용물과 관계없이 E로 take/store
- G로 actual held pose에서 질량 무시 약한 velocity change를 받아 free world로 전환
- slot/drop/physics/EndPlay 중에도 contents와 revision은 actor inventory에 유지

다른 key/mop/basket/wrench를 들고 있으면 pickup하지 않는다. fixed-slot placement는 towel transfer가 아니며 inventory delegate를 발생시키지 않는다. basket EndPlay가 정상 world shutdown이 아니라면 contents snapshot을 circulation recovery ledger에 한 번 이전한다.

## Clean Stack And Used Bin

`ACleanTowelStackActor`는 clean towel inventory와 customer facility slot을 가진다.

- player Clean basket -> stack 이동
- LMB 한 장, 누르고 있으면 반복
- customer가 한 장을 towel-use token으로 획득
- `EBathhouseFacilityType::TowelShelf`로 customer navigation에 등록

`AUsedTowelBinActor`는 기존 `EBathhouseFacilityType::TowelBasket`의 customer return 위치를 유지한다.

- customer Used towel return
- bin -> player Used basket 이동
- RMB 한 장, 누르고 있으면 반복
- bin capacity가 가득 차면 customer towel을 보유하지 않고 floor overflow로 전환

bin 내부 visible towels는 count 기반 presentation 전용이며 개별 collision/interaction을 갖지 않는다.

Clean Stack과 Used Bin은 authoritative towel token endpoint이므로 `SupportsFacilityActorConversion()`이 false다. Q recovery row를 노출하지 않고 전용 facility item/typed payload를 만들지 않으며, 이 class를 대상으로 하는 Placement Definition은 validation에서 거부한다. Washer/Dryer만 아래의 안전한 empty/Waiting gate를 통해 설비 Actor 변환에 참여한다.

## Used Towel Overflow

`AWorldUsedTowelActor`는 used bin 주변 바닥에 넘친 towel 한 장을 나타내는 authoritative token actor다.

Return flow:

1. customer handle이 Used token을 보유한다.
2. bin에 공간이 있으면 bin count +1로 commit한다.
3. full이면 bin의 authorable annulus 안 random point를 선택한다.
4. floor trace, facility/wall blocking, existing towel spacing과 Pawn overlap을 검증한다.
5. valid point에 staged actor를 생성한 뒤 token owner를 customer handle에서 actor로 commit한다.
6. 성공 후 customer는 return 단계에서 벗어난다.

`AUsedTowelBinActor` authoring 값:

- overflow min/max radius
- floor trace channel/distance
- placement attempts
- towel spacing/Pawn clearance
- `WorldUsedTowelClass`

후보 또는 spawn에 실패하면 token을 `PendingSpill` ledger로 원자 이전하고 customer를 진행시킨다. Circulation Subsystem이 bin 유효 시 재시도한다.

World towel은 held-use Take(RMB) 한 장만 지원한다. E·LMB로는 줍지 않는다.

- held object가 compatible basket인지 확인
- basket이 empty/Used이며 여유가 있는지 확인
- actor token 감소와 basket Used +1을 한 transaction으로 commit
- 성공 후 actor를 `Consumed`로 표시하고 제거
- F와 E 이동은 없음

비정상 EndPlay의 unconsumed world towel은 PendingSpill/recovery ledger로 돌아가며, consumed actor는 다시 recovery하지 않는다.

## Washer And Dryer

`ATowelProcessingMachineActor`는 inventory, machine state와 process end time을 소유한다.

- transfer port: basket과 LMB(넣기)·RMB(빼기) towel 이동
- separate control component: E로 processing 시작
- `Waiting`에서만 correct input towel 투입
- count > 0일 때만 control interaction으로 시작
- `Processing` 중 port/control mutation 차단
- timer 완료 시 count를 유지한 채 전체 state를 output state로 한 번 변환
- `Complete`에서만 output 회수
- count가 0이 되면 `Waiting`/`None`으로 복귀

완료 machine에서 basket 회수:

- empty basket은 허용
- 같은 output state와 남은 capacity가 있는 basket은 허용
- 다른 state 또는 full basket은 정확한 이유로 실패

machine capacity와 process duration은 instance/default authoring 값이다. normalized progress는 stored end time에서 파생하며 Blueprint가 timer 정본을 복제하지 않는다.

machine Actor는 canonical target에서 `IPlaceableFacility`만 구현하고 `IPhysicalCarryable` 책임은 전용 설비 아이템으로 이전한다. 회수 query는 authoritative inventory count가 0이고 machine state가 `Waiting`인 경우만 성공한다. machine은 `MachineKind`와 `ProcessingDurationSeconds`만 typed placement payload로 export/import하며 count, towel state와 processing progress는 전달하지 않는다. Placement는 count/state를 복제하거나 강제로 비우지 않으며 자세한 Actor 변환 transaction은 [PlacementSystem.md](PlacementSystem.md)를 따른다.

## Transfer Direction

2026-09-30 서비스 2단위: `Clean stack`은 held basket ← stack(RMB, 빈·Clean 바구니)도 허용하고, `Waiting washer/dryer`는 machine → held basket(RMB, 빈 바구니 또는 기계 안과 같은 상태)도 허용한다. 아래 Service Unit 2 Display Changes 절.

| Target | Direction |
|---|---|
| Used bin | bin Used -> held basket |
| World used towel | actor token -> held basket, RMB only |
| Waiting washer | held basket Used -> washer |
| Complete washer | washer Wet -> held basket |
| Waiting dryer | held basket Wet -> dryer |
| Complete dryer | dryer Clean -> held basket |
| Clean stack | held basket Clean -> stack |

held basket → 대상 방향은 LMB(Apply), 대상 → held basket 방향은 RMB(Take)다. Port/interactable가 held basket과 target snapshot으로 가능 여부를 결정한다. Character, Widget과 input action은 towel type을 판별하지 않는다.

## Customer Token And Fallback

`FTowelUseHandle`은 customer가 clean stack에서 가져간 towel 한 장의 owner를 명시한다.

- acquired clean token
- used 여부
- returned/cleanup terminal guard
- original stack identity

Clean towel이 없으면 customer는 authorable wait limit 동안 availability event를 기다린다. 만료 시 towel 없이 routine을 계속하고 towel-dependent use/return을 건너뛰며 session satisfaction을 authorable amount만큼 감소시킨다.

Used bin capacity는 acquisition을 차단하지 않는다. full return은 floor overflow/PendingSpill로 보존한다.

Customer interruption:

- 사용 전: original clean stack으로 반환, 불가능하면 recovery ledger
- 사용 후: used bin 또는 overflow/PendingSpill로 commit
- terminal cleanup은 idempotent하고 token owner가 항상 정확히 하나여야 한다.

## Blueprint Presentation Contract

`UTowelInventoryComponent::OnInventoryChanged`는 previous/current snapshot과 transaction id를 제공한다. 각 snapshot에 state, count, capacity와 revision이 포함된다.

Stack/Pile/Slot의 native mesh selection, ISM layout, count animation, revision convergence와 연결 범위는 [TowelPresentationSystem.md](TowelPresentationSystem.md)를 따른다. 연결 actor의 collision-free reflected default-subobject 계약명은 공통 `TowelPresentationVisual`이다. Blueprint는 pivot/bounds/profile과 machine animation/sound만 authoring하며 authoritative count를 변경하지 않는다.

Machine 표현 event:

- `OnMachineStateChanged`
- `OnMachineProgressChanged`
- `OnMachineContentsChanged`

Towel actor 표현 event:

- `UTowelInventoryComponent::OnInventoryChanged`
- `ATowelBasketActor::OnHeldPresentationChanged`

## Dependencies

- Towel → Service `UDisplayCueComponent`·`UServiceDisplaySettings`(표시만), Interaction `UOpeningPresentationComponent`(2026-09-30).

- Towel -> Interaction/Physical Carry의 intent/carry/fixed-slot public 계약
- Towel -> Facility의 generic actor/slot registration
- Towel -> Placement의 placeable facility interface
- Customer -> Towel transaction/token API
- UI -> Interaction prompt data
- Towel은 Customer concrete StateTree와 UI concrete class에 의존하지 않는다.

## Service Unit 2 Display Changes

2026-09-30 설계, Source 반영. 입력: `.md/PROMPT_ARCHITECTURE.md` 서비스 2단위 TOWL-001~018. 상태 전환·작동·정원·손님 사용·바구니 규칙은 바꾸지 않는다.

- 이동 조건:
  - `FTowelHeldTransferRules`: 선반 Take와 Waiting 기계 Take를 허용한다(빈 바구니 또는 같은 상태). 다른 상태면 `다른 상태의 수건`, 비었으면 `꺼낼 수건 없음`(TOWL-012, 014, 015). 누르고 있으면 기존 Repeat로 바구니가 차거나 선반이 빌 때까지 옮긴다(TOWL-013).
  - `ATowelProcessingMachineActor::AllowsInventoryTransfer`: Waiting 상태의 machine → basket 이동을 허용한다. 모두 빼도 Waiting을 유지한다(TOWL-011). Complete·Processing 규칙은 그대로다.
  - 선반 빼기와 손님 가져가기는 기존 `UTowelTransferSubsystem` 원자 이동이라 섞이지 않는다(TOWL-018).
- 넣기 프리뷰·꺼내기 강조:
  - 선반·사용 수건통·transfer port(기계)는 `IPlayerInteractionFocusObserver`를 구현한다.
  - 각 owner에 `UDisplayCueComponent` native default subobject를 하나 추가하고, 수건 visual component에 붙인다.
  - 알림 query가 Apply 가능이면 visual의 `GetIndexPresentation(Count, 바구니 상태)`에 프리뷰를 둔다.
  - Take 가능이면 `GetIndexPresentation(Count−1, 대상 상태)`에 강조를 둔다(설정 옵션).
  - Count는 inventory authoritative 값이다. 표시 animation의 displayed count가 아니다.
  - cue 재계산 경로(2026-09-30 재작업): cue는 focus 알림에서만 계산하고, 수량 변화는 query 변화로 알림을 다시 받는다.
    - 선반·사용 수건통·transfer port의 `QueryInteraction`이 `FPlayerInteractionQuery::PresentationRevision` = 대상 inventory `Revision` + 든 바구니 inventory `Revision`(바구니가 아니면 0)을 채운다. 두 값은 commit마다 1씩 오르므로 어느 쪽이 바뀌어도 합이 바뀐다. TargetName·행동명·가능 여부·이유는 바꾸지 않는다(HUD 문구 불변).
    - `Equals`가 이 필드를 비교하므로 수량이 바뀐 다음 query commit에서 `SyncFocusObservers`가 같은 대상에 알림을 다시 보내고, owner는 새 Count로 `TowelDisplayCueUtils::Update`를 다시 실행한다.
    - 플레이어 넣기·빼기와 연속 반복: `UPlayerHeldTargetUseComponent`가 매 실행 직후 `RefreshInteractionQuery`를 호출하므로 같은 프레임에 따라간다.
    - 손님 가져가기·기계 상태 전환 등 외부 변화: 조준 중 매 tick의 `RefreshInteractionQuery`에서 반영된다(늦어도 다음 tick). 별도 inventory 구독·마지막 query 보관은 두지 않는다.
    - 숨김·닫힘(focus 종료, suppression, 작동 시작)과 뚜껑 규칙은 그대로다. revision 변화만으로는 뚜껑 열림 요청이 바뀌지 않는다(같은 가능 방향이면 같은 요청).
    - 진열 공간·router·냉장고는 필드를 채우지 않는다(0). 이들은 TargetName 수량으로 이미 따라간다([ServiceSystem.md](ServiceSystem.md)).
    - 결정 근거: 리뷰 선택지 C. A(owner별 inventory 구독)는 알림 시점의 query 가능 여부가 이전 값이라 잠깐 틀린 cue를 낼 수 있고 owner 세 곳에 수명 관리가 필요하다. B(TargetName 수량)는 HUD 문구 변경이라 기능 계약 밖이다.
  - 사용 수건통은 Apply가 없어 프리뷰가 없다. 바닥 수건은 대상이 아니다(TOWL-001, 004, 005, 016).
- 기계 안 자리: Pile의 index 자리를 결정적으로 바꾼다([TowelPresentationSystem.md](TowelPresentationSystem.md) Deterministic Index Layout). 프리뷰 자리 = 넣었을 때 놓이는 자리다. 빼고 다시 넣으면 같은 자리다. 기계가 비면 배치를 새로 정한다(TOWL-006~008, 010).
- 뚜껑·문:
  - `ATowelProcessingMachineActor`에 native default subobject `LidPivot`(USceneComponent), `LidMesh`(UStaticMeshComponent, NoCollision, Navigation off, pivot 자식), `LidPresentation`(`UOpeningPresentationComponent`, pivot 주입)을 추가한다. 이름·위치·열림 값은 `BP_Washer`·`BP_Dryer`가 authoring한다.
  - transfer port의 focus 알림에서 Apply 또는 Take가 가능하면 열림을 요청하고, 아니면 해제한다. focus 종료·suppression에서도 해제한다.
  - 작동 시작은 query 변화로 닫히며, 작동 중에는 가능 방향이 없어 열리지 않는다(TOWL-002, 003, 009, 건조기 TOWL-017).
  - 표현 전용이며 이동 조건·작동 조작에 영향이 없다.
- 기존 E/F·held-use 규칙, CTRL-017·018·021~024 결과는 유지한다.

## Manual Review Points

- 모든 LMB/RMB transfer 전후 총 towel token 수가 보존되는지 확인한다.
- bin 내부 towel은 container 단위 RMB이고 overflow towel만 개별 RMB인지 확인한다.
- mixed state, full capacity, processing state와 repeated input이 양쪽 endpoint를 변경하지 않는지 확인한다.
- count가 남았거나 `Processing`/`Complete`인 machine의 Q 회수가 정확한 이유로 실패하는지 확인한다.
- bulk stack presentation 중단 뒤 C++ count와 visible count가 재동기화되는지 확인한다.
- customer interruption과 actor EndPlay에서 token이 정확히 한 owner 또는 recovery ledger에 남는지 확인한다.
- non-empty basket의 fixed-slot take/store와 G drop이 state/count/revision을 바꾸지 않는지 확인한다.
- Stack/Pile/Slot 표현 변경이 inventory transfer, machine state와 recovery ledger를 변경하지 않는지 확인한다.
- 다른 towel state/tool durability/consumable 기능이 이번 범위에 추가되지 않았는지 확인한다.
