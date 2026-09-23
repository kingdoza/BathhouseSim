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

`WBP_BathWaterMap`의 `BathTileWidgetClass`는 `WBP_BathWaterBathTile_C`다. **화면 안에 배치된 `BathMap` 위젯 템플릿에도 같은 class가 저장돼 있다.** Class Default만 설정하면 중첩 템플릿에서 `None`으로 남아 PIE 타일이 0개가 될 수 있다. `MapStack`의 순서는 `GridCanvas` → `BathTileCanvas` → `EmptyStateText`이며 두 Canvas는 `ClipToBounds`, `GridCanvas`는 `HitTestInvisible`이다. 격자선과 Zone 경계의 위치·수명은 native Map Widget이 소유하고 타일의 Button hit test를 가리지 않는다. Detail slider 두 개는 0~1 범위다. Root는 `RootOverlay > ManagementSize (SizeBox 1024×576) > ManagementFrame > ManagementColumn` 안에서 제목, 전체 폭의 utility summary, 그 아래 지도(좌)와 detail(우)을 배치한다. Detail은 `ScrollBox` 안에 있다. 어두운 패널/지도 바탕에 밝은 텍스트, 순환 초록·가열 주황·냉각 청록 색을 사용한다. 지도 좌표와 타일 수명은 WBP graph가 아닌 native Map Widget 계약을 따른다.

## 컴퓨터 연결

- `/Game/Bathhouse/Blueprints/Computer/BP_BathhouseComputer`의 inherited `ScreenWidget.WidgetClass`는 `/Game/Bathhouse/UI/WBP_BathWaterManagementScreen.WBP_BathWaterManagementScreen_C`다.
- `ScreenWidget`은 World Space, Draw Size `(1024,576)`, Receive Hardware Input `false`를 유지한다. Focus camera와 입력 설정은 변경하지 않았다.
- `/Game/Maps/DefaultMap.DefaultMap:PersistentLevel.BP_BathhouseComputer_C_UAID_F02F7433CA3690F802_2051456727`의 `ManagedBathPlacementZone`은 같은 Level의 `BP_FacilityPlacementZone_C_UAID_F02F7433CA36D1FF02_1155169559`를 참조한다.
- Level reference는 World Partition external actor `/Game/__ExternalActors__/Maps/DefaultMap/7/EH/E4FLO971KSWUJ40H7W7PHK`에 저장됐다. `DefaultMap.umap` 자체는 이 연결 때문에 변경하지 않았다.

UE 5.8 DLL 빌드 후 새 Editor에서 다섯 WBP와 컴퓨터 BP의 Data Validation 6/6 `VALID`를 확인했다. PIE의 1024×576 RenderTarget에는 Zone 격자·경계, Bath 타일 2개, utility summary와 detail이 표시됐다. 타일 Button의 `OnClicked` 이벤트를 호출하면 선택·detail·slider 활성화가 갱신된다. 실제 플레이어 LMB 조준/클릭과 물 제어·회수 전체 시나리오는 아직 직접 플레이 검증이 필요하며 [USER_UNREAL.md](../USER_UNREAL.md)에 남겼다.
