# Customer System

## Implementation Status

이 문서는 현재 customer loop와 UE 5.8 StateTree 계약을 정의한다. native session은 신발 단계를 deprecated 처리하고 unnumbered locker 활동, capacity lease와 수위 유효성 기반 BathLoop를 구현한다. Facility 경계는 [FacilitySystem.md](FacilitySystem.md), 물과 조작부는 [BathWaterSystem.md](BathWaterSystem.md), 배치/수용량은 [PlacementSystem.md](PlacementSystem.md), knockdown은 [CustomerRecoverySystem.md](CustomerRecoverySystem.md)를 따른다.

## Source Scope

```text
Source/BathhouseSim/Public/Customer/
  BathhouseCustomerTypes.h
  CustomerRoutineDefinition.h
  CustomerSessionComponent.h
  CustomerQueueNavigationComponent.h
  CustomerMontagePlaybackComponent.h
  CustomerBathLoopLog.h
  BathhouseCustomerCharacter.h
  BathhouseCustomerAIController.h
  BathhouseCustomerSpawner.h
  StateTree/CustomerStateTreeTasks.h
  StateTree/CustomerQueueStateTreeTasks.h
  StateTree/CustomerStateTreeConditions.h
  StateTree/CustomerTowelStateTreeTasks.h

Source/BathhouseSim/Private/Customer/
  BathhouseCustomerTypes.cpp
  CustomerRoutineDefinition.cpp
  CustomerSessionComponent.cpp
  CustomerSessionBath.cpp
  CustomerBathLoopLog.cpp
  CustomerQueueNavigationComponent.cpp
  CustomerMontagePlaybackComponent.cpp
  BathhouseCustomerCharacter.cpp
  BathhouseCustomerAIController.cpp
  BathhouseCustomerSpawner.cpp
  StateTree/CustomerStateTreeTasks.cpp
  StateTree/CustomerQueueStateTreeTasks.cpp
  StateTree/CustomerStateTreeConditions.cpp
  StateTree/CustomerTowelStateTreeTasks.cpp

Source/BathhouseSim/Private/Tests/
  BathhouseDomainTests.cpp  # Bath snap cleanup, montage candidate와 playback token coverage
  CleaningTowelAutomationTests.cpp  # towel acquire/shortage/return/interruption coverage
```

## Responsibilities

- customer별 key/session, queue, facility reservation과 bath stay 상태
- StateTree 기반 routine orchestration과 gameplay event 전달
- check-in 60초 timeout과 미응대 퇴장
- unnumbered locker, shower, random bath loop, checkout과 정상 퇴장
- 전역 임계 수위 기반 bath 탐색·예약·강제 퇴장, 탐색 제한시간과 실제 입욕 누적
- 완료·timeout·기술 실패의 대칭 cleanup
- clean towel token 획득·사용·반납과 shortage fallback
- customer session satisfaction와 towel cleanup
- Combat health component 조립과 Customer Recovery soft-interruption 통합

Customer는 towel endpoint count/overflow, facility slot, key actor lifecycle, player carry, wallet과 UI 상태를 소유하지 않는다.

## State And Execution Owners

| 책임 | Owner |
|---|---|
| 현재 routine state와 transition | `ST_CustomerRoutine` StateTree |
| key token reference, clothes/lease, timer와 runtime handles | `UCustomerSessionComponent` |
| 일반 navigation request | native `FCustomerRestartableMoveToTask`와 AIController |
| queue 이동·도착 회전·overflow wander | `UCustomerQueueNavigationComponent`; Counter assignment를 읽고 mutation하지 않음 |
| queue | `ABathhouseCounterActor` |
| facility reservation/occupancy | `UBathhouseFacilitySlotComponent` |
| Bath approach/action snap 상태와 복구 | `UCustomerSessionComponent` |
| Bath 탐색 window, 실제 입욕 누적과 현재 Bath usability 구독 | `UCustomerSessionComponent` |
| 욕탕 물·전역 입욕 임계치와 usability event | Bath Water System |
| montage 재생, playback token과 종료 결과 | `UCustomerMontagePlaybackComponent` |
| key state | `ABathhouseKeyActor` |
| money | `UPlayerWalletComponent` |
| timed activity 실행 | native Customer StateTree Task |
| customer-held towel token과 satisfaction | `UCustomerSessionComponent` |
| towel count, overflow와 transfer | Towel System |
| health/depleted | Combat `UHealthComponent` |
| ragdoll/soft interruption/restart serial | Customer Recovery components |
| 설치 locker capacity와 active lease registry | `ULockerCapacitySubsystem` |

## `UCustomerRoutineDefinition`

공용 Data Asset으로 NPC 행동 시간과 random 범위를 소유한다.

- `CheckInTimeoutSeconds = 60`
- `BathStayDurationSeconds = 60`
- `BathSearchTimeoutSeconds = 10`
- `BathDwellMinSeconds = 10`
- `BathDwellMaxSeconds = 20`
- undress, pre-shower, main shower, drying, towel return과 dress 시간
- facility retry 간격과 navigation 최대 재시도
- queue 이동 허용 반경, 도착 Yaw 회전 속도와 허용 오차
- checkout overflow wander 도착 반경과 지점 재선택 대기 범위
- `UsageFee = 10000`
- towel availability wait limit
- towel unavailable satisfaction penalty

값은 Editor에서 조정하지만 runtime 도중 시설이나 Widget이 변경하지 않는다. 향후 성격·만족도 같은 인과요인은 별도 modifier로 추가하며 현재는 고정 bath stay 값을 그대로 사용한다.

기존 `StoreShoesSeconds`, `WearShoesSeconds`는 StateTree asset migration 동안 deprecated reflected property로 보존하되 신규 routine에서 읽지 않는다.

## `UCustomerSessionComponent`

- assigned `ABathhouseKeyActor`와 token 표시용 `KeyNumber`; locker lookup에는 사용하지 않음
- opaque locker capacity lease handle과 idempotent release guard
- `ClothesStored`; locker slot은 의류 inventory를 소유하지 않음
- current counter lane/queue handle
- current facility slot reservation
- current Bath action-point snap 여부, 예약 당시 발바닥 기준 approach/action transform과 collision-independent snap/return
- last used bath actor
- bath stay end time와 expiry timer
- 현재 bath search end time/expiry timer와 반복 retry에 리셋되지 않는 search serial
- 실제 입욕 누적 초, 활성 입욕 segment 시작시각과 idempotent 종료 guard
- 현재 예약 Bath의 usability delegate와 forced-exit pending reason
- current logical activity와 service interaction gate
- cash claimed, departure reason와 cleanup guard
- optional `FTowelUseHandle`, towel-use stage와 towel cleanup guard
- current satisfaction value
- check-in wait와 checkout offer의 non-interruptible queue-service guard

Session은 queue lane membership을 보유하지만 index/assignment/배회 위치를 복제하지 않는다. 최신 assignment는 Counter에 위임하고 StateTree Task가 session API를 통해 transaction을 수행한다.

전체 Bath timer가 만료되면 session은 `Customer.Event.BathStayExpired`, 현재 탐색 window가 만료되면 `Customer.Event.BathSearchExpired`, 예약·이동·사용 중인 Bath가 임계치 미만이 되면 `Customer.Event.BathBecameUnusable`를 StateTree에 전달한다. event 전송 전 실제 입욕 segment를 중지해 StateTree 반응 frame이 누적시간에 포함되지 않게 한다.

## `ABathhouseCustomerCharacter`

- customer Pawn의 composition root다.
- private `CustomerSession` default subobject로 `UCustomerSessionComponent`를 생성하고 외부 C++에는 `GetCustomerSession()` 접근만 제공한다.
- private `CustomerMontagePlayback` default subobject로 montage lifecycle을 조립하며 gameplay session state와 분리한다.
- `CustomerQueueNavigation` default subobject로 queue AI request, 도착 회전과 overflow wander lifecycle을 조립한다.
- `Health`, `CustomerKnockdown`, `CustomerRoutineInterruption` private default subobject를 조립하고 각 책임을 getter로만 노출한다.
- `CustomerSession`은 `VisibleAnywhere`, `BlueprintReadOnly`, `AllowPrivateAccess` 계약으로 Blueprint와 StateTree의 읽기 binding을 유지한다.
- check-in 중 `IPlayerInteractable`을 구현하고 session에 query/execute를 위임한다.
- logical activity 변경을 Blueprint 표현 event로 전달한다.

Blueprint event:

- `OnActivityStarted(ActivityType)`
- `OnActivityFinished(ActivityType)`
- `OnCustomerPresentationStateChanged(PresentationState)`
- `OnCustomerSatisfactionChanged(PreviousSatisfaction, NewSatisfaction)`

`UCustomerMontagePlaybackComponent`는 AnimInstance, 현재 montage, monotonic playback token과 종료 결과를 소유한다. StateTree Task는 token으로 자신이 시작한 montage만 조회·중단한다. AnimNotify, Motion Warping, prop animation과 신발·의상 전환은 이번 target에 포함하지 않는다.

## `ABathhouseCustomerAIController`

- UE 5.8 `UStateTreeAIComponent`를 brain component로 사용한다.
- `UStateTreeAIComponentSchema` 기반 `ST_CustomerRoutine` asset을 실행한다.
- navigation과 StateTree lifecycle만 담당한다.
- key, facility, timer와 cash 상태를 소유하지 않는다.

필요 module/plugin:

- `StateTree`
- `GameplayStateTree`
- `StateTreeModule`
- `GameplayStateTreeModule`
- `AIModule`, `GameplayTasks`, `NavigationSystem`, `GameplayTags`

## StateTree Boundary

StateTree asset이 소유하는 것:

- state hierarchy와 transition
- native Task/Condition 배치와 binding
- key received, timeout, facility available, bath expired, cash claimed event 전이
- towel available과 towel wait expired event 전이
- Bath approach 이동, action snap, montage 실행과 approach 복귀 순서
- Bath search-expired/invalidated event의 재탐색·다음 shower 전이와 한번 시작한 퇴장 유지

Native C++이 소유하는 것:

- Task/Condition 구현
- session transaction과 cleanup
- cached Bath 발바닥 transform에 scaled capsule half height를 한 번 더하는 actor/capsule-center 변환과 unswept teleport
- queue/facility/key/wallet API 호출
- locker capacity lease와 random action-slot API 호출
- gameplay event 발행
- montage 후보 검증, 단일 선택과 실제 playback 종료 판정
- soft interruption serial, restartable MoveTo와 기존 Task local restart
- queue revision 기반 assignment 재조회와 recovery queue-pose gate
- Bath search timer, current Bath delegate, 실제 입욕 segment와 진단 reason 기록

Blueprint StateTree Task와 Blueprint graph에 domain mutation을 구현하지 않는다.

## Native StateTree Tasks

- queue join/leave와 front 도착 대기
- `FCustomerMoveToCurrentQueueAssignmentTask`: service/queue/overflow를 한 실행으로 처리하고 service point 위치·Yaw 정렬 뒤에만 성공
- check-in key 대기와 timeout 시작·취소
- facility/slot 선택·예약·release
- 탈의/착의용 random locker action-slot 선택과 행동 단위 release
- native `FCustomerRestartableMoveToTask`는 기존 목적지 binding을 수용하며, `ST_CustomerRoutine` asset의 built-in `FStateTreeMoveToTask` 교체는 Editor 단계에서 수행
- Bath action/approach point의 collision-independent snap과 movement mode 복구
- logical activity begin, finish와 timer-only fallback
- 후보 중 하나를 한 번 선택해 실제 종료를 기다리는 one-shot montage
- 후보 중 하나를 한 번 선택해 같은 montage만 지정 시간 동안 반복하는 duration-loop montage
- bath stay timer 시작과 random bath loop
- Bath 탐색 window 시작/완료, pre-entry 수위 재검증과 임계 하락 exit-pending 처리
- checkout key 배치와 cash actor 생성·claim 대기
- normal/timeout/technical cleanup
- clean towel acquire/wait/fallback, mark-used와 used return Task

조건은 session과 owner API를 읽기만 하고 상태를 바꾸지 않는다.

Queue removal은 session membership와 wait/service guard를 먼저 지운 뒤 counter에서 dequeue한다. Counter의 동기 lane broadcast는 남은 customer와 queue navigation component에 전달하지만, 이미 떠나는 customer와 active check-in wait/checkout offer에는 StateTree event를 보내지 않는다. Check-in wait 시작은 idempotent하다. Knockdown soft pause는 StateTree reselect/exit가 아니며 queue, facility와 checkout cleanup을 호출하지 않는다.

## Full Routine Flow

1. Spawner가 entry에 customer를 생성하고 routine definition/counter를 주입한다.
2. Customer가 check-in lane에 enqueue하고 최신 assignment 위치로 이동한 뒤 authored Yaw로 정렬한다.
3. Front service point 위치·회전 완료 후 `WaitingForKey`가 되고 60초 timeout을 시작한다.
4. Player가 유효한 physical key를 주면 capacity lease, key와 session을 한 transaction으로 commit하고 timeout을 취소한다.
5. Timeout이면 check-in lane을 떠나 exit로 이동한 뒤 소멸한다.
6. random available locker action slot을 탈의 동안만 reserve/use하고 완료 시 `ClothesStored=true`로 commit한 뒤 release한다.
7. `TowelShelf` slot에서 clean towel 한 장 획득을 시도한다.
8. 없으면 authorable limit 동안 availability를 기다리고 만료 시 towel 없이 진행하며 satisfaction을 감소시킨다.
9. Shower slot의 pre-shower 완료 순간 고정 60초 전체 bath stay timer를 시작한다.
10. 최대 `min(10초, 남은 전체시간)`인 새 search window를 시작하고 전역 임계 수위 이상인 Bath의 available slot만 random 예약한다. 반복 조회는 같은 window를 리셋하지 않는다.
11. 예약 성공 시 search window를 끝내고 NavMesh 위 approach point로 이동한다. 이동 중 임계 미달이면 event로 이동을 중단하고 reservation을 정리한 뒤 새 window를 시작한다.
12. approach 도착 후 수위를 다시 검사하고, 유효할 때만 action point로 unswept snap해 occupancy와 실제 입욕 segment를 시작한다.
13. 탕마다 `10~20초`와 남은 전체시간 중 짧은 duration 동안 선택 montage 하나만 반복한다. 실제 입욕 segment만 누적한다.
14. 임계 미달은 segment를 즉시 중지하고 퇴장을 시작한다. dwell/강제퇴장/전체 만료 모두 approach 복귀 뒤 slot을 release하며 이미 시작한 퇴장은 수위 상승으로 취소하지 않는다.
15. 시간이 남은 정상 dwell 또는 강제퇴장 뒤에는 새 search window로 돌아가고, search/전체 timer 만료 시 만족도 변화 없이 Shower slot의 main-shower로 진행한다.
16. towel handle이 있으면 Drying에서 Used로 mark하고 기존 `TowelBasket` facility의 used bin에 반환한다.
17. used bin full이면 주변 floor의 individual used towel, spawn 불가면 PendingSpill ledger로 보존한다.
18. towel handle이 없으면 towel-dependent drying/return을 건너뛴다.
19. 그 시점의 random available locker action slot을 착의 동안만 사용하고 `ClothesStored=false`로 commit한 뒤 release한다.
20. Checkout lane에 enqueue한다. visible capacity 안에서는 고유 service/queue point로 이동·정렬하고 초과 시 FIFO를 유지한 채 전용 volume을 배회한다.
21. service point 도착 후 동일 key를 counter에서 `OnCounter`로 전환하고 cash actor를 제시한다.
22. Cash claim 성공 시 capacity lease를 release하고 checkout lane을 떠나 exit로 이동한 뒤 소멸한다.

Player의 key pickup/rack 반환은 customer 퇴장 조건이 아니다.

## BathLoop Timer And Invalidation

`UCustomerSessionComponent`는 전체 60초 timer, 현재 search window와 실제 입욕 누적을 서로 다른 값으로 소유한다. `BeginBathSearchWindow()`는 이미 active면 no-op이고 reservation 성공·포기에서 명시적으로 끝난 뒤에만 다음 window가 새 serial로 시작한다. timeout은 Data Asset의 `BathSearchTimeoutSeconds`와 남은 전체시간 중 작은 값이다. search 시작 시 이 resolved duration을 별도로 보존하고 active elapsed는 `resolved duration - remaining`으로 계산하여 10초 기본값, 남은 stay clamp와 pause/resume을 정확히 구분한다.

실제 입욕시간은 Tick 누적이 아니라 segment start/stop의 world-time 차이로 합산한다. action point snap과 `BeginUse`가 성공한 뒤 시작하고 dwell 완료, 임계 하락, 전체 만료, knockdown suspend, abort와 EndPlay에서 idempotent하게 멈춘다. knockdown resume은 현재 Bath가 계속 usable일 때만 새 segment를 시작한다. search/이동/입퇴장과 paused interval은 포함되지 않는다.

Session은 Bath reservation 수명에만 `OnCustomerUsabilityChanged`를 구독한다. 임계 하락 시 이동 전이면 reservation cleanup event, 사용 중이면 실제 segment stop과 exit-pending event를 보낸다. current activity exit은 Bath에서 즉시 slot을 풀지 않고 cached ApproachPoint 복귀 뒤 기존 facility-root cleanup으로 release한다. threshold가 다시 올라와도 pending exit을 되돌리지 않는다.

## BathLoop Diagnostics

`CustomerBathLoopLog.*`의 공유 `LogBathhouseCustomerBath` category를 session과 native StateTree Task가 사용한다. native Task는 Bath navigation 결과, action snap/begin-use validation, search/stay/dwell advance와 forced-exit approach cleanup 결과를 서로 다른 phase/reason으로 남긴다. terminal transition만 기록하고 Tick, water frame update와 Condition 반복 평가는 기본 로그에 남기지 않는다.

- `Log`: loop/search/dwell 시작·완료, reservation 성공, 임계 하락 퇴장, timeout과 다음 단계
- `Verbose`: 후보 집계/거부, retry, 이동 결과와 입장 직전 재검증
- `Warning`: navigation/recovery처럼 재시도 가능한 실패
- `Error`: reservation 없는 dwell, foreign slot release, 중복 timer/segment 같은 invariant 위반

모든 항목은 가능한 범위에서 `Customer`, loop `Iteration`, `Phase`, `BathActor`, `Slot`, `WaterPercent`, `ThresholdPercent`, 전체 남은시간, 실제 누적시간, search elapsed, `Result`와 `Reason`을 포함한다. 공통 reason enum은 `NoBathCandidate`, `WaterBelowThreshold`, `NoAvailableSlot`, `ReservationLost`, `NavigationFailed`, `EntryValidationFailed`, `DwellCompleted`, `BathStayExpired`, `SearchExpired`, `KnockdownInterrupted`, `StateTreeExited`, `TechnicalFailure`를 구분한다. Blueprint `Print String`은 사용하지 않고 graph 자체 추적은 `LogStateTree VeryVerbose`와 StateTree Debugger를 병행한다.

## Check-In Transaction

- check-in lane front customer만 key query를 제공한다.
- player가 자신의 exact hook과 연결된 `HeldByPlayer` key를 들고 있어야 한다. locker 번호 topology는 검사하지 않는다.
- `ULockerCapacitySubsystem`이 provisional lease를 확보한 뒤 key를 `AssignedToCustomer`로 전이하고 player hand/session을 commit한다.
- key/session commit 실패는 lease와 key를 rollback하며 성공 후 key number는 token identity/표시에만 남는다.
- key receive와 timeout이 같은 frame에 경쟁하면 game thread에서 먼저 commit한 terminal event만 유효하다.

## Checkout Queue And Key Transaction

- checkout queue는 visible lane과 overflow를 나누는 두 배열이 아니라 Counter의 단일 FIFO다.
- `FCustomerMoveToCurrentQueueAssignmentTask`는 queue point에서 성공하지 않고 revision을 기다리며, service point 위치·Yaw 정렬 후에만 checkout offer로 전이한다.
- 반환 시 새 key를 spawn하지 않고 session의 동일한 `AssignedKey`를 Counter drop 후보 transform으로 옮긴다.
- overlap 검사, free-world physics 적용과 `AssignedToCustomer -> OnCounter`를 한 key-owned transaction으로 commit한다. 실패하면 key/session과 cash offer 전 상태를 유지하고 authorable retry 간격 뒤 다시 시도한다.
- 성공한 `OnCounter` key는 player가 회수하지 않아도 cash claim과 customer 퇴장을 막지 않는다.

## Customer Towel Transaction

- clean towel 획득은 stack count 감소와 session `FTowelUseHandle` 생성이 한 transaction이다.
- handle은 token owner, original stack, used 여부와 terminal cleanup guard를 가진다.
- clean shortage wait 만료는 gameplay fallback이며 technical abort가 아니다.
- fallback은 towel-dependent 상태를 건너뛰고 authorable satisfaction penalty를 한 번 적용한다.
- used bin capacity는 clean acquire를 막지 않는다. full return은 individual floor overflow 또는 PendingSpill로 보존한다.
- session interruption 전 사용하지 않은 token은 original stack, 사용한 token은 bin/overflow/recovery ledger로 한 번만 이전한다.

## Activity And Montage Contract

animation을 사용하는 논리 행동은 다음 순서를 사용한다.

1. slot reserve
2. navigation target으로 이동하고 Bath면 발바닥 action point에 capsule 높이를 한 번 적용해 collision-independent unswept snap
3. `BeginActivity`와 `OnActivityStarted`
4. StateTree montage Task가 유효 후보를 필터링하고 EnterState에서 정확히 하나 선택
5. one-shot은 실제 montage 정상 종료, duration-loop는 같은 선택 montage의 지정 시간 반복을 완료 기준으로 사용
6. logical completion commit과 `OnActivityFinished`
7. Bath면 발바닥 approach point에 같은 capsule 높이 변환을 적용해 복귀
8. slot release

후보가 하나면 random 호출 없이 그 montage를 사용하고 후보가 없거나 재생할 수 없으면 Task가 실패한다. Loop Task는 정상 실행중 후보를 다시 선택하지 않는다. 정상 StateTree exit은 token owner의 montage를 blend-out하고 session cleanup을 실행한다. Knockdown soft interruption은 cleanup 없이 playback을 중지하고 기립 후 후보를 다시 선택해 local action을 처음부터 재시작한다. animation이 없는 상태는 기존 timer-only activity를 사용할 수 있다.

## Bath Snap Collision Policy

Bath ActionPoint와 ApproachPoint는 reservation-time cached 발바닥 transform을 사용하고 scaled capsule half height를 정확히 한 번 적용한다. ActionPoint snap은 `IsActionTransformClear` 또는 capsule overlap 사전 검사를 수행하지 않는다.

`SnapToCurrentFacilityActionPoint()`는 유효한 reservation/slot, cached transform, Character, capsule과 movement component만 검증한다. AI와 movement를 정지하고 movement mode를 저장한 뒤 `SetActorLocationAndRotation`의 `bSweep=false`, `ETeleportType::TeleportPhysics`로 정확한 transform을 적용한다. Blocking Volume, facility mesh 또는 다른 collision과 겹쳐도 그 사실만으로 snap을 실패시키거나 navigation failure를 증가시키지 않는다.

Snap 중 capsule/Actor collision enabled 상태와 response는 변경하지 않는다. 요구사항은 placement 검사 무시이며 고객을 facility 사용 전체 동안 ghost actor로 바꾸지 않는다. movement는 기존처럼 `MOVE_None`으로 유지하므로 사용 중 CharacterMovement가 위치를 수정하지 않는다.

ActionPoint에서 나올 때도 cached ApproachPoint로 unswept teleport하고 저장했던 movement mode를 복원한다. invalid owner/cache/component 또는 transform 적용 자체 실패만 technical failure다. release, normal StateTree exit, technical abort와 EndPlay cleanup은 같은 return/restore 경로를 사용한다. Knockdown은 이 cleanup 경로를 사용하지 않고 reservation을 유지한 채 ragdoll 최종 위치에서 ApproachPoint로 다시 이동한다.

## Technical Abort

Navigation이 설정된 횟수만큼 반복 실패하면 gameplay 분기가 아니라 technical abort로 처리한다.

- active check-in/bath-stay/bath-search/towel timer와 StateTree wait 취소, 실제 입욕 segment 중지
- Bath action point에 있으면 collision 사전 검사 없이 cached approach point 복귀와 movement mode 복구
- current slot release
- locker capacity lease를 idempotent하게 release
- queue entry 제거
- assigned key를 원래 hook으로 복구
- towel handle을 used stage에 따라 clean stack 또는 used bin/overflow/recovery ledger로 정리
- cash actor가 있으면 제거하되 이미 지급된 money는 되돌리지 않음
- 오류 기록 후 exit 이동 시도와 소멸

Check-in 외 gameplay timeout은 두지 않는다.

## Spawner

`ABathhouseCustomerSpawner`는 customer class, routine definition, counter, entry transform, spawn interval과 max active count를 authoring한다. Customer 완료 delegate로 active count를 정리하며 개별 routine phase를 직접 제어하지 않는다.

## Dependencies

- Customer -> Facility
- Customer -> Bath Water public query/usability delegate
- Customer -> Interaction
- Customer -> Economy
- Customer -> Towel
- Customer -> Placement/Facility locker capacity public API
- Customer -> Combat health/damage public 계약
- Customer -> UE 5.8 GameplayStateTree/AI/Navigation
- Customer montage playback -> Engine Animation/AnimInstance
- Customer는 UI concrete class에 의존하지 않는다.

## Manual Review Points

- check-in timeout이 front 도착 후 시작되고 key 수령 시 취소되는지 확인한다.
- check-in/checkout customer가 queue point의 위치뿐 아니라 Yaw까지 정렬하며, knockdown 기립 후 최신 assignment로 복귀한 뒤 routine을 재개하는지 확인한다.
- checkout overflow customer가 FIFO 순번을 잃지 않고 전용 volume 안을 배회하며 promotion 때 active wander를 중단하는지 확인한다.
- player가 준 key가 token으로 유지되지만 locker slot 선택과 번호 대응하지 않는지 확인한다.
- check-in key/lease가 함께 commit 또는 rollback되고 checkout/timeout/cleanup에서 lease가 한 번만 반환되는지 확인한다.
- 탈의/착의가 서로 독립된 random locker action slot을 행동 동안만 사용하고 `ClothesStored`를 session에만 기록하는지 확인한다.
- bath timer가 pre-shower 완료 시 시작하고 정확히 60초에 current montage를 중단한 뒤 approach 복귀와 release를 수행하는지 확인한다.
- search window가 retry마다 재시작되지 않고 `10초`와 남은 전체시간 중 작은 값에서 만료되며 진단 elapsed가 각각 실제 약 10초와 clamp된 시간으로 기록되는지 확인한다.
- 임계치 미만 Bath가 후보/이동/입장/체류에서 제외되고 사용 중 하락 시 실제 입욕 누적을 즉시 멈춘 뒤 approach 복귀 후 release하는지 확인한다.
- 실제 입욕 누적에 search/이동/입퇴장/knockdown이 들어가지 않고 여러 Bath segment만 합산되는지 확인한다.
- blocking collision이 action point를 점유해도 snap이 성공하고 정확한 cached transform, `MOVE_None`과 기존 collision enabled 상태를 유지하는지 확인한다.
- blocked action snap 후 정상 release/technical abort가 cached approach로 복귀하고 movement mode를 복원하는지 확인한다.
- bath random dwell과 다른 bath 선택이 고정 전체 입욕시간 종료를 지연하지 않는지 확인한다.
- BathLoop 로그가 iteration/phase/reason과 수위·타이머 상관 필드를 제공하면서 Tick spam을 만들지 않는지 확인한다.
- 모든 StateTree exit/abort에서 queue, slot, timer와 key가 정리되는지 확인한다.
- 동일한 assigned key가 counter 후보 위치에서 physics `OnCounter`로 전환되고 blocked drop은 key/session을 보존하며, cash claim 뒤 NPC가 key 회수를 기다리지 않고 퇴장하는지 확인한다.
- montage 후보가 0/1/여러 개인 경우 각각 failure/단일 선택/random 단일 선택으로 동작하는지 확인한다.
- duration-loop가 처음 선택한 montage만 반복하고 StateTree exit에서 다른 playback을 중단하지 않는지 확인한다.
- montage가 없는 timer-only 상태의 기존 logical loop가 유지되는지 확인한다.
- 신발 상태/시설/StateTree 전이가 제거되고 의상 mesh/visibility/AnimNotify는 이번 범위에 추가되지 않았는지 확인한다.
- `CustomerSession`이 외부 C++에서 직접 접근되지 않고 Blueprint/StateTree 읽기 binding과 public getter가 유지되는지 확인한다.
- clean towel shortage가 authorable wait 뒤 routine을 계속하고 satisfaction penalty를 한 번만 적용하는지 확인한다.
- used bin full이 customer를 막지 않고 individual overflow/PendingSpill로 token을 보존하는지 확인한다.
- customer StateTree exit/EndPlay에서 towel token owner가 중복되거나 사라지지 않는지 확인한다.
- knockdown soft pause가 StateTree `ExitState()`를 발생시키지 않고 session timer/자원/예약을 보존하는지 [CustomerRecoverySystem.md](CustomerRecoverySystem.md)의 수용 기준으로 확인한다.
