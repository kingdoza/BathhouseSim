# 구현 프롬프트 — 서비스 1단위 수직: 품목 박스·진열·음료 냉장고·공용 수거함

## R. 아키텍처 재검토 반영 재작업 (2026-09-30)

이 절이 이번 구현 입력이다. 아래 `단계와 입력`부터는 이미 구현된 본문이며 계속 유효하다. 코드 리뷰 재작업 프롬프트 `.md/PROMPT_IMPLEMENTATION_R.md`(F1~F5)도 함께 읽는다. F1은 아래 설계로 확정했고, F2~F5 세부는 R 문서를 따른다. 둘이 다르면 이 절과 정본이 우선이며, 차이를 보고한다.

### 시작 조건

- 현재 작업 트리의 Service 구현(미커밋)을 유지한 채 수정한다. `Content/`, `Config/`는 변경이 없어야 한다(`git --no-optional-locks status --short -- Content Config`).
- 설계 정본: `.md/Architecture/ServiceSystem.md` Status, Item Box, Display Space, Drink Fridge, Placement Payload Extension, Shop Integration, Editor Authoring, Verification. `.md/Architecture/PlacementSystem.md`의 "placement staged 순서" 문단.

### F1 — construction 뒤 payload 적용(필수)

- `IPlaceableFacility`에 `virtual bool FinalizePlacementPayloadAfterConstruction(FText& OutFailureReason) { return true; }`를 추가한다.
- `FFacilityActorConversionTransaction`(placement 경로):
  - `FinishSpawning` 직후, `IsValid` 확인 뒤, `FinalizeStagedPlacementCollisionSnapshot` 앞에서 호출한다.
  - 실패하면 기존 import 실패와 같이 `DestroyStaged()` 후 `nullptr`을 반환한다. item은 소모하지 않고 publication도 없다.
  - 기존 테스트 fault 주입 형식을 따라 `ETestFault::PlacementFinalizePayload`를 enum 끝에 추가한다(테스트 전용).
- `ABathhouseFacilityActor`:
  - `ImportPlacementPayload`(fresh·non-fresh 모두)에서 `ImportFacilityExtension` 호출을 제거한다. 확장 data 포인터를 `UPROPERTY(Transient) PendingFacilityExtension`에 보관하고 `bPendingFacilityExtension=true`로 둔다. 신규 설치는 null이다.
  - `FinalizePlacementPayloadAfterConstruction` override: pending이 있으면 `ImportFacilityExtension(Pending)`을 호출하고 결과와 관계없이 pending을 비운다. pending이 없으면 true다.
  - base 필드 import 동작과 실패 문구는 그대로 둔다.
- `ADrinkFridgeActor::ImportFacilityExtension`은 지금 로직(`CollectSpaces` → 전부 검증 → 전부 적용)을 유지한다. 이제 construction 뒤에 불리므로 BP 공간이 보인다.
- 기존 테스트 중 `ImportPlacementPayload`만 직접 호출하고 확장 적용을 기대하는 곳(`ServiceFridgeAutomationTests.cpp` 등)은 `FinishSpawning` → `FinalizePlacementPayloadAfterConstruction`까지 호출하도록 고친다. 다른 설비 테스트(BathWater·Utility·Shop)는 결과가 같아야 한다.
- fixture: 공간·slot을 생성자 subobject로 만들지 않는 `AServiceAutomationConstructedFridge`를 추가한다. `OnConstruction`에서 NewObject + `RegisterComponent`로 만든다(SCS와 같은 시점). 이 fixture로 실제 transaction 경로를 검증한다.
  - 신규 설치 → 네 공간 빔
  - 회수 → 재설치 복원
  - 잘못된 payload(index 불일치·정원 초과·분류 불허) → placement 실패, 원래 item과 payload 유지, 새 Actor 없음
  - finalize fault 주입 → 동일한 실패 결과
  - 기존 native subobject fixture 테스트는 유지해도 된다.

### F2 — Editor 인계와 박스 미리보기

- `AItemBoxActor`: 미리보기 적용 조건을 `EWorldType::Editor || EWorldType::EditorPreview`로 넓힌다. game world 동작은 그대로다.
- `ADrinkFridgeActor::IsDataValid`: CDO(`IsTemplate()`)일 때 native subobject와 `UBlueprintGeneratedClass` 상속 사슬의 `SimpleConstructionScript` 노드 `ComponentTemplate`을 모아 `CollectSpaces`와 같은 규칙(공간 index 고유·연속, 자리 ≥ 1, 분류 태그, slot 정확히 1)을 검사한다. 규칙 판정 함수는 인스턴스 경로와 공유한다. WITH_EDITOR 전용이다.
  - 자동화: 테스트용 `UBlueprint`를 만들 수 있으면 SCS 노드 2~3개 경우(정상·index 중복·slot 없음)를 검사한다. 불가하면 판정 함수를 component 목록 입력으로 직접 검사하고, BP 검사는 Editor 단계 확인으로 넘긴다.
- `PROMPT_UNREAL.md`는 R 문서 F2 1~6과 정본 Editor Authoring 표(정확한 경로, allowlist, World Partition external actor 규칙, Editor 검증 방법, 갱신할 Unreal 정본)를 그대로 반영해 다시 쓴다. 대표 시나리오 PIE 절차와 DISP-003~011·020·022 관찰 방법(키, 조준 대상, 기대 HUD 문구, 병 위치)을 포함한다.

### F3 — 정본 정렬

- 코드 이름·순서는 이미 정본(재작업본)에 반영했다. 코드를 정본에 맞춰 바꿀 것은 없다. `DisplayOffset * SlotTransforms[i]`를 유지한다.

### F4 — 자동화 추가

- DISP-021: 진열이 든 냉장고 아이템(payload 보유)을 쓰레기통 경로로 버린다. item·payload 소멸, 지갑·수거함 금액 불변.
- 박스 버리기 테스트에 지갑·수거함 금액 불변 단언을 추가한다.

### F5 — 정리

- `FShopUnboxItemShape`와 factory 두 개를 `Private/Shop/ShopUnboxItemShape.h/.cpp`로 옮긴다.
- 설비 전용 `FindSpawnTransforms(Definitions)` overload를 제거하고, 그 테스트 12곳을 shape 경로로 옮긴다. 의미는 유지한다. `ShopUnboxingPlacement.cpp`는 400줄 미만이어야 한다.
- `UServiceDisplaySettings::LoadInsertPreviewMaterial` 정의를 `Private/Service/ServiceDisplaySettings.cpp`로 옮긴다.

### 검증과 결과물

- 빌드: UE 5.8 `Build.bat`.
- copy-first load gate: hook 호출 시점이 바뀌었으므로 `BP_Shower`만 기존 `BathhouseSim.Service.BlueprintLoad` 순서로 다시 실행한다.
- 집중: `BathhouseSim.Service`, `BathhouseSim.Shop`, `BathhouseSim.FacilityPlacement`(또는 Placement 테스트 필터), BathWater·Utility 회수 테스트.
- 전체 회귀: `Automation RunTests BathhouseSim`. 수치와 실패 이름을 보고하고, 실패는 스스로 무관 판정하지 않는다.
- `.md/PROMPT_REVIEW.md`: F1~F5 대응, 새 fixture 방식, 수치. `.md/PROMPT_UNREAL.md`: F2 반영 재작성.
- 금지는 아래 본문 `금지`를 그대로 따른다. `.md/Architecture/*` 수정 금지.

## 단계와 입력

- 2026-09-30 사용자가 기능 계약 `.md/PROMPT_ARCHITECTURE.md`(DISP-001~022, FRDG-001~015, SHOP-S01·S02)의 설계 진행을 지시했다. 외곽선 방식은 사용자가 A(Custom Depth Stencil + 후처리)로 결정했다.
- 현재 단계: **수직 구현**. 음료는 바나나우유 1종이다. 화장대·샤워 비품·수건 변경은 만들지 않는다.
- `.md/AGENT_WORKFLOW.md` → `.md/AGENT_IMPLEMENTATION.md`를 읽고 C++ 구현 단계만 수행한다.
- 설계 정본(전체 읽기): `.md/Architecture/ServiceSystem.md`.
- 관련 절:
  - `HeldTargetUseSystem.md` 전체(넣기·빼기가 이 계약을 쓴다)
  - `ShopSystem.md` Catalog And Settings·Unboxing
  - `FacilitySystem.md` `ABathhouseFacilityActor`·Facility Types
  - `PlacementSystem.md` 회수 아이템 요약 문단
  - `PhysicalCarrySystem.md` Consume And Discard Extension
  - `InteractionSystem.md`·`UISystem.md` HUD 항목
  - `EconomySystem.md`
- 이 문서와 정본이 다르면 구현을 멈추고 보고한다.
- headless 실행은 `.md/AGENT_WORKFLOW.md` UE 5.8 Headless Automation Policy 형식을 따른다. git은 `--no-optional-locks`로 실행한다.
- 시작 조건: `git --no-optional-locks status --short -- Source Content Config`가 비어 있어야 한다. `.md/` 변경(설계 산출물)은 정상이다.

## 현재 → 목표

| 영역 | 현재 | 목표 |
|---|---|---|
| 품목 | 없음 | `UServiceItemDefinition`, `FServiceItemStack`, `FServiceItemTransfer` |
| 휴대물 | 설비 아이템·배송 상자 등 | + `AItemBoxActor`(`ItemBox` kind, FreeDrop, 버리기 가능) |
| 진열 | 수건 선반만 | + `UDisplaySpaceComponent`(held-use 대상, 프리뷰·강조 proxy) |
| 설비 | 7종 | + `ADrinkFridgeActor`(`DrinkFridge` type, 공간 4, slot 1, 회수 보존) |
| payload | facility data `final` | 파생 hook 3개, `UDrinkFridgePlacementInstanceData`, 요약 virtual |
| 돈 | 계산대 현금 | + `UDrinkSalesSubsystem`, `ADrinkCollectionBoxActor` |
| 상점 | 설비 상품만 | 설비 또는 품목 박스 상품, 개봉 시 박스 생성 |
| HUD | held-use 행 | + 들고 있는 물건 요약 |

## 0. 사전 조건과 백업

- UnrealEditor가 모두 종료됐는지 확인한다.
- `Saved/MigrationBackup/20260930_service_unit1/`에 파일 복사한다:
  - `Content/Bathhouse/Data/Shop/DA_ShopCatalog.uasset`
  - `Content/Bathhouse/UI/WBP_InteractionPrompt.uasset`
  - `BP_Shower`의 실제 uasset(경로는 `DA_FacilityPlacement_Shower`의 `PlacedFacilityClass`로 확인)
- Content·Config·Level을 저장하지 않는다(아래 11의 복사본 확인만 예외).

## 1. Service 기반 타입

- 새 폴더 `Public/Service`, `Private/Service`.
- `ServiceItemTypes.h`:
  - `FServiceItemStack`
  - `FDisplaySpaceSnapshot { SpaceIndex, Kind, Count }`
  - native tag `TAG_Display_Fridge`("Display.Fridge")를 `UE_DEFINE_GAMEPLAY_TAG`로 선언한다. Config는 수정하지 않는다.
- `UServiceItemDefinition`: 정본 Item Definition 표의 필드와 Data Validation.
- `UServiceDisplaySettings`: 정본 Settings 표. getter는 범위를 clamp한다.

## 2. 전송 규칙 `FServiceItemTransfer`

- `Private/Service/ServiceItemTransfer.h/.cpp`, 순수 static.
- `EvaluateApply`, `EvaluateTake`, `TryMoveOne`, `TryRemoveOne`(손님용)을 둔다.
- 이유 문구와 우선순위는 정본 Stack And Transfer를 그대로 따른다.
- 실패 시 두 stack을 바꾸지 않는다. 성공 시 두 Revision을 올린다. 변경 방송은 호출한 owner가 commit 뒤에 한다.

## 3. `AItemBoxActor`

- 정본 Item Box 절을 구현한다. 배송 상자(`AShopDeliveryBoxActor`)의 carry·lifecycle·scale 정책 코드를 구조 참고로 쓴다. 단, 장비(`IHeldEquipmentUsable`)는 구현하지 않는다.
- `EPhysicalCarryKind::ItemBox`를 enum 끝에 append한다.
- `InitializeContents(Kind, Count)`: spawn 중 한 번만 허용한다. Count는 0 ~ `BoxCapacity`다.
- `static bool BuildClassCollisionQuery(TSubclassOf<AItemBoxActor>, const FTransform&, FVector& OutLocation, FQuat& OutRotation, FCollisionShape&, const UPrimitiveComponent*& OutTemplate, FText&)`: 상점 무리 배치용이다. CDO `BoxMesh` bounds × CDO root relative scale을 쓴다.
- Editor 미리보기 필드는 WITH_EDITORONLY_DATA이며, game world 결과에 영향이 없어야 한다.

## 4. `UDisplaySpaceComponent`

- 정본 Display Space 절을 구현한다.
- query와 execute는 held-use 계약 필드를 쓴다. 그 밖의 필드는 기존 수건·삽 대상의 형식을 따른다.
- 런타임 표현 component 세 개는 `OnRegister`에서 Transient로 만든다. 저장·Blueprint 노출을 하지 않는다.
- focus observer는 표현만 바꾼다. domain 변경 금지.
- 설정 `InsertPreviewMaterial`이 없으면 프리뷰를 생략하고 경고 로그를 한 번 남긴다. 강조는 설정값을 매 알림마다 읽는다.

## 5. Facility 확장과 `ADrinkFridgeActor`

- `EBathhouseFacilityType::DrinkFridge`를 enum 끝에 append한다.
- `UBathhouseFacilityPlacementInstanceData`의 `final`을 제거한다. `ABathhouseFacilityActor`에 hook 3개를 추가하고 기존 export/import에서 호출한다. 기본 동작은 지금과 같아야 한다(기존 설비 회귀).
- `ADrinkFridgeActor`: 정본 Drink Fridge 절.
  - 공간 수집·Validation
  - `IsAvailableForReservation`
  - 회수 보류 override
  - 사용자 EndPlay 구독 해제
  - `TryTakeDrinkForCustomer`
  - payload hook
- `UDrinkFridgePlacementInstanceData`: 검증 먼저, 적용은 전부 또는 전무.
- `UFacilityPlacementInstanceData::GetPlacementContentsSummary()`를 추가한다. `APlaceableFacilityItemActor`의 표시 이름에 요약을 붙이고 `GetHeldSummaryText`를 구현한다.

## 6. HUD 요약

- `IPhysicalCarryable::GetHeldSummaryText()`(기본 빈 값)를 추가한다.
- `FPlayerInteractionQuery::HeldObjectSummary`를 추가하고 `Equals`에 포함한다.
- `UPlayerEquipmentUseComponent::MergeEquipmentQuery`에서 들고 있는 carryable의 요약을 채운다. 장비를 든 경우도 요약만 채우며 기존 held-use 필드 비우기 규칙은 유지한다.
- `UInteractionPromptWidget`: `BindWidgetOptional` `HeldSummaryText`. 없으면 생략한다.
- `UPlayerInteractionComponent`에는 아무것도 추가하지 않는다(CoreSystem 성장 정책).

## 7. 판매 적립과 수거함

- `UDrinkSalesSubsystem`, `ADrinkCollectionBoxActor`: 정본 Drink Sales And Collection 절.
- wallet 해석은 `ABathhouseCashPaymentActor::ResolveWallet`과 같은 경로를 쓴다. 공용 helper로 뽑지 말고 같은 방식으로 구현한다(Economy 변경 최소화).

## 8. 상점 통합

- 정본 Shop Integration 절.
- `FShopProductEntry`와 `FShopOrderLine`에 `ItemBoxDefinition`을 추가한다.
- catalog Validation을 "정확히 하나"로 바꾼다.
- `UShopSettings::ItemBoxClass`를 추가한다.
- 주문 평가·배송 상자 내용 검증에 박스 줄을 허용한다.
- `FShopUnboxingPlacement::FindSpawnTransforms`의 입력을 Definition 목록에서 항목별 shape 목록으로 바꾼다. 설비·박스 공용이다. 무리 계산·월드 검사 규칙은 바꾸지 않는다.
- `FShopUnboxingTransaction`: 항목 종류별로 생성한다. rollback 흐름은 그대로다.

## 9. 비 shipping 콘솔 명령

- `Private/Service/DrinkFridgeDebugCommands.cpp`, `#if !UE_BUILD_SHIPPING`. 정본 Customer Contract Verification의 명령 3개를 만든다.
- 조준 대상은 로컬 플레이어의 Interaction focus hit owner가 `ADrinkFridgeActor`일 때만 처리한다. 결과를 로그로 남긴다.

## 10. 빌드

- `.md/AGENT_WORKFLOW.md`의 UE 5.8 `Build.bat`만 사용한다.

## 11. 로드 검증 — 복사본 먼저

- 기존 `BathhouseSim.Shop.BlueprintLoad` 형식을 따라 `BathhouseSim.Service.BlueprintLoad`를 추가한다.
  - 대상: `DA_ShopCatalog`, `WBP_InteractionPrompt`, `BP_Shower`.
  - 확인:
    - catalog 기존 상품 7종 값 유지
    - WBP 기존 필수 BindWidget 유지
    - `BP_Shower` native parent와 facility slot 유지
  - 저장하지 않는다.
- 순서(Fatal·`Serial size mismatch`·`Failed to load`면 즉시 멈춤):
  1. 복사본을 `Content/Developers/MigrationCheck/`에 두고 Template 맵으로 로드한다.
  2. 복사본을 삭제하고 Content 무변경을 확인한다.
  3. 원본을 로드한다.
  4. DefaultMap으로 로드한다.
  5. 다시 무변경을 확인한다.

## 12. 자동화

정본 Verification 표 전체를 구현한다. 새 테스트는 `Private/Tests/ServiceDisplayAutomationTests.cpp`와 `Private/Tests/ServiceFridgeAutomationTests.cpp`에 나눠 둔다. `ShopAutomationTests.cpp`처럼 큰 기존 파일에 누적하지 않는다. 추가 조건:

- asset이 아직 없으므로 테스트는 transient `UServiceItemDefinition`, native `AItemBoxActor`·`ADrinkFridgeActor` 파생 fixture로 구성한다. fixture는 테스트 전용 native class다.
- 넣기·빼기는 `UPlayerHeldTargetUseComponent` 경유로 검증한다. 반복은 기존 held-use 테스트처럼 DeltaTime을 직접 주입한다.
- 프리뷰·강조: focus 알림 뒤 proxy의 visibility, 위치(자리 transform), mesh를 확인한다. 강조 proxy는 `bRenderInMainPass=false`, `bRenderCustomDepth=true`, stencil 값을 확인한다. 설정을 끄면 숨김이다.
- payload: export → 새 staged fridge import 왕복. 잘못된 index·정원 초과·분류 불허 import는 실패하고 공간을 바꾸지 않는다. fresh는 빔이다.
- 손님: 테스트 사용자 Actor로 reserve/begin → take ×n → 적립 합계 → 빈 뒤 예약 불가. 예약 중 recovery query 거부, 회수 보류 중 예약 거부·취소 후 복구, 사용자 Destroy 시 slot Available.
- 수거함: 두 냉장고 적립 합산, 수금 1회, 즉시 두 번째 E 무지급·이유, wallet 실패 시 적립 유지. carry·placeable·discardable 인터페이스 미구현 확인.
- 상점: catalog Validation(둘 다 있음·둘 다 없음·박스 정원 0 거부), 박스 상품 주문 → 배송 상자 → 개봉 시 박스 12/12, 설비+박스 혼합 개봉, 박스 수량 1 상한.
- 기존 회귀(유지 필수): Shop 전체(개봉 무리 포함), Placement 회수·배치 전체, Facility·Customer 예약, Interaction held-use, Towel, Utility, Economy, Computer.
- 전체 회귀: `Automation RunTests BathhouseSim`. 수치와 실패 이름을 보고하고, 실패는 스스로 무관 판정하지 않는다.

## 금지

- 기존 export를 가진 class·property·subobject rename·삭제·class 변경, 기존 enum 순서 변경, native `Serialize` 변경, Core Redirect 추가.
- Config 파일 수정(Custom Depth 설정 포함), Content·Level 저장, 머티리얼 작성 등 Editor authoring.
- `UPlayerInteractionComponent`에 기능 추가, Character·Interaction·Widget에 품목·냉장고 concrete cast.
- 손님 루틴·StateTree 변경, 수건 시스템 변경, 화장대·샤워 품목 선행 구현.
- 기존 설비·상점·held-use 규칙 변경.

## 단계 결과물

- `.md/PROMPT_REVIEW.md`: 변경 파일, 영역별 요약, 11의 결과와 로그 위치, automation 수치와 시나리오 ID별 대응, 미검증(PIE 전용).
- `.md/PROMPT_UNREAL.md`: 정본 Editor Authoring 목록을 실제 결과에 맞게 구체화한다. 반드시 포함할 것:
  - Project Settings `Custom Depth-Stencil Pass = Enabled with Stencil`
  - `M_PP_TakeHighlightOutline`의 세 규칙: stencil 값 일치 판정, `ViewportUV` clamp 후 `BufferUV` 변환, 이웃 가림 검사
  - 카메라 blendable 연결(레벨 PPV 금지)
  - PIE 확인: 강조 중 화면 네 테두리에 선 없음(PIE 창 크기·Screen Percentage 변경 포함)
  - 콘솔 명령으로 FRDG-006~008·013 확인
- `.md/Architecture/*`는 수정하지 않는다. 설계와 달라야 하면 멈추고 보고한다.
