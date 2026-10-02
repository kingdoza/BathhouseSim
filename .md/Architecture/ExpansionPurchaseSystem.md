# Expansion Purchase System

## Status And Scope

- 2026-10-02 `EXP-U2`(확장 구입 수직, 홀 1회) 설계·구현, 사용자 PIE 통과·병합(`3c17e41`).
- 2026-10-02 `EXP-U3`(전체 확장 묶음) 설계·구현, 사용자 PIE 통과: 사용자 결정 D2(전체 상한 삭제, 공간별·넓힘별 가격, 선택지별 단계 표시)와 D3(넓힘 한 번에 여러 벽)를 본문에 합쳤다. 구현 지시는 병합 커밋의 `.md/Work/EXPANSION-PURCHASE/EXP-U3/PROMPT_IMPLEMENTATION.md`(Git 이력)다.
- `EXPANSION-PURCHASE` 완료(2026-10-02, U3 병합 `201b2d0`). 작업 폴더는 제거됐고, 이 문서의 `.md/Work/EXPANSION-PURCHASE/…` 경로는 해당 병합 커밋 이력에서 읽는다.
- 입력 계약은 `.md/Work/EXPANSION-PURCHASE/PROMPT_ARCHITECTURE.md`(EXP-020~032, EXP-040~048)다.
- 컴퓨터 `확장` 탭에서 돈을 내고 공간 하나를 넓힌다. 공간마다 그 공간의 넓힘 줄 수까지 넓힐 수 있고 가격은 그 공간의 몇 번째 넓힘인지로 정한다. 홀을 넓히면 열쇠 수와 락커 칸 설치 한도가 오른다. 상점은 1·4·8칸 락커를 판다.
- 공간 넓힘의 형상·검증·편집 미리보기는 [BuildingSystem.md](BuildingSystem.md) Expansion 절이 정본이다. 이 문서는 구입 상태·transaction·확장 데이터·확장 탭·락커 판매 규칙을 다룬다.
- 저장·환불·되돌리기·공사 시간은 없다. 게임을 다시 시작하면 모든 공간이 확장 0회다.

## Source Scope

```text
Public/Building/
  BathhouseExpansionTypes.h                 구입 실패 코드, 화면 view struct(non-reflected)
  BathhouseExpansionPurchaseSubsystem.h     UBathhouseExpansionPurchaseSubsystem : UWorldSubsystem
Private/Building/
  BathhouseExpansionPurchaseSubsystem.cpp
Public|Private/Facility/
  BathhouseExpansionDefinition.*            홀 효과 표(Tiers)
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
| 공간별 넓힘 벽·벽별 양·가격·상한 | 공간 Level instance `ExpansionSteps`(줄 = 넓힘 한 번: `Sides`·`Price`, 줄 수 = 상한) | [BuildingSystem.md](BuildingSystem.md) |
| 전체 구입 횟수·전체 상한 | 없음(D2) | — |
| 홀 효과 표 | `UBathhouseExpansionDefinition::Tiers`(`DA_BathhouseExpansion_Default`) | 읽기 전용 |
| 홀 효과 tier(열쇠 수·락커 한도) | `ABathhouseExpansionAuthority` 현재 tier | transaction이 `TryAdvanceToTier` |
| 열쇠 Actor | `ABathhouseKeyRackActor` | tier 변경 delegate |
| 돈 | `UPlayerWalletComponent`([EconomySystem.md](EconomySystem.md)) | transaction이 `TrySpendMoney` |
| 구입 transaction, 공간 등록부, 화면 view | 없음(transient guard만) | `UBathhouseExpansionPurchaseSubsystem` |
| 탭 선택, 공간 선택, 확인 대기, 완료 표시 | 화면 widget 표시 상태 | `UComputerScreenRootWidget`, `UExpansionScreenWidget` |
| 락커 상품 가격 | `DA_ShopCatalog` 상품 줄 | [ShopSystem.md](ShopSystem.md) |

## Expansion Data

- 넓힘 가격은 공간 넓힘 줄 `FBathhouseSpaceExpansionStep::Price`다(그 공간의 k번째 넓힘 가격, 전체 구입 순번과 무관). 공간 Actor `GetNextExpansionPrice()`가 다음 줄 가격(상한이면 0), `IsAtExpansionLimit()`가 상한 도달을 준다. 가격 ≤ 0은 공간 검증 오류이고 `CanApplyNextExpansion`이 거부한다.
- `UBathhouseExpansionDefinition`
  - `Tiers`(`KeyPoolSize`, `MaxInstalledLockerSlots`) = **홀 넓힘 횟수별 효과 표**. index = 홀 넓힌 횟수(0회부터), 표 끝을 넘으면 마지막 줄. 이름은 asset 호환을 위해 유지한다. 줄 수가 홀 넓힘 줄 수 + 1 이상인지는 DA가 Level을 모르므로 공간 검증 `ExpansionHallEffectShort`(owner 홀)가 본다.
  - 전체 상한·구입 순번 가격 필드는 없다(D2로 삭제, deprecated로 두지 않음).
  - `ValidatePurchaseData`(runtime 사용 가능 판정과 Data Validation 공용, 모두 오류): 표 비어 있음, 열쇠 ≥ 한도, 줄마다 줄지 않음.
  - C++ API: `GetHallEffect(HallCount)`, `GetHallEffectIndex(HallCount)`, `GetTier`.
- `ABathhouseExpansionAuthority`: `GetExpansionDefinition()`. Data Validation 오류: Definition 없음, `InitialTierIndex ≠ 0`. tier 상태·`TryAdvanceToTier`·`OnExpansionTierChanged`는 기존이며 구입 transaction만 호출한다.
- 불변식: Authority 현재 tier = `GetHallEffectIndex(홀 넓힌 횟수)`. subsystem이 매번 확인하고 어긋나면 구입을 막는다(`Unavailable`). 여러 벽이 물러나도 홀 줄 하나 = 홀 넓힘 한 번이다(D3).
- `ABathhouseKeyRackActor` Data Validation: 같은 world Authority Definition의 `Tiers` **모든 줄** 최대 열쇠 수 > `PairTransforms.Num()`이면 오류(열쇠걸이가 Building의 홀 줄 수를 알지 않게 함). runtime 생성은 기존.

## Purchase Subsystem

`UBathhouseExpansionPurchaseSubsystem : UWorldSubsystem`(C++ 전용 API).

- 공간 등록부: 공간 Actor가 game world BeginPlay에 `RegisterSpace`, EndPlay에 `UnregisterSpace`. 종류마다 하나이며 두 번째는 `LogBathhouseExpansion` Error 후 무시.
- `BuildView(Buyer)` → `FBathhouseExpansionView`: 사용 가능 여부, transaction 중(`bBusy`), `bAllAtLimit`(등록 공간 ≥ 1이고 모두 `IsAtExpansionLimit`), 잔액, 설치 락커 칸·한도, 공간별 선택지 `FBathhouseExpansionOptionView`(있음, `bCanExpand` = `CanApplyNextExpansion`, `AppliedCount`, `StepCount`, `bAtLimit`, `NextPrice`(넓힐 수 있을 때만), 현재·다음 안쪽 크기(다음 = 다음 줄의 모든 벽 적용 결과), 홀 효과 전후 열쇠·한도).
  - 사용 가능 조건: Authority 등록, Definition `ValidatePurchaseData` 통과, 등록 공간 ≥ 1, Buyer wallet 있음, tier 불변식. 원인별 Error 로그 1회.
- `EvaluatePurchase(Buyer, Kind, ExpectedAppliedCount, OutPrice)` 실패 순서: `Busy` → `Unavailable` → `SpaceUnavailable` → `StaleState`(기대 횟수 ≠ 고른 공간 넓힌 횟수) → `SpaceMaxReached`(`CanApplyNextExpansion` 실패) → `InsufficientMoney`(고른 공간 `GetNextExpansionPrice()` 기준) → 홀 다음 tier 사전 검사(`Unavailable`).
- `TryPurchase(Buyer, Kind, ExpectedAppliedCount)` 동기 transaction:

| 순서 | 동작 | 실패 시 |
|---|---|---|
| 1 | 재진입 guard, `EvaluatePurchase` | 그 코드, 변화 없음 |
| 2 | 공간 `ApplyNextExpansion(Undo)`(횟수 +1, 다음 줄의 모든 벽을 한 번에, 구역·형상·띠 조각) | 공간이 스스로 되돌림, `ApplyFailed` |
| 3 | `Wallet->TrySpendMoney(고른 공간 다음 줄 Price)` | `UndoExpansion`, `InsufficientMoney` |
| 4 | 홀이고 새 효과 index > 현재 tier면 `Authority->TryAdvanceToTier` | Error 로그, 환불, `UndoExpansion`, `ApplyFailed`(정상 데이터에서 도달하지 않음) |
| 5 | guard 해제 후 `OnExpansionChanged` 1회 | — |

- 되돌릴 수 없는 tier 상승(열쇠 생성)을 마지막에 둔다. 사전 검사가 2·4단계 성공을 보장한다.
- `ExpectedAppliedCount`(확인을 연 순간 고른 공간의 넓힌 횟수)가 같은 확인의 두 번째 요청과, 확인 뒤 가격이 바뀐 요청을 거절한다(연타 1회 결제).
- 효과 표가 홀 줄 수보다 짧은 잘못된 데이터에서는 index clamp로 tier가 오르지 않은 채 구입된다(계약 4.7 "표 끝을 넘으면 마지막 값"). 데이터 오류는 공간 검증이 알린다.
- 구현 상세: 사용 가능 판정(`Resolve`)은 transaction 도중(`bPurchasing`)에는 tier 일관성을 확인하지 않고, Authority 변경 방송은 transaction 안에서는 전달하지 않는다(commit 방송 한 번으로 묶음). Buyer wallet이 없는 평가는 로그 없이 사용 불가로 끝낸다. 자동화 실패 주입(4·5·6단계)은 `WITH_DEV_AUTOMATION_TESTS` 안의 private 값이다.
- `OnExpansionChanged`(native)는 구입 commit, 공간 등록·해제, Authority 등록 변경(`UBathhouseFacilitySubsystem::OnExpansionAuthorityChanged` 전달) 때 방송한다.
- 3단계 wallet `OnMoneyChanged` 동기 callback에서 `BuildView`는 `bBusy=true`를 돌려주고, `TryPurchase` 재호출은 `Busy`다.

## Computer Expansion Tab

[UISystem.md](UISystem.md) Native Widget Policy를 따른다.

- `UComputerScreenRootWidget`: 탭 enum `Management·Shop·Expansion`(기본 Management, Switcher index 0·1·2). `ExpansionTabButton`, `ExpansionScreen`은 `BindWidgetOptional`(content 자동화가 존재를 확인). Expansion 탭을 떠나면 확인 대기를 취소한다. `NotifyComputerUseEnded()`도 확인 대기만 취소하고 나머지 화면 상태는 유지한다.
- `IComputerScreenContextReceiver::NotifyComputerUseEnded()`(기본 빈 구현): `ABathhouseComputerActor::ReleaseReservation`이 실제로 사용자를 지울 때 호출한다([ComputerSystem.md](ComputerSystem.md)).
- `UExpansionScreenWidget`(Abstract): 필수 BindWidget `BalanceText`, `LockerText`, `OptionsPanel`, `HallOption`·`BathOption`·`WorkOption`(`UExpansionSpaceOptionWidget`), `PurchasePanel`, `PurchaseButton`, `PurchaseButtonText`, `ConfirmPanel`, `ConfirmButton`, `CancelButton`, `ShortfallText`, `ResultText`, `MessageText`. 화면 단위 단계·가격 글자는 없다(D2).
  - 표시 상태: 선택 공간, 확인 대기와 그때 고른 공간의 넓힌 횟수(`ConfirmAppliedCount`), 완료 표시. domain 값은 보관하지 않는다.
  - 구독: 사용자 wallet `OnMoneyChanged`, `ULockerCapacitySubsystem::OnLockerCapacityChanged`, subsystem `OnExpansionChanged`. 이벤트마다 `BuildView`를 받아 `bBusy`가 아니면 모델 결과를 적용한다. Tick 없음.
  - 확인은 확인 대기일 때만 대기를 먼저 내리고 `TryPurchase(사용자, 선택, ConfirmAppliedCount)`를 부른다. 성공하면 선택 해제와 `확장 완료`.
- `UExpansionSpaceOptionWidget`(Abstract): 필수 BindWidget `SelectButton`, `NameText`, `SizeText`, `EffectText`, `StatusText`, `SelectionHighlight`. `BindWidgetOptional` `StageText`(`확장 단계 N/M`), `PriceText`(`다음 넓힘 N원`)(기존 WBP가 구현~Editor 사이 compile 오류가 되지 않게 함). 종류는 parent가 지정하고 클릭은 C++ delegate로 parent에 알린다.
- `FExpansionScreenModel`(순수): view + 표시 상태 → 문구·활성·가시성.
  - 선택지: `확장 단계 {AppliedCount}/{StepCount}`(공간 없으면 숨김). 넓힐 수 있으면 `{현재} → {다음}`, `다음 넓힘 {N}원`, 고를 수 있음(홀은 효과 문구). 아니면 현재 크기만, 가격 숨김, `이 공간은 더 넓힐 수 없습니다`, 비활성(상한 도달과 데이터 문제 모두).
  - 구입 버튼: 고른 선택지가 고를 수 있으면 `확장 구입 ({그 공간 다음 가격}원)`, 아니면 `확장 구입`. 부족액 `{N}원 부족`은 고른 선택지 가격 기준이며 선택 전에는 없다.
  - 구입 가능 = 사용 가능 && `!bAllAtLimit` && 선택 있음 && 그 선택지 고를 수 있음 && 잔액 ≥ 그 가격. 확인 대기 중 확인 활성도 같은 판정.
  - `bAllAtLimit`: `최대 확장 단계입니다`, 선택지·구입·확인·부족액 숨김, 잔액·락커·완료 보임. 일부 공간만 상한이면 이 문구는 없다. 사용 불가: `확장을 사용할 수 없습니다`, 잔액만 보임.
  - 그 밖의 문구(LOCTEXT): `잔액 {N}원`, `설치된 락커 칸 {a}/{b}`, `{X}m×{Y}m`(world X 먼저, 소수 최대 1자리), `열쇠 {a}개 → {b}개` / `락커 칸 한도 {a}칸 → {b}칸`, `확장 완료`. 공간 이름은 LOCTEXT 표. WBP 고정 문구는 `정말 구입할까요?`, `확인`, `취소`, 탭 `확장`.
- WBP: `/Game/Bathhouse/UI/WBP_ExpansionScreen`, `WBP_ExpansionSpaceOption`, `WBP_ComputerScreenRoot` Switcher child [2]. 1024×576. `ConfirmPanel`은 `PurchasePanel` 자리이되 확인 버튼이 구입 버튼 위치에 오지 않는다. 스크롤 영역을 만들면 Computer Screen Wheel Scroll 계약을 따른다.

## Locker Products

- `FShopProductRules::ValidateDefinitions`: 설비 상품은 `LockerSlotCount == 0`이면 `Facility.Discardable` 필수, `> 0`이면 그 태그 금지. 나머지 상점 경로는 그대로다([ShopSystem.md](ShopSystem.md)).
- 한도 초과 구매는 막지 않고 설치만 기존 `ULockerCapacitySubsystem::CanInstallLockerSlots`가 막는다. 홀 구입으로 tier가 오르면 다음 미리보기 검증부터 설치된다.
- `DA_ShopCatalog` 맨 뒤에 1·4·8칸 락커가 이 순서로 있다(`EXP-U2` 1칸, `EXP-U3` 4·8칸 데이터만 추가, 코드 변경 없음).

## Dependencies

- Building → Facility(`UBathhouseFacilitySubsystem`, Authority, Definition 효과 표, `ULockerCapacitySubsystem`), Economy(wallet). Facility·Economy는 Building을 모른다.
- UI → Building(subsystem·view type), Economy, Facility(락커 용량 delegate), Computer(context interface).
- Computer는 확장을 모른다. 화면에 사용 종료만 알린다.
- Interaction(열쇠걸이) → Facility(Authority, Definition) 기존 방향.
- 새 module 없음.

## Blueprint/API Contracts

- reflected: Definition `Tiers`, root `ExpansionTabButton`·`ExpansionScreen`(optional), 두 확장 widget class와 BindWidget(option `StageText`·`PriceText` optional), subsystem class. 공간 넓힘 struct는 [BuildingSystem.md](BuildingSystem.md).
- `EXP-U3` 삭제: Definition `MaxPurchaseCount`·`PurchasePrices`, 화면 `StageText`·`PriceText` binding. property 삭제라 Core Redirect가 필요 없고 옛 asset load는 안전하다(없는 tagged property 건너뜀). Editor 단계가 재저장한다.
- Editor 원본 위치는 `.md/Unreal/`이 기록한다(`FacilitySystem.md` 확장 정의, `InteractionUISystem.md` 확장 화면, `ShopSystem.md` 락커 상품, `BuildingSystem.md` 넓힘 줄).

## Verification

- 자동화: transaction 성공(돈 1회·고른 공간만·홀 tier/열쇠/한도·방송 1회), 공간별·넓힘별 가격(다른 공간 구입 뒤 홀 2번째 가격), 공간 상한 뒤 다른 공간 구입 가능, `bAllAtLimit`, 실패 코드별 무변화, 강제 실패 주입 되돌림, 재진입 `Busy`, `StaleState`, Definition·Authority·열쇠걸이 검증, 락커 상품 규칙, 0회 한도 거부 → 홀 구입 뒤 설치, 표시 모델 상태 표, 확인 취소 경로, content 계약(Editor 뒤).
- 사용자 PIE: EXP-020~032(U2), EXP-040~048(U3, 대표 EXP-042·047).

