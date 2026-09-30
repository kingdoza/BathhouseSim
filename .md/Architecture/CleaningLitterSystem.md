# Cleaning Litter System

## Status And Scope

- [CleaningSystem.md](CleaningSystem.md)의 하위 문서다. 서비스 3단위(쓰레기·수거)의 쓰레기, 집게+봉투, 묶은 쓰레기봉투, 쓰레기 수거 구역, 인원 기반 생성, 배치 확정 시 발밑 정리를 소유한다.
- 2026-10-01 설계. Source 구현 완료. 같은 날 코드 리뷰 F1·F2로 Floor Rule 5단계를 충돌 응답 기반으로 재확정했다(재작업 반영 대기). 입력은 `.md/PROMPT_ARCHITECTURE.md`(TRSH-001~030, COLL-001~009)다. 기술 선택은 `.md/QNA_ARCHITECTURE.md` Q1 A(구역 가중치·종류 즉시 삭제)다.
- 수직 구현이다. 대표 흐름은 탈의실 구역 쓰레기 → 집게 줍기 → RMB 묶기 → 수거 구역 → 수거다.
- 비대상: 손님이 버리는 행동, 만족도, 날짜 시스템, 수거 시점 표시, 쓰레기통 코드 삭제, 저장.

## Source Scope

```text
Public/Cleaning/
  LitterActor.h                  ALitterActor, 쓰레기 한 개
  LitterSpawnZoneActor.h         ALitterSpawnZoneActor
  LitterTongsActor.h             ALitterTongsActor, 집게+봉투 레벨 도구
  TrashBagActor.h                ATrashBagActor, 묶은 쓰레기봉투
  TrashCollectionZoneActor.h     ATrashCollectionZoneActor
Private/Cleaning/
  (위 .cpp)
  CleaningSpawnRules.h/.cpp      순수·query helper: 발생 clock, 바닥 판정, footprint 겹침
  TrashBagDropPlacement.h/.cpp   봉투 정면 놓기 위치 계산
  TrashCollectionDebugCommands.cpp  비 shipping 수거 즉시 실행
Public/Interaction/
  HeldEquipmentSecondaryUsable.h 신규 C++ 전용 interface(RMB 장비 보조 사용)
Public/Placement/
  FacilityPlacementEventSubsystem.h  신규, 배치 확정 world 이벤트
```

수정: `CleaningDirectorActor`, `StainSpawnZoneActor`, `WaterStainActor`, `CleaningWorldSubsystem`, `CleaningTypes.h`(enum 삭제), `PhysicalCarryable.h`(kind append), `PhysicalCarryDiscardable.h`, `PlayerEquipmentUseComponent`, `FirstPersonCharacter`(RMB 분기), `FacilityActorConversionTransaction.cpp`(이벤트 한 줄), 버릴 수 있는 네 휴대물(품목 박스, 배송 상자, 설비 아이템, 봉투).

## Ownership

| 대상 | 상태 owner | 실행 owner |
|---|---|---|
| 쓰레기·얼룩 등록부, 발밑 정리 | `UCleaningWorldSubsystem` | subsystem |
| 종류별 전역 값, 구역별 발생 clock | `ACleaningDirectorActor` | director timer |
| 구역 범위·바닥 plane·구역 최대 | 각 spawn zone Actor | 읽기 전용 |
| 쓰레기 외형·종료 상태 | `ALitterActor` | 쓰레기 |
| 봉투 개수 | `ALitterTongsActor` | 집게 |
| 묶은 봉투 개수 | `ATrashBagActor` | 불변 |
| 수거 주기·대상 판정 | `ATrashCollectionZoneActor` | zone timer |
| 버릴 수 있는 종류 판정 | 각 휴대물(`IPhysicalCarryDiscardable`) | 휴대물 |
| RMB 입력 owner | `AFirstPersonCharacter` | `UPlayerEquipmentUseComponent` |

## Spawn Schedule (쓰레기·물 얼룩 공통)

`ACleaningDirectorActor`가 두 종류를 같은 규칙으로 돌린다. 기존 전역 timer와 가중치 구역 선택은 삭제한다.

- timer 간격 `SpawnUpdateIntervalSeconds`(EditDefaultsOnly, 기본 0.25, ≥ 0.05).
- update마다:
  1. `TActorIterator<ABathhouseCustomerCharacter>`로 손님 Actor 위치를 한 번 모은다. 플레이어는 제외된다.
  2. 구역마다 `n` = `SpawnBounds` oriented box 안의 손님 수. 행동과 관계없이 Actor 위치(capsule 중심)로 판정한다. 넘어진 동안은 capsule이 쓰러진 자리에 남으므로 그 위치로 센다.
  3. 구역 clock `Remaining`(Exp(1) 표본)에서 `n × Δt / 평균간격`을 뺀다. `n = 0`이면 줄지 않는다.
  4. `Remaining ≤ 0`이면 그 구역에 한 번 생성을 시도하고 새 Exp(1) 표본으로 바꾼다. 넘친 양은 이월하지 않는다. 한 update에 구역당 최대 한 번이다.
  5. 시도는 전역·구역 최대에 도달했거나 `MaxPlacementAttemptsPerInterval`번 안에 자리를 못 찾으면 건너뛴다(몰아서 생성 없음).
- 이 규칙은 손님 n명이면 평균 `평균간격/n`의 무작위 간격이다. 손님이 드나들면 다음 update부터 빈도가 바뀐다.
- clock은 director가 구역 weak key로 보관하며, 구역 등록·해제 시 추가·삭제된다. random stream은 director 소유이고 테스트는 seed를 주입한다.
- 순수 계산은 `FCleaningSpawnClock`(Private)이다: `Advance(Remaining, CustomerCount, DeltaSeconds, MeanInterval) → bFire`, `SampleNext(Stream)`.

| director 값 | 종류 | 기본 | 비고 |
|---|---|---:|---|
| `SpawnIntervalSeconds` | 물 얼룩 | 레벨 값 유지 | 의미만 "손님 1명당 평균 간격"으로 바뀐다. 이름을 유지해 레벨 값이 보존된다 |
| `MaxActiveStains` | 물 얼룩 | 유지 | 전역 최대 |
| `StainClass`, `DefaultStainSpacing` | 물 얼룩 | 유지 | |
| `LitterMeanIntervalPerCustomerSeconds` | 쓰레기 | 120 | 신규, ≥ 1 |
| `MaxActiveLitter` | 쓰레기 | 20 | 신규 |
| `LitterClass` | 쓰레기 | Editor 지정 | 신규, `ALitterActor` 자식 |
| `DefaultLitterSpacing` | 쓰레기 | 40 cm | 신규, 쓰레기끼리 최소 간격 |
| (삭제) `DefaultPawnClearance` | 공통 | — | 2026-10-01 사용자 결정: 생성 위치는 플레이어·손님과 무관. Q1 A와 같은 즉시 삭제 |
| `SpawnClearanceHeightCm` | 공통 | 30 | 신규, 바닥 위 겹침 검사 높이 |
| `MaxPlacementAttemptsPerInterval` | 공통 | 유지 | 의미: 생성 1회당 자리 시도 수 |

## Floor Rule (생기는 자리)

두 구역 class는 같은 private `FCleaningFloorSpawnQuery`로 자리를 찾는다. 구역은 설정만 넘긴다.

- 구역에 native `SpawnFloor`(`USceneComponent`, `SpawnBounds` 자식, 기본 relative Z = −기본 extent Z)를 둔다. `SpawnFloor`의 world Z가 그 구역의 **바닥 높이 정본**이다. `PlacementZone`의 `PlacementFloor`와 같은 방식이다.
- 후보 절차:
  1. bounds 안 무작위 XY에서 bounds 위부터 아래로 `FloorTraceChannel` trace를 한다(기존).
  2. hit Z가 바닥 높이 ± `FloorHeightToleranceCm`(기본 5) 밖이면 기각한다. 설비 윗면, 물건 위, 욕탕 바닥, 카운터 위가 여기서 빠진다.
  3. hit Actor가 `IPlaceableFacility` 또는 `IPhysicalCarryable`이면 기각한다. 또 hit component가 **고정 레벨 지형**이 아니면 기각한다. 조건은 object type `WorldStatic`이고 Mobility `Static`인 것이다.
     - 2단계 허용 오차(±5cm) 안의 낮은 물체 윗면이 바닥으로 인정되는 것을 막는다. 예: 바닥에 놓인 사용한 수건(`AWorldUsedTowelActor`)은 휴대물이 아닌 BlockAllDynamic mesh라 3단계 interface 검사로는 걸리지 않는다(2026-10-01 재검토).
     - 이 조건이 있어야 5단계가 hit component를 제외해도 안전하다. 제외되는 것은 항상 바닥 지형뿐이다.
     - 물 얼룩·쓰레기는 1단계 trace가 통과해 지나간다(구현 유지).
  4. 기존 조건(구역 XY 포함, 경사, 선택 tag)을 확인한다.
  5. clearance box로 **충돌 응답 기반** 검사를 한다(2026-10-01 코드 리뷰 F1·F2 재검토).
     - box: 반폭 = 종류의 바닥 반경 R, 높이 `SpawnClearanceHeightCm`, 바닥 hit 위 1cm부터, 축 정렬.
     - "바닥에 놓인 물리 물체" 가정으로 `OverlapMultiByChannel`을 쓴다. query 채널은 `ECC_PhysicsBody`, 응답은 WorldStatic·WorldDynamic·PhysicsBody Block, 나머지 Ignore다. `bBlockingHit`인 결과만 막힘으로 센다(`FacilityPlacementCollision::HasBlockingOverlap`과 같은 응답 기반).
       - 막힘: 설비 몸체(BlockAllDynamic), 바닥에 놓인 휴대물·사용한 수건(physics 응답), 벽.
       - 막히지 않음: 영역 표시용 QueryOnly box(배치 구역 `ZoneBounds`, 진열 공간, router, 투입구 등 PhysicsBody Ignore), 생성·수거 구역(NoCollision), 물 얼룩·쓰레기(Visibility만 Block).
     - 1단계 floor trace가 맞힌 **바닥 component는 overlap에서 제외**한다(`AddIgnoredComponent`). 그래서 허용 경사(`MaximumFloorSlopeDegrees`) 안의 경사나 같은 mesh의 줄눈 단차가 바닥 자신과 겹쳐 기각되지 않는다. 다른 mesh의 1cm 이상 턱은 막힘으로 남긴다(보수적).
     - 물 얼룩·쓰레기 Actor는 방어적으로 한 번 더 무시한다(Q62 B).
     - `APawn` Actor의 component는 무시한다. 넘어진 손님의 ragdoll mesh(physics 응답)가 막지 않게 하기 위함이다. 생성 위치는 플레이어·손님과 무관하다.
     - 벽을 막힘으로 두어 벽 가장자리 반경 R 안에는 생기지 않는다(2026-10-01 사용자 결정 A, 벽에 묻혀 보이지 않게, TRSH-030).
     - object type만 보는 query(`OverlapMultiByObjectType`)는 상대 응답을 보지 않아 QueryOnly trigger까지 잡으므로 쓰지 않는다.
  6. 같은 종류끼리는 subsystem 등록부로 간격을 확인한다. 물 얼룩은 얼룩끼리, 쓰레기는 쓰레기끼리만 본다.
  7. (삭제) Pawn clearance sphere 검사는 하지 않는다. 플레이어·손님 바로 옆이나 발밑에도 생긴다(2026-10-01 사용자 결정, TRSH-029).
- 종류별 R:
  - 물 얼룩: `AWaterStainActor::FloorRadiusCm`(신규, EditDefaultsOnly, 기본 30) × class 기본 `MaxXYScale`(최악값).
  - 쓰레기: `ALitterActor::FloorRadiusCm`(기본 15, 모든 외형 후보를 덮는 값).
- `AStainSpawnZoneActor`: `SpawnFloor`, `FloorHeightToleranceCm`을 추가하고 `SelectionWeight`, `ZoneKind`와 `EStainSpawnZoneKind`는 삭제한다(Q1 A). `PawnClearanceOverride`와 director `DefaultPawnClearance`도 같은 방식으로 삭제한다(2026-10-01).
- `ALitterSpawnZoneActor`: 같은 구성의 독립 class다(부모 변경 없음). `MaxActiveLitterInZone`(기본 5), `LitterSpacingOverride`, floor filter 값을 가진다. 두 구역은 따로 배치하며 같은 영역이어도 된다.

## Litter

`ALitterActor`(`BP_Litter`). 구현: `IPlayerInteractable`.

- 컴포넌트:
  - root `InteractionCollision`(`USphereComponent`, 기본 반경 12): QueryOnly, Visibility만 Block, 나머지 Ignore, Navigation off.
  - `LitterMesh`(`UStaticMeshComponent`): NoCollision, Navigation off.
  - 물리·이동이 없다. 밀리지 않고 Pawn·물건을 막지 않는다(TRSH-006). Visibility를 막으므로 뒤의 물 얼룩 조준을 가린다(Q62 B, TRSH-026).
- 외형: `MeshVariants`(EditDefaultsOnly, `TArray<UStaticMesh>`)에서 seed로 하나를 고르고 yaw 0~360을 적용한다. 선택 방식은 물 얼룩 seed 계약(`ConfigureVisualVariationSeed` → BeginPlay 적용)과 같다(TRSH-005).
- 등록: BeginPlay에서 `RegisterLitter`, EndPlay에서 해제한다. `SetSpawnZone`으로 구역별 수를 센다.
- 상태: `Active → Removed`(terminal). `CommitCollected()`와 `ClearForFacilityPlacement()`는 같은 terminal 경로다. collision을 끄고, 등록 해제, Destroy 순서이며 중복 호출은 no-op이다.
- query:
  - TargetName `쓰레기`(외형 무관). E `ActionName` 비움, secondary 없음. E 실행은 빈 이유 실패다(TRSH-006).
  - Apply 필드: 들고 있는 carry kind가 `Facility`면 비운다(배치가 LMB 소유). 그 밖에는 visible, 불가, 이유 `집게가 필요합니다`다.
    - 장비(대걸레·몽키스패너·배송 상자·집게)를 들면 `MergeEquipmentQuery`가 held-use 필드를 지우고 장비 행을 쓴다. 따라서 TRSH-028 기존 행동이 유지되고, 집게는 `줍기`를 표시한다.
    - 빈손·품목 박스·수건바구니·삽·봉투·열쇠는 held-use Apply 경로가 이유만 한 번 보인다(TRSH-020).
  - `ExecuteHeldTargetUse`: 기본(빈 이유 실패) 그대로다. 줍기는 집게 장비 경로만 한다.

## Tongs

`ALitterTongsActor`(`BP_LitterTongs`). 구현: `IPlayerInteractable`, `IPhysicalCarryable`, `IHeldEquipmentUsable`, `IHeldEquipmentSecondaryUsable`.

- carry: `EPhysicalCarryKind::LitterTongs`(append), 기본 capability `FreeDrop|FixedSlot`. exact fixed slot, held-pose drop, CCD·Pawn Ignore, fixed slot 우선 복구는 `AWetMopActor`와 같은 구조다. 봉투 개수는 모든 carry 전이에서 유지된다(TRSH-013, 023).
- 값: `BagCapacity`(EditDefaultsOnly, 20, ≥ 1), `TiedBagClass`(`ATrashBagActor` 자식), `TieForwardDistanceCm` 60, `TieMinForwardDistanceCm` 30. 상태는 `BagCount`(Transient)다.
- `GetHeldSummaryText()` = `봉투 n/20`(HUD, 조준 무관).
- E query(월드·거치대 밖): 빈손이면 물걸레와 같은 들기 문구, 표시 이름 `집게`.
- LMB `QueryEquipmentUse`(`Instant`):
  - focus hit Actor가 유효한 `ALitterActor`이면 visible, action `줍기`. `BagCount ≥ BagCapacity`면 불가, `봉투 가득 참`.
  - 그 밖이면 `bVisible=false`, 이유 비움이다. 행이 없고 누르면 아무 일도 없다(TRSH-024).
- `BeginEquipmentUse`: query를 재평가한 뒤 `Litter->CommitCollected()` → `BagCount + 1` 순서로 실행한다. 두 단계 모두 검증 뒤라 실패하지 않는다. Instant이므로 누른 채 유지해도 한 개다(TRSH-007~009).
- RMB `QuerySecondaryEquipmentUse`(조준 무관): visible, action `봉투 묶기`. `BagCount == 0`이면 불가, `봉투가 비어 있음`.
- `ExecuteSecondaryEquipmentUse`:
  1. 재평가한다.
  2. `FTrashBagDropPlacement::Find`로 위치를 구하고, 실패하면 `봉투를 놓을 공간이 없음`을 반환하며 아무것도 바꾸지 않는다.
  3. `ATrashBagActor::SpawnTiedBag(World, Class, BagCount, Transform)`: deferred → `InitializeCount` → Finish → `ActivateFreeWorld`(추가 속도 없음). 실패하면 만든 Actor를 제거하고 실패한다.
  4. 성공 뒤에만 `BagCount = 0`이다(TRSH-010~012, 021, 022).

### Front Drop Placement

`FTrashBagDropPlacement::Find(World, UserPawn, IgnoredActors, BagClass, Distances, OutTransform)`:

- 발바닥(capsule 바닥)과 카메라 수평 전방을 쓴다. 거리 d는 `TieForwardDistanceCm`부터 10cm씩 `TieMinForwardDistanceCm`까지 시도한다.
- 후보 중심 = 발바닥 + 전방 × d + up × (봉투 half Z + 5), 회전은 플레이어 yaw다.
- 검사:
  - capsule 중심 → 후보 중심 Visibility line trace가 막히지 않음(벽 너머 금지).
  - 봉투 class collision template(`BuildClassCollisionQuery`)로 `FacilityPlacementCollision::HasBlockingOverlap`. 플레이어·집게는 무시한다. template 응답을 쓰므로 Pawn·쓰레기·얼룩은 막지 않는다.
- 모두 실패하면 공간 없음이다.

## Trash Bag

`ATrashBagActor`(`BP_TrashBag`). 구현: `IPlayerInteractable`, `IPhysicalCarryable`, `IPhysicalCarryDiscardable`. 장비가 아니다.

- `AItemBoxActor`와 같은 비도구 휴대물 구조다.
  - root `BagMesh`(physical root, QueryAndPhysics, CCD, Pawn Ignore).
  - class 소유 `HeldTransform`, CDO root scale 보존, lifecycle `Staged/FreeWorld/Held/Consumed`, last-safe 복구.
  - `EPhysicalCarryKind::TrashBag`(append), `FreeDrop`만.
- `LitterCount`는 spawn 중 `InitializeCount(≥ 1)` 한 번만 설정하고 이후 불변이다(풀기·다시 담기 없음).
- TargetName·`GetHeldSummaryText()`는 `쓰레기봉투 (n개)`, E는 빈손 들기다(TRSH-014).
- 버리기는 held·world 모두 가능하다(아래).

## World Discard And Collection

### Discard Contract Extension

[PhysicalCarrySystem.md](PhysicalCarrySystem.md) Consume And Discard Extension의 확장이다. `IPhysicalCarryDiscardable`에 추가한다(기본 구현 = 불가):

- `CanDiscardFromWorld(OutFailureReason) const`: FreeWorld 상태이고 종류 규칙을 만족할 때만 true.
- `HandleDiscardFromWorldCommitted()`: 내용과 payload를 비우고, 일반 EndPlay 복구를 막는 소비 상태로 전이한 뒤 Destroy.
- 종류 규칙은 기존 held 판정과 같은 private 함수를 공유한다. 쓰레기통 규칙과 같게 유지된다.
  - 설비 아이템: `Facility.Discardable`, `LockerSlotCount == 0`.
  - 박스·상자·봉투: 내용 초기화됨.
- 구현: 품목 박스, 배송 상자, 설비 아이템, 봉투. 열쇠·레벨 도구는 구현하지 않아 남는다(COLL-002).

### Collection Zone

`ATrashCollectionZoneActor`(`BP_TrashCollectionZone`).

- 컴포넌트: root `CollectionBounds`(`UBoxComponent`): NoCollision, Navigation off.
- 바닥 표시는 Blueprint가 NoCollision·Navigation off의 decal 또는 plane으로 한다.
- `CollectionIntervalSeconds`: EditAnywhere, 기본 300, ≥ 1(COLL-005, 009).
- BeginPlay에 looping timer를 시작한다. game time이며 pause에 멈춘다. 첫 수거는 시작 후 한 주기 뒤다. EndPlay에 timer를 해제한다.
- `CollectNow()`(C++ public, 테스트·디버그):
  1. zone box로 AllObjects overlap을 한다. held 물건은 collision이 꺼져 있어 빠진다.
  2. 고유 Actor마다 다음을 모두 만족하면 대상 목록에 넣는다.
     - `IPhysicalCarryDiscardable` 구현
     - `CanDiscardFromWorld`
     - `IPhysicalCarryable::GetPhysicalCarryPrimitive()` bounds 중심이 zone oriented box 안(COLL-007)
  3. 목록을 만든 뒤 각 대상의 `HandleDiscardFromWorldCommitted()`를 호출한다. 돈 변화·표시·방송은 없다(COLL-001, 004).
- 배치된 설비, 쓰레기, 바닥 수건, 열쇠, 도구는 계약을 구현하지 않거나 FreeWorld 휴대물이 아니라서 남는다(COLL-002, 008).
- 비 shipping 콘솔 `bathhouse.Debug.TrashCollection.CollectNow`: 레벨의 모든 수거 구역을 즉시 수거한다(PIE에서 300초를 기다리지 않기 위함).

### Trash Bin

`ABathhouseTrashBinActor` 코드와 held 버리기 경로는 그대로 둔다. 레벨 instance만 Editor에서 제거한다(Q39 A, COLL-006). [ShopSystem.md](ShopSystem.md) Trash Bin 참고.

## Footprint Clear On Placement

- Placement는 배치 확정 publication 직후 `UFacilityPlacementEventSubsystem::BroadcastFacilityPlaced(Event)`를 호출한다. 이벤트 값은 placed Actor weak, footprint world transform, unscaled extent다([PlacementSystem.md](PlacementSystem.md)). preview·취소·실패에서는 발행하지 않는다. 신규·재배치 모두 같은 경로다.
- `UCleaningWorldSubsystem`:
  - `Initialize`에서 이 subsystem에 dependency를 걸고 구독한다. `Deinitialize`에서 해제한다.
  - handler는 등록된 모든 물 얼룩·쓰레기에 `FCleaningFootprintOverlap::Intersects`를 적용하고, 겹치면 `ClearForFacilityPlacement()`를 호출한다.
- 겹침(Q64 A, 조금이라도):
  - XY: 대상 중심 원(반경 R)과 footprint 사각형(extent × |scale|, footprint 회전)의 최근접 거리 ≤ R.
  - Z: 대상 Z가 footprint 바닥 − 5 ~ 윗면 + 5 안.
  - 물 얼룩 R은 `FloorRadiusCm × max(선택된 X, Y scale)`, 쓰레기 R은 `FloorRadiusCm`다.
- 물걸레 진행 중인 얼룩도 제거된다. 기존 EndPlay 정리가 청소자 잠금을 풀고, 물걸레는 target 없이 mopping 상태만 유지한다.
- 등록 해제로 전역·구역 수가 줄어 다시 생성될 수 있다(TRSH-025).

## Input Routing (RMB 장비 보조 사용)

[HeldTargetUseSystem.md](HeldTargetUseSystem.md) Input Ownership의 RMB 규칙을 바꾼다.

- `IHeldEquipmentSecondaryUsable`(C++ 전용): `QuerySecondaryEquipmentUse(Context) const → FHeldEquipmentUseQuery`, `ExecuteSecondaryEquipmentUse(Context) → FHeldEquipmentUseResult`. press당 한 번이며 반복·hold가 없다.
- `UPlayerEquipmentUseComponent`:
  - `HasSecondaryEquipmentUse()`
  - `ExecuteSecondaryEquipmentUse()`: 재평가 → 실행 → `ReportExternalInteractionAttempt(intent HeldTake)` → query refresh.
  - `MergeEquipmentQuery`: held-use 필드를 지운 뒤 장비가 보조 사용을 구현하면 `HeldTake*` 필드(RMB 행)를 그 query로 채운다. mode는 Instant다.
  - `BeginEquipmentUse`: 장비 query가 `!bVisible && 이유 비움`이면 result를 보고하지 않고 끝난다(집게의 쓰레기 아닌 곳 LMB 무반응). 다른 장비는 항상 visible이라 결과가 같다.
- `HeldTake` intent는 "RMB 행 결과"로 쓴다. widget 변경은 없다.
- Character RMB Started 순서:
  1. Computer capture, Placement active, held-use 진행 중, LMB 장비 입력 진행 중 → 무시
  2. 장비를 들었고 보조 사용 있음 → `ExecuteSecondaryEquipmentUse`
  3. 장비를 들었고 보조 사용 없음 → 무시(기존)
  4. 그 밖 → HeldTargetUse Take(기존)
- RMB 해제는 보조 사용 owner면 아무것도 하지 않는다.

## Blueprint/API And Editor Contracts

- 신규 reflected:
  - 위 class 다섯 개와 그 property·component 이름(`InteractionCollision`, `LitterMesh`, `SpawnBounds`, `SpawnFloor`, `BagMesh`, `CollectionBounds`)
  - `EPhysicalCarryKind::LitterTongs`, `TrashBag`(append)
  - director·stain zone·stain 추가 property
  - `UFacilityPlacementEventSubsystem`
- 삭제(Q1 A): `AStainSpawnZoneActor::SelectionWeight`, `ZoneKind`, `EStainSpawnZoneKind`. 2026-10-01 추가 삭제: `AStainSpawnZoneActor::PawnClearanceOverride`, `ACleaningDirectorActor::DefaultPawnClearance`. property 삭제라 Core Redirect를 쓰지 않는다. Editor에서 `BP_StainSpawnZone`과 레벨 구역 instance를 load·compile·resave한다.
- 기존 이름 변경은 없다. `SpawnIntervalSeconds`는 이름을 유지하고 의미·tooltip만 바뀐다.
- copy-first load gate: `BP_CleaningDirector`, `BP_StainSpawnZone`, `BP_WaterStain`(native property·subobject 추가), `BP_ItemBox`, `BP_ShopDeliveryBox`, `BP_PlaceableFacilityItem`(interface 추가).

## Verification

| 시나리오 | 자동화 |
|---|---|
| TRSH-001, 002, 018, 019 | `FCleaningSpawnClock` 결정적 seed: n=0 발생 없음, n=2의 600초 기대 10±허용, n=1 대비 약 2배, 구역 이동 시 전환, 얼룩 평균간격 = `SpawnIntervalSeconds` |
| TRSH-003, 025 | 전역·구역 최대에서 발생 건너뜀, 제거 뒤 재발생, 몰아서 생성 없음 |
| TRSH-004, 029, 030 | 바닥 plane 허용 오차, 설비 윗면·박스·욕탕 바닥·벽 기각, 플레이어·손님(서 있음·넘어짐) 옆·발밑 생성 성공, clearance 겹침 기각, 쓰레기·얼룩 서로 무시. 배치 구역(DefaultMap 구성의 얇은 QueryOnly bounds)·겹친 물 얼룩/쓰레기 구역 안 생성 성공, 20° 경사 바닥·같은 mesh 1cm 단차 생성 성공 |
| TRSH-005 | 같은 seed 같은 외형·yaw |
| TRSH-006, 020, 028 | 쓰레기 E 무변화, Pawn·물건 비충돌, 빈손·박스·바구니·삽 LMB 이유, 렌치·걸레·배송 상자 장비 행 유지 |
| TRSH-007~012, 021, 022, 024 | 집게 줍기 1개, 가득 참, 묶기 조준 무관, 빈 봉투, 막힌 정면 무변화, press당 1회, 비대상 LMB 무반응 |
| TRSH-013, 014, 023 | 거치대·drop·낙하 복구 개수 유지, 봉투 E·G·요약 |
| TRSH-016, 017, 026, 027 | 실제 conversion transaction 배치: 겹침(가장자리 포함) 제거·수 감소·청소 중 얼룩 제거, preview·취소 무변화, 쓰레기가 얼룩 조준을 가림 |
| COLL-001~004, 007, 008 | `CollectNow`: 종류별 제거·잔존, held 제외, 중심 판정, 내용물 동반 소멸, 지갑 불변 |
| COLL-005, 009 | 간격 300·60 timer |
| 회귀 | Cleaning(물걸레), Shop 쓰레기통 held 버리기, Service·Placement·Interaction held-use·Combat 전체 |

PIE: 대표 시나리오, HUD 문구(RMB 행 포함), 쓰레기 외형·수거 구역 표시, 손님 수에 따른 빈도, 콘솔 즉시 수거.

## Dependencies

- Cleaning → Customer(손님 Actor class, 위치 읽기만), Placement(이벤트 subsystem, collision helper), Interaction(carry·equipment·discard 계약)
- Placement는 Cleaning을 모른다. Customer는 Cleaning을 모른다.
- 새 module은 없다.
