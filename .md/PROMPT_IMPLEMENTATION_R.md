# 구현 재작업 프롬프트 — Bath Control Runtime Guard와 BathLoop 진단

## 재검토 결론

급수·배수 수직 구현의 상태 owner, 수위 계산, Customer invalidation과 recovery snapshot 구조는 유지한다. 다만 조작부 motion authoring의 runtime fail-closed 경계와 승인된 BathLoop 진단 계약이 완성되지 않아 Editor 단계로 넘기지 않는다.

## P1 — 조작부 motion authoring을 interaction 전에 runtime 검증

대상:

- `Source/BathhouseSim/Public/Facility/BathWaterControlComponent.h`
- `Source/BathhouseSim/Private/Facility/BathWaterControlComponent.cpp`
- `Source/BathhouseSim/Private/Tests/BathWaterAutomationTests.cpp`

현재 `QueryInteraction()`은 Static Mesh/collision geometry만 검증한다. `LocalRotationAxis`가 zero/non-finite이거나 `OpenAngleDegrees`가 non-finite이거나 `RotationDurationSeconds`가 non-finite/0 이하이어도 water-state request가 먼저 성공할 수 있다. 그 결과 물은 흐르는데 control이 닫힌 자세에 머물거나, non-finite duration이 motion Tick과 component quaternion으로 전파될 수 있다.

필수 수정:

1. motion authoring을 검사하는 단일 side-effect-free native helper를 둔다.
2. 축은 finite/non-zero, 각도는 finite, duration은 finite/양수여야 한다.
3. `QueryInteraction()`은 geometry와 motion authoring이 모두 유효할 때만 `bCanInteract=true`가 될 수 있어야 한다. invalid 상태는 명시적 실패 이유를 반환한다.
4. `ExecuteInteraction()`도 같은 query를 통과한 뒤에만 `RequestSetControlOpen()`을 호출한다. invalid authoring에서는 logical open state, water amount/state, relative rotation, motion/tick과 Niagara가 하나도 변하지 않아야 한다.
5. `CalculateOpenRotation()`, `StartMotion()`과 Tick은 호출 순서가 잘못되더라도 non-finite quaternion/duration을 component transform에 쓰지 않도록 마지막 방어를 유지한다.
6. Editor `IsDataValid()`과 runtime helper가 서로 다른 유효성 규칙을 갖지 않게 한다.

자동화 수용 기준:

- 실제 query collision geometry가 있는 control fixture에서 valid axis/angle/duration은 interaction 가능하다.
- zero axis, NaN/Inf axis, NaN/Inf angle, NaN/Inf/0/negative duration 각각은 query/execute가 실패한다.
- 각 실패 뒤 water logical state, amount, control rotation과 Tick 상태가 불변이고 transform에 NaN/Inf가 없다.
- 기존 양·음 local axis 등가성, authored closed pose, moving 재입력 거부와 full auto-close 중간 반전은 계속 성공한다.

## P2 — StateTree를 포함한 BathLoop 진단 계약 완성

대상:

- `Source/BathhouseSim/Private/Customer/CustomerSessionBath.cpp`
- `Source/BathhouseSim/Private/Customer/CustomerSessionComponent.cpp`
- `Source/BathhouseSim/Private/Customer/StateTree/CustomerStateTreeTasks.cpp`
- 필요 시 `Source/BathhouseSim/Public/Customer/CustomerSessionComponent.h`
- `Source/BathhouseSim/Private/Tests/BathWaterAutomationTests.cpp`

현재 전용 category는 session에서만 사용되고 native StateTree Task에는 사용처가 없다. 따라서 이동 성공/실패, pre-entry validation, search/stay 종료에 따른 다음 단계 진입을 승인된 phase/reason/correlation 형식으로 추적할 수 없다. 또한 `HandleBathSearchExpired()`는 전체 stay가 4초 남아 search가 4초로 clamp된 경우에도 `SearchElapsed`를 Data Asset의 10초로 기록한다.

필수 수정:

1. `LogBathhouseCustomerBath`를 Bath 관련 native StateTree Task 경로에서도 사용한다.
2. 최소한 Bath 이동 결과, entry validation 결과, search/stay expiry로 BathLoop를 떠나는 결정과 forced-exit cleanup 결과를 구분해 기록한다.
3. 가능한 로그는 `Customer`, `Iteration`, `Phase`, `BathActor`, `Slot`, `WaterPercent`, `ThresholdPercent`, `BathStayRemaining`, `ActualBathSeconds`, 실제 `SearchElapsed`, `Result`, `Reason` 순서를 유지한다.
4. search 시작 시 실제 시작시각 또는 resolved duration을 보존해 10초 기본 window와 남은 stay에 clamp된 window를 모두 정확히 기록한다.
5. Tick, water amount frame, Condition 반복 평가는 Log/Warning으로 출력하지 않는다. 정상 no-candidate retry는 계속 Verbose다.
6. 진단 추가를 위해 StateTree/Blueprint에 domain mutation을 옮기거나 generic Task의 다른 facility 의미를 바꾸지 않는다.

자동화 수용 기준:

- 10초 search expiry는 실제 약 10초, 남은 stay 4초에 clamp된 expiry는 실제 약 4초로 진단 값이 계산된다.
- retry/reselect가 같은 search start/end와 elapsed를 리셋하지 않는다.
- StateTree Bath 이동/entry/advance 분기에서 전용 category와 구분 가능한 reason을 사용하는 정적 또는 native 검증이 있다.
- 로그 추가가 frame/Tick 단위 spam을 만들지 않는다.

## 보존할 계약

- `UBathWaterSettings` 전역 threshold와 기본 80%
- `UBathWaterStateComponent`의 순유량, exact endpoint, 만수 auto-close와 recovery snapshot
- `ABathhouseBathFacilityActor`의 native component hierarchy, 평면 mover와 Niagara owner
- Facility candidate/reserve 직전 availability 재검증
- Customer 전체 stay/search/actual segment와 exact Bath delegate lifecycle
- Q Hold begin/cancel/stage/rollback 순서
- 기존 reflected 이름과 enum ordinal; Core Redirect 미추가
- Content, Config와 `.md/Unreal/*` 미수정

## 재검증

- `git diff --check`
- UE 5.8 `BathhouseSimEditor Win64 Development` 빌드
- `BathhouseSim.BathWater` focused automation 전부 성공
- 현재 작업 범위와 무관한 기존 Placement dirty 상태를 분리해 보고하되, BathWater 및 Customer/Recovery 관련 기존 테스트에 신규 실패가 없어야 한다.
- 수정된 `PROMPT_REVIEW.md`와 `PROMPT_UNREAL.md`가 실제 runtime guard와 진단 필드를 정확히 인계해야 한다.
