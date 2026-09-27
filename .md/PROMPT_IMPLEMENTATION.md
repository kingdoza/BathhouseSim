# 구현 프롬프트 — 상점 주문·배송 상자·쓰레기통 수직(샤워기)

## 단계와 입력

- 2026-09-27 사용자가 기능 계약 `.md/PROMPT_ARCHITECTURE.md`(상점 주문·배송 상자·쓰레기통, Q1~Q25)의 설계 진행을 지시했다. 계약서 머리의 "승인 대기" 문구는 기능 명세 단계 소유라 이 단계에서 고치지 않는다.
- 현재 단계: **수직 구현**. 판매 상품은 샤워기 하나다. 대상은 SHOP-001~031, 036, 037이고, SHOP-032~035(확장)는 완료로 보고하지 않는다.
- `.md/AGENT_WORKFLOW.md` → `.md/AGENT_IMPLEMENTATION.md`를 읽고 C++ 구현 단계만 수행한다.
- 설계 정본(전체 읽기): `.md/Architecture/ShopSystem.md`.
- 관련 절:
  - `EconomySystem.md` wallet
  - `PlacementSystem.md` Fresh Install Payload And Discard Tag
  - `PhysicalCarrySystem.md` Consume And Discard Extension
  - `ComputerSystem.md` screen context
  - `UISystem.md` Computer Tab Root·HUD
  - `CoreSystem.md` Core Redirect·Class Growth
- 이 문서와 정본이 다르면 구현을 멈추고 보고한다.
- headless 실행은 `.md/AGENT_WORKFLOW.md` UE 5.8 Headless Automation Policy(`-DDC-ForceMemoryCache` 포함) 형식을 따른다. git은 `--no-optional-locks`로 실행한다.

## 현재 → 목표

| 영역 | 현재 Source | 목표 |
|---|---|---|
| 돈 | 시작 0, 증가만 | `StartingMoney` 100,000, `TrySpendMoney` |
| 설비 아이템 | 회수로만 생성, payload에 domain data 필수 | 신규 설치 payload(null `InstanceData`)와 factory |
| 버리기 | 없음 | `Facility.Discardable` 태그, `IPhysicalCarryDiscardable`, 쓰레기통 |
| 손에서 소모 | 배치 전용 commit | 범용 `CommitConsumeHeldObject`, 배치 경로는 wrapper |
| 상점 | 없음 | catalog·settings·cart·주문 subsystem·배송 지점·상자·개봉 |
| 컴퓨터 화면 | 관리 화면 직접 cast | `IComputerScreenContextReceiver`, 탭 root |
| HUD | 상호작용 prompt만 | 잔액·변화량, 배송 도착 알림 |

## 0. 사전 조건과 백업

- UnrealEditor가 모두 종료됐는지 확인한다.
- `Saved/MigrationBackup/20260927_shop/`에 파일 복사한다:
  - `Content/Bathhouse/Blueprints/Computer/BP_BathhouseComputer.uasset`
  - `Content/Bathhouse/Blueprints/Game/BP_BathhousePlayerState.uasset`
  - `Content/Bathhouse/Blueprints/Game/BP_BathhouseHUD.uasset`
  - `Content/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem.uasset`
- Content·Config·Level을 저장하지 않는다(아래 8의 복사본 확인만 예외).

## 1. Economy

- `UPlayerWalletComponent`:
  - `StartingMoney`(EditDefaultsOnly, 기본 100000, ClampMin 0) 추가.
  - `bWantsInitializeComponent`와 `InitializeComponent`에서 `CurrentMoney = StartingMoney`를 적용한다. 방송하지 않는다.
- `CanSpendMoney`, `TrySpendMoney`(C++ 전용): 양수, 잔액 이하, 한 번 차감, `OnMoneyChanged(Previous, Current)`.
- 기존 cash claim과 `TryAddMoney` 동작은 유지한다. 시작 0을 가정한 기존 Economy test는 harness에서 `StartingMoney`를 0으로 두거나 기대값을 시작 금액 기준으로 바꾼다. 의미 변경 없이 맞춘다.

## 2. Placement

- `FFacilityPlacementPayload::Validate`는 `InstanceData == nullptr`을 허용하고, 추가하는 `IsFreshInstall()`로 구분한다. non-null이면 기존 소유·값 검사를 그대로 한다.
- import 세 곳에서 신규 설치면 domain import를 건너뛰고 class 기본값을 유지한다. 그 밖의 Definition·class·staged 검사는 유지한다.
  - `Private/Facility/BathhouseFacilityPlacementDomain.cpp`
  - `Private/Facility/BathWaterUtilityFacilityActor.cpp`: utility는 operation state 미보유와 같은 신규 0 상태
  - `Private/Towel/TowelProcessingMachinePlacement.cpp`
- `static APlaceableFacilityItemActor* APlaceableFacilityItemActor::SpawnFreshItem(UWorld&, UFacilityPlacementDefinition&, const FTransform&, FText& OutFailure)`
  - 순서: `FacilityActorConversionTransaction`의 회수 item 생성과 같은 deferred spawn(RecoveryItemClass) → `InitializeStaged` → 신규 설치 payload → `FinishSpawning` → `ValidatePlacementPayload`.
  - 실패 시 만든 Actor를 제거하고 null을 반환한다. free-world 활성화는 호출자가 한다.
- native tag `TAG_Facility_Discardable`("Facility.Discardable")를 `UE_DEFINE_GAMEPLAY_TAG`로 Placement type 파일에 선언한다. Config 파일은 수정하지 않는다.
- `UFacilityPlacementDefinition` Data Validation: 이 태그와 `LockerSlotCount > 0`이 함께 있으면 오류.
- `APlaceableFacilityItemActor`가 `IPhysicalCarryDiscardable`을 구현한다.
  - `CanDiscard`: Definition이 태그를 갖고 `LockerSlotCount == 0`이며 배치 staged·소모 중이 아닐 때.
  - `HandleDiscardCommitted`: payload 정리 후 Actor 제거.

## 3. Interaction과 Carry

- 신규 `Public/Interaction/PhysicalCarryDiscardable.h`: `UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))`.
  - `virtual bool CanDiscardCarriedObject(FText& OutFailureReason) const = 0;`
  - `virtual void HandleDiscardCommitted() = 0;`
- `UPlayerCarryComponent::CommitConsumeHeldObject(AActor*, TFunctionRef<bool()>)`: 현재 `CommitReleasePhysicalObjectForPlacement`의 본문(guard, equipment cancel, silent clear, domain commit, 실패 복원, publish)을 옮긴다. 기존 함수는 이를 호출하는 wrapper로 남긴다. 다른 상태·Tick을 추가하지 않는다.
- `EPhysicalCarryKind::DeliveryBox`를 enum 끝에 append한다.

## 4. Shop

`.md/Architecture/ShopSystem.md` Source Scope와 각 절을 그대로 구현한다.

- `ShopTypes.h`: `FShopOrderLine { FName ProductId; TObjectPtr<UFacilityPlacementDefinition> PlacementDefinition; FText DisplayName; int32 Quantity; }`, 주문 snapshot, 결과·실패 enum.
- `UShopCatalog`, `FShopProductEntry`, Data Validation. `UShopSettings`(Config=Game, defaultconfig, 기본값 표 그대로).
- `UShopCartComponent`: `ABathhousePlayerState` 생성자의 default subobject(`ShopCart`). 모든 API와 `OnCartChanged`.
- `UShopOrderSubsystem : UTickableWorldSubsystem`: Game·PIE world에서만 생성(`DoesSupportWorldType`).
  - `EvaluatePlaceOrder`, `TryPlaceOrder`, 0.25초 throttle 배송 Tick, `OnOrdersChanged`, `OnOrderDelivered`, `GetOrderSnapshots`.
  - 배송 지점 등록·해제.
- `AShopDeliveryPointActor`: `FindDropTransform` 1~4. editor-only billboard·arrow는 `CreateEditorOnlyDefaultSubobject`.
- `AShopDeliveryBoxActor`: 4개 interface, `InitializeContents`, 요약 문구, LMB → 개봉. 삽·설비 아이템의 carry 구현을 참고하되 공통 carry base를 만들지 않는다.
- `FShopUnboxingPlacement`, `FShopUnboxingTransaction`: 1~3단계 배치, 생성 → free-world 활성 → carry consume → 상자 제거 순서와 실패 rollback.
- `ABathhouseTrashBinActor`: 판정 표와 실행 순서.
- 금액 계산은 int64로 누적하고 int32 범위를 넘으면 `InvalidProduct`로 거부한다.

## 5. Computer

- 신규 `Public/Computer/ComputerScreenContext.h`:
  - `FComputerScreenContext { TWeakObjectPtr<ABathhouseComputerActor> Computer; TWeakObjectPtr<UBathWaterOperationsSubsystem> Operations; TWeakObjectPtr<AFacilityPlacementZoneActor> ManagedZone; }`
  - `IComputerScreenContextReceiver`: `InitializeComputerScreen(const FComputerScreenContext&)`, `NotifyComputerUserChanged(APlayerState*)`
- `ABathhouseComputerActor::BeginPlay`: user widget이 interface를 구현하면 context를 전달한다. 기존 관리 화면 직접 cast를 제거하고, `UBathWaterManagementScreenWidget`이 interface를 구현해 기존 `InitializeManagementContext`로 위임한다.
- 사용 시작 성공 시(reservation + session begin 성공 직후) `NotifyComputerUserChanged(Controller의 PlayerState)`. 종료 시에는 호출하지 않는다.
- 컴퓨터 포커스·이탈 로직은 바꾸지 않는다.

## 6. UI

- `UComputerScreenRootWidget`: BindWidget 5개. 탭 버튼이 `ScreenSwitcher` index를 바꾸고, 활성 탭 버튼은 비활성 표시한다. context·사용자를 자식에 전달한다.
- `UShopScreenWidget`과 행 widget 3종: `ShopSystem.md` UI 절의 BindWidget과 구독·갱신 규칙.
  - 구독은 `NativeConstruct`·`NotifyComputerUserChanged`에서 하고 `NativeDestruct`·사용자 변경에서 해제한다.
  - 행 widget 재생성은 cart·주문 변경 시에만 한다. 남은 시간 표시는 1초 간격으로만 갱신한다.
- `UMoneyHudWidget`, `UShopNoticeWidget`.
- `ABathhouseHUD`: `MoneyHudWidgetClass`, `ShopNoticeWidgetClass` 추가, 생성·bind·EndPlay 해제.
  - wallet bind는 PlayerController의 PlayerState가 생길 때까지 possession 변경과 짧은 timer로 재시도한다.
  - notice는 order subsystem을 구독한다.

## 7. 빌드

- `.md/AGENT_WORKFLOW.md`의 UE 5.8 `Build.bat`만 사용한다.

## 8. 로드 검증 — 복사본 먼저

기존 Blueprint load automation 형식을 따라 `BathhouseSim.Shop.BlueprintLoad`를 추가한다.

- 대상: 기본 4개 Blueprint, `-BathhouseShopLoadPath=`로 단일 package 지정.
- 확인:
  - native parent
  - PlayerState의 `ShopCart`·wallet `StartingMoney` 기본값
  - 컴퓨터 `ScreenWidget` WidgetClass 비어 있지 않음
  - HUD 기존 `InteractionPromptWidgetClass` 유지
- 저장하지 않는다.

순서(Fatal·`Serial size mismatch`·`Failed to load`면 즉시 멈춤):

1. 4개를 `Content/Developers/MigrationCheck/`에 복사하고 Template 맵으로 로드한다.
2. 복사본을 삭제하고 Content 무변경을 확인한다.
3. 원본을 로드한다.
4. DefaultMap으로 로드한다.
5. 다시 무변경을 확인한다.

## 9. 자동화

`ShopSystem.md` Verification 표 전체를 구현한다. 추가 조건:

- 신규 설치 샤워기(SHOP-022): `SpawnFreshItem` → 기존 배치 transaction으로 배치 → class 기본값(번호 없음·가중치 1) 확인 → 회수 → 재배치 가능.
- 보일러 신규 설치 payload: 배치 시 잔량 0. 확장 SHOP-034의 사전 확인이며 완료로 보고하지 않는다.
- 쓰레기통: 샤워기·회수 보일러(SHOP-037)는 가능. 락커 1·4·8(SHOP-036), 열쇠, 걸레, 렌치, 바구니, 삽은 거부. 빈손 거부.
- 개봉 배치: 트인 바닥, 50cm 앞 벽(벽 너머 없음), 사방이 막힌 구석(위로 쌓기). 아이템이 Pawn을 무시함.
- 배송 지점: 빈 지점, 상자 위에 쌓기, 낮은 천장으로 대기 뒤 상자 제거 시 도착, 두 주문 FIFO, 딜레이 0.
- carry consume: 성공 시 `OnHeldObjectChanged` 1회, domain 실패 시 원래 소지 복원, 배치 session 종료.
- 기존 회귀(유지 필수): Economy, Placement 회수·배치 전체, Computer, Utility, Interaction.
- 전체 회귀: `Automation RunTests BathhouseSim`. 수치와 실패 이름을 보고하고, 실패는 스스로 무관 판정하지 않는다.

## 금지

- 기존 export를 가진 class·property·subobject rename·삭제·class 변경, native `Serialize` 변경, Core Redirect 추가.
- Config 파일 수정(gameplay tag는 native 선언, Project Settings 값은 Editor 단계).
- Widget에 cart·주문·돈 보관, generic Interaction/Character에 상점 concrete 분기.
- 확장 상품 판매 활성화, 락커 판매·버리기 허용.
- Content·Level 저장과 Editor authoring.

## 단계 결과물

- `.md/PROMPT_REVIEW.md`: 변경 파일, 영역별 요약, 8의 결과와 로그 위치, automation 수치와 SHOP별 대응, 미검증(PIE 전용).
- `.md/PROMPT_UNREAL.md`: `ShopSystem.md` Blueprint/API Contracts의 Editor 목록을 실제 결과에 맞게 구체화한다.
  - 수직 가격은 Editor에서 정한다.
  - 샤워기 외 6종은 catalog에 `bForSale=false`로 등록할 수 있다.
  - `Facility.Discardable` 태그는 7종 Definition 모두에 단다.
- `.md/Architecture/*`는 수정하지 않는다. 설계와 달라야 하면 멈추고 보고한다.
