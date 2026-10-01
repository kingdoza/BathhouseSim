# Editor 사전 조사 — COMPUTER-WHEEL-SCROLL 컴퓨터 화면 스크롤 영역 마우스 휠 스크롤

- 작업 ID: `COMPUTER-WHEEL-SCROLL`
- 단계: Editor 사전 조사
- 상태: 완료

(Editor 워커의 Write가 하네스에 거부되어 완료 보고 전문을 마스터가 사용자 승인 후 저장함, 2026-10-01)

## 명세 가정과 다른 사실·사용자 선택에 영향을 줄 사실

1. **컴퓨터 화면의 실제 WidgetClass는 탭 root `WBP_ComputerScreenRoot_C`다.** BP CDO와 DefaultMap instance가 같다(override 없음). `Unreal/InteractionUISystem.md` 23행의 "`WBP_BathWaterManagementScreen_C`"는 낡은 기술이다. Architecture `ShopSystem.md`·`ComputerSystem.md`의 "탭 root로 교체"가 맞다. `Unreal/ShopSystem.md`에는 탭 root·WidgetClass 기술이 없다.
2. **스크롤 영역은 정확히 4개다. 모두 세로이고 서로 중첩되지 않는다**: 상점 `ProductScroll`·`CartScroll`·`OrderScroll`, 관리 상세 `DetailScroll`. 가로 영역과 중첩 영역이 없으므로 P4·P12 전제를 유지하고, CWS-019는 PIE 대상이 아니다. 2.1 표의 미확정 단서(CartScroll·OrderScroll·DetailScroll)는 모두 실제 ScrollBox로 확인됐다.
3. 네 영역의 휠 설정은 모두 엔진 기본값이다: `ConsumeMouseWheel=WhenScrollingPossible`, `WheelScrollMultiplier=1.0`, `AnimateWheelScrolling=false`.
   - 한 칸 이동량은 엔진 `SScrollBox::OnMouseWheel` 기준 `Slate.GlobalScrollAmount`(기본 32 Slate unit) × 1.0이다.
   - 내용이 영역보다 작으면 휠을 처리하지 않고 넘긴다.
   - 휠 스크롤은 overscroll 없이 끝에서 멈춘다(`EAllowOverscroll::No`).
   - 4.6 표의 "기본 = 현재 Content 설정"은 칸당 32 unit이다.
4. 순환도·목표 수온 슬라이더는 `DetailScroll` 안에 있다. 엔진 `SSlider`에는 `OnMouseWheel` 처리가 없고, UMG Slider에 휠 관련 설정도 없다. Q3 A와 충돌하는 Content 설정은 없다.
5. 프로젝트의 마우스 휠 mapping은 `IMC_FirstPerson`의 `MouseWheelAxis → IA_PlacementRotate`(Axis1D, modifier·trigger 없음) 하나뿐이다. `IMC_Default`·`IMC_MouseLook`은 mapping이 0개이고 controller가 쓰지 않는다. P8은 유지된다.
6. 관리 화면(`ManagementScreen`, 1024×576 고정 SizeBox)은 root의 `ManagementScale`(ScaleBox) 안에 있어 축소되어 그려진다. 그래서 `DetailScroll`의 32 unit은 컴퓨터 화면 px로는 더 작게 보인다. 배율(Stretch, 탭 바 높이)은 측정하지 않았다. 상점 화면은 ScaleBox 없이 switcher에 직접 있다.

## 범위와 방법

- 범위: `PROMPT_ARCHITECTURE.md` 6절 5개 항목. 읽기 전용이며 mutation·Compile·Save·PIE는 하지 않았다.
- 실행 방식: 작업용 숨김 Editor(`UnrealEditor.exe … -NoSplash -log -ExecCmds="py …"`)에서 DefaultMap이 로드된 뒤 읽기 전용 Python을 실행했다. 실행 전후 dirty package는 0개였다.
- 근거 파일: `Saved/Claude/Discovery3/disc_01_probe.py`, `probe.json`, `probe_run3.log`.
- WidgetTree root는 Python에 노출되지 않는다. package 이름표의 이름으로 WidgetTree 안 widget을 찾고, 부모가 없는 widget을 root로 삼았다. WBP마다 찾은 widget 수와 순회한 수가 같다(누락 없음).
- 엔진 근거:
  - `Engine/Source/Runtime/Slate/Private/Widgets/Layout/SScrollBox.cpp` 1158행
  - `Runtime/SlateCore/Private/SlateCoreClasses.cpp` 16행
  - `Runtime/UMG/Private/Components/WidgetInteractionComponent.cpp` 713행

## 1. `BP_BathhouseComputer` ScreenWidget (CDO와 DefaultMap instance가 같음)

| 항목 | 값 |
|---|---|
| WidgetClass | `/Game/Bathhouse/UI/WBP_ComputerScreenRoot.WBP_ComputerScreenRoot_C` |
| Draw Size | 1024×576 |
| Receive Hardware Input | false |
| Window Focusable | true |
| Window Visibility | SelfHitTestInvisible |
| Geometry / Pivot | Plane / (0.5,0.5) |
| Collision | QueryOnly, Visibility Block |
| Relative | loc (51.667,0,0), scale (0.833,0.083,0.143) |

- instance: label `Computer`, 위치 (-470,0,160), package `/Game/__ExternalActors__/Maps/DefaultMap/7/EH/E4FLO971KSWUJ40H7W7PHK`.
- component(`ComputerMesh`, `ScreenWidget`, `FocusCamera`, `FocusExitPoint`, `FocusExitArrow`)는 모두 native다. SCS에 추가된 Widget component는 없다.
- `WidgetSpace`는 Python에 노출되지 않아 읽지 못했다.

## 2. 화면 hierarchy와 스크롤 영역

### 2.1 Root `WBP_ComputerScreenRoot`

`RootOverlay > RootSize(SizeBox 1024×576) > RootFrame(Border) > RootColumn(VerticalBox)` 아래에 두 갈래가 있다.

- `TabBar > TabRow`: `ManagementTabButton`, `ShopTabButton`. 각각 132×32 SizeBox 안에 있다.
- `ScreenSwitcher`(Fill)
  - index 0: `ManagementScale`(ScaleBox) > `ManagementScreen`(`WBP_BathWaterManagementScreen_C`)
  - index 1: `ShopScreen`(`WBP_ShopScreen_C`)
- 조상 widget은 모두 SelfHitTestInvisible 또는 Visible이다. 스크롤 영역의 hit test를 막는 설정은 없다.

### 2.2 스크롤 영역

| 영역 | WBP와 경로 | 방향 | 내용 | slot |
|---|---|---|---|---|
| `ProductScroll` | `WBP_ShopScreen` `/RootOverlay/ShopFrame/ShopBody/ProductColumn/ProductScroll` | 세로 | `ProductGrid`(WrapBox) | ProductColumn 안 Fill 1.0, 헤더 아래 |
| `CartScroll` | `WBP_ShopScreen` `…/ShopBody/CartPanel/CartColumn/CartScroll` | 세로 | `CartList`(VerticalBox) | CartColumn 안 Fill 0.55 |
| `OrderScroll` | `WBP_ShopScreen` `…/ShopBody/CartPanel/CartColumn/OrderScroll` | 세로 | `OrderList`(VerticalBox) | CartColumn 안 Fill 0.45 |
| `DetailScroll` | `WBP_BathWaterDetail` `/RootOverlay/DetailFrame/DetailScroll` | 세로 | `DetailColumn`(문구 14개, 슬라이더 2개와 라벨) | DetailFrame(Border) 내용 |

- 중첩: 없다. `CartScroll`·`OrderScroll`은 형제다. `ProductColumn`(ShopBody Fill 2.0)과 `CartPanel`(Fill 1.0)도 형제라서 SHOP-032 레이아웃과 일치한다.
- 관리 화면: `BathDetail`은 `ManagementBody`의 Fill 0.38, 지도는 Fill 0.62다.
- 다음 WBP에는 ScrollBox·Slider가 없다: `WBP_ShopProductCard`, `WBP_ShopCartLine`, `WBP_ShopOrderLine`, `WBP_BathWaterMap`, `WBP_BathWaterBathTile`, `WBP_BathWaterCapacitySummary`, `WBP_BathWaterManagementScreen`, root. `/Game/Bathhouse/UI`의 다른 WBP에도 ScrollBox가 없다.

### 2.3 네 영역의 공통 설정 (모두 같음)

- 방향·휠
  - Orientation Vertical
  - ConsumeMouseWheel WhenScrollingPossible
  - WheelScrollMultiplier 1.0
  - AnimateWheelScrolling false
  - AllowOverscroll true. 터치·드래그용이며, 휠 경로는 엔진이 overscroll을 금지한다.
- 스크롤바: ScrollBarVisibility Visible, AlwaysShowScrollbar false, ScrollbarThickness 9×9
- 기타
  - AllowRightClickDragScrolling true
  - ScrollWhenFocusChanges NoScroll
  - IsFocusable false
  - EnableTouchScrolling true
  - Visibility Visible, Enabled true, Clipping ClipToBounds
- 엔진 동작: 스크롤바가 필요할 때만 `ScrollBy(−WheelDelta×32×배율)`를 하고, 실제로 움직였을 때만 Handled다. 끝이면 Unhandled로 부모에게 넘긴다. 현재는 넘겨받을 바깥 ScrollBox가 없다.
- RMB 드래그 스크롤이 켜져 있지만, 가상 포인터는 RMB를 전달하지 않으므로 관찰되는 영향은 없을 것으로 판단한다(PIE 미확인).

## 3. 슬라이더 (`WBP_BathWaterDetail`)

| 슬라이더 | 위치 | Min/Max/Step | MouseUsesStep | RequiresControllerLock | IsFocusable |
|---|---|---|---|---|---|
| `CirculationSlider` | `DetailScroll/DetailColumn` | 0/1/0.01 | false | true | true |
| `TargetTemperatureSlider` | `DetailScroll/DetailColumn` | 0/1/0.01 | false | true | true |

- 휠 관련 설정이 없고 `SSlider`가 휠을 처리하지 않는다. 슬라이더 위 휠은 엔진 기본 경로로 `DetailScroll`에 올라간다.
- `IsFocusable=true`는 키보드 focus 라우팅(관련 BUG 현상 2)과 관련될 수 있어 기록만 한다.

## 4. 입력 mapping

| IMC | mapping | 휠 |
|---|---|---|
| `/Game/Input/IMC_FirstPerson` | 8개: E Interact, F SecondaryInteract, G DropCarry, LMB PrimaryUse, Q RecoverFacility, LeftControl PlacementSnap, MouseWheelAxis PlacementRotate, Escape Cancel | `MouseWheelAxis → /Game/Input/Actions/IA_PlacementRotate`, modifier·trigger 없음 |
| `/Game/Input/IMC_Default` | 0 | 없음 |
| `/Game/Input/IMC_MouseLook` | 0 | 없음 |

- `IA_PlacementRotate`: Axis1D이고 modifier·trigger가 없다.
- Controller: `BP_FirstPersonController.DefaultMappingContext=IMC_FirstPerson`이다. native는 이 IMC 하나만 추가한다(`FirstPersonController.cpp` 24행).
- `BP_FirstPersonCharacter.PlacementRotateAction=IA_PlacementRotate`.
- Content `.uasset` 바이트 검색에서 `MouseWheel`이 나오는 asset은 `IMC_FirstPerson`뿐이다. `DefaultInput.ini`에는 엔진 기본 `AxisConfig MouseWheelAxis`만 있다.

## 5. `BP_FirstPersonCharacter.ComputerWidgetInteraction` (native, CDO)

- 설정
  - InteractionSource Mouse
  - InteractionDistance 500
  - TraceChannel Visibility
  - PointerIndex 0, VirtualUserIndex 0
  - EnableHitTesting false
  - ShowDebug false
  - relative 0
- 엔진의 `UWidgetInteractionComponent::ScrollWheel(float)`은 마지막 hover 경로(`LastWidgetPath`)로 `MouseWheelAxis` 이벤트를 보낸다. 설계 참고용 사실로만 적고, 경로 선택은 아키텍처 단계가 정한다.

## 미확정

- 측정하지 않은 값: `ManagementScale` 배율, 탭 바 높이(관리 화면 px 환산용).
- PIE에서만 확인되는 것: 휠 수용, hover 경로, 스크롤바 표시.

## 기준선과 종료

- 시작 커밋 `2c24374`(`work/SERVICE-U4`). 전후 git status는 `?? .claude/worktrees/`뿐이고 Content 변경은 없다.
- 작업용 Editor는 MCP 확인용 PID 8736(정상 종료)과 probe용 PID 26816·30948·31736(스크립트 종료 후 QUIT)을 썼다. 남은 프로세스는 없고, 저장한 asset·Level도 없다.
