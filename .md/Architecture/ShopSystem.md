# Shop System

## Status And Scope

- 2026-09-27 설계, Source 미반영. 입력은 `.md/PROMPT_ARCHITECTURE.md`(상점 주문·배송 상자·쓰레기통, SHOP-001~037)와 `.md/QNA_FEATURE_SPEC.md` Q1~Q25다. 기능 계약과 설계 진행은 2026-09-27 사용자가 지시했다.
- 수직 단계: 판매 상품은 샤워기 하나. 대상 SHOP-001~031, 036, 037. 확장 단계(SHOP-032~035, 7종 판매·혼합 주문)는 수직 사용자 승인 뒤 상품 목록 authoring과 검증만 추가하는 것을 목표로 구조를 잡는다.
- 주문 취소·환불, 재고·할인, 배송 지점 여러 곳, 저장·멀티플레이, 개봉 애니메이션은 만들지 않는다.

## Source Scope

```text
Source/BathhouseSim/Public/Shop/
  ShopTypes.h                  주문 줄·주문 snapshot·결과 코드
  ShopCatalog.h                UShopCatalog (UPrimaryDataAsset)
  ShopSettings.h               UShopSettings (UDeveloperSettings)
  ShopCartComponent.h          UShopCartComponent (PlayerState)
  ShopOrderSubsystem.h         UShopOrderSubsystem (UTickableWorldSubsystem)
  ShopDeliveryPointActor.h     AShopDeliveryPointActor
  ShopDeliveryBoxActor.h       AShopDeliveryBoxActor
  BathhouseTrashBinActor.h     ABathhouseTrashBinActor
Source/BathhouseSim/Private/Shop/
  (위 .cpp), ShopUnboxingPlacement.h/.cpp, ShopUnboxingTransaction.h/.cpp
```

UI native class는 `Public/UI`, `Private/UI`에 둔다([UISystem.md](UISystem.md)). 새 Source 폴더 `Shop`은 이 문서가 정본이다.

## Ownership

| 대상 | 상태 owner | 실행 owner | 비고 |
|---|---|---|---|
| 돈 | `UPlayerWalletComponent` | wallet | 시작 금액·차감 추가([EconomySystem.md](EconomySystem.md)) |
| 장바구니 | `UShopCartComponent` | cart | PlayerState에 붙어 컴퓨터·탭·Pawn과 무관하게 유지 |
| 주문·배송 | `UShopOrderSubsystem` | subsystem Tick | 월드 단위 FIFO, 게임시간 |
| 배송 위치 | `AShopDeliveryPointActor` | point | 공간 판정 계산만 제공 |
| 상자 내용 | `AShopDeliveryBoxActor` | box | 생성 시 한 번 정해지고 불변 |
| 개봉 | 없음 | `FShopUnboxingTransaction` | 위치 계산·아이템 생성·상자 소모를 한 호출로 조율 |
| 새 설비 아이템 | `APlaceableFacilityItemActor` payload | Placement factory | 신규 설치 payload([PlacementSystem.md](PlacementSystem.md)) |
| 버리기 판정 | 들고 있는 물건 | `ABathhouseTrashBinActor` | `IPhysicalCarryDiscardable`([PhysicalCarrySystem.md](PhysicalCarrySystem.md)) |
| 탭·화면 표시 | 없음(표시 상태만) | UI widget | domain mutation은 위 owner API만 호출 |

화면(Widget)은 장바구니·주문·돈을 보관하지 않는다. 컴퓨터를 나가도 widget이 유지되지만, 유지되지 않아도 domain 상태는 그대로다.

## Catalog And Settings

`UShopCatalog : UPrimaryDataAsset`의 `Products : TArray<FShopProductEntry>`가 상품 정의의 유일 정본이다. 배열 순서가 화면 카드 순서다.

| 필드 | 규칙 |
|---|---|
| `ProductId` (FName) | 고유, None 금지 |
| `bForSale` | 화면 표시·담기 허용 여부. 수직 단계는 샤워기만 true |
| `DisplayName` (FText) | 카드·장바구니·요약 문구 |
| `Price` (int32) | > 0 |
| `Icon` (`TSoftObjectPtr<UTexture2D>`) | 선택. 없으면 이름 표시로 대체 |
| `PlacementDefinition` | 필수. placement 활성, `LockerSlotCount == 0`, `Facility.Discardable` 태그 보유 |

- Data Validation은 위 규칙을 검사한다. 락커 판매와 "팔지만 버릴 수 없는 설비"를 데이터 단계에서 막는다.
- 설비 정의에 가격을 넣지 않는다. 가격 편집 위치는 catalog 하나다.

`UShopSettings : UDeveloperSettings`(Config=Game, Project Settings 노출):

| 값 | 기본 | 범위 |
|---|---:|---|
| `Catalog` (soft) | 없음, Editor 지정 | 필수 |
| `DeliveryBoxClass` (soft class) | 없음, Editor 지정 | `AShopDeliveryBoxActor` 자식 |
| `CartTotalQuantityLimit` | 10 | ≥ 1 |
| `PerProductQuantityLimit` | 99 | ≥ 1 |
| `DeliveryDelaySeconds` | 10 | finite ≥ 0 |
| `UnboxForwardDistanceCm` | 100 | finite ≥ 0 |
| `DeliveryNoticeSeconds` | 3 | finite > 0 |

시작 금액은 wallet 소유, 잔액 변화량 표시 2초는 HUD widget 소유다.

## Cart

`UShopCartComponent`는 `ABathhousePlayerState`의 default subobject다. 줄은 `{ProductId, Quantity}`이며 처음 담은 순서를 유지한다.

- `EvaluateAdd(ProductId)`: 판매 중 상품, 상품 수량 < 상품별 상한, 전체 수량 < 전체 상한. 실패 코드 `NotForSale / ProductLimit / CartLimit`.
- `TryAdd`, `TryIncrement`는 `EvaluateAdd`를 쓴다. `TryDecrement`는 0이 되면 줄을 지운다. `TryRemoveLine`은 줄을 지운다.
- 잔액과 무관하다(SHOP-007).
- 변경 시 `OnCartChanged`(native·Blueprint)를 한 번 방송한다. 조회는 `GetLines`, `GetTotalQuantity`, `CalculateTotalPrice(Catalog)`(int64 누적 후 int32 범위 확인).
- 판매 중지된 상품 줄은 표시와 합계에서 제외하지 않고 주문 평가에서 실패시킨다(수직·확장 전환 중 catalog 변경 대비).

## Order Placement

`UShopOrderSubsystem::EvaluatePlaceOrder(PlayerState)` → `{bCanOrder, Failure, TotalPrice, ShortfallAmount}`. 실패: `EmptyCart / InvalidProduct / InsufficientMoney / Busy / MissingCatalog`.

`TryPlaceOrder(PlayerState)`(동기, 재진입 guard):

1. 평가 통과 확인. 이때 cart 줄을 catalog로 해석해 주문 줄 `FShopOrderLine {ProductId, PlacementDefinition, DisplayName, Quantity}`을 만든다. 이후 catalog가 바뀌어도 주문·상자 내용은 이 snapshot을 쓴다.
2. `Wallet->TrySpendMoney(TotalPrice)`. 실패하면 아무것도 바꾸지 않는다.
3. 주문 `{OrderId(증가), Lines, ReadyGameTime = Now + DeliveryDelaySeconds, bWaitingForSpace=false}`를 FIFO 끝에 추가하고 cart를 비운다. 이 단계는 실패하지 않는다.
4. `OnOrdersChanged`를 방송한다. 딜레이가 0이면 같은 호출 안에서 배송을 시도한다(SHOP-012).

- 빠른 두 번째 클릭은 cart가 비어 `EmptyCart`로 실패한다(SHOP-010). guard 중 재진입도 `Busy`다.
- 게임시간은 `UWorld::GetTimeSeconds()`다. pause에 멈추고 컴퓨터 사용 중에도 흐른다(SHOP-030).

## Delivery

subsystem Tick(0.25초 간격 throttle):

1. FIFO 머리 주문만 본다. 머리가 준비되지 않았거나 공간이 없으면 뒤 주문도 처리하지 않는다(SHOP-015).
2. 머리가 준비되면 등록된 배송 지점의 `FindDropTransform(BoxHalfExtent)`로 위치를 구한다.
3. 성공: 상자를 deferred spawn → `InitializeContents(OrderId, Lines)` → `FinishSpawning` → free-world 물리 활성. 주문을 제거하고 `OnOrderDelivered(OrderId)`, `OnOrdersChanged`를 방송한다. 이어서 다음 머리를 같은 Tick에 시도한다.
4. 실패: `bWaitingForSpace=true`로 표시하고 멈춘다(SHOP-014). 다음 throttle에 다시 시도한다.

- 배송 지점이 없거나 둘 이상이면 경고 로그를 남긴다. 없으면 모든 준비된 주문이 대기이고, 둘 이상이면 먼저 등록된 지점을 쓴다.
- 상자 class가 없거나 spawn이 실패하면 대기로 두고 오류 로그를 남긴다. 돈은 이미 차감됐으므로 주문을 버리지 않는다.
- UI 조회 `GetOrderSnapshots()`: 요약 줄, 남은 초(`max(0, Ready - Now)`), 대기 여부.

## Delivery Point

`AShopDeliveryPointActor`: root `USceneComponent`, editor-only billboard·arrow(표시 전용). `MaxSearchHeightCm=1000`(EditAnywhere, > 0), `DropGapCm=10`(EditDefaultsOnly, ≥ 0). BeginPlay에 subsystem에 등록, EndPlay에 해제한다.

`FindDropTransform(BoxHalfExtent, BoxCollisionTemplate)`:

1. 지점 위치 B에서 위로 `MaxSearchHeightCm`까지 WorldStatic object type만 line trace해 천장 높이 Zc를 구한다. 상자·설비 아이템·Pawn은 천장으로 보지 않는다. 없으면 `Zc = B.Z + MaxSearchHeightCm`.
2. `(B.XY, Zc - HalfZ - 1)`에서 `(B.XY, B.Z + HalfZ)`까지 상자 collision으로 box sweep한다. 시작이 이미 막혀 있으면 공간 없음이다.
3. hit가 있으면 그 중심 높이, 없으면 `B.Z + HalfZ`가 더미 위 중심이다. spawn 중심은 그 높이 + `DropGapCm`.
4. spawn 상자 윗면이 Zc를 넘거나 spawn 위치 overlap이 있으면 공간 없음이다. 아니면 지점 yaw로 transform을 반환한다.

상자는 Pawn을 무시하는 free-world 물체라 지점 위에 선 플레이어를 밀지 않고 통과해 떨어진다. 쌓인 더미가 무너져도 되돌리지 않는다.

## Delivery Box

`AShopDeliveryBoxActor`: root `BoxMesh`(physical root). 구현 interface: `IPlayerInteractable`, `IPhysicalCarryable`, `IHeldEquipmentUsable`, `IPhysicalCarryDiscardable`.

- carry: `EPhysicalCarryKind::DeliveryBox`(enum 끝에 append), capability `FreeDrop`만(지정 거치대 없음). free-world는 기존 carry 계약대로 CCD·Pawn Ignore·약한 release다.
- 내용: `OrderId`, `Contents : TArray<FShopOrderLine>`. `InitializeContents`는 spawn 중 한 번만 허용하고 이후 불변이다. drop·재집기·carrier 복구에서 같은 Actor가 유지된다(SHOP-017).
- 요약 문구 `배송 상자 — 샤워기 2, 보일러 1`: 줄 순서대로 `DisplayName 수량`을 쉼표로 잇는다.
- E query: 빈손이면 `상자 들기`, TargetName에 요약을 쓴다(SHOP-016).
- LMB equipment query: DisplayName에 요약, action `상자 열기`, `Instant`. 기존 합성 규칙에 따라 조준 대상이 없으면 요약이 target 이름으로 보인다. 컴퓨터·배치가 LMB를 먼저 가져가는 우선순위는 그대로다(SHOP-021).
- `BeginEquipmentUse` → `FShopUnboxingTransaction::Open`. End/Cancel은 아무것도 되돌리지 않는다.
- 버리기: 항상 가능. 버리면 내용물도 함께 사라진다(SHOP-025).
- validation: mesh 지정, root QueryAndPhysics·CCD·Pawn Ignore, held unit scale.

## Unboxing

`FShopUnboxingPlacement`(순수 world query helper):

- 입력: world, player capsule·위치, 플레이어 view yaw, 아이템 목록(주문 줄을 수량만큼 펼친 Definition 목록, 줄 순서), `UnboxForwardDistanceCm`.
- 각 아이템 shape는 `APlaceableFacilityItemActor::BuildDefinitionCollisionQuery`와 같은 Definition 기준 collision이다. 겹침 검사는 기존 `FacilityPlacementCollision::HasBlockingOverlap`을 쓴다.
- 1단계, 정면 줄: 거리 d를 설정값부터 10cm씩 0까지 줄이며 시도한다. d마다 아이템을 플레이어 오른쪽 방향으로 폭 + 10cm 간격의 한 줄로 가운데 정렬하고, 높이는 발바닥 + 아이템 반높이 + 20cm로 둔다. 모든 아이템이 겹침 없고 플레이어 중심(같은 높이)에서 각 중심까지 line trace가 막히지 않으면 그 d를 채택한다(SHOP-018, 019).
- 2단계, 위로 쌓기: 플레이어 XY, capsule 윗면 + 10cm부터 아이템 높이 + 10cm씩 올려 쌓는다. 각 칸이 겹침 없고 머리에서 위로 line trace가 막히지 않아야 한다(SHOP-020).
- 3단계, 최후: 플레이어 위치(발바닥 + 반높이)에서 아이템 높이씩 쌓는다. Pawn만 겹치고 다른 물체와 겹치면 경고 로그를 남긴다. 정상 공간에서는 도달하지 않는 경로다.
- 아이템은 free-world에서 Pawn을 무시하므로 플레이어를 밀지 않는다.

`FShopUnboxingTransaction::Open(Box, Context)`:

1. 들고 있는 상자 identity, suppression·입력 owner, 내용 유효성과 재진입 guard를 확인한다.
2. 위치를 계산한다.
3. 모든 아이템을 Placement factory로 신규 설치 payload와 함께 생성하고 free-world로 활성화한다. 하나라도 실패하면 이미 만든 아이템을 모두 제거하고 실패를 반환한다. 이때 상자는 손에 남는다.
4. `Carry->CommitConsumeHeldObject(Box, …)`로 손에서 뗀다. 실패하면 3에서 만든 아이템을 모두 제거한다.
5. 상자 Actor를 제거한다. 결과적으로 빈손이다.

## Trash Bin

`ABathhouseTrashBinActor`: root `BinMesh`(static, Visibility Block). 배치·회수·carry 대상이 아니다. 개수·위치는 Level authoring이다.

| 들고 있는 것 | query | E |
|---|---|---|
| 없음 | `버릴 물건이 없습니다`, 실행 불가 | 변화 없음(SHOP-027) |
| `IPhysicalCarryDiscardable` 아님 또는 `CanDiscard` 실패 | `버릴 수 없는 물건`, 실행 불가 | 변화 없음(SHOP-026, 036) |
| 버릴 수 있음 | action `버리기 (되돌릴 수 없음)` | 버림 |

- 실행 순서: `BeginPrimaryInteraction`의 fresh trace/query 뒤 `Carry->CommitConsumeHeldObject(Object, …)`, 이어서 `Discardable->HandleDiscardCommitted()`(Actor 제거)다. 돈은 바뀌지 않는다(SHOP-024).
- 설비 아이템은 Definition의 `Facility.Discardable` 태그로만 판정한다. 판매 여부와 무관하다(SHOP-037). 회수 아이템의 보존 잔량도 함께 사라진다(SHOP-028).
- 손에서 떼면 기존 `OnHeldObjectChanged`로 배치 preview session이 정리된다.

## UI

[UISystem.md](UISystem.md)의 Native Widget Policy를 따른다.

- `UComputerScreenRootWidget`: BindWidget `ManagementTabButton`, `ShopTabButton`, `ScreenSwitcher`(UWidgetSwitcher), `ManagementScreen`(`UBathWaterManagementScreenWidget`), `ShopScreen`(`UShopScreenWidget`). 선택 탭 index는 widget 표시 상태이며 기본은 관리다. 컴퓨터 context와 사용자 변경을 두 자식에 전달한다([ComputerSystem.md](ComputerSystem.md)).
- `UShopScreenWidget`: BindWidget `ProductScroll`(UScrollBox), `ProductGrid`(UWrapBox), `BalanceText`, `CartList`(UVerticalBox), `CartQuantityText`(`7/10`), `CartTotalText`, `OrderButton`, `OrderFeedbackText`, `OrderList`(UVerticalBox). EditDefaultsOnly 행 widget class 3종.
  - 사용자 PlayerState의 wallet `OnMoneyChanged`, cart `OnCartChanged`, subsystem `OnOrdersChanged`를 구독한다. 남은 시간만 1초 간격 NativeTick으로 갱신한다.
  - 주문 버튼 활성·부족액·상한 안내는 매번 `EvaluatePlaceOrder`·`EvaluateAdd`로 계산한다(SHOP-006~008).
  - 주문 성공 시 `주문 완료`를 표시한다.
- 행 widget: `UShopProductCardWidget`(Name/Price/Icon/AddButton), `UShopCartLineWidget`(Name/Quantity/LineTotal/Plus/Minus/Remove), `UShopOrderLineWidget`(Summary/Status). 버튼은 해당 domain API만 호출한다.
- HUD: `ABathhouseHUD`가 `UMoneyHudWidget`(MoneyText, DeltaText, `DeltaDisplaySeconds=2`)과 `UShopNoticeWidget`(NoticeText)을 추가로 생성한다.
  - money widget은 PlayerController의 PlayerState wallet에 bind한다. PlayerState가 아직 없으면 possession 변경과 짧은 재시도로 bind한다. bind 시 현재 금액을 바로 표시하고, `OnMoneyChanged(Previous, Current)`의 차이를 `+10,000`/`−30,000` 형식으로 2초 표시한다. 새 변화가 오면 새 값으로 바꾸고 2초를 다시 시작한다.
  - notice widget은 `OnOrderDelivered`에 `배송 도착`을 `DeliveryNoticeSeconds` 동안 표시한다. HUD는 컴퓨터 사용 중에도 viewport에 남는다(SHOP-030).
- 금액은 천 단위 구분 + `원`으로 표시한다.

## Dependencies

- Shop → Economy wallet, Placement Definition·item factory·collision helper, Interaction interactable·carry·equipment-use·discardable 계약, DeveloperSettings.
- UI → Shop, Economy, Computer screen context interface.
- Computer는 Shop을 모른다. screen context interface로 사용자만 전달한다.
- 새 module은 없다(UMG·DeveloperSettings·GameplayTags 재사용).

## Blueprint/API Contracts

- 신규 reflected: 위 class들, `FShopProductEntry`, `FShopOrderLine`, `UShopSettings` 값, `EPhysicalCarryKind::DeliveryBox`(append), `TAG_Facility_Discardable`(native tag `Facility.Discardable`), wallet `StartingMoney`, `IPhysicalCarryDiscardable`, `IComputerScreenContextReceiver`.
- 기존 이름 rename·삭제 없음. Core Redirect 불필요.
- Editor(수직):
  - `DA_ShopCatalog`(샤워기 판매, 다른 6종은 판매 꺼짐으로 등록 가능).
  - Project Settings에 catalog·상자 class 지정.
  - 7종 설비 정의에 `Facility.Discardable` 태그.
  - WBP: root·shop·행 3종·money·notice.
  - `BP_ShopDeliveryBox`, `BP_ShopDeliveryPoint`, `BP_TrashBin`.
  - 컴퓨터 `ScreenWidget.WidgetClass`를 root WBP로 교체.
  - HUD Blueprint widget class 지정.
  - DefaultMap에 배송 지점 1, 쓰레기통 1 이상.

## Verification

| 시나리오 | 자동화 |
|---|---|
| SHOP-001, 029 | wallet 시작 금액(InitializeComponent, 방송 없음), 손님 현금 +10,000 delegate |
| SHOP-004~008 | cart 담기·±·삭제, 상품별 99·전체 10, 잔액 무관, 부족액 평가와 잔액 변화 후 재평가 |
| SHOP-009, 010 | 주문 원자성, 차감 1회·주문 1개·cart 비움, 두 번째 호출 실패 |
| SHOP-011~015, 030 | 게임시간 딜레이 10·0초, FIFO, 대기와 공간 회복 후 도착, 쌓임 높이, 도착 이벤트 |
| SHOP-016~021 | 상자 E·요약·drop 보존, 개봉 transaction, 트인 곳·벽 앞·구석 배치, 실패 rollback, 컴퓨터·배치 LMB 우선 |
| SHOP-022, 028 | 신규 설치 payload로 샤워기 배치 시 class 기본값, 회수 아이템 버리기 |
| SHOP-023~027, 036, 037 | 쓰레기통 판정 표, 락커·열쇠·도구·빈손 거부, 7종 태그 기준 |
| catalog | Data Validation: 중복 id, 가격, 락커·태그 없는 설비 판매 금지 |

PIE: 탭·상점·HUD 배치(SHOP-002, 003, 031), 실제 입력·물리 낙하 모습.
