# 구현 재작업 프롬프트 — 보일러 노동 가동 수직 2차

## 재검토 결론

이전 리뷰의 계기 baseline 멱등성, 소진 Tick 전후 0→양수 판정, boiler gauge 필수 mesh/hierarchy 검증, shovel 필수 mesh/collision 검증, public `PlaceItemAsFacility()` success/failure 및 G/fixed-slot/Hold 경로 보강은 정적 코드상 반영됐다. 그러나 현재 재구현분은 테스트 translation unit이 컴파일되지 않는 명확한 오류가 있고, 해당 오류를 고친 뒤에도 operation delegate 재진입 테스트가 기대하는 failure code와 실제 transaction 분기 순서가 일치하지 않는다. 최신 변경분은 실제 UE 5.8 build와 automation도 한 번도 완료되지 않았다.

따라서 LAB-001~025, LAB-037~039 보일러 수직 구현은 아래 항목을 수정·검증한 뒤 다시 코드 리뷰를 요청한다. LAB-026~036을 추가하거나 `Content/`, `Config/`, Level, Blueprint/WBP, `.md/Unreal/*`을 수정하지 않는다.

## P1 — 신규 fuel automation compile blocker 제거

대상:

- `Source/BathhouseSim/Private/Tests/UtilityLaborFuelAutomationTests.cpp`

현재 `ShovelLoadVisual`은 69, 73~78행에서 먼저 사용되고 92행에서 선언된다. 현재 변경분을 UE 5.8로 빌드하면 이 translation unit은 선언되지 않은 식별자로 컴파일될 수 없다.

필수 수정:

1. `LoadVisual` component 조회·선언을 최초 사용보다 앞으로 옮긴다.
2. mesh, hierarchy, collision, navigation assertion에서 dereference하기 전에 component 존재를 명시적으로 검증한다.
3. missing `WorldMesh`, missing `LoadVisual`, 잘못된 parent, root collision/WorldStatic/Pawn/CCD, child collision/navigation 각각의 fail-closed assertion을 유지한다.
4. unity build 순서나 다른 test translation unit의 선언에 의존하지 않는 상태로 실제 compile한다.

## P1 — mutation guard를 failure-code mapping보다 먼저 판정

대상:

- `Source/BathhouseSim/Private/Utility/UtilityFuelTransaction.cpp`
- `Source/BathhouseSim/Private/Utility/UtilityOperationComponent.cpp`
- `Source/BathhouseSim/Private/Tests/UtilityLaborAutomationTestProbe.cpp`
- `Source/BathhouseSim/Private/Tests/UtilityLaborFuelAutomationTests.cpp`

현재 `Insert()`는 `Operation->CanAcceptFuel()`을 호출한 뒤 transaction guard를 검사한다. operation/fuel callback 안에서 중첩 `Insert()`가 실행되면 `CanAcceptFuel()`은 `bMutationInProgress` 때문에 거부하지만, `Insert()`는 이를 `NotInstalled`로 매핑하고 나중의 `TransactionBusy` 분기에는 도달하지 않는다. 따라서 probe와 lambda가 요구하는 `EUtilityFuelFailure::TransactionBusy` assertion은 실패한다.

필수 수정:

1. transaction 재진입은 다른 authoring/domain failure로 오분류되기 전에 `TransactionBusy`로 판정한다.
2. `Scoop()`, `Insert()`, `Return()`의 guard 판정과 failure mapping을 함께 점검해 같은 재진입 상태가 `InvalidAuthoring`, `ShovelAlreadyLoaded`, `WrongFuel`, `NotInstalled`로 변형되지 않게 한다.
3. 첫 validation과 guard 획득 후 revalidation 사이에는 mutation이나 delegate를 넣지 않는다. silent 양쪽 commit 뒤 완성된 상태만 publish하는 현재 원자성 경계는 유지한다.
4. operation callback과 shovel fuel callback 양쪽에서 중첩 투입이 `TransactionBusy`로 거부되고, 원래 삽은 empty, 중첩 삽은 Coal 25, operation은 25, Active capacity publication은 정확히 한 번임을 automation으로 확인한다.

## P2 — 구현 단계 문서 소유권 복구

대상:

- `.md/USER_UNREAL.md`
- `.md/PROMPT_UNREAL.md`
- `.md/PROMPT_REVIEW.md`

구현 단계는 `PROMPT_UNREAL.md`에 Editor 작업과 예상되는 tool 한계를 인계할 수 있지만, 실제 Unreal MCP capability preflight 전에 `USER_UNREAL.md` 미완료 큐를 작성하는 소유자는 아니다. 현재 `USER_UNREAL.md` 선두의 `보일러 노동 가동 수직 구현 — 시각 에셋·화면 작업` 블록은 구현 단계에서 추가됐다.

필수 수정:

1. 이번 구현이 추가한 해당 블록만 `USER_UNREAL.md`에서 제거하고 기존 다른 미완료 항목은 보존한다.
2. mesh 후보 미확정, 1024×576 layout과 시각 판정은 현재처럼 `PROMPT_UNREAL.md`의 exact asset/수용 기준에 유지한다.
3. 코드 리뷰 승인 뒤 Unreal MCP 단계가 실제 tool capability를 확인한 다음에만 수행 불가능한 항목을 `USER_UNREAL.md`에 기록한다.
4. `PROMPT_REVIEW.md`에는 수정 뒤 실제 build/automation 결과와 미실행 Editor/PIE 항목을 구분해 갱신한다.

## 재검증 조건

현재 `UnrealEditor` PID 5580, 22060과 대응 Live Coding Console이 같은 project module을 로드하고 있어 리뷰 단계에서는 재빌드하지 않았다. 구현자는 사용자 작업을 임의 종료하지 말고, Editor/Live Coding이 닫힌 조건에서 다음을 수행한다.

1. `.md/AGENT_WORKFLOW.md`의 UE 5.8 `Build.bat` 진입점으로 `BathhouseSimEditor Win64 Development` 전체 build를 완료한다.
2. `BathhouseSim.Utility.Labor` 세 automation을 모두 실행한다.
3. `BathhouseSim.BathWater`와 Placement/Interaction/Physical Carry/Computer focused 회귀를 실행한다.
4. 전체 `BathhouseSim` automation을 실행하고 신규 failure, error, ensure marker가 없는지 확인한다. 기존 실패가 있다면 exact test 이름과 이번 변경과의 분리 근거를 적는다.
5. `git diff --check`를 다시 실행한다.
6. build·automation을 실제 완료하기 전에는 코드 단계 승인이나 Unreal 작업 진입을 주장하지 않는다.

## 유지할 승인 경계

- `UUtilityOperationComponent`가 잔량과 게임시간 정본, `UBathWaterOperationsSubsystem`이 Installed/Active 집계 정본이다.
- boiler child composition, private fuel transaction, existing conversion transaction 재사용을 유지한다.
- 계기 authored baseline과 표시 quaternion 분리, 같은 게임시각 projected 0→양수 edge 판정을 되돌리지 않는다.
- 기존 reflected symbol과 enum ordinal을 유지하고 `Shovel`은 끝에 append한다. Core Redirect는 추가하지 않는다.
- runtime 상태, transaction, validation, input 판단과 Widget 값 계산을 Blueprint로 넘기지 않는다.
