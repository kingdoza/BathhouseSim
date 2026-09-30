# Service System

## Status And Scope

- 2026-09-30 설계. 입력은 `.md/PROMPT_ARCHITECTURE.md`(서비스 1단위: 진열 기반, 음료 냉장고)와 `.md/NextWork/QNA_FEATURE_SPEC.md`의 해당 Q다. 사용자 결정: 꺼내기 강조는 Custom Depth Stencil + 후처리 외곽선(2026-09-30).
- 구현 상태: 1단위 완료(2026-09-30 사용자 확인, 커밋 `5b42a47`). Source 구현, 코드 리뷰의 아키텍처 재검토(F1~F5) 재작업, Editor authoring까지 끝났다.
- 2단위(진열 확장) 설계: 2026-09-30, [ServiceFacilityDisplaySystem.md](ServiceFacilityDisplaySystem.md)가 정본이다. 이 문서의 1단위 구조 중 다음을 일반화한다.
  - 냉장고 payload → 공용 `UServiceDisplayManagerComponent`와 설비 extension
  - 공간 proxy → `UDisplayCueComponent`
  - 냉장고 회수 보류·사용자 정리 → 설비 base·manager
- 재작업 이력(1단위):
  - F1: 냉장고 payload 적용을 construction 뒤 단계로 옮겼다(Placement Payload Extension).
  - F2: Editor 정확한 경로, Data Validation의 SCS 검사, 박스 미리보기 world 범위를 정했다.
  - F3: 구현 API 이름과 `DisplayOffset` 순서를 정본에 맞췄다.
  - F5: 상점 개봉 파일을 분리했다.
- 수직 구현 단계다. 범위: 품목 박스, 진열 공통 규칙(넣기·빼기·프리뷰·강조·수량 표시), 음료 냉장고, 음료 공용 수거함, 상점 판매·개봉, 냉장고 회수 보존, 손님 이용 계약의 시설 쪽.
- 이후 단위(화장대·샤워 비품)는 품목 정의·품목 박스·진열 공간 규칙을 재사용한다. 그 기능을 미리 만들지 않는다.
- 비대상: 손님 루틴 행동, 만족도, 수건 시스템 변경, 쓰레기·수거 구역, 설정 메뉴, 저장.

## Source Scope

```text
Source/BathhouseSim/Public/Service/
  ServiceItemTypes.h            FServiceItemStack, 진열 공간 snapshot, native tag Display.Fridge
  ServiceItemDefinition.h       UServiceItemDefinition (UPrimaryDataAsset)
  ServiceDisplaySettings.h      UServiceDisplaySettings (UDeveloperSettings)
  ItemBoxActor.h                AItemBoxActor
  DisplaySpaceComponent.h       UDisplaySpaceComponent
  DrinkFridgeActor.h            ADrinkFridgeActor : ABathhouseFacilityActor
  DrinkFridgePlacementInstanceData.h
  DrinkSalesSubsystem.h         UDrinkSalesSubsystem (UWorldSubsystem)
  DrinkCollectionBoxActor.h     ADrinkCollectionBoxActor
Source/BathhouseSim/Private/Service/
  (위 .cpp), ServiceItemTransfer.h/.cpp(순수 규칙·원자 이동), DrinkFridgeDebugCommands.cpp(비 shipping)
  ServiceDisplaySettings.cpp     settings getter·`LoadInsertPreviewMaterial` 정의 위치
```

새 Source 폴더 `Service`는 이 문서가 정본이다.

## Ownership

| 대상 | 상태 owner | 실행 owner |
|---|---|---|
| 품목 종류 값(정원·배치·외형·음료 값·진열 분류) | `UServiceItemDefinition` asset | 읽기 전용 |
| 박스 내용(종류·수량) | `AItemBoxActor` | 박스 + `FServiceItemTransfer` |
| 진열 공간 내용(종류·수량) | `UDisplaySpaceComponent` | 공간 + `FServiceItemTransfer` |
| 냉장고 손님 자리·회수 보류 | `ADrinkFridgeActor`(기존 facility slot) | 냉장고 |
| 음료 판매 적립금 | `UDrinkSalesSubsystem` | subsystem |
| 수금 | 없음 | `ADrinkCollectionBoxActor` → wallet |
| 넣기·빼기 입력·연속 | 없음 | 기존 `UPlayerHeldTargetUseComponent` |
| 프리뷰·강조 표시 | 없음(표시 상태만) | 공간 component |

## Item Definition

`UServiceItemDefinition : UPrimaryDataAsset`, 종류 하나당 asset 하나(`DA_ServiceItem_BananaMilk`).

| 필드 | 규칙 |
|---|---|
| `ItemId` (FName) | 고유, None 금지 |
| `DisplayName` (FText) | HUD·요약 문구 |
| `ItemMesh` (UStaticMesh) | 필수. 박스·진열 외형 |
| `BoxCapacity` (int32) | ≥ 1. 바나나우유 12 |
| `BoxSlotTransforms` (TArray<FTransform>) | 박스 root 기준 cm. 개수 = `BoxCapacity`. 배열 순서가 채우는 순서 |
| `BoxItemOffset` (FTransform, 기본 Identity) | 2단위 추가([ServiceFacilityDisplaySystem.md](ServiceFacilityDisplaySystem.md) Item Definition Additions, `ConsumableUses`와 함께). 박스 안 모든 자리에 공통으로 적용하는 품목 보정. 자리 i의 최종값은 `BoxItemOffset * BoxSlotTransforms[i]`(품목 자기 기준 공통 회전·스케일·피벗 보정 → 자리 transform). 회전은 합성, 스케일은 곱. 공통 스케일은 균일값 권장(비균일 + 자리 회전은 찌그러짐). Validation은 finite·스케일 양수. 진열 쪽 `DisplayOffset`과 별개 |
| `DisplayMesh` (선택) / `DisplayOffset` (FTransform) | 진열 공간 자리 위 외형. 없으면 `ItemMesh` |
| `DisplayCategories` (FGameplayTagContainer) | 넣을 수 있는 진열 분류. 음료는 `Display.Fridge` |
| `SaleValue` (int32) | ≥ 0. 손님이 가져갈 때 적립. 바나나우유 2000 |

Data Validation이 위 규칙과 transform finite·scale 양수를 검사한다. 새 음료는 asset을 추가하고 상점 catalog에 박스 상품을 추가하면 된다.

## Stack And Transfer

- `FServiceItemStack { TObjectPtr<UServiceItemDefinition> Kind; int32 Count; int32 Revision; }`. Count 0이면 Kind null이다(종류 풀림). Count > 0이면 Kind 필수.
- 박스 정원 = `Kind ? Kind->BoxCapacity : 0`. 빈 박스는 첫 물품의 정원을 따른다. 공간 정원 = 그 공간의 자리 수다.
- `FServiceItemTransfer`(Private, 순수 static, 구현 이름 기준):
  - `EvaluateApply(Box, Space, ...)`, `EvaluateTake(Box, Space, ...)`: side-effect 없이 `FServiceTransferEvaluation{bCan, Reason}`을 반환한다.
  - `TryApplyOne`(박스 → 공간), `TryTakeOne`(공간 → 박스): 재평가 뒤 두 stack을 연속 변경하고 두 Revision을 올린다. 실패 시 둘 다 그대로다.
  - `TryRemoveOne(Space, OutKind, OutFailure)`: 손님용 한 개 제거.
  - `GetBoxCapacity(Box, Space)`: 빈 박스로 Take할 때는 공간 종류의 정원을 쓴다. `IsConsistent(Stack)`.
  - 변경 알림은 두 owner가 commit 뒤 방송한다.
- 이유 우선순위(첫 번째를 씀). 문구는 기능 계약 표 그대로다.
  - Apply: 박스 종류가 공간 분류에 없음 `여기에 넣을 수 없는 물건` → 공간 종류와 다름 `다른 음료가 진열됨` → 박스 빔 `박스가 비어 있음` → 공간 가득 `가득 참`
  - Take: 박스 종류(비지 않은 박스)가 공간 분류에 없음 `여기에 넣을 수 없는 물건` → 박스 종류와 공간 종류 다름 `다른 음료가 진열됨` → 공간 빔 `꺼낼 물건 없음` → 박스 가득 `박스 가득 참`
- 채우기 순서: 넣으면 index Count 자리에 놓이고 Count+1. 빼면 index Count-1 자리에서 빠진다(박스·공간 공통, LIFO).
- 모든 이동은 game thread 동기 호출이다. 손님 가져가기와 플레이어 이동은 각자 `TryMoveOne`/`TryRemoveOne` 한 번이며 섞이지 않는다.

## Item Box

`AItemBoxActor`(Blueprintable, `BP_ItemBox`). 구현: `IPlayerInteractable`, `IPhysicalCarryable`, `IPhysicalCarryDiscardable`. 장비(`IHeldEquipmentUsable`)가 아니다.

- 컴포넌트: root `BoxMesh`(UStaticMeshComponent, physical root, 위가 열린 박스), `ContentsVisual`(UInstancedStaticMeshComponent, NoCollision).
- carry: `EPhysicalCarryKind::ItemBox`(enum 끝 append), capability `FreeDrop`만. CCD·Pawn Ignore·약한 release·FellOutOfWorld last-safe 복구는 기존 비도구 휴대물 규칙(배송 상자와 같은 구조)이다. held transform·scale 정책은 [ShopSystem.md](ShopSystem.md) Delivery Box의 통일 정책과 같다(class 소유 `HeldTransform`, CDO root scale 보존).
- 내용: `FServiceItemStack Contents`. `InitializeContents(Kind, Count)`는 spawn 중 한 번만(상점 개봉). 생성은 `static SpawnFilledBox(World, Class, Kind, Count, Transform, OutFailure)` factory만 쓴다(deferred spawn → `InitializeContents` → FinishSpawning). 이후 변경은 `FServiceItemTransfer`만이며, 공간이 `GetMutableContents()`로 stack을 넘기고 commit 뒤 `NotifyContentsChanged()`로 방송한다. drop·던지기·복구에서 내용이 보존되고 쏟아지지 않는다(DISP-019).
- 외형: `ContentsVisual` instance는 `Kind->BoxSlotTransforms[0..Count-1]`에 `ItemMesh`로 둔다. Contents 변경 시 다시 만든다(DISP-004, 018).
- E query: 빈손이면 `들기`. TargetName은 요약이다: `바나나우유 7/12` 또는 `빈 박스`(DISP-002, 014). 다른 물건을 들었으면 불가 이유.
- `GetHeldSummaryText()`(아래 HUD)도 같은 요약을 반환한다.
- 버리기: 항상 가능. 내용도 함께 사라지고 환불 없음(DISP-021). 3단위부터 수거 구역의 world 버리기도 구현한다([CleaningLitterSystem.md](CleaningLitterSystem.md)).
- 수건 대상 규칙은 품목 박스를 수건바구니로 보지 않으므로 수건을 옮기지 않는다. 진열 공간은 품목 박스만 받는다(DISP-022).
- Editor 미리보기(기능 계약: PIE 없이 종류·개수를 골라 박스 안 모습 확인):
  - WITH_EDITORONLY_DATA `EditorPreviewKind`, `EditorPreviewCount`(EditAnywhere)를 둔다.
  - `OnConstruction`과 CallInEditor `RefreshEditorPreview()`가 `EWorldType::Editor`와 `EWorldType::EditorPreview`(Blueprint 에디터 뷰포트) world에서만 `ContentsVisual`에 적용한다.
  - 기본 확인 방법은 `BP_ItemBox` Blueprint 에디터 뷰포트에서 Class Defaults의 두 값을 바꾸는 것이다.
  - 레벨에 놓은 미리보기 인스턴스는 저장하지 않는다. 게임에서 레벨 배치 박스는 내용이 초기화되지 않은 상태라 조준·들기 대상이 아니다. 박스는 상점 개봉으로만 생긴다.
  - game world `BeginPlay`는 미리보기를 지우고 실제 Contents로 다시 만든다.
- Data Validation: mesh 지정, root QueryAndPhysics·CCD·Pawn Ignore, root relative location/rotation 0·scale finite > 0, `HeldTransform` scale 경고.

## Display Space

`UDisplaySpaceComponent : UBoxComponent`. 진열 공간 하나 = 조준 대상 하나. 구현: `IPlayerInteractable`, `IPlayerInteractionFocusObserver`.

- 조준 판정: QueryOnly, Visibility Block, Navigation off. Box extent가 조준 범위다.
- authoring(EditAnywhere, 설비 BP 기본값):
  - `SpaceIndex`(int32): 설비 안 고유, 0부터 연속. payload key다.
  - `AcceptedCategory`(GameplayTag): 냉장고는 `Display.Fridge`.
  - `SlotTransforms`(TArray<FTransform>, MakeEditWidget): 이 component 기준 자리. 배열 순서가 채우는 순서다. 개수가 공간 정원(`GetCapacity()`)이다(기본 6).
- 상태: `FServiceItemStack Stock`.
- `IsOperational()`: owner가 `IPlaceableFacility`면 placed-domain active일 때만 true다. placeable이 아닌 owner(이후 단위의 레벨 고정 진열대)는 게이트하지 않는다.
- 설비 쪽 API: `ValidateAuthoring`, `CanAcceptKind`, `ValidateStockImport`/`ImportStock`(payload 적용, 검증 먼저), `RemoveOneForCustomer`, `PublishStockChanged`.
- 런타임 표현(Transient, OnRegister에서 생성, 저장하지 않음):
  - `StockVisual`: ISM. 자리 i의 transform은 `Kind->DisplayOffset * SlotTransforms[i]`다(UE 순서: child * parent, 즉 `DisplayOffset`은 자리 로컬 보정). 프리뷰·강조 proxy도 같은 식이다.
  - `InsertPreview`: StaticMesh, NoCollision, 그림자 없음. 설정의 반투명 머티리얼. 기본 숨김.
  - `TakeHighlightProxy`: StaticMesh, NoCollision, `bRenderInMainPass=false`, `bRenderInDepthPass=false`, `bRenderCustomDepth=true`, `CustomDepthStencilValue` = 설정값. 기본 숨김.
- query(held-use 계약, [HeldTargetUseSystem.md](HeldTargetUseSystem.md)):
  - TargetName: `바나나우유 4/6` 또는 `빈 공간 0/6`(박스 없이도, FRDG-001).
  - E: `ActionName` 비움(DISP-012). F 없음.
  - 품목 박스를 들었을 때만 Apply(`넣기`)·Take(`빼기`) 두 행을 표시하고 불가 쪽은 이유를 넣는다. 둘 다 `Repeat`. 그 밖의 들고 있는 물건이나 빈손에서는 두 방향을 모두 비운다(DISP-020, 022).
- `ExecuteHeldTargetUse`: Apply는 박스 → 공간, Take는 공간 → 박스 `TryMoveOne`. 연속·조준 이탈 멈춤은 기존 component가 처리한다. 같은 냉장고의 다른 공간은 다른 target object라 멈춘다(DISP-010).
- focus observer(표현 전용):
  - 넣기 프리뷰: 알림 query가 `bHeldApplyVisible && bCanHeldApply`면 `InsertPreview`를 `SlotTransforms[Count]`에 박스 종류 외형으로 보인다. 아니면 숨긴다(DISP-003, 005, 009).
  - 꺼내기 강조: `bHeldTakeVisible && bCanHeldTake`이고 설정 `bShowTakeHighlight`면 `TakeHighlightProxy`를 `SlotTransforms[Count-1]`에 공간 종류 외형으로 보인다. 아니면 숨긴다(DISP-006, 007, 013).
  - 이동 뒤 Count가 바뀌면 TargetName이 바뀌어 query가 바뀌므로 다음 알림에서 자리가 따라간다. focus 종료·suppression에서 둘 다 숨긴다.
- 변경 방송: `OnStockChanged(Snapshot)`(native·Blueprint). owner 설비가 구독해 가용 상태를 갱신한다.

## Take Highlight Outline

- 방식: Custom Depth Stencil + 후처리 외곽선(사용자 결정 A).
- 프로젝트 설정: Rendering `Custom Depth-Stencil Pass = Enabled with Stencil`(`r.CustomDepth=3`). Editor 단계가 Project Settings로 바꾼다. 현재 Custom Depth 사용처가 없어 기존 표현 회귀가 없다.
- 머티리얼 `M_PP_TakeHighlightOutline`(Editor 작성, Post Process domain, Blendable Location Before Tonemapping). 반드시 지킬 규칙:
  - 판정은 CustomStencil이 설정값과 **같은지**로만 한다. 깊이·"0이 아님"으로 판정하지 않는다.
  - 이웃 샘플 좌표는 `ViewportUV`에서 오프셋 → `[0,1]` clamp → `ViewportUV → BufferUV` 변환으로 읽는다. view rect 밖 버퍼를 읽지 않는다. 강조 대상이 있을 때 화면 테두리에 선이 생기던 문제(초기화되지 않은 view rect 밖 영역)를 막는다.
  - 외곽선 픽셀 = 중심은 강조 아님 AND 이웃 중 하나가 강조. 이웃의 CustomDepth ≤ 이웃의 SceneDepth + bias일 때만 인정해 벽 뒤에서 선이 보이지 않게 한다.
  - 두께·색은 머티리얼 파라미터다.
- 적용: `BP_FirstPersonCharacter`의 `FirstPersonCamera` PostProcessSettings Weighted Blendables에 weight 1로 한 번 붙인다. 레벨 PostProcessVolume에 넣지 않는다.
- 설정 꺼짐이면 proxy가 custom depth를 쓰지 않으므로 선이 그려지지 않는다.

## Drink Fridge

`ADrinkFridgeActor : ABathhouseFacilityActor`(`BP_DrinkFridge`). `FacilityType = DrinkFridge`(enum 끝 append). 배치·회수·Q Hold는 기존 설비 경로 그대로다.

- 공간: BP에 `UDisplaySpaceComponent` 4개(`SpaceIndex` 0~3, 자리 6개씩). 런타임은 `CollectSpaces`로 `GetComponents`를 모아 SpaceIndex 순으로 정렬한다. 규칙: index 고유·연속, 자리 ≥ 1, 분류 태그 지정, 손님 slot 정확히 1개.
- 공간과 slot은 Blueprint SCS component라 `FinishSpawning` 뒤에만 존재한다. 공간 구조 검증과 진열 적용은 construction 이후 단계에서만 한다(Placement Payload Extension).
- Data Validation(`IsDataValid`, WITH_EDITOR): CDO에는 SCS component가 없으므로 인스턴스 검사만으로는 항상 통과한다. 따라서 CDO일 때는 native subobject와 `UBlueprintGeneratedClass` 상속 사슬의 `SimpleConstructionScript` 노드 `ComponentTemplate`을 모아 같은 규칙을 검사한다. 인스턴스일 때는 `CollectSpaces`를 쓴다.
- 손님 자리: `UBathhouseFacilitySlotComponent` 1개(1명 점유, J9).
- `IsAvailableForReservation()` = Super && 진열 합계 > 0 && 회수 보류 아님 && placed-domain active(FRDG-007).
- 회수:
  - 기존 `QueryFacilityRecovery`가 slot이 Available이 아니면 거부한다(FRDG-008, "사용 또는 예약 중인 설비는 회수할 수 없습니다").
  - `TryBeginFacilityRecoveryHold` override: Super·recovery query 통과 시 `bRecoveryHoldActive=true`. 이후 새 예약 불가. `CancelFacilityRecoveryHold`가 해제한다(FRDG-010). 욕탕 override와 같은 패턴이다.
- 손님 사용자 정리: slot `OnSlotStateChanged`에서 Reserved/Occupied가 되면 그 사용자의 `OnEndPlay`를 구독한다. 아직 그 사용자면 `ForceRelease`한다. 이미 가져간 병과 적립금은 되돌리지 않는다.
- `TryTakeDrinkForCustomer(AActor& Customer, UServiceItemDefinition*& OutKind, FText& OutFailure)`:
  1. placed-domain active, 회수 보류 아님, slot 현재 사용자 == Customer를 확인한다.
  2. SpaceIndex 오름차순 첫 비지 않은 공간을 고른다. 없으면 실패.
  3. 그 공간 `TryRemoveOne`(LIFO) → `UDrinkSalesSubsystem::AddSale(Kind->SaleValue)`. 둘 다 실패 불가 단계로 연속 실행한다(FRDG-006).
  4. 공간 변경 방송은 commit 뒤다. 플레이어 넣기·빼기와 같은 game thread라 각 1병 이동은 원자적이다(FRDG-013).
- 회수 payload: 아래 Placement 확장.

## Placement Payload Extension

- `UBathhouseFacilityPlacementInstanceData`에서 `final`을 제거한다(Transient native, 직렬화 영향 없음).
- `ABathhouseFacilityActor`의 protected virtual hook(구현 완료, 호출 시점만 바뀜):
  - `CreateFacilityPlacementInstanceData(Outer)`: 기본은 base class NewObject.
  - `ExportFacilityExtension(Data, OutFailure) const`: 기본 true.
  - `ImportFacilityExtension(const Data* OrNullForFresh, OutFailure)`: 기본 true.
- 2단계 import(2026-09-30 재검토 F1):
  1. `ImportPlacementPayload`(construction 전): base 필드(type·번호·가중치·enabled)만 검증·적용한다. 확장 data 포인터(신규 설치면 null)는 Transient `PendingFacilityExtension`과 `bPendingFacilityExtension=true`로 보관만 한다. hook은 호출하지 않는다.
  2. `IPlaceableFacility::FinalizePlacementPayloadAfterConstruction(OutFailure)`(신규 virtual, 기본 true): transaction이 `FinishSpawning` 직후에 호출한다. `ABathhouseFacilityActor`는 pending이 있으면 `ImportFacilityExtension(Pending)`을 호출하고, 성공·실패와 관계없이 pending을 비운다. 실패는 transaction 실패로 전파되어 staged Actor 제거, item 미소모, publication 없음이다([PlacementSystem.md](PlacementSystem.md) staged 순서).
  - 기존 설비는 hook 기본값이라 결과가 같다. 호출 위치만 construction 뒤로 바뀐다.
- `UDrinkFridgePlacementInstanceData : UBathhouseFacilityPlacementInstanceData`: `TArray<FDisplaySpaceSnapshot{SpaceIndex, Kind, Count}>`.
  - export: 모든 공간 snapshot.
  - `ImportFacilityExtension`(construction 뒤): 먼저 `CollectSpaces`(공간 ≥ 1, slot 1 포함)를 검사한다. non-fresh면 공간 수·index 일치, Kind 분류 허용, Count ≤ 자리 수(`ValidateStockImport`)를 모든 공간에 대해 확인한 뒤에만 `ImportStock`을 적용한다. 하나라도 틀리면 아무것도 적용하지 않고 실패한다.
  - fresh(null): 구조 검사만 하고 모든 공간은 빈 상태다(FRDG-012).
- `UFacilityPlacementInstanceData`에 `virtual FText GetPlacementContentsSummary() const`(기본 빈 값)를 추가한다. 냉장고 data는 종류별 합계 `바나나우유 14병`을 반환한다. `APlaceableFacilityItemActor`는 표시 이름 뒤에 `— 요약`을 붙이고 `GetHeldSummaryText()`로도 반환한다(FRDG-009).
- 버리기: 냉장고 아이템은 `Facility.Discardable` 태그로 기존 쓰레기통 경로를 탄다. payload와 함께 진열도 사라진다(DISP-021).
- 자동화 fixture 규칙: 공간과 slot은 생성자 subobject가 아니라 construction 중에 생성해야 한다(예: fixture `OnConstruction`에서 NewObject + RegisterComponent). SCS와 같은 시점을 재현하고, 실제 `FFacilityActorConversionTransaction` 경로를 통과시킨다.

## Drink Sales And Collection

- `UDrinkSalesSubsystem : UWorldSubsystem`: `PendingAmount`(int32, ≥ 0), `AddSale(Value)`(int64 합산 후 int32 상한 clamp·경고), `TryCollect(UPlayerWalletComponent&, int32& OutAmount)`.
  - 0이면 실패("모인 돈 없음").
  - 아니면 `Wallet.TryAddMoney(Amount)` 성공 시에만 0으로 만든다. 실패하면 그대로 둔다.
  - 변경 시 `OnPendingAmountChanged`를 방송한다. 어느 냉장고든 한곳에 모인다(FRDG-011).
- `ADrinkCollectionBoxActor`(`BP_DrinkCollectionBox`, 레벨 고정물): root static mesh(Visibility Block), `IPlayerInteractable`만 구현한다. carry·placeable·discardable이 아니다(FRDG-015).
  - query: TargetName `음료 수거함 12,000원`. 금액 > 0이면 action `수금`, 0이면 불가 이유 `모인 돈 없음`. 들고 있는 물건과 무관하다(J1).
  - execute: fresh query 재검증 → wallet 해석(기존 cash 액터와 같은 PlayerState 경로) → `TryCollect`. 동기·재진입 guard로 빠른 반복 입력에서 한 번만 지급한다(FRDG-004, 005, 014).
- 레벨에 수거함이 없어도 적립은 subsystem에 쌓인다(경고 로그 한 번).

## Shop Integration

[ShopSystem.md](ShopSystem.md)의 확장이다.

- `FShopProductEntry`·`FShopOrderLine`에 `ItemBoxDefinition`(UServiceItemDefinition)을 추가한다. 상품은 `PlacementDefinition`과 `ItemBoxDefinition` 중 정확히 하나를 가진다. 규칙은 private `FShopProductRules`(`ValidateDefinitions`, `ValidateProduct`, `ValidateOrderLine`)가 catalog Validation·주문 평가·배송 상자 내용 검증에서 공용으로 쓴다. 박스 상품은 `BoxCapacity`·배치 유효가 조건이다. `Facility.Discardable` 요구는 설비 상품에만 적용한다.
- `UShopSettings::ItemBoxClass`(soft class, `AItemBoxActor` 자식)를 추가한다.
- 개봉:
  - 주문 줄을 수량만큼 펼친 항목은 설비 또는 품목 박스다.
  - 항목 shape는 `FShopUnboxItemShape`다. factory `FromFacilityDefinition`(기존 Definition query), `FromItemBoxClass`(`AItemBoxActor::BuildClassCollisionQuery`)가 만든다.
  - `FShopUnboxingPlacement::FindSpawnTransforms`는 shape 목록 하나만 받는다. 설비 전용 Definition overload는 제거하고, 테스트는 shape 경로로 옮긴다(F5).
  - `FShopUnboxItemShape`와 두 factory는 private `ShopUnboxItemShape.h/.cpp`로 분리한다. `ShopUnboxingPlacement.cpp`를 400줄 경고선 아래로 되돌린다.
  - 박스는 `AItemBoxActor::SpawnFilledBox(Kind, Kind->BoxCapacity)` → free-world 활성이다. 실패 rollback은 기존과 같다(DISP-001).
- 장바구니 수량·상한·결제·배송은 그대로다. 박스 1개 = 1개(SHOP-S02).

## HUD

- `IPhysicalCarryable::GetHeldSummaryText()`(신규 virtual, 기본 빈 값). 품목 박스와 내용 요약이 있는 설비 아이템만 반환한다.
- `FPlayerInteractionQuery::HeldObjectSummary`(신규 FText, `Equals` 포함). `UPlayerEquipmentUseComponent::MergeEquipmentQuery`가 들고 있는 carryable의 요약을 채운다. 이 merge는 들고 있는 물건을 이미 읽는 유일한 query 합성 지점이다.
- `UInteractionPromptWidget`: `BindWidgetOptional` `HeldSummaryText`. 요약이 있으면 조준 대상과 무관하게 표시한다(DISP-002).
- 진열 공간·박스·수거함의 수량·금액 표시는 각 target의 TargetName이다.

## Customer Contract Verification

- 손님 루틴이 없으므로 시설 쪽 API를 자동화로 검증한다. 테스트용 사용자 Actor가 slot `TryReserve`/`BeginUse` → `TryTakeDrinkForCustomer` → `Release`를 호출한다.
- PIE 확인용 비 shipping 콘솔 명령(`DrinkFridgeDebugCommands.cpp`, `#if !UE_BUILD_SHIPPING`). 대상은 로컬 플레이어가 조준 중인 냉장고이고, 사용자는 플레이어 Pawn이다.
  - `bathhouse.Debug.DrinkFridge.Reserve`: slot 예약 + BeginUse
  - `bathhouse.Debug.DrinkFridge.Take`: `TryTakeDrinkForCustomer`
  - `bathhouse.Debug.DrinkFridge.Release`: slot 해제

## Settings

`UServiceDisplaySettings : UDeveloperSettings`(Config=Game, "Bathhouse Service Display"):

| 값 | 기본 | 비고 |
|---|---|---|
| `bShowTakeHighlight` | true | 꺼내기 강조 옵션(Q56 A). 게임 설정 메뉴가 생기면 옮긴다 |
| `TakeHighlightStencilValue` | 1 | 1~255, 외곽선 머티리얼과 같은 값 |
| `InsertPreviewMaterial` | 없음, Editor 지정 | 반투명. 없으면 프리뷰를 생략하고 경고 로그 |

연속 간격은 기존 `UPlayerHeldTargetUseComponent::RepeatIntervalSeconds`(0.15)를 공유한다.

## Compatibility

- 추가만 한다: 새 class, enum append(`EPhysicalCarryKind::ItemBox`, `EBathhouseFacilityType::DrinkFridge`), struct field(`FShopProductEntry`, `FShopOrderLine`, `FPlayerInteractionQuery`), virtual, settings, native tag. rename·삭제·class 변경이 없어 Core Redirect가 필요 없다.
- `final` 제거와 facility hook은 reflected layout을 바꾸지 않는다.
- copy-first load gate: `DA_ShopCatalog`(struct field 추가), `WBP_InteractionPrompt`(optional widget), `BP_Shower`(facility base hook 회귀 대표).

## Editor Authoring

정확한 asset 경로(모두 개별 저장):

| asset | 경로 | 내용 |
|---|---|---|
| 품목 정의 | `/Game/Bathhouse/Data/Service/DA_ServiceItem_BananaMilk` | 임시 mesh, `BoxCapacity` 12, 자리 12개, `Display.Fridge`, `SaleValue` 2000 |
| 품목 박스 | `/Game/Bathhouse/Blueprints/Service/BP_ItemBox` | parent `AItemBoxActor`, 위가 열린 박스 mesh(가장 큰 배치가 들어가는 크기), `HeldTransform` |
| 냉장고 | `/Game/Bathhouse/Blueprints/Service/BP_DrinkFridge` | parent `ADrinkFridgeActor`, 공간 4개(extent·자리 6개씩, index 0~3), slot 1개, footprint |
| 수거함 | `/Game/Bathhouse/Blueprints/Service/BP_DrinkCollectionBox` | parent `ADrinkCollectionBoxActor` |
| 배치 정의 | `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_DrinkFridge` | `Facility.Placeable`·`Facility.Discardable`, `PlacedFacilityClass` = BP_DrinkFridge, `RecoveryItemClass` = `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem` |
| 외곽선 | `/Game/Bathhouse/Materials/Service/M_PP_TakeHighlightOutline` | Take Highlight Outline 절의 세 규칙 |
| 프리뷰 | `/Game/Bathhouse/Materials/Service/MI_DisplayInsertPreview` | 반투명 Unlit, Two Sided |

기존 asset 수정(allowlist):

- `/Game/Bathhouse/Data/Shop/DA_ShopCatalog`: 냉장고 30,000, 바나나우유 박스 12,000
- `/Game/FirstPersonCharacter/BP_FirstPersonCharacter`: `FirstPersonCamera` Weighted Blendables에 외곽선 머티리얼
- `/Game/Bathhouse/UI/WBP_InteractionPrompt`: `HeldSummaryText`
- Project Settings(`Config/DefaultGame.ini`, `DefaultEngine.ini`): `ShopSettings.ItemBoxClass`, `ServiceDisplaySettings.InsertPreviewMaterial`, Rendering Custom Depth-Stencil Pass = Enabled with Stencil
- DefaultMap 카운터에 `BP_DrinkCollectionBox` 1개. DefaultMap은 World Partition이므로 이 Actor는 `/Game/__ExternalActors__/Maps/DefaultMap/...` external actor package로만 저장한다. `DefaultMap.umap`은 저장하지 않는다.

Editor 검증 방법:

- `BP_DrinkFridge` 구조: Data Validation(SCS 검사)과 MCP의 SCS template readback(`SpaceIndex`, `SlotTransforms` 수, `AcceptedCategory`, slot 수) 둘 다 확인한다. PIE에서 배치 직후 네 공간 HUD `빈 공간 0/6`을 확인한다.
- 박스 미리보기: `BP_ItemBox` Blueprint 에디터 뷰포트에서 `EditorPreviewKind`·`EditorPreviewCount`로 확인한다.
- 갱신할 Unreal 정본: 신규 `.md/Unreal/ServiceSystem.md`, `0_UNREAL.md` 지도, `ShopSystem.md`(상품 2개), `PlacementSystem.md`(Definition 표), `InteractionUISystem.md`(`HeldSummaryText`, 카메라 blendable).

## Verification

| 시나리오 | 자동화 |
|---|---|
| 전송 규칙 | Apply·Take 이유 우선순위 표, 종류 잠금·풀림, 정원, LIFO |
| DISP-001, SHOP-S01, S02 | catalog 두 상품 종류 Validation, 주문 → 상자 → 개봉 시 12/12 박스, 박스 1개 = 수량 1 |
| DISP-002, 014, 018, 019 | 박스 E·요약·빈 박스, instance 수, drop·복구 내용 보존 |
| DISP-003~011, 020, 022 | held-use 경유 넣기·빼기, 0.15 반복·멈춤, 공간 전환 멈춤, 내려놓기·suppression 멈춤, 비대상·수건바구니 무변화 |
| DISP-012 | 공간에 E·F 무변화 |
| DISP-006, 007, 013 | focus 알림에 따른 프리뷰·proxy 위치·표시, 설정 끔에서 proxy 숨김 |
| FRDG-001, 002, 012 | 공간 독립, 빈 공간 표시, 신규 설치 빔 |
| FRDG-006~008, 013 | 테스트 사용자 예약·가져가기·적립·LIFO·빈 냉장고 예약 불가·예약 중 회수 거부·회수 보류 중 예약 거부·사용자 소멸 시 해제, 이용 중 플레이어 이동 |
| FRDG-009, 010, 012 | construction 중 생성 fixture로 실제 transaction 경로: 신규 설치 빈 공간, 회수 → 재설치 복원, 잘못된 payload 거부 시 item·공간 불변, 아이템 요약·Q 취소 보존 |
| F1 회귀 | 기존 설비 설치·회수(Shower·utility·수건 기계) 결과 동일, finalize 기본값 |
| DISP-021 | 박스 버리기와 진열이 든 냉장고 아이템 버리기: item·payload 소멸, 지갑·수거함 금액 불변 |
| FRDG-004, 005, 011, 014, 015 | 수금·0원·두 냉장고 합산·반복 E 1회·수거함 carry/회수/버리기 불가 |

PIE: 외곽선 모양과 화면 테두리(창 크기·Screen Percentage 변경 포함), 프리뷰 반투명, 박스 안 모습, 0.15초 감각, 콘솔 명령으로 FRDG-006~008·013.

## Dependencies

- Service → Interaction(held-use·focus observer·carry·discardable), Facility(base actor·slot), Placement(payload), Economy(wallet), GameplayTags, DeveloperSettings
- Shop → Service(품목 정의·박스 class)
- UI → Interaction query 필드
- Service는 Customer·Shop·UI concrete에 의존하지 않는다.
