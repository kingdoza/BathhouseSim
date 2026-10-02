# PROMPT_REVIEW — EXP-U3 전체 확장 묶음 코드 리뷰 입력

- 작업 ID: `EXP-U3`
- 단계: 구현
- 상태: 완료

## 1. 기능 계약과 현재 단계

- 상위 계약 [../PROMPT_ARCHITECTURE.md](../PROMPT_ARCHITECTURE.md)(D1~D4 포함), 설계 [PROMPT_IMPLEMENTATION.md](PROMPT_IMPLEMENTATION.md)(이 문서가 U2 19절을 대체). 단계 시작 커밋 `11e1c0c`, 브랜치 `work/EXP-U3`.
- 범위: 원래 U3 EXP-040~046, D2·D3 EXP-021~023·040~042·047·048, D4 EXP-050~052. 구현은 설계 대로이며 이름·API 변경 없음(설계에서 벗어난 점은 8절 "설계와 다른 점" 참조).
- 적용한 개선 후보: FBK-001(world 검사 trace는 `bTraceComplex=false`, 기존 `ValidateWorld` 코드가 이미 만족해 변경 없음), FBK-004(그리기 조건은 설계 11.3절·8.2절, 새 component·hidden flag 없음, 자동화로 `ShouldRender()` 확인), FBK-006(값은 설계 절 기준).

## 2. 시나리오 ID별 코드·테스트 연결

| ID | 구현 경로 | 자동화 |
|---|---|---|
| EXP-021 | `ExpansionScreenModel.cpp` 선택지별 `Stage`·`Price`, `FBathhouseExpansionOptionView`(`AppliedCount`·`StepCount`·`NextPrice`), `PurchaseSubsystem::BuildView` | `Expansion.UI.ScreenModel`(선택 전 문구·가격 차이·헤더 문구 없음), `Expansion.Purchase.Transaction`(view) |
| EXP-022 | 모델 `bSelectedUsable`·`SelectedPrice` 기반 `Shortfall`·구입 가능, `EvaluatePurchase` `InsufficientMoney` | `Expansion.UI.ScreenModel`(선택 전 부족액 없음, 더 싼 공간은 부족 아님), `Expansion.Purchase.FailuresAndRollback` |
| EXP-023 | `ApplyNextExpansion`(줄의 모든 벽 한 번), `TryPurchase` 가격 = 고른 공간 `GetNextExpansionPrice()` | `Expansion.World.SpaceRuntime`, `Expansion.Purchase.Transaction` |
| EXP-040·041·047 | `ExpandInterior`·`ExpansionBandRects`·`SpaceExpansion.cpp` 조각 목록, 줄 `Sides` | `Expansion.MultiSide.Layout`, `Expansion.MultiSide.Runtime`(목욕공간 남·북·동, 조각 넓이·모서리), `Expansion.World.SpaceRuntime`(작업공간 조각 0) |
| EXP-042 | `EvaluatePurchase` 공간별 `ExpectedAppliedCount`, 홀 tier 사전 검사 | `Expansion.Purchase.Transaction`(목욕 뒤 홀 2번째 가격, 열쇠 tier 2행) |
| EXP-043·044 | `IsAtExpansionLimit`, 뷰 `bAllAtLimit`, 모델 `최대 확장 단계입니다` | `Expansion.Purchase.Transaction`, `Expansion.UI.ScreenModel` |
| EXP-045 | `ExpansionHallEffectShort` 검증, 홀 줄·`Tiers` 데이터 구동 | `Expansion.Validation.Rules`(효과 표 길이, world 검증) |
| EXP-046 | 코드 변경 없음(상점 규칙 기존). 데이터는 Editor 단계 | `Expansion.Content.ScreenContract`(Editor 작업 뒤), 기존 `Expansion.Locker.LimitAndShopRules` |
| EXP-048 | 벽 항목별 양, 중복·빈 벽·가격·맞닿음·출입구 검사 | `Expansion.MultiSide.Layout`(벽별 양, 순서 무관, 중복 무시), `Expansion.MultiSide.Runtime`(남 1.5배·북 0.5배), `Expansion.Validation.Rules` |
| EXP-050·051 | `RefreshPreview` `SetActorHiddenInGame(!bHasCandidate)`, `ValidateCurrentPlacement` `bOutHasCandidate`, `ConfirmPlacement` 실패 경로 그대로 | `Placement.PreviewHiddenWithoutAim`(하늘·거리 밖·비구역 물체, 시작 시 숨김, 회전 유지, 격자·안내 문구·아이템 유지) |
| EXP-052 | 후보 계산 뒤 실패는 숨기지 않음 | `Placement.PreviewHiddenWithoutAim`(구역 불허·겹침은 보임·invalid) |
| 회귀 EXP-020, 024~032, 001~015 | 기존 경로 | 기존 `Expansion.*`·Building·Computer·Shop·FacilityPlacement 자동화(7절) |

## 3. 변경 파일과 구현 요약

- Building: `Public/Building/BathhouseSpaceTypes.h`(`FBathhouseSpaceExpansionSide` 신규, `FBathhouseSpaceExpansionStep` = `Sides` + `Price`, 옛 `Side`·`AmountCm` 삭제), `Public/Building/BathhouseSpaceActor.h`·`Private/Building/BathhouseSpaceExpansion.cpp`(`GetNextExpansionPrice`·`IsAtExpansionLimit`, `CollectStepSnapshots`, `CanApplyNextExpansion` 새 조건·문구, 조각 목록), `Private/Building/BathhouseSpaceLayout.h`·`BathhouseSpaceExpansionLayout.cpp`(`IsUsableSide`·`IsStepApplicable`·다중 벽 `ExpandInterior`·`ExpansionBandRects`, `ExpansionBand` 삭제), `Private/Building/BathhouseSpaceValidation.h`·`BathhouseSpaceExpansionValidation.cpp`·`BathhouseSpaceWorldValidation.cpp`(새 코드 4개, `ExpansionStepsBelowCap` 삭제, `MaxPurchaseCount` 인자 → `HallEffectRowCount`), `Public/Building/BathhouseExpansionTypes.h`·`BathhouseExpansionPurchaseSubsystem.h`·`Private/Building/BathhouseExpansionPurchaseSubsystem.cpp`(전체 횟수·가격·`MaxPurchasesReached` 삭제, 공간별 view·가격, `EvaluatePurchase`·`TryPurchase`의 기대 횟수 = 고른 공간 횟수).
- Facility·Interaction: `Public/Facility/BathhouseExpansionDefinition.h`·`Private/Facility/BathhouseExpansionDefinition.cpp`(`MaxPurchaseCount`·`PurchasePrices`·`GetMaxPurchaseCount`·`TryGetPurchasePrice` 삭제, 규칙은 `Tiers`만), `Private/Interaction/BathhouseKeyRackActor.cpp`(효과 표 모든 줄 열쇠 수 검사).
- UI: `Private/UI/ExpansionScreenModel.h/.cpp`(선택지별 단계·가격, 화면 헤더 단계·가격 삭제, 버튼·부족액은 고른 공간 가격), `Public/UI/ExpansionScreenWidget.h`·`Private/UI/ExpansionScreenWidget.cpp`(`StageText`·`PriceText` binding 삭제, `ConfirmAppliedCount`, `bAllAtLimit`), `Public/UI/ExpansionSpaceOptionWidget.h`·`Private/UI/ExpansionSpaceOptionWidget.cpp`(`BindWidgetOptional` `StageText`·`PriceText`).
- Placement: `Public/Placement/PlayerFacilityPlacementComponent.h`(시그니처 + 새 테스트 friend), `Private/Placement/PlayerFacilityPlacementValidation.cpp`(`bOutHasCandidate`), `Private/Placement/PlayerFacilityPlacementComponent.cpp`(`RefreshPreview`·`ConfirmPlacement`만).
- 자동화: `Private/Tests/BathhouseExpansionAutomationTestSupport.h`, `BathhouseExpansionAutomationTests.cpp`, `ExpansionScreenAutomationTests.cpp`(수정), `BathhouseExpansionMultiSideAutomationTests.cpp`(신규, D3), `FacilityPlacementPreviewAimAutomationTests.cpp`(신규, D4).
- Config·Content·`BathhouseSpaceLayout.cpp`·`BathhouseSpaceValidation.cpp`·`BathhouseSpaceActor.cpp`·`AFacilityPlacementPreviewActor` 변경 없음.

## 4. 클래스 크기·책임 변화

| 파일 | 전 → 후(줄) | 판단 |
|---|---|---|
| `BathhouseSpaceLayout.cpp`·`BathhouseSpaceValidation.cpp`·`BathhouseSpaceActor.cpp` | 419·399·293 → 변화 없음 | 설계 금지 파일 무변경 |
| `BathhouseSpaceExpansion.cpp` | 204 → 249 | 같은 책임(넓힘 적용). 가격·상한 조회와 조건 문구 |
| `BathhouseSpaceExpansionLayout.cpp` | 101 → 129 | 같은 책임(넓힘 순수 계산) |
| `BathhouseSpaceExpansionValidation.cpp` | 176 → 204 | 같은 책임(넓힘 검사) |
| `BathhouseExpansionPurchaseSubsystem.cpp` | 355 → 330 | 전체 횟수·가격 코드 삭제로 감소 |
| `PlayerFacilityPlacementComponent.cpp` | 418 → 430 | `RefreshPreview` 표시 조건 12줄 안팎. 독립 책임 추가 없음(미리보기 갱신의 표시 조건) |
| `PlayerFacilityPlacementValidation.cpp` | 229 → 233 | 후보 flag |
| `BathhouseSpaceTypes.h`·`BathhouseSpaceActor.h` | 177 → 192, 146 → 150 | struct 한 개 추가, 조회 함수 2개 |

신규 클래스·module·의존 방향 없음. Building → Facility 의존은 그대로(`Tiers.Num()` 읽기).

## 5. Blueprint/API/Core Redirect 영향

- reflected 변경: 신규 `FBathhouseSpaceExpansionSide`, `FBathhouseSpaceExpansionStep::Sides`·`Price`, `UExpansionSpaceOptionWidget::StageText`·`PriceText`(Optional). 삭제 `FBathhouseSpaceExpansionStep::Side`·`AmountCm`, `UBathhouseExpansionDefinition::MaxPurchaseCount`·`PurchasePrices`, `UExpansionScreenWidget::StageText`·`PriceText` binding.
- rename·클래스 삭제 없음 → Core Redirect 불필요. 옛 Level 데이터의 `Side`·`AmountCm`은 load 때 건너뛰고 새 필드는 기본값(빈 `Sides`, 가격 0)이라 Editor 작업 전에는 공간 Data Validation 오류가 나는 것이 정상 전환 상태(설계 10절).
- Architecture 정본은 아키텍처 단계가 이미 구현 이름대로 갱신해 두었고 구현에서 구조가 바뀌지 않아 이 단계에서 갱신하지 않았다.

## 6. 빌드와 정적 검증 (빌드 시점 Source 식별값)

- 명령: UE_BUILD_POLICY.md 고정 명령(`BathhouseSimEditor Win64 Development`). 빌드 시 BathhouseSim Editor는 실행 중이 아니었다(프로세스 확인).
- 결과: `Result: Succeeded`. 최종 빌드 로그 `Saved/Logs/agent/build_u3_4.log`(앞서 `build_u3_1.log`·`build_u3_2.log`·`build_u3_3.log`는 중간 빌드: 1회차는 새 테스트 파일의 protected 접근 오류, 이후 수정).
- 식별값: HEAD `11e1c0c8f4ecd6ffb0300e68d2ac0236007ab447`, `git diff HEAD -- Source Config` SHA-256 `3c438cf0e562e6f0d4f4e463ec99ab143758d63533c970aff7df6f522f55d87f`(추적 파일 26개). 추적되지 않는 신규 파일 2개는 diff에 없어 따로 기록: `BathhouseExpansionMultiSideAutomationTests.cpp` SHA-256 `34172af29e9e163ecb2b41f2c0a4344c253b8935e1c83ca3df6c684da9b93a3d`, `FacilityPlacementPreviewAimAutomationTests.cpp` SHA-256 `f6217ecbe352f30539a96606ad71360cec307365af2d796a07bac48086454e17`. Content·Config 변경 없음.
- 정적 검사: `git diff --check -- Source` 출력 없음. 변경 파일은 기존 규약대로 CRLF. `MaxPurchaseCount`·`PurchasePrices`·`GetPurchaseCount`·`TryGetPurchasePrice`·`bMaxReached`·`ExpansionBand(`·`ExpansionStepsBelowCap`·`MaxPurchasesReached`는 Source에 남아 있지 않다(Grep).

## 7. 자동화 결과

- 실행: `Automation RunTests BathhouseSim`(headless, UE_BUILD_POLICY 형식), 로그 `Saved/Logs/agent/auto_u3_all3.log`, 보고 `Saved/Automation/Reports/20261002/u3_all3`. 177개 중 175개 통과, 2개 실패.
- 실패 2개:
  1. `BathhouseSim.Expansion.Content.ScreenContract` — **예상 실패(Editor 작업 전 전환 상태)**. 이유: `WBP_ExpansionSpaceOption`에 `StageText`·`PriceText` 없음, `WBP_ExpansionScreen`에 아직 두 widget이 남아 있음, `DA_ShopCatalog` 마지막 세 상품이 1·4·8칸 락커가 아님. Editor 작업(PROMPT_UNREAL 3.7~3.9) 뒤 통과해야 한다.
  2. `BathhouseSim.Shop.BlueprintLoad` — **이 작업과 무관한 기존 실패**. 이유: `ShopAutomationTests.cpp(1302)`가 PlayerState Blueprint 지갑 기본값을 100000으로 단언하는데 asset 값이 100000000이다(PIE용 임시 값이 저장된 상태로 보임). 이번 변경은 Content·지갑·상점 코드를 건드리지 않았고, `git status`에서 Content는 변경 없음이다. 조치는 마스터 판단(asset 값 되돌림 또는 테스트 기대 수정은 이 작업 범위 밖).
- 통과 항목: 새 `Expansion.MultiSide.Layout`·`Runtime`, `Placement.PreviewHiddenWithoutAim`, 수정된 `Expansion.*` 전부(`Layout`·`Validation.Rules`·`Data.DefinitionAuthorityKeyRack`·`World.SpaceRuntime`·`Purchase.*`·`Locker.*`·`Preview.*`·`UI.*`), `Computer.Input.ScreenWheelContentContract`, 기존 Building·Computer·Shop(위 1건 제외)·FacilityPlacement·Economy.
- 기대값은 모두 fixture 값(넓힘 줄 벽·양·가격, `Tiers`, Settings 배치 거리, `FixtureAmountCm`)에서 읽거나 계산했다. 값 자체를 고정하는 회귀는 없다. fixture 가격(1000·2000, 1500·2500, 1200·2200)은 공간마다 다름을 검증하기 위한 입력이며 Editor 제안값과 무관하다.

## 8. 설계와 다른 점, 리뷰 중점, 전역 영향과 미검증

설계와 다른 점:
- 설계 17절의 "홀 줄 + 1 효과 표" fixture: `Tiers`는 3줄(홀 줄 2 + 1)로 줄였다(이전 fixture는 4줄). `HallEffectShort`·열쇠걸이 전 줄 검사를 소극적 데이터로 시험하기 위해서다.
- 설계 17절 "widget" 행의 "`StageText`·`PriceText` 없는 option widget에서도 `ApplyModel` 안전"은 Abstract 해제 시험용 `UExpansionOptionTestWidget`(테스트 support, 통과)으로 확인했다.
- D4 자동화는 설계의 (a)(b)(c)와 시작 시 숨김·EXP-052에 더해 공간 불허 구역 조준과 겹침 조준까지 같은 test에 포함했다. 겹침 검사는 존 trace를 막지 않는 WorldDynamic 상자로 만들었다.

리뷰 중점(설계 17절 기준과 같음):
- 줄 하나의 모든 벽이 한 번의 shell 재생성으로 적용되고 이웃 shell을 다시 만들지 않는다(`SpaceRuntime`·`MultiSide.Runtime`이 Hall shell wall component identity 유지 확인).
- 가격은 고른 공간 `GetNextExpansionPrice()`에서만 온다. 전체 구입 횟수·상한·순번 가격이 Source에 없다.
- `ExpandInterior`와 `IsStepApplicable`·`CanApplyNextExpansion`이 같은 규칙(처음 나온 벽, 양 유효)을 쓴다. 단 `CanApplyNextExpansion`은 중복·빈 벽·양 잘못·가격을 구분해 로그 문구를 내려고 중복 판정을 한 번 더 한다(규칙은 같고 `IsStepApplicable`이 최종 판정).
- D4: 숨김 판정은 `bHasCandidate` 하나이고, 숨길 때 옮기지 않으며 Actor를 파괴하지 않는다. 미리보기 mesh에 hidden 상태 그림자 flag를 켜지 않았다(`FacilityPlacementPreviewActor.cpp` 무변경). 이 판정으로 정상 플레이에서 안 생기는 상태이상(들고 있는 설비 변경 등)도 후보 없음으로 숨는다(설계 11.1, fail-closed).
- `ExpansionHallEffectShort`는 Authority·Definition이 없으면 생략(`INDEX_NONE`)한다.
- 새 binding이 Optional이고 content 계약이 존재를 단언한다.
- 문구가 계약과 같다(`확장 단계 N/M`, `다음 넓힘 N원`, `확장 구입 (N원)`, `N원 부족`, `이 공간은 더 넓힐 수 없습니다`, `최대 확장 단계입니다`, `확장을 사용할 수 없습니다`, `설치 가능한 구역을 바라보세요.`).

전역 영향:
- Project/World/Input/Collision/Nav 설정 변경 없음, Config 변경 없음. `SetActorHiddenInGame`은 값이 바뀔 때만 일한다(엔진 소스 확인은 설계 11.3절).
- 확장 데이터 lifecycle: Editor 작업 전에는 세 공간이 "더 넓힐 수 없음"으로 보이고 공간 Data Validation 오류가 난다(정상 전환 상태, 설계 10절).

미검증:
- Editor 작업(Content 값), 실제 Level 끝 모습의 겹침·Nav 범위, 카드 글자 잘림, 모서리 이음새·깜빡임·Recast 반영 등 눈·PIE 항목은 검증하지 못했다(`PROMPT_UNREAL.md` 5절과 마스터 `PIE_CHECKLIST.md` 대상).
- `ShouldRender()`는 nullrhi 자동화에서 확인했으나 실제 렌더 결과(그림자·간접광)는 PIE에서 사용자가 본다.
- `Shop.BlueprintLoad` 기존 실패의 원인 asset은 확인만 했고 고치지 않았다.

## 9. 추가 지시: `Shop.BlueprintLoad` 수정 (마스터)

- 원인: 사용자가 `BP_BathhousePlayerState` 지갑 `StartingMoney`를 100000000으로 바꿔 커밋했다(사용자 조정 원본). `ShopAutomationTests.cpp`(PlayerState CDO 단언)가 100000 리터럴을 기대해 조정값 원본 원칙에 어긋났다.
- 수정: 그 단언만 "지갑이 있고 `StartingMoney`가 읽히며 0 이상"으로 바꿨다. 값 고정 의도는 같은 파일 앞쪽 native C++ 기본값 단언(`New wallets start with 100000`)이 이미 맡고 있어 BP 값은 고정하지 않는다. Content 변경 없음.
- 재빌드 `Result: Succeeded`(`Saved/Logs/agent/build_u3_7.log`), 전체 자동화 177개 중 176 통과, 실패는 예상 `Expansion.Content.ScreenContract` 하나뿐(`Saved/Logs/agent/auto_u3_all6.log`). 위 7절의 `Shop.BlueprintLoad` 실패 설명은 이 수정으로 해소됐다.
- 새 식별값: HEAD `11e1c0c8f4ecd6ffb0300e68d2ac0236007ab447`, `git diff HEAD -- Source Config` SHA-256 `4282d6fc8d927e65bb00f7ddc9f2bb15343349926a3e668dd0e8b58ec46e880a`(신규 테스트 2개 해시는 6절 그대로).
