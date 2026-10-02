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

## 8. 재작업 1회차(코드 리뷰 1회차, 출처 PROMPT_IMPLEMENTATION_R.md)

재작업 시작 커밋 `2741e57`. 범위는 R1~R3만이며 유지 범위는 바꾸지 않았다.

| 지적 | 변경 위치 | 새 단언 |
|---|---|---|
| R1 가짜 Error 로그 | `BathhouseExpansionPurchaseSubsystem.cpp` `Resolve`: wallet을 먼저 구하고 없으면 어떤 로그도 없이 사용 불가로 반환(판정 규칙·순서·view 값 불변) | `Purchase.FailuresAndRollback`에 Authority 없는 world 블록 추가. `BuildView(nullptr)`·`EvaluatePurchase(nullptr)`는 expected error 없이 통과(로그가 남으면 실패), wallet 있는 `BuildView(Player)`는 `LogBathhouseExpansion` Error를 정확히 1회 |
| R2 설비 소속 | `BathhouseSpaceWorldValidation.cpp` 설비 소속 루프가 `BaseSnapshots`(0회 복사본) 사용 | 코드 확인(world 적용 횟수 변경 불변 단언은 기존 `Validation.Rules`가 유지) |
| R3 content 계약 | `ExpansionScreenAutomationTests.cpp`: `ValidatePurchaseData`를 먼저 호출한 뒤 결과·`Reason`으로 단언, `GetMaxPurchaseCount() >= 1` 단언 추가 | Editor 작업 전에는 계속 실패(예상) |

문서: `PROMPT_UNREAL.md` content 계약 설명에 `Max Purchase Count` 1 이상 추가, `ExpansionPurchaseSystem.md` 구현 상세의 wallet 문장 정정.

새 빌드 식별값: 성공(오류·경고 0, `Saved/Logs/build_exp_u2_r1.log`). HEAD `2741e57037de8fe2d63ca869d53ab863465d9239`, `git diff HEAD -- Source Config | sha256sum` = `9e5225e246b75c61a68b57539b0e14cabc3f81309aa77b80f9ff02cd455bd863`(신규 Source 파일은 이제 모두 HEAD에 있어 untracked 0). 자동화 `BathhouseSim` 전체 173개 중 통과 172, 실패 1(`Expansion.Content.ScreenContract`, Editor 작업 전 예상 실패, 이제 `Max Purchase Count` 단언도 실패 목록에 포함). `Computer.Input.ScreenWheelContentContract` 통과. 로그 `Saved/Logs/auto_exp_u2_r1.log`.

## 9. 재작업 2회차(Editor 작업 R1·R2, 아키텍처 복귀 RET-003 18절)

재작업 시작 커밋 `add655f`. 범위는 18절 미리보기 글자와 R2뿐이다. Content는 수정하지 않았다.

- R1 구현: `UTextRenderComponent`를 없애고 편집 world 전용 Transient `UWidgetComponent`(World space, 원점 pivot 북쪽 붙임/남쪽 붙임)에 코드로 만든 `UTextBlock` widget을 띄운다. 신규 `Private/Building/BathhouseSpacePreviewLabelWidget.h/.cpp`(`Initialize`에서 tree root 생성, `SetLabel`은 기본 글꼴 object 유지·크기만 변경). 순수 helper `FBathhouseSpaceLayout::PreviewLabelPlacement`(`BathhouseSpaceExpansionLayout.cpp`): Z = 모든 공간의 가장 높은 천장 판 윗면 + `EditorPreviewLabelHeightCm`, 지하(바깥 직사각형이 겹치는 더 높은 바닥이 있는 공간)는 남쪽, 나머지는 북쪽. 회전은 `UWidgetComponent` 축(+X 앞면, +Z 위쪽)에서 앞면 +Z·위쪽 +Y로 정했고(`MakeFromXZ(Up, Right)`) 자동화가 확인한다. scale = `EditorPreviewLabelWorldSizeCm / EditorPreviewLabelFontSize`.
- Settings: 신규 `EditorPreviewLabelHeightCm`, `EditorPreviewLabelFontSize`, 기존 `EditorPreviewLabelWorldSizeCm`(뜻을 글자 높이로). 마스터 지시에 따라 `Config/DefaultGame.ini` `[/Script/BathhouseSim.BathhouseBuildingSettings]`에 세 키를 넣었고 값은 C++ 기본값과 같다(100 / 300 / 64). 18.2는 "C++ 기본값만"이라 했으므로 리뷰에서 둘 중 하나를 원본으로 정할 것(지금은 ini가 우선, C++ 값은 같은 기본값).
- R2: `ServiceBlueprintLoadAutomationTests.cpp`의 상품 수 `== 20` 리터럴 단언을 제거하고(이후 단위가 카탈로그 뒤에 덧붙이므로) 서비스 4단위 상품 존재 단언은 유지.
- 새 자동화: `Expansion.Preview.LabelPlacementAndComponent`(공간 하나 Z, 세 글자 같은 Z·지하만 남쪽·XY = 효과 횟수 중심, 글자 component의 editor-only·Transient·NoCollision·Nav 비관련·hidden in game·World space·widget class·위치·앞면 +Z·위쪽 +Y·scale·pivot·`GetPreviewLabelText()`, 편집 플래그 없는 재생성 뒤 글자 없음). headless에서 Slate가 없으면 widget 내부 문구 확인은 건너뛴다(정보 로그).
- 빌드: 성공(오류·경고 0, `Saved/Logs/build_exp_u2_r2.log`). HEAD `add655f7c3b78f6aaf22a69e4426c9cb1df36a60`, `git diff HEAD -- Source Config | sha256sum` = `0909817bdfbb039f8d26c1b21824b1a5cd1ec2b82878f5689ec7c0aa1dbcea7a`, 추적 안 된 신규 Source 2개(`BathhouseSpacePreviewLabelWidget.h/.cpp`)의 파일별 sha256 합 = `4a525b6e54fecb94d328c0087195efad7a935dcc717072e0cbc85a4eac789c7a`.
- 자동화: `BathhouseSim` 전체 174개 전부 통과(`Expansion.Content.ScreenContract` 포함, `Saved/Logs/auto_exp_u2_r2.log`).
- 미검증: 편집 world에서 실제 렌더(한글 표시, 크기, 방향), Slate가 있는 환경의 widget 내부 문구·글꼴 object 동일성 — Editor 재확인 대상.
