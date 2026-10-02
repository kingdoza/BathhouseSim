# Interaction/UI Editor Authoring

## Bath Water 관리 화면

다섯 Widget Blueprint는 모두 `/Game/Bathhouse/UI/`에 저장돼 있다. 아래 WBP 이름 앞에 이 경로를 붙인 것이 exact asset path이고, native parent 이름 앞에는 `/Script/BathhouseSim.`을 붙인다. hierarchy/layout만 WBP가 소유하고 물·용량 계산, slider mutation, 타일 생성은 native Widget이 처리한다. 각 WBP는 native class를 직접 상속하며 추가 Event Graph gameplay 로직을 사용하지 않는다.

| WBP | Native parent | 필수 자식 |
|---|---|---|
| `WBP_BathWaterManagementScreen` | `BathWaterManagementScreenWidget` | `CapacitySummary` (`WBP_BathWaterCapacitySummary`), `BathMap` (`WBP_BathWaterMap`), `BathDetail` (`WBP_BathWaterDetail`) |
| `WBP_BathWaterCapacitySummary` | `BathWaterCapacitySummaryWidget` | `CirculationCapacityText/Bar/StatusText`, `HeatingCapacityText/Bar/StatusText`, `CoolingCapacityText/Bar/StatusText` (`TextBlock/ProgressBar/TextBlock`) |
| `WBP_BathWaterMap` | `BathWaterMapWidget` | `MapSize` (`SizeBox`), `MapFrame` (`Border`), `GridCanvas`, `BathTileCanvas` (`CanvasPanel`), `EmptyStateText` (`TextBlock`) |
| `WBP_BathWaterBathTile` | `BathWaterBathTileWidget` | `SelectButton` (`Button`), `BathNameText`, `ActualTemperatureText`, `ContaminationText`, `ThermalStatusText`, `CapacityStatusText` (`TextBlock`) |
| `WBP_BathWaterDetail` | `BathWaterDetailWidget` | `BathNameText`, `WaterAmountText`, `ActualTemperatureText`, `TargetTemperatureText`, `ContaminationText`, `CirculationText`, `CirculationDemandText`, `HeatingDemandText`, `CoolingDemandText`, `ThermalStatusText`, `ThermalThresholdText`, `CapacityStatusText`, `FeedbackText` (`TextBlock`), `CirculationSlider`, `TargetTemperatureSlider` (`Slider`) |

Summary의 접두사별 이름은 표기된 접두사에 `CapacityText`, `CapacityBar`, `CapacityStatusText`를 각각 이어 붙인 정확한 `BindWidget` 이름이다. 다섯 WBP의 필수 child 이름·타입은 디스크 재로드 뒤 검사됐다.

`WBP_BathWaterCapacitySummary`는 `SummaryFrame > SummaryColumn > SummaryRows` 아래 `CirculationRow/HeatingRow/CoolingRow` 세 개를 같은 가로 Fill 비율로 유지한다. 제목과 행 레이블은 `설비 용량`, `순환/가열/냉각`이다. 각 행의 값 Text는 native `예약/가동/설치` 문자열을, 상태 Text는 설치·가동 부족이 동시에 나오는 긴 문구를 표시하고 둘 다 줄바꿈을 허용한다. 글자 크기·굵기는 각 TextBlock의 `Font`, 줄바꿈 폭은 해당 TextBlock의 wrap 설정, 행 여백·간격은 각 slot `Padding`이 원본이며 이 문서는 수치를 적지 않는다. 실제 플레이 화면 잘림 여부는 사용자 확인이 남아 있다.

`WBP_BathWaterMap`의 `BathTileWidgetClass`는 `WBP_BathWaterBathTile_C`다. **화면 안에 배치된 `BathMap` 위젯 템플릿에도 같은 class가 저장돼 있다.** Class Default만 설정하면 중첩 템플릿에서 `None`으로 남아 PIE 타일이 0개가 될 수 있다. `MapStack`의 순서는 `GridCanvas` → `BathTileCanvas` → `EmptyStateText`이며 두 Canvas는 `ClipToBounds`, `GridCanvas`는 `HitTestInvisible`이다. 격자선과 Zone 경계의 위치·수명·두께 보정은 native Map Widget이 소유하고 타일의 Button hit test를 가리지 않는다. 선 두께(render target px)·색의 원본은 `WBP_BathWaterMap` Class Defaults `GridLineThicknessPx`, `BoundaryLineThicknessPx`, `GridLineColor`, `BoundaryLineColor`(Category `Bath Water Map|Grid`)다.

`WBP_BathWaterBathTile`의 hierarchy는 `RootOverlay > SelectButton`(OverlaySlot H/V Fill) `> TileTextScale`(ScaleBox, Stretch `ScaleToFit`, Stretch Direction `DownOnly`, `SelfHitTestInvisible`, ButtonSlot H/V Fill) `> TileColumn`(ScaleBoxSlot H/V Center) `>` 글자 BindWidget 5개다.
- 보이는 영역과 클릭 영역은 native `UBathWaterMapWidget`이 canvas slot에 쓰는 footprint 사각형 전체다.
- 글자 묶음은 타일보다 클 때만 축소된다(작업 `BUG-2026-10-02_bath_water_map_grid_scale_mismatch` QNA_ARCHITECTURE Q1 = A).
- `TileTextScale`은 BindWidget이 아니다.
- 상태색·불투명도의 원본은 Class Defaults `DeficitTileColor`, `SelectedTileColor`, `NormalTileColor`, `SelectedRenderOpacity`, `UnselectedRenderOpacity`(Category `Bath Water Tile`)이고 적용 분기는 C++가 소유한다. 글자 크기는 각 TextBlock `Font`가 원본이다.
- 위 구조는 2026-10-02 Editor Python으로 저장했고, 새 프로세스에서 Compile·GUID 정리 저장과 재로드를 확인했다.

Detail slider 두 개는 native 정규화 범위(0~1)를 쓴다. Root는 `RootOverlay > ManagementSize`(SizeBox, 크기 원본은 Width/Height Override) `> ManagementFrame > ManagementColumn` 안에서 제목, 전체 폭의 utility summary, 그 아래 지도(좌)와 detail(우)을 배치한다. Detail은 `ScrollBox` 안에 있다. 어두운 패널/지도 바탕에 밝은 텍스트를 쓰고 순환·가열·냉각을 서로 다른 색으로 구분한다. 색 원본은 각 widget의 색 프로퍼티다. 지도 좌표와 타일 수명은 WBP graph가 아닌 native Map Widget 계약을 따른다.

## 컴퓨터 연결

- `/Game/Bathhouse/Blueprints/Computer/BP_BathhouseComputer`의 inherited `ScreenWidget.WidgetClass`는 탭 root `/Game/Bathhouse/UI/WBP_ComputerScreenRoot.WBP_ComputerScreenRoot_C`다. CDO와 DefaultMap instance가 같고 instance override는 없다(2026-10-01 읽기 전용 조사, `COMPUTER-WHEEL-SCROLL`).
- `WBP_ComputerScreenRoot`는 `RootOverlay > RootSize (SizeBox 1024×576) > RootFrame > RootColumn` 아래에 `TabBar > TabRow`(`ManagementTabSize > ManagementTabButton`, `ShopTabSize > ShopTabButton`, `ExpansionTabSize > ExpansionTabButton`, 라벨 `관리 · 상점 · 확장`, 같은 크기·간격·style)와 `ScreenSwitcher`를 둔다. Switcher index 0은 `ManagementScale`(ScaleBox) > `ManagementScreen`(`WBP_BathWaterManagementScreen_C`), index 1은 `ShopScreen`(`WBP_ShopScreen_C`, ScaleBox 없음), index 2는 `ExpansionScreen`(`WBP_ExpansionScreen_C`, ScaleBox 없음)이다. `ExpansionTabButton`·`ExpansionScreen`은 native `BindWidgetOptional`이고 존재·순서는 자동화 `BathhouseSim.Expansion.Content.ScreenContract`가 검사한다. 조상 widget은 모두 Visible 또는 SelfHitTestInvisible이라 스크롤 영역 hit test를 막지 않는다.
- ScreenWidget은 World Space, Draw Size (1024,576), Receive Hardware Input false를 유지한다. Focus camera 기본값은 변경하지 않았다.
- 새 Editor 프로세스 재로드에서 BP_BathhouseComputer CDO의 FocusExitPoint 위치는 (1500,0,-228.5714285714), 회전은 (0,180,0), SearchRadius는 100cm였다. FocusExitArrow는 이 컴포넌트 자식이며 local 원점/회전 0, editor-only, 길이 80cm다. DefaultMap 컴퓨터 인스턴스도 동일한 상대 transform을 재로드했다. Actor transform은 홀 바닥 높이를 따른다(위치는 Level instance가 정본, `EXP-U1`에서 Z만 홀 바닥 윗면만큼 올림). yaw 0, scale (0.12,1.2,0.7)이며 계산된 world 발 위치는 홀 바닥 윗면 위, 방향은 컴퓨터를 향하는 -X다. 이전 MCP 인계의 X=1000은 stale한 값이며 새 프로세스 재로드 결과를 현재 저장 상태로 본다.
- Level computer instance의 `ManagedBathPlacementZone`은 목욕공간 Actor `Space_Bath`(`BP_BathhouseSpace_C_UAID_F02F7433CA362F0703_1645196495`, [BuildingSystem.md](BuildingSystem.md))를 참조한다. CDO 속성은 None이므로 class default와 Level instance 참조를 구분한다.
- BP_FirstPersonCharacter.CancelAction은 IA_Cancel이고 기존 InteractAction은 IA_Interact다. IA_Cancel은 Boolean Input Action이다. IMC_FirstPerson은 기존 7개 mapping과 Escape → IA_Cancel을 포함해 8개다. BP_FirstPersonController.DefaultMappingContext는 IMC_FirstPerson이다.
- 변경된 네 asset(BP_BathhouseComputer, BP_FirstPersonCharacter, IMC_FirstPerson, IA_Cancel)은 각각 저장한 뒤 새 프로세스에서 다시 읽었고 모두 clean이었다. DefaultMap도 clean이며 성공한 map/external actor 저장은 없었다. 앞선 SceneTools.save_actor 시도는 external actor registry 경로 오류로 실패했다.

- `/Game/Maps/DefaultMap.DefaultMap:PersistentLevel.BP_BathhouseComputer_C_UAID_F02F7433CA3690F802_2051456727`의 Level reference는 World Partition external actor `/Game/__ExternalActors__/Maps/DefaultMap/7/EH/E4FLO971KSWUJ40H7W7PHK`에 저장됐다(Python `save_packages`, 새 프로세스 재로드 확인). `DefaultMap.umap` 자체는 변경하지 않았다.

UE 5.8 DLL 빌드 후 새 Editor에서 다섯 WBP와 컴퓨터 BP의 Data Validation 6/6 `VALID`를 확인했다. PIE의 1024×576 RenderTarget에는 Zone 격자·경계, Bath 타일 2개, utility summary와 detail이 표시됐다. 타일 Button의 `OnClicked` 이벤트를 호출하면 선택·detail·slider 활성화가 갱신된다. 실제 플레이어 LMB 조준/클릭과 물 제어·회수 전체 시나리오는 아직 직접 플레이 검증이 필요하며 [USER_UNREAL.md](../USER_UNREAL.md)에 남겼다. 이번 FocusExitPoint/취소 입력 변경은 fresh-process reload와 PIE 시작·종료까지만 확인했고, Data Validation과 E/ESC/마우스 입력 수용은 미완료다.

### 확장 화면

두 WBP는 `/Game/Bathhouse/UI/`에 있고 native class를 직접 상속하며 graph가 없다(`EXP-U2` 생성, `EXP-U3` 수정; 저장 후 새 프로세스 Compile/Save로 widget GUID 정리, 재로드 `BS_UP_TO_DATE`·Data Validation VALID). 문구는 고정 문구(`정말 구입할까요?`, `확인`, `취소`) 외에 C++가 채운다. 스크롤 영역은 없다.

| WBP | Native parent | 구조 |
|---|---|---|
| `WBP_ExpansionSpaceOption` | `ExpansionSpaceOptionWidget` | `RootOverlay > OptionSize`(SizeBox 높이 고정) `> SelectButton`(Button, 카드 전체) `> CardOverlay` 아래 `SelectionHighlight`(Border, 반투명 청록, HitTestInvisible)와 `CardColumn`(`NameText` → `StageText` → `SizeText` → `PriceText` → `EffectText`(두 줄) → `StatusText`) |
| `WBP_ExpansionScreen` | `ExpansionScreenWidget` | `RootOverlay > ExpansionFrame > ExpansionColumn`: `HeaderRow`(`LockerText` 왼쪽 정렬·`BalanceText` 오른쪽 정렬, 같은 Fill 비율) → `OptionsPanel`(HorizontalBox, `HallOption`·`BathOption`·`WorkOption` = `WBP_ExpansionSpaceOption_C` 가로 3열) → `InfoColumn`(`ShortfallText`·`ResultText`·`MessageText`) → `ActionOverlay`에 같은 자리로 겹친 `PurchasePanel`(`PurchaseButtonSize > PurchaseButton > PurchaseButtonText`)과 `ConfirmPanel`(`ConfirmPromptText` → `CancelButton` → `ConfirmButton`) |

- 선택지별 단계·다음 넓힘 가격은 카드의 `StageText`·`PriceText`(native `BindWidgetOptional`, 문구·가시성은 C++)가 보여 준다. 화면 머리의 옛 `StageText`·`PriceText`는 없다(native binding 삭제, 존재·부재는 자동화 `BathhouseSim.Expansion.Content.ScreenContract`가 검사). 카드 높이 원본은 `OptionSize` height override이며, 두 줄이 늘어 높였다. 세 카드와 정보·구입 영역의 높이 합은 root `RootSize` 안에 드는 것을 글꼴 크기·패딩 합으로 확인했고, 실제 1024×576 화면 잘림은 사용자 PIE 확인 대상이다(숨김 Editor에서 화면 미표시, FBK-003).

- `ConfirmPanel`에서 구입 버튼과 같은 왼쪽 자리는 안내 문구(`ConfirmPromptSize`, 구입 버튼 폭과 같음)이고 `취소`·`확인` 버튼은 그 오른쪽이다. 구입 버튼 더블클릭이 확인으로 이어지지 않게 하는 배치다.
- 컨테이너는 SelfHitTestInvisible(바탕 `ExpansionFrame`만 Visible), 버튼은 Visible이다. 패널 표시·숨김과 버튼 활성은 C++가 바꾼다. 색은 상점·관리 화면 팔레트(어두운 패널·밝은 글씨)다.

### 컴퓨터 화면 스크롤 영역

컴퓨터 화면의 휠 스크롤 대상은 아래 `ScrollBox` 4개뿐이다. 모두 세로(Orientation Vertical)이고 서로 중첩되지 않는다. `/Game/Bathhouse/UI`의 다른 WBP(상품 카드·장바구니 행·주문 행·지도·타일·용량 요약·관리 화면·확장 화면·root 포함)에는 ScrollBox가 없다.

| ScrollBox | WBP와 경로 | 내용 |
|---|---|---|
| `ProductScroll` | `WBP_ShopScreen` `/RootOverlay/ShopFrame/ShopBody/ProductColumn/ProductScroll` | `ProductGrid`(WrapBox) |
| `CartScroll` | `WBP_ShopScreen` `…/ShopBody/CartPanel/CartColumn/CartScroll` | `CartList`(VerticalBox), `OrderScroll`과 형제 |
| `OrderScroll` | `WBP_ShopScreen` `…/ShopBody/CartPanel/CartColumn/OrderScroll` | `OrderList`(VerticalBox) |
| `DetailScroll` | `WBP_BathWaterDetail` `/RootOverlay/DetailFrame/DetailScroll` | `DetailColumn`(상세 문구, `CirculationSlider`·`TargetTemperatureSlider`와 라벨) |

- 휠 계약 설정(네 영역 공통, [ComputerSystem.md](../Architecture/ComputerSystem.md) `Screen Wheel Scroll`): `ConsumeMouseWheel=WhenScrollingPossible`, `AnimateWheelScrolling=false`. 휠 경로는 엔진이 overscroll을 막아 끝에서 멈추고, 내용이 영역보다 작거나 끝이면 휠을 처리하지 않고 바깥으로 넘긴다.
- 한 칸 이동량의 원본은 각 ScrollBox의 `WheelScrollMultiplier` 프로퍼티(영역별 authoring 위치)와 엔진 cvar `Slate.GlobalScrollAmount`다. 한 칸 = `Slate.GlobalScrollAmount` × 그 ScrollBox의 `WheelScrollMultiplier`이며 단위는 ScrollBox local unit이다. 이 문서는 수치를 기록하지 않는다. `WheelScrollMultiplier` 현재 값은 Editor에서 해당 ScrollBox를 읽거나 자동화 `BathhouseSim.Computer.Input.ScreenWheelContentContract`의 Info 로그로 확인하고, cvar 값은 Editor 콘솔에서 읽는다.
- `DetailScroll`은 root의 `ManagementScale`(ScaleBox) 안이라 같은 local 이동량이 컴퓨터 화면 px로는 더 작게 보인다. 상점의 세 영역은 ScaleBox 밖이다.
- 이동량 감각 조정은 해당 ScrollBox의 `WheelScrollMultiplier`만 바꾼다. 휠 분기·주입·hover gate는 C++(`AFirstPersonCharacter`, `UPlayerComputerUseComponent`)가 소유하므로 WBP에 휠 이벤트 graph, `OnMouseWheel` override, ScrollBox 직접 조작을 만들지 않는다.
- 두 Slider에는 휠 설정이 없고 엔진 Slider는 휠을 처리하지 않으므로, 슬라이더 위 휠은 `DetailScroll`로 올라간다.

### 공용 휠 입력

- `/Game/Input/IMC_FirstPerson`의 `MouseWheelAxis → /Game/Input/Actions/IA_PlacementRotate`(Axis1D, modifier·trigger 없음)는 프로젝트의 유일한 마우스 휠 mapping이며 공용 휠 intent다. 컴퓨터를 쓰지 않을 때는 배치 회전, 컴퓨터 사용 중에는 화면 스크롤로 쓰인다(분기는 C++). `BP_FirstPersonCharacter.PlacementRotateAction=IA_PlacementRotate`이고 `IMC_Default`·`IMC_MouseLook`에는 mapping이 없다.
- 아키텍처 설계 없이 이 mapping이나 `IA_PlacementRotate`에 modifier·trigger를 추가하거나 두 번째 휠 mapping을 만들지 않는다. 자동화 `ScreenWheelContentContract`가 이 전제를 load 검증한다.


## 서비스 단계 연결

`BP_FirstPersonCharacter.FirstPersonCamera`의 PostProcessSettings `WeightedBlendables`에 `/Game/Bathhouse/Materials/Service/M_PP_TakeHighlightOutline`(weight 1)이 저장돼 있다(레벨 PostProcessVolume 아님). `/Game/Bathhouse/UI/WBP_InteractionPrompt`에는 `HeldSummaryText`(TextBlock, `BindWidgetOptional`)가 `PromptRoot`(Overlay) 자식으로 저장돼 있다. Overlay slot padding top 600·Left/Top 정렬, 폰트·색·그림자는 `ActionNameText`와 동일(Roboto Bold 24), 기본 Visibility Collapsed(C++가 표시 제어). 기존 필수 BindWidget 15개는 모두 유지된다. RMB 행으로 `RmbKeyText`("RMB", 위쪽 640·왼쪽 0), `HeldTakeActionNameText`(640·80), `HeldTakeFailureReasonText`(680·80) TextBlock 세 개가 `PromptRoot` 자식으로 저장돼 있다(Collapsed 기본, 기존 `ActionNameText`와 같은 폰트·색·그림자, `BindWidgetOptional`). 텍스트·표시는 C++가 제어한다. `PrimaryKeyText`·`LmbKeyText`는 이 WBP에 없다.

## 세신 HUD

`/Game/Bathhouse/UI/WBP_ScrubFocusHud`(parent `ScrubFocusHudWidget`)는 루트 `RootOverlay` 아래 하단 중앙 `ScrubColumn`(VerticalBox, 아래 여백 90)에 `ScrubGaugeBar`(ProgressBar, 360×22 SizeBox 안, 노란색)와 `ScrubWaitText`(TextBlock, 중앙 정렬, Roboto Bold 24, 그림자)를 둔다(둘 다 BindWidget). `TickFrequency=Auto`이고 root 기본 Visibility는 SelfHitTestInvisible이며 graph는 없다. `BP_BathhouseHUD.ScrubFocusHudWidgetClass=WBP_ScrubFocusHud_C`이고 기존 InteractionPrompt·MoneyHud·ShopNotice class는 유지된다. 기존 Input Action/Mapping 변경은 없다.
