# PROMPT_IMPLEMENTATION — COMPUTER-WHEEL-SCROLL 컴퓨터 화면 스크롤 영역 마우스 휠 스크롤

- 작업 ID: `COMPUTER-WHEEL-SCROLL`
- 단계: 아키텍처
- 상태: 완료

## 0. 착수 조건과 작업 위치

- 구현은 관련 BUG 작업 `BUG-2026-09-25_bath_water_slider_overrun_and_computer_focus_out`의 구현이 main에 병합되고, 이 작업 브랜치(`work/COMPUTER-WHEEL-SCROLL`)에 그 main이 병합된 뒤 시작한다.
- 시작 전 확인한다. 하나라도 없으면 구현하지 않고 마스터에게 보고한다.
  - `Source/BathhouseSim/BathhouseSim.Build.cs`의 `PrivateDependencyModuleNames`에 `"Slate"`, `"SlateCore"`가 있다.
  - `Source/BathhouseSim/Private/Tests/ComputerKeyboardFocusAutomationTests.cpp`가 있다.
- 작업 위치는 마스터가 지정한 이 작업의 worktree다. 아래 명령은 그 절대 경로를 `$Worktree`로 쓴다. 아키텍처 단계 기준값은 `C:\UnrealProjects\BathhouseSim\.claude\worktrees\wheel`이다. 메인 트리와 다른 worktree는 수정·빌드하지 않는다.

## 1. 기능 계약과 시나리오

입력 계약은 같은 폴더의 `PROMPT_ARCHITECTURE.md`(CWS-001~020, 2026-10-01 사용자 승인)와 `QNA_FEATURE_SPEC.md`(Q1~Q4 A, P1~P12, S1~S3 A), `REPORT_UNREAL_DISCOVERY.md`다. 시나리오 문장은 `PROMPT_ARCHITECTURE.md` 5절이 정본이다. 아래 표는 각 시나리오를 어느 계층이 보장하고 무엇이 검증하는지만 적는다.

| ID | 보장 계층 | 자동화 | PIE |
|---|---|---|---|
| CWS-001 (대표), 002 | 휠 경로(4.1) + 엔진 `SScrollBox` | A, B | O |
| CWS-003 | Active phase gate(클릭 불필요) | B | O |
| CWS-004 | Slate bubble, `SButton` 휠 무처리 | A | O |
| CWS-005 | 엔진 휠 경로 overscroll 금지, 끝이면 Unhandled | A | O |
| CWS-006 | `ConsumeMouseWheel=WhenScrollingPossible` | A, C | O |
| CWS-007, 008 | 커서 아래 경로만 bubble(형제 영역 불변) | A | O |
| CWS-009, 010 | `SSlider` 휠 무처리 → `DetailScroll` | A, C | O |
| CWS-011 | 스크롤 영역 조상 없음 → 무처리 | A | O |
| CWS-012 | hover gate(화면 밖이면 주입 없음) | B | O |
| CWS-013 | Active phase gate | B | O |
| CWS-014 | widget 미파괴(기존 계약), native 스크롤 조작 없음 | — | O |
| CWS-015 | 휠은 hover 경로를 바꾸지 않음, 클릭은 기존 경로 | — | O |
| CWS-016 | 휠 경로가 실제 사용자 focus·input mode를 건드리지 않음 | A, B | O |
| CWS-017, 018 | Character 휠 분기(4.2) | B | O |
| CWS-019 | Slate bubble 중첩 규칙(P4) | A | 대상 아님 |
| CWS-020 | 기존 Character 차단 유지 | 기존 테스트 | O |

자동화 A·B·C는 7절의 세 테스트다.

## 2. 현재 단계

단일 작업으로 전체 범위(Q4 A)를 구현한다. 수직 구현·전체 확장 구분이 없다. 대표 시나리오는 CWS-001(상점 상품 목록)이다. 범위는 컴퓨터 화면의 스크롤 영역 4개(`ProductScroll`, `CartScroll`, `OrderScroll`, `DetailScroll`)와 앞으로 추가될 컴퓨터 화면 스크롤 영역 전부다. 영역별 코드가 없으므로 영역 추가에 코드 변경이 필요 없다.

## 3. 목적·수용 기준·비목표

목적: 컴퓨터 포커스 Active 동안 실제 마우스 휠 양을 플레이어의 world-space 화면 가상 포인터(`UWidgetInteractionComponent`)로 주입한다. 그러면 커서 아래의 가장 안쪽 스크롤 영역이 엔진 규칙대로 스크롤된다. 컴퓨터를 쓰지 않을 때의 휠(배치 회전)은 그대로다.

수용 기준:

1. 1절 표의 자동화 A·B·C가 통과한다. 기존 `BathhouseSim.Computer.*`, held-use 입력 owner, 세신 입력, Placement 회귀가 통과한다(8절 필터).
2. 사용자 PIE 12절 항목이 통과한다.
3. Content·Config 변경이 없다. `git status`에 `Content/`, `Config/` 변경이 없다.
4. 컴퓨터 사용 중 실제 사용자 keyboard focus 불변식([ComputerSystem.md](../../Architecture/ComputerSystem.md) Keyboard Focus Invariant)을 깨지 않는다.

비목표: `PROMPT_ARCHITECTURE.md` 11절 그대로다. 범용 desktop·창·text input, 키보드·드래그 관성 스크롤, 가로 스크롤, 스크롤 위치 저장, 세신 포커스 휠, 슬라이더 휠 조정, 영역 밖 휠로 주 목록 스크롤, BUG 현상 1·2 수정, 레이아웃·스타일 변경. 추가로 한 칸 이동량의 C++ 설정값 신설, 상점·관리 Widget의 휠 코드도 만들지 않는다.

## 4. 설계

### 4.1 휠 입력 전달 경로

```text
실제 마우스 휠
→ Slate가 실제 커서 아래 widget으로 라우팅(키보드 focus와 무관)
→ game viewport `FSceneViewport::OnMouseWheel` → `InputAxis(EKeys::MouseWheelAxis, WheelDelta)`
→ Enhanced Input: `IMC_FirstPerson`의 `MouseWheelAxis → IA_PlacementRotate`(Axis1D, modifier·trigger 없음 → 값이 0이 아니면 Triggered)
→ `AFirstPersonCharacter::MouseWheelInput(Value)`(신규 이름, 4.2 분기)
   ├ computer session capture 중 → `UPlayerComputerUseComponent::ScrollPointerWheel(Delta)` → 끝(배치로 가지 않음)
   ├ 그 밖의 포커스 capture(세신) → 무시
   └ 그 밖 → `UPlayerFacilityPlacementComponent::AddRotationInput(Delta)`(기존)
→ `ScrollPointerWheel`: Active·hit testing·hover gate(4.3) 통과 시에만 `UWidgetInteractionComponent::ScrollWheel(Delta)`
→ 엔진: `LastWidgetPath`(마지막 hover 경로)로 virtual user `FPointerEvent(MouseWheelAxis, Delta)`를 `FSlateApplication::RouteMouseWheelOrGestureEvent`(bubble)
→ 커서 아래 가장 안쪽 `SScrollBox::OnMouseWheel`: 스크롤 가능하면 `ScrollBy(-Delta × Slate.GlobalScrollAmount × WheelScrollMultiplier, overscroll 금지, 애니메이션=WBP 값)`
   실제로 움직였으면 Handled, 아니면 Unhandled로 바깥 영역에 넘김
```

엔진 근거(UE 5.8):

- `Engine/Source/Runtime/Engine/Private/Slate/SceneViewport.cpp` 946~970행. 휠은 `InputAxis(MouseWheelAxis, GetWheelDelta())`다. 위로 굴리면 +이다.
- `Plugins/EnhancedInput/Source/EnhancedInput/Private/EnhancedPlayerInput.cpp` 241행. trigger가 없으면 `IsNonZero`로 Triggered다. 트랙패드의 작은 양도 그대로 전달된다.
- `Runtime/UMG/Private/Components/WidgetInteractionComponent.cpp`
  - 713~734행 `ScrollWheel`: `LastWidgetPath`, 같은 virtual user, `EKeys::MouseWheelAxis`.
  - 424~462행 `SimulatePointerMovement`: hit testing이 켜져 있을 때 매 tick hover 경로를 갱신한다. 맞은 widget이 없으면 경로를 비운다.
  - 556~558행 `ReleasePointerKey`: 경로를 비운다.
- `Runtime/Slate/Private/Widgets/Layout/SScrollBox.cpp`
  - 1158~1180행 `OnMouseWheel`.
  - 1214~1243행 `ScrollBy`: 범위 clamp. 움직였을 때만 handled.
  - 496행 `GetScrollOffset`: DesiredScrollOffset.
- `Runtime/Slate/Private/Framework/Application/SlateApplication.cpp` 6244행 이후 `RouteMouseWheelOrGestureEvent`: 같은 pointer의 capture가 있으면 captor 경로를 쓴다. 이벤트는 bubble로 전달된다.

방향(P2): Character와 component는 값을 그대로 넘긴다. 부호 반전·배율·clamp를 하지 않는다. 휠 아래(음수) → `ScrollBy(+)` → 아래쪽 내용이 올라온다.

비례(4.6): 한 frame에 여러 칸이면 Enhanced Input 값이 합산돼 한 번 호출된다(예: 2.0 → 64 unit). 트랙패드 0.25 → 8 unit.

### 4.2 기존 `IA_PlacementRotate`와의 분리

결정: 새 Input Action을 만들지 않는다. 기존 휠 mapping 하나를 Character가 입력 owner별로 분기하는 **공용 휠 intent**로 쓴다. 이는 E(`InteractAction`), ESC(`CancelAction`), LMB(`PrimaryUseAction`)가 이미 쓰는 "한 입력 → Character가 owner 분기" 패턴과 같다.

분리 규칙:

| 상태 | 휠이 닿는 곳 | 닿지 않는 곳 |
|---|---|---|
| computer phase가 `Inactive`가 아님 | `ScrollPointerWheel`(Active일 때만 실제 주입) | Placement 회전 |
| 세신 포커스 capture | 없음(소비) | Placement, computer |
| 그 밖 | Placement 회전(기존, 배치 중이 아니면 no-op) | computer 화면 |

- 컴퓨터는 빈손일 때만 진입하고 배치는 설비 아이템을 들어야 하므로 두 owner가 동시에 유효하지 않다. 그래도 분기 순서로 상호 배타를 보장한다.
- 이름 정리: Character의 protected 비-reflected handler `PlacementRotateInput`을 `MouseWheelInput`으로 바꾼다. reflected property `PlacementRotateAction`(이름·category·flag)은 Content 계약이므로 그대로 둔다.

거부한 대안:

- 새 `IA_ComputerScroll`·IMC mapping 추가: Content 3개(IA 신규, `IMC_FirstPerson`, `BP_FirstPersonCharacter`)를 바꿔야 한다. 같은 키에 두 action이 모두 Triggered되므로 두 handler가 각각 같은 mode gate를 중복으로 가져야 한다. 관찰 결과 이득은 없다. 승인 기대(Content 변경 없음)와 S2 경계도 넘는다.
- legacy `BindAxisKey`/`BindKey(EKeys::MouseWheelAxis)`, PlayerController `InputKey` override: 프로젝트의 Enhanced Input intent 규칙과 IMC remap 경로를 우회한다.
- `ScreenWidget` `bReceiveHardwareInput`, 화면에 실제 사용자 focus 부여: Keyboard Focus Invariant 위반이다. LMB가 이중으로 전달된다.
- production에서 `FSlateApplication`으로 wheel 이벤트를 직접 route: 이미 같은 일을 하는 공개 엔진 API(`ScrollWheel`)가 있다. production Slate 직접 사용 금지 규칙에도 어긋난다.

Content 전제(자동화 C가 지킴): `IA_PlacementRotate`는 Axis1D이고 modifier·trigger가 없다. `IMC_FirstPerson`에서 휠 키를 쓰는 mapping은 이것 하나다. 앞으로 회전 감도·스냅 같은 modifier가 필요하면 IA에 넣지 않는다. Placement 설정(`UFacilityPlacementSettings::RotationStepDegrees`)이나 별도 설계로 해결한다. 이 제약은 Unreal 정본에도 기록한다(9절).

### 4.3 커서 아래 영역 판정, 중첩, 슬라이더

- 영역 판정은 엔진 hover 경로와 Slate bubble에 맡긴다. 프로젝트 코드는 어떤 ScrollBox도 이름·타입으로 찾지 않는다. 4개 영역과 이후 추가 영역(ListView·TileView 포함)이 같은 규칙을 따른다.
- 영역 밖(CWS-011): 경로에 `OnMouseWheel`을 처리하는 조상이 없으면 Unhandled로 끝난다. 현재 Source의 화면 widget에는 `NativeOnMouseWheel` override가 없다. 앞으로도 추가하지 않는다(6절).
- 화면 밖(CWS-012): world trace가 화면을 맞히지 않으면 hovered component가 null이다. `ScrollPointerWheel`이 주입하지 않는다.
- 중첩(P4, CWS-019): 엔진 `SScrollBox`는 실제로 움직였을 때만 Handled를 반환하고 끝이면 Unhandled로 넘긴다. 따라서 "안쪽 우선, 끝이면 바깥, 한 칸에 둘 중 하나만"이 엔진 동작으로 성립한다.
- 슬라이더(Q3 A, CWS-010): `SSlider`에는 `OnMouseWheel`이 없어 휠이 `DetailScroll`로 올라간다. 슬라이더 drag 중(virtual pointer capture)에는 captor 경로(슬라이더 → `DetailScroll`)로 bubble해 같은 결과다. 휠은 슬라이더 값을 쓰지 않는다. 그 뒤 마우스 이동으로 값이 바뀌는 것은 기존 drag 동작이다.
- 형제 영역(CWS-007, 008): bubble은 커서 아래 경로만 지난다. `CartScroll`·`OrderScroll`이 서로 영향을 주지 않는다. 주문 행 재생성(`RefreshOrders` → `ClearChildren`)으로 경로가 끊겨도 `FWeakWidgetPath::ToWidgetPath` 기본값 `Truncate`가 살아 있는 조상(`OrderScroll`)까지 경로를 보존한다. 남은 시간 갱신(`RefreshOrderCountdowns`)은 행을 재생성하지 않는다.

hover gate 이유: `bEnableHitTesting=false`이면 엔진은 `LastWidgetPath`·hovered component를 갱신하지 않는다. 이전 세션이나 다른 컴퓨터 화면의 낡은 경로가 남을 수 있다. 그래서 `ScrollPointerWheel`은 Active, hit testing 켜짐, hovered component == 현재 컴퓨터의 `ScreenWidget`을 모두 요구한다.

허용하는 1 frame 한계:

- Active 전환 직후 첫 hover 갱신 전, 또는 LMB release 직후(`ReleasePointerKey`가 경로를 비움)의 같은 frame 휠은 낡은 경로나 빈 경로로 간다. 빈 경로는 아무 일도 하지 않는다.
- Character 입력(PlayerController tick)이 WidgetInteraction tick보다 먼저 실행되므로 hover는 직전 frame 기준이다.
- 엔진 protected API(`SimulatePointerMovement`)를 우회 호출하지 않는다. subclass로 hover를 강제 갱신하지도 않는다.

### 4.4 한 칸당 이동량 authoring

- 단일 정본: 각 `UScrollBox`의 `WheelScrollMultiplier`(WBP authoring, 현재 4개 모두 1.0). 실제 이동량은 `Slate.GlobalScrollAmount`(엔진 전역 cvar, 기본 32) × multiplier × 휠 양이다. 단위는 그 ScrollBox의 local UI unit이다.
- C++ 설정값, DeveloperSettings, component property를 새로 만들지 않는다. 같은 의미의 값이 두 곳에 생기기 때문이다. `Slate.GlobalScrollAmount`는 Editor UI를 포함한 모든 Slate에 영향을 주므로 이 기능을 위해 바꾸지 않는다.
- `AnimateWheelScrolling=false`가 "애니메이션 없이 그 frame에 반영"(P3, 4.2) 계약이다. 자동화 C가 지킨다. 영역별 감각 조정은 PIE 뒤 Editor 단계에서 해당 ScrollBox의 multiplier만 바꾼다.

### 4.5 책임 변화

| 항목 | 판단 |
|---|---|
| 기존 책임 | Character: 휠 → Placement 회전, computer capture 중 휠 차단. `UPlayerComputerUseComponent`: Active 동안 virtual pointer LMB press/release 주입 |
| 신규 책임 | Character: 휠 owner 분기(Computer > 세신 소비 > Placement). Component: Active 동안 virtual pointer 휠 주입 |
| 상태 owner | 없음(신규 runtime 상태 없음). 스크롤 위치는 엔진 `SScrollBox`의 표시 상태이며 widget 수명(컴퓨터 Actor 수명) 동안 유지 |
| 실행 owner | `UPlayerComputerUseComponent`(phase·hit testing·hover gate와 주입) |
| authoring owner | WBP의 각 `UScrollBox` `WheelScrollMultiplier`·`ConsumeMouseWheel`·`AnimateWheelScrolling` |
| 의존 방향 | 변경 없음(Character → Computer → UMG `UWidgetInteractionComponent`). 테스트만 Slate·SlateCore·EnhancedInput 직접 사용 |
| 분리 후보 | 별도 wheel router component, `UWidgetInteractionComponent` subclass, private helper |
| 최종 판단 | 기존 두 타입 확장. 휠 주입은 같은 component의 press/release 주입과 같은 책임·같은 gate다. 새 상태·Tick·delegate가 없다. 20줄 남짓을 분리하면 CoreSystem이 금지한 LOC용 wrapper가 된다. Character(604줄, 경고선 초과)에는 입력 분기만 넣는다(CoreSystem 4단위 규칙과 같음). `UPlayerComputerUseComponent`는 487줄에서 약 520줄이 된다. 추가는 4.1의 두 함수로 한정한다 |

### 4.6 Lifecycle·rollback·전역 설정

- 휠 주입은 동기 단발 호출이다. 보관 상태·timer·delegate가 없어 정리할 것이 없다.
- `ForceCleanup`·`HandleComputerUnavailable`·EndPlay 뒤 phase는 `Inactive`다. 다음 휠은 Character 분기에서 Placement 경로로 간다(8절 복구 계약). component가 소멸했거나 null이면 Character는 Placement 경로만 쓴다.
- FocusingIn/FocusingOut(CWS-013): Character가 소비하고 component가 Active가 아니어서 주입하지 않는다.
- 휠은 `SetInputMode`, `bShowMouseCursor`, focus API, phase를 건드리지 않는다. E·ESC 결과가 같다(CWS-016).
- Project/World/Input/Collision/Nav 설정 변경 없음. 전역 회귀 범위는 Character의 휠 분기(Placement 회전 경로)뿐이다.

## 5. 변경 파일·API

### 5.1 `Public/Computer/PlayerComputerUseComponent.h` / `Private/Computer/PlayerComputerUseComponent.cpp`

public(C++ 전용, UFUNCTION 아님):

```cpp
// Active 동안 커서 아래 화면 경로로 휠 양을 virtual pointer에 주입한다. 주입했으면 true.
bool ScrollPointerWheel(float WheelDelta);
```

private:

```cpp
bool CanInjectPointerWheel(float WheelDelta, const UWidgetComponent* HoveredComponent) const;
friend class FBathhouseComputerWheelRoutingTest;
```

`CanInjectPointerWheel`은 다음을 모두 만족할 때만 true다.

1. `Phase == EPlayerComputerUsePhase::Active`
2. `IsValid(WidgetInteraction)`이고 `WidgetInteraction->bEnableHitTesting`
3. `FMath::IsFinite(WheelDelta)`이고 `!FMath::IsNearlyZero(WheelDelta)`
4. `ActiveComputer`가 유효하고 `GetScreenWidget()`이 유효
5. `HoveredComponent != nullptr`이고 `HoveredComponent == ActiveComputer->GetScreenWidget()`

`ScrollPointerWheel`의 동작:

- `CanInjectPointerWheel(WheelDelta, WidgetInteraction ? WidgetInteraction->GetHoveredWidgetComponent() : nullptr)`가 false면 false를 반환한다.
- 통과하면 `WidgetInteraction->ScrollWheel(WheelDelta)` 후 true를 반환한다.
- 값을 바꾸지 않는다. 상태·로그·UPROPERTY·Tick 변경이 없다. 반환값은 Character가 쓰지 않으며 테스트·진단용이다.
- `UWidgetComponent` forward 선언과 `Components/WidgetComponent.h` include(cpp, 이미 있음)를 확인한다.

### 5.2 `Public/Character/FirstPersonCharacter.h` / `Private/Character/FirstPersonCharacter.cpp`

- `void PlacementRotateInput(const FInputActionValue&)` → `void MouseWheelInput(const FInputActionValue& Value)`로 바꾼다(protected, 비-reflected). `SetupPlayerInputComponent`의 `PlacementRotateAction` Triggered binding 대상만 바꾼다.
- 본문:

```cpp
const float WheelDelta = Value.Get<float>();
if (PlayerComputerUse && PlayerComputerUse->IsCapturingInput())
{
    PlayerComputerUse->ScrollPointerWheel(WheelDelta);
    return;
}
if (IsFocusCapturingInput())
{
    return;
}
if (PlayerFacilityPlacement)
{
    PlayerFacilityPlacement->AddRotationInput(WheelDelta);
}
```

- `friend class FBathhouseComputerWheelRoutingTest;`를 추가한다.
- `PlacementRotateAction` property, 다른 handler, press owner 로직은 바꾸지 않는다.

### 5.3 `Public/Placement/PlayerFacilityPlacementComponent.h`

- `friend class FBathhouseComputerWheelRoutingTest;`만 추가한다. 테스트가 `PreviewFacility`, `AccumulatedYaw`를 읽고 쓰기 위해서다. 동작 변경은 없다.

### 5.4 신규 테스트 `Private/Tests/ComputerWheelScrollAutomationTests.cpp`

7절 세 테스트를 둔다. helper는 파일 고유 named namespace `ComputerWheelScrollTest`에 둔다.

- unity build에서 `ComputerAutomationTests.cpp`의 익명 namespace(`FScopedComputerAutomationWorld` 등)나 BUG 작업 테스트 namespace와 이름이 겹치면 안 된다.
- 컴퓨터 world fixture는 BUG 병합 뒤 공유 support header가 있으면 재사용한다. 없으면 `ComputerAutomationTests.cpp`의 `FScopedComputerAutomationWorld`·`BeginActorForComputerTest`를 `Private/Tests/ComputerAutomationTestSupport.h`의 named namespace `ComputerAutomationTestSupport`로 옮겨 두 파일이 같이 쓴다. 복제는 금지한다. 옮길 때 기존 테스트 동작·이름은 바꾸지 않는다.

### 5.5 바꾸지 않는 것

- `BathhouseSim.Build.cs`: BUG 작업이 넣은 `Slate`·`SlateCore`를 그대로 쓴다. `EnhancedInput`은 이미 public이다.
- `ABathhouseComputerActor`
- 모든 `Public/UI`·`Private/UI` widget
- `UPlayerScrubFocusComponent`
- Config·Content

## 6. 구현 금지 범위·금지 우회

- 화면 widget(또는 자식)에 실제 사용자 focus를 주지 않는다: `SetWidgetToFocus`, `UWidget::SetFocus`·`SetKeyboardFocus`·`SetUserFocus`. `FInputModeUIOnly`, `DisableInput()`, `ScreenWidget` `bReceiveHardwareInput`도 쓰지 않는다. `CompleteFocusIn`·`RequestEndComputerUse` 순서는 바꾸지 않는다.
- production 코드에서 `FSlateApplication`·Slate API를 직접 쓰지 않는다. 휠 주입은 `UWidgetInteractionComponent::ScrollWheel`만 쓴다. `UWidgetInteractionComponent` subclass를 만들지 않는다. protected `SimulatePointerMovement`를 우회 호출하지 않는다.
- 화면·상점·관리 widget에 `NativeOnMouseWheel` override를 만들지 않는다. C++에서 ScrollBox를 찾아 `SetScrollOffset`·`ScrollToStart/End`·`ScrollWidgetIntoView`로 스크롤을 흉내 내지 않는다. 영역 이름 목록을 하드코딩하지 않는다.
- 슬라이더에 휠 처리를 추가하지 않는다(Q3 A).
- 새 Input Action, IMC mapping, BP property 할당을 만들지 않는다. legacy `BindAxisKey`·`BindKey`, PlayerController `InputKey` override를 쓰지 않는다. `PlacementRotateAction` 등 reflected 이름을 바꾸지 않는다.
- 이동량용 C++ 설정을 신설하지 않는다. `Slate.GlobalScrollAmount`를 바꾸지 않는다. ScrollBox property를 runtime에 덮어쓰지 않는다.
- WBP graph, Level·Blueprint 값, Content·Config 수정으로 우회하지 않는다. Content 수정이 필요해 보이면 멈추고 보고한다(S2 A).
- BUG 작업 소유 파일(`BathWaterDetailWidget.*`, `BathWaterSliderInputAutomationTests.cpp`, `ComputerKeyboardFocusAutomationTests.cpp`, `Build.cs`)을 수정하지 않는다. BUG 현상 1·2를 고치지 않는다.
- PIE를 띄우는 latent automation이나 Automation Driver 입력 주입을 만들지 않는다.
- domain(cart·wallet·order·bath setting) 호출을 휠 경로에 추가하지 않는다.

## 7. 자동화 테스트

모든 테스트는 `WITH_DEV_AUTOMATION_TESTS`로 감싸고 `EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter`를 쓴다. `FSlateApplication`이 초기화되지 않았거나 harness를 만들 수 없으면 `AddError`로 실패시킨다. 조용히 건너뛰지 않는다(headless `-nullrhi`도 null renderer로 `FSlateApplication`을 만든다).

### 7.1 A — `BathhouseSim.Computer.Input.WheelScrollsHoveredScrollBoxThroughSlate` (실제 Slate 경로)

엔진 `UWidgetInteractionComponent::ScrollWheel`이 보내는 것과 같은 이벤트를 virtual user로 `FSlateApplication::RouteMouseWheelOrGestureEvent`에 보낸다. 실제 UMG `UScrollBox`·`USlider`·`UButton`의 휠 결과를 검증한다.

- world `UWidgetComponent` hover는 headless에서 만들 수 없다. `-nullrhi`면 `UWidgetComponent::DrawWidgetToRenderTarget`이 `GUsingNullRHI`에서 반환해 hit-test grid가 비기 때문이다(`WidgetComponent.cpp` 1389행).
- 그래서 경로는 `FindPathToWidget`으로 만들고, 이벤트 구성은 엔진 `ScrollWheel`과 같게 한다.

Fixture(UMG widget을 `NewObject`로 만들고 테스트 동안 GC에서 보호):

- root `UHorizontalBox`, 크기는 1024×576 virtual window.
  - `ProductScroll`: 기본값 `UScrollBox`(Vertical). 안에 `UVerticalBox`와 고정 높이(`USizeBox` 100) 항목 20개. 첫 항목 안에 click 횟수를 세는 `UButton`(담기 버튼 모델).
  - 가운데 column `UVerticalBox`: `CartScroll`(Fill 0.55), `OrderScroll`(Fill 0.45) 형제. 처음에는 Cart 내용이 영역보다 작고, Order 내용은 넘친다.
  - `DetailScroll`: 넘치는 내용, `USlider`(값 0.3, `OnValueChanged` 횟수 기록), 중첩 검증용 `InnerScroll`(`USizeBox` 높이 100 안의 `UScrollBox`, 내용 300).
  - 어떤 ScrollBox에도 속하지 않는 `UButton`(탭 버튼 모델, click 횟수 기록).
- ScrollBox 휠 설정은 엔진 기본값으로 둔다. 이는 사전 조사의 4개 영역 값과 같다: `WhenScrollingPossible`, 1.0, 애니메이션 꺼짐.

Slate 준비:

1. `SNew(SVirtualWindow).Size(FVector2D(1024, 576))`에 root `TakeWidget()`을 넣는다.
2. `FSlateApplication::Get().RegisterVirtualWindow` → `SlatePrepass(1.0f)`.
3. ScrollBox의 스크롤 가능 상태(`SScrollBar` state)는 Slate tick에서만 정해진다. 엔진 경로로 만든다.
   - 우선: `FSlateWindowElementList`로 `SWindow::PaintWindow`.
   - 대안: `FindPathToWidget`으로 얻은 각 `SScrollBox`의 arranged geometry로 public `SWidget::Tick` 호출.
   - `SetScrollOffset` 등으로 상태를 흉내 내지 않는다.
   - 준비 뒤 `ProductScroll`·`OrderScroll`·`DetailScroll`·`InnerScroll`의 `GetScrollOffsetOfEnd() > 0`, `CartScroll`은 0을 단언한다.
4. `FindOrCreateVirtualUser(<이 파일 고유 index, BUG 테스트의 13과 다른 값>)`.
5. helper `RouteWheel(UWidget* Target, float Delta) -> bool(Handled)`:
   - `FindPathToWidget(Target->GetCachedWidget())`로 경로를 만든다. 실패하면 `AddError`.
   - `FPointerEvent(VirtualUserIndex, 0, Target 중심 절대 좌표, 같은 좌표, {}, EKeys::MouseWheelAxis, Delta, FModifierKeysState())`.
   - `RouteMouseWheelOrGestureEvent(Path, Event, nullptr).IsEventHandled()`.
   - 내용 추가 뒤에는 prepass·tick을 다시 한다.
6. `Step = Slate.GlobalScrollAmount cvar 값 × 1.0`. `Step == 32`도 단언한다(4.6 기본값 계약).

순서와 단언(각 단계에서 언급하지 않은 ScrollBox offset은 직전 값 그대로임을 함께 단언):

1. CWS-001: `ProductScroll` 두 번째 항목에 -1. Handled, Product offset == Step.
2. 비례: -3 → +3×Step. -0.25 → +0.25×Step.
3. CWS-002: +1 → -Step.
4. CWS-004: 첫 항목의 `UButton`에 -1. Product가 움직이고 버튼 click 횟수 0.
5. CWS-005:
   - 큰 값(-1000)으로 끝까지 → offset == `GetScrollOffsetOfEnd()`.
   - 이어서 -1 → Unhandled, offset 그대로.
   - +1 → 즉시 end - Step. 맨 위에서 +1도 Unhandled, 0 유지.
6. CWS-006: `CartScroll` 내용에 -1 → Unhandled, 모든 offset 불변.
7. CWS-008: `OrderScroll` 내용에 -1 → Order만 Step.
8. CWS-007: Cart에 항목을 추가해 넘치게 하고 prepass·tick을 다시 한다. Cart에 -1 → Cart만 Step, Order·Product 불변.
9. CWS-009·010:
   - `DetailScroll` 문구에 -1 → Detail Step.
   - `USlider`에 -1 → Detail이 다시 Step 이동, 슬라이더 `GetValue()` 0.3 그대로, `OnValueChanged` 0회.
10. CWS-011: 바깥 `UButton`에 -1 → Unhandled, 모든 offset 불변, click 0회.
11. CWS-019(P4):
    - `InnerScroll` 내용에 -1 → Inner만 Step, Detail 불변.
    - Inner를 끝까지 보낸 뒤 -1 → Detail만 Step, Inner 불변.
    - 모든 단계에서 한 이벤트로 두 offset이 동시에 바뀌지 않는다.
12. CWS-016 보조: 시작 전과 끝의 `FSlateApplication::Get().GetUserFocusedWidget(0)`(실제 사용자 0)이 같다.

정리: `UnregisterVirtualWindow`, virtual user handle 해제, GC 보호 해제.

엔진 제약으로 3단계(스크롤 가능 상태)나 경로를 headless에서 만들 수 없으면 우회하지 않는다. 로그 근거와 함께 `QNA_IMPLEMENTATION.md`에 기록하고 리뷰 판단을 받는다.

### 7.2 B — `BathhouseSim.Computer.Input.WheelRoutesByComputerPhase` (production 분기·gate)

Fixture: 기존 컴퓨터 세션 테스트와 같은 world·PlayerController·Character·컴퓨터(sample screen widget)를 쓴다(5.4 공유 fixture). 다른 hovered 비교용 `UWidgetComponent`(두 번째 컴퓨터 또는 독립 component) 하나를 둔다.

단언:

1. Inactive:
   - `CanInjectPointerWheel(1, Screen)` false.
   - `MouseWheelInput(FInputActionValue(1.f))`가 crash 없이 끝난다.
2. 실제 `BeginComputerUse` 경로로 blend > 0 진입 → FocusingIn:
   - `CanInjectPointerWheel(±1, Screen)` false.
   - hit testing false(CWS-013).
3. Active(blend 완료 또는 friend `CompleteFocusIn`):
   - `CanInjectPointerWheel(-1, Screen)`·`(+1, Screen)` true. 클릭 없이 바로 가능하다(CWS-003).
   - 다음은 false: `(0, Screen)`, `(NaN, Screen)`, `(1, nullptr)`(CWS-012), `(1, 다른 component)`.
4. Active에서 `ScrollPointerWheel(1)` 실제 호출은 headless hover가 null이므로 false이고 crash가 없다. 이 사실과 이유를 `AddInfo`로 남긴다.
5. 분리(CWS-017·018 기준선): placement preview를 friend로 둔다(`PreviewFacility = APlaceableFacilityItemActor`, held-use owner 테스트와 같은 방식).
   - Active 상태에서 `MouseWheelInput(1)` → `AccumulatedYaw` 불변.
   - computer Inactive 상태에서 preview를 다시 두고 `MouseWheelInput(1)` → `AccumulatedYaw == NormalizePlacementYaw(이전 + RotationStepDegrees)`.
   - `AddRotationInput`은 yaw를 먼저 갱신한 뒤 `RefreshPreview`가 preview를 취소한다. 매 단계 preview를 다시 둔다.
   - 같은 방식으로 phase만 바꾸는 단계는 friend로 phase를 직접 설정해도 된다(held-use owner 테스트 선례). 2·3단계는 실제 진입 경로로 한다.
6. CWS-016:
   - Active에서 `MouseWheelInput`을 여러 번 호출한 뒤 `InteractStartInput()` 한 번 → focus-out(blend 0이면 Inactive, 아니면 FocusingOut).
   - FocusingOut에서 `CanInjectPointerWheel` false.
   - 휠 전후로 `PlayerController->bShowMouseCursor`가 같다(Active 중 true).
7. 비정상 종료: Active에서 컴퓨터 `HandleComputerUnavailable`(또는 Destroy) → Inactive. `CanInjectPointerWheel` false. 다음 `MouseWheelInput`은 placement 경로다(5와 같은 방식으로 yaw 변화 확인).

### 7.3 C — `BathhouseSim.Computer.Input.ScreenWheelContentContract` (Content 전제, 읽기 전용 load)

- WBP class를 load한다: `/Game/Bathhouse/UI/Shop/WBP_ShopScreen.WBP_ShopScreen_C`, `/Game/Bathhouse/UI/WBP_BathWaterDetail.WBP_BathWaterDetail_C`, `/Game/Bathhouse/UI/WBP_ComputerScreenRoot.WBP_ComputerScreenRoot_C`. `UWidgetBlueprintGeneratedClass::GetWidgetTreeArchetype()`를 쓴다.
  - Shop: `ProductScroll`, `CartScroll`, `OrderScroll`. Detail: `DetailScroll`. 네 `UScrollBox`가 있다.
  - 각각 `GetOrientation() == Orient_Vertical`, `GetConsumeMouseWheel() != Never`, `IsAnimateWheelScrolling() == false`.
  - 각각 `GetWheelScrollMultiplier()`가 finite이고 > 0. 값은 `AddInfo`로 기록한다. 1.0을 단언하지 않는다. 영역별 Editor 조정을 허용하기 때문이다.
  - 각각 Visibility가 hit test 가능.
  - 세 tree의 모든 `UScrollBox`(`ForEachWidget`)가 `ConsumeMouseWheel != Never`다. 이후 추가 영역 규칙이다.
- 입력: `/Game/Input/Actions/IA_PlacementRotate`.
  - `ValueType == Axis1D`, `Modifiers`·`Triggers` 비어 있음.
  - `/Game/Input/IMC_FirstPerson`의 mapping 중 key가 `MouseWheelAxis`·`MouseScrollUp`·`MouseScrollDown`인 것은 정확히 하나(`MouseWheelAxis → IA_PlacementRotate`)이고 그 mapping에 modifier·trigger가 없다.
  - `/Game/FirstPersonCharacter/BP_FirstPersonCharacter` CDO의 `PlacementRotateAction`이 그 IA다. 기존 BlueprintLoad 테스트처럼 reflection으로 읽는다.
- asset을 저장·변경하지 않는다. load 실패는 `AddError`다.

### 7.4 회귀 필터와 기대

`BathhouseSim.Computer`가 포함하는 테스트:

- 새 3개
- 기존 `FocusSessionSuppressionAndSampleScreen`, `FocusExitPlacement`, `BlueprintLoad`
- BUG의 `Input.ActiveFocusKeepsKeyboardOnGameViewport`

추가로 함께 돌릴 테스트:

- `BathhouseSim.Interaction.HeldTargetUse.InputOwners`: Character 입력 owner 회귀
- `BathhouseSim.Interaction.HeldTargetUse.BlueprintLoad`: Character BP input 참조
- `BathhouseSim.Service.Amenity.Scrub.InputOwnershipEntryReleaseExitSearchAndTransitions`: `IsFocusCapturingInput` 회귀
- `BathhouseSim.Placement`: 배치 회전·transaction 회귀

변경 전 코드에서는 B가 컴파일되지 않는다(신규 API). A·C는 엔진·Content 사실을 검증하므로 변경 전에도 통과할 수 있다. 이것이 정상이며 구현 보고에 그렇게 적는다.

## 8. 빌드·Automation 명령

```powershell
$Worktree = 'C:\UnrealProjects\BathhouseSim\.claude\worktrees\wheel'   # 마스터가 지정한 이 작업의 worktree 절대 경로로 바꾼다
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  BathhouseSimEditor Win64 Development `
  -Project="$Worktree\BathhouseSim.uproject" `
  -WaitMutex -NoHotReloadFromIDE
```

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  "$Worktree\BathhouseSim.uproject" /Engine/Maps/Templates/Template_Default `
  -unattended -nullrhi -NoSplash -NoSound -DDC-ForceMemoryCache `
  -ExecCmds="Automation RunTests BathhouseSim.Computer+BathhouseSim.Interaction.HeldTargetUse.InputOwners+BathhouseSim.Interaction.HeldTargetUse.BlueprintLoad+BathhouseSim.Service.Amenity.Scrub.InputOwnershipEntryReleaseExitSearchAndTransitions+BathhouseSim.Placement; Quit" -TestExit="Automation Test Queue Empty" `
  -ReportExportPath="$Worktree\Saved\Automation\Reports\<날짜>\computer-wheel-scroll" -log
```

- 공통 정책(`AGENT_WORKFLOW.md` UE 5.8 Build/Headless Automation Policy)의 진입점·옵션을 그대로 쓰고 프로젝트 경로만 worktree로 바꾼다.
- 에셋 로드 전 환경 Fatal은 검증 결과가 아니다. 명령을 고쳐 다시 실행한다.
- 사용자가 같은 worktree Binaries로 Editor를 열어 둔 상태에서는 빌드하지 않는다.

## 9. Blueprint/API·Core Redirect·Content·Unreal 정본

- reflected class·property·function·component·BindWidget 추가·삭제·rename 없음. 추가는 C++ 전용 public 함수 1개, private 함수 1개, friend 선언, 비-reflected handler rename뿐이다. Core Redirect·asset migration·BP 복사본 load gate가 필요 없다.
- **Content 변경 없음.** `PROMPT_UNREAL.md`(구현 단계 작성)는 "Content 변경 없음"을 선언하고 아래 Unreal 정본 문서 갱신만 Editor 단계 범위로 넘긴다. asset 저장은 없다.
- Unreal 정본 갱신 필요(Editor 작업 단계 소유, 문서만). 사실 근거는 `REPORT_UNREAL_DISCOVERY.md`다. Editor 역할이 필요하면 읽기 전용으로 재확인한다.
  1. `.md/Unreal/InteractionUISystem.md` "컴퓨터 연결": `BP_BathhouseComputer` `ScreenWidget.WidgetClass`를 `/Game/Bathhouse/UI/WBP_ComputerScreenRoot.WBP_ComputerScreenRoot_C`로 고친다(CDO·DefaultMap instance 동일, override 없음). 낡은 `WBP_BathWaterManagementScreen_C` 기술을 대체한다.
  2. 같은 문서에 컴퓨터 화면 스크롤 영역 authoring을 기록한다.
     - 대상 4개: `WBP_ShopScreen`의 `ProductScroll`·`CartScroll`·`OrderScroll`(Cart/Order 형제), `WBP_BathWaterDetail`의 `DetailScroll`(슬라이더 2개 포함). 모두 세로이고 중첩이 없다.
     - 휠 계약 설정: `ConsumeMouseWheel=WhenScrollingPossible`, `AnimateWheelScrolling=false`.
     - 영역별 이동량 authoring: `WheelScrollMultiplier` 1.0. 한 칸 = `Slate.GlobalScrollAmount` 32 × multiplier, ScrollBox local unit. `DetailScroll`은 `ManagementScale` 안이라 화면 px로는 더 작다.
  3. 같은 문서 입력 절: `IMC_FirstPerson`의 `MouseWheelAxis → IA_PlacementRotate`(Axis1D, modifier·trigger 없음)는 공용 휠 intent다. 컴퓨터를 쓰지 않을 때는 배치 회전, 컴퓨터 Active 때는 화면 스크롤이다. modifier·trigger나 두 번째 휠 mapping을 아키텍처 설계 없이 추가하지 않는다.
- `USER_UNREAL.md` 항목 없음.

## 10. 관련 BUG 작업과의 겹침·충돌

| 지점 | 내용 | 처리 |
|---|---|---|
| `Build.cs` | BUG가 `Slate`·`SlateCore` private 의존을 추가한다. 이 작업 테스트 A가 같은 모듈을 쓴다 | 이 작업은 수정하지 않는다. BUG 병합 전이면 착수하지 않는다(0절) |
| 입력 경계 | BUG가 정한 Keyboard Focus Invariant와 같은 경로를 확장한다. 휠은 Slate가 키보드 focus가 아니라 실제 커서 위치로 라우팅하므로, BUG의 focus 결과와 무관하게 viewport에 도달한다 | focus·input mode 코드 불변. BUG PIE CMP-001이 실패해 아키텍처 복귀로 focus 경로가 바뀌면, 이 작업은 4.1의 viewport 도달 전제만 재확인한다 |
| `PlayerComputerUseComponent.*`, `FirstPersonCharacter.*` | BUG는 production을 바꾸지 않는다. 테스트용 friend 선언을 추가했을 수 있다 | BUG 병합 뒤 착수하므로 friend 목록에 한 줄만 추가한다. 순서는 상관없다 |
| 테스트 파일·namespace | BUG는 `ComputerKeyboardFocusAutomationTests.cpp`(namespace `ComputerKeyboardFocusTest`)와 `BathWaterSliderInputAutomationTests.cpp`를 추가한다. 둘 다 `BathhouseSim.Computer`/`BathWater` 필터 안이다 | 새 파일·고유 namespace `ComputerWheelScrollTest`. virtual user index를 BUG(13)와 다르게 한다. BUG 파일은 수정하지 않는다 |
| 컴퓨터 테스트 fixture | BUG 테스트 5.3도 기존 컴퓨터 world fixture가 필요하다 | 공유 header가 이미 있으면 재사용, 없으면 5.4대로 추출한다. 복제는 금지한다 |
| Architecture 정본 | 이 브랜치는 BUG 아키텍처 커밋 위에서 `ComputerSystem.md`(Keyboard Focus Invariant, Input Routing)·`CoreSystem.md`(Slate use site)·`0_ARCHITECTURE.md`를 고쳤다 | BUG 구현·리뷰·PIE 단계가 같은 절을 고치면 main 병합 때 문서 텍스트 충돌이 날 수 있다. 내용은 양립하며 두 절을 모두 살려 수동으로 해결한다 |
| 슬라이더 | BUG는 drag 중 손잡이를 확정값으로 되돌린다. 휠은 슬라이더 값을 쓰지 않는다(Q3 A) | 동작 겹침 없음. 사용자 PIE에서 CWS-010과 SLD 시나리오를 독립으로 본다 |

## 11. 코드 리뷰 기준

- 휠 값이 부호·배율 변경 없이 `ScrollWheel`까지 가는가.
- Character 분기 순서가 computer capture → 세신 소비 → Placement이고, computer capture 중 Placement에 절대 닿지 않는가.
- `CanInjectPointerWheel`이 5.1의 다섯 조건을 모두 검사하는가(phase, hit testing, finite·non-zero, 화면 유효, hovered == 현재 화면).
- 신규 상태·Tick·UPROPERTY·로그가 없는가. reflected 이름이 그대로인가.
- production에 Slate 직접 사용, focus API, `NativeOnMouseWheel`, ScrollBox 직접 조작, 영역 이름 하드코딩이 없는가.
- 테스트 A가 엔진 tick·routing 경로로 상태를 만들고 offset을 흉내 내지 않는가. B가 실제 진입 경로로 FocusingIn·Active를 만드는가. C가 load만 하는가.
- helper namespace·virtual user index 충돌이 없는가. fixture 복제가 없는가.
- Content·Config 변경이 없는가.

## 12. 사용자 PIE 관찰 항목

선행: 이 작업 build를 사용자 Editor에 반영한다(병합 뒤 메인 트리 build 등 마스터가 정한 방법). 공통 Given은 DefaultMap 컴퓨터에 빈손 E 진입, 시점 전환 완료, 커서 표시다. 상점 상품은 현재 catalog 20종으로 넘친다.

1. CWS-003·001·002: 진입 직후 아무것도 클릭하지 않는다.
   - 커서를 상품 카드 사이 빈 곳에 두고 휠을 아래로 몇 칸 → 상품 목록만 아래쪽 카드가 올라온다.
   - 위로 굴리면 돌아간다.
   - 장바구니·주문 패널, 잔액, 탭, 시점·캐릭터는 그대로다.
2. CWS-004: 카드의 담기 버튼·이름·가격 위에서 휠 → 스크롤만 되고 장바구니 수량·잔액 불변.
3. CWS-005: 맨 아래·맨 위에서 같은 방향으로 계속 굴리면 멈추고 튕기지 않는다. 반대 한 칸은 즉시 움직인다.
4. CWS-006: 장바구니에 상품 1종일 때 장바구니 목록 위 휠 → 아무것도 움직이지 않는다.
5. CWS-007: 장바구니를 넘칠 만큼 담고 그 위에서 휠 → 장바구니만 움직인다.
6. CWS-008: 대기 주문이 넘칠 만큼 주문하고 주문 목록 위에서 휠 → 주문 목록만 움직이고 남은 시간이 계속 갱신된다.
7. CWS-009: 관리 탭에서 욕탕 선택, 상세 문구 위 휠 → 상세 영역만 스크롤, 지도·용량 요약 불변.
8. CWS-010: 순환도·목표 수온 슬라이더 위 휠 → 상세 영역 스크롤(끝이면 정지), 슬라이더 값·욕탕 설정·용량 표시 불변.
9. CWS-011: 탭 버튼, 주문 버튼, 잔액 문구, 욕탕 지도 위 휠 → 아무 변화 없음, 선택·탭 불변.
10. CWS-012: 커서를 모니터 바깥 월드에 두고 휠 → 화면·시점·캐릭터 변화 없음.
11. CWS-013: 진입 전환 중, E로 나가는 전환 중 휠 → 스크롤 위치 불변, 진입·이탈 정상.
12. CWS-014: 상품 목록을 중간까지 내린다. E로 나갔다가 재진입하면 같은 위치다. 관리 탭에 갔다가 상점 탭으로 돌아와도 같은 위치다.
13. CWS-015: 휠로 스크롤한 직후 커서 아래 카드의 담기 버튼 클릭 → 그 카드 상품이 담긴다.
14. CWS-016: 휠을 몇 번 쓴 뒤 E → 휠을 쓰지 않았을 때와 같은 결과다(BUG 수정이 병합됐다면 클릭 없이 이탈). ESC는 PIE 종료 단축키와 겹칠 수 있어 E로 판정한다.
15. CWS-017: 컴퓨터를 쓰지 않고 설비 아이템을 들어 배치 프리뷰가 보일 때 휠 → 기존처럼 회전한다.
16. CWS-018: 빈손으로 모니터를 바라보며 휠 → 아무 일 없다.
17. CWS-020: 컴퓨터 사용 중 휠을 쓴 뒤 이동키·Space·Shift·F·G·Q·LCtrl → 아무 일 없다.

- 이동량 감각(특히 `DetailScroll`이 상대적으로 느린지)은 수용 실패 사유가 아니다. 영역과 원하는 감각을 알려 주면 Editor 단계가 해당 ScrollBox `WheelScrollMultiplier`만 조정한다.
- 실패 시 기록: 시나리오 ID, 커서 위치(영역·항목), 휠 방향·칸 수, 움직인 영역.
- 휠이 전혀 반응하지 않으면 콘솔 `showdebug enhancedinput`으로 휠 중 `IA_PlacementRotate` 값이 보이는지 함께 적는다.

## 13. 복귀 재설계 여부

복귀 재설계가 아니다. 유지되는 완료 범위: 컴퓨터 포커스 CMP-001~020 production(`61f72c1`), 상점·관리 화면 구조와 레이아웃, BUG 작업 설계·구현.
