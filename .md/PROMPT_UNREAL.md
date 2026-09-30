# Unreal Editor 인계 — 서비스 1단위 수직(품목 박스·진열·음료 냉장고·수거함) 재작업본

## 단계와 현재 상태

- 작업 필요. 현재 단계는 **수직 구현**의 Editor 작업이다. 음료는 바나나우유 1종이며 화장대·샤워 비품·수건 변경은 만들지 않는다.
- C++·UE 5.8 build·copy-first load gate·전체 자동화(88개, 실패 0)는 통과했다(`.md/PROMPT_REVIEW.md`). C++ 단계에서 Content·Config·Level을 저장하지 않았다.
- 재작업으로 바뀐 Editor 관련 사실:
  - 냉장고 payload는 `FinishSpawning`(Blueprint SCS component 생성) **뒤** `IPlaceableFacility::FinalizePlacementPayloadAfterConstruction`에서 적용된다. 따라서 공간·slot은 `BP_DrinkFridge`의 SCS component로 두면 된다.
  - `ADrinkFridgeActor::IsDataValid`는 CDO에서 native subobject와 Blueprint 상속 사슬의 SCS template을 모아 검사한다(공간 index 고유·연속 0부터, 자리 ≥ 1, 분류 태그, slot 정확히 1).
  - 품목 박스 Editor 미리보기는 `EWorldType::Editor`와 `EditorPreview`(Blueprint 에디터 뷰포트) 모두에서 동작한다. game world는 항상 실제 내용으로 다시 그린다.
- Editor 프로세스는 이 단계에서 실행하지 않았다. 백업: `Saved/MigrationBackup/20260930_service_unit1/`(`DA_ShopCatalog`, `WBP_InteractionPrompt`, `BP_Shower`).
- `DrinkFridge` 공간 `DisplayOffset`은 자리 기준 로컬 오프셋(`DisplayOffset * SlotTransforms[i]`)이다. 정본과 구현이 같다.

## 저장 allowlist (각각 개별 Save, 그 밖은 저장 금지)

신규:

| Asset | 경로 | Parent / 종류 |
|---|---|---|
| 품목 정의 | `/Game/Bathhouse/Data/Service/DA_ServiceItem_BananaMilk` | `ServiceItemDefinition` |
| 품목 박스 | `/Game/Bathhouse/Blueprints/Service/BP_ItemBox` | `ItemBoxActor` |
| 냉장고 | `/Game/Bathhouse/Blueprints/Service/BP_DrinkFridge` | `DrinkFridgeActor` |
| 수거함 | `/Game/Bathhouse/Blueprints/Service/BP_DrinkCollectionBox` | `DrinkCollectionBoxActor` |
| 배치 정의 | `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_DrinkFridge` | `FacilityPlacementDefinition` |
| 외곽선 | `/Game/Bathhouse/Materials/Service/M_PP_TakeHighlightOutline` | Post Process Material |
| 프리뷰 | `/Game/Bathhouse/Materials/Service/MI_DisplayInsertPreview` | 반투명 Unlit, Two Sided |

기존 수정:

- `/Game/Bathhouse/Data/Shop/DA_ShopCatalog`: 기존 7종 유지 + 냉장고(30,000, `PlacementDefinition`) + 바나나우유 박스(12,000, `ItemBoxDefinition`). 상품마다 두 정의 중 정확히 하나.
- `/Game/FirstPersonCharacter/BP_FirstPersonCharacter`: `FirstPersonCamera` PostProcessSettings Weighted Blendables에 `M_PP_TakeHighlightOutline` weight 1. 그 밖 변경 없음(입력 참조·`PlayerHeldTargetUse` 0.15 유지).
- `/Game/Bathhouse/UI/WBP_InteractionPrompt`: `HeldSummaryText`(TextBlock, BindWidgetOptional) 추가. 기존 필수 BindWidget 15개와 RMB 행·키 라벨 유지.
- Project Settings: `Config/DefaultGame.ini`(`ShopSettings.ItemBoxClass` = `BP_ItemBox`, `ServiceDisplaySettings.InsertPreviewMaterial` = `MI_DisplayInsertPreview`), `Config/DefaultEngine.ini`(Rendering `Custom Depth-Stencil Pass = Enabled with Stencil`). Config는 이 항목에 한정한다.
- 수거함 배치: DefaultMap 카운터에 `BP_DrinkCollectionBox` 1개. DefaultMap은 World Partition이므로 이 Actor는 `/Game/__ExternalActors__/Maps/DefaultMap/...` external actor package로만 저장된다. `DefaultMap.umap`은 저장하지 않는다. 저장 뒤 `git status`에서 `.umap` 변경이 없는지 확인한다.
- `Content/Developers/MigrationCheck/`는 비어 있어야 한다.

## Asset 계약

- `DA_ServiceItem_BananaMilk`: `ItemId` 고유, `DisplayName` 바나나우유, 임시 `ItemMesh`, `BoxCapacity` 12, `BoxSlotTransforms` 정확히 12개(박스 root 로컬 cm, 배열 순서가 채우는 순서), `DisplayCategories`에 `Display.Fridge`, `SaleValue` 2000. `DisplayMesh` 비우면 `ItemMesh`. Data Validation 통과.
- `BP_ItemBox`: 루트 `BoxMesh` 위가 열린 박스(QueryAndPhysics, CCD, Pawn Ignore, 상대 위치·회전 0, 양의 scale, 가장 큰 배치가 들어가는 크기). `HeldTransform`은 location·rotation만 사용. `ContentsVisual` NoCollision 유지. `EditorPreviewKind`(`DA_ServiceItem_BananaMilk`)·`EditorPreviewCount`는 미리보기 전용이다.
- `BP_DrinkFridge`: `DisplaySpaceComponent` 4개(`SpaceIndex` 0~3, `AcceptedCategory` `Display.Fridge`, `SlotTransforms` 자리 6개씩, `BoxExtent`가 조준 범위; QueryOnly·Visibility Block·Navigation off 유지), 손님 자리 `BathhouseFacilitySlotComponent` 정확히 1개. `FacilityType`은 DrinkFridge(C++ 기본값). PlacementFootprint·PackagePhysicalRoot는 기존 설비 BP와 같은 구조.
- `DA_FacilityPlacement_DrinkFridge`: `Facility.Placeable`·`Facility.Discardable`, `PlacedFacilityClass` = `BP_DrinkFridge`, `RecoveryItemClass` = `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem`.
- `BP_DrinkCollectionBox`: 루트 `BoxMesh` Visibility Block. carry·placeable·discardable 아님.
- `M_PP_TakeHighlightOutline`(Post Process domain, Blendable Location Before Tonemapping; 두께·색은 파라미터) 필수 규칙:
  1. 판정은 CustomStencil이 설정값(기본 1)과 **같은지**로만 한다.
  2. 이웃 샘플 좌표는 `ViewportUV` 오프셋 → `[0,1]` clamp → `ViewportUV → BufferUV` 변환으로 읽는다. view rect 밖 버퍼를 읽지 않는다.
  3. 외곽선 픽셀 = 중심은 강조 아님 AND 이웃 중 하나가 강조. 이웃 CustomDepth ≤ 이웃 SceneDepth + bias일 때만 인정한다.
  - 레벨 PostProcessVolume에는 넣지 않는다.

## 갱신할 `.md/Unreal/*System.md` (Editor 단계 소유, 저장·재로드 확인 뒤 현재 상태만 기록)

- 신규 `.md/Unreal/ServiceSystem.md` + `0_UNREAL.md` 지도 항목 추가.
- `ShopSystem.md`: catalog 상품 2개와 `ItemBoxClass`.
- `PlacementSystem.md`: Definition 표에 `DA_FacilityPlacement_DrinkFridge`.
- `InteractionUISystem.md`: WBP `HeldSummaryText`, 카메라 blendable.
- 미완료 MCP 불가 작업(Project Settings 렌더링 옵션, 머티리얼 그래프 등)은 우회하지 않고 `.md/USER_UNREAL.md`에 남긴다. 이전 held-use 단계의 미완료 항목(`IMC_FirstPerson` RMB 등)은 그 문서가 계속 소유한다.

## Editor 검증 방법

1. Blueprint Compile, 개별 Save, 새 Editor 프로세스 reload. 각 asset Data Validation.
2. `BP_DrinkFridge` 구조는 두 가지로 확인한다(CDO에서 BP 검사가 통과해도 SCS 값이 맞다는 뜻이 아니므로 readback을 병행한다):
   - Data Validation(SCS template 검사 포함).
   - MCP로 SCS template readback: 공간별 `SpaceIndex`, `SlotTransforms` 수(6), `AcceptedCategory`, slot 수(1).
3. 박스 미리보기: `BP_ItemBox` Blueprint 에디터 뷰포트에서 `EditorPreviewKind`·`EditorPreviewCount`(예: 7)를 바꿔 병 7개가 박스 안에 보이는지 확인한다. 레벨 인스턴스로 확인했다면 저장하지 않는다(그 인스턴스는 game에서 초기화되지 않은 박스라 들 수 없다).
4. 전체 자동화 재실행(`Automation RunTests BathhouseSim`). `BathhouseSim.Service.BlueprintLoad`는 원본 상태로 통과해야 한다.
5. 콘솔 명령(조준한 냉장고, 사용자 = 플레이어 Pawn, 결과는 Output Log): `bathhouse.Debug.DrinkFridge.Reserve` / `.Take` / `.Release`.

## PIE 절차 — 대표 시나리오(순서대로 1회)

1. 컴퓨터 상점에서 냉장고(30,000)와 바나나우유 박스(12,000)를 산다. 배송 상자가 배송 지점에 나타난다.
2. 배송 상자를 E로 들고 LMB로 개봉한다. 냉장고 설비 아이템과 `바나나우유 12/12` 박스가 무리로 나온다(DISP-001).
3. 냉장고 아이템을 설치 모드로 배치한다. 배치 직후 네 공간을 각각 조준하면 HUD `빈 공간 0/6`이 네 번 나온다(FRDG-001·012). 배치가 실패하면 진열 공간 구성(SCS)을 먼저 의심한다.
4. 박스를 E로 든다. HUD 요약 `바나나우유 12/12`가 조준 대상과 무관하게 보인다(DISP-002).
5. 첫 공간을 조준한다. 첫 자리에 반투명 병과 HUD `LMB 넣기`가 보인다(DISP-003). LMB를 누르고 유지한다: 0.15초마다 1병씩 들어가 `6/6`에서 멈추고 프리뷰가 사라지며 `가득 참`이 보인다(DISP-005).
6. RMB를 한 번 누른다. 여섯째 병이 박스로 돌아오고 마지막 자리 외곽선 강조가 보인다(DISP-006). 화면 네 테두리에 선이 없어야 한다(창 크기·Screen Percentage 변경 포함).
7. 냉장고를 Q Hold로 회수하고 아이템을 조준한다. HUD 이름에 `— 바나나우유 N병` 요약이 붙는다(FRDG-009). 다시 배치하면 같은 공간에 같은 수량이 복원된다. Q 취소 시 진열이 그대로다(FRDG-010).
8. 콘솔 `Reserve` → `Take`로 병을 가져가 수거함 금액이 오르는 것을 확인한다(FRDG-006). 그 동안 `Q Hold` 회수는 거부된다(FRDG-008).
9. 카운터의 수거함을 E로 수금한다(무엇을 들고 있어도). 지갑이 오르고 수거함이 0원이 된다. 다시 E는 `모인 돈 없음`(FRDG-004·005).

## DISP 관찰 방법 (키, 조준 대상, 기대 HUD, 병 위치)

| ID | 방법 | 기대 |
|---|---|---|
| DISP-003·004 | `12/12` 박스를 들고 빈 공간 조준, LMB 1회 | 프리뷰가 첫 자리 → LMB 후 병이 첫 자리, HUD 공간 `바나나우유 1/6`, 박스 `11/12`, 박스 안 병 1개 감소, 프리뷰가 둘째 자리로 이동 |
| DISP-007 | `3/6` 공간, 빈 박스, RMB 유지 | 셋째·둘째·첫째 자리 순으로 빠지고 박스 `바나나우유 3/12`, 공간이 비면 멈추고 `빈 공간 0/6` |
| DISP-008 | 가득 찬 박스, 물품 있는 공간, RMB | `박스 가득 참`, 변화 없음 |
| DISP-009 | 빈 박스·빈 공간, LMB/RMB | `박스가 비어 있음` / `꺼낼 물건 없음`, 프리뷰·강조 없음 |
| DISP-010 | LMB 유지 중 같은 냉장고 옆 공간으로 조준 이동 | 이동 즉시 멈춤, 새 공간 자동 재개 없음, 키를 떼고 다시 누르면 시작 |
| DISP-011 | LMB 유지 중 G 또는 컴퓨터 진입 | 즉시 멈춤, 옮긴 병 유지 |
| DISP-012 | 박스를 든 채 공간에 E·F | 옮기지 않음, HUD E 행 없음 |
| DISP-013 | Project Settings `bShowTakeHighlight` 끔 | 강조 없음, 넣기 프리뷰와 RMB는 그대로 |
| DISP-018·019 | 박스에서 5병 넣은 뒤 / G 드롭 | 박스 안 병이 마지막 넣은 자리부터 5개 사라져 7개 / 드롭해도 수량 그대로 |
| DISP-020·022 | 냉장고가 아닌 곳, 또는 수건바구니를 든 채 공간에 LMB·RMB | 아무 일도 없음 |
| DISP-021 | 박스·진열이 든 냉장고 아이템을 쓰레기통 E | 버려지고 내용 소멸, 지갑·수거함 금액 불변 |

## Blueprint에서 구현하면 안 되는 로직

- 품목 이동·정원·종류 잠금·이유 문구, 판매 적립, 수금, 회수 payload 적용, 프리뷰·강조 위치와 표시 조건은 모두 C++다.
- Blueprint는 mesh·component 배치·자리 transform·asset 연결·머티리얼만 담당한다. 진열 규칙이나 금액을 그래프에 복제하지 않는다.
