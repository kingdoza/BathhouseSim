# Unreal Editor 인계 — 상점 주문·배송 상자·쓰레기통

## 현재 상태와 경계

- Source 구현과 headless 검증은 앞 단계에서 완료했다. UE 5.8 Build, Blueprint load gate, Shop/Economy/Placement focused automation과 전체 BathhouseSim 회귀가 통과했다.
- [완료, MCP 저장·재로드 확인] DA_ShopCatalog에는 Shower 상품(판매 가능, 표시명 샤워기, 가격 10000, Shower Placement Definition 참조)이 저장돼 있다. 일곱 Placement Definition은 Facility.Placeable 및 Facility.Discardable 태그를 가지고 LockerSlotCount=0이다.
- [완료, Compile·개별 Save] BP_ShopDeliveryBox, BP_ShopDeliveryPoint, BP_TrashBin을 지정 native class의 자식으로 만들었다. 각 asset은 compile 및 개별 저장을 마쳤고 생성 세션에서 parent/CDO 기본값을 확인했다. 새 Editor 프로세스 재로드는 아직 확인되지 않았다.
- [완료, Compile/load gate] BP_BathhousePlayerState와 BP_PlaceableFacilityItem을 compile/load 확인했고 dirty 상태가 아니었다.
- [미완료, MCP 기능 부재] Data Validation 도구가 없다. 저장 과정에서 Catalog validation 로그가 “Every shop product requires a valid placement definition.”를 반환했다. Definition 참조는 재로드로 확인했지만 원인을 해소하거나 validation 통과로 판정하지 않았다.
- [미완료, MCP 기능 부족·저장 실패] 위젯 계층 편집 및 Project Settings config 저장 tool이 없다. World Partition actor 저장 호출은 external actor package 경로가 없어 실패했다. 실제 결과와 수동 작업은 .md/USER_UNREAL.md의 상점 인계에 있다.
- [재개 필요, MCP 재시작 차단] 새 Editor 프로세스에서 방금 저장한 세 Blueprint의 재로드와 PIE 시작·종료를 확인하지 못했다. 마지막 작업용 Editor는 toolset 초기화가 두 차례 시간 초과되어 종료했다. 작업용 Editor와 MCP listener는 현재 닫혀 있다.
- 이번 Shop MCP 작업에서는 Config나 DefaultMap에 변경을 저장하지 않았다. 임시로 생성한 DeliveryPoint는 제거했고 Map은 clean 상태였다. 기존 사용자의 다른 변경은 보존한다.
- 샤워기만 판매한다. 혼합 주문과 7종 판매 확장은 이 단계의 대상이 아니다.

## 새 asset 경로와 API 계약

| 종류 | 경로 | Parent/API |
|---|---|---|
| Catalog | /Game/Bathhouse/Data/Shop/DA_ShopCatalog | UShopCatalog; Products 배열 순서가 카드 순서 |
| 배송 상자 | /Game/Bathhouse/Blueprints/Shop/BP_ShopDeliveryBox | AShopDeliveryBoxActor |
| 배송 지점 | /Game/Bathhouse/Blueprints/Shop/BP_ShopDeliveryPoint | AShopDeliveryPointActor |
| 쓰레기통 | /Game/Bathhouse/Blueprints/Shop/BP_TrashBin | ABathhouseTrashBinActor |
| 컴퓨터 root | /Game/Bathhouse/UI/WBP_ComputerScreenRoot | UComputerScreenRootWidget |
| 상점 화면 | /Game/Bathhouse/UI/Shop/WBP_ShopScreen | UShopScreenWidget |
| 상품 카드 | /Game/Bathhouse/UI/Shop/WBP_ShopProductCard | UShopProductCardWidget |
| cart 행 | /Game/Bathhouse/UI/Shop/WBP_ShopCartLine | UShopCartLineWidget |
| 주문 행 | /Game/Bathhouse/UI/Shop/WBP_ShopOrderLine | UShopOrderLineWidget |
| 잔액 HUD | /Game/Bathhouse/UI/WBP_MoneyHud | UMoneyHudWidget |
| 배송 알림 | /Game/Bathhouse/UI/WBP_ShopNotice | UShopNoticeWidget |

Shop 및 Data/Shop 폴더는 MCP로 생성했다. 모든 위젯은 아래 BindWidget 이름·native class를 그대로 사용한다.

## Widget hierarchy와 BindWidget

- WBP_ComputerScreenRoot: ManagementTabButton(UButton), ShopTabButton(UButton), ScreenSwitcher(UWidgetSwitcher), ManagementScreen(기존 WBP_BathWaterManagementScreen; UBathWaterManagementScreenWidget), ShopScreen(WBP_ShopScreen). Switcher 순서는 관리 index 0, 상점 index 1. 두 탭 버튼과 Switcher를 같은 root에 둔다.
- WBP_ShopScreen: ProductScroll(UScrollBox) 안에 ProductGrid(UWrapBox); BalanceText, CartList(UVerticalBox), CartQuantityText, CartTotalText, OrderButton, OrderFeedbackText, OrderList(UVerticalBox). native defaults에 ProductCardWidgetClass, CartLineWidgetClass, OrderLineWidgetClass를 연결한다.
- WBP_ShopProductCard: NameText, PriceText, IconImage, AddButton.
- WBP_ShopCartLine: NameText, QuantityText, LineTotalText, PlusButton, MinusButton, RemoveButton.
- WBP_ShopOrderLine: SummaryText, StatusText.
- WBP_MoneyHud: MoneyText, DeltaText. DeltaDisplaySeconds는 2초.
- WBP_ShopNotice: NoticeText. 스타일·레이아웃만 Blueprint에서 authoring하고 domain 상태·버튼 처리는 native에 둔다.

## 상품·Project Settings

- [완료] DA_ShopCatalog의 샤워기 entry는 ProductId=Shower, bForSale=true, DisplayName=샤워기, PlacementDefinition=/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Shower, Price=10000이다. 단, Data Validation 로그의 placement-definition 오류는 미해결이다.
- 선택적으로 Bath, Washer, Dryer, Boiler, Cooler, Circulator entry를 추가할 수 있다. 각각 unique ProductId와 해당 Definition 참조를 사용하고 이 단계에서는 bForSale=false로 둔다. Catalog Data Validation은 비판매 entry도 유효한 Definition과 가격을 요구한다.
- [미완료] Project Settings > Bathhouse Shop의 Catalog와 DeliveryBoxClass를 각각 위 DA와 BP_ShopDeliveryBox로 지정하고 config에 저장한다. 현재 MCP 시도는 memory만 바꿀 수 있어 Catalog를 None으로 되돌렸고 DefaultGame.ini는 변경되지 않았다. 기본값은 CartTotalQuantityLimit=10, PerProductQuantityLimit=99, DeliveryDelaySeconds=10초, UnboxForwardDistanceCm=100cm, DeliveryNoticeSeconds=3초다.
- [완료, 저장·새 프로세스 재로드 확인] 다음 일곱 Placement Definition 모두 Facility.Discardable native gameplay tag를 가진다: /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Bath, DA_FacilityPlacement_Shower, DA_FacilityPlacement_Washer, DA_FacilityPlacement_Dryer, DA_FacilityPlacement_Boiler, DA_FacilityPlacement_Cooler, DA_FacilityPlacement_Circulator. LockerSlotCount는 모두 0이다. ClothesLocker에는 이 태그를 추가하지 않았다.

## Actor defaults와 기존 Blueprint 연결

- [완료, 새 프로세스 재로드 대기] BP_ShopDeliveryBox의 BoxMesh root에 Engine BasicShapes Cube가 연결됐고 relative transform은 zero/unit이다. native 기본값 QueryAndPhysics, CCD, Pawn Ignore는 parent contract에서 읽었다. Data Validation은 미실행.
- [완료, 새 프로세스 재로드 대기] BP_ShopDeliveryPoint parent에는 SceneRoot와 editor-only billboard/arrow가 있다. MaxSearchHeightCm=1000, DropGapCm=10이며 arrow는 표시 전용이다.
- [완료, 새 프로세스 재로드 대기] BP_TrashBin의 BinMesh root에 Engine BasicShapes Cube가 연결됐고 relative transform은 zero/unit이다. native parent가 collision 및 carry/placement 제외 동작을 소유한다.
- [미완료] BP_BathhouseComputer의 ScreenWidget.WidgetClass를 WBP_ComputerScreenRoot로 지정한다. 기존 관리 화면과 ManagedBathPlacementZone 연결은 root 하위 ManagementScreen에서 보존한다.
- [미완료] BP_BathhouseHUD의 MoneyHudWidgetClass=WBP_MoneyHud, ShopNoticeWidgetClass=WBP_ShopNotice로 설정한다. 기존 InteractionPromptWidgetClass는 유지한다.
- [완료] BP_BathhousePlayerState와 BP_PlaceableFacilityItem은 이번 authoring에서 변경할 native property가 없으며 compile/load gate를 통과했다.

## Level, 저장, 재검증 순서

1. 원본 4개는 Source gate 시작 전 개별 백업했다. 경로는 `Saved/MigrationBackup/20260927_shop/`이며 현재 원본과 SHA-256이 모두 일치한다. Editor save 전에 원본 상태가 바뀌었다면 새 백업을 먼저 만든다. 무관한 기존 dirty Content는 저장하지 않는다.
   | 원본 | SHA-256 |
   |---|---|
   | BP_BathhouseComputer.uasset | `1FABD0E9693BAEEF1DAD96FC82C611393304F9FB44D63E5F3C6557A87F8E4375` |
   | BP_BathhousePlayerState.uasset | `C0B4376DC79770D7AEEF65CC7F6F0179B2E50E08CBB90A0AD93AA760CB5DB003` |
   | BP_BathhouseHUD.uasset | `B1E1BECD8EE2FC91AC6F38BB2A3A5F176E722EE7037A03BEE25964557601002D` |
   | BP_PlaceableFacilityItem.uasset | `6B83D9F5B2CE3F341DEEAE9356A5572F38574AD1C2DC41A3B4B5B98A84AC7471` |
2. [미완료] DefaultMap에 BP_ShopDeliveryPoint 하나와 BP_TrashBin 하나 이상을 저장 배치한다. 후보 위치 (1800,650,0)은 floor trace상 바닥 Z=0이고 주변이 열려 있다. 이전 SaveActor는 World Partition external package 경로가 없어 실패했고 임시 actor는 제거했다. 낙하 위치에 바닥, stack 공간과 낮은 천장 검증용 공간을 둔다.
3. [부분 완료] 세 Shop Actor Blueprint는 compile 및 개별 Save 완료다. Catalog와 7 Definition도 저장·재로드 확인했다. WBP, BP_BathhouseComputer/HUD wiring, Project Settings, DefaultMap external actor는 미완료다. Save All은 금지하며 변경하지 않은 dirty package는 저장하지 않는다.
4. Source gate load check는 2026-09-27에 완료했다. 복사본 4개를 단일 package gate로 각각 통과시킨 뒤 삭제했고, Content 무변경을 확인했다. 이어 원본 4개를 Template_Default와 DefaultMap에서 모두 통과시켰다. 각 보고서는 1/1 success, warning/fail 0이다.
   - 복사본: `Saved/Automation/Reports/20260927/Shop_Copy_Computer/`, `Shop_Copy_PlayerState/`, `Shop_Copy_HUD/`, `Shop_Copy_PlaceableItem/`.
   - 원본: `Saved/Automation/Reports/20260927/Shop_Originals_Template/`, `Shop_Originals_DefaultMap/`.
   Editor authoring/save 이후에는 다시 실행한다. Fatal, Serial size mismatch, Failed to load가 보이면 멈춘다.
   Catalog Project Settings 저장이 필요하면 Editor가 관리하는 Game config에만 기록하고, Source 구현 단계의 Config는 수정하지 않는다.
5. Source automation은 2026-09-27에 완료했다.
   - Shop focused: `Shop_Focused_Final/index.json` — 4 success, 0 warning, 0 fail.
   - Economy: `Economy_Fixture_Final/index.json` — 2 success, 0 warning, 0 fail.
   - Placement: `Placement_Final/index.json` — 5 success, 0 fail, 2 warning 상태 테스트.
   - 전체: `All_Regression_Final3/index.json` — 56 success, 6 succeeded-with-warnings, 0 fail, 0 notRun.
   전체 보고서 폴더는 `Saved/Automation/Reports/20260927/`이다. PIE와 Blueprint asset compile/save/reload는 Editor pass에서 실행한다.
6. [미완료] MCP 재연결이 정상화되면 PIE 시작·종료를 수행하고, 지원되는 로그/runtime 확인을 실행한다. 관리/상점 탭 전환과 상품·주문·배송·쓰레기통의 키보드/마우스 입력 및 화면 수용은 직접 입력 도구가 없어 별도 수용이 필요하다.

Shop catalog와 Definition의 재로드 확인 상태는 .md/Unreal/ShopSystem.md에 기록한다. Actor Blueprint의 새 프로세스 재로드가 확인된 뒤에만 Unreal 정본에 parent/CDO hierarchy를 추가한다. WBP/project settings/level actor 연결은 실제 저장으로 확인된 범위만 기록한다. 화면 배치, 충돌 수용, 실제 입력 우선순위는 수용 근거 전까지 완료로 기록하지 않는다.
