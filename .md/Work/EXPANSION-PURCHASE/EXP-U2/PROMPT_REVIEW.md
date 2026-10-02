# PROMPT_REVIEW — EXP-U2 확장 구입 수직(홀 1회)

- 작업 ID: `EXP-U2`
- 단계: 구현
- 상태: 완료

## 1. 기능 계약과 현재 단계

- 입력: [../PROMPT_ARCHITECTURE.md](../PROMPT_ARCHITECTURE.md)(승인, D1 포함), [PROMPT_IMPLEMENTATION.md](PROMPT_IMPLEMENTATION.md)(완료). 정본은 `.md/Architecture/ExpansionPurchaseSystem.md`와 `BuildingSystem.md` Expansion 절이다.
- 단계: 구현(C++). 시나리오 EXP-020~032, 대표 EXP-023 → EXP-028. Content·Level·Config 변경 없음(Editor 작업은 [PROMPT_UNREAL.md](PROMPT_UNREAL.md)).
- 단계 시작 커밋 `5d08266`, 브랜치 `work/EXP-U2`. 커밋하지 않았다(작업 트리 변경).

## 2. 시나리오별 코드·테스트 연결

테스트 접두는 `BathhouseSim.Expansion.`이다. 기대값은 fixture 입력(`BathhouseExpansionAutomationTestSupport.h` `FBathhouseExpansionTestWorld`, 테스트 파일의 `MakeSnapshots`)이나 `DA` 대신 만든 Definition 값에서 계산한다.

| 시나리오 | 구현 경로 | 자동화 |
|---|---|---|
| EXP-020 탭 순서·기본 탭 | `UComputerScreenRootWidget`(`EComputerScreenTab`, 기본 `Management`, Switcher index = enum), `ExpansionTabButton`(optional) | `UI.ScreenWidgetNative`(root 탭 전환), `Content.ScreenContract`(Switcher child 3개, Editor 뒤) |
| EXP-021 화면 내용 | `UBathhouseExpansionPurchaseSubsystem::BuildView`, `FExpansionScreenModel::Build` | `UI.ScreenModel`(문구·크기 형식·홀 효과), `Purchase.Transaction`(view 값) |
| EXP-022 잔액 부족·돈 도착 | view `Shortfall`, 모델 구입 가능 규칙, 화면이 wallet `OnMoneyChanged` 구독 | `UI.ScreenModel`, `Purchase.FailuresAndRollback`(1원 부족 → 해소), `UI.ScreenWidgetNative` |
| EXP-023 홀 구입 | `TryPurchase`(7.3 순서), `ABathhouseSpaceActor::ApplyNextExpansion`, `TryAdvanceToTier` | `Purchase.Transaction`(돈 1회·`OnMoneyChanged` 1회·열쇠·한도·방송 1회), `World.SpaceRuntime`(구역·shell·조각) |
| EXP-024·025 | 형상 규칙(옛 안쪽 불변, 새 형상은 옛 바깥 밖), 이웃 shell 재생성 없음, Recast Dynamic | `Layout.InteriorBandAndNeighbors`(이웃 `BuildPlan` 불변), `World.SpaceRuntime`(이웃 component identity). 실제 끼임·손님 길은 PIE |
| EXP-026 취소·이탈 | `CancelPendingConfirm`, root `SelectTab`, `NotifyComputerUseEnded`, `ABathhouseComputerActor::ReleaseReservation` 알림 | `UI.ScreenWidgetNative` |
| EXP-027 연타 | 확인 대기 먼저 내림 + `ExpectedPurchaseCount` → `StaleState` | `Purchase.Transaction`, `UI.ScreenWidgetNative`(확인 두 번 = 구입 1회) |
| EXP-028·029 락커 | 상점 규칙 변경만(`FShopProductRules`), 설치 한도 판정은 기존 `CanInstallLockerSlots` | `Locker.LimitAndShopRules`(0회 거부 문구 → 홀 구입 뒤 허용, 규칙 4조합) |
| EXP-030 | 코드 변경 없음(락커 정의에 Discardable 없음, Editor 확인) | `Content` 단계 확인 |
| EXP-031 사용 불가 | `Resolve`(Authority·Definition·공간·tier 일관성) + 원인별 한 번 `LogBathhouseExpansion` Error | `Purchase.FailuresAndRollback`(Definition 무효·tier 불일치·Authority 없음 → `Unavailable`, 돈 불변), `UI.ScreenModel` |
| EXP-032 기존 탭 | `ManagementScreen`·`ShopScreen` 경로 변경 없음 | 전체 Computer·Shop 회귀, `Computer.Input.ScreenWheelContentContract`(Editor 전 통과) |
| 형상·검증(U2 공통) | `ExpandInterior`/`ExpansionBand`/`WithExpansionCount`, `ValidateExpansion` 8개 코드, `ValidateWorld` 0회 복사본 | `Layout.InteriorBandAndNeighbors`, `Validation.Rules`(코드별 양성·음성, 횟수 불변), `Data.DefinitionAuthorityKeyRack` |
| 실패·되돌림 | 주입 4(적용 직후)·5(결제)·6(tier) | `Purchase.FailuresAndRollback`(돈·횟수·tier·열쇠·한도·조각·shell·크기 원상, 재진입 `Busy`, `StaleState`, `SpaceMaxReached`, `MaxPurchasesReached`) |

## 3. 변경 파일과 구현 요약

신규(15): `Public/Building/BathhouseExpansionTypes.h`, `BathhouseExpansionPurchaseSubsystem.h`, `Public/UI/ExpansionScreenWidget.h`, `ExpansionSpaceOptionWidget.h`, `Private/Building/BathhouseExpansionPurchaseSubsystem.cpp`, `BathhouseSpaceExpansion.cpp`, `BathhouseSpaceExpansionLayout.cpp`, `BathhouseSpaceExpansionValidation.cpp`, `Private/UI/ExpansionScreenModel.h/.cpp`, `ExpansionScreenWidget.cpp`, `ExpansionSpaceOptionWidget.cpp`, `Private/Tests/BathhouseExpansionAutomationTestSupport.h`, `BathhouseExpansionAutomationTests.cpp`, `ExpansionScreenAutomationTests.cpp`.

수정(21, Source): 공간 Actor(`.h/.cpp`), `BathhouseSpaceTypes.h`(`FBathhouseSpaceExpansionStep`), `BathhouseSpaceLayout.h`(snapshot 필드·helper 선언), `BathhouseSpaceValidation.h`(문제 코드·`ValidateExpansion`), `BathhouseSpaceWorldValidation.cpp`, `BathhouseSpacePositionSuggestion.cpp`(`ApplyMove`가 `BaseInterior`도 이동), `BathhouseSpaceShellComponent`(미리보기 글자), `BathhouseBuildingSettings.h`, Facility `BathhouseExpansionDefinition`·`Authority`, `BathhouseKeyRackActor`, `ComputerScreenContext.h`(`NotifyComputerUseEnded`), `BathhouseComputerActor.cpp`, `ComputerScreenRootWidget`, `ShopProductRules.cpp`. Architecture 정본은 상태 줄만 갱신(8절).

요약:
- 공간 Actor: `ExpansionSteps`, `EditorPreviewExpansionCount`(editor-only Transient), `AppliedExpansionCount`(Transient). 효과 횟수는 편집 world면 미리보기(clamp), 그 밖이면 적용 횟수. `ApplyNextExpansion`/`UndoExpansion`/`CanApplyNextExpansion`은 새 cpp. `BeginPlay`에서 횟수 0으로 시작, Nav dirty area 한 번, 구입 subsystem 등록. `EndPlay`에서 해제.
- 구입 subsystem(Building): 종류별 weak 등록부, `BuildView`, `EvaluatePurchase`(Busy → Unavailable → StaleState → MaxPurchasesReached → SpaceUnavailable → SpaceMaxReached → InsufficientMoney → 홀 tier 사전 검사), `TryPurchase`(적용 → 결제 → tier 상승 → 방송, 실패는 되돌림). 전체 구입 횟수는 공간 횟수 합이며 어디에도 저장하지 않는다.
- Definition: `MaxPurchaseCount`, `PurchasePrices`, `GetHallEffect`·`GetHallEffectIndex`·`TryGetPurchasePrice`·`ValidatePurchaseData`(runtime과 `IsDataValid` 공용). Authority: `GetExpansionDefinition`, `IsDataValid`(Definition 없음·`InitialTierIndex≠0`). 열쇠걸이: `IsDataValid`(도달 가능한 최대 열쇠 수 > 자리 수).
- UI: 순수 `FExpansionScreenModel`, `UExpansionScreenWidget`(표시 상태만 보관, Tick·timer 없음, 구독 대칭 해제), `UExpansionSpaceOptionWidget`, root 탭 enum 3개. 새 BindWidget은 모두 필수이고 root의 `ExpansionTabButton`·`ExpansionScreen`만 `BindWidgetOptional`.
- 상점: `LockerSlotCount > 0`이면 `Facility.Discardable` 금지, 0이면 필수(문구 두 개).

설계 대비 구현 중 확정한 세부(정본 `ExpansionPurchaseSystem.md`에 반영):
- `ApplyZoneGeometry`가 `FloorSizeCm` 대신 `GetInteriorRect()` 크기로 `ZoneBounds` extent를 정한다(0회에서는 같은 값). 설계 6.2의 "ApplyZoneGeometry는 기존대로 GetInteriorRect()를 쓴다"를 만족시키려면 필요했다(자동화가 잡음).
- `Resolve`는 transaction 도중 tier 일관성을 확인하지 않는다(결제 callback에서 `BuildView`가 불리면 잠시 어긋나 가짜 Error 로그를 남기므로). Authority 변경 방송은 transaction 안에서는 `OnExpansionChanged`로 전달하지 않아 commit 방송 한 번으로 묶는다. Buyer wallet이 없는 상태는 정상(사용자 없음)이라 로그를 남기지 않는다.
- `UBathhouseExpansionDefinition::IsDataValid`의 기존 영어 tier 오류 문구를 한국어로 바꿨다(규칙은 같음, 문구를 참조하는 테스트 없음).
- 자동화 실패 주입(`InjectedFailureStep`)은 `WITH_DEV_AUTOMATION_TESTS` 안 private 값이다.

## 4. 클래스 크기·책임 변화

| 파일 | 전 → 후(줄) | 판단 |
|---|---|---|
| `BathhouseSpaceActor.cpp` / `.h` | 269 → 291 / 99 → 146 | 신규 책임(넓힘 상태·적용)은 새 `BathhouseSpaceExpansion.cpp`(204)에 두고 기존 cpp에는 효과 횟수 연결만 추가 |
| `BathhouseSpaceLayout.cpp` / `BathhouseSpaceValidation.cpp` | 419 / 399 (변경 없음) | 400줄 파일에 새 규칙을 넣지 않음. 새 cpp 두 개(66, 176) |
| `BathhouseSpaceWorldValidation.cpp` | 262 → 281 | 0회 복사본·`ValidateExpansion` 호출·Authority 상한 읽기만 |
| `BathhouseSpaceShellComponent.cpp` | 235 → 268 | 미리보기 글자 component 하나 |
| `ComputerScreenRootWidget.cpp` | → 158 | 탭 enum 3개, 확장 화면 전달, 확인 취소 |
| `BathhouseExpansionPurchaseSubsystem.cpp` | 신규 350 | 구입 조율 단일 책임. 400줄 근처가 되지 않게 view·transaction만(문구·표시 규칙은 UI 모델) |
| `ExpansionScreenWidget.cpp` | 신규 311 | 입력 의도·적용만, 표시 규칙은 `ExpansionScreenModel`(132) |

새 의존: Building → Facility(Authority·Definition·FacilitySubsystem·`ULockerCapacitySubsystem`), Building → Economy(wallet), UI → Building. 순환 없음(Facility·Economy는 Building을 모름). 새 module·plugin·Config 없음(`UTextRenderComponent`는 Engine).

## 5. Blueprint/API/Core Redirect 영향

- 신규 reflected: `FBathhouseSpaceExpansionStep`, 공간 `ExpansionSteps`·`EditorPreviewExpansionCount`·`AppliedExpansionCount`, Definition `MaxPurchaseCount`·`PurchasePrices`, Settings `EditorPreviewLabelWorldSizeCm`(기본 100cm는 C++ 기본값), root `ExpansionTabButton`·`ExpansionScreen`(optional), `UExpansionScreenWidget`·`UExpansionSpaceOptionWidget`과 BindWidget, `UBathhouseExpansionPurchaseSubsystem`.
- rename·삭제 없음 → Core Redirect 불필요. default subobject 추가 없음. 모두 property 추가라 기존 export와 호환된다. 기존 `WBP_ComputerScreenRoot`는 새 빌드에서 compile 오류 없이 로드되고 `Computer.Input.ScreenWheelContentContract`가 통과했다.
- `IComputerScreenContextReceiver::NotifyComputerUseEnded()`는 기본 빈 구현이라 기존 구현을 바꾸지 않았다.

## 6. 빌드와 정적 검증

- 빌드: UE 5.8 `BathhouseSimEditor Win64 Development`, 성공(오류·경고 0). 최종 로그 `Saved/Logs/build_exp_u2_final.log`. BathhouseSim Editor는 꺼져 있었고 BeekeepingSim Editor(PID 30900)는 건드리지 않았다.
- 빌드 시점 Source 식별값: HEAD `5d08266e6c1f38ddbba52a8f5277cfaafda00650`. `git diff HEAD -- Source Config | sha256sum` = `7e070a7d6785ba45e23f350074eb293b03dca26bfc04607a11149c65c7c16488`. 추적되지 않는 신규 15개 Source 파일은 이 diff에 없으므로 `for f in $(git ls-files --others --exclude-standard -- Source | sort); do sha256sum "$f"; done | sha256sum` = `8eaffdd2c2a2ac8a1644da8aadfbe4a84e57ed7673777e92c73cbd150d9f8876`. Config 변경 없음.
- 자동화(headless, `-DDC-ForceMemoryCache`): 필터 `BathhouseSim` 전체 173개 중 통과 172, 실패 1. 실패 1개는 `BathhouseSim.Expansion.Content.ScreenContract`이며 Editor 작업 전이라 새 WBP·카탈로그 줄이 없어서 실패하는 것이 설계다(Editor 작업 뒤 통과해야 함). 신규 `Expansion.*` 9개 통과(`Layout`, `Validation.Rules`, `Data`, `World.SpaceRuntime`, `Purchase.Transaction`, `Purchase.FailuresAndRollback`, `Locker.LimitAndShopRules`, `UI.ScreenModel`, `UI.ScreenWidgetNative`). 기존 Building·Computer·Shop·FacilityPlacement·Economy 등 회귀 모두 통과. 로그 `Saved/Logs/auto_exp_u2_full2.log`, 리포트 `Saved/Automation/Reports/<날짜>/exp_u2_full2`.
- `git diff --check` 깨끗(작업 트리 line ending은 기존처럼 CRLF). focused 검사: 조정값 상수 — 코드의 숫자는 설계가 허용한 예외(cm→m 100, 크기 표기 소수 1자리, 효과 표 clamp, 부동소수 허용 오차)뿐이고 가격·상한·넓힘 양·두께·글자 크기는 모두 원본(Definition, 공간 instance, Settings)에서 읽는다. 자동화 기대값도 fixture에서 계산한다. 실제 user focus 호출(`SetFocus` 계열)·Tick·timer는 확장 화면에 없다.

## 7. 리뷰 중점·전역 영향·미검증

리뷰 중점(지시서 15절 기준):
- 7.3 순서(되돌릴 수 없는 tier 상승이 마지막), 실패 경로 무변화, 방송 횟수(`OnExpansionChanged` 1회, 돈 변화 1회).
- 공간 횟수가 공간 Actor 한 곳이고 전체 횟수는 저장되지 않음.
- 편집 미리보기가 game world에 새지 않음(`GetEffectiveExpansionCount`의 editor-only 분기, shell 글자 component는 Transient·editor-only·hidden in game, `bPreviewChunks`와 같은 world 분기).
- 이웃 shell 재생성 없음, 이미 놓인 Actor 미접촉.
- 화면이 view를 매번 subsystem에서 받고 표시 상태만 보관, delegate 대칭 해제, Tick 없음.
- 400줄 파일에 새 규칙 없음, 새 의존이 4절 목록 안.
- 위 "설계 대비 확정한 세부" 4개.

전역 영향: 공간 형상 재생성은 넓힌 공간 하나의 Static ISM·Movable 조명만이다. `BeginPlay`의 Nav dirty area는 End 모습 범위 한 번이다. Project/World/Input/Collision/Nav 설정 변경 없음.

미검증(통과로 기록하지 않음):
- 편집 world 경로: `EditorPreviewExpansionCount` 분기, 글자 component 생성·방향·크기, `PostEditChangeProperty` clamp, 저장·재로드 후 미리보기 0(자동화로 편집 world를 만들지 않음). Editor 작업 단계가 미리보기로 확인한다.
- 실제 Nav 갱신(Recast Dynamic이 넓힌 바닥을 즉시 덮는지)과 Nav dirty area 호출, 넓히는 순간 손님·물건 끼임(EXP-024·025), HUD 변화량 표시, WBP 배치·클릭·hit test, `ComputerActor::ReleaseReservation` → 화면 알림의 실제 컴퓨터 이탈 흐름(화면 쪽 `NotifyComputerUseEnded`와 root 전달만 자동화함).
- 락커 설치 전 경로: 한도 판정(`CanInstallLockerSlots`)만 자동화했고 상점 카트·주문·배송·개봉·운반·배치·회수로 이어지는 락커 상품 end-to-end와 "설치된 락커 칸 +1", 손님 3명 동시 락커는 PIE.
- 목욕·작업공간 구입(같은 경로라 `Purchase.Transaction`이 Bath를 포함하나 PIE는 U3), 두 번째 구입·최대 표시·상한 조정의 PIE.
- `Content.ScreenContract`(Editor 작업 뒤 실행).
