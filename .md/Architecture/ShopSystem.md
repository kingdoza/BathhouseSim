# Shop System

## Status And Scope

- 수직 단계(샤워기): 2026-09-27 설계, 2026-09-28 구현 완료(`94f0f11`). 계약 원문은 그 커밋의 `.md/PROMPT_ARCHITECTURE.md`(SHOP-001~037), 선택은 `.md/QNA_FEATURE_SPEC.md` Q1~Q25다.
- 확장 단계: 2026-09-28 설계·구현, 같은 날 사용자 통합 승인. 입력은 현재 `.md/PROMPT_ARCHITECTURE.md`(Q26~Q30, SHOP-018~020·032~035·038~045)와 사용자 지시 "배송 상자 held transform·scale 정책을 기존 held 물품과 통일"이다. 범위는 7종 판매, 개봉 물품 흩어짐, 배송 상자 scale 정책이다.
- 2026-09-28 코드 리뷰 재검토: 환경 여유를 아래 방향에 적용하지 않도록 고쳤다(World Placement). `Dc`가 바닥 여유(`UnboxForwardFloorClearanceCm`) 이상이면 부푼 밑면이 바닥에 닿아 트인 바닥에서도 정면 단계가 전부 실패하던 규칙 충돌을 없앤다.
- 2026-10-01 UNBOX-SPAWN-VIEW 설계: 개봉 1단계를 카메라 시선 앞(pitch 포함)으로 바꾸고 기존 정면 규칙을 2단계(바닥 정면)로 남긴다. 입력은 `.md/Work/UNBOX-SPAWN-VIEW/PROMPT_ARCHITECTURE.md`(USV-001~021)다.
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
  ShopUnboxingCluster.h/.cpp   개봉 무리 모양 순수 계산(확장)
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

2026-09-30 서비스 1단위: 상품은 설비(`PlacementDefinition`) 또는 품목 박스(`ItemBoxDefinition`) 중 정확히 하나를 가진다. 박스 상품, `ItemBoxClass` 설정, 개봉 시 박스 생성과 항목별 shape 무리 배치는 [ServiceSystem.md](ServiceSystem.md) Shop Integration이 정본이다. 아래 설비 규칙(`Facility.Discardable` 등)은 설비 상품에만 적용한다.

`UShopCatalog : UPrimaryDataAsset`의 `Products : TArray<FShopProductEntry>`가 상품 정의의 유일 정본이다. 배열 순서가 화면 카드 순서다.

| 필드 | 규칙 |
|---|---|
| `ProductId` (FName) | 고유, None 금지 |
| `bForSale` | 화면 표시·담기 허용 여부. 확장 단계는 7종 모두 true |
| `DisplayName` (FText) | 카드·장바구니·요약 문구 |
| `Price` (int32) | > 0 |
| `Icon` (`TSoftObjectPtr<UTexture2D>`) | 선택. 없으면 이름 표시로 대체 |
| `PlacementDefinition` | 필수. placement 활성. `LockerSlotCount == 0`이면 `Facility.Discardable` 태그 보유, `> 0`(락커)이면 그 태그 없음(EXP-U2) |

- Data Validation은 위 규칙을 검사한다(판정 한 곳 `FShopProductRules::ValidateDefinitions`). 락커가 아닌 "팔지만 버릴 수 없는 설비"를 데이터 단계에서 막는다. 락커 상품은 2026-10-02 `EXP-U2`부터 허용하며 버릴 수 없다. 한도 초과 구매는 막지 않고 설치만 기존 락커 한도가 막는다([ExpansionPurchaseSystem.md](ExpansionPurchaseSystem.md) Locker Products).
- 설비 정의에 가격을 넣지 않는다. 가격 편집 위치는 catalog 하나다.

`UShopSettings : UDeveloperSettings`(Config=Game, Project Settings 노출).

값의 정본: 기본값은 `Public/Shop/ShopSettings.h`의 생성자 초기값(getter 비유한 fallback과 같은 `Default*` 상수), 저장된 조정값은 `Config/DefaultGame.ini` `[/Script/BathhouseSim.ShopSettings]`, 유효 범위는 각 UPROPERTY `ClampMin`/`ClampMax`와 getter다. 이 문서는 값을 적지 않는다(2026-10-01 사용자 지시).

| 값 | 의미 |
|---|---|
| `Catalog` (soft) | Editor 지정, 필수 |
| `DeliveryBoxClass` (soft class) | Editor 지정, `AShopDeliveryBoxActor` 자식 |
| `CartTotalQuantityLimit`, `PerProductQuantityLimit` | 장바구니 전체·상품별 상한 |
| `DeliveryDelaySeconds` | 주문 → 배송 게임시간 |
| `DeliveryNoticeSeconds` | `배송 도착` 표시 시간 |
| `DeliveryAttemptIntervalSeconds` | 배송 대기 주문의 도착 재시도 간격(0이면 매 Tick) |
| `UnboxOverlapDepthCm` | 개봉 물품끼리 목표 겹침 깊이 D. 클수록 세게 튄다(SHOP-043). 여유 shape `Dc`도 이 getter 값이다 |
| `UnboxCluster*` | 무리 모양(Cluster Layout): 물품당 배치 시도 수, 고도각 하한·상한, 다른 쌍 깊이 허용 오차, 쌍 목표 깊이의 half extent 비율 |
| `UnboxViewDistanceCm`, `UnboxViewMinDistanceCm`, `UnboxViewPullStepCm`, `UnboxViewLayoutAttempts` | 1단계 시선 앞: 카메라 → 무리의 가장 가까운 부분(시선 방향) 시작 거리·하한·당김 간격, 거리당 layout 시도 수 |
| `UnboxForwardDistanceCm`, `UnboxMinForwardDistanceCm`, `UnboxForwardPullStepCm`, `UnboxForwardLayoutAttempts`, `UnboxForwardFloorClearanceCm` | 2단계 바닥 정면: 발바닥 기준 수평 중심 거리·하한·당김 간격, 거리당 시도 수, 최저 바닥면의 발바닥 위 높이(`UnboxForwardDistanceCm` 의미 유지) |
| `UnboxCameraClearanceCm` | P10 카메라 여유(바닥 정면 push 기준) |
| `UnboxOverheadClearanceCm`, `UnboxOverheadStepCm`, `UnboxOverheadStepCount`, `UnboxOverheadLayoutAttempts` | 3단계 머리 위: capsule 윗면 위 시작 여유, 단 간격, 단 수, 단당 시도 수 |

개봉 코드는 이 값들을 private `FShopUnboxingTuning::FromSettings(const UShopSettings&)` 한 곳에서 읽어 계산 helper에 인자로 넘긴다. 자동화도 같은 함수로 기대값을 계산한다.

getter의 비유한 fallback과 member 초기값은 header의 `UShopSettings::Default*` 상수 한 곳을 같이 읽는다.

시작 금액은 wallet 소유, 잔액 변화량 표시 시간은 HUD widget(`UMoneyHudWidget::DeltaDisplaySeconds`) 소유다.

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

subsystem Tick(고정 간격 throttle. 간격은 `UShopSettings::DeliveryAttemptIntervalSeconds`):

1. FIFO 머리 주문만 본다. 머리가 준비되지 않았거나 공간이 없으면 뒤 주문도 처리하지 않는다(SHOP-015).
2. 머리가 준비되면 등록된 배송 지점의 `FindDropTransform(BoxHalfExtent)`로 위치를 구한다.
3. 성공: 상자를 deferred spawn → `InitializeContents(OrderId, Lines)` → `FinishSpawning` → free-world 물리 활성. 상자 world scale은 class CDO root scale이다(Delivery Box 절). 주문을 제거하고 `OnOrderDelivered(OrderId)`, `OnOrdersChanged`를 방송한다. 이어서 다음 머리를 같은 Tick에 시도한다.
4. 실패: `bWaitingForSpace=true`로 표시하고 멈춘다(SHOP-014). 다음 throttle에 다시 시도한다.

- 배송 지점이 없거나 둘 이상이면 경고 로그를 남긴다. 없으면 모든 준비된 주문이 대기이고, 둘 이상이면 먼저 등록된 지점을 쓴다.
- 상자 class가 없거나 spawn이 실패하면 대기로 두고 오류 로그를 남긴다. 돈은 이미 차감됐으므로 주문을 버리지 않는다.
- UI 조회 `GetOrderSnapshots()`: 요약 줄, 남은 초(`max(0, Ready - Now)`), 대기 여부.

## Delivery Point

`AShopDeliveryPointActor`: root `USceneComponent`, editor-only billboard·arrow(표시 전용). `MaxSearchHeightCm`(EditAnywhere), `DropGapCm`(EditDefaultsOnly). BeginPlay에 subsystem에 등록, EndPlay에 해제한다.

값의 정본: 기본값은 `Public/Shop/ShopDeliveryPointActor.h` 초기값, 실제 값은 `/Game/Bathhouse/Blueprints/Shop/BP_ShopDeliveryPoint` Class Defaults(`MaxSearchHeightCm`은 레벨 instance override 가능), 유효 범위는 `ClampMin`과 `FindDropTransform` 입력 검사(비유한·`MaxSearchHeightCm ≤ 0`·`DropGapCm < 0`이면 공간 없음)다.

`FindDropTransform(BoxHalfExtent, BoxCollisionTemplate)`:

1. 지점 위치 B에서 위로 `MaxSearchHeightCm`까지 WorldStatic object type만 line trace해 천장 높이 Zc를 구한다. 상자·설비 아이템·Pawn은 천장으로 보지 않는다. 없으면 `Zc = B.Z + MaxSearchHeightCm`.
2. `(B.XY, Zc - HalfZ - ε)`(ε는 천장 접촉 회피용 고정 epsilon, 조정값 아님)에서 `(B.XY, B.Z + HalfZ)`까지 상자 collision으로 box sweep한다. 시작이 이미 막혀 있으면 공간 없음이다.
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

### Held Transform And Scale

2026-09-28 사용자 지시로 기존 held 물품(수건 바구니·걸레·렌치·열쇠·삽)과 설비 아이템의 규칙에 맞춘다.

- `HeldTransform`(EditDefaultsOnly, BlueprintReadOnly, Category `Carry|Presentation`, 기본 Identity)을 상자 class가 소유한다. `GetHeldTransform()`은 이 값의 scale을 `OneVector`로 정규화해 반환하고, 잡을 때 location/rotation만 root에 적용한다. `UFacilityPlacementSettings::FacilityItemHeldTransform`은 설비 아이템 전용이며 상자는 더 이상 읽지 않는다.
- 상자 크기의 정본은 Blueprint CDO `BoxMesh` relative scale이다(설비 아이템 `ItemRoot` scale 정본과 같은 방식). 각 축이 finite > 0이면 non-uniform도 허용한다.
- 모든 전이가 scale을 보존한다.
  - 배송 spawn은 unit scale drop transform으로 `FinishSpawning`하고 엔진 기본 root 합성으로 CDO scale을 얻는다.
  - `ActivateFreeWorld`와 last-safe 복구는 location/rotation만 적용한다. 현재 `SetActorTransform`이 scale을 1로 덮는 결함을 고친다.
  - 잡기는 `SnapToTargetNotIncludingScale`, 놓기는 PhysicalCarry 공통 규칙(기존 Root scale 보존)이다.
- `GetBoxHalfExtent()` = mesh bounds extent × |CDO root relative scale|로 한 번만 곱한다. root가 곧 Actor root라 현재의 root scale × Actor scale은 scale²가 되므로 제거한다. 배송 지점 sweep·overlap은 이 값을 쓴다.
- Data Validation
  - 오류: mesh 없음, root collision이 QueryAndPhysics 아님, CCD 꺼짐, Pawn 응답이 Ignore 아님, root relative location/rotation이 0 아님, root scale 축이 non-finite이거나 `KINDA_SMALL_NUMBER` 이하.
  - 경고: `HeldTransform` scale ≠ 1. 문구는 다른 held 물품과 같은 "HeldTransform scale is ignored at runtime. Author a unit scale and use location/rotation only."
  - Actor·root scale = 1 요구는 제거한다.

## Unboxing

개봉 물품은 카메라 시선 앞에 무작위로 모인 3차원 무리로 생긴다. 물품끼리만 일부 겹쳐 생성돼 엔진 충돌 해소로 튄다(Q26~Q30). 추가 속도와 튐 상한은 없다.

### Cluster Layout

`FShopUnboxingCluster`(Private, 순수 계산, world·UObject 접근 없음)가 무리 모양을 만든다.

- 입력: 물품별 box half extent(Definition collision query와 같은 mesh bounds × CDO scale), 목표 깊이 D, 무리 조정값(`UnboxCluster*`), `FRandomStream&`.
- 모든 물품은 선 자세이고 yaw만 전체 원 범위에서 무작위다(Q27). 두 물품의 겹침은 두 yaw의 XY face normal 4축과 Z축의 5축 SAT로 정확히 판정한다. 쌍의 깊이 = 5축 overlap의 최솟값(음수면 분리)이며, 엔진이 풀어야 하는 최소 분리 거리다.
- 쌍별 목표 깊이 `Dij = min(D, UnboxClusterPairDepthExtentRatio × 두 물품 half extent 중 최솟값)`. 작은 물품이 큰 물품 안에 묻히지 않게 한다.
- 생성 순서: 물품 목록을 stream으로 섞는다. 첫 물품은 무리 원점이다. 이후 물품 i마다 최대 `UnboxClusterPlacementAttempts`회 다음을 시도한다.
  1. 놓인 물품 중 anchor j를 무작위로 고른다.
  2. 방향 u: 수평각 전체 원, 고도각 크기 `[UnboxClusterMinElevationDegrees, UnboxClusterMaxElevationDegrees]`, 부호(위·아래) 무작위.
  3. 축 a마다 `k_a = |u·n_a|`, `R_a` = 두 물품 projected half extent 합일 때 `t = min_{k_a>0} (R_a − Dij) / k_a`. i 중심을 j 중심 + u·t에 두면 i–j 깊이가 정확히 Dij다.
  4. 다른 모든 놓인 물품 k와의 깊이가 `Dik + UnboxClusterDepthToleranceCm` 이하이면 채택한다.
  - 시도를 모두 실패하면 그 layout 시도 전체가 실패다.
- 고도각 하한 때문에 같은 물품도 서로 다른 높이에 생긴다(SHOP-018). 무리 크기는 물품 수·크기만큼 자라며 고정 반경이 없다(SHOP-039, 040).
- 출력: 무리 local 좌표의 shape 중심과 yaw. 호출자가 XY bounds 중심과 가장 낮은 바닥면을 단계 기준에 맞춰 평행 이동한다.
- D가 범위 하한 0이면 물품이 맞닿기만 하고 튀지 않는다. 조정 끝점으로 허용한다.

### World Placement

`FShopUnboxingPlacement::FindSpawnTransforms(World, Player, PlayerCapsule, Box, Items, Request, Tuning, RandomStream, OutTransforms, OutFailureReason, OutStage)`. `FShopUnboxingPlacementRequest`는 입력 순간의 카메라 위치·시선 방향(pitch 포함)과 발바닥이고, `FShopUnboxingTuning`은 위 Settings 값 전체다. `EShopUnboxPlacementStage`(ViewFront/FloorFront/Overhead/FinalStack)는 자동화가 단계를 단언하는 결과다. 물품 shape는 기존 `APlaceableFacilityItemActor::BuildDefinitionCollisionQuery`(CDO scale 포함)와 같고, Actor 위치 = shape 중심 − 회전 × (scale × bounds origin)이다.

물품마다 다음 world 검사를 한다. 하나라도 실패하면 그 layout을 버린다.

- 여유 shape: `Dc = UnboxOverlapDepthCm` getter 값(범위 clamp는 getter 한 곳). 수평과 위쪽으로만 부풀리고 밑면은 원래 위치에 둔다. 물품은 선 자세(yaw만)이므로 world Z 기준으로 half extent = `(Ex + Dc, Ey + Dc, Ez + Dc/2)`, 중심 = 원래 중심 + `(0, 0, Dc/2)`다(1/2은 밑면 고정 기하).
  - 수평·위 여유는 겹침이 풀리며 밀려나도 벽·천장에 박혀 시작하지 않게 한다.
  - 아래 여유는 두지 않는다(모든 단계). 바닥 정면은 `UnboxForwardFloorClearanceCm`, 위 단계는 `UnboxOverheadClearanceCm`만큼 떨어져 시작하고, 시선 앞은 원래 밑면의 바닥 overlap을 기각한다. 아래로 밀리면 바닥 collision과 CCD가 받는다. 아래 여유가 바닥 기준 간격 이상이면 트인 바닥에서도 모든 정면 layout이 기각된다(2026-09-28 재검토). 이 때문에 아래 여유를 다시 넣지 않는다.
- 환경: 여유 shape로 `FacilityPlacementCollision::HasBlockingOverlap`. player·상자는 무시한다. 원래 extent의 밑면이 바닥에 닿거나 파고들면 기각된다.
- 손님: 같은 여유 shape로 `ECC_Pawn` object type overlap. player만 무시한다. item collision이 Pawn을 무시하므로 별도로 검사한다(SHOP-041).
- 시야: 단계 기준점에서 shape 중심까지 Visibility line trace가 막히지 않는다(player·상자 무시). 벽 너머 생성을 막는다(SHOP-019, 042).
- player를 무시하므로 발밑을 보면 플레이어 몸 자리와 겹치는 자리도 쓴다(USV P9). 같은 무리의 물품끼리는 검사하지 않는다.

단계(1~3단계 모두 같은 여유 shape를 쓴다). 거리열은 `PlayerViewFrontPlacement::BuildPullDistances(시작, 하한, 간격)`(시작에서 간격씩 줄이고 마지막 하한)다. 기하 helper는 [InteractionSystem.md](InteractionSystem.md) Player View-Front Spawn Geometry.

1. 시선 앞: d = `UnboxViewDistanceCm` → `UnboxViewMinDistanceCm`(간격 `UnboxViewPullStepCm`). d마다 새 layout을 최대 `UnboxViewLayoutAttempts`회 만든다. 무리 translation은 `ComputeViewFrontTranslation(layout, 카메라 위치, 시선 방향, d)`이다. 시선 방향 투영 최솟값이 d이고 카메라 right·up 투영 범위 중앙이 시선 ray라 카메라를 감싸지 않는다. 시야 기준점은 카메라 위치다(USV-001~006, 008, 012).
2. 바닥 정면: 기존 정면 규칙이다. d = `UnboxForwardDistanceCm` → `UnboxMinForwardDistanceCm`(간격 `UnboxForwardPullStepCm`), d마다 layout 최대 `UnboxForwardLayoutAttempts`회, XY 중심은 `발바닥 + 수평 전방 × d`, 가장 낮은 바닥면은 발바닥 + `UnboxForwardFloorClearanceCm`, 시야 기준점은 capsule 중심이다(SHOP-018, 019, 033, 038~041). 수평 전방은 시선 방향의 수평 성분이며 0이면 이 단계를 건너뛴다. translation 뒤 `GetCameraClearancePushCm(…, UnboxCameraClearanceCm)`만큼 수평 전방으로 민다. 무리 윗면이 카메라보다 여유 이상 아래면 0이라 작은 무리는 현재 위치 그대로다. 높은 무리는 카메라 앞 여유 밖으로 밀려 카메라를 감싸지 않는다(P10, USV-004, 007, 013).
3. 위로 쌓기: XY 중심은 player XY, 가장 낮은 바닥면은 capsule 윗면 + `UnboxOverheadClearanceCm` + `UnboxOverheadStepCm` × k(k < `UnboxOverheadStepCount`). k마다 layout 최대 `UnboxOverheadLayoutAttempts`회, 시야 기준점은 capsule 윗면 중심이다. 겹침·튐을 허용한다(SHOP-020).
4. 최후: 기존 결정적 수직 쌓기(겹침 없음, player 위치, 경고 로그)를 유지한다. 정상 공간에서는 도달하지 않으며 개봉 항상 성공을 보장한다. 이 단계만 카메라를 감쌀 수 있다.

worst case query 수는 (단계별 거리·단 수 × 시도 수의 합) × 물품 수 × 3이다. 기본값에서 클릭 한 번의 동기 처리로 허용한다. 시도 수를 크게 올리면 이 비용이 비례해 늘어난다.

### Escape Guard

밀려나는 속도는 엔진 결과 그대로다(Q29 B). 이탈 방지는 생성 조건과 기존 물리 계약으로 보장한다.

- 수평·위 여유를 둔 환경 검사와 시야 검사로 벽·천장 안이나 너머에 생기지 않는다.
- 설비 아이템 root는 항상 CCD다([PhysicalCarrySystem.md](PhysicalCarrySystem.md)). 빠르게 튀어도 벽·천장을 통과하지 않는다.
- 맵 아래로 빠지면 기존 `FellOutOfWorld` → last-safe transform(= 생성 transform) 복구가 동작한다.
- Pawn Ignore라 player와 손님을 밀지 않는다.
- Project·Body의 depenetration 설정은 바꾸지 않는다. 검증에서 겹침이 튐으로 나타나지 않으면 설정을 바꾸지 않고 멈춰 보고한다. Q29 B와 충돌하므로 기능 명세로 복귀한다.

### Transaction

`FShopUnboxingTransaction::Open(Box, Context)`:

1. 들고 있는 상자 identity, suppression·입력 owner, 내용 유효성과 재진입 guard를 확인한다.
2. `FMath::Rand()`로 seed한 `FRandomStream`과 request·tuning으로 위치를 계산한다. request의 카메라 위치·방향은 장비 context(`CameraOrigin`, `CameraDirection`), tuning은 `FShopUnboxingTuning::FromSettings`다. 자동화는 placement helper에 seed를 직접 준다.
3. 모든 아이템을 Placement factory로 신규 설치 payload와 함께 생성하고 free-world로 활성화한다. 같은 호출 안에서 활성화하므로 같은 physics step에서 겹침을 푼다. 추가 속도는 주지 않는다(Q28). 하나라도 실패하면 이미 만든 아이템을 모두 제거하고 실패를 반환하며, 상자는 손에 남는다.
4. `Carry->CommitConsumeHeldObject(Box, …)`로 손에서 뗀다. 실패하면 3에서 만든 아이템을 모두 제거한다.
5. 상자 Actor를 제거한다. 결과적으로 빈손이다.

## Trash Bin

2026-10-01 서비스 3단위 설계(Q39 A): 쓰레기통은 레벨에서 제거하고 코드는 그대로 둔다. 버리기 수단은 쓰레기 수거 구역이 되고, 같은 종류 판정을 world 버리기로 공유한다([CleaningLitterSystem.md](CleaningLitterSystem.md) World Discard And Collection). 아래는 코드에 남는 held 경로다.

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

- `UComputerScreenRootWidget`: BindWidget `ManagementTabButton`, `ShopTabButton`, `ScreenSwitcher`(UWidgetSwitcher), `ManagementScreen`(`UBathWaterManagementScreenWidget`), `ShopScreen`(`UShopScreenWidget`). 선택 탭은 widget 표시 상태이며 기본은 관리다. 컴퓨터 context와 사용자 변경을 자식 화면에 전달한다([ComputerSystem.md](ComputerSystem.md)). 2026-10-02 `EXP-U2` 설계로 세 번째 탭 `확장`(optional `ExpansionTabButton`·`ExpansionScreen`)이 붙는다([ExpansionPurchaseSystem.md](ExpansionPurchaseSystem.md)).
- `UShopScreenWidget`: BindWidget `ProductScroll`(UScrollBox), `ProductGrid`(UWrapBox), `BalanceText`, `CartList`(UVerticalBox), `CartQuantityText`(`전체 수량/CartTotalQuantityLimit`), `CartTotalText`, `OrderButton`, `OrderFeedbackText`, `OrderList`(UVerticalBox). EditDefaultsOnly 행 widget class 3종.
  - 사용자 PlayerState의 wallet `OnMoneyChanged`, cart `OnCartChanged`, subsystem `OnOrdersChanged`를 구독한다. 남은 시간만 고정 간격 NativeTick으로 갱신한다. 간격은 `UShopScreenWidget::CountdownRefreshIntervalSeconds`다.
  - 주문 버튼 활성·부족액·상한 안내는 매번 `EvaluatePlaceOrder`·`EvaluateAdd`로 계산한다(SHOP-006~008).
  - 주문 성공 시 `주문 완료`를 표시한다.
  - 상품이 넘치면 `ProductScroll`만 세로로 스크롤한다. cart·주문 panel은 `ProductScroll` 밖 형제로 둬 항상 보인다(SHOP-032, WBP layout).
  - `ProductScroll`·`CartScroll`·`OrderScroll`의 마우스 휠 스크롤은 Shop 코드 없이 Computer 휠 주입이 처리한다([ComputerSystem.md](ComputerSystem.md) Screen Wheel Scroll).
- 행 widget: `UShopProductCardWidget`(Name/Price/Icon/AddButton), `UShopCartLineWidget`(Name/Quantity/LineTotal/Plus/Minus/Remove), `UShopOrderLineWidget`(Summary/Status). 버튼은 해당 domain API만 호출한다.
- HUD: `ABathhouseHUD`가 `UMoneyHudWidget`(MoneyText, DeltaText, `DeltaDisplaySeconds`)과 `UShopNoticeWidget`(NoticeText)을 추가로 생성한다.
  - `DeltaDisplaySeconds`(EditDefaultsOnly)의 기본값은 `Public/UI/MoneyHudWidget.h` 초기값, 실제 값은 `/Game/Bathhouse/UI/WBP_MoneyHud` Class Defaults, 범위는 `ClampMin`이다.
  - money widget은 PlayerController의 PlayerState wallet에 bind한다. PlayerState가 아직 없으면 possession 변경과 짧은 재시도로 bind한다. bind 시 현재 금액을 바로 표시하고, `OnMoneyChanged(Previous, Current)`의 차이를 `+10,000`/`−30,000` 형식(형식 예시)으로 `DeltaDisplaySeconds` 동안 표시한다. 새 변화가 오면 새 값으로 바꾸고 표시 시간을 다시 시작한다.
  - notice widget은 `OnOrderDelivered`에 `배송 도착`을 `DeliveryNoticeSeconds` 동안 표시한다. HUD는 컴퓨터 사용 중에도 viewport에 남는다(SHOP-030).
- 금액은 천 단위 구분 + `원`으로 표시한다.

## Dependencies

- Shop → Economy wallet, Placement Definition·item factory·collision helper, Interaction interactable·carry·equipment-use·discardable 계약, DeveloperSettings.
- UI → Shop, Economy, Computer screen context interface.
- Computer는 Shop을 모른다. screen context interface로 사용자만 전달한다.
- 새 module은 없다(UMG·DeveloperSettings·GameplayTags 재사용).

## Blueprint/API Contracts

- 수직 신규 reflected: 위 class들, `FShopProductEntry`, `FShopOrderLine`, `UShopSettings` 값, `EPhysicalCarryKind::DeliveryBox`(append), `TAG_Facility_Discardable`(native tag `Facility.Discardable`), wallet `StartingMoney`, `IPhysicalCarryDiscardable`, `IComputerScreenContextReceiver`.
- 확장 신규 reflected: `UShopSettings::UnboxOverlapDepthCm`, `AShopDeliveryBoxActor::HeldTransform`. property 추가만이므로 기존 export와 호환된다.
- UNBOX-SPAWN-VIEW 신규 reflected: Catalog And Settings 표의 `UnboxView*`, `UnboxMinForwardDistanceCm`, `UnboxForwardPullStepCm`, `UnboxForwardLayoutAttempts`, `UnboxForwardFloorClearanceCm`, `UnboxCameraClearanceCm`, `UnboxOverhead*`, `UnboxCluster*`. 추가만이며 C++ 기본값이 기존 코드 상수와 같아 Config 키·Content 변경이 없다.
- 기존 이름 rename·삭제 없음. Core Redirect 불필요.
- Editor(수직, 완료): catalog·Project Settings·7종 `Facility.Discardable` 태그·WBP·Shop Actor Blueprint·DefaultMap 배송 지점과 쓰레기통.
- Editor(확장):
  - `DA_ShopCatalog`에 6종 entry 추가: `ProductId` Bath·Washer·Dryer·Boiler·Cooler·Circulator, `bForSale=true`, 표시 이름, 가격, 아이콘(선택). Definition은 기존 7종 `DA_FacilityPlacement_*`.
  - `BP_ShopDeliveryBox`: `HeldTransform`을 authoring한다. 시작값은 당시 `UFacilityPlacementSettings::FacilityItemHeldTransform` 값이다. root scale은 원하는 크기로 둔다. 현재 값의 정본은 `BP_ShopDeliveryBox` Class Defaults다.
  - Project Settings `UnboxOverlapDepthCm`: PIE에서 "약간 튐"으로 정한다. 욕탕·샤워기 혼합 상자로 확인한다.
  - 상점 WBP: cart panel이 `ProductScroll` 밖인지 확인한다.

## Verification

| 시나리오 | 자동화 |
|---|---|
| SHOP-001, 029 | wallet 시작 금액(`UPlayerWalletComponent::StartingMoney`, InitializeComponent, 방송 없음), 손님 현금 +`ABathhouseCashPaymentActor::PaymentAmount` delegate |
| SHOP-004~008 | cart 담기·±·삭제, 테스트가 Settings에 설정한 상품별·전체 상한(fixture, 저장·복원), 잔액 무관, 부족액 평가와 잔액 변화 후 재평가 |
| SHOP-009, 010 | 주문 원자성, 차감 1회·주문 1개·cart 비움, 두 번째 호출 실패 |
| SHOP-011~015, 030 | 게임시간 딜레이(fixture 양수 값과 0), FIFO, 대기와 공간 회복 후 도착, 쌓임 높이, 도착 이벤트 |
| SHOP-016, 017, 021 | 상자 E·요약·drop 보존, 실패 rollback, 컴퓨터·배치 LMB 우선 |
| 상자 scale | fixture class `AShopDeliveryBoxScaleAutomationActor`의 비단위 CDO root scale s: half extent s배, 배송 spawn·drop·집기·last-safe 복구 뒤 world scale s 유지, validation 오류·경고 조건 |
| 무리 계산 | anchor 깊이 = Dij, 다른 쌍 ≤ Dik + `UnboxClusterDepthToleranceCm`, 고도각이 설정 범위 안, yaw만 회전, 같은 seed 동일·다른 seed 다름(SHOP-038), 1개(SHOP-039), 혼합 크기 최대 수량(SHOP-040), D 하한 접촉. 기대값은 `FShopUnboxingTuning::FromSettings`로 계산 |
| SHOP-018~020, 041 | 바닥 정면 단계 강제 시 d = `UnboxForwardDistanceCm`·최저 바닥면 = 발바닥 + `UnboxForwardFloorClearanceCm`, 앞 벽 당김·벽 너머 없음, 구석 위로 쌓기, 손님 capsule 회피, 환경 여유(수평·위만), 항상 개봉 |
| USV-001~021 | 고정 seed: 시선 앞 가장 가까운 부분 = d·가운데 정렬·카메라 비포함, pitch −80~+45, 천장·벽 당김과 단계 전환, P10 분리, 봉투 경로([CleaningLitterSystem.md](CleaningLitterSystem.md)) |
| 여유 방향 | 시선 단계를 막은 트인 바닥에서 `UnboxOverlapDepthCm` 범위 양끝과 중간 여러 값 모두 바닥 정면 단계 채택·최저 바닥면 = 발바닥 + `UnboxForwardFloorClearanceCm`. 수평 `Dc` 안의 벽과 위 `Dc` 안의 천장은 기각, 발바닥 높이 바닥은 기각하지 않음 |
| 튐 물리 | test world tick: 겹친 두 물품이 0.5초 안에 분리, 깊이 2 대 20의 최고 속도 비교(SHOP-043), 닫힌 방 10개 개봉 3초 뒤 모두 방 안·바닥 위(SHOP-042, 045) |
| 7종 설비 | 실제 7종 Definition의 collision query·`SpawnFreshItem`·free-world 활성 성공(SHOP-035, 045 사전). 7종 판매 catalog 단언은 Editor authoring 뒤 확인 |
| SHOP-022, 028 | 신규 설치 payload 배치 시 class 기본값, 보일러 잔량 0(SHOP-034), 회수 아이템 버리기 |
| SHOP-023~027, 036, 037 | 쓰레기통 판정 표, 락커·열쇠·도구·빈손 거부, 7종 태그 기준 |
| catalog | Data Validation: 중복 id, 가격, 태그 없는 비락커 설비 판매 금지, 락커 상품 허용·태그 있는 락커 금지(EXP-U2) |

PIE: 탭·상점·HUD 배치(SHOP-002, 003, 031), 목록 스크롤(SHOP-032), 실제 튐 모습과 세기(SHOP-018, 043), 7종 배치(SHOP-034, 035, 044).
