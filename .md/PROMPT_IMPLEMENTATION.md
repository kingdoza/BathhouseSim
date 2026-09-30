# 구현 프롬프트 — 서비스 3단위: 쓰레기·수거

## 재작업 — 생성 자리 clearance 판정 (2026-10-01)

코드 리뷰 결론(`.md/PROMPT_IMPLEMENTATION_R.md` F1~F3)에 대한 재작업이다. 2026-10-01 사용자 결정 두 가지를 함께 반영한다: 생성 위치는 플레이어·손님과 무관(Pawn 여유 삭제, TRSH-029), 벽 가장자리 제외 유지(TRSH-030). 두 결정은 기능 명세에 반영돼 있다(`.md/QNA_FEATURE_SPEC.md` 명세 정정 기록 2026-10-01). 대상 시나리오: TRSH-001~005, 018, 019, 029, 030과 대표 시나리오.

아래 `단계와 입력`과 0~10절은 완료된 3단위 본 구현 기록이며 다시 수행하지 않는다. 그 절의 clean 시작 조건은 이 재작업에 적용하지 않는다. headless 실행 형식과 git `--no-optional-locks` 규칙은 그대로 따른다.

### 시작 조건

- UnrealEditor가 모두 종료됐는지 확인한다.
- 현재 Source 변경(3단위 C++, 미커밋)은 이 단위 결과물이므로 유지한다.
- Content·Config는 수정·저장하지 않는다. 시작 전 `git --no-optional-locks status --short -- Content Config` 결과를 기록하고, 끝난 뒤 같은지 확인한다.

### 결정(정본)

- `.md/Architecture/CleaningLitterSystem.md` Floor Rule 5단계(충돌 응답 기반 clearance, 바닥 hit component 제외)
- 이 절과 정본이 다르면 멈추고 보고한다.

### 구현

1. `Private/Cleaning/CleaningSpawnRules.cpp` `FCleaningFloorSpawnQuery::Find`의 clearance 검사:
   - `OverlapMultiByObjectType(WorldStatic|WorldDynamic|PhysicsBody)`를 `OverlapMultiByChannel(ECC_PhysicsBody)`로 바꾼다.
     - `FCollisionResponseParams`: 전부 Ignore로 시작하고 WorldStatic·WorldDynamic·PhysicsBody만 Block.
     - 결과 중 `bBlockingHit && IsValid(GetComponent())`만 막힘으로 센다.
   - box 크기·위치(hit 위 1cm부터 `ClearanceHeight`, 반폭 R, 축 정렬)는 바꾸지 않는다.
   - query params에 floor trace hit component를 `AddIgnoredComponent`로 추가한다.
   - 물 얼룩·쓰레기 Actor 무시는 유지한다.
   - `HasBlockingOverlap`을 직접 호출할 필요는 없다. 가상 template 없이 위 응답을 직접 만든다. 공용 helper에 새 overload를 추가하지 않는다.
2. 같은 함수의 3단계 hit 기각(설비·휴대물 interface)에 조건 하나를 더한다. hit component의 `GetCollisionObjectType() != ECC_WorldStatic`이거나 `Mobility != EComponentMobility::Static`이면 기각한다.
   - 바닥 높이 ±5cm 안의 낮은 물체(바닥 사용한 수건 등) 윗면에 생기는 것을 막는다.
   - 1의 hit component 제외는 이 조건을 통과한 바닥 지형에만 적용된다.
   - 그 밖의 Floor Rule 단계(바닥 높이 허용 오차, 경사, tag, 간격, 물 얼룩·쓰레기 trace 통과)는 바꾸지 않는다.
2-1. 플레이어·손님 무관(사용자 결정):
   - Pawn clearance sphere 검사와 설정 struct의 Pawn 여유 값을 삭제한다.
   - 1의 clearance overlap에서 `APawn` Actor의 component를 무시한다(넘어진 손님 ragdoll).
   - reflected property 삭제(Q1 A와 같은 즉시 삭제): `ACleaningDirectorActor::DefaultPawnClearance`, `AStainSpawnZoneActor::PawnClearanceOverride`, `ALitterSpawnZoneActor::PawnClearanceOverride`.
   - 벽은 계속 막힘이다(사용자 결정 A).
3. F3: LOCTEXT 키를 class에 맞게 바꾼다. 문구·동작은 바꾸지 않는다.
   - `LitterTongsActor.cpp`: `TakeMop`, `MopNotHeldForDrop` → 집게용 키
   - `TrashBagActor.cpp`: `InvalidBoxActivation`, `BoxPhysicsFailed`, `BoxNotDiscardable`, `InvalidBoxAuthoring` → 봉투용 키
4. 금지: 발생 clock·RMB routing·HUD 합성·집게·봉투·world 버리기·수거 구역·배치 이벤트(리뷰 "유지할 것" 전체), Placement collision helper, 설비·휴대물 collision profile 변경.

### 자동화

바닥 판정 자동화에 추가한다. 실제 zone `FindSpawnTransform` 경로를 쓴다.

- DefaultMap 구성의 `AFacilityPlacementZoneActor`(ZoneBounds extent `(1400,900,10)`, 바닥 Z=0 중심) 안의 물 얼룩 구역·쓰레기 구역에서 생성이 성공한다.
- 같은 영역에 겹쳐 둔 물 얼룩 구역과 쓰레기 구역이 서로를 막지 않고 둘 다 생성된다.
- 진열 공간과 같은 QueryOnly(Visibility만 Block) box가 바닥에 걸쳐 있어도 막지 않는다.
- 경사: 허용 경사 안(예: 20°)의 한 component 경사 바닥에서 생성이 성공한다. 이때 바닥 높이 허용 오차 안의 후보만 쓰도록 fixture를 둔다. 같은 mesh의 1cm 단차도 성공한다.
- 기존 기각은 유지한다: 설비 윗면·설비 몸체 옆, 박스·설비 아이템·바닥 사용한 수건·열쇠, 벽, 욕탕 바닥. 각각 실제 collision profile(BlockAllDynamic, PhysicsActor 계열)로 만든다.
- TRSH-029 플레이어·손님 무관: 선 Pawn capsule 바로 옆·발밑 후보와 ragdoll 상태 skeletal mesh(physics 응답) 위 후보가 성공한다.
- TRSH-030 벽: 벽에서 R 안의 후보는 기각되고, R 밖은 성공한다. Pawn 기각을 단언하던 기존 테스트는 이 결정에 맞게 바꾼다.
- 낮은 물체 윗면: 높이 3cm의 바닥 사용한 수건(`AWorldUsedTowelActor`)과 3cm 판(Movable, BlockAllDynamic)을 두고, 그 윗면을 맞힌 후보가 기각된다. 이 fixture는 hit component 제외와 함께 검증한다.
- Editor 확인: DefaultMap 바닥 mesh가 `WorldStatic`·`Static`인지 `PROMPT_UNREAL.md` 확인 항목에 넣는다. 아니면 모든 생성이 멈추므로 PIE 전에 확인한다.
- director 통합: `AdvanceSpawnScheduleForTesting`으로 배치 구역 안 구역에서 쓰레기·물 얼룩이 실제로 생긴다.
- 회귀: Cleaning 전체, 전체 `Automation RunTests BathhouseSim`. 수치와 실패 이름을 보고하고, 실패는 스스로 무관 판정하지 않는다.

### 빌드와 로드

- UE 5.8 `Build.bat`로 빌드한다.
- Pawn 여유 property 삭제로 native 구조가 바뀐다. 본 구현 9절과 같은 copy-first load gate를 다시 실행한다.
  - 대상: `BP_CleaningDirector`, `BP_StainSpawnZone`, DefaultMap(레벨 director·구역 instance)
  - 삭제 property의 로드 경고만 있고 Fatal·`Serial size mismatch`가 없어야 한다.
  - 저장하지 않는다.
- 그 밖의 reflected 변경은 없어야 한다. 있으면 멈추고 보고한다.

### 결과물

- `.md/PROMPT_REVIEW.md`를 다시 쓴다: 선택된 판정 기준, 변경 파일, 추가 fixture 목록과 시나리오 대응, 빌드·전체 회귀 수치, Content·Config 무변경 확인.
- `.md/PROMPT_UNREAL.md`:
  - PIE 관찰 항목 추가
    - "배치 구역 안 탈의실·욕실 바닥에서 손님이 있을 때 쓰레기·물 얼룩이 실제로 생긴다"
    - "설비·놓인 물건 위나 겹치는 자리, 욕탕 안, 벽 바로 옆에는 생기지 않는다"
    - "손님·플레이어 발밑이나 바로 옆에도 생길 수 있다"
  - `BP_CleaningDirector`·`BP_StainSpawnZone`·레벨 구역 resave 항목에 Pawn 여유 property 삭제를 포함한다.
- `.md/Architecture/*`는 수정하지 않는다.

## 단계와 입력

- 기능 계약: `.md/PROMPT_ARCHITECTURE.md`(서비스 3단위). 대상 시나리오는 TRSH-001~030, COLL-001~009다(029·030은 재작업 절에서 추가). 사용자가 2026-09-30 이 명세로 진행하도록 지시했고, 설계 중 추가 질문 Q62~Q65와 F5 정정이 반영돼 있다.
- 기술 선택: `.md/QNA_ARCHITECTURE.md` Q1 A(구역 `SelectionWeight`·`ZoneKind`·`EStainSpawnZoneKind` 즉시 삭제).
- 단계: **수직 구현(단위 확장)**. 1·2단위는 커밋 `5b42a47`, `c9a1150`로 완료됐다.
- `.md/AGENT_WORKFLOW.md` → `.md/AGENT_IMPLEMENTATION.md`를 읽고 C++ 구현 단계만 수행한다.
- 설계 정본(전체 읽기):
  - `.md/Architecture/CleaningLitterSystem.md`
  - `.md/Architecture/CleaningSystem.md`
- 관련 절:
  - `PhysicalCarrySystem.md` Consume And Discard Extension
  - `HeldTargetUseSystem.md` Input Ownership·HUD Data
  - `PlacementSystem.md` 배치 확정 이벤트
  - `ShopSystem.md` Trash Bin
  - `ServiceSystem.md` Item Box
  - `CoreSystem.md` Class Growth Policy·Core Redirect Policy
- 이 문서와 정본이 다르면 구현을 멈추고 보고한다.
- headless 실행은 `.md/AGENT_WORKFLOW.md` UE 5.8 Headless Automation Policy 형식을 따른다. git은 `--no-optional-locks`로 실행한다.
- 시작 조건: `git --no-optional-locks status --short -- Source Content Config`가 비어 있어야 한다. `.md/` 변경은 정상이다.

## 0. 사전 조건과 백업

- UnrealEditor가 모두 종료됐는지 확인한다.
- `Saved/MigrationBackup/20261001_service_unit3/`에 다음 uasset을 파일 복사한다.
  - `BP_CleaningDirector`, `BP_StainSpawnZone`, `BP_WaterStain`(`/Game/Bathhouse/Blueprints/Cleaning/`)
  - `BP_ItemBox`(`/Game/Bathhouse/Blueprints/Service/`)
  - `BP_ShopDeliveryBox`(`/Game/Bathhouse/Blueprints/Shop/`)
  - `BP_PlaceableFacilityItem`(`/Game/Bathhouse/Blueprints/Placement/`)
- Content·Config·Level을 저장하지 않는다(아래 9의 복사본 확인만 예외).

## 1. 물 얼룩 구역·director 정리 (Q1 A)

- `AStainSpawnZoneActor`:
  - 삭제: `SelectionWeight`, `ZoneKind`, `GetSelectionWeight()`
  - 추가: native `SpawnFloor`(`USceneComponent`, `SpawnBounds` 자식, relative Z = −기본 extent Z), `FloorHeightToleranceCm`(EditAnywhere, 5, ≥ 0)
- `CleaningTypes.h`: `EStainSpawnZoneKind`를 삭제한다. 다른 enum은 유지한다.
- `ACleaningDirectorActor`:
  - `SelectZone`과 가중치 선택을 삭제한다.
  - `SpawnIntervalSeconds`는 이름을 유지하고 tooltip을 "손님 1명당 평균 간격"으로 바꾼다.
  - 정본 Spawn Schedule 표의 신규 값을 추가한다.
- `AWaterStainActor`:
  - `FloorRadiusCm`(EditDefaultsOnly, 30, > 0)
  - `GetFloorRadius()` = `FloorRadiusCm × max(선택된 X, Y scale)`
  - `ClearForFacilityPlacement()`: 기존 Removed terminal 경로와 같다. 청소자 잠금을 풀고 등록을 해제한 뒤 Destroy한다. 중복 호출은 no-op이다.
- 기존 청소 진행·완료·외형 변화 코드는 바꾸지 않는다.

## 2. 공용 생성 규칙 helper

`Private/Cleaning/CleaningSpawnRules.h/.cpp`(non-UObject, 테스트 가능):

- `FCleaningSpawnClock`: `SampleNext(FRandomStream&)`(Exp(1)), `Advance(Remaining&, CustomerCount, DeltaSeconds, MeanIntervalSeconds) → bool`(정본 절차 3·4, 이월 없음).
- `CountCustomersInBox(const TArray<FVector>&, const FTransform& BoxTransform, const FVector& Extent)`: 순수 함수.
- `FCleaningFloorSpawnQuery`:
  - 설정 struct(trace channel·거리, tag, 경사, 바닥 Z·허용 오차, R, clearance 높이, 간격, Pawn 여유)를 받는다.
  - 정본 Floor Rule 1~7을 수행하고 후보 transform을 반환한다.
  - 같은 종류 간격은 호출자가 넘긴 callback으로 확인한다.
  - clearance overlap은 `AWaterStainActor`·`ALitterActor`를 무시한다.
- `FCleaningFootprintOverlap::Intersects(Center, Radius, FootprintTransform, UnscaledExtent) → bool`: 정본 Footprint Clear 겹침(XY 원·사각형, Z ±5).
- 두 zone class의 `FindSpawnTransform`은 이 query에 설정만 넘긴다. 기존 stain zone의 floor 코드 중복을 이 helper로 옮긴다.

## 3. Director와 subsystem

- `ACleaningDirectorActor`:
  - looping timer `SpawnUpdateIntervalSeconds`
  - 손님 위치를 한 번 수집(`TActorIterator<ABathhouseCustomerCharacter>`)
  - 두 종류 구역 clock 진행, 발생 시 1회 시도
  - clock map은 구역 weak key로 관리한다(`TMap`, 등록 변화 반영).
  - random stream은 BeginPlay에 `FMath::Rand()`로 seed한다.
  - 테스트용 C++ 전용 `AdvanceSpawnScheduleForTesting(DeltaSeconds, const TArray<FVector>& CustomerLocations)`와 seed 주입을 둔다(`WITH_DEV_AUTOMATION_TESTS`).
  - 쓰레기 spawn: deferred → `ConfigureVisualVariationSeed` → `SetSpawnZone` → Finish.
- `UCleaningWorldSubsystem`:
  - 쓰레기 구역·쓰레기 등록부(`RegisterLitterZone/Litter`, 수, 구역별 수, 간격)를 추가한다.
  - `Initialize`: `Collection.InitializeDependency<UFacilityPlacementEventSubsystem>()` → 구독. `Deinitialize`: 해제.
  - handler: 등록 얼룩·쓰레기 전체에 `Intersects`를 적용한다. 대상 목록을 먼저 만든 뒤 `ClearForFacilityPlacement()`를 호출한다(순회 중 compact 방지).

## 4. 쓰레기와 쓰레기 구역

- `ALitterSpawnZoneActor`: stain zone과 같은 구성의 독립 class. 부모를 바꾸거나 공통 부모를 만들지 않는다.
  - 값: `MaxActiveLitterInZone` 5, `LitterSpacingOverride`, `PawnClearanceOverride`, floor filter, `SpawnFloor`, 허용 오차
  - BeginPlay 등록, EndPlay 해제
- `ALitterActor`: 정본 Litter 절 그대로다.
  - collision 응답: Visibility만 Block, 나머지 Ignore, Navigation off. 물리 없음.
  - query 문구: TargetName `쓰레기`, Apply 이유 `집게가 필요합니다`, carry kind `Facility`면 Apply 비움.
  - `CommitCollected()`·`ClearForFacilityPlacement()`는 같은 terminal 경로다.
  - Data Validation: `MeshVariants`에 null이 아닌 항목 ≥ 1, `FloorRadiusCm` > 0.

## 5. 집게·봉투·입력

- `EPhysicalCarryKind`: 끝에 `LitterTongs`, `TrashBag`을 append한다. 기존 순서는 바꾸지 않는다.
- `IHeldEquipmentSecondaryUsable`(Public/Interaction, `CannotImplementInterfaceInBlueprint`): 정본 Input Routing.
- `UPlayerEquipmentUseComponent`:
  - 추가: `HasSecondaryEquipmentUse()`, `ExecuteSecondaryEquipmentUse()`
  - `MergeEquipmentQuery`: held-use 필드를 지운 뒤 보조 사용 query로 `HeldTake*`를 채운다.
  - `BeginEquipmentUse`: `!bVisible && 이유 비움`이면 무보고 종료한다.
- `AFirstPersonCharacter`:
  - RMB Started 순서를 정본대로 바꾼다. `ESecondaryUsePressOwner`에 `EquipmentSecondary`를 추가한다.
  - Completed/Canceled는 아무것도 하지 않는다.
  - LMB 경로는 바꾸지 않는다.
- `ALitterTongsActor`: 정본 Tongs 절 그대로다.
  - carry·fixed slot·복구는 `AWetMopActor` 구조를 따른다. 공통 부모나 component를 새로 만들지 않는다.
  - 문구: `줍기`, `봉투 가득 참`, `봉투 묶기`, `봉투가 비어 있음`, `봉투를 놓을 공간이 없음`, 요약 `봉투 n/20`.
- `TrashBagDropPlacement.h/.cpp`: 정본 Front Drop Placement 절. `FacilityPlacementCollision::HasBlockingOverlap`을 재사용한다.
- `ATrashBagActor`:
  - 정본 Trash Bag 절 그대로다. `AItemBoxActor`의 carry·scale·복구 구조를 따른다.
  - 추가: `SpawnTiedBag`, `BuildClassCollisionQuery`
  - 요약: `쓰레기봉투 (n개)`
  - Data Validation: 품목 박스와 같은 규칙.

## 6. World 버리기와 수거 구역

- `IPhysicalCarryDiscardable`: `CanDiscardFromWorld`, `HandleDiscardFromWorldCommitted`를 기본 불가·no-op 구현으로 추가한다.
- 품목 박스, 배송 상자, 설비 아이템, 봉투에 world 버리기를 구현한다.
  - 종류 규칙은 기존 held 판정과 같은 private 함수로 공유한다.
  - FreeWorld에서만 가능하다.
  - 소비 상태로 전이한 뒤 Destroy한다. 설비 아이템은 EndPlay 복구가 돌지 않아야 한다.
  - 기존 held 버리기 결과는 바꾸지 않는다.
- `ATrashCollectionZoneActor`: 정본 Collection Zone 절 그대로다(`CollectNow`, timer, 중심 판정, 목록 먼저 → 제거).
- `TrashCollectionDebugCommands.cpp`(`#if !UE_BUILD_SHIPPING`): `bathhouse.Debug.TrashCollection.CollectNow`. `FacilityDisplayDebugCommands.cpp` 형식을 따른다.
- `ABathhouseTrashBinActor`는 수정하지 않는다.

## 7. 배치 확정 이벤트

- `UFacilityPlacementEventSubsystem`(Public/Placement, `UWorldSubsystem`)과 `FFacilityPlacedEvent`, `FOnFacilityPlacedNative`: 정본 PlacementSystem 배치 확정 이벤트.
- `FFacilityActorConversionTransaction::PlaceItemAsFacility`: 성공 publication 블록 안에서 한 번 발행한다. 다른 경로는 건드리지 않는다.

## 8. 빌드

- `.md/AGENT_WORKFLOW.md`의 UE 5.8 `Build.bat`만 사용한다.

## 9. 로드 검증 — 복사본 먼저

- 대상:
  - `BP_CleaningDirector`, `BP_StainSpawnZone`, `BP_WaterStain`: property 삭제·추가, native `SpawnFloor`
  - `BP_ItemBox`, `BP_ShopDeliveryBox`, `BP_PlaceableFacilityItem`: interface 추가
- 확인:
  - native parent
  - `SpawnFloor` 존재
  - 삭제 property 로드 경고만 있고 Fatal·`Serial size mismatch` 없음
  - 기존 component 유지
- 저장하지 않는다.
- 순서(Fatal·`Serial size mismatch`·`Failed to load`면 즉시 멈춤):
  1. 복사본을 `Content/Developers/MigrationCheck/`에 두고 Template 맵으로 로드한다.
  2. 복사본을 삭제하고 Content 무변경을 확인한다.
  3. 원본을 로드한다.
  4. DefaultMap으로 로드한다(레벨 stain zone·director·쓰레기통 instance 포함).
  5. 다시 무변경을 확인한다.
- 가능하면 기존 `BathhouseSim.Service.BlueprintLoad` 형식의 Cleaning load test를 추가한다.

## 10. 자동화

정본 Verification 표 전체를 구현한다. 새 테스트는 파일을 나눠 둔다.

- `CleaningLitterSpawnAutomationTests.cpp`: clock·손님 수·바닥 규칙·최대·외형 seed·발밑 정리
- `CleaningLitterToolAutomationTests.cpp`: 쓰레기 query·집게·봉투·RMB routing
- `TrashCollectionAutomationTests.cpp`: world 버리기·수거 구역

추가 조건:

- 빈도(TRSH-002, 019): `FCleaningSpawnClock`을 고정 seed, Δt 0.25로 시뮬레이션한다.
  - n=2, 600초 기대값 10을 여러 seed 평균으로 확인한다(허용 ±20%).
  - n=0이면 0이다.
  - n 변화는 다음 step부터 반영된다.
  - 한 step 최대 1회, 이월 없음을 확인한다.
- director 통합: `AdvanceSpawnScheduleForTesting`에 구역 안·밖 손님 위치를 주입한다. 가능하면 `ABathhouseCustomerCharacter` 실제 Actor 두 개로 수집 경로를 한 번 확인하고, 불가하면 이유를 보고한다.
- 바닥 규칙: test world에 바닥, 설비 fixture 윗면, 박스, 벽, 낮은 욕탕 바닥(허용 오차 밖)을 두고 기각·허용을 확인한다. 쓰레기가 얼룩 위, 얼룩이 쓰레기 위에 생길 수 있어야 한다.
- 발밑 정리(TRSH-016, 017, 027, 025): Service fixture 규칙(construction 중 생성)으로 실제 `PlaceItemAsFacility` 경로를 쓴다.
  - 가장자리 겹침·중심 밖 겹침은 제거되고 떨어진 것은 남는다.
  - 청소 중 얼룩은 제거되고, 물걸레는 mopping을 유지하고 진행이 없다.
  - preview·실패·취소에서는 이벤트가 없다.
  - 제거 뒤 수가 감소해 재생성이 가능하다.
- 입력 routing:
  - 집게 RMB는 조준 대상(수건 선반·냉장고 fixture)과 무관하게 묶는다.
  - 렌치·걸레·배송 상자 RMB 무변화(기존)
  - LMB 입력 중 RMB 무시
  - 빈손·박스 RMB는 기존 Take
- TRSH-026: 쓰레기를 얼룩 앞에 두고 조준 trace가 쓰레기에서 멈춰 물걸레 진행이 없음 → 줍기 뒤 진행.
- world 버리기: 설비 아이템 world 버리기 뒤 EndPlay 복구가 새 아이템을 만들지 않는다. 락커 아이템·열쇠·집게·걸레는 불가다. held 상태는 world 버리기 불가다.
- 기존 회귀(유지 필수):
  - Cleaning 전체(stain spawn 테스트는 새 규칙에 맞게 수정. 가중치·kind 단언은 삭제)
  - Shop 쓰레기통 held 버리기
  - Service 전체, Placement 전체, Interaction held-use, Combat, Towel, Utility
- 전체 회귀: `Automation RunTests BathhouseSim`. 수치와 실패 이름을 보고하고, 실패는 스스로 무관 판정하지 않는다.

## 금지

- 기존 export를 가진 class·property·subobject의 rename·class 변경. 부모 변경, 기존 enum 순서 변경, native `Serialize` 변경, Core Redirect 추가도 금지다. Q1 A 삭제 세 항목만 예외다.
- `UPlayerCarryComponent`, `UPlayerInteractionComponent`, `UPlayerHeldTargetUseComponent` 수정.
- `UInteractionPromptWidget`·`FPlayerInteractionQuery` 구조 변경. `HeldTake` intent를 RMB 행 결과로 재사용한다.
- `ABathhouseTrashBinActor` 수정·삭제.
- Customer 코드·StateTree 변경. Cleaning은 손님 위치만 읽는다.
- Placement preview validity에 쓰레기·얼룩 조건 추가.
- 쓰레기의 게임 영향(만족도 등), 날짜 시스템, 수거 시점 표시, 봉투 채움 외형.
- Config·Content·Level 저장과 Editor authoring.

## 단계 결과물

- `.md/PROMPT_REVIEW.md`:
  - 변경 파일, 영역별 요약
  - 9의 결과와 로그 위치
  - automation 수치와 시나리오 ID별 대응
  - 미검증(PIE 전용)
  - 클래스 줄 수 변화. 특히 `FacilityActorConversionTransaction.cpp`, `ItemBoxActor.cpp`, `ShopDeliveryBoxActor.cpp`, `PlaceableFacilityItemActor.cpp`, `FirstPersonCharacter.cpp`, `PlayerEquipmentUseComponent.cpp`
- `.md/PROMPT_UNREAL.md`: 아래 Editor 목록을 exact path·Parent Class·저장 allowlist·검증 방법·갱신할 `.md/Unreal/*`로 구체화한다. 1·2단위와 같은 수준으로 쓴다.
  - 신규 Blueprint(`/Game/Bathhouse/Blueprints/Cleaning/`):
    - `BP_Litter`: 임시 외형 후보 3종 이상(빈 병·면봉·휴지 뭉치 대용 기본 도형), `FloorRadiusCm`
    - `BP_LitterSpawnZone`
    - `BP_LitterTongs`: mesh, `HeldTransform`, `TiedBagClass`
    - `BP_TrashBag`: 위 닫힌 봉투 대용 mesh, root scale, `HeldTransform`
    - `BP_TrashCollectionZone`: 바닥 표시 decal/plane, NoCollision·Nav off
  - 기존 수정:
    - `BP_CleaningDirector`: `LitterClass`, 쓰레기 값. 레벨 값 `SpawnIntervalSeconds` 유지 확인
    - `BP_WaterStain`: `FloorRadiusCm`을 실제 외형 반경에 맞춤
    - `BP_StainSpawnZone`: resave(Q1 A)
    - `WBP_InteractionPrompt`: RMB 행 `HeldTakeActionNameText`, `HeldTakeFailureReasonText`와 `RmbKeyText`(`BindWidgetOptional`, `.md/USER_UNREAL.md` 항목 3과 같은 내용). 없으면 RMB 행 `봉투 묶기`가 보이지 않는다.
  - DefaultMap(World Partition external actor package만 저장, `DefaultMap.umap` 저장 금지):
    - 기존 물 얼룩 구역 instance의 `SpawnFloor`를 실제 바닥 높이에 맞추고 욕탕 포함 여부를 확인·resave
    - 쓰레기 구역: 대표 시나리오의 탈의실 등, 손님이 머무는 곳
    - 집게 1개와 `BP_PhysicalCarryFixedSlot` instance 1개(`AssignedItem` = 집게)
    - 수거 구역 1개: 가게 밖. 배송 지점·개봉 자리·도구 거치대·음료 수거함과 겹치지 않음
    - `BP_TrashBin` instance 제거
  - 대표 시나리오 PIE 절차와 TRSH·COLL 관찰 방법. 키, 조준 위치, 기대 HUD 문구(`쓰레기`, `줍기`, `봉투 3/20`, `봉투 묶기`, `쓰레기봉투 (n개)`, `집게가 필요합니다`)를 적는다. 수거는 콘솔 `bathhouse.Debug.TrashCollection.CollectNow`로 확인한다.
- `.md/Architecture/*`는 수정하지 않는다. 설계와 달라야 하면 멈추고 보고한다.
