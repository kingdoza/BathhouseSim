# Service Amenity System

## Status And Scope

- [ServiceSystem.md](ServiceSystem.md)의 하위 문서다. 서비스 4단위(배치형 서비스)의 안마의자, 평상, TV, 세신대·때수건·세신 포커스, 테스트 인형과 손님 없는 검증 명령을 소유한다.
- 2026-10-01 설계, Source 미반영.
  - 입력: `.md/PROMPT_ARCHITECTURE.md`(MASS-001~015, REST-001~006, SCRB-001~020, SVC4-001~003)
  - 설계 중 사용자 결정: Q66 A(이용 중 쓰러짐 = 기존 회복 규칙), Q67 A(대상 전용 도구도 대상의 이유 표시)
- 수직 구현이다. 대표 흐름은 세신대·안마의자 구매·배치 → 테스트 인형 세신 → 현금 받기 → 안마의자 이용 완료·수금·수리다.
- 비대상: 손님 루틴(언제 이용하러 오는지·눕기/앉기 동작), 만족도·TV 효과, 세신사, 때수건 소모, 저장.

## Source Scope

```text
Public/Service/
  ServiceAmenityTypes.h              IServiceFacilityUser, EServiceUseEndReason
  MassageChairActor.h                AMassageChairActor : ABathhouseFacilityActor
  MassageChairPlacementInstanceData.h
  TelevisionActor.h                  ATelevisionActor : ABathhouseFacilityActor
  ScrubTableActor.h                  AScrubTableActor : ABathhouseFacilityActor
  ScrubTowelActor.h                  AScrubTowelActor(레벨 도구)
  PlayerScrubFocusComponent.h        플레이어 세신 포커스 세션
  ServiceTestUserActor.h             테스트 인형(손님 대역)
Private/Service/
  (위 .cpp), ServiceAmenityDebugCommands.cpp(비 shipping)
Public/Combat/
  WrenchRepairable.h                 IWrenchRepairable(C++ 전용)
Private/Combat/
  WrenchRepairSession.h/.cpp         수리 진행 private helper
Public/UI/ Private/UI/
  ScrubFocusHudWidget.h/.cpp         세신 포커스 게이지·대기 HUD
```

- 수정할 기존 파일:
  - `BathhouseFacilityTypes.h`, `PhysicalCarryable.h`(enum append)
  - `MonkeyWrenchActor`
  - `PlaceableFacility.h`, `BathhouseFacilityActor`(회수 수행자)
  - `PlayerFacilityPlacementComponent`(회수 수행자 전달 두 줄)
  - `PlayerEquipmentUseComponent`(Q67 합성 규칙)
  - `FirstPersonCharacter`(세신 포커스 입력 분기)
  - `BathhouseHUD`
- 평상은 코드가 없다. `ABathhouseFacilityActor` Blueprint에 slot 3개만 둔다.

## Ownership

| 대상 | 상태 owner | 실행 owner |
|---|---|---|
| 동전함 금액, 고장 여부, 이용 timer | `AMassageChairActor` | 안마의자 |
| 수리 진행(누르는 동안) | `FWrenchRepairSession`(렌치 소유) | `AMonkeyWrenchActor` |
| TV 전원 | `ATelevisionActor` | TV |
| 누운 손님의 대기 마감·게이지·세신자 예약 | `AScrubTableActor` | 세신대 |
| 포커스 phase·시점·입력·커서 위치 | `UPlayerScrubFocusComponent` | 플레이어 |
| 좌석·누움 점유 | 기존 `UBathhouseFacilitySlotComponent` | slot |
| 세신 현금 | 기존 `ABathhouseCashPaymentActor` | 현금 Actor |
| 이용 종료 뒤 손님 행동 | 손님(`IServiceFacilityUser` 구현) | 테스트 인형 / 이후 손님 루틴 |

## Common Facility Contract

- 네 설비는 기존 `ABathhouseFacilityActor` 계열이다. 상점 구매, 설비 아이템, 배치, Q 회수, 버리기·수거는 기존 경로 그대로다.
- `EBathhouseFacilityType` 끝에 `MassageChair`, `RestBench`, `Television`, `ScrubTable`을 append한다.
- 이용 중 회수 불가: 기존 `QueryFacilityRecovery`(slot이 모두 Available이어야 함)로 충족된다. 회수 Hold 중 새 이용 금지는 기존 `bRecoveryHoldActive`가 `IsAvailableForReservation`에 반영한다.
- **쓰러짐(Q66 A):** 기존 손님 회복은 slot을 `Occupied → Reserved`(EndUse)로 돌리고, 일어나면 다시 `BeginUse`한다. 설비는 이 전이를 "이용 중단"으로 처리한다.
  - 중단 시: 진행 중인 이용 timer·게이지를 버리고 요금·고장 판정을 하지 않는다. 자리는 Reserved로 그 손님에게 남는다.
  - 다시 `Occupied`가 되면 처음부터 시작한다.
- **사라짐:** 설비가 현재 사용자의 `OnEndPlay`를 구독해 `ForceRelease`한다(1단위 냉장고·`UServiceDisplayManagerComponent`와 같은 규칙). 요금은 없다.
- 이용 종료 알림은 `IServiceFacilityUser`(C++ 전용 interface, 선택)로 한다.
  - `HandleServiceUseEnded(AActor& Facility, EServiceUseEndReason Reason)`. Reason은 `Completed`(안마의자 60초 완료) 또는 `Abandoned`(세신 대기 만료)다.
  - `HandleScrubCashOffered(AScrubTableActor& Table, ABathhouseCashPaymentActor& Cash, const FTransform& StandTransform)`
  - 테스트 인형과 이후 손님이 같은 계약을 구현한다. 테스트 인형 전용 규칙은 설비에 없다.

## Massage Chair

`AMassageChairActor`(`BP_MassageChair`, FacilityType `MassageChair`).

- 구현 추가: `IWrenchRepairable`.
- authoring(EditDefaultsOnly): `UseSeconds` 60(> 0), `UseFee` 3000(≥ 0), `BreakChancePercent` 10(0~100), `RepairSeconds` 3(> 0).
- 상태(Transient): `CoinBalance`(int32), `bBroken`.
- slot: Blueprint slot 정확히 1개. BeginPlay에서 `OnSlotStateChanged`를 구독한다. Data Validation은 CDO SCS 검사로 slot 수를 확인한다(1단위 냉장고 방식).
- 이용 흐름:
  1. slot → `Occupied(User)`: `UseSeconds` timer를 시작하고, 사용자의 `OnEndPlay`를 구독한다.
  2. `Occupied`를 벗어남(쓰러짐 `Reserved`, `ForceRelease`): timer를 지운다. 요금·고장 없음(MASS-011).
  3. timer 만료(같은 사용자가 여전히 `Occupied`, placed-domain active):
     - `CoinBalance += UseFee`(int64 합산 후 int32 상한 clamp, 경고)
     - 고장 판정 `FMath::FRand() * 100 < BreakChancePercent`. 고장이면 `bBroken=true`(J12, MASS-003, 013)
     - slot `EndUse` → `Release`로 자리를 비운다.
     - 사용자 `HandleServiceUseEnded(Completed)`, 가용성 변경 방송
- `IsAvailableForReservation()` = Super && !`bBroken`(MASS-004).
- E(들고 있는 물건 무관, J1):
  - TargetName `안마의자 · 9,000원 · 정상/고장/이용 중`. 금액 > 0이면 `수금`, 아니면 이유 `모인 돈 없음`(MASS-010).
  - execute: fresh 재검증 → interactor PlayerState wallet → `TryAddMoney` 성공 시에만 0으로 만든다. 재진입 guard로 반복 입력에도 한 번만 지급한다(MASS-002, 014). 음료 수거함과 같은 구조다.
- LMB 안내(Q67 A, MASS-012, 015): 고장이면 held-use Apply 필드를 visible·불가·`몽키스패너가 필요합니다`로 둔다. 들고 있는 carry kind가 `Facility`면 비운다(배치가 LMB 소유). 장비 합성 규칙은 아래 Equipment Merge다.
- 수리(`IWrenchRepairable`):
  - `IsWrenchRepairRequired()` = `bBroken` && placed-domain active
  - `GetWrenchRepairSeconds()` = `RepairSeconds`
  - `CommitWrenchRepair(OutFailure)`: `bBroken=false`, 가용성 방송, BP event
- 회수와 payload:
  - `CreateFacilityPlacementInstanceData`를 override해 `UMassageChairPlacementInstanceData : UBathhouseFacilityPlacementInstanceData { bool bBroken; }`를 만든다.
  - `ExportFacilityExtension`·`ImportFacilityExtension`은 Super 뒤에 `bBroken`을 쓰고 읽는다. 신규 설치는 정상이다.
  - 회수 아이템 요약 `GetPlacementContentsSummary()`: Super 결과에 `고장`을 덧붙인다(MASS-008).
  - 동전함: `StagePlacedDomainUnregistration`에서 Super 뒤에 회수 수행자(아래 Recovery Instigator)의 wallet을 확인한다. 없으면 stage가 실패하고 회수도 실패한다.
    - 기존 publication callback을 감싸 commit 뒤 `TryAddMoney(금액)`를 실행한다. 금액은 stage 시점 값을 복사한다.
    - Q 취소·rollback에서는 아무것도 바뀌지 않는다(J2, MASS-008).
- BP event: `OnBrokenStateChanged(bool)`(고장 외형), `OnCoinBalanceChanged(int32)`(동전함 표현, 선택).

### Recovery Instigator

- `IPlaceableFacility`에 기본 no-op `SetFacilityRecoveryInstigator(AActor*)`를 추가한다.
- `UPlayerFacilityPlacementComponent`:
  - `TryBeginFacilityRecoveryHold` 직전에 owner pawn을 넘긴다.
  - `CancelRecovery`에서 nullptr를 넘긴다.
- `ABathhouseFacilityActor`는 Transient weak로 보관한다. 안마의자만 읽는다.
- Placement는 지갑을 모른다.

## Wrench Repair

[CombatSystem.md](CombatSystem.md)의 확장이다.

- `IWrenchRepairable`(Public/Combat, C++ 전용): 위 세 함수.
- `FWrenchRepairSession`(Private/Combat): target weak, 경과 시간. `Begin`, `Update(FocusActor, Delta) → Running/Succeeded/Failed`, `Reset`.
- `AMonkeyWrenchActor`(현재 375줄. 세션 로직을 helper로 둬 400줄 경고선 근처에서 멈춘다):
  - `QueryEquipmentUse`: focus hit Actor가 수리가 필요한 `IWrenchRepairable`이면 다음을 반환한다. 그 밖에는 기존 휘두르기다(MASS-007).
    - action `수리`, mode `Hold`
    - progress = 같은 target 세션의 진행률
    - 공격 중이면 기존 이유
  - `BeginEquipmentUse`: 수리 대상이면 세션을 시작하고 `StartAttack`을 호출하지 않는다(타격 없음, MASS-005).
  - `UpdateEquipmentUse`:
    - 매 호출마다 focus Actor가 세션 target과 같고 여전히 수리가 필요한지 확인한다. 아니면 이유 없이 `Failed`로 끝내고 진행을 버린다(MASS-006).
    - `RepairSeconds`에 도달하면 `CommitWrenchRepair` 뒤 `Succeeded`를 반환한다.
  - `EndEquipmentUse`(LMB 뗌), `CancelEquipmentUse`(내려놓기·거치·컴퓨터·배치·EndPlay): 세션을 버린다. 다시 누르면 0부터다.
  - BP event `OnRepairActiveChanged(bool)`: 수리 표현은 Editor가 정한다.

## Equipment Merge (Q67 A)

[HeldTargetUseSystem.md](HeldTargetUseSystem.md) `MergeEquipmentQuery`를 바꾼다.

- 장비 LMB query가 visible(대걸레·렌치·배송 상자, 집게의 쓰레기 조준): 지금처럼 held-use Apply·Take 필드를 모두 지운다.
- 장비 LMB query가 `!bVisible`(집게로 쓰레기가 아닌 곳 조준):
  - Take 필드는 지운다.
  - Apply 필드는 이유 안내로만 남긴다. `bHeldApplyVisible`과 `HeldApplyFailureReason`은 유지하고, `bCanHeldApply=false`, `HeldApplyActionName` 비움.
  - 그래서 LMB 행에 대상이 요구하는 이유가 보인다(고장 안마의자 `몽키스패너가 필요합니다`, 수건 선반 `수건 바구니 필요` 등).
  - observer는 `bCan`을 함께 보므로 프리뷰·강조·뚜껑은 켜지지 않는다.
- `BeginEquipmentUse`의 무보고 종료(장비 `!bVisible`, 이유 비움): 합성 query에 Apply 이유가 있으면 그 이유로 실패 result를 보고한다(intent `EquipmentUse`, LMB 행). 행동은 없다.
- 때수건은 장비가 아니다. 기존 held-use 경로로 대상의 이유가 보인다(SCRB-019, MASS-015).

## Television

`ATelevisionActor`(`BP_Television`, FacilityType `Television`, slot 없음).

- `bPoweredOn`(Transient, 기본 false). 신규·재설치는 꺼짐이고 payload가 없다(J17, REST-003, 006).
- E: 빈손이면 `켜기`/`끄기`, 아니면 이유 `빈손으로 켜고 끌 수 있음`이다(REST-001, 002). 실행은 재검증 → 반전 → BP `OnPowerChanged(bool)`이다.
- TargetName `TV · 켜짐/꺼짐`. 회수 Hold 취소는 전원을 바꾸지 않는다.

## Rest Bench

- `BP_RestBench`: parent `ABathhouseFacilityActor`, FacilityType `RestBench`, slot 3개(J16, Editor 조정).
- 좌석 점유, 회수 불가, 쓰러짐 Reserved 유지는 기존 slot·회수 규칙 그대로다(REST-004, 005). 새 코드는 없다.

## Scrub Towel

`AScrubTowelActor`(`BP_ScrubTowel`). 구현: `IPlayerInteractable`, `IPhysicalCarryable`. 장비가 아니다.

- `EPhysicalCarryKind::ScrubTowel`(append). 기본 `FreeDrop|FixedSlot`이다.
- exact fixed slot, held-pose drop, CCD·Pawn Ignore, 거치대 우선 복구는 `AWetMopActor`의 carry 부분과 같은 구조다. 공통 부모는 만들지 않는다.
- 버리기 계약을 구현하지 않아 수거되지 않는다(SCRB-017). 소모가 없다.
- E 빈손 들기, 표시 이름 `때수건`이다.
- LMB에 자기 행동이 없다. 세신대가 아닌 곳에서는 대상의 held-use 이유만 보인다.

## Scrub Table

`AScrubTableActor`(`BP_ScrubTable`, FacilityType `ScrubTable`).

- native default subobject(위치·크기는 Editor authoring):
  - `ScrubCamera`(`UCameraComponent`, 세신 시점)
  - `ScrubArea`(`UBoxComponent`, NoCollision·Navigation off. local XY extent가 때수건 이동 범위, local +Z가 표면 법선)
  - `ScrubCursor`(`UStaticMeshComponent`, NoCollision, 기본 hidden. 포커스 중 커서 위치에 표시)
  - `ScrubExitPoint`(`USceneComponent`, 발바닥 위치·방향)
  - `CashOfferPoint`(현금 생성 transform)
  - `CashStandPoint`(현금을 내미는 사용자 위치)
- slot: Blueprint slot 정확히 1개. 누운 자리는 slot action transform이다.
- authoring(EditDefaultsOnly):
  - `ScrubFee` 20000, `WaitLimitSeconds` 90
  - `RequiredRubDistanceCm`(커서 이동 거리 합, PIE 조정, 기본 3000)
  - `RubCmPerInputUnit` 1.0
  - `ExitSearchRadiusCm` 100
  - `FocusBlendInSeconds` 0.35, `FocusBlendOutSeconds` 0.25(컴퓨터 기본과 같음)
  - `CashOfferClass`(`ABathhouseCashPaymentActor` 자식. Editor 기본은 기존 `/Game/Bathhouse/Blueprints/Economy/BP_BathhouseCashPayment`)
- 상태(Transient): 현재 누운 사용자 weak, `WaitDeadline`(world time), `RubDistanceCm`, `ActiveScrubber`(포커스 중 플레이어 component weak).
- 누움 흐름:
  1. slot → `Occupied(User)`: 대기 마감 = 현재 + `WaitLimitSeconds`, 게이지 0, 사용자 `OnEndPlay`를 구독한다. 대기 timer는 마감 시각에 한 번 실행한다.
  2. `Occupied`를 벗어남(쓰러짐): timer를 지우고 게이지 0으로 만든다. 포커스 중이면 세션을 끝낸다. 다시 `Occupied`면 1부터다(SCRB-020).
  3. 대기 만료:
     - 게이지를 버리고 slot `EndUse` → `Release`
     - 세션 종료 통지
     - 사용자 `HandleServiceUseEnded(Abandoned)`(SCRB-009, 018)
  4. 사용자 EndPlay: `ForceRelease`, 세션 종료.
- 문지르기: `AddRubDistance(UPlayerScrubFocusComponent&, float Cm)`.
  - `ActiveScrubber`와 누운 사용자를 확인한 뒤 누적한다. 게이지 = `RubDistanceCm / RequiredRubDistanceCm`이다.
  - 1 도달 시 완료 commit:
    1. `CashOfferClass`를 `CashOfferPoint`에 deferred spawn → `ConfigurePaymentAmount(ScrubFee)` → Finish. 실패하면 게이지를 1 직전 값으로 두고 오류 로그를 남긴다. 완료하지 않는다.
    2. slot `EndUse` → `Release`(세신대가 빔, SCRB-014)
    3. 세션 종료 통지
    4. 사용자 `HandleScrubCashOffered(Table, Cash, CashStandPoint transform)`
  - 현금은 세신대와 독립된 Actor다. 세신대가 회수돼도 남는다.
- 세신자 예약: `TryBeginScrubSession(Component)` / `EndScrubSession(Component)`. 한 번에 한 플레이어다.
- E query(`IPlayerInteractable`):
  - TargetName: 누운 사용자가 있으면 `세신대 · 대기 72초 · 40%`, 없으면 `세신대`(SCRB-001).
  - 행동: 때수건(carry kind `ScrubTowel`)을 들었고 누운 사용자가 있으며 세신자가 없으면 `세신`이다.
  - 이유: 때수건이 아니면 `때수건이 필요합니다`(조준만으로 보임, SCRB-003), 사용자가 없으면 `세신할 손님 없음`(SCRB-004)이다.
  - execute: 재검증 → interactor의 `UPlayerScrubFocusComponent::BeginScrubFocus(Table)`
- 세션 종료 통지: native `OnScrubSessionEnded`(C++ multicast). 포커스 component가 구독한다.
- `EndPlay`(파괴·회수): 세션을 강제 종료하고 사용자를 해제한다. 회수는 slot이 Available일 때만 가능하다.

## Scrub Focus Session

`UPlayerScrubFocusComponent`(Character default subobject). 컴퓨터 포커스와 같은 규칙이지만 `UPlayerComputerUseComponent`는 수정하지 않는다.

- 재사용과 비재사용:
  - 컴퓨터 component는 화면 widget·커서·AA·예약이 섞인 487줄이고 PIE 승인된 흐름이다. 그래서 공통 부모로 분해하지 않는다.
  - 이탈 위치 탐색은 기존 private helper `FComputerFocusExitPlacement::Resolve`를 그대로 호출한다. 이름은 유지하며 범용 인자 helper다.
  - 시점 blend·이동 정지·suppression 절차는 컴퓨터와 같은 순서로 이 component에 둔다. 두 포커스의 공통화는 이후 세 번째 포커스가 생길 때 검토한다.
- phase: `Inactive → FocusingIn → Active → FocusingOut`. 컴퓨터와 같은 snapshot·`ForceCleanup` 대칭이다.
- `BeginScrubFocus(Table)`:
  - 조건: 로컬 제어, 때수건 소지, 컴퓨터 포커스 아님, `TryBeginScrubSession` 성공.
  - 순서:
    1. 이동 정지·snapshot
    2. `SetInteractionSuppressed(true)`
    3. `SetViewTargetWithBlend(Table, BlendIn)`
    4. 커서를 `ScrubArea` 중심에 두고 `ScrubCursor`를 표시
- Active 입력(Character가 전달):
  - `AddRubInput(FVector2D LookAxis)`: 커서 local XY에 `LookAxis × RubCmPerInputUnit`(X→local +Y, Y→local +X)을 더하고 area extent로 clamp한다.
  - LMB가 눌려 있으면 clamp 뒤 실제 이동 거리를 `Table->AddRubDistance`로 보낸다(SCRB-005, 006). 커서 world transform을 갱신한다.
  - `SetRubbing(bool)`: LMB 누름·뗌
- 종료:
  - `RequestEndScrubFocus()`(E·ESC, 진입 E의 release·중복 입력 무시, SCRB-011)
  - 세신대 통지(완료·만료·사용자 상실, SCRB-008, 009)
  - 절차: 커서를 숨기고 `ScrubExitPoint`·`ExitSearchRadiusCm`로 `FComputerFocusExitPlacement::Resolve`(SCRB-010) → 캐릭터 이동·회전 → `SetViewTargetWithBlend(Pawn, BlendOut)` → 완료 시 suppression 해제, 이동 복원, `EndScrubSession`.
  - 세신대 파괴: 이탈 위치 없이 view·이동·suppression을 즉시 복원한다.
- 게이지는 세신대에 있으므로 중간 이탈 뒤 유지되고, 재진입하면 이어진다(SCRB-007). 때수건은 손에 남는다.
- 포커스 HUD: `UScrubFocusHudWidget`.
  - `ABathhouseHUD`가 money widget처럼 생성하고 possessed pawn의 component를 주입한다.
  - BindWidget: `ScrubGaugeBar`(UProgressBar), `ScrubWaitText`(UTextBlock, `대기 72초`)
  - Active 동안만 보이고, NativeTick이 세신대 상태를 읽는다.

## Character Input Routing

`AFirstPersonCharacter`:

- private `IsFocusCapturingInput()` = 컴퓨터 capture || 세신 phase가 Inactive 아님. 기존 컴퓨터 전용 차단 검사(이동, 점프, sprint, F, G, Q, RMB, placement 입력)를 이 함수로 바꾼다. 컴퓨터 쪽 결과는 같다.
- E Started:
  - 컴퓨터 phase가 Inactive가 아니면 기존 처리
  - 세신 phase가 Inactive가 아니면 세신 종료 intent로 소비. Completed/Canceled는 세신이 press를 소유하면 소비
  - 그 밖은 기존 처리
- ESC(`IA_Cancel`): 컴퓨터, 그다음 세신 순서로 종료를 요청한다.
- Look: 세신 Active면 시점을 돌리지 않고 `AddRubInput`으로 보낸다.
- LMB: owner에 `Scrub`를 추가한다. 순서는 Computer > Scrub > Placement > 기존이다. Started → `SetRubbing(true)`, End → `SetRubbing(false)`.
- 세신 중 컴퓨터·배치 진입은 suppression과 이 차단으로 막힌다(SCRB-016).

## Test User And Debug Commands

`AServiceTestUserActor`(`BP_ServiceTestUser`). 임시 mesh, collision 없음, 물리·이동·피격 반응 없음.

- `IServiceFacilityUser`를 구현한다.
  - `Completed`·`Abandoned`: Destroy
  - `HandleScrubCashOffered`: `StandTransform`으로 teleport하고 현금 `OnCashClaimed`를 구독해, 받으면 Destroy(SCRB-008, 015)
- 비 shipping 콘솔(`ServiceAmenityDebugCommands.cpp`, `#if !UE_BUILD_SHIPPING`, SVC4-003). 대상은 로컬 플레이어가 조준한 설비다.
  - `bathhouse.Debug.Service.SpawnTestUser`: `IsAvailableForReservation` → 빈 slot `TryReserve` → 인형을 action transform에 spawn → `BeginUse`. 실패하면 인형을 제거한다. 안마의자 이용, 평상 좌석, 세신대 누움에 같은 명령을 쓴다.
  - `bathhouse.Debug.Service.KnockdownTestUser`: Occupied 인형 하나를 `EndUse`한다(쓰러짐 대역).
  - `bathhouse.Debug.Service.StandUpTestUser`: Reserved 인형 하나를 `BeginUse`한다(기립 대역).
  - `bathhouse.Debug.Service.RemoveTestUser`: 인형 하나를 Destroy한다(사라짐 대역).
- 기존 `bathhouse.Debug.Facility.BeginUse/EndUse`(로컬 pawn 사용자)는 그대로 쓸 수 있다.

## Blueprint/API And Editor Contracts

- 신규 reflected:
  - 위 class들과 property, component 이름
  - `EBathhouseFacilityType`·`EPhysicalCarryKind` append
  - `UMassageChairPlacementInstanceData`
  - widget BindWidget 두 개
  - `ABathhouseHUD::ScrubFocusHudWidgetClass`(EditDefaultsOnly)
- 기존 symbol rename·삭제는 없다. Core Redirect가 필요 없다.
  - `IPlaceableFacility`에는 기본 구현 virtual 추가만 한다.
  - `ABathhouseFacilityActor`에는 Transient 필드만 추가한다.
- copy-first load gate:
  - `/Game/FirstPersonCharacter/BP_FirstPersonCharacter`(새 default subobject)
  - `/Game/Bathhouse/Blueprints/Game/BP_BathhouseHUD`
  - `/Game/Bathhouse/Blueprints/Combat/BP_MonkeyWrench`
  - 기존 facility BP 대표 `BP_Shower`(base Transient 필드 추가)
- Editor 신규:
  - `/Game/Bathhouse/Blueprints/Service/`: `BP_MassageChair`, `BP_RestBench`, `BP_Television`, `BP_ScrubTable`, `BP_ScrubTowel`, `BP_ServiceTestUser`
  - `/Game/Bathhouse/UI/WBP_ScrubFocusHud`
  - Definition 네 개(`Facility.Placeable`·`Facility.Discardable`)
  - `DA_ShopCatalog` 4상품(60,000·15,000·25,000·20,000)
  - DefaultMap: 때수건과 `BP_PhysicalCarryFixedSlot` 1개
  - HUD class에 위젯 class 지정

## Verification

| 시나리오 | 자동화 |
|---|---|
| MASS-001, 003, 004, 013 | 테스트 사용자 Actor로 예약·BeginUse → timer 60초 진행 → 동전함 +3,000, 확률 0·100에서 고장 없음·항상, 고장 시 예약 불가 |
| MASS-002, 010, 014 | E 수금, 0원 이유, 반복 E 1회, 들고 있는 물건 무관 |
| MASS-005~007 | 렌치 Hold 3초 수리, 타격 없음, 조준 이탈·뗌 초기화, 정상 의자 휘두르기 |
| MASS-008, 009 | 실제 conversion 경로: 회수 시 수행자 지갑 +금액, 고장 payload 보존·요약, Q 취소 무변화, 이용 중 회수 거부 |
| MASS-011 | EndUse(쓰러짐) → 요금 없음, Reserved 유지, BeginUse 재시작 60초 / 사용자 Destroy → Available |
| MASS-012, 015, SCRB-019 | 빈손·박스·집게·때수건의 LMB 이유, 대걸레·배송 상자 기존 행, 집게 비대상 LMB 무행동 |
| REST-001~006 | TV 빈손 E 전환·이유·재설치 꺼짐, 평상 3석 점유·회수 거부 |
| SCRB-001~004 | 세신대 query: 대기·게이지 문구, `세신`·이유 |
| SCRB-005~011, 016 | 포커스 component: 진입 조건, LMB 없이 이동 시 게이지 불변, 누른 채 이동량 = clamp 뒤 거리, 이탈·재진입 유지, 완료·만료 자동 이탈, 이탈 위치·막힘 탐색, 차단 입력 |
| SCRB-008, 014, 015 | 완료 → 현금 spawn·slot 비움·인형 이동, 새 인형 누움 가능, 현금 1회 지급 후 인형 제거 |
| SCRB-009, 012, 018, 020 | 만료 시 무요금·제거, 누운 동안 회수 거부, 쓰러짐 게이지 0·재누움 90초 |
| SCRB-013, 017 | 때수건 거치대·drop·복구, 수거 대상 아님 |
| SVC4-002, 003 | 네 설비 아이템 world 버리기 가능, 콘솔 명령 non-shipping 전용 |
| 회귀 | 컴퓨터 포커스 전체, Combat, Interaction held-use, 3단위 집게(Q67 변경 외), Placement, Service·Shop 전체 |

PIE: 대표 시나리오, 세신 시점·커서 범위·문지르기 감도(`RequiredRubDistanceCm` 결정), 고장 외형, TV 화면, 포커스 HUD.

## Dependencies

- Service → Facility(base actor·slot), Placement(instigator·payload), Interaction(carry·held-use·suppression), Economy(wallet·현금 Actor), Combat(`IWrenchRepairable`), Computer(exit placement helper)
- Combat는 Service를 모른다. Placement는 Economy를 모른다.
- Character → Service(`UPlayerScrubFocusComponent`)
- UI → Service(포커스 HUD)
- 새 module은 없다.
