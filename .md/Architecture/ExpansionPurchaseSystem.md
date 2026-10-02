# Expansion Purchase System

## Status And Scope

- 2026-10-02 `EXP-U2`(확장 구입 수직, 홀 1회) 설계, Source 반영(Editor 작업 전, 빌드·자동화 통과). 입력은 `.md/Work/EXPANSION-PURCHASE/PROMPT_ARCHITECTURE.md`(EXP-020~032)이고 구현 지시는 `.md/Work/EXPANSION-PURCHASE/EXP-U2/PROMPT_IMPLEMENTATION.md`다.
- 사용자 결정 D2(전체 상한 삭제, 공간별·넓힘별 가격)는 **D2 설계, Source 미반영, 다음 단위에서 구현**이다. 아래 D2 Redesign 절에 따로 두며 본문은 현재 구현이다.
- 컴퓨터 `확장` 탭에서 돈을 내고 공간 하나를 넓힌다. 홀을 넓히면 열쇠 수와 락커 칸 설치 한도가 오른다. 상점은 락커 상품을 팔 수 있다.
- 공간 넓힘의 형상·검증·편집 미리보기는 [BuildingSystem.md](BuildingSystem.md) Expansion 절이 정본이다. 이 문서는 구입 상태·transaction·확장 데이터·확장 탭·락커 판매 규칙을 다룬다.
- 저장·환불·되돌리기·공사 시간은 없다. 게임을 다시 시작하면 확장 0회다.

## Source Scope

```text
Public/Building/
  BathhouseExpansionTypes.h                 구입 실패 코드, 화면 view struct(non-reflected)
  BathhouseExpansionPurchaseSubsystem.h     UBathhouseExpansionPurchaseSubsystem : UWorldSubsystem
Private/Building/
  BathhouseExpansionPurchaseSubsystem.cpp
Public|Private/Facility/
  BathhouseExpansionDefinition.*            가격·전체 상한·홀 효과 표(기존 asset class 확장)
  BathhouseExpansionAuthority.*             홀 효과 tier 상태(기존), Definition getter·검증
Public|Private/Interaction/
  BathhouseKeyRackActor.*                   tier만큼 열쇠 생성(기존), 열쇠 수 검증
Public|Private/UI/
  ComputerScreenRootWidget.*                탭 3개
  ExpansionScreenWidget.*, ExpansionSpaceOptionWidget.*
Private/UI/
  ExpansionScreenModel.h/.cpp               순수 표시 모델
Private/Shop/
  ShopProductRules.cpp                      락커 상품 허용 규칙
```

`Building` 폴더 아래 하위 영역이다([CoreSystem.md](CoreSystem.md) System Documents).

## Ownership

| 대상 | 상태·authoring owner | 실행 owner |
|---|---|---|
| 공간별 넓힌 횟수(runtime) | 각 `ABathhouseSpaceActor::AppliedExpansionCount`(Transient) | 구입 transaction만 바꾼다 |
| 공간별 넓힘 방향·양·상한 | 공간 Level instance `ExpansionSteps`(줄 수 = 상한) | [BuildingSystem.md](BuildingSystem.md) |
| 전체 구입 횟수 | 저장하지 않음. 등록된 공간의 넓힌 횟수 합 | subsystem 계산 |
| 구입 가격·전체 상한·홀 효과 표 | `UBathhouseExpansionDefinition`(`DA_BathhouseExpansion_Default`) | 읽기 전용 |
| 홀 효과 tier(열쇠 수·락커 한도) | `ABathhouseExpansionAuthority` 현재 tier | transaction이 `TryAdvanceToTier` |
| 열쇠 Actor | `ABathhouseKeyRackActor` | tier 변경 delegate |
| 돈 | `UPlayerWalletComponent`([EconomySystem.md](EconomySystem.md)) | transaction이 `TrySpendMoney` |
| 구입 transaction, 공간 등록부, 화면 view | 없음(transient guard만) | `UBathhouseExpansionPurchaseSubsystem` |
| 탭 선택, 공간 선택, 확인 대기, 완료 표시 | 화면 widget 표시 상태 | `UComputerScreenRootWidget`, `UExpansionScreenWidget` |
| 락커 상품 가격 | `DA_ShopCatalog` 상품 줄 | [ShopSystem.md](ShopSystem.md) |

## Expansion Data

- `UBathhouseExpansionDefinition`
  - 기존 `Tiers`(`KeyPoolSize`, `MaxInstalledLockerSlots`)는 **홀 넓힘 횟수별 효과 표**다. index = 홀 넓힌 횟수(0회부터), 표 끝을 넘으면 마지막 줄. 이름은 asset 호환을 위해 유지한다.
  - `MaxPurchaseCount`: 전체 구입 횟수 상한. `PurchasePrices`: index k = k+1번째 구입 가격(고른 공간과 무관).
  - `ValidatePurchaseData` 규칙(runtime 사용 가능 판정과 Data Validation 공용, 모두 오류): 표 비어 있음, 기존 tier 규칙(열쇠 ≥ 한도, 줄마다 줄지 않음), 가격 줄 < 상한, 상한 안 가격 ≤ 0, 효과 줄 < 상한 + 1. 상한보다 긴 줄은 허용.
  - C++ API: `GetMaxPurchaseCount`, `TryGetPurchasePrice`, `GetHallEffect(HallCount)`, `GetHallEffectIndex(HallCount)`.
- `ABathhouseExpansionAuthority`: `GetExpansionDefinition()`. Data Validation 오류: Definition 없음, `InitialTierIndex ≠ 0`(시작은 홀 0회). tier 상태·`TryAdvanceToTier`·`OnExpansionTierChanged`는 기존 그대로이며 구입 transaction만 호출한다.
- 불변식: Authority 현재 tier = `GetHallEffectIndex(홀 넓힌 횟수)`. subsystem이 매번 확인하고 어긋나면 구입을 막는다(`Unavailable`).
- `ABathhouseKeyRackActor` Data Validation: 같은 world Authority Definition의 도달 가능한 효과 줄(index `0..Min(MaxPurchaseCount, Tiers.Num()-1)`) 최대 열쇠 수 > `PairTransforms.Num()`이면 오류. runtime 생성은 기존(`min(열쇠 수, 자리 수)`까지 추가, 넘으면 Error 로그).

## Purchase Subsystem

`UBathhouseExpansionPurchaseSubsystem : UWorldSubsystem`(C++ 전용 API).

- 공간 등록부: 공간 Actor가 game world BeginPlay에 `RegisterSpace`, EndPlay에 `UnregisterSpace`. 종류마다 하나이며 두 번째는 `LogBathhouseExpansion` Error 후 무시.
- `GetPurchaseCount()` = 등록 공간 넓힌 횟수 합.
- `BuildView(Buyer)` → `FBathhouseExpansionView`: 사용 가능 여부, transaction 중(`bBusy`), 구입 횟수·상한·최대 도달, 다음 가격, 잔액·부족액, 설치 락커 칸·한도, 공간별 선택지(현재·다음 안쪽 크기, 넓힐 수 있음, 홀 효과 전후 열쇠·한도).
  - 사용 가능 조건: Authority 등록, Definition `ValidatePurchaseData` 통과, 등록 공간 ≥ 1, Buyer wallet 있음, tier 불변식. 원인별 Error 로그 1회.
- `EvaluatePurchase(Buyer, Kind, ExpectedPurchaseCount, OutPrice)` 실패 순서: `Busy` → `Unavailable` → `StaleState`(기대 횟수 ≠ 현재) → `MaxPurchasesReached` → `SpaceUnavailable` → `SpaceMaxReached`(공간 `CanApplyNextExpansion` 실패) → `InsufficientMoney` → 홀 다음 tier 사전 검사(`Unavailable`).
- `TryPurchase(Buyer, Kind, ExpectedPurchaseCount)` 동기 transaction:

| 순서 | 동작 | 실패 시 |
|---|---|---|
| 1 | 재진입 guard, `EvaluatePurchase` | 그 코드, 변화 없음 |
| 2 | 공간 `ApplyNextExpansion(Undo)`(횟수 +1, 구역·형상·띠 조각) | 공간이 스스로 되돌림, `ApplyFailed` |
| 3 | `Wallet->TrySpendMoney(Price)` | `UndoExpansion`, `InsufficientMoney` |
| 4 | 홀이고 새 효과 index > 현재 tier면 `Authority->TryAdvanceToTier` | Error 로그, 환불, `UndoExpansion`, `ApplyFailed`(정상 데이터에서 도달하지 않음) |
| 5 | guard 해제 후 `OnExpansionChanged` 1회 | — |

- 되돌릴 수 없는 tier 상승(열쇠 생성)을 마지막에 둔다. 사전 검사가 2·4단계 성공을 보장한다.
- `ExpectedPurchaseCount`가 같은 확인에서 온 두 번째 요청을 거절한다(연타 1회 결제).
- 구현 상세: 사용 가능 판정(`Resolve`)은 transaction 도중(`bPurchasing`)에는 tier 일관성을 확인하지 않고(공간 횟수와 tier가 잠시 어긋남), Authority 변경 방송은 transaction 안에서는 전달하지 않는다(commit 방송 한 번으로 묶음). Buyer wallet이 없는 평가는 정상 상태(사용자 없음, BeginPlay 순서)라 다른 원인 확인 전에 로그 없이 사용 불가로 끝낸다. 자동화 실패 주입(4·5·6단계)은 `WITH_DEV_AUTOMATION_TESTS` 안의 private 값이다.
- `OnExpansionChanged`(native)는 구입 commit, 공간 등록·해제, Authority 등록 변경(`UBathhouseFacilitySubsystem::OnExpansionAuthorityChanged` 전달) 때 방송한다.
- 3단계 wallet `OnMoneyChanged` 동기 callback에서 `BuildView`는 `bBusy=true`를 돌려주고, `TryPurchase` 재호출은 `Busy`다.

## Computer Expansion Tab

[UISystem.md](UISystem.md) Native Widget Policy를 따른다.

- `UComputerScreenRootWidget`: 탭 enum `Management·Shop·Expansion`(기본 Management, Switcher index 0·1·2). 신규 `ExpansionTabButton`, `ExpansionScreen`은 `BindWidgetOptional`(기존 `WBP_ComputerScreenRoot`가 구현~Editor 단계 사이에 compile 오류가 되지 않게 함, content 자동화가 존재를 확인). Expansion 탭을 떠나면 확인 대기를 취소한다. `NotifyComputerUseEnded()`도 확인 대기만 취소하고 나머지 화면 상태는 유지한다.
- `IComputerScreenContextReceiver::NotifyComputerUseEnded()`(기본 빈 구현): `ABathhouseComputerActor::ReleaseReservation`이 실제로 사용자를 지울 때 호출한다([ComputerSystem.md](ComputerSystem.md)).
- `UExpansionScreenWidget`(Abstract): 필수 BindWidget `StageText`, `BalanceText`, `LockerText`, `OptionsPanel`, `HallOption`·`BathOption`·`WorkOption`(`UExpansionSpaceOptionWidget`), `PriceText`, `PurchasePanel`, `PurchaseButton`, `PurchaseButtonText`, `ConfirmPanel`, `ConfirmButton`, `CancelButton`, `ShortfallText`, `ResultText`, `MessageText`.
  - 표시 상태: 선택 공간, 확인 대기와 그때의 구입 횟수, 완료 표시. domain 값은 보관하지 않는다.
  - 구독: 사용자 wallet `OnMoneyChanged`, `ULockerCapacitySubsystem::OnLockerCapacityChanged`, subsystem `OnExpansionChanged`. 이벤트마다 `BuildView`를 받아 `bBusy`가 아니면 모델 결과를 적용한다. Tick 없음.
  - 확인은 확인 대기일 때만 대기를 먼저 내리고 `TryPurchase(사용자, 선택, 확인을 연 순간 구입 횟수)`를 부른다. 성공하면 선택 해제와 `확장 완료`.
- `UExpansionSpaceOptionWidget`(Abstract): 필수 BindWidget `SelectButton`, `NameText`, `SizeText`, `EffectText`, `StatusText`, `SelectionHighlight`. 종류는 parent가 지정하고 클릭은 C++ delegate로 parent에 알린다.
- `FExpansionScreenModel`(순수): view + 표시 상태 → 문구·활성·가시성. 문구(LOCTEXT): `현재 확장 단계: {N}`, `잔액 {N}원`, `설치된 락커 칸 {a}/{b}`, `{X}m×{Y}m → {X}m×{Y}m`(world X 먼저, 소수 최대 1자리), `열쇠 {a}개 → {b}개` / `락커 칸 한도 {a}칸 → {b}칸`(홀만), `이 공간은 더 넓힐 수 없습니다`, `이번 구입 가격 {N}원`, `확장 구입 ({N}원)`, `{N}원 부족`, `확장 완료`, `최대 확장 단계입니다`, `확장을 사용할 수 없습니다`. 공간 이름은 LOCTEXT 표(UENUM DisplayName은 game build에 없음). WBP 고정 문구는 `정말 구입할까요?`, `확인`, `취소`, 탭 `확장`.
- WBP: `/Game/Bathhouse/UI/WBP_ExpansionScreen`, `WBP_ExpansionSpaceOption`, `WBP_ComputerScreenRoot` Switcher child [2]. 1024×576. `ConfirmPanel`은 `PurchasePanel` 자리이되 확인 버튼이 구입 버튼 위치에 오지 않는다. 스크롤 영역을 만들면 Computer Screen Wheel Scroll 계약을 따른다.

## Locker Products

- `FShopProductRules::ValidateDefinitions`: 설비 상품은 `LockerSlotCount == 0`이면 `Facility.Discardable` 필수, `> 0`이면 그 태그 금지. 나머지 상점 경로는 그대로다([ShopSystem.md](ShopSystem.md)).
- 한도 초과 구매는 막지 않고 설치만 기존 `ULockerCapacitySubsystem::CanInstallLockerSlots`가 막는다. 홀 구입으로 tier가 오르면 다음 미리보기 검증부터 설치된다.
- `EXP-U2`는 `DA_ShopCatalog`에 1칸 락커 줄만 추가한다. 4·8칸은 같은 규칙으로 `EXP-U3`가 데이터만 추가한다.

## Dependencies

- Building → Facility(`UBathhouseFacilitySubsystem`, Authority, Definition, `ULockerCapacitySubsystem`), Economy(wallet). Facility·Economy는 Building을 모른다.
- UI → Building(subsystem·view type), Economy, Facility(락커 용량 delegate), Computer(context interface).
- Computer는 확장을 모른다. 화면에 사용 종료만 알린다.
- Interaction(열쇠걸이) → Facility(Authority, Definition) 기존 방향.
- 새 module 없음.

## Blueprint/API Contracts

- 신규 reflected: Definition `MaxPurchaseCount`·`PurchasePrices`, root `ExpansionTabButton`·`ExpansionScreen`(optional), 두 확장 widget class와 BindWidget, subsystem class.
- rename·삭제 없음, Core Redirect 불필요. 모두 property 추가라 기존 export와 호환된다.
- Editor: `DA_BathhouseExpansion_Default` 새 필드, `DA_ShopCatalog` 1칸 락커 줄, 새 WBP 2개와 root 탭. 원본 위치는 `.md/Unreal/`이 기록한다.

## D2 Redesign (D2 설계, Source 미반영, 다음 단위에서 구현)

2026-10-02 사용자 결정 D2(상위 계약 3절 D2 행, 커밋 `ef4d1db`). U2는 D2 없이 사용자 PIE 승인됐으므로 **이 절 위의 모든 내용이 현재 구현**이다. 아래는 다음 단위(U3 묶음)에서 구현할 설계이며 상세는 `.md/Work/EXPANSION-PURCHASE/EXP-U2/PROMPT_IMPLEMENTATION.md` 19절이다. 구현되면 위 본문을 이 절대로 바꾼다.

| 항목 | 현재 구현 | D2 설계 |
|---|---|---|
| 가격 | `UBathhouseExpansionDefinition::PurchasePrices`(몇 번째 전체 구입별) | 공간 넓힘 줄 `FBathhouseSpaceExpansionStep::Price`(그 공간의 몇 번째 넓힘별). 공간 `GetNextExpansionPrice()` |
| 상한 | `MaxPurchaseCount`(전체)와 공간 줄 수 | 공간 줄 수만. `MaxPurchaseCount`·`PurchasePrices`와 `GetMaxPurchaseCount`·`TryGetPurchasePrice` 삭제(tagged property라 옛 asset load 안전, redirect 불필요, Editor 재저장) |
| Definition 규칙 | 가격 줄 ≥ 상한, 가격 > 0, 효과 줄 ≥ 상한 + 1, tier 규칙 | tier 규칙만. 효과 줄 ≥ 홀 넓힘 줄 + 1은 공간 world 검증(`ExpansionHallEffectShort`) |
| 열쇠걸이 검증 | 도달 가능 줄(상한까지) | `Tiers` 모든 줄 |
| view | `PurchaseCount`, `MaxPurchaseCount`, `bMaxReached`, `NextPrice`, `Shortfall` | 위 필드 삭제, `bAllAtLimit`. 선택지마다 `AppliedCount`, `StepCount`, `bAtLimit`, `NextPrice` |
| 평가·구입 | `ExpectedPurchaseCount`(전체), `MaxPurchasesReached` | `ExpectedAppliedCount`(고른 공간 넓힌 횟수), `MaxPurchasesReached` 삭제, 가격 = 고른 공간 다음 줄 `Price`. transaction 순서는 같다 |
| 화면 | `현재 확장 단계: N`, 화면 `이번 구입 가격`, 최대 = 전체 상한 | 선택지 `확장 단계 N/M`·`다음 넓힘 N원`(optional `StageText`·`PriceText`), 버튼 가격·부족액은 고른 공간 기준(선택 전 없음), 최대 = 등록된 모든 공간 상한. 화면 `StageText`·`PriceText` binding 삭제 |

## Verification

- 자동화: transaction 성공(돈 1회·고른 공간만·홀 tier/열쇠/한도·방송 1회), 실패 코드별 무변화, 강제 실패 주입 되돌림, 재진입 `Busy`, `StaleState`, Definition·Authority·열쇠걸이 검증, 락커 상품 규칙, 0회 한도 거부 → 홀 구입 뒤 설치, 표시 모델 상태 표, 확인 취소 경로(탭·사용 종료·취소), content 계약(Editor 뒤).
- 사용자 PIE: EXP-020~032, 대표 EXP-023 → EXP-028.
