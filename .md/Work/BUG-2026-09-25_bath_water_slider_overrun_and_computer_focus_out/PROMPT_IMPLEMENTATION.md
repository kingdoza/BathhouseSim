# PROMPT_IMPLEMENTATION — 욕탕 관리 슬라이더 한계 초과·컴퓨터 클릭 없는 포커스아웃

- 작업 ID: `BUG-2026-09-25_bath_water_slider_overrun_and_computer_focus_out`
- 단계: 아키텍처
- 상태: 완료

## 1. 기능 계약과 시나리오

기능 명세는 생략됐다(CONTEXT 근거). 기능 계약은 다음 셋이다.

- 버그 리포트 `.md/BugReports/2026-09-25_bath_water_slider_overrun_and_computer_focus_out.md`의 기대 동작
- 채택 전 `QNA_FEATURE_SPEC.md`(커밋 `d6c6dbc`) Q6 A·Q18 A·Q23 A·Q32 A
- 컴퓨터 포커스 계약 CMP-001~020(커밋 `61f72c1`의 `PROMPT_ARCHITECTURE.md`)

현재 단계: 단순 버그 수정이다. 수직 구현이나 전체 확장이 아니다. 아래 시나리오 ID는 이 작업에서 붙인 것이고, 관찰 결과는 위 계약 문장을 그대로 옮겼다.

| ID | 조건 | 입력 | 기대 결과 |
|---|---|---|---|
| SLD-001 | 욕탕 선택, 순환 설치 정격용량이 순환도 100%보다 작음 | 순환도 슬라이더를 한계 너머까지 드래그(계속 끌기) | 드래그 중 한계까지는 값이 즉시 적용된다. 한계에서 손잡이가 더 움직이지 않는다. 제한 이유(종류·부족 포인트)가 표시된다 |
| SLD-002 | SLD-001 상태 | 한계 너머에서 마우스를 놓음 | 손잡이가 확정값(한계) 위치에 있다. 상세 패널 순환도 수치와 손잡이 위치가 같다 |
| SLD-003 | 가열 설치용량 부족 / 냉각 설치용량 부족 | 목표 수온 슬라이더를 실온 위쪽 / 아래쪽 한계 너머로 드래그 후 놓음 | SLD-001·002와 같다. 손잡이는 확정 목표 수온(0.5°C 단위)에 있다 |
| SLD-004 | 설치 정격용량은 충분하고 가동용량만 0 또는 부족 | 슬라이더를 설치 한계 이내로 드래그 | 제한 없이 설정된다(Q32). 설정은 보존되고 해당 효과만 중지되며 용량 부족 상태가 표시된다 |
| SLD-005 | SLD-001 한계 상태 | 한계 아래로 되돌려 드래그 | 손잡이가 자유롭게 따라오고 제한 이유가 사라진다. 다른 욕탕 설정은 바뀌지 않는다(Q6) |
| SLD-006 | 한계 너머로 드래그하는 중 | E 또는 ESC로 포커스아웃 후 재진입 | 손잡이는 확정값 위치이고 나간 뒤 값이 더 바뀌지 않는다(CMP-005) |
| CMP-001 | 포커스인 완료, 화면을 한 번도 클릭하지 않음 | E 한 번 | 포커스아웃, 고정 위치·방향으로 나온다 |
| CMP-004 | 화면 버튼 클릭 또는 슬라이더 조작을 마친 뒤 | E 한 번 | 포커스아웃, 재진입 시 마지막 화면 상태 그대로 |
| CMP-006 | 컴퓨터 조준, 빈손 | E를 눌러 진입하고 전환이 끝날 때까지 누르고 있다가 뗌 | 진입이 유지되고, 떼는 것만으로 나가지 않는다 |

비목표: 마우스 휠 스크롤(`COMPUTER-WHEEL-SCROLL`), ESC 동작 변경, 용량 계산·domain 규칙 변경, 슬라이더 외 관리 화면 변경, Widget layout 변경.

## 2. 원인 확정

### 현상 1 — 슬라이더 손잡이 한계 초과 (Source로 확정)

1. 슬라이더를 끌 때마다 `SSlider::OnMouseMove`가 `CommitValue(PositionToValue(...))`를 부른다. `CommitValue`는 먼저 Slate 값 attribute를 커서 위치 값으로 바꾸고, 그다음 `OnValueChanged`를 실행한다(`Engine/Source/Runtime/Slate/Private/Widgets/Input/SSlider.cpp` 336~349행, 415~430행). 즉 손잡이는 커서를 따라가고, 되돌리는 일은 handler 책임이다. `USlider` Value에는 property binding이 없다. `WBP_BathWaterDetail`에도 `Get_*` binding 함수가 없음을 읽기 전용으로 확인했다.
2. `UBathWaterDetailWidget::HandleCirculationChanged`·`HandleTargetTemperatureChanged`는 subsystem 요청 결과를 `ApplyRequestFeedback`(문구)에만 넘긴다. `Result.CommittedValue`로 손잡이를 되돌리지 않는다(`Private/UI/BathWaterDetailWidget.cpp` 221~269행).
3. 한계에 처음 닿아 domain 값이 바뀌는 요청은 다음 순서로 손잡이를 한 번 되돌린다. `RequestCirculationPercent`/`RequestTargetTemperature` → `BroadcastMutation` → root `HandleOperationsChanged` → `ApplyBathSnapshot` → `SetValue`.
4. 이미 한계에 있으면 같은 확정값이 다시 나온다(`Committed == Current`). 이때 subsystem은 mutation·broadcast 없이 반환한다(`BathWaterOperationsSubsystem.cpp` 219~222행, 305~308행). 그래서 3의 경로가 실행되지 않는다.
5. root `NativeTick`의 polling `ApplyBathSnapshot`도 손잡이를 되돌리지 않는다. domain 값이 cache와 같으면 일찍 반환하고, 그 반환 지점이 슬라이더 쓰기보다 앞에 있기 때문이다(`BathWaterDetailWidget.cpp` 154~170행). 손잡이 위치는 cache 비교 대상이 아니다.
6. 결과: 한계에 닿은 뒤 두 번째 mouse move부터 손잡이가 커서를 따라 한계를 넘는다. 놓은 뒤에도 polling이 고치지 않아 그 자리에 남는다. 리포트의 두 증상과 같다.

domain은 계약대로 동작한다. 제한 기준은 설치 정격용량(`CalculateTotalCapacity` = 설치 포인트 합)이다. 가동용량 부족은 상태 표시만 하고(Q32), 다른 욕탕 설정은 바꾸지 않는다(Q6). 따라서 결함은 UI 표시 동기화에만 있다.

### 현상 2 — 클릭 없이는 E로 포커스아웃 안 됨 (원인 확정, Source는 이미 수정됨)

리포트를 작성한 시점(`d6c6dbc`, 2026-09-25)의 `CompleteFocusIn`은 `FInputModeGameAndUI::SetWidgetToFocus(ScreenWidget->TakeWidget())`를 적용했다. 엔진에서는 다음 순서로 E가 사라진다.

1. `FInputModeGameAndUI::ApplyInputMode` → `SetFocusAndLocking`은 local player의 `SlateOperations` reply에 `SetUserFocus(화면 Slate widget)`을 넣는다(`Engine/Private/PlayerController.cpp` 6313~6328행, 6398~6414행).
2. 이 reply는 frame마다 `FEngineLoop::ProcessLocalPlayerSlateOperations`가 실제 Slate user(그 controller의 user index)에게 적용한다(`Launch/Private/LaunchEngineLoop.cpp` 5231~5263행 → `SlateApplication.cpp` 3713~3724행).
3. 화면 widget은 `UWidgetComponent`가 `RegisterVirtualWindow`로 등록한 virtual window 안에 있다(`UMG/Private/Components/WidgetComponent.cpp` 1206행). `FSlateApplication::SetUserFocus`는 일반 window에서 못 찾으면 virtual window까지 검색해 focus를 옮긴다(`SlateApplication.cpp` 2721~2740행). 그래서 실제 사용자의 키보드 focus가 game viewport를 떠나 world-space 화면 widget으로 간다.
4. Slate는 key 이벤트를 focus된 widget 경로로 보낸다. 게임 viewport(`FSceneViewport::OnKeyDown`, `SceneViewport.cpp` 1266행 이후 `InputKey`)가 E를 받지 못한다. PlayerInput → Enhanced Input → `IA_Interact` Started → `AFirstPersonCharacter::InteractStartInput()`이 모두 실행되지 않는다. Character 분기(`RequestEndComputerUse`)는 문제가 아니었다.
5. 화면을 실제 마우스로 클릭하면 game viewport의 `SViewport`가 pointer 아래 첫 focusable widget이다. Slate가 그 user의 focus를 viewport로 옮기고(`SlateApplication.cpp` 5486~5505행), `CaptureDuringMouseDown`으로 capture도 잡는다. 그 뒤 E는 viewport → Enhanced Input으로 도달한다. 이것이 클릭 전후 차이다. world-space 화면의 버튼·슬라이더 클릭은 `UWidgetInteractionComponent`의 virtual user 입력이라 실제 사용자 focus와 무관하다.

현재 Source(`61f72c1`, CMP-001 설계)는 이미 고쳐져 있다. `CompleteFocusIn`이 widget focus 없이 `FInputModeGameAndUI`를 적용하고 `UWidgetBlueprintLibrary::SetFocusToGameViewport()`를 호출한다(`PlayerComputerUseComponent.cpp` 309~322행). `FSlateApplication::SetAllUserFocusToGameViewport`가 실제 사용자 focus를 등록된 game viewport에 둔다(`SlateApplication.cpp` 2655~2666행). 위 1~3단계가 일어나지 않으므로 두 번째 E는 클릭과 무관하게 Enhanced Input에 도달한다.

추가 확인(읽기 전용):

- `Content/` 전체의 binary 문자열 검색 결과, 관리·상점·root WBP에 `SetKeyboardFocus`·`SetUserFocus`·`SetFocusToGameViewport`·`SetInputMode` 노드가 없다. 유일한 일치는 사용하지 않는 ThirdPerson 템플릿의 `UI_TouchSimple`이다.
- Source의 화면 widget에도 focus 호출이 없다.
- 이 수정은 아직 사용자 PIE로 검증되지 않았다. `USER_UNREAL.md`의 CMP 남은 수용 항목이다.
- 남은 공백은 이를 지키는 자동화가 없다는 점이다. 기존 테스트는 `InteractStartInput()`을 직접 호출하고, 테스트 world에 game viewport가 없어 `SetInputMode`가 no-op이다.

따라서 현상 2는 production 동작을 바꾸지 않는다. 회귀 방지 자동화를 추가하고 사용자 PIE로 닫는다. 이 build 뒤에도 PIE에서 CMP-001이 실패하면 8절의 조건부 진단으로 간다.

## 3. 설계

### 3.1 책임 변화

| 항목 | 판단 |
|---|---|
| 기존 책임 | `UBathWaterDetailWidget`: 선택 욕탕 표시, slider intent 전달, 요청 피드백 |
| 신규 책임 | slider 손잡이를 항상 domain 확정값과 일치시킨다(요청 직후 동기, polling 보정) |
| 상태 owner | 변경 없음. 순환도·목표 수온은 `UBathWaterConditionComponent`, 원장은 `UBathWaterOperationsSubsystem` |
| 실행 owner | detail widget의 slider callback 안 동기 보정 |
| authoring owner | 변경 없음(WBP layout, slider 0~1 범위) |
| 의존 방향 | 변경 없음. 테스트만 `Slate`·`SlateCore`를 직접 사용 |
| 최종 판단 | 기존 detail widget 확장(269줄, 경고선 아래). 새 타입 없음 |

거부한 대안:

- subsystem이 no-op 요청에도 broadcast하거나 revision을 올리는 안: no-op mutation 알림을 만들어 revision 의미를 깬다. 모든 구독자 refresh 비용도 늘어난다.
- slider `MaxValue`를 한계로 동적으로 줄이는 안: 슬라이더 축척이 바뀌어 위치가 의미를 잃는다.
- `MouseUsesStep`·`Locked`로 막는 안: 한계는 slider 속성이 아니라 domain 결과다.
- polling만으로 고치는 안: 한 frame 이상 한계 밖 손잡이가 그려질 수 있다. 요청 직후 동기 보정이 기본이고 polling은 안전망이다.
- WBP graph 보정: 금지(Native Widget Policy).

### 3.2 `UBathWaterDetailWidget` 변경 (`Public/UI/BathWaterDetailWidget.h`, `Private/UI/BathWaterDetailWidget.cpp`)

reflected·BindWidget 계약은 바꾸지 않는다. private 멤버와 test friend만 추가한다.

1. 정규화 helper(private, 파일 내부 함수 또는 private 멤버). 기존 `ApplyBathSnapshot`의 계산을 옮겨 한 곳에서만 쓴다.
   - 순환도 `Percent → Percent * 0.01`
   - 목표 수온 `C → (C - Min) / max(Max - Min, 0.001)`, 0~1로 clamp. `UBathWaterSettings` 기본값을 사용한다.
2. 무음 쓰기 helper `WriteSliderValueSilently(USlider*, float NormalizedValue)`.
   - 값이 다를 때만 `SetValue`한다. 비교는 기존과 같이 `FMath::IsNearlyEqual`.
   - 쓰는 동안 private 재진입 guard `bWritingSliderValue = true`(`TGuardValue`).
   - 근거: UE 5.8 `USlider::SetValue`는 `HandleOnValueChanged`를 통해 `OnValueChanged`를 다시 broadcast한다(`UMG/Private/Components/Slider.cpp` 152~164행, 95~101행). guard가 없으면 보정이 새 요청을 만든다. 그 요청은 제한 없는 결과라 제한 피드백을 지운다.
   - 기존 automation counter `SliderWriteCount`는 이 helper가 실제로 쓸 때만 증가시킨다.
3. `HandleCirculationChanged(Value)` / `HandleTargetTemperatureChanged(Value)`:
   - `bApplyingSnapshot || bWritingSliderValue`이면 즉시 반환한다.
   - 기존처럼 요청하고 `ApplyRequestFeedback(Result)`를 호출한다.
   - 이어서 **같은 callback 안에서** 해당 slider 하나만 다시 쓴다.
     - `Result.bSucceeded`: `Result.CommittedValue`를 정규화해 쓴다(순환도 %, 목표 수온 °C).
     - 실패: `Operations->GetBathSnapshot(SelectedBath)`가 성공하면 그 domain 값을 쓴다. 아니면 같은 욕탕의 `CachedSnapshot` 값을 쓴다. 둘 다 없으면 쓰지 않는다.
   - 이 쓰기는 `SSlider::CommitValue`의 `OnValueChanged` 안에서 실행된다. 따라서 같은 입력 이벤트가 끝나기 전, paint 전에 Slate attribute가 확정값으로 바뀐다.
   - 요청 중 broadcast로 `ApplyBathSnapshot`이 중첩 호출돼도 결과는 같다. 같은 값이면 쓰지 않는다.
4. `ApplyBathSnapshot(Snapshot)`: slider 두 개의 값 동기화를 **presentation cache 조기 반환보다 앞**으로 옮긴다.
   - snapshot이 있으면 매 호출마다 2의 helper로 순환도·목표 수온을 맞춘다. 다를 때만 쓴다.
   - text·step size 쓰기와 `PresentationWriteCount`는 지금처럼 cache gate 뒤에 둔다. 기존 "동일 snapshot이면 presentation write 없음" 테스트 의미는 유지한다.
   - `bApplyingSnapshot` guard는 유지한다.
5. 피드백 정책(`ApplyRequestFeedback`, revision 기반 해제)은 변경하지 않는다. 한계에서 반복된 no-op 요청도 `bWasLimited`이므로 제한 이유가 유지된다. 한계 아래로 돌아오면 제한 없는 결과가 문구를 지운다.
6. `NativeConstruct`/`NativeDestruct`의 delegate 대칭 연결은 유지한다. 테스트가 같은 연결 경로를 쓰도록 새 test class를 friend로 추가한다. 연결 부분을 private helper로 빼는 것은 구현 재량이다.

Root(`UBathWaterManagementScreenWidget`), map, tile, summary, subsystem과 domain은 변경하지 않는다.

### 3.3 컴퓨터 포커스 (현상 2)

- `UPlayerComputerUseComponent`·`AFirstPersonCharacter` production 코드는 변경하지 않는다. 현재 `CompleteFocusIn` 순서를 유지한다. 순서는 커서 표시 → widget focus 없는 `FInputModeGameAndUI`(`DoNotLock`, `HideCursorDuringCapture(false)`) → `SetFocusToGameViewport` → 중앙 `SetMouseLocation` → hit testing이다.
- 계약으로 고정하는 불변식: 컴퓨터 사용 중 실제 사용자(local player Slate user)의 키보드 focus는 game viewport에 있다. world-space 화면 widget과 그 자식에는 실제 사용자 focus를 주지 않는다. 화면 입력은 virtual user(`UWidgetInteractionComponent`)로만 들어간다.
- 회귀 방지는 5.2 자동화가 맡는다.

### 3.4 Build.cs

`Source/BathhouseSim/BathhouseSim.Build.cs`의 `PrivateDependencyModuleNames`에 `"Slate"`, `"SlateCore"`를 추가한다.

- 사용처는 5절 자동화 테스트(`SViewport`, `SVirtualWindow`, `FSlateApplication`, `FWidgetPath`, `FPointerEvent`)뿐이다.
- `Engine`의 public 의존은 include 경로만 주고 import library 링크는 보장하지 않으므로 직접 선언한다.
- production 코드에서 Slate API를 직접 쓰지 않는다는 규칙은 유지한다.

## 4. Lifecycle·rollback·전역 설정 영향

- slider 보정은 synchronous이고 상태를 새로 갖지 않는다. guard는 `TGuardValue` scope로만 존재한다.
- focus-out(CMP-005)은 기존 `ReleasePointerIfNeeded`가 drag를 끝낸다. 그 뒤 손잡이는 마지막 callback이 쓴 확정값이다. 재진입 polling이 domain과 다시 맞춘다.
- widget이 소멸하거나 선택이 바뀌어도 새 상태가 없으므로 정리할 것이 없다.
- Project/World/Input/Collision/Nav 설정은 바꾸지 않는다. 전역 회귀 영향은 Build.cs 의존 2개뿐이다. Engine이 이미 public 의존하는 모듈이라 런타임 동작 영향은 없다.

## 5. 자동화 테스트

새 테스트는 모두 `EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter`를 쓰고 `WITH_DEV_AUTOMATION_TESTS`로 감싼다.

- 600줄을 넘은 `ComputerAutomationTests.cpp`와 807줄 `BathWaterOperationsAutomationTests.cpp`에는 추가하지 않고 새 파일을 만든다.
- unity build에서 익명 namespace 이름이 충돌하지 않도록, 새 파일의 helper는 파일 고유 이름의 named namespace에 둔다. 예: `BathWaterSliderInputTest`, `ComputerKeyboardFocusTest`.
- 공유 fixture가 필요하면 기존 `FScopedBathWaterOperationsWorld`·`AddProvider`를 `Private/Tests/BathWaterOperationsAutomationTestSupport.h`의 고유 namespace로 옮겨 두 파일이 같이 쓴다. 복제는 금지한다.
- FSlateApplication이 초기화되지 않은 환경이면 `AddError`로 실패시킨다. 조용히 건너뛰지 않는다. headless `UnrealEditor-Cmd -nullrhi`도 Slate null renderer로 `FSlateApplication`을 만든다(`LaunchEngineLoop.cpp` 3107~3119행, 3343~3346행).

### 5.1 `BathhouseSim.BathWater.ManagementUI.SliderPointerDragHoldsCommittedValue` (새 파일 `Private/Tests/BathWaterSliderInputAutomationTests.cpp`)

실제 Slate 슬라이더 입력 경로로 SLD-001·002·005를 검증한다. `UWidgetInteractionComponent`가 world 화면 슬라이더를 끌 때와 같은 `FSlateApplication::RoutePointer*` + virtual user 경로다.

Fixture:

- 테스트 world와 operations subsystem.
- 순환 provider 설치 60(욕탕 최대 순환 요구 100 → 한계 60%). 욕탕 A 등록.
- 비교용 욕탕 B는 순환 0%로 등록. 다른 욕탕 불변 확인용이다.

Detail 준비:

1. `NewObject<UBathWaterDetailWidget>`에 필요한 BindWidget(`CirculationSlider`, `TargetTemperatureSlider`, `FeedbackText`, 표시 Text들)을 `NewObject`로 채운다.
2. friend로 `NativeConstruct`(또는 연결 helper)를 호출해 실제 delegate를 연결한다.
3. `SetOperationsContext`를 호출한다.
4. `Operations->GetBathSnapshot(BathA)`로 `ApplyBathSnapshot`해 선택·활성화한다.
5. root의 중첩 refresh를 재현하려고 `Operations->OnOperationsChanged`에 "욕탕 A snapshot을 다시 `ApplyBathSnapshot`" lambda를 붙인다. 테스트 끝에서 해제한다.

Slate 준비:

- `CirculationSlider->TakeWidget()`을 `SNew(SVirtualWindow).Size(FVector2D(400, 40))`의 content로 넣는다.
- `FSlateApplication::Get().RegisterVirtualWindow` 후 `SlatePrepass(1.0f)`를 실행한다.
- `FSlateApplication::Get().FindPathToWidget`으로 `FWidgetPath`를 얻는다. 실패하면 `AddError`.
- `FindOrCreateVirtualUser(<고유 index, 예 13>)`의 user index로 `FPointerEvent`를 만든다(LMB, pointer index 0).

순서와 단언:

1. 20% 위치에서 `RoutePointerDownEvent`. 순환도 20%가 commit되고 손잡이 0.2.
2. 50% 위치로 `RoutePointerMoveEvent`. domain 50%, 손잡이 0.5, 피드백 없음.
3. 90% 위치로 move(처음 한계에 닿아 60% commit). 이벤트 직후 tick 없이 손잡이 `GetValue()` = 0.6, domain 60%, 피드백에 `순환`과 부족 포인트.
4. 100% 위치로 다시 move. 이것이 버그 경로다(`Committed == Current`, broadcast 없음). 이벤트 직후 손잡이 = 0.6, domain 60%, 피드백 유지.
5. 100%에서 `RoutePointerUpEvent`. 손잡이 = 0.6, capture 해제.
6. 이어서 `ApplyBathSnapshot`을 같은 snapshot으로 다시 호출(polling 재현). 손잡이 0.6 유지.
7. SLD-005: 다시 down/move로 40%까지 내리면 domain 40%, 손잡이 0.4, 피드백 비워짐. 욕탕 B 순환도는 처음부터 끝까지 0%.

정리: pointer up 보장, `UnregisterVirtualWindow`, virtual user handle 해제, delegate 해제, `NativeDestruct`.

변경 전 코드에서는 4·5·6단계 손잡이 값이 1.0으로 남아 이 테스트가 실패해야 한다. 구현 보고에 변경 전 실패를 확인했는지, 또는 논리상 실패함을 근거와 함께 남긴다.

엔진 제약으로 headless에서 경로를 만들 수 없을 때(FindPathToWidget 실패, capture 미성립 등)는 우회하지 않는다. 로그 근거와 함께 `QNA_IMPLEMENTATION.md`에 기록하고 5.2 테스트만으로 진행할지 리뷰 판단을 받는다.

### 5.2 `BathhouseSim.BathWater.ManagementUI.SliderCommittedSyncAndFeedback` (같은 파일)

`USlider::SetValue`로 사용자 값 변경(`OnValueChanged` broadcast) 경로를 넓게 검증한다.

- SLD-001/002 순환: 위 fixture에서 `SetValue(0.9)`, 이어서 `SetValue(1.0)`. 각 호출 직후 손잡이 0.6, domain 60%, 제한 피드백 유지, 재진입 요청 없음.
  - 재진입 요청이 없다는 것은 domain revision이 한 번만 증가하고 피드백이 지워지지 않는 것으로 확인한다.
- SLD-003 가열: 가열 설치 50(실온 20°C, 5 pt/°C → 30°C 한계)에서 목표 수온 slider를 45°C 위치로 두 번 설정. 손잡이는 30°C 정규값, domain 30°C, 피드백 `가열`.
- SLD-003 냉각: 냉각 설치 25(→ 실온 아래 한계)에서 10°C 위치로 두 번 설정. 손잡이와 domain은 확정 냉각 한계, 피드백 `냉각`. 값은 0.5°C 단위로 계산해 단언한다.
- SLD-004: 순환 설치 100이지만 Operation 없는 provider(가동 0)에서 `SetValue(0.8)`. 제한 없이 domain 80%, 손잡이 0.8, 제한 피드백 없음, snapshot `bCirculationCapacityDeficit == true`.
- 실패 경로: 선택 욕탕의 condition을 subsystem에서 등록 해제(또는 욕탕 파괴)해 요청 실패를 만든다. 손잡이는 마지막 domain/cache 값으로 돌아가고 피드백은 `설정을 변경할 수 없습니다`. crash·stale reference가 없어야 한다.
- polling 안전망: 손잡이를 guard 없이 임의값으로 바꾼 상태를 직접 만든다. 같은 snapshot `ApplyBathSnapshot` 한 번에 domain 값으로 돌아오고 `PresentationWriteCount`는 증가하지 않는다.
- 기존 `NativeWidgetPresentation` 테스트의 counter 의미가 유지되는지 함께 실행한다.

### 5.3 `BathhouseSim.Computer.Input.ActiveFocusKeepsKeyboardOnGameViewport` (새 파일 `Private/Tests/ComputerKeyboardFocusAutomationTests.cpp`)

CMP-001의 깨진 지점(실제 사용자 키보드 focus가 화면 widget으로 가는 것)을 엔진 `SetInputMode` 실경로로 검증한다.

Fixture:

- 기존 컴퓨터 테스트와 같은 world, PlayerController, `ULocalPlayer`, Character, 컴퓨터를 준비한다. blend는 0, 화면은 `UComputerSampleScreenWidget`.
- harness game viewport:
  - `UGameViewportClient`를 `NewObject`(Init 호출 없음).
  - `SNew(SViewport)`.
  - `MakeShared<FSceneViewport>(ViewportClient, ViewportWidget)` 생성으로 `AddAssociation`이 `ViewportClient->Viewport`를 설정한다.
  - 테스트 world의 `FWorldContext::GameViewport`에 지정한다. `GEngine->GameViewport`·FSlateApplication의 전역 game viewport는 바꾸지 않는다.
  - scope guard가 역순으로 되돌린다: `WorldContext.GameViewport = nullptr`, FSceneViewport 해제, ViewportClient 참조 해제.

단언:

1. 음성 대조(harness가 결함을 관찰할 수 있음을 증명): `LocalPlayer->GetSlateOperations() = FReply::Unhandled()` 뒤 `FInputModeGameAndUI`에 `SetWidgetToFocus(화면 TakeWidget())`를 넣어 PlayerController에 직접 적용한다. `GetSlateOperations().GetUserFocusRecepient()`가 화면 Slate widget이다. 확인 후 reply를 다시 비운다.
2. E 진입(`InteractStartInput`, blend 0 → Active) 직후:
   - `GetSlateOperations()`에 viewport가 아닌 focus 요청이 없다(`ShouldSetUserFocus()`가 false이거나 recipient가 `ViewportWidget`).
   - recipient는 화면 user widget의 cached Slate widget이 아니다.
   - harness client의 `GetMouseCaptureMode() == CaptureDuringMouseDown`, `GetMouseLockMode() == DoNotLock`, `HideCursorDuringCapture() == false`, `IgnoreInput() == false`. GameAndUI가 실제로 적용됐음을 보인다.
3. 진입 E release(`InteractEndInput`) 뒤에도 session이 Active이고 위 상태가 유지된다(CMP-006).
4. 클릭 없이 두 번째 E(`InteractStartInput`) → session Inactive.
   - `GetSlateOperations().GetUserFocusRecepient() == ViewportWidget`(GameOnly 복구).
   - capture mode가 `CapturePermanently` 계열.
5. 같은 harness로 재진입 → `PressPointer`/`ReleasePointer`(CMP-004의 화면 클릭) → E. 결과가 4와 같다.

이 테스트는 Slate가 reply를 실제 user에 적용하는 마지막 단계(`ProcessLocalPlayerSlateOperations`)와 실제 키보드 이벤트는 실행하지 않는다. 전역 game viewport 등록과 PIE 기동이 필요하기 때문이다. 그 부분은 사용자 PIE CMP-001이 닫는다.

에이전트는 PIE를 띄우는 latent automation이나 Automation Driver 입력 주입을 만들지 않는다.

## 6. Blueprint/API·Core Redirect·Editor migration

- reflected class·property·function·BindWidget 이름 변경 없음. 추가는 private C++ 멤버와 friend뿐이다. Core Redirect 불필요.
- Content 변경 없음. `WBP_BathWaterDetail` slider는 0~1 범위·binding 없음으로 그대로 쓴다. `PROMPT_UNREAL.md`는 "Content 변경 없음"을 선언한다(Editor 단계 생략 가능 조건).

## 7. 구현 금지 범위·금지 우회

- `UBathWaterOperationsSubsystem`의 제한 계산, no-op 조기 반환, revision·broadcast 규칙 변경 금지. 가짜 broadcast·revision 증가 금지.
- slider `MinValue/MaxValue`, `MouseUsesStep`, `Locked`, step 변경으로 한계를 흉내 내지 않는다. NativeTick 보정만으로 고치지 않는다.
- WBP graph, property binding, Level/Blueprint 값으로 우회 금지. Content·Config 수정 금지.
- 컴퓨터:
  - `SetWidgetToFocus`, `UWidget::SetFocus`·`SetKeyboardFocus`·`SetUserFocus`로 화면 widget(또는 자식)에 실제 사용자 focus를 주지 않는다.
  - `FInputModeUIOnly`, `DisableInput()`, `ScreenWidget` `bReceiveHardwareInput`을 쓰지 않는다.
  - production 코드에서 `FSlateApplication`을 직접 사용하지 않는다.
  - `UPlayerComputerUseComponent`·`AFirstPersonCharacter` 동작 변경 금지(이 작업 범위 밖).
- 휠 입력(`COMPUTER-WHEEL-SCROLL`) 구현 금지.
- 메인 작업 트리(`C:/UnrealProjects/BathhouseSim`) 수정·빌드 금지. 모든 작업은 worktree `C:/UnrealProjects/BathhouseSim/.claude/worktrees/bug-0925`에서 한다.

## 8. 빌드·Automation·리뷰 기준

빌드(worktree 프로젝트 경로; 공통 정책의 진입점과 옵션 그대로):

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  BathhouseSimEditor Win64 Development `
  -Project='C:\UnrealProjects\BathhouseSim\.claude\worktrees\bug-0925\BathhouseSim.uproject' `
  -WaitMutex -NoHotReloadFromIDE
```

Automation(같은 worktree 프로젝트, 공통 headless 정책 옵션):

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  'C:\UnrealProjects\BathhouseSim\.claude\worktrees\bug-0925\BathhouseSim.uproject' /Engine/Maps/Templates/Template_Default `
  -unattended -nullrhi -NoSplash -NoSound -DDC-ForceMemoryCache `
  -ExecCmds="Automation RunTests BathhouseSim.BathWater+BathhouseSim.Computer+BathhouseSim.Interaction.HeldTargetUse.InputOwners; Quit" -TestExit="Automation Test Queue Empty" `
  -ReportExportPath='C:\UnrealProjects\BathhouseSim\.claude\worktrees\bug-0925\Saved\Automation\Reports\2026-10-01\bug-0925' -log
```

- 필터에 포함되는 것: 새 3개 테스트, 기존 `BathhouseSim.BathWater.*`(widget presentation·request atomicity 회귀), `BathhouseSim.Computer.*`(focus session·Blueprint load 회귀).
- `BathhouseSim.Interaction.HeldTargetUse.InputOwners`(`FBathhouseHeldTargetUseOwnerRoutingTest`): 컴퓨터 LMB owner 우선순위 회귀.
- 이 작업에서 `ServiceAmenity` 세신 포커스 테스트는 회귀 대상이 아니다. Character를 바꾸지 않는다.

코드 리뷰 기준:

- slider callback이 같은 이벤트 안에서 확정값을 쓰는가.
- 재진입 guard가 보정 쓰기의 재요청을 막는가.
- polling 동기화가 cache gate보다 앞에 있는가.
- 피드백 유지·해제 규칙이 그대로인가.
- 다른 욕탕 불변인가.
- 컴퓨터 production 경로가 그대로이고 새 테스트에 음성 대조가 있는가.
- 테스트 helper namespace 충돌이 없는가.
- Build.cs 변경이 테스트 use site와 일치하는가.

## 9. 사용자 PIE 관찰 항목

선행: worktree build를 사용자 Editor에 반영한다(병합 후 메인 트리 build 등 마스터가 정한 방법).

1. SLD-001·002: 순환 설치 정격용량을 100% 미만이 되게 둔다(순환기 1대 등). 순환도 슬라이더를 끝까지 끌고 계속 오른쪽으로 움직인다.
   - 손잡이가 한계에서 멈춰 있다.
   - `순환 용량 제한: N 포인트 부족`이 보인다.
   - 놓은 뒤 손잡이와 `순환도` 수치가 같다.
   - 기록: 설치·가동·예약 값, 확정값.
2. SLD-003: 목표 수온을 실온 위로(가열 부족), 실온 아래로(냉각 부족) 각각 같은 방법으로 확인한다. 손잡이는 0.5°C 단위 확정값이다.
3. SLD-004: 설비를 비가동(가동수치 0)으로 둔 채 설치용량 안에서 올린다. 제한 없이 설정되고 용량 요약·상세에 가동 부족이 표시된다. 설비를 가동하면 효과가 재개된다.
4. SLD-005: 한계에서 아래로 끌면 손잡이가 따라오고 제한 문구가 사라진다. 다른 욕탕 타일·설정은 그대로다.
5. SLD-006/CMP-005: 한계 너머로 끄는 중 E로 나간다. 다시 들어가면 손잡이가 확정값이다.
6. CMP-001: 컴퓨터를 조준해 E로 들어간다. 커서가 보인 뒤 아무것도 클릭하지 않고 E를 누른다. 고정 위치·방향으로 나온다. 3회 반복한다.
7. CMP-006: E를 누른 채 전환을 기다렸다가 뗀다. 나가지 않는다. 이어서 E 한 번으로 나간다.
8. CMP-004: 탭·타일·슬라이더를 클릭한 뒤 E로 나간다. 재진입 시 선택·값이 유지된다.

ESC(CMP-002)는 이 작업의 관찰 대상이 아니다. PIE에서는 Editor의 PIE 종료 단축키와 겹칠 수 있다.

### 조건부 진단 (6번 CMP-001이 이 build에서도 실패할 때만)

PIE 중 사용자가 확인한다(Tools > Debug > Widget Reflector).

- 포커스인 직후 키보드 focus(User 0 Focus) 경로가 game viewport인지, `WidgetComponent` virtual window의 화면 widget인지.
- 콘솔 `showdebug enhancedinput`으로 E가 `IA_Interact`에 도달하는지.

결과를 첨부해 아키텍처로 복귀한다. Editor 진단 역할의 읽기 전용 조사 대상(실행 수단 미지정):

- `WBP_ComputerScreenRoot`, `WBP_BathWaterManagementScreen`·하위 4개, `/Game/Bathhouse/UI/Shop/*` WBP의 Event Graph focus·input mode 노드.
- 각 root·자식의 `IsFocusable`.
- `BP_FirstPersonController`·`BP_FirstPersonCharacter` graph의 `SetInputMode`·focus 노드.
- `BP_BathhouseComputer.ScreenWidget`의 `bReceiveHardwareInput`·`WindowFocusable`·현재 `WidgetClass`.

## 10. 관련 작업 경계 — `COMPUTER-WHEEL-SCROLL`

이 작업은 입력 모드·focus 경로를 바꾸지 않는다. 휠 작업이 확장할 현재 경계는 다음과 같다(정본: `ComputerSystem.md` Input Routing).

- 실제 사용자 focus는 game viewport에 있다. 휠도 `FSceneViewport` → PlayerInput → Enhanced Input으로 들어온다.
- Character가 컴퓨터 phase로 분기해 `UPlayerComputerUseComponent`에 전달한다. component가 `UWidgetInteractionComponent`(virtual user)로 화면에 주입한다.
- 화면 widget에 실제 사용자 focus나 hardware input을 주는 방식으로 확장하지 않는다.

## 11. 복귀 재설계 여부

복귀 재설계가 아니다. 유지되는 완료 범위: CMP-001~020 production 구현(`61f72c1`), 욕탕 운영 domain·관리 화면 구조.
