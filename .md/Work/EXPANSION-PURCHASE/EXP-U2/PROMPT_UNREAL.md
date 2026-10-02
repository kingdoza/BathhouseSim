# PROMPT_UNREAL — EXP-U2 확장 구입 수직(홀 1회) Editor 작업

- 작업 ID: `EXP-U2`
- 단계: 구현
- 상태: 완료

작업 상태: **작업 필요**(Content 변경 있음). 현재 단계는 구현 완료, 다음 입력 단계는 Editor 작업이다. C++ 계약은 [PROMPT_REVIEW.md](PROMPT_REVIEW.md)와 `.md/Architecture/ExpansionPurchaseSystem.md`·`BuildingSystem.md`가 정본이다. 이 문서는 Editor 쪽 exact asset·값 원본·검증 기준만 적는다.

## 1. 작업 모드와 기존 사용자 변경

- 모드: 수정(기존 asset 값 추가·Level instance 값 추가)과 생성(Widget Blueprint 2개). 새 빌드(구현 단계 최종 빌드, [PROMPT_REVIEW.md](PROMPT_REVIEW.md) 6절 식별값)로 Editor를 띄운다(`-ModelContextProtocolStartServer`, 메모리의 MCP 실행 주의 참조). BathhouseSim Editor만 대상이며 BeekeepingSim Editor(PID 30900)는 건드리지 않는다.
- 작업 시작 시 `git status`로 사용자 소유 변경(특히 `Content/`)을 확인한다. 이 단위에서 나온 것이 아닌 변경은 저장 대상에 넣지 않는다. 구현 단계는 Content·Config를 수정하지 않았다.
- 구현 단계 이후 Editor 작업 전 순서: **먼저** 새 빌드로 아래 asset을 저장하지 않고 로드해 오류가 없는지 확인한다(CoreSystem 규칙): `DA_BathhouseExpansion_Default`, `WBP_ComputerScreenRoot`, 공간 external actor 3개(`Space_Hall`·`Space_Bath`·`Space_Work`), `DA_ShopCatalog`. 구현 단계에서 `Computer.Input.ScreenWheelContentContract`와 기존 Building·Computer·Shop 자동화는 이미 통과했다.

### 생성·수정·저장 allowlist

| 구분 | 대상 | 허용 |
|---|---|---|
| 수정·저장 | `/Game/Bathhouse/Data/Expansion/DA_BathhouseExpansion_Default` | `Max Purchase Count`, `Purchase Prices`. `Tiers`는 값 확인만(수정은 확인 결과 규칙 위반일 때만) |
| 수정·저장 | `/Game/Bathhouse/Data/Shop/DA_ShopCatalog` | 맨 뒤 `ClothesLocker1` 줄 추가 |
| 수정·저장 | `Space_Hall`·`Space_Bath`·`Space_Work` external package 3개(아래 package 경로) | `Expansion Steps` 줄 추가. 저장 전 `Editor Preview Expansion Count` 0 |
| 조건부 수정·저장 | DefaultMap `NavMeshBounds`(NavMeshBoundsVolume) | 데이터 검증이 끝 모습 Nav 오류를 낼 때만 크기 조정(지하 제외 계약 유지) |
| 생성·저장 | `/Game/Bathhouse/UI/WBP_ExpansionSpaceOption`, `/Game/Bathhouse/UI/WBP_ExpansionScreen` | 새 Widget Blueprint |
| 수정·저장 | `/Game/Bathhouse/UI/WBP_ComputerScreenRoot` | 탭 버튼과 Switcher child 추가(8.6) |
| 확인만 | `ExpansionAuthority` Level instance, `KeyRack`(`BP_BathhouseKeyRack`) instance, `DA_FacilityPlacement_ClothesLocker_1` | 값 확인. 오류가 있으면 수정 전에 멈추고 보고 |
| 선택 | Project Settings > Game > Bathhouse Building `Editor Preview Label World Size Cm` | C++ 기본값을 쓰고, 편집 화면에서 글자가 너무 작거나 크면 조정(`Config/DefaultGame.ini` `[/Script/BathhouseSim.BathhouseBuildingSettings]`) |

allowlist 밖 asset(다른 설비 정의, 다른 Widget, 지형, 설비·장식 Actor 이동 등)은 수정하지 않고 멈춰 묻는다(상위 계약 사용자 결정 S2 A). 끝 모습 바닥·새 벽 자리에 Level Actor(설비·장식·마당 물건)가 있으면 옮기지 말고 보고한다.

## 2. 값 원본과 제안값

수치는 코드에 없다. 아래 값은 상위 계약 4.7의 **제안값**이며 확정된 정본은 asset 값이 된다.

| 값 | 원본 | 제안값(Editor가 넣음) |
|---|---|---|
| 전체 구입 횟수 상한 | `DA_BathhouseExpansion_Default` `Max Purchase Count` | 2 |
| 구입 가격 | 같은 asset `Purchase Prices`(줄 k = k+1번째 구입) | 1번째 100000, 2번째 300000(원) |
| 홀 넓힘 횟수별 열쇠·락커 한도 | 같은 asset `Tiers`(줄 = 홀 넓힘 횟수) | 기존 3줄 유지: 0회 3개·2칸, 1회 4개·4칸, 2회 8개·8칸. 줄 수는 상한 + 1 이상이어야 한다 |
| 공간별 넓힘 | 공간 instance `Expansion Steps`(줄 = 몇 번째 넓힘, `Side`·`Amount Cm`) | 세 공간 각 2줄, 양 400cm(4m) |
| 1칸 락커 가격 | `DA_ShopCatalog` `ClothesLocker1` `Price` | 8000(원) |
| 미리보기 글자 크기 | Settings `Editor Preview Label World Size Cm` | C++ 기본값 |

## 3. 항목별 작업

각 항목의 실행 경로(MCP·Editor Python·helper)는 Unreal Editor 단계가 실제 capability로 판정한다. 아래 힌트는 구현 단계에서 확인한 사실이다. "MCP 불가면 수동"을 미리 정하지 않았다.

### 3.1 `DA_BathhouseExpansion_Default`

- Class `UBathhouseExpansionDefinition`(UPrimaryDataAsset, 새 필드 `MaxPurchaseCount` int, `PurchasePrices` int 배열은 `EditDefaultsOnly`·`BlueprintReadOnly`라 Python `set_editor_property('max_purchase_count', …)`·`'purchase_prices'`로 접근 가능, 기존 `Tiers`는 같은 방식 존재 확인된 타입).
- 값: 위 2절. 저장·새 프로세스 재로드 뒤 `ValidatePurchaseData` 통과와 Data Validation 오류 0. 규칙: 가격 줄 ≥ 상한, 상한 안 가격 > 0, 효과 줄 ≥ 상한 + 1, 효과 줄이 줄어들지 않고 열쇠 ≥ 한도.

### 3.2 공간 Actor 3개 `Expansion Steps`

| Label | Actor | external package |
|---|---|---|
| `Space_Hall` | `BP_BathhouseSpace_C_UAID_F02F7433CA362F0703_1270228495` | `/Game/__ExternalActors__/Maps/DefaultMap/8/7N/V36YOPHA8C46E77EZIX78C` |
| `Space_Bath` | `BP_BathhouseSpace_C_UAID_F02F7433CA362F0703_1645196495` | `/Game/__ExternalActors__/Maps/DefaultMap/6/85/2UD4T92UQSBNFKC3N8BZFK` |
| `Space_Work` | `BP_BathhouseSpace_C_UAID_F02F7433CA362F0703_1645216496` | `/Game/__ExternalActors__/Maps/DefaultMap/9/0G/5Z3D27MZMK7HDNFHFAEYU2` |

- Property: Details `Bathhouse Space › Expansion`의 `Expansion Steps`(`TArray<FBathhouseSpaceExpansionStep>`, `BlueprintType` struct라 Python에서 `unreal.BathhouseSpaceExpansionStep` 구조체 배열로 접근 가능한지 Editor 단계가 확인). `Editor Preview Expansion Count`(Transient, EditInstanceOnly, 0~줄 수 자동 clamp)는 미리보기 전용이라 **저장 전에 반드시 0**.
- 각 공간 2줄, `Amount Cm` = 400. `Side` 규칙(world 축, 동 +X 서 -X 북 +Y 남 -Y): 0회에 다른 공간과 **맞닿은 변은 쓰지 않는다**(오류), 바깥 출입구 변도 쓰지 않는다(경고).
  - 홀: 동쪽(목욕공간과 맞닿음, 통로 벽 포함)과 서쪽(바깥 출입구) 제외 → **남·북 중**. 제안 줄1 = 남, 줄2 = 북.
  - 목욕공간: 서쪽(홀과 맞닿음) 제외 → 동·남·북 중. 제안 줄1 = 북, 줄2 = 남.
  - 작업공간: 지하라 같은 높이 공간이 없어 아무 변. 제안 줄1 = 서, 줄2 = 동. 홀 계단(`Space_Hall` `Stairs[0]`, 동쪽으로 내려감)과 아래층 손님 길 계약(Nav 지하 제외)을 해치지 않는지 미리보기로 확인.
- 위는 제안이다. 데이터 검증 오류가 나거나 끝 모습에 Level Actor가 걸리면 같은 규칙 안에서 다른 변을 고르고, 그래도 안 되면 멈춰 보고한다.
- 확인 절차(설정마다): `Editor Preview Expansion Count`를 1, 2로 올려 벽·바닥·천장·조명·배치 격자·조각 미리보기 선이 넓힌 모습을 따르고 공간 위에 `넓힘 미리보기 N회`(겹치면 ` · 겹침 있음`)가 뜨는지 확인 → 끝 모습의 바닥·새 벽 자리에 Level Actor(설비·장식·마당 물건)와 지형이 걸리지 않는지 확인 → 0으로 되돌린 뒤 공간 3개 Data Validation(오류 0, 경고는 기존 `Space_Bath`의 opt-out 2개만) → `save_packages`로 external package 저장 → 새 프로세스 재로드로 값 확인.
- 편집 world에서 글자가 잘 보이는지(크기·방향: 위에서 읽히고 위쪽이 북)는 이번에 처음 확인되는 경로다. 이상하면 Settings 글자 크기만 조정하고, 방향 등 코드 문제면 구현 복귀로 보고한다.

### 3.3 `NavMeshBounds`

- 공간 Data Validation이 `목록 끝까지 넓힌 모습: … 손님 길 범위(NavMeshBoundsVolume)` 오류를 내는 경우에만 크기를 조정한다. 조정 때 홀·목욕공간 끝 모습 바닥과 출입구 앞 바깥 지점이 한 volume에 들고, 작업공간 끝 바닥 중심은 volume 밖(지하 제외)이어야 한다. 편집 world에서 끝 모습 미리보기로 손님 길(Nav)이 덮이는지 눈으로 확인한 뒤 미리보기를 0으로 되돌린다.

### 3.4 Widget Blueprint

공통: 해상도 1024×576 화면 안(탭 바 아래 영역). graph는 만들지 않는다(상태·domain 호출·선택 표시 금지). 스크롤 영역은 필요 없다. 만들면 `COMPUTER-WHEEL-SCROLL` 계약(`ConsumeMouseWheel=WhenScrollingPossible`, `AnimateWheelScrolling=false`, hit test Visible)을 따르고 `Computer.Input.ScreenWheelContentContract`가 통과해야 한다. 고정 문구만 WBP에 두고 나머지 문구는 C++가 채운다.

**`/Game/Bathhouse/UI/WBP_ExpansionSpaceOption`** — Parent `/Script/BathhouseSim.ExpansionSpaceOptionWidget`(`UExpansionSpaceOptionWidget`, Abstract·Blueprintable).

| BindWidget 이름 | 타입 | 필수 |
|---|---|---|
| `SelectButton` | `Button` | 필수. 카드 전체를 덮는다 |
| `NameText` | `TextBlock` | 필수 |
| `SizeText` | `TextBlock` | 필수 |
| `EffectText` | `TextBlock` | 필수. 두 줄(열쇠 / 락커 칸 한도) 들어갈 높이 |
| `StatusText` | `TextBlock` | 필수 |
| `SelectionHighlight` | `Widget`(예: Border/Image) | 필수. 선택 표시. 기본 Visibility는 상관없음(C++가 Visible/Collapsed로 정함) |

**`/Game/Bathhouse/UI/WBP_ExpansionScreen`** — Parent `/Script/BathhouseSim.ExpansionScreenWidget`(`UExpansionScreenWidget`, Abstract·Blueprintable).

| BindWidget 이름 | 타입 | 내용 |
|---|---|---|
| `StageText` | `TextBlock` | `현재 확장 단계: N`(C++) |
| `BalanceText` | `TextBlock` | `잔액 N원`(C++) |
| `LockerText` | `TextBlock` | `설치된 락커 칸 a/b`(C++) |
| `OptionsPanel` | `PanelWidget` | 세 선택지 묶음(C++가 숨김) |
| `HallOption`, `BathOption`, `WorkOption` | `WBP_ExpansionSpaceOption_C`(`ExpansionSpaceOptionWidget` 파생) | 가로 3열 권장. 종류는 C++가 NativeConstruct에서 지정 |
| `PriceText` | `TextBlock` | `이번 구입 가격 N원`(C++) |
| `PurchasePanel` | `PanelWidget` | 구입 버튼 묶음 |
| `PurchaseButton` | `Button` | 클릭 → 확인 대기 |
| `PurchaseButtonText` | `TextBlock` | `확장 구입 (N원)`(C++), `PurchaseButton` 안 |
| `ConfirmPanel` | `PanelWidget` | WBP 고정 문구 `정말 구입할까요?`와 두 버튼. `PurchasePanel`과 같은 자리 |
| `ConfirmButton`, `CancelButton` | `Button` | 확인·취소(WBP 고정 문구 `확인`, `취소`) |
| `ShortfallText` | `TextBlock` | `N원 부족`(C++) |
| `ResultText` | `TextBlock` | `확장 완료`(C++) |
| `MessageText` | `TextBlock` | `최대 확장 단계입니다` / `확장을 사용할 수 없습니다`(C++) |

- 배치 순서(제안): 헤더(단계·잔액·락커) → 세 선택지 → 가격·부족·완료·메시지 → 구입/확인 자리. 컨테이너 Visibility는 `SelfHitTestInvisible` 또는 Visible, 버튼은 Visible(C++가 패널을 `SelfHitTestInvisible`/`Collapsed`로 바꾼다).
- **`ConfirmButton`이 `PurchaseButton` 바로 그 위치에 오지 않게** 둔다(구입 버튼 더블클릭이 확인으로 이어지지 않게. `ConfirmPanel` 안에서 `ConfirmButton` 자리에 `취소`나 안내 문구가 오는 배치 등). 이 점은 EXP-027 PIE 관찰 항목이다.
- 글꼴·색은 기존 `WBP_ShopScreen`·`WBP_BathWaterManagementScreen`과 일관되게(어두운 패널·밝은 글씨). Compile 경고 0, 개별 Save.

**`/Game/Bathhouse/UI/WBP_ComputerScreenRoot`** — `TabBar`에서 `ShopTabButton` 뒤에 `ExpansionTabButton`(`Button`, 라벨 `확장`, 기존 탭 버튼과 같은 크기·간격, BindWidgetOptional이라 이름 정확히 `ExpansionTabButton`), `ScreenSwitcher` child [2]에 `ExpansionScreen`(`WBP_ExpansionScreen_C`, 이름 정확히 `ExpansionScreen`). 기존 child 0·1과 계층·이름은 그대로. 조상 widget은 Visible 또는 SelfHitTestInvisible로 유지한다. 탭 순서는 `관리 · 상점 · 확장`이다.

### 3.5 `DA_ShopCatalog`

- 맨 뒤(21번째)에 새 줄: `ProductId=ClothesLocker1`, `bForSale=true`, `DisplayName=1칸 락커`, `Price`=8000(제안), `PlacementDefinition=/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_1`, `ItemBoxDefinition` 비움. 기존 20개와 순서는 유지. Icon 등 다른 필드는 기존 줄의 관례를 따르고 기존 줄의 설정은 건드리지 않는다.
- `DA_FacilityPlacement_ClothesLocker_1`에 `Facility.Discardable`이 **없음**을 확인한다(있으면 멈추고 보고: 새 상점 규칙이 상품을 거부함). Catalog Data Validation 오류 0.

### 3.6 확인만 하는 항목

- `ExpansionAuthority` Level instance `Initial Tier Index` = 0, `Expansion Definition` = `DA_BathhouseExpansion_Default`. Data Validation 오류 0.
- `KeyRack`(`BP_BathhouseKeyRack` instance) `Pair Transforms` 개수 ≥ `Tiers`에서 도달 가능한(상한 2 → index 0~2) 최대 열쇠 수. 부족하면 멈추고 보고(열쇠걸이 자리 추가는 allowlist 밖). Data Validation 오류 0.

## 4. 실행 전제·검증 순서·멈춤 조건

- helper 설치·버전 조건: 구현 단계가 새 helper를 만들지 않았다. 기존 helper(external actor 저장 `save_packages` 등)만 쓴다. Python/MCP로 구조체 배열·위젯 트리 편집이 막히면 어느 단계에서 막혔는지 `USER_UNREAL.md`에 정확히 기록한다.
- 순서: 비저장 로드 확인 → 3.1 → 3.5 → 3.2(미리보기 확인 포함) → 3.3(필요 시) → 3.4 → 3.6. 각각 Compile/개별 Save/새 프로세스 재로드로 확인하고 확인하지 못한 항목은 미검증으로 적는다.
- 끝으로 자동화를 다시 돌린다: 필터 `BathhouseSim` 전체. 구현 단계 기준은 173개 중 172 통과, 실패 1개가 `BathhouseSim.Expansion.Content.ScreenContract`이며 이 테스트는 Editor 작업 뒤 통과해야 한다(root 탭 버튼·화면·Switcher child 3개, 두 WBP 로드, 카탈로그 마지막 상품이 1칸 락커 정의, `DA_BathhouseExpansion_Default` `ValidatePurchaseData` 통과와 `Max Purchase Count` 1 이상). `Computer.Input.ScreenWheelContentContract`는 계속 통과해야 한다.
- 멈춤 조건(저장하지 않고 보고, 영향 없는 allowlist 항목은 저장·재로드까지 마침): 비저장 로드에서 오류, 데이터 검증이 같은 규칙 안에서 풀리지 않는 오류, 끝 모습에 Level Actor가 걸림, allowlist 밖 asset 수정이 필요, 상점 락커 상품이 규칙에 걸림, 미리보기 글자 방향·크기 이상(코드 문제), C++ 계약이 실제 asset·엔진 동작과 맞지 않음(아키텍처 복귀).

## 5. Blueprint에서 구현하면 안 되는 C++/domain 로직

- 구입 가능 판단, 가격·횟수 계산, 넓힘 적용, 열쇠·한도 상승, 결제, 선택·확인 대기·완료 표시 상태, 문구 조립은 모두 C++다. WBP graph는 hierarchy·layout·style·asset 연결만 한다(WBP graph로 클릭 처리·domain 호출을 만들지 않는다).
- Data Validation 규칙, 공간 형상, 미리보기, 효과 횟수는 C++가 소유한다. Level 값이나 WBP로 우회하지 않는다.

## 6. 시나리오별 사용자 PIE 관찰 항목

대표: EXP-023 → EXP-028. 목욕·작업공간 구입, 두 번째 구입, 최대 표시, 4·8칸 락커는 U3 범위라 이번 PIE 대상이 아니다.

| 시나리오 | 관찰 항목과 기대 결과 |
|---|---|
| EXP-020 | 컴퓨터를 처음 쓰면 `관리` 탭이 기본이고 탭 순서가 `관리 · 상점 · 확장`이다 |
| EXP-021 | 확장 탭: `현재 확장 단계: 0`, `설치된 락커 칸 2/2`, 가격, 잔액, 세 선택지(전후 크기), 홀에 `열쇠 3개 → 4개`·`락커 칸 한도 2칸 → 4칸`(수치는 데이터), 구입 버튼 꺼짐 |
| EXP-022 | 잔액이 가격보다 적으면 구입 버튼 꺼짐과 `N원 부족`. 손님 결제로 잔액이 가격 이상이 되면 탭을 다시 열지 않아도 켜진다 |
| EXP-023 | `홀` 선택 → 구입 → `확인`: 돈이 한 번 줄고(HUD 변화량), 홀 바깥벽이 물러나고 바닥·천장·조명이 넓은 홀을 덮고, 4번 열쇠가 걸이에 나타나고, 탭이 `확장 완료`·`현재 확장 단계: 1`·`설치된 락커 칸 2/4` |
| EXP-024 | 넓어진 바닥을 걷고 홀 설비 미리보기·설치 가능, 손님이 지나다니고 쓰레기가 생길 수 있다. 몇 초 뒤 손님이 넓은 바닥을 지나는지(Recast Dynamic 갱신) |
| EXP-025 | 넓히는 순간 옛 벽 근처의 플레이어·손님·물건이 밀리거나 끼이거나 사라지지 않는다 |
| EXP-026 | 확인 대기 중 `취소`·다른 탭·컴퓨터 이탈 → 아무것도 안 바뀌고 다시 오면 구입 버튼부터 |
| EXP-027 | `확인` 연타 → 돈 변화량 HUD 한 번, 열쇠 하나만 추가, 구입 버튼 더블클릭이 확인으로 이어지지 않는다 |
| EXP-028 | 확장 1회 상태에서 상점 `1칸 락커` 구입·배송·개봉·설치 → `설치된 락커 칸 3/4`, 손님 3명 이상이 동시에 락커를 쓴다 |
| EXP-029 | 0회 상태(재시작)에서 1칸 락커 구입은 되고 설치는 `현재 확장 단계의 설치 가능한 락커 칸 수를 초과합니다`로 막혀 손에 남는다. 홀 확장 뒤 다시 조준하면 설치된다 |
| EXP-030 | 락커 아이템은 목욕공간·지하에서 공간 불가 문구, 쓰레기 수거 구역에서 사라지지 않는다 |
| EXP-031 | `ExpansionAuthority`의 `Expansion Definition`을 **저장하지 않고** 비운 채 PIE → 확장 탭에 `확장을 사용할 수 없습니다`, 돈 불변, Output Log `LogBathhouseExpansion` Error가 원인별 한 번. 끝나면 되돌림(저장 금지) |
| EXP-032 | 관리·상점 탭 동작과 휠 스크롤이 지금과 같다 |

관찰 메모: EXP-022·029는 시작 잔액이 구입 가격과 락커 가격 합보다 적으면 손님 결제로 모으거나 PlayerState Blueprint `StartingMoney`를 **저장하지 않고** 임시로 올린다.

## 7. 갱신할 `.md/Unreal/*System.md` 정본

- `Unreal/BuildingSystem.md`: 공간별 `Expansion Steps` 원본 위치와 현재 줄(Side·Amount)·`Editor Preview Expansion Count` 사용법·`Editor Preview Label World Size Cm`, Data Validation 현재 상태, 필요하면 `NavMeshBounds` 조정 결과.
- `Unreal/ShopSystem.md`: `DA_ShopCatalog` 21번째 `ClothesLocker1` 줄(가격 원본 위치).
- `Unreal/InteractionUISystem.md`: 탭 3개, `WBP_ExpansionScreen`·`WBP_ExpansionSpaceOption` 구조(BindWidget·배치), `WBP_ComputerScreenRoot` Switcher child [2]. 같은 문서의 낡은 `WidgetClass` 관련 기록이 새 상태와 어긋나면 정정.
- `Unreal/FacilitySystem.md`: 확장 정의 새 필드(`MaxPurchaseCount`, `PurchasePrices`, `Tiers` 뜻이 "홀 넓힘 횟수별")의 원본 위치와 현재 줄 수, `ExpansionAuthority`·`KeyRack` 검증 결과.
- 날짜별 기록·수치 복제는 하지 않는다(원본 위치만).
