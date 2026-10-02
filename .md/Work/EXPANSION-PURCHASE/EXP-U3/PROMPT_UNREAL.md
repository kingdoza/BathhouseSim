# PROMPT_UNREAL — EXP-U3 전체 확장 묶음 Editor 작업

- 작업 ID: `EXP-U3`
- 단계: 구현
- 상태: 완료

작업 상태: **작업 필요**(Content 변경 있음). 현재 단계는 구현 완료, 다음 입력 단계는 Editor 작업이다. C++ 계약의 정본은 [PROMPT_IMPLEMENTATION.md](PROMPT_IMPLEMENTATION.md) 13.3절과 `.md/Architecture/ExpansionPurchaseSystem.md`·`BuildingSystem.md`다. 이 문서는 13.3절을 exact asset·값 원본 위치·검증 기준으로 옮긴 것이며 구현에서 바뀐 이름·API는 없다(PROMPT_REVIEW.md 참조). 값이 이 문서와 설계가 다르면 설계 절이 우선한다(FBK-006).

## 1. 작업 모드와 기존 사용자 변경

- 모드: 수정(기존 asset·Level instance 값 변경)만. 새 asset 생성 없음. 새 빌드([PROMPT_REVIEW.md](PROMPT_REVIEW.md) 6절 식별값)로 Editor를 띄운다(`-ModelContextProtocolStartServer`, 메모리의 MCP 실행 주의 참조). BathhouseSim Editor만 대상이며 BeekeepingSim Editor는 실행 인자로 구분해 건드리지 않는다.
- 작업 시작 시 `git status`로 사용자 소유 변경(특히 `Content/`)을 확인한다. 이 단위에서 나온 것이 아닌 변경은 저장 대상에 넣지 않는다. 구현 단계는 Content·Config를 수정하지 않았다. 다른 세션 파일(`AGENTS.md`, `.md/MODELING_*`, `ArtSource`, `.md/Work/MODEL-M1`)은 건드리지 않는다.
- 전환 상태: 새 빌드 직후 Editor 작업 전에는 세 공간 `Expansion Steps` 줄이 새 형식에서 빈 값(`Sides` 비어 있음, `Price` 0)이다. 이때 공간 Data Validation 오류와 확장 탭의 "이 공간은 더 넓힐 수 없습니다"가 나오는 것은 정상 전환 상태다(설계 10절). 옛 `Side`·`Amount Cm`(줄 안 직접 필드)은 load 시 건너뛰어지고 값은 읽히지 않는다.

### 허용 목록 (allowlist)

| 구분 | 대상 | 허용 |
|---|---|---|
| 수정·저장 | `/Game/Bathhouse/Data/Expansion/DA_BathhouseExpansion_Default` | 재저장만(지운 `MaxPurchaseCount`·`PurchasePrices` 정리). `Tiers`는 그대로 |
| 수정·저장 | `Space_Hall`·`Space_Bath`·`Space_Work` external package 3개(3.2) | `Expansion Steps` 다시 입력. 저장 전 `Editor Preview Expansion Count` 0 |
| 수정·저장 | DefaultMap `NavMeshBounds`(3.3) | 끝 모습을 덮도록 X·Y 확대. Z 그대로 |
| 수정·저장 | `/Game/Bathhouse/UI/WBP_ExpansionScreen` | `StageText`·`PriceText` 삭제, 남은 배치 정리 |
| 수정·저장 | `/Game/Bathhouse/UI/WBP_ExpansionSpaceOption` | 카드에 `StageText`·`PriceText` 추가 |
| 수정·저장 | `/Game/Bathhouse/Data/Shop/DA_ShopCatalog` | 맨 뒤에 `ClothesLocker4`, `ClothesLocker8` 줄 추가 |
| 확인만 | `ExpansionAuthority`·`KeyRack` Level instance, `DA_FacilityPlacement_ClothesLocker_4`·`_8` | 값·Data Validation 확인. 오류가 있으면 수정 전에 멈추고 보고 |

allowlist 밖 asset(root WBP, 열쇠걸이, 확장 관리자, 설비 정의, 지형, 설비·장식 Actor 이동 등)은 수정하지 않는다. 끝 모습 띠에 Level Actor(설비·장식·마당 물건)가 있으면 옮기지 말고 멈춰 보고한다(사전 허용 S2 A, 승인 범위 밖 asset).

## 2. 값 원본과 값

수치는 코드에 없고 이 문서에도 복제하지 않는다. 넣는 값은 상위 계약 [../PROMPT_ARCHITECTURE.md](../PROMPT_ARCHITECTURE.md) 4.7절 표의 해당 칸과 정확히 같다. 확정된 정본은 asset 값이 된다.

| 값 | 원본(asset·property) | 4.7절 칸 |
|---|---|---|
| 넓힘 가격(공간별·몇 번째) | 공간 Actor `Expansion Steps[k].Price` | "확장 구입 가격 (D2)" 행 |
| 넓힘 방향·벽별 양 | 공간 Actor `Expansion Steps[k].Sides[i].Side`·`Amount Cm` | "한 번 넓어지는 방향과 변별 양 (D3)" 행(변마다 같은 양) |
| 공간별 넓힘 횟수 상한 | 공간 Actor `Expansion Steps` 줄 수 | "공간별 넓힘 횟수 상한 (D2)" 행 |
| 홀 넓힘 횟수별 열쇠 수·락커 칸 한도 | `DA_BathhouseExpansion_Default` `Tiers` | "홀 넓힘 횟수별 열쇠 수·락커 칸 한도" 행(그대로) |
| 락커 상품 가격 | `DA_ShopCatalog` `ClothesLocker4`·`ClothesLocker8` `Price` | "락커 상품 가격" 행의 4칸·8칸 |
| 손님 길 범위 | DefaultMap `NavMeshBounds` | 4.7절 값 없음. 끝 모습 검증이 통과하는 크기 |

## 3. 항목별 작업 (순서대로)

각 항목의 실행 경로(MCP·Editor Python·helper)는 Unreal Editor 단계가 실제 capability로 판정한다. 아래는 구현 단계가 확인한 사실이며 "MCP 불가면 수동"을 미리 정하지 않았다.

### 3.1 사전 load 확인 (저장하지 않음)

- 새 빌드로 `DA_BathhouseExpansion_Default`, 공간 external actor 3개, `WBP_ExpansionScreen`, `WBP_ExpansionSpaceOption`, `WBP_ComputerScreenRoot`를 비저장 load해 Fatal·load 오류가 없는지 본다(지운 property 경고만 허용, CoreSystem 규칙). `WBP_ExpansionScreen`에 옛 binding 이름의 widget이 남아 있어도 BindWidget이 없는 일반 widget이라 compile된다.
- 구현 단계 자동화에서 이미 확인된 것: 새 구조체 형식에서 `ScreenWheelContentContract`(root가 품은 WBP load)는 통과했다.

### 3.2 `DA_BathhouseExpansion_Default`

- Class `UBathhouseExpansionDefinition`(UPrimaryDataAsset). 새 빌드로 열어 재저장해 지운 `MaxPurchaseCount`·`PurchasePrices`를 파일에서 정리한다. `Tiers` 3줄(홀 줄 2 + 1)은 그대로 둔다(줄 수는 홀 `Expansion Steps` 줄 수 + 1 이상이어야 한다).
- 확인: 새 프로세스 재로드, Data Validation 오류 0. 규칙은 효과 표가 비어 있지 않고, 줄끼리 줄어들지 않고, 열쇠 ≥ 락커 한도뿐이다.
- 경로 힌트: 기존 두 필드는 이제 C++에 없어 Python `get_editor_property`로 읽히지 않는다. 재저장 후 값이 없어졌는지는 새 프로세스에서 `Tiers`만 읽혀도 충분하다.

### 3.3 공간 Actor 3개 `Expansion Steps`

| Label | Actor | external package |
|---|---|---|
| `Space_Hall` | `BP_BathhouseSpace_C_UAID_F02F7433CA362F0703_1270228495` | `/Game/__ExternalActors__/Maps/DefaultMap/8/7N/V36YOPHA8C46E77EZIX78C` |
| `Space_Bath` | `BP_BathhouseSpace_C_UAID_F02F7433CA362F0703_1645196495` | `/Game/__ExternalActors__/Maps/DefaultMap/6/85/2UD4T92UQSBNFKC3N8BZFK` |
| `Space_Work` | `BP_BathhouseSpace_C_UAID_F02F7433CA362F0703_1645216496` | `/Game/__ExternalActors__/Maps/DefaultMap/9/0G/5Z3D27MZMK7HDNFHFAEYU2` |

- Property: Details `Bathhouse Space › Expansion`의 `Expansion Steps`(`TArray<FBathhouseSpaceExpansionStep>`). 줄은 `Sides`(`TArray<FBathhouseSpaceExpansionSide>`, 항목은 `Side`·`Amount Cm`)와 `Price`(int, 원)다. Python에서는 `unreal.BathhouseSpaceExpansionStep`·`unreal.BathhouseSpaceExpansionSide` 구조체 배열로 접근 가능한지 Editor 단계가 확인한다(둘 다 `BlueprintType`, 필드 `EditAnywhere`). `Sides`는 접힌 상태에서도 항목 이름이 `Side`로 보인다(`TitleProperty`). `Editor Preview Expansion Count`(Transient, EditInstanceOnly)는 **저장 전에 반드시 0**.
- 줄 구성(각 공간 줄 2개, `Sides` 항목 순서 그대로, `Amount Cm`은 4.7절 방향·양 행의 "변마다" 값으로 모든 항목 같음, `Price`는 4.7절 가격 행의 해당 칸):

| 공간 | 줄 | `Sides`(순서) | `Price` |
|---|---|---|---|
| `Space_Hall` | `[0]` | 남(-Y), 북(+Y) | 홀 1번째 |
| `Space_Hall` | `[1]` | 남(-Y), 북(+Y) | 홀 2번째 |
| `Space_Bath` | `[0]` | 남(-Y), 북(+Y), 동(+X) | 목욕공간 1번째 |
| `Space_Bath` | `[1]` | 남(-Y), 북(+Y), 동(+X) | 목욕공간 2번째 |
| `Space_Work` | `[0]` | 동(+X), 북(+Y) | 작업공간 1번째 |
| `Space_Work` | `[1]` | 동(+X), 북(+Y) | 작업공간 2번째 |

- 같은 벽을 한 줄에 두 번 넣지 않는다. 서쪽(출입구 벽)·홀 동쪽(목욕공간 통로 벽)·목욕공간 서쪽(홀과 맞닿음)은 넣지 않는다(오류 또는 경고).

### 3.4 끝 모습 확인 (저장하지 않음)

- 세 공간 `Editor Preview Expansion Count`를 2로 둔다. 편집 화면 미리보기 글자에 ` · 겹침 있음`이 없어야 한다.
- 지상 넓힘 띠(홀 남·북, 목욕공간 남·북·동, 모서리 포함)에 Level Actor(편집 전용 sprite·frustum 제외)가 없는지, 지형 면이 지상 바닥 판 아래인지 본다. 지형·장애물 판정은 WorldStatic·WorldDynamic object type 단순 충돌 trace(`bTraceComplex=false`)로 한다(FBK-001). 띠에 Level Actor가 있으면 옮기지 말고 멈춰 보고한다.
- 판정이 `Editor` 편집 world 한정이면 Recast·Landscape 구성이 PIE와 다를 수 있다. 눈 확인이 필요한 것은 PIE 체크리스트로 넘긴다.

### 3.5 `NavMeshBounds`

- DefaultMap `NavMeshBounds`(`NavMeshBoundsVolume`, package `/Game/__ExternalActors__/Maps/DefaultMap/8/8U/DVJA0LL4M6BXDCMLJ35BD5`). 3.4의 끝 모습에서 공간 Data Validation에 `ExpansionNavOutside`가 나오지 않을 때까지 X·Y 범위(scale 또는 크기)를 넓힌다. 홀 서쪽 출입구 밖 마당은 계속 덮고, Z 범위는 그대로 두어 작업공간 바닥을 넣지 않는다(`ExpansionNavWorkCovered` 없음).
- 편집 world에서 끝 모습 미리보기로 손님 생성기에서 홀·목욕공간 넓힘 띠까지 경로가 있고 작업공간 띠에는 Nav가 없는지 본다(Recast 편집 결과는 저장하지 않음). 판정이 어려우면 PIE 체크리스트로 넘긴다.

### 3.6 저장과 재로드

- 세 공간 미리보기 횟수를 0으로 돌린 뒤 공간 3개와 `NavMeshBounds`의 external package를 저장한다(Python `unreal.EditorLoadingAndSavingUtils.save_packages`, 개별 Save). 새 프로세스에서 재로드해 값을 읽는다.
- 공간 Data Validation: 오류 0. 경고는 기존 `Space_Bath` opt-out 2개(허용 종류가 아닌 `UsedTowelBin`·`CleanTowelStack`)만, 출입구 변 경고는 없어야 한다.

### 3.7 `/Game/Bathhouse/UI/WBP_ExpansionScreen`

- `HeaderRow`의 `StageText`, `InfoColumn`의 `PriceText`를 삭제하고 남은 배치를 정리한다. native `UExpansionScreenWidget`은 이 두 binding을 더 이상 갖지 않는다(삭제됨). 다른 binding(`BalanceText`, `LockerText`, `OptionsPanel`, `HallOption`·`BathOption`·`WorkOption`, `PurchasePanel`, `PurchaseButton`, `PurchaseButtonText`, `ConfirmPanel`, `ConfirmButton`, `CancelButton`, `ShortfallText`, `ResultText`, `MessageText`)은 그대로다.
- 확인: Compile 경고 0, 개별 Save, 재로드 뒤 두 widget 이름이 없음(content 자동화가 단언).

### 3.8 `/Game/Bathhouse/UI/WBP_ExpansionSpaceOption`

- 부모 `UExpansionSpaceOptionWidget`. `CardColumn`을 `NameText` → `StageText` → `SizeText` → `PriceText` → `EffectText` → `StatusText` 순서로 만든다. 새 두 TextBlock(`StageText`, `PriceText`)은 `BindWidgetOptional`이라 이름이 정확해야 연결된다(Variable 체크). 기존 문구 style을 따른다. 문구와 가시성은 C++가 설정하므로 기본 텍스트·가시성 값은 중요하지 않다(가시성은 C++가 덮어쓴다).
- 필요하면 `OptionSize` 높이를 늘리되 세 카드와 아래 정보·구입 영역이 `RootSize` 1024×576 안에 들어가는지 widget 크기 합으로 확인한다. Compile 경고 0, 개별 Save.
- 화면 잘림의 눈 확인은 숨김 Editor에서 컴퓨터 화면이 그려지지 않으면 사용자 PIE 체크리스트로 넘긴다(FBK-003).

### 3.9 `/Game/Bathhouse/Data/Shop/DA_ShopCatalog`

- 맨 뒤에 두 줄 추가한다(기존 21개 순서 유지, `ClothesLocker1` 뒤).

| ProductId | DisplayName | bForSale | Price | PlacementDefinition | ItemBoxDefinition·Icon |
|---|---|---|---|---|---|
| `ClothesLocker4` | `4칸 락커` | true | 4.7절 "락커 상품 가격" 행의 4칸 | `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_4` | 비움 |
| `ClothesLocker8` | `8칸 락커` | true | 같은 행 8칸 | `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_8` | 비움 |

- 두 정의에 `Facility.Discardable` 태그가 없고 `LockerSlotCount`가 4·8인지 확인한다(`.md/Unreal/PlacementSystem.md`). Catalog Data Validation 오류 0, 저장·재로드.

### 3.10 확인만

- `ExpansionAuthority`(Definition 연결, `Initial Tier Index` 0)와 `KeyRack`(`Pair Transforms` 개수 ≥ `Tiers` 모든 줄 열쇠 수의 최대) Data Validation 오류 0을 확인한다(저장 안 함).

### 3.11 자동화 확인

- `BathhouseSim.Expansion.*`(content 계약 포함), `BathhouseSim.Computer.Input.ScreenWheelContentContract`, `BathhouseSim.Placement.PreviewHiddenWithoutAim`을 실행한다. Editor 작업 전 상태에서 실패가 예상되는 항목은 [PROMPT_REVIEW.md](PROMPT_REVIEW.md) 8절에 있다. 작업 뒤에는 모두 통과해야 한다.

## 4. 멈춰야 하는 조건

- 끝 모습 띠에 Level Actor가 있어 옮겨야 할 때, 공간 Data Validation이 새 오류(`ExpansionOverlap`, `ExpansionOutsideOpeningBlocked`, `ExpansionNavWorkCovered` 등)를 낼 때, `Tiers`·열쇠걸이 자리 수를 바꿔야 할 때, 새 asset이 필요해질 때는 저장하지 않고 멈춰 보고한다. 영향이 없는 allowlist 항목은 저장·재로드까지 마친다.
- 상위 단계 결함(C++ 계약과 asset 불일치)을 Blueprint graph·asset 값으로 우회하지 않는다.

## 5. 사용자 PIE 관찰 항목 (마스터 `PIE_CHECKLIST.md` 입력)

| ID | 관찰 | 기대 |
|---|---|---|
| EXP-021 | 확장 탭을 연다 | 탭 머리에 "현재 확장 단계"가 없고 선택지마다 `확장 단계 0/2`, 전후 크기, 그 공간 1번째 가격(공간마다 다름), 홀은 효과 문구. 구입 버튼은 꺼져 있고 `확장 구입`(가격 없음) |
| EXP-022 | 홀 선택 후 잔액을 홀 1번째 가격보다 낮게(Editor 값 조정) | 버튼 꺼짐, `N원 부족`(홀 가격 기준). 잔액이 오르면 즉시 켜짐. 선택 전에는 부족액 없음 |
| EXP-023 | 홀을 확인해 구입 | 홀 1번째 가격 1회 차감, 남·북 벽이 동시에 물러남, 서쪽 출입구 벽·동쪽 통로 벽 그대로, 열쇠 4번, `확장 완료`, `설치된 락커 칸 2/4`, 홀 `확장 단계 1/2`·홀 2번째 가격, 다른 공간 `0/2` |
| EXP-040·041·047 | 목욕공간(남·북·동), 작업공간(동·북) 구입 | 여러 벽이 한 번에, 모서리까지 바닥·천장 직사각형 하나. 목욕공간은 모서리 포함 늘어난 바닥에 물 얼룩 조각(손님이 있는 곳에만 생겨 시간이 걸림), 작업공간은 조각 없음. 열쇠·한도 그대로. 가격 1회. 목욕 욕탕 미리보기가 모서리까지 초록 |
| EXP-042 | 목욕 1회·홀 1회 뒤 홀 구입 | 홀 2번째 가격, 열쇠 5~8, 한도 8칸, 홀 `2/2` |
| EXP-043·044 | 홀만 `2/2`, 이어서 모두 `2/2` | 홀만 `이 공간은 더 넓힐 수 없습니다`·다른 공간은 1번째 가격으로 구입 가능. 모두 상한일 때만 `최대 확장 단계입니다` |
| EXP-045·048 | Editor에서 저장하지 않고 값을 바꾼 뒤 PIE(끝나면 되돌림) | 홀 줄 3개 + `Tiers` 4줄이면 홀 3번 구입, 매번 홀의 그 번째 가격. 북 2m·남 6m는 각각 적용되고 열쇠·한도 1회. 오류·경고는 `Space_Hall` 저장 또는 우클릭 Validate로 본다 |
| EXP-046 | 4·8칸 락커 구입·배송·개봉·운반·설치 | 설치 칸 수가 4·8 오른다. 홀 2회(한도 8칸)에서 1칸 락커 2개 설치 시 4칸은 설치(6/8), 8칸은 한도로 막힘. 8칸 설치는 1칸 락커를 Q 길게 눌러 회수한 뒤 |
| EXP-050·051 | 아이템을 든 채 거리 밖, 계단 쪽 벽, 천장, 출입구 밖 지형, 하늘을 차례로 본다 | 빨간 잔상 없이 미리보기가 사라지고, 돌아오면 그 자리에 즉시 보이고 회전 유지. 좌클릭해도 설치되지 않고 안내 문구. 아이템·격자 그대로 |
| EXP-052 | 공간 불허·한도·겹침 자리 조준 | 조준한 자리에 빨간 미리보기와 이유 문구(숨지 않음) |
| 회귀 | EXP-024·025·027·028·029 | 기존대로 |
| 카드 | 카드 문구(`확장 단계`, `다음 넓힘`, 홀 효과 두 줄)가 1024×576 화면에서 잘리지 않는가 | Editor가 눈으로 확인하지 못한 경우 |
| 손님 | 넓어진 남·북·동 바닥까지 손님이 다니는가(몇 초 뒤, Recast Dynamic) | 다닌다 |

가격 합이 크다. 필요하면 PlayerState Blueprint `StartingMoney`를 저장하지 않고 임시로 올린다(U2와 같음).

## 6. 갱신할 `.md/Unreal/*System.md`

- `Unreal/BuildingSystem.md`: 넓힘 줄 형식(`Sides`·`Price`), 방향, 가격 원본, 줄 수 = 공간별 상한, "합이 Max Purchase Count 이상" 문장 삭제, Data Validation 결과.
- `Unreal/FacilitySystem.md`: 확장 정의 필드(`Tiers`만), 열쇠걸이 검사 범위(효과 표 모든 줄).
- `Unreal/InteractionUISystem.md`: 확장 화면 binding(`StageText`·`PriceText`가 선택지 카드로 이동).
- `Unreal/ShopSystem.md`: 4·8칸 락커 상품.
- `Unreal/WorldSystem.md`: Nav 범위.

## 7. Blueprint에서 구현하면 안 되는 C++/domain 로직

- 가격·상한·방향 판정, 구입 transaction, 표시 문구 계산, 선택·확인 상태, 미리보기 숨김 판정은 모두 C++다. WBP는 hierarchy·layout·style·asset 연결만 한다. WBP graph, Tick, timer, focus 호출을 만들지 않는다.
