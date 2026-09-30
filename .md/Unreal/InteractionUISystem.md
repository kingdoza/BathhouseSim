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

`WBP_BathWaterCapacitySummary`는 `SummaryFrame > SummaryColumn > SummaryRows` 아래 `CirculationRow/HeatingRow/CoolingRow` 세 개를 같은 가로 Fill 비율로 유지한다. 제목과 행 레이블은 `설비 용량`, `순환/가열/냉각`이다. 각 행의 값 Text는 기존 native `예약/가동/설치` 문자열을 13pt Bold로, 상태 Text는 설치·가동 부족이 동시에 나오는 긴 문구를 10.5pt Bold로 표시하고 둘 다 최대 280px 줄바꿈을 허용한다. 레이블은 11pt, 제목은 14pt이며 행 좌우 8px과 값·Bar·상태 사이 작은 세로 간격을 둔다. Bar 세 개와 9개 BindWidget 이름·타입, native 계산과 아래 지도·상세 layout은 변경하지 않았다. 이 WBP 하나만 Editor Python API로 개별 저장·새 프로세스 재로드·컴파일(`BS_UP_TO_DATE`)을 확인했으며 실제 1024×576 플레이 화면 잘림 여부는 직접 확인이 남아 있다.

`WBP_BathWaterMap`의 `BathTileWidgetClass`는 `WBP_BathWaterBathTile_C`다. **화면 안에 배치된 `BathMap` 위젯 템플릿에도 같은 class가 저장돼 있다.** Class Default만 설정하면 중첩 템플릿에서 `None`으로 남아 PIE 타일이 0개가 될 수 있다. `MapStack`의 순서는 `GridCanvas` → `BathTileCanvas` → `EmptyStateText`이며 두 Canvas는 `ClipToBounds`, `GridCanvas`는 `HitTestInvisible`이다. 격자선과 Zone 경계의 위치·수명은 native Map Widget이 소유하고 타일의 Button hit test를 가리지 않는다. Detail slider 두 개는 0~1 범위다. Root는 `RootOverlay > ManagementSize (SizeBox 1024×576) > ManagementFrame > ManagementColumn` 안에서 제목, 전체 폭의 utility summary, 그 아래 지도(좌)와 detail(우)을 배치한다. Detail은 `ScrollBox` 안에 있다. 어두운 패널/지도 바탕에 밝은 텍스트, 순환 초록·가열 주황·냉각 청록 색을 사용한다. 지도 좌표와 타일 수명은 WBP graph가 아닌 native Map Widget 계약을 따른다.

## 컴퓨터 연결

- `/Game/Bathhouse/Blueprints/Computer/BP_BathhouseComputer`의 inherited `ScreenWidget.WidgetClass`는 `/Game/Bathhouse/UI/WBP_BathWaterManagementScreen.WBP_BathWaterManagementScreen_C`다.
- ScreenWidget은 World Space, Draw Size (1024,576), Receive Hardware Input false를 유지한다. Focus camera 기본값은 변경하지 않았다.
- 새 Editor 프로세스 재로드에서 BP_BathhouseComputer CDO의 FocusExitPoint 위치는 (1500,0,-228.5714285714), 회전은 (0,180,0), SearchRadius는 100cm였다. FocusExitArrow는 이 컴포넌트 자식이며 local 원점/회전 0, editor-only, 길이 80cm다. DefaultMap 컴퓨터 인스턴스도 동일한 상대 transform을 재로드했다. Actor transform은 위치 (-470,0,160), yaw 0, scale (0.12,1.2,0.7)이며 계산된 world 발 위치는 (-290,0,0), 방향은 컴퓨터를 향하는 -X다. 이전 MCP 인계의 X=1000은 stale한 값이며 새 프로세스 재로드 결과를 현재 저장 상태로 본다.
- Level computer instance의 ManagedBathPlacementZone은 BP_FacilityPlacementZone_C_UAID_F02F7433CA36D1FF02_1155169559를 참조한다. CDO 속성은 None이므로 class default와 Level instance 참조를 구분한다.
- BP_FirstPersonCharacter.CancelAction은 IA_Cancel이고 기존 InteractAction은 IA_Interact다. IA_Cancel은 Boolean Input Action이다. IMC_FirstPerson은 기존 7개 mapping과 Escape → IA_Cancel을 포함해 8개다. BP_FirstPersonController.DefaultMappingContext는 IMC_FirstPerson이다.
- 변경된 네 asset(BP_BathhouseComputer, BP_FirstPersonCharacter, IMC_FirstPerson, IA_Cancel)은 각각 저장한 뒤 새 프로세스에서 다시 읽었고 모두 clean이었다. DefaultMap도 clean이며 성공한 map/external actor 저장은 없었다. 앞선 SceneTools.save_actor 시도는 external actor registry 경로 오류로 실패했다.

- `/Game/Maps/DefaultMap.DefaultMap:PersistentLevel.BP_BathhouseComputer_C_UAID_F02F7433CA3690F802_2051456727`의 `ManagedBathPlacementZone`은 같은 Level의 `BP_FacilityPlacementZone_C_UAID_F02F7433CA36D1FF02_1155169559`를 참조한다.
- Level reference는 World Partition external actor `/Game/__ExternalActors__/Maps/DefaultMap/7/EH/E4FLO971KSWUJ40H7W7PHK`에 저장됐다. `DefaultMap.umap` 자체는 이 연결 때문에 변경하지 않았다.

UE 5.8 DLL 빌드 후 새 Editor에서 다섯 WBP와 컴퓨터 BP의 Data Validation 6/6 `VALID`를 확인했다. PIE의 1024×576 RenderTarget에는 Zone 격자·경계, Bath 타일 2개, utility summary와 detail이 표시됐다. 타일 Button의 `OnClicked` 이벤트를 호출하면 선택·detail·slider 활성화가 갱신된다. 실제 플레이어 LMB 조준/클릭과 물 제어·회수 전체 시나리오는 아직 직접 플레이 검증이 필요하며 [USER_UNREAL.md](../USER_UNREAL.md)에 남겼다. 이번 FocusExitPoint/취소 입력 변경은 fresh-process reload와 PIE 시작·종료까지만 확인했고, Data Validation과 E/ESC/마우스 입력 수용은 미완료다.


## 서비스 단계 연결

`BP_FirstPersonCharacter.FirstPersonCamera`의 PostProcessSettings `WeightedBlendables`에 `/Game/Bathhouse/Materials/Service/M_PP_TakeHighlightOutline`(weight 1)이 저장돼 있다(레벨 PostProcessVolume 아님). `/Game/Bathhouse/UI/WBP_InteractionPrompt`에는 `HeldSummaryText`(TextBlock, `BindWidgetOptional`)가 `PromptRoot`(Overlay) 자식으로 저장돼 있다. Overlay slot padding top 600·Left/Top 정렬, 폰트·색·그림자는 `ActionNameText`와 동일(Roboto Bold 24), 기본 Visibility Collapsed(C++가 표시 제어). 기존 필수 BindWidget 15개는 모두 유지된다. 이 WBP에는 `HeldTake*`, `PrimaryKeyText`, `LmbKeyText`, `RmbKeyText`가 아직 없다([USER_UNREAL.md](../USER_UNREAL.md)).
