# Service Facility Display System

## Status And Scope

- [ServiceSystem.md](ServiceSystem.md)의 하위 문서다. 서비스 2단위(진열 확장)의 설비 전체 조준 진열, 소모품, 공용 화장대, 샤워 비품, 진열 payload 일반화, 공용 표시 도구를 소유한다.
- 2026-09-30 설계. 구현 완료(커밋 `c9a1150`, 코드 리뷰 재작업 F1~F4와 수건 cue 재작업 포함). 입력은 `.md/PROMPT_ARCHITECTURE.md`(서비스 2단위, DISP-015~017·023·024, VANI-001~017, SHWR-001~011, TOWL-001~018, SHOP-S03·S04)이고 사용자가 설계를 승인했다.
- 수건 쪽 변경(규칙·표현·뚜껑)은 [TowelSystem.md](TowelSystem.md), [TowelPresentationSystem.md](TowelPresentationSystem.md)의 2단위 절이 정본이다. 이 문서는 수건이 쓰는 공용 표시 도구만 정한다.
- 1단위 구조(품목 정의·품목 박스·진열 공간·냉장고·외곽선)는 유지하며, 아래에서 일반화하는 부분만 바뀐다.

## Source Scope

```text
Public/Service/
  ServiceDisplayManagerComponent.h    신규, 설비 진열 공간 모음·payload·소모·사용자 정리
  DisplayFacilityTargetComponent.h    신규, 설비 전체 조준 router
  DisplayCueComponent.h               신규, 넣기 프리뷰·꺼내기 강조 proxy 공용 도구
  ServiceDisplayPlacementData.h       신규, 진열 payload 확장 data
Public/Facility/
  FacilityPlacementExtension.h        신규, 설비 payload 확장 component interface
Public/Interaction/Presentation/
  OpeningPresentationComponent.h      신규, 조준 기반 뚜껑·문 열림 표현
```

삭제: `UDrinkFridgePlacementInstanceData`(Transient native, asset 참조 없음). 냉장고는 공용 manager와 payload data로 옮긴다.

삭제(2026-09-30 리뷰 F4): `ADrinkFridgeActor`의 빈 private `UFUNCTION` 세 개, Transient `BoundSpaces`, 옛 payload 주석. 동적 바인딩 전용 private 함수와 Transient 필드는 asset에 저장되지 않아 호환 근거가 없다. native 구조 변경이므로 `BP_DrinkFridge`·DefaultMap copy-first load gate를 다시 실행한다.

## Item Definition Additions

`UServiceItemDefinition`에 추가한다. 기존 필드와 asset은 그대로다.

| 필드 | 규칙 |
|---|---|
| `ConsumableUses` (int32, 기본 0) | 0이면 비품·일반 품목. > 0이면 소모품의 1개당 사용 횟수. 스킨로션 10, 면봉 30, 샴푸 30, 바디워시 30 |
| `BoxItemOffset` (FTransform, 기본 Identity) | 박스 안 모든 자리에 공통 적용하는 품목 보정. 자리 i = `BoxItemOffset * BoxSlotTransforms[i]`. 품목 자기 기준 회전·스케일·피벗 보정 뒤 자리 transform이다. 회전은 합성, 스케일은 곱. 공통 스케일은 균일값 권장. Validation은 finite·스케일 양수 |

- 새 asset: `DA_ServiceItem_HairDryer`(박스 2), `SkinLotion`(6, 10회), `CottonSwab`(6, 30회), `Comb`(12), `Shampoo`(6, 30회), `BodyWash`(6, 30회). 설비 전용 품목이라 `DisplayCategories`는 비워 둔다. 냉장고 공간은 `Display.Fridge`가 없어 거부한다(DISP-015).
- 박스 안 소모품은 항상 새것이다. 박스는 사용 횟수 상태를 갖지 않는다.

## Display Space Modes

`UDisplaySpaceComponent`에 추가(EditAnywhere, 설비 BP 기본값):

- `TargetMode`(`EDisplaySpaceTargetMode`):
  - `SelfAim`(기본, 1단위 냉장고): 자기 Box가 조준 대상, 종류는 처음 넣은 품목으로 잠김.
  - `FacilityRouted`(화장대·샤워기): 자기 collision은 NoCollision이고 query를 받지 않는다. 설비 router가 선택해 조작한다.
- `FixedKind`(`UServiceItemDefinition`): `FacilityRouted`에서 필수. 이 묶음은 이 품목만 받고 빈 상태에서도 품목이 바뀌지 않는다. `CanAcceptKind`는 `Kind == FixedKind`다.
- 상태 추가: `Stock.InUseRemaining`(int32, 0 = 사용 중 없음). 소모품 묶음에서만 쓴다. 사용 중인 것은 항상 index 0 자리의 물품이다. Count는 사용 중인 것을 포함한다.
- 외형: 기존과 같이 index 0..Count-1 자리에 그린다. 사용 중인 것이 다 소모돼 Count가 줄면 남은 새것이 앞자리로 당겨 보인다(VANI-006).

## Consumable Rules

`FServiceItemTransfer`와 공간 API 확장:

- 꺼낼 수 있는 수 = Count − (InUseRemaining > 0 ? 1 : 0). Take는 이 수가 0이면 거부한다: Count 0이면 `꺼낼 물건 없음`, 아니면 `사용 중인 것은 꺼낼 수 없음`(VANI-005). 이 이유는 기존 Take 우선순위에서 `꺼낼 물건 없음` 자리에 둔다.
- Take는 항상 마지막 자리(index Count−1)의 새것을 뺀다. 사용 중인 것은 index 0이라 마지막 한 개가 사용 중이면 위 규칙으로 거부된다(VANI-004).
- Apply는 사용 중인 것이 있어도 다음 빈자리에 넣는다.
- `UDisplaySpaceComponent::ConsumeOneUse(OutbDepleted)`(신규):
  1. FixedKind가 소모품이고 Count > 0일 때만 동작한다.
  2. InUseRemaining == 0이면 `Kind->ConsumableUses`로 시작한다(VANI-007).
  3. 1을 뺀다. 0이 되면 index 0 물품을 제거해 Count−1, InUseRemaining 0으로 둔다(VANI-006, SHWR-011).
  4. Revision을 올리고 commit 뒤 방송한다. Count 0이면 아무것도 하지 않는다(VANI-008, SHWR-005).
- 비품(`ConsumableUses == 0`)은 줄지 않는다(VANI-014).

## Facility Target Router

`UDisplayFacilityTargetComponent : UBoxComponent`, 구현 `IPlayerInteractable`, `IPlayerInteractionFocusObserver`. 화장대·샤워기 BP에 하나를 두고, Box extent로 설비 외형 전체를 덮는다(QueryOnly, Visibility Block, Navigation off). 설비 어느 부분을 조준해도 이 Box가 맞는다.

- 묶음 선택(query와 execute가 같은 함수):
  - 비지 않은 품목 박스를 들었으면 `FixedKind == 박스 종류`인 묶음이다. 없으면 두 방향 모두 `여기에 넣을 수 없는 물건`이다(DISP-015).
  - 빈 박스를 들었으면 조준선(`HitResult.TraceStart → TraceEnd`)에 가장 가까운 묶음이다. 기준점은 묶음 자리들의 world 중심이다. 같은 거리면 SpaceIndex가 작은 쪽이다(Q61 A, DISP-023). 빈 박스라 Apply는 `박스가 비어 있음`이다.
  - 품목 박스가 아니면 두 방향 모두 비운다.
- 선택된 묶음으로 기존 `EvaluateApply/Take`, `TryApplyOne/TryTakeOne`을 호출한다. 두 방향 모두 `Repeat`다. 선택 묶음이 비었거나 사용 중인 것만 남았으면 다른 묶음에서 대신 빼지 않는다(VANI-012).
- `HeldUseTargetKey` = 선택 묶음의 SpaceIndex다(아래 held-use 확장). focus cue 경로와 반복 guard에 쓴다.
- 빈 박스 연속 빼기(DISP-024, 2026-09-30 정정): 첫 1개가 빠지면 박스가 비지 않으므로 다음 query부터 박스 종류 묶음이 고정 선택된다. 조준이 다른 묶음 쪽으로 옮겨 가도 선택과 key는 그대로이며 같은 묶음에서 계속 뺀다. 별도 잠금 상태는 두지 않는다(선택 함수만으로 성립). 반복은 조준이 router 박스를 벗어나 target object가 바뀌거나, 묶음이 비거나(`꺼낼 물건 없음`/사용 중만 남음), 박스가 차면 기존 held-use 규칙대로 멈춘다.
- TargetName: router의 `FacilityDisplayName`(FText, EditAnywhere, 비면 Validation 오류. 화장대 `화장대`, 샤워기 `샤워기`) 다음 줄부터 묶음마다 한 줄(SpaceIndex 순). router는 설비 종류를 분기하지 않는다(2026-09-30 리뷰 F4):
  - 비품 또는 사용 중 없음: `{품목} {Count}/{정원}`
  - 사용 중 있음: `{품목} 새것 {Count−1} + 사용 중 {남은}/{횟수}회`
  - 박스 없이도 보인다(VANI-013, SHWR-001).
- E: 행동 없음. 기존 설비 E(없음)와 같다.
- focus observer: 알림 query의 `HeldUseTargetKey` 묶음에 기존 공간 프리뷰·강조 규칙을 적용하고, 다른 묶음은 숨긴다. 강조는 꺼낼 수 있는 새것에만 뜬다.

## Held-Use Extension

[HeldTargetUseSystem.md](HeldTargetUseSystem.md)의 추가 규칙:

- `FPlayerInteractionQuery::HeldUseTargetKey`(int32, 기본 `INDEX_NONE`, `Equals` 포함).
- `UPlayerHeldTargetUseComponent`는 `BeginUse` 때 target query의 key를 저장한다. 반복 Tick에서 같은 target object여도 key가 다르면 조용히 멈춘다. 멈춘 뒤 재개 규칙은 기존과 같다.
- 이 guard는 방어 규칙이다. DISP-024 연속 빼기에서는 key가 바뀌지 않아 발동하지 않는다. 발동 예: 넣기 반복 중 박스가 비어 선택이 조준선 최근접 묶음으로 바뀌는 경우(이때도 Apply는 `박스가 비어 있음`으로 멈춘다).
- key가 `INDEX_NONE`인 기존 대상(냉장고 공간·수건·삽)은 동작이 같다.

## Display Manager And Payload

`UServiceDisplayManagerComponent : UActorComponent`, 구현 `IFacilityPlacementExtension`. 진열이 있는 설비마다 하나다. 냉장고는 native default subobject, 화장대·샤워기는 BP component다.

- 공간 수집: owner의 `UDisplaySpaceComponent`를 SpaceIndex 순으로 모은다.
- 규칙:
  - index 고유·연속, 자리 ≥ 1
  - `SelfAim`은 분류 태그 필수
  - `FacilityRouted`는 `FixedKind` 필수, 같은 FixedKind 중복 금지
  - `FacilityRouted` 묶음이 있으면 router 정확히 1개
  - `RequiredCustomerSlotCount`(EditDefaultsOnly, 냉장고 1, 화장대 1, 샤워기 2)와 slot 수 일치
- 사용자 정리: owner slot의 Reserved/Occupied 전이마다 사용자 `OnEndPlay`를 구독한다. 아직 그 사용자면 `ForceRelease`한다(1단위 냉장고 로직 이동).
- 이용 시작 소모: `bConsumeOnCustomerUseStart`(EditDefaultsOnly, 화장대·샤워기 true, 냉장고 false). slot 예약 주기마다 한 번, 그 주기의 첫 Occupied 진입에서 소모품 묶음마다 `ConsumeOneUse` 한 번이다(2026-09-30 리뷰 F1).
  - 예약 주기: slot이 Available에서 벗어난 때부터 다시 Available이 될 때까지다. knockdown은 slot을 Occupied → Reserved로 낮췄다가 같은 사용자로 다시 Occupied로 올린다([FacilitySystem.md](FacilitySystem.md) knockdown 규칙). 이 재진입은 같은 주기이므로 소모하지 않는다.
  - manager가 런타임 전용 `TSet<TWeakObjectPtr<UBathhouseFacilitySlotComponent>> ConsumedReservationSlots`를 소유한다. Occupied 진입 시 slot이 집합에 없으면 소모하고 넣는다. 있으면 아무것도 하지 않는다. slot이 Available이 되면(`Release`·`ForceRelease` 모두) 집합에서 뺀다. slot unbind(EndPlay·회수)에서 비운다. 저장·payload 대상이 아니다(회수는 slot 사용 중 거부이므로 주기가 걸치지 않는다).
  - Customer 코드·StateTree는 바꾸지 않는다. slot 전이만으로 판정한다.
  - 샤워기는 목욕 전·후 샤워가 각각 별도 예약 주기(Reserve → Release)이므로 각 1회다. 두 자리가 같은 비품을 쓰므로 두 slot에서 각각 시작하면 합계 2회다(J6, J7, SHWR-004, 009, 010).
  - 비품이 없어도 이용은 진행된다.
- `CustomerUseSeconds`(EditDefaultsOnly, 화장대 20, 그 밖 0 = 루틴 기본): 이후 손님 루틴이 읽는 설비 값이다. 이 단위는 값만 제공한다.
- payload(`IFacilityPlacementExtension`):
  - `ExportPlacementExtension(Outer)`: `UServiceDisplayPlacementData`(`TArray<FDisplaySpaceSnapshot{SpaceIndex, Kind, Count, InUseRemaining}>`)를 반환한다. `FDisplaySpaceSnapshot`에 `InUseRemaining`을 추가한다.
  - `ValidatePlacementExtension(DataOrNull)`: 공간 구조 규칙과, non-null이면 index 일치·Kind 허용(SelfAim 분류 / Routed FixedKind)·Count ≤ 자리 수·InUseRemaining 0~`ConsumableUses`(소모품이고 Count > 0일 때만 > 0)를 확인한다.
  - `ApplyPlacementExtension(DataOrNull)`: 검증 뒤에만 호출한다. null이면 모든 공간이 빈 상태다(VANI-015, SHWR-003).
  - `GetContentsSummary`: 품목별 합계 문구다(예: `스킨로션 7개, 빗 3개`, 냉장고 `바나나우유 14병`). 사용 중인 것도 개수에 포함한다.

## Facility Extension Contract

[FacilitySystem.md](FacilitySystem.md)·[PlacementSystem.md](PlacementSystem.md)에 반영한다.

- `IFacilityPlacementExtension`(Facility, C++ 전용 UINTERFACE)을 component가 구현한다. key(FName), export, validate, apply, 템플릿 검증 `ValidateExtensionAuthoring(OwnerComponents, OutFailure)`를 제공한다.
- `UFacilityPlacementExtensionData : UObject`(Abstract, Transient): `Key`, `virtual GetContentsSummary()`.
- `UBathhouseFacilityPlacementInstanceData`에 `UPROPERTY(Transient) TArray<TObjectPtr<UFacilityPlacementExtensionData>> Extensions`를 추가한다. `GetPlacementContentsSummary()`는 extension 요약을 잇는다.
- `ABathhouseFacilityActor` 기본 hook이 extension component를 처리한다(subclass override 불필요).
  - Export: 모든 extension component의 data를 모은다.
  - `ImportFacilityExtension`(construction 뒤 finalize 단계): component 집합과 data key 집합을 맞춘다. 모두 validate한 뒤에만 모두 apply한다. 하나라도 실패하면 아무것도 적용하지 않는다. fresh는 모든 component에 null이다.
  - `IsDataValid`: 인스턴스는 `GetComponents`, CDO는 native subobject + BP SCS 템플릿을 모아 각 extension의 `ValidateExtensionAuthoring`에 넘긴다. 1단위 냉장고의 SCS 검사를 일반화한 것이다.
- 회수 보류 일반화: `ABathhouseFacilityActor`가 `bRecoveryHoldActive`를 `TryBeginFacilityRecoveryHold`(성공 시)와 `CancelFacilityRecoveryHold`에서 관리한다. `IsAvailableForReservation()` 기본이 보류 중 false를 반환한다. 모든 설비에서 회수 Q Hold 중에는 새 예약을 받지 않는다(VANI-017). 욕탕은 `IsAvailableForReservation()`에서 Super를 호출하고, `TryBegin`/`Cancel`은 자체 동결·commit-pending 경로로 같은 base flag를 관리한다(Super 미호출, 기존 동작 유지). 냉장고의 개별 보류 코드는 제거한다. 보류 시작·취소의 가용성 알림 규칙은 [FacilitySystem.md](FacilitySystem.md) `ABathhouseFacilityActor` 회수 보류 알림이 정본이다(2026-09-30 리뷰 F2).
- `EBathhouseFacilityType::Vanity`를 enum 끝에 append한다.

## Vanity And Shower

- 공용 화장대 `BP_Vanity`: parent `ABathhouseFacilityActor`, `FacilityType = Vanity`. component 구성:
  - manager(`bConsumeOnCustomerUseStart`, `CustomerUseSeconds` 20, slot 1)
  - router Box
  - `FacilityRouted` 묶음 4개: 드라이기 2, 스킨로션 4, 면봉 4, 빗 6 자리. 서로 떨어진 위치
  - slot 1, footprint, 거울·몸체 외형(임시 mesh는 Editor Authoring의 placeholder 규칙)
  - Placement Definition과 상점 상품(20,000)
- 샤워기 `BP_Shower`: 기존 parent·slot 2개·배치 설정은 그대로다. BP component로 manager(소모, slot 2), router Box, 샴푸·바디워시 묶음(각 2자리)을 추가한다. 레벨의 기존 샤워기 instance도 BP 변경을 상속하며 빈 상태로 시작한다(SHWR-001).
- 손님 없이 검증: 비 shipping 콘솔 명령 `bathhouse.Debug.Facility.BeginUse`(조준한 설비의 첫 Available slot을 플레이어 Pawn으로 reserve·BeginUse), `.EndUse`(EndUse·Release). 화장대 VANI-006~009·014·017용이다. 샤워 비품은 실제 손님으로 확인한다(SHWR-004·005·009~011).

## Display Cue Component

`UDisplayCueComponent : USceneComponent`(Service). 1단위 공간의 프리뷰·강조 proxy를 공용 도구로 뺀다.

- 런타임 Transient 자식 두 개: `InsertPreview`(반투명, NoCollision)와 `TakeHighlightProxy`(main·depth pass 제외, custom depth + stencil)다.
- API:
  - `ShowInsertPreview(Mesh, RelativeTransform)`
  - `ShowTakeHighlight(Mesh, RelativeTransform)`: 설정 `bShowTakeHighlight`가 꺼져 있으면 숨김
  - `HideInsertPreview()`, `HideTakeHighlight()`, `HideAll()`
- 설정은 기존 `UServiceDisplaySettings`(프리뷰 머티리얼, 강조 켜기, stencil 값)를 쓴다. 외곽선 머티리얼·카메라 blendable은 1단위 그대로다.
- 사용처: `UDisplaySpaceComponent`(기존 proxy를 이 도구로 교체, 결과 동일), 수건 선반·사용 수건통·세탁기/건조기(TowelSystem 2단위 절).
- 의존: Towel → Service(표현 도구와 설정만). Service는 Towel에 의존하지 않는다.

## Opening Presentation

`UOpeningPresentationComponent : UActorComponent`(Interaction/Presentation, 신규).

- 알고리즘·authoring은 `UUtilityFuelDoorComponent`와 같다: source별 열림 요청, alpha 선형, `LocalRotationAxis`, `OpenAngleDegrees`, `OpenSeconds`, `CloseSeconds`, `Configure(Pivot)`, Editor preview.
- 기존 보일러 문 class의 부모는 바꾸지 않는다(serialized 호환, CoreSystem 규칙). 두 class가 공존하며, 보일러 문 이전은 이 단위 범위가 아니다.
- 사용처: 세탁기·건조기 뚜껑(TowelSystem 2단위 절).

## Editor Authoring

| asset | 경로 | 내용 |
|---|---|---|
| 품목 정의 6종 | `/Game/Bathhouse/Data/Service/DA_ServiceItem_{HairDryer,SkinLotion,CottonSwab,Comb,Shampoo,BodyWash}` | 정원·횟수·박스 자리·`BoxItemOffset`·진열 외형 |
| 화장대 | `/Game/Bathhouse/Blueprints/Service/BP_Vanity` | 위 구성 |
| 화장대 배치 정의 | `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Vanity` | Placeable·Discardable, `RecoveryItemClass` 공통 설비 아이템 |
| 샤워기 | `/Game/Bathhouse/Blueprints/Facility/BP_Shower` | manager·router·묶음 2개 추가 |
| 세탁기·건조기 | `/Game/Bathhouse/Blueprints/Towel/BP_Washer`, `/Game/Bathhouse/Blueprints/Towel/BP_Dryer` | 뚜껑 pivot·mesh·열림 값, Pile 범위 확인 |
| 상점 | `/Game/Bathhouse/Data/Shop/DA_ShopCatalog` | 화장대 20,000, 박스 6종 가격 |

박스 크기(`BP_ItemBox`)는 바꾸지 않는다. 들어가지 않는 품목이 있으면 멈추고 기능 명세로 보고한다.

Placeholder mesh 규칙(2026-09-30 리뷰 F3): 이 단위는 새 Mesh·Material asset을 만들지 않는다. 화장대 거울·몸체, 세탁기·건조기 `LidMesh`, 품목 진열 외형의 임시 mesh는 Engine 기본 도형만 쓴다: `/Engine/BasicShapes/Cube.Cube`, `/Engine/BasicShapes/Cylinder.Cylinder`, `/Engine/BasicShapes/Plane.Plane`. Material은 mesh 기본값 또는 기존 프로젝트 Material만 쓴다. 기존 `MachineVisual`은 유지하며, 뚜껑 열림과 Pile 프리뷰가 가려지면 `MachineVisual`의 transform만 조정한다(asset 교체 없음). exact 값은 구현의 `PROMPT_UNREAL.md`가 정한다.

## Verification

| 시나리오 | 자동화 |
|---|---|
| DISP-015~017, 023, 024 | router 묶음 선택: 박스 종류·빈 박스 조준선 최근접·동거리 규칙, 거부 이유. DISP-024: 빈 박스 첫 빼기 뒤 박스 종류 묶음 고정, 조준을 다른 묶음 쪽으로 옮겨도 같은 묶음에서 계속, router 이탈·묶음 빔·박스 가득에서 멈춤, 다른 묶음 품목은 빠지지 않음. key guard 단위 검증 |
| VANI-001~005, 011~013, 015 | 묶음별 넣기·빼기, 정원, 사용 중 거부, 최근접 빼기, 다른 묶음 대체 금지, HUD 문구, 신규 빈 상태 |
| VANI-006~009, 014, 017 | construction fixture와 slot Occupied 전이: 소모·당김·사용 중 시작, 빈 화장대 이용, 이용 중 조작 가능·회수 거부, 비품 불변, 보류 중 예약 거부 |
| VANI-010, 016, SHWR-007 | payload 왕복(InUseRemaining 포함)·잘못된 값 거부·요약·버리기 |
| SHWR-001~003, 006, 008 | 레벨 instance 빈 상태, 샤워기 넣기·빼기, 회수 거부 |
| SHWR-004, 005, 009~011 | 예약 주기마다 1회: Reserve → BeginUse → EndUse(Reserved) → BeginUse는 1회, Release 뒤 새 주기는 1회 추가, 두 slot 각각 시작은 합계 2회, `ForceRelease` 뒤 새 주기 1회. 비품 없음, 소진 당김(PIE는 실제 손님과 knockdown 재개) |
| 냉장고 회귀 | 1단위 Service 테스트 전체(manager 이전 뒤 동일 결과) |
| 회수 보류 일반화 | 다른 설비(샤워기·욕탕·락커)의 Q Hold 중 예약 거부와 취소 복구, 기존 회수 결과 동일. 취소 시 가용성 알림 1회·보류 시작 알림 없음, 회수 성공 경로 알림 수 기존과 같음(1회), 보류 중 예약 실패 손님이 취소 알림으로 재시도해 예약 성공 |
| BoxItemOffset | 박스 instance transform = offset * slot, Identity 기본 동일 |
| SHOP-S03, S04 | catalog 7상품 Validation, 화장대·스킨로션 박스 개봉 |

## Dependencies

- Service → Facility(extension interface·base actor), Interaction(held-use·focus·presentation), Placement
- Towel → Service(표시 도구), Interaction(opening presentation)
- Facility는 Service·Towel concrete에 의존하지 않는다.
