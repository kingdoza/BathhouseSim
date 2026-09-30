# 재작업 프롬프트 — 서비스 1단위 수직: 냉장고 payload import 시점과 Editor 인계 보완

## 재검토 결론

- 2026-09-30 코드 리뷰 결론: **아키텍처 재검토**(F1) 후 **구현 재검토**(F1 반영, F2~F5).
- 입력은 `.md/PROMPT_REVIEW.md`, `.md/PROMPT_UNREAL.md`와 현재 작업 트리다. 기능 계약은 `.md/PROMPT_ARCHITECTURE.md`(DISP-001~022, FRDG-001~015, SHOP-S01·S02), 단계는 **수직 구현**이다.
- 확인된 사항:
  - 작업명·시나리오 ID·단계가 네 입력에서 일치한다.
  - 리뷰 시점에 UE 5.8 `Build.bat`를 다시 실행했고 결과는 "Target is up to date"다. 빌드 로그 이후 변경된 Source는 없다.
  - `git status -- Content Config`에 변경이 없다.
  - 전체 회귀 리포트는 성공 75, 경고 포함 성공 11, 실패 0으로 보고와 일치한다.
- Editor 단계로 넘기지 않는다. 아래 F1 때문에 대표 시나리오의 "냉장고 배치"가 실제 `BP_DrinkFridge`에서 성립하지 않는다.

## F1 — 치명: 배치 import가 Blueprint 진열 공간 생성 전에 실행된다 (아키텍처 결정 필요)

### 근거

- `FFacilityActorConversionTransaction`(`Private/Placement/FacilityActorConversionTransaction.cpp:323-382`)의 순서는 다음과 같다.
  1. `SpawnActorDeferred`
  2. `PrepareForStagedPlacement`
  3. `ImportPlacementPayload`
  4. `FinishSpawning`
- Blueprint SCS 컴포넌트는 `FinishSpawning`의 construction에서 생성된다.
- `ADrinkFridgeActor::ImportFacilityExtension`(`Private/Service/DrinkFridgeActor.cpp:247`)은 신규 설치(`Data == nullptr`)와 재설치 모두에서 `CollectSpaces`를 먼저 호출한다. `CollectSpaces`는 `GetComponents<UDisplaySpaceComponent>`와 slot 수 1개를 요구한다.
- `PROMPT_UNREAL.md`와 `ServiceSystem.md`는 진열 공간 4개와 손님 slot을 `BP_DrinkFridge`에 추가하는 구조로 정했다.
- 따라서 import 시점에는 공간 0개, slot 0개다. `"냉장고에는 진열 공간이 1개 이상 필요합니다."`로 실패하고 staged Actor가 파괴된다.
- 결과적으로 신규 설치(FRDG-012, 대표 시나리오)와 재설치 복원(FRDG-009)이 모두 실패한다.
- 자동화가 이를 잡지 못한 이유:
  - fixture `AServiceAutomationFridge`는 공간과 slot을 생성자의 `CreateDefaultSubobject`로 만든다.
  - `SpawnInstalledFridge`도 `FinishSpawning` 전에 import한다.
  - 즉 native subobject라서 통과한 것이며, 실제 BP authoring 구조를 재현하지 않는다.

### 아키텍처 단계가 정할 것

리뷰는 구조를 고르지 않는다. 다음 불변식을 모두 지키는 import 시점과 경로를 `ServiceSystem.md` Placement Payload Extension과 `PlacementSystem.md` transaction 순서에 확정한다.

- 공간 수, 공간 정원, 자리 위치는 Editor(BP)에서 조정한다. 기능 계약 "공간 수와 공간당 정원은 Editor에서 조정한다"를 유지한다.
- 잘못된 payload는 전부 거부하고 아무 공간도 바꾸지 않는다. 실패는 staged placement rollback(item 미소모, publication 없음)으로 전파한다.
- 기존 설비(`BP_Shower` 등)의 hook 기본 동작, placement 순서와 결과는 바뀌지 않는다.
- 고려할 후보(선택은 아키텍처 단계):
  - transaction이 `FinishSpawning` 뒤, 배치 검증 전에 확장 import 단계를 호출한다.
  - import 시점에는 payload만 보관하고, construction 뒤 검증·적용 실패를 기존 실패 경로로 전파한다.
  - 그 밖의 방식.
- 구현 단계는 확정된 설계로 `PROMPT_IMPLEMENTATION.md`를 받은 뒤 수정한다.

### 재검증 조건

- 공간과 slot이 **생성자 subobject가 아닌** fixture로 테스트한다. 예: `FinishSpawning` 중 construction에서 생성되는 component, 또는 SCS를 가진 테스트용 Blueprint 대체 경로.
- 이 fixture로 실제 `FFacilityActorConversionTransaction` 경로를 통과시켜 다음을 확인한다.
  - 신규 설치가 빈 공간으로 성공한다.
  - 회수 → 재설치가 공간별 종류·수량을 복원한다.
  - 잘못된 payload는 거부되고 item과 공간이 모두 변하지 않는다.
- 기존 Placement·Shop·Facility 회귀가 유지된다.

## F2 — 중요: `PROMPT_UNREAL.md`가 Editor 계약을 정확히 인계하지 않는다

`AGENT_IMPLEMENTATION.md`의 `PROMPT_UNREAL.md` 항목과 `AGENT_REVIEW.md` 기능 계약 검토 기준에 미달한다. F1 반영 후 다시 작성한다.

1. **exact asset path가 없다.** 문서 전체에 `/Game/` 경로가 0개다.
   - 대상: 신규 asset 전부(`DA_ServiceItem_BananaMilk`, `BP_ItemBox`, `BP_DrinkFridge`, `BP_DrinkCollectionBox`, `DA_FacilityPlacement_DrinkFridge`, `M_PP_TakeHighlightOutline`)와 이름이 없는 "프리뷰 반투명 머티리얼".
   - 기존 규약을 따른다: `/Game/Bathhouse/Data/...`, `/Game/Bathhouse/Blueprints/...`, `/Game/Bathhouse/Materials/...`.
   - `RecoveryItemClass`도 정확한 경로로 적는다(`/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem`).
2. **저장 allowlist가 이름만 적혀 있다.**
   - `BP_FirstPersonCharacter`, `WBP_InteractionPrompt`는 full path로 적는다.
   - `DefaultMap`은 World Partition이다(`.md/Unreal/InteractionUISystem.md` 30~31행 참조). 수거함 배치가 저장될 external actor package 경로 규칙과, `DefaultMap.umap` 자체는 저장하지 않는지를 명시한다.
3. **갱신할 `.md/Unreal/*System.md`가 없다.**
   - 신규 `.md/Unreal/ServiceSystem.md`와 `0_UNREAL.md` 지도 항목을 추가 대상으로 적는다.
   - `ShopSystem.md`(catalog 상품 2개), `PlacementSystem.md`(Definition 표), `InteractionUISystem.md`(WBP `HeldSummaryText`, 카메라 blendable)의 갱신 범위도 적는다.
4. **검증 방법이 성립하지 않는 항목이 있다.**
   - `ADrinkFridgeActor::IsDataValid`는 `IsTemplate()`에서 검사를 건너뛴다. BP CDO에는 SCS component도 없다. 그래서 `BP_DrinkFridge` Data Validation은 항상 통과한다.
   - 공간 index·자리·분류·slot 수는 다른 방법으로 확인하도록 명시한다. 예: MCP로 SCS template 값 readback, PIE 배치 성공과 HUD `빈 공간 0/6` ×4.
5. **PIE 절차에 대표 시나리오 전체가 없다.**
   - 기능 계약 "단계 판단"의 대표 시나리오를 순서대로 1회 수행하는 절차를 넣는다.
   - DISP-003~011·020·022의 관찰 방법도 적는다: 키, 조준 대상, 기대 HUD 문구, 병 위치.
6. **품목 박스 Editor 미리보기 방법이 없다.**
   - `AItemBoxActor::OnConstruction`과 `RefreshEditorPreview`는 `EWorldType::Editor`에서만 동작한다. Blueprint 에디터 뷰포트(`EditorPreview`)와 class defaults 버튼에서는 보이지 않는다.
   - 레벨에 둔 미리보기 인스턴스는 게임에서 초기화되지 않은 박스로 남는다. 들 수 없고 조준 표시도 없다.
   - 미리보기 방법(레벨 인스턴스, 저장 금지)을 명시한다.
   - BP 뷰포트 미리보기가 기능 계약상 필요한지는 아키텍처 단계가 판단한다(F3).

## F3 — 보통: 설계 정본과 구현의 불일치 (아키텍처 단계 정리)

구현 프롬프트는 Architecture 수정을 금지했고, 구현 단계는 차이를 보고만 했다. F1 설계와 함께 `ServiceSystem.md`를 현재 상태로 맞춘다.

- Status가 "Source 미반영"으로 남아 있다.
- `DisplayOffset` 합성
  - 정본: `SlotTransforms[i] * Kind->DisplayOffset`
  - 구현: `DisplayOffset * SlotTransforms[i]`(자리 로컬, `DisplaySpaceComponent.cpp:253`)
  - `PROMPT_UNREAL.md`는 구현 쪽 해석을 따른다. MCP 단계가 두 문서에서 서로 다른 기준을 받지 않도록 하나로 정한다.
- 정본에 없는 공개 API와 이름 차이
  - 이름 차이: 정본 `TryMoveOne` / 구현 `TryApplyOne`·`TryTakeOne`
  - 추가 API: `AItemBoxActor::SpawnFilledBox`, `GetMutableContents`/`NotifyContentsChanged`, `UDisplaySpaceComponent::ImportStock`/`RemoveOneForCustomer`/`PublishStockChanged`/`IsOperational`
  - 추가 동작: placeable이 아닌 owner는 게이트하지 않는 규칙
  - 추가 helper: `FShopProductRules`, `FShopUnboxItemShape`
- 품목 박스 Editor 미리보기 world 범위(F2-6).

## F4 — 낮음: 자동화 누락

- DISP-021 냉장고 아이템 버리기가 없다. 정본 Verification 표에 있는 항목이다.
  - 진열이 든 냉장고 아이템(payload 보유)을 쓰레기통 경로로 버린다.
  - item과 payload 소멸, 지갑·수거함 금액 불변을 확인한다.
- 박스 버리기 테스트에 돈 불변 검증을 추가한다.

## F5 — 낮음: 클래스 성장·정리

- `ShopUnboxingPlacement.cpp`가 399줄에서 499줄로 늘어 400줄 경고선을 넘었다.
- 설비 전용 `FindSpawnTransforms(Definitions)` overload는 이제 production 호출처가 없고 테스트 12곳만 쓴다.
  - 구현 프롬프트는 "입력을 shape 목록으로 바꾼다"였다.
  - 선택지: 테스트를 shape 경로로 옮기고 overload를 제거한다. 또는 유지 근거와 제거 조건을 정본에 남긴다.
  - `FShopUnboxItemShape` factory 두 개를 별도 private 파일로 옮기는 것은 선택 사항이다.
- `UServiceDisplaySettings::LoadInsertPreviewMaterial`이 `ServiceItemDefinition.cpp`에 정의돼 있다. `ServiceDisplaySettings.cpp`로 옮긴다.

## 유지할 것 (리뷰에서 문제없음)

- `FServiceItemTransfer`
  - 이유 우선순위와 문구가 정본·기능 계약 표와 일치한다.
  - 실패 시 두 stack·Revision이 불변이다.
  - 빈 박스는 Take 정원으로 공간 종류의 정원을 쓴다.
- held-use 경로
  - `BeginUse`와 `TickRepeat`이 매번 재평가한 뒤 1개만 이동한다.
  - 공간 전환, 박스 내려놓기, suppression에서 멈춘다.
  - 가득 참 이유가 1회 보고된다.
- 프리뷰·강조
  - observer는 표현만 바꾼다.
  - proxy는 main/depth pass 제외, custom depth와 stencil 설정값을 쓴다.
  - 설정값을 매 알림마다 읽는다.
  - transient component는 NoCollision·Navigation off라 `ValidateNavigationContract`와 충돌하지 않는다.
- 냉장고 손님 계약
  - 가져가기와 적립이 같은 단계이고 방송은 commit 뒤다.
  - 회수 보류 중 예약을 거부한다. 취소하면 해제된다.
  - 사용자 EndPlay에서 `ForceRelease`한다.
- 수거함
  - query를 다시 검증한다.
  - `CanAddMoney`를 먼저 확인하고, 성공할 때만 0으로 만든다.
  - 재진입 guard가 있다.
  - wallet 해석 경로가 기존 cash 액터와 같다.
- 상점: 정확히 하나의 정의만 허용하고, 박스 12/12 개봉, rollback이 기존과 같다.
- HUD: `HeldObjectSummary`를 `Equals`에 포함하고, `UPlayerInteractionComponent`는 무변경이다.
- 추가만 한 reflected 변경: enum append, struct field, 비 final. Core Redirect가 필요 없다.

## 재작업 후 리뷰 입력

- 갱신된 `ServiceSystem.md`(F1·F3)와 `PROMPT_IMPLEMENTATION.md`
- 재작성된 `PROMPT_REVIEW.md`: F1~F5 대응, 새 fixture 방식, 전체 회귀 수치
- 재작성된 `PROMPT_UNREAL.md`(F2)
- Content·Config 무변경을 유지한다. 기존 copy-first load gate는 hook 시점이 바뀌면 `BP_Shower`만 다시 실행한다.
