# PROMPT_IMPLEMENTATION — DOC-TUNING-REFS 코드 상수의 데이터 이전
- 작업 ID: `DOC-TUNING-REFS`
- 단계: 아키텍처
- 상태: 보류 — 사용자 지시로 구현 대기, 구현 단계, 사용자 재개 지시

## 1. 기능 계약과 범위

- 기능 명세 생략(사용자 동작 변화 없음, `CONTEXT.md`). 입력은 사용자 지시(2026-10-01) "코드 상수 자체 작성하지 말고 원본 데이터가 위치한 곳을 참조하도록 해야 해. 값 바꾸면 문서도 바꿔야 하잖아."
- 이번 아키텍처 단계에서 `ShopSystem.md`, `CleaningLitterSystem.md`, `InteractionSystem.md`의 조정값 숫자를 원본 위치 참조로 바꿨다. 원본이 데이터(UPROPERTY·Config·BP)인 값은 문서 정리로 끝났다. 이 문서는 원본이 코드 상수로 남은 값만 데이터로 옮기는 설계다.
- 현재 단계: 단일 리팩터(수직·확장 구분 없음). 시나리오 ID 없음.
- 목적: 아래 상수를 데이터 property로 옮기되 **모든 기본값 = 현재 상수**라 동작이 바뀌지 않는다.
- 비목표: 값 조정, 새 동작, UNBOX-SPAWN-VIEW가 다루는 개봉 위치 계산·봉투 놓기(`ShopUnboxing*`, `TrashBagDropPlacement`, `ALitterTongsActor::Tie*`) 상수, 부동소수 epsilon·기하 정의, 설정 유효 범위(ClampMin·getter clamp), 테스트 seed·허용 오차·fixture 기하(제품 값이 아님).

## 2. 분류 원칙(UNBOX-SPAWN-VIEW와 같음)

| 수 | 처리 |
|---|---|
| 동작을 조정하는 값 | 데이터 property. 기본값 = 현재 상수 |
| 데이터 기본값을 복제한 리터럴(getter fallback, 함수 기본 인자, struct 기본값) | 한 원본을 같이 쓰거나 제거 |
| epsilon·기하 정의·ClampMin·getter clamp | 코드에 둔다(조정값 아님) |
| 테스트 fixture | 테스트에 둔다. 단 제품 기본값을 고정한 기대값은 원본에서 계산 |

## 3. 값별 이전 설계

### 3.1 상점 배송 재시도 간격

- 현재: `Private/Shop/ShopOrderSubsystem.cpp` `Tick`의 `NextDeliveryAttemptTime = Now + 0.25`.
- 원본: `UShopSettings::DeliveryAttemptIntervalSeconds` — `UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "0.0"))`. 0이면 매 Tick 시도.
- 기본값: header `static constexpr float DefaultDeliveryAttemptIntervalSeconds = 0.25f`를 member 초기값과 getter fallback이 같이 쓴다. getter `GetDeliveryAttemptIntervalSeconds()`: 유한이면 `Max(0, 값)`, 아니면 Default.
- 읽는 지점: `UShopOrderSubsystem::Tick`에서 `GetDefault<UShopSettings>()->GetDeliveryAttemptIntervalSeconds()`.
- Config: 키 추가 없음(키가 없으면 C++ 기본값). Content 변경 없음.

### 3.2 상점 Settings getter fallback 중복

- 현재: `ShopSettings.cpp` `GetDeliveryDelaySeconds`의 `: 10.0f`, `GetDeliveryNoticeSeconds`의 `: 3.0f`, `Private/UI/ShopNoticeWidget.cpp`의 `Settings ? … : 3.0f`.
- 설계: `DefaultDeliveryDelaySeconds`, `DefaultDeliveryNoticeSeconds` 상수를 header에 두고 member 초기값과 getter fallback이 같이 쓴다. `ShopNoticeWidget`은 `GetDefault<UShopSettings>()`가 null이 아니므로 삼항을 지우고 getter만 호출한다.
- 동작·Config·Content 변화 없음. `Config/DefaultGame.ini`의 기존 `DeliveryDelaySeconds` 저장값은 그대로 우선한다.

### 3.3 상점 화면 남은 시간 갱신 간격

- 현재: `Private/UI/ShopScreenWidget.cpp` `NativeTick`의 `1.0f` 두 곳(비교·`Fmod`).
- 원본: `UShopScreenWidget::CountdownRefreshIntervalSeconds` — `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|Display", meta = (ClampMin = "0.1", UIMin = "0.1"))`, 기본 `1.0f`.
- 읽는 지점: `NativeTick`에서 `const float Interval = FMath::Max(0.1f, CountdownRefreshIntervalSeconds)`(ClampMin과 같은 하한, `UMoneyHudWidget::DeltaDisplaySeconds`와 같은 방식)로 두 곳을 바꾼다.
- `/Game/Bathhouse/UI/Shop/WBP_ShopScreen`은 C++ 기본값을 상속한다. resave·Core Redirect 불필요.

### 3.4 쓰레기·물 얼룩 생성 clearance box 띄움 높이

- 현재: `Private/Cleaning/CleaningSpawnRules.cpp` `FCleaningFloorSpawnQuery::Find`의 overlap 중심 `Hit.ImpactPoint + FVector(0, 0, 1 + S.ClearanceHeight / 2)`의 `1`.
- 원본: `ACleaningDirectorActor::SpawnClearanceFloorOffsetCm` — `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning Spawn", meta = (ClampMin = "0.0"))`, 기본 `1.0f`. `SpawnClearanceHeightCm`과 같은 owner(공통 clearance 값)다.
- 전달: `FCleaningFloorSpawnSettings::ClearanceFloorOffset`(신규). 두 구역의 `FindSpawnTransform`에 인자를 추가하고 director `TrySpawnStain`·`TrySpawnLitter`가 넘긴다. `Find`는 비유한·음수면 false(다른 clearance 값과 같은 입력 검사).
- 같은 변경에서 데이터 기본값 복제를 지운다.
  - `AStainSpawnZoneActor::FindSpawnTransform`·`ALitterSpawnZoneActor::FindSpawnTransform`의 기본 인자(`FloorRadius`, `ClearanceHeight`)를 제거한다. 모든 호출자가 값을 명시한다.
  - `FCleaningFloorSpawnSettings` member 기본값(trace 거리·경사·허용 오차·반경·높이·간격)을 0/중립으로 바꾼다. 구역이 항상 모두 채우며, 빠뜨리면 `Find` 입력 검사로 실패한다.
- 테스트용 C++ 전용 public getter: `ACleaningDirectorActor::GetSpawnClearanceHeightCm()`, `GetSpawnClearanceFloorOffsetCm()`.
- `BP_CleaningDirector`는 C++ 기본값을 상속한다. resave·Core Redirect 불필요.

### 3.5 배치 확정 발밑 정리 높이 허용 오차

- 현재: `CleaningSpawnRules.cpp` `FCleaningFootprintOverlap::Intersects`의 `- 5`, `+ 5`.
- 원본: 종류별 Actor 값. `AWaterStainActor::PlacementClearHeightToleranceCm`, `ALitterActor::PlacementClearHeightToleranceCm` — `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))`, 기본 `5.0f`, Category는 각 class의 `FloorRadiusCm`과 같게. getter `GetPlacementClearHeightToleranceCm()`.
- 전달: `Intersects(Center, Radius, Transform, Extent, HeightToleranceCm)`로 인자를 추가하고 `UCleaningWorldSubsystem::HandleFacilityPlaced`가 `Entry->GetFloorRadius()`와 함께 넘긴다.
- owner 선택 근거: 같은 판정의 반경 R이 이미 종류별 Actor 값이다. director는 레벨에 없을 수 있고 subsystem이 director를 모른다. 값 하나를 위한 새 DeveloperSettings는 과하다.
- `BP_WaterStain`·`BP_Litter`는 C++ 기본값을 상속한다. resave·Core Redirect 불필요.

### 3.6 carry 놓기 속도의 코드 기본값

- 현재
  - `Public/Interaction/PhysicalCarryable.h` `IPhysicalCarryable::GetThrowImpulseStrength()`·`GetUpwardThrowImpulseStrength()` 기본 구현이 `120.0f`·`15.0f`를 반환한다.
  - `AUtilityShovelActor`는 `ThrowImpulseStrength`·`UpwardThrowImpulseStrength` UPROPERTY를 선언했지만 getter를 override하지 않아 위 상수를 쓴다(결함: Editor에서 값을 바꿔도 반영되지 않음).
  - `ABathhouseFacilityActor`, `ABathWaterUtilityFacilityActor`, `ATowelProcessingMachineActor`의 getter가 `FacilityPlacement` 없음 fallback으로 `120.0f`·`15.0f`를 반환한다.
- 설계
  - 두 인터페이스 함수를 pure virtual(`= 0`)로 바꾼다. 같은 인터페이스의 `CanBeTakenBy` 등과 같은 방식이며, 새 carryable이 값을 소유하지 않으면 컴파일 오류가 난다. 현재 구현체 중 override가 없는 것은 삽뿐이다(테스트 구현체 없음).
  - `AUtilityShovelActor`에 두 getter override를 추가해 자기 UPROPERTY를 반환한다.
  - 세 설비 Actor fallback은 `GetDefault<UFacilityPlacementComponent>()`의 같은 getter로 바꾼다(component class 기본값이 원본). 배치 설비 Actor는 `CanBeTakenBy`가 false라 이 값은 놓기에 쓰이지 않으며 동작 변화가 없다.
- 동작 보존 근거: 삽 UPROPERTY 기본값 = 현재 인터페이스 상수. 이 단계에서 `BP_UtilityShovel.uasset` 이름표를 읽기 전용으로 검사했을 때 두 property 이름이 없어 CDO override 흔적이 없다. 구현 단계는 이 근거를 `PROMPT_UNREAL.md`에 적고, Editor 단계는 CDO 값이 C++ 기본값과 같은지 읽기로 확인한다. 다르면 저장하지 않고 멈춰 사용자에게 묻는다(값 선택은 사용자 결정).

## 4. 테스트 기대값

기대값은 숫자로 쓰지 않고 원본에서 계산한다. fixture 값은 테스트가 설정·복원하는 지역 값으로 둔다.

| 테스트 | 변경 |
|---|---|
| `ShopAutomationTests.cpp` `OrderDelayWaitingAndFIFO` | SettingsGuard에 `DeliveryAttemptIntervalSeconds`를 저장·복원 대상으로 추가하고 fixture 간격을 설정한다. world tick 간격 = 그 값, 대기 tick 수 = 딜레이 fixture를 간격으로 나눈 올림 + 1. 차단 해제 뒤 한 간격 tick으로 둘 다 도착 |
| `ServiceFridgeAutomationTests.cpp` Settings guard | 같은 property를 저장·복원 대상에 추가(값을 바꾸지 않으면 guard만) |
| 신규 `BathhouseSim.Shop.SettingsDefaults`(기존 Shop 테스트 파일) | `DeliveryDelay`·`DeliveryNotice`·`DeliveryAttemptInterval` getter가 비유한 입력에서 `UShopSettings::Default*`를 반환. 숫자 리터럴 없이 상수와 비교 |
| `CleaningSpawnClearanceAutomationTests.cpp` | `FSpawnZones::Expect`가 반경 fixture를 명시하고 높이·띄움은 `GetDefault<ACleaningDirectorActor>()` getter로 넘긴다. `SlopeAndStep`의 다른 mesh 턱 높이는 띄움 값 기준 상대 배치(띄움 + 여유)로 만들고, 주석의 "1 cm"를 "floor offset"으로 바꾼다 |
| `CleaningTowelAutomationTests.cpp` 구역 후보 테스트 | 바뀐 시그니처에 반경 fixture와 director CDO 높이·띄움을 명시 |
| `CleaningLitterSpawnAutomationTests.cpp` footprint | `Intersects`에 허용 오차 fixture를 넘기고 경계 안·밖 Z를 그 값 기준으로 계산. subsystem 경로 단언은 Actor getter 값을 사용 |
| `PhysicalCarryFixedSlotAutomationTests.cpp` 120/15 단언 | 기대 속도 = `Basket->GetThrowImpulseStrength()`·`GetUpwardThrowImpulseStrength()` |
| 신규 `BathhouseSim.Utility.Labor.ShovelReleaseVelocityUsesProperty` | 삽 인스턴스 property를 기본값과 다른 fixture로 바꾸면 getter가 그 값을 반환(결함 회귀) |
| `HeldTargetUseAutomationTests.cpp` 851행 | BP component 값이 C++ 기본값을 유지하는지 보는 회귀다. 리터럴 대신 `GetDefault<UPlayerHeldTargetUseComponent>()->RepeatIntervalSeconds`와 비교 |

`BathhouseEconomyTests.cpp`의 `StartingMoney == 100000`, `+10000` 단언도 데이터 기본값 고정이다. 이 작업 범위 밖이며 리뷰 참고로만 남긴다(`ABathhouseCashPaymentActor::GetPaymentAmount()`로 바꿀 수 있음).

## 5. 대상 파일과 책임 변화

| 파일 | 변경 |
|---|---|
| `Public/Shop/ShopSettings.h`, `Private/Shop/ShopSettings.cpp` | 3.1, 3.2 |
| `Private/Shop/ShopOrderSubsystem.cpp` | 3.1 읽기 |
| `Private/UI/ShopNoticeWidget.cpp` | 3.2 |
| `Public/UI/ShopScreenWidget.h`, `Private/UI/ShopScreenWidget.cpp` | 3.3 |
| `Public/Cleaning/CleaningDirectorActor.h`, `Private/Cleaning/CleaningDirectorActor.cpp` | 3.4 property·getter·전달 |
| `Public/Cleaning/StainSpawnZoneActor.h/.cpp`, `LitterSpawnZoneActor.h/.cpp` | 3.4 시그니처 |
| `Private/Cleaning/CleaningSpawnRules.h/.cpp` | 3.4, 3.5 |
| `Public/Cleaning/WaterStainActor.h`, `LitterActor.h`, `Private/Cleaning/CleaningWorldSubsystem.cpp` | 3.5 |
| `Public/Interaction/PhysicalCarryable.h`, `Public/Utility/UtilityShovelActor.h`, 세 설비 Actor `.cpp` | 3.6 |
| 4절 테스트 파일 | 기대값 |

- 상태 owner 변화 없음. authoring owner: 상점 값 `UShopSettings`(Project Settings·`DefaultGame.ini`), 화면 갱신 간격 `WBP_ShopScreen`, clearance 띄움 `BP_CleaningDirector`, 발밑 정리 허용 오차 `BP_WaterStain`·`BP_Litter`, 놓기 속도 각 carryable BP.
- 의존 방향 변화 없음(설비 Actor → Placement component는 기존 의존).
- 클래스 성장: 각 class에 property 1~2개와 getter만 추가한다. 독립 책임 추가 없음.

## 6. Lifecycle·전역 설정·Blueprint 영향

- lifecycle·rollback 변화 없음. 모든 값은 읽기 시점이 기존 상수 사용 지점과 같다.
- 전역 설정: `UShopSettings`에 Config property 하나 추가(키 없음). Project·Collision·Input 설정 변경 없음.
- Blueprint/API: reflected property 추가만. rename·삭제 없음, Core Redirect 불필요. `IPhysicalCarryable` 두 함수의 pure virtual 전환은 C++ 전용 계약 변경이다(Blueprint 비노출).
- Content·Config 변경 없음. `PROMPT_UNREAL.md`는 Content 변경 없음과 3.6의 `BP_UtilityShovel` 읽기 확인 1건만 적는다.

## 7. 순서와 병합 의존

- `work/UNBOX-SPAWN-VIEW`가 `ShopSettings.h/.cpp`에 `Default*` 패턴과 개봉 property를 추가한다. 3.1·3.2는 그 브랜치 병합 뒤 그 패턴 위에 구현한다(같은 파일 충돌 방지). 병합 전에 재개되면 구현 단계가 마스터에게 순서를 묻는다.
- 정본 문서의 "이전 예정" 문구(ShopSystem Delivery·UI·Catalog And Settings, CleaningLitterSystem Floor Rule·Footprint, InteractionSystem Editor authoring 값)는 구현 완료 뒤 원본 property 이름 참조로 바꾼다. 이 갱신은 구현 단계 결과를 받은 아키텍처 또는 마스터가 한다.

## 8. 구현 금지 범위

- 값 변경 금지(모든 기본값 = 현재 상수).
- UNBOX-SPAWN-VIEW 대상(`ShopUnboxing*`, `TrashBagDropPlacement`, `ALitterTongsActor` Tie 값, `PlayerViewFrontPlacement`) 수정 금지.
- Content·Config 저장 금지. 기존 property rename·삭제 금지.

## 9. 검증

- 빌드: `AGENT_WORKFLOW.md` UE 5.8 Build Policy.
- 자동화(Headless Policy): `BathhouseSim` 전체 통과(carry 인터페이스 변경이 Towel·Combat·Facility·Service carryable까지 닿으므로 부분 filter로 줄이지 않는다). 4절 신규 테스트 포함.
- 코드 리뷰 기준: 2절 분류에 맞지 않는 새 리터럴이 없음, 기본값 = 현재 상수, 데이터 기본값 복제 리터럴 제거, 삽 결함 회귀 테스트 존재.
- 사용자 PIE(동작 불변 확인): 주문 → 배송 도착과 상점 화면 남은 시간 갱신, 쓰레기·물 얼룩 생성과 설비 배치 시 발밑 정리, 삽·상자·설비 아이템 G 놓기 속도가 이전과 같음.
