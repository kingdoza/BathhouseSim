# REPORT_UNREAL_DISCOVERY — EXPANSION-PURCHASE
- 작업 ID: `EXPANSION-PURCHASE`
- 단계: Editor 사전 조사
- 상태: 완료

(Editor 워커 전문을 기능 명세 일반 세션이 저장 — 하위 에이전트 보고서 파일 쓰기가 하네스에 막힘, 회고 대상)

## 범위와 방법

- 모드: 읽기 전용 조사. modify·Compile·Save·asset 생성·삭제 없음. PIE 미실행.
- 실행 경로: 작업용 숨김 Editor(UE 5.8, `-ModelContextProtocolStartServer -NoSplash -log -ExecCmds="py ..."`)를 띄워 공식 Editor Python API로 조회한 뒤 `QUIT_EDITOR`로 종료. MCP 도구 호출은 쓰지 않았다(Python으로 충분).
  - 실행 3회(PID 22756, 6156, 19092). 1·2회는 속성 이름 오류(`b_for_sale`→`for_sale`, `b_auto_start`→`auto_start`) 정정용이며 최종 값은 3회차 기준.
  - 세 PID 모두 `LogExit: Exiting.`로 정상 종료, 잔여 `UnrealEditor` 프로세스 없음.
- 스크립트: `Saved/Claude/ExpansionDiscovery/exp_01_probe.py`
- 결과: `Saved/Claude/ExpansionDiscovery/probe.json`(최종), 요약 `comp_summary.txt`, 로그 `editor.log`(이전 회차 `*_run1/_run2`)
- 보조 근거: `Content/` 바이너리 문자열 grep(읽기만), Source 헤더·cpp 읽기.
- 기준선: 시작 시 사용자 Editor 없음. `git status`에서 `Content/` 변경 없음(작업 전후 동일). Editor dirty package는 조회 전후 모두 0개.
- 기본 Level: `Config/DefaultEngine.ini`의 `EditorStartupMap=/Game/Maps/DefaultMap`. 참고로 `GameDefaultMap=/Engine/Maps/Templates/OpenWorld`이다. 이 보고의 Level 사실은 DefaultMap 기준이다.
- World Partition: DefaultMap의 actor descriptor 199개가 모두 로드됐다(로드된 actor 199개). 미로드 actor는 없다.

## 1. 확장 정의 `DA_BathhouseExpansion_Default`

- Asset: `/Game/Bathhouse/Data/Expansion/DA_BathhouseExpansion_Default` (class `/Script/BathhouseSim.BathhouseExpansionDefinition`)
- `Tiers`: 3개

| index | KeyPoolSize | MaxInstalledLockerSlots |
|---|---|---|
| 0 | 3 | 2 |
| 1 | 4 | 4 |
| 2 | 8 | 8 |

- 다른 `UBathhouseExpansionDefinition` asset: 없음. 근거: AssetRegistry의 `/Game` 전체 class 검색(하위 class 포함) 결과가 이 asset 하나다. 바이너리 grep 결과도 같다.

## 2. `BP_BathhouseExpansionAuthority`

- CDO(`/Game/Bathhouse/Blueprints/Placement/BP_BathhouseExpansionAuthority`): `ExpansionDefinition=DA_BathhouseExpansion_Default`, `InitialTierIndex=0`
- DefaultMap instance: 1개
  - `BP_BathhouseExpansionAuthority_C_UAID_F02F7433CA36D1FF02_1155526560`(label `ExpansionAuthority`)
  - external package `/Game/__ExternalActors__/Maps/DefaultMap/1/T8/BSY81Z2446YMM4DNR4DC1B`, 위치 (0,0,0)
  - instance 값은 `ExpansionDefinition=DA_BathhouseExpansion_Default`, `InitialTierIndex=0`으로 CDO와 같다. override 값 차이 없음.
- 따라서 시작 tier는 0이다(KeyPoolSize 3, 락커 칸 상한 2).

## 3. `BP_BathhouseKeyRack`

- CDO(`/Game/Bathhouse/Blueprints/Interaction/BP_BathhouseKeyRack`):
  - `PairTransforms` 8개(local). 위 줄 Z=0에 Y=0, -120, -260, -380. 아래 줄 Z=120에 같은 Y 4개. 회전 0, scale 1.
  - `FirstKeyNumber=1`
  - `KeyClass=/Game/Bathhouse/Blueprints/Interaction/BP_BathhouseKey.BP_BathhouseKey_C`
  - `HookClass=/Game/Bathhouse/Blueprints/Interaction/BP_BathhouseKeyHook.BP_BathhouseKeyHook_C`
- DefaultMap instance: 1개
  - `BP_BathhouseKeyRack_C_UAID_F02F7433CA36D1FF02_1155541561`(label `KeyRack`)
  - package `/Game/__ExternalActors__/Maps/DefaultMap/8/3G/CDTHV2ALC56NYO6HJ2VG2Q`, 위치 (0,-260,50)
  - `PairTransforms` 8개이며 CDO 배열과 완전히 같다. `FirstKeyNumber=1`, Key/Hook class는 CDO와 같다.
- 물리적으로 걸 수 있는 최대 열쇠 수는 8개(1~8번)다. 최상위 tier 2의 KeyPoolSize 8과 같다.
- 코드상 동작(`BathhouseKeyRackActor.cpp`): `MaterializeToPoolSize`가 `min(KeyPoolSize, PairTransforms.Num())`까지 hook과 key를 추가로 spawn한다. 줄이지는 않는다. KeyPoolSize가 PairTransforms보다 크면 Error 로그를 남긴다. Tier 변경은 `OnExpansionTierChanged`를 C++에서 바인딩해 받는다.

## 4. Clothes Locker와 락커 획득 경로

- DefaultMap의 `BathhouseFacilityActor` 계열 11개 중 `LockerSlotCount>0`인 것은 2개다.

| label | Blueprint | Definition | 칸 | 위치 |
|---|---|---|---|---|
| ClothesLocker_1 | `BP_ClothesLocker_C` | `Facility.ClothesLocker.1` | 1 | (1150,-380,0) |
| ClothesLocker_2 | `BP_ClothesLocker_C` | `Facility.ClothesLocker.1` | 1 | (1150,-180,0) |

  - 칸 수 합계는 2이고, tier 0의 `MaxInstalledLockerSlots=2`와 같다. 즉 시작 상태에서 락커 칸 상한이 이미 찼다.
  - 둘 다 Mode=Placed다. 4칸·8칸 Locker(`BP_ClothesLocker_4/_8`)의 Level instance는 없다.
  - 참고로 `ShoeLocker_1/2`(`BP_ShoeLocker_C`)는 Definition이 없어 칸 계산 대상이 아니다.
- Level의 free-world 포장 설비 아이템(`PlaceableFacilityItemActor` 계열)은 0개다. 포장된 락커도 없다.
- 락커 Placement Definition 3개
  - `DA_FacilityPlacement_ClothesLocker_1/_4/_8`(LockerSlotCount 1/4/8)
  - AssetRegistry referencer는 각각 자기 Blueprint(`BP_ClothesLocker`, `_4`, `_8`) 하나뿐이다.
  - 상점·다른 Content·Config(grep)에서 참조하지 않는다.
- 상점 catalog
  - Catalog asset은 `/Game/Bathhouse/Data/Shop/DA_ShopCatalog` 하나다.
  - Products는 20개이며 모두 `bForSale=true`다. 락커 상품은 없다. 모든 PlacementDefinition 상품의 LockerSlotCount는 0이다.
  - 상품 순서: Shower, Bath, Washer, Dryer, Boiler, Cooler, Circulator, DrinkFridge, BananaMilkBox, Vanity, HairDryerBox, SkinLotionBox, CottonSwabBox, CombBox, ShampooBox, BodyWashBox, MassageChair, RestBench, Television, ScrubTable
- 결론: 현재 Content에는 플레이어가 새 락커를 얻는 경로가 없다. 기존 락커 2개를 회수해 재설치하는 것은 Placement 일반 경로로 가능하다(코드 근거, PIE 미확인). 칸 수는 늘지 않는다.
- 코드상 상한 검사: `LockerCapacitySubsystem::CanInstallLockerSlots`는 설치 칸 합 + 추가 칸이 현재 tier의 `MaxInstalledLockerSlots`를 넘으면 거부한다.

## 5. 컴퓨터와 화면 Widget

- DefaultMap 컴퓨터 instance: 1개
  - `BP_BathhouseComputer_C_UAID_F02F7433CA3690F802_2051456727`(label `Computer`)
  - package `/Game/__ExternalActors__/Maps/DefaultMap/7/EH/E4FLO971KSWUJ40H7W7PHK`
  - 위치 (-470,0,160), scale (0.12,1.2,0.7)
  - `ManagedBathPlacementZone`은 `BP_FacilityPlacementZone_C_UAID_F02F7433CA36D1FF02_1155169559`다.
- `ScreenWidget`(CDO와 instance 동일)
  - `WidgetClass=/Game/Bathhouse/UI/WBP_ComputerScreenRoot.WBP_ComputerScreenRoot_C`
  - Draw Size (1024,576), `ReceiveHardwareInput=false`, Pivot (0.5,0.5), DrawAtDesiredSize=false
  - `.md/Unreal/InteractionUISystem.md`의 "컴퓨터 연결"에는 WidgetClass가 `WBP_BathWaterManagementScreen_C`로 적혀 있지만, 디스크 값은 `WBP_ComputerScreenRoot_C`다. 정본 갱신 대상이며 이번 조사에서는 수정하지 않았다.
  - Widget Space 속성명은 Python에서 읽지 못했다(미확인). 정본상으로는 World다.
- `WBP_ComputerScreenRoot`(native `ComputerScreenRootWidget`, widget 16개). 계층:
  - `RootOverlay > RootSize`(SizeBox 1024×576) `> RootFrame`(Border) `> RootColumn`(VerticalBox)
  - `TabBar`(Border, Auto) `> TabRow`(HorizontalBox, padding 12/6)
    - 탭 버튼은 2개다. 순서대로 `ManagementTabButton`(라벨 "관리", SizeBox 132×32, 오른쪽 6 간격), `ShopTabButton`(라벨 "상점", 132×32).
  - `ScreenSwitcher`(WidgetSwitcher, Fill, 저장된 ActiveWidgetIndex 0). child 순서:
    - [0] `ManagementScale`(ScaleBox) `> ManagementScreen`(`WBP_BathWaterManagementScreen_C`)
    - [1] `ShopScreen`(`WBP_ShopScreen_C`)
  - native BindWidget 5개(`ManagementTabButton`, `ShopTabButton`, `ScreenSwitcher`, `ManagementScreen`, `ShopScreen`)가 모두 있다. 탭 선택 상태는 native `bShopSelected` bool 하나다(`ComputerScreenRootWidget.h`). 세 번째 탭을 넣을 구조는 없다.
- `WBP_ShopScreen`(native `ShopScreenWidget`, widget 23개). 레이아웃 개요:
  - `RootOverlay > ShopFrame`(Border) `> ShopBody`(HorizontalBox, padding 14/12)
  - 왼쪽 `ProductColumn`(Fill 2.0, 오른쪽 12 간격)
    - `ProductHeader` "상품"
    - `ProductScroll`(ScrollBox 세로, Fill) `> ProductGrid`(WrapBox). 상품 카드가 여기에 생성된다.
  - 오른쪽 `CartPanel`(Border, Fill 1.0) `> CartColumn`(VerticalBox, padding 12/10). 위에서 아래 순서:
    - `BalanceText` "잔액 0원"
    - `CartHeader`: `CartTitle` "장바구니", `CartQuantityText` "0/10"
    - `CartScroll`(Fill 0.55) `> CartList`
    - `CartTotalText` "합계 0원"
    - `OrderButtonSize`(높이 36) `> OrderButton` "주문하기"
    - `OrderFeedbackText`
    - `OrderTitle` "배송 중인 주문"
    - `OrderScroll`(Fill 0.45) `> OrderList`
  - 즉 상품 카드 목록은 왼쪽 2/3, 장바구니·주문 버튼·주문 목록은 오른쪽 1/3 세로 열에 있다.
  - 주문 버튼은 장바구니 합계 바로 아래, 오른쪽 열 중간에 있다.
  - CDO class 설정: `ProductCardWidgetClass=WBP_ShopProductCard_C`, `CartLineWidgetClass=WBP_ShopCartLine_C`, `OrderLineWidgetClass=WBP_ShopOrderLine_C`
- `WBP_ShopProductCard`
  - `CardSize` 196×180 `> CardFrame > CardColumn`
  - `CardColumn` 안: `IconArea`(높이 72, `IconBackdrop`·`IconFallbackText` "아이콘 없음"·`IconImage`), `NameText`, `PriceText`, `CardSpacer`, `AddButtonSize`(높이 30) `> AddButton` "담기"
- `WBP_ShopCartLine`: `NameText`, `LineTotalText`, `MinusButton` "−", `QuantityText`, `PlusButton` "+", `RemoveButton` "삭제"
- `WBP_ShopOrderLine`: `SummaryText`, `StatusText`
- 화면 해상도: WidgetComponent Draw Size 1024×576이고, Root SizeBox override도 1024×576이다.

## 6. 손님 생성기와 확장 관계

- `BP_BathhouseCustomerSpawner` CDO: `CustomerClass=BP_BathhouseCustomer_C`, `RoutineDefinition=DA_CustomerRoutine_Default`, `Counter=None`, `SpawnIntervalSeconds=5.0`, `MaxActiveCustomers=4`, `bAutoStart=true`
- DefaultMap instance: 1개
  - `BP_BathhouseCustomerSpawner_C_UAID_F02F7433CA3615F402_1440546856`(label `Spawner`)
  - package `/Game/__ExternalActors__/Maps/DefaultMap/5/LL/B3I5R5QCCN6TPIOM68MSKO`, 위치 (700,-600,0)
  - instance override는 `Counter=BP_BathhouseCounter_C_UAID_F02F7433CA3615F402_1440216855` 하나다. 나머지 값(5.0초, 최대 4명, auto start)은 CDO와 같다.
- 확장 단계와 연결되지 않는다.
  - Spawner에는 tier·authority 참조 속성이 없다.
  - `BathhouseCustomerSpawner.cpp`는 `ActiveCustomers.Num() >= MaxActiveCustomers`만 검사한다. Expansion 참조가 없다.
  - asset 바이너리에도 tier API 참조가 없다.
- 참고: 시작 tier의 KeyPoolSize는 3이고 MaxActiveCustomers는 4다. 열쇠 부족 시 손님 처리 흐름은 이번 조사 범위 밖이다(미확인).

## 7. 확장 단계와 연결될 만한 다른 Content

- Placement Zone: DefaultMap에 1개 있다.
  - `BP_FacilityPlacementZone_C_UAID_F02F7433CA36D1FF02_1155169559`, 위치 (600,-100,0), Extent (1400,900,10)
  - `AllowedFacilityTags=Facility.Placeable`, `MajorGridIntervalCells=5`
  - native `FacilityPlacementZoneActor`에 잠김·단계 조건 속성이 없다(헤더 확인). Level에도 잠긴 영역 actor가 없다.
- `OnExpansionTierChanged`·`TryAdvanceToTier`·`GetCurrentTierIndex`를 쓰는 Blueprint asset: 없음.
  - 근거: `Content/` 전체 `.uasset/.umap`에서 이 이름들을 바이너리 문자열로 grep했고 0건이었다.
  - `BathhouseExpansionAuthority` 문자열은 BP 자신과 Level external actor 하나에서만 나온다.
  - `BathhouseExpansionDefinition` 문자열은 Authority BP와 DA에서만 나온다.
- C++ 소비처(참고)
  - `BathhouseKeyRackActor`: tier 변경 시 열쇠를 추가 생성한다.
  - `LockerCapacitySubsystem`: 설치 칸 상한을 검사한다.
  - `BathhouseFacilitySubsystem`: Authority 등록·조회를 담당한다.
  - `TryAdvanceToTier`는 현재보다 높은 유효 index만 허용하고 단계를 건너뛸 수 있다. 호출처는 테스트 외에 없다.

## 미확인·주의

- WidgetComponent의 Widget Space 값은 Python 속성명 불일치로 읽지 못했다.
- 상점 화면의 실제 렌더 크기·잘림, tier 변경 시 열쇠 생성 위치, 열쇠 부족 시 손님 흐름은 PIE·시각 확인 대상이며 이번 조사에서 하지 않았다.
- `InteractionUISystem.md`의 ScreenWidget WidgetClass 기록이 stale하다. Unreal 정본 갱신은 이후 Editor 작업 단계의 몫이다.
