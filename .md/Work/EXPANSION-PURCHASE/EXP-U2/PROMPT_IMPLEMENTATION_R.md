# PROMPT_IMPLEMENTATION_R — EXP-U2 코드 리뷰 1회차 재작업

- 작업 ID: `EXP-U2`
- 단계: 코드 리뷰
- 상태: 완료
- 출처: 코드 리뷰 1회차

## 0. 범위와 판정

- 리뷰 대상: `git diff 5d08266 27789da`(구현 커밋 `27789da`, 브랜치 `work/EXP-U2`). 빌드 식별값은 [PROMPT_REVIEW.md](PROMPT_REVIEW.md) 6절과 같아서 다시 빌드하지 않았다. 자동화 리포트 `Saved/Automation/Reports/20261002/exp_u2_full2`의 실패는 `Expansion.Content.ScreenContract` 하나다.
- 판정: 구현 재작업. 설계·명세는 바꾸지 않는다. 아래 R1은 반드시 고친다. R2·R3은 작은 정합 수정이라 같은 재작업에서 함께 고친다.
- 그대로 유지할 범위: transaction 순서(넓힘 → 결제 → tier → 방송 1회)와 실패 되돌림, `ExpectedPurchaseCount` 연타 방지, shell 재생성·띠 조각·Nav dirty, 확장 탭 상태 전이, 컴퓨터 이탈 취소, 상점 락커 규칙, 미리보기 격리, 400줄 정책. 이 부분은 다시 설계하거나 구조를 바꾸지 않는다.
- 구현이 설계에 맞춰 확정한 세부 4건은 모두 설계 의도와 맞다. `ApplyZoneGeometry`가 효과 횟수 크기를 쓰는 것, transaction 중 tier 확인·방송 생략, wallet 없음 무로그, Definition 문구 한국어화가 여기에 해당한다. 다만 wallet 무로그 판단은 R1처럼 사용자 없는 다른 원인까지 넓혀야 완결된다.
- `Expansion.Content.ScreenContract`가 Editor 작업 전에 실패하는 것은 설계(지시서 15절 "content(Editor 작업 뒤 실행)")대로다. 재작업 뒤에도 이 1개 실패는 허용한다.

## 1. 지적

### R1 (중요) 게임 시작 때 BeginPlay 순서에 따라 `LogBathhouseExpansion` Error가 잘못 남을 수 있음

- 위치:
  - `Private/Building/BathhouseExpansionPurchaseSubsystem.cpp` `Resolve`(213·219·226·236·245행의 `LogUnavailableOnce`)
  - 호출 경로: `Private/Computer/BathhouseComputerActor.cpp` `BeginPlay` → `InitWidget` → root `InitializeComputerScreen` → `ExpansionScreen->NotifyComputerUserChanged(nullptr)` → `UExpansionScreenWidget::RebindDomain` → `RefreshFromDomain` → `BuildView(nullptr)` → `Resolve(nullptr)`
- 원인:
  - 컴퓨터 BeginPlay가 `ABathhouseExpansionAuthority`나 공간 Actor의 BeginPlay보다 먼저 돌면 문제가 생긴다. Authority 등록과 공간 등록이 아직 안 된 상태라 `NoAuthority` 또는 `NoSpaces` Error가 한 번 남는다.
  - BeginPlay 순서는 정해져 있지 않다(World Partition external actor). 실제로 같은 코드베이스에 "Startup lockers are pending until the expansion authority is ready" 처리가 있다(`Placement.StartupLockerReconciliation`).
  - 이때 Buyer는 null이다. 그래서 view는 어차피 사용 불가이고, 구현 보고 3절이 "정상 상태(사용자 없음)라 로그를 남기지 않는다"고 한 경우와 같다. wallet만 로그에서 뺐고 같은 상황의 다른 원인은 그대로 남았다.
- 영향:
  - 데이터가 정상이어도 PIE 시작마다 빨간 Error가 남을 수 있다. 설계 7.2와 ExpansionPurchaseSystem.md는 이 Error를 "데이터가 없거나 잘못될 때 원인별 1회"로 정한다.
  - 사용자 PIE의 Output Log 판단과 EXP-031 관찰("원인별 한 번")을 흐린다. 원인별 1회 플래그도 가짜 로그가 먼저 써 버린다.
- 수정 방향:
  - 구매자 wallet이 없는 평가에서는 사용 불가 판정만 하고 Error를 남기지 않는다.
  - 예: `Resolve`가 wallet을 먼저 구한 뒤, wallet이 없으면 로그 없이 사용 불가로 끝낸다. 또는 로그를 남길지 결정하는 인자를 받는다. 방식은 구현이 고른다.
  - wallet 있는 사용자가 탭을 볼 때는 원인별 1회 Error가 지금처럼 남아야 한다(EXP-031).
  - 사용 가능 판정 규칙·순서·`FBathhouseExpansionView` 값은 바꾸지 않는다.
  - Authority·공간 등록 때 `OnExpansionChanged`가 오므로 화면은 나중에 바르게 갱신된다. 이 경로는 그대로 둔다.
- 재검증:
  - `Purchase.FailuresAndRollback`(또는 새 자동화)에 다음 두 단언을 넣는다. 첫째, Authority가 없는 world에서 `BuildView(nullptr)`가 `LogBathhouseExpansion` Error를 남기지 않는다(expected error 없이 통과하거나 로그 수로 확인). 둘째, 같은 world에서 wallet 있는 `BuildView(Player)`는 원인별 Error를 한 번 남긴다.
  - 자동화가 실제 컴퓨터 BeginPlay 순서를 재현할 필요는 없다.

### R2 (낮음) `ValidateWorld`의 설비 소속 검사가 편집 미리보기 횟수를 따름

- 위치: `Private/Building/BathhouseSpaceWorldValidation.cpp` "배치된 설비 소속" 루프. `OutSnapshots`(효과 횟수 = 편집 world 미리보기 횟수)의 `Interior`를 쓴다.
- 계약:
  - BuildingSystem.md Validation (U2)에 "`ValidateWorld`는 위 표를 0회 복사본에 적용"이라고 되어 있다. 이 표에 "배치된 `IPlaceableFacility`가 어느 공간에도 없거나 … Warning" 행이 들어 있다.
  - 지시서 15절은 "미리보기 횟수를 바꿔도 `ValidateWorld` 결과 불변"을 요구한다.
  - 지금은 미리보기를 켜면, 0회 공간 밖이지만 넓힘 띠 안에 있는 설비의 Warning이 사라진다.
- 수정 방향: 이 루프는 이미 만든 `BaseSnapshots`(0회 복사본)를 쓴다. 계단 trace는 안쪽 직사각형과 무관하므로 그대로 둔다.
- 재검증: 순수 함수가 아니므로 코드 확인으로 충분하다. 가능하면 `Validation.Rules` 또는 world fixture에서 미리보기 횟수를 바꿔도 같은 문제 목록이 나오는지 단언한다(편집 world 자동화를 새로 만들 필요는 없음).

### R3 (낮음) `Expansion.Content.ScreenContract`가 Editor 값 누락을 놓침

- 위치: `Private/Tests/ExpansionScreenAutomationTests.cpp` 352~359행.
- 문제:
  - `MaxPurchaseCount` C++ 기본값은 0이다. 이 상태로는 `ValidatePurchaseData`가 통과한다(가격 0줄, 효과 1줄 이상). 그래서 Editor 단계가 `Max Purchase Count`·`Purchase Prices`를 넣지 않아도 계약 테스트가 통과하고, PIE에서는 바로 `최대 확장 단계입니다`가 보인다.
  - 메시지의 `Reason`은 같은 호출식 안에서 `ValidatePurchaseData`보다 먼저 평가될 수 있어 비어 있을 수 있다(인자 평가 순서가 정해져 있지 않음).
- 수정 방향:
  - `GetMaxPurchaseCount() >= 1`(U2 홀 1회 구입이 가능해야 함)을 단언한다. 기대값은 리터럴로 복제하지 않고 "1회 이상"이라는 계약만 확인한다.
  - `ValidatePurchaseData`를 먼저 호출한 뒤 결과와 `Reason`으로 단언한다.
- 재검증: Editor 작업 전에는 계속 실패하는 것이 맞다.

## 2. 변경 금지와 공통 재검증

- Content·Config·Architecture 정본은 수정하지 않는다. R1~R3은 구현 상세라 정본 변경이 필요 없다. 다만 ExpansionPurchaseSystem.md "구현 상세" 문장이 "wallet이 없으면 … Error 로그는 남기지 않는다"로 되어 있으므로, 고친 범위(구매자 wallet 없는 평가는 무로그)에 맞게 그 한 문장만 고쳐도 된다.
- 빌드: UE_BUILD_POLICY 고정 명령, 오류·경고 0.
- 자동화: 필터 `BathhouseSim` 전체. 실패는 `Expansion.Content.ScreenContract` 1개만 허용한다. `Computer.Input.ScreenWheelContentContract`는 계속 통과해야 한다.
- `PROMPT_REVIEW.md`에 재작업 절을 덧붙인다. 지적별 변경 위치, 새 단언, 새 빌드 식별값(HEAD와 Source diff 해시)을 적는다. `PROMPT_UNREAL.md`는 R3로 content 계약 내용이 바뀌므로 4절의 "content 계약" 설명에 `Max Purchase Count` 1 이상 확인을 추가한다.
- 재검증은 같은 리뷰어가 이 지적과 새 diff만 본다.
