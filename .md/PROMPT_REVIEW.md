# 코드 리뷰 입력 — 서비스 1단위 수직 재작업(F1~F5)

## 단계와 범위

- 기능 계약 `.md/PROMPT_ARCHITECTURE.md`(DISP-001~022, FRDG-001~015, SHOP-S01·S02), 단계는 **수직 구현**. 입력은 `.md/PROMPT_IMPLEMENTATION.md` 맨 앞 "R. 아키텍처 재검토 반영 재작업"과 `.md/PROMPT_IMPLEMENTATION_R.md`(F1~F5)다.
- 이전 Service 구현(미커밋)을 그대로 두고 그 위에서 수정했다. `Content/`, `Config/`, `.md/Architecture/*`, `.md/Unreal/*`는 수정·저장하지 않았다(`git status -- Content Config` 무변경).
- 구현 QNA 미해결 없음. 설계와 달라서 멈춘 항목 없음.

## F1~F5 대응

| 항목 | 구현 | 검증 |
|---|---|---|
| F1 construction 뒤 payload 적용 | `IPlaceableFacility::FinalizePlacementPayloadAfterConstruction`(기본 true). `FFacilityActorConversionTransaction`이 `FinishSpawning` 직후·`IsValid` 확인 뒤·collision snapshot 확정 앞에서 호출, 실패하면 `DestroyStaged` 후 `nullptr`(item 미소모, publication 없음). `ETestFault::PlacementFinalizePayload` 추가. `ABathhouseFacilityActor::ImportPlacementPayload`는 base 필드만 처리하고 확장 data를 `PendingFacilityExtension`/`bPendingFacilityExtension`으로 보관(신규 설치는 null), finalize override가 `ImportFacilityExtension(Pending)` 호출 후 결과와 무관하게 pending 비움 | 새 fixture `AServiceAutomationConstructedFridge`(공간 4개·slot 1개를 `OnConstruction`의 NewObject+`RegisterComponent`로 생성) + 실제 transaction: 신규 설치 4공간 빔, 회수→재설치 복원(공간 0 우유 3, 공간 2 주스 2), 잘못된 payload 4종(index 불일치·정원 초과·분류 불허·공간 누락)·finalize fault → 배치 실패, 원래 item·payload·held 유지, 새 Actor 없음, 이후 정상 payload 재설치 성공. 기존 native subobject fixture 테스트는 `FinishSpawning`→finalize까지 호출하도록 수정 |
| F2 Editor 인계·미리보기 | `AItemBoxActor` 미리보기 world를 `Editor`+`EditorPreview`로 확대. `ADrinkFridgeActor::ValidateSpaceLayout`(공유 규칙)과 CDO 경로 `IsDataValid`(native subobject + Blueprint 상속 사슬 SCS `ComponentTemplate` 수집, WITH_EDITOR) | `Fridge.LayoutRuleAndDiscard`: 정상·slot 0·slot 2·공간 없음·index 중복·index gap·분류 태그 없음·자리 0. 테스트용 `UBlueprint`는 만들지 않았다: SCS 노드 수집 자체(BP CDO 경로)는 자동화 미검증이고 Editor 단계(Data Validation + SCS readback)로 넘겼다. `PROMPT_UNREAL.md` 전면 재작성 |
| F3 정본 정렬 | 코드 변경 없음. `DisplayOffset * SlotTransforms[i]` 유지(정본과 일치) | — |
| F4 자동화 | DISP-021: 진열이 든 냉장고 아이템(payload 보유)을 쓰레기통으로 버림 → item 소멸, 지갑·수거함 금액 불변. 박스 버리기도 쓰레기통 경로로 바꾸고 지갑·수거함 불변 단언 추가 | `Fridge.LayoutRuleAndDiscard`, `ItemBox.LifecycleAndContents` |
| F5 정리 | `FShopUnboxItemShape`와 factory 두 개를 `Private/Shop/ShopUnboxItemShape.h/.cpp`로 이동. 설비 전용 `FindSpawnTransforms(Definitions)` overload 삭제, 테스트 12곳은 테스트 helper `ShopUnboxTest::MakeShapes`(Private/Tests/ShopUnboxShapeTestSupport.h)로 shape 경로 사용. `LoadInsertPreviewMaterial`을 `Private/Service/ServiceDisplaySettings.cpp`로 이동 | `ShopUnboxingPlacement.cpp` 391줄(<400). Shop 개봉 무리 테스트 전부 통과 |

## 변경 파일 (이번 재작업분)

- Placement: `PlaceableFacility.h`(virtual 추가), `FacilityActorConversionTransaction.h/.cpp`(fault enum·호출).
- Facility: `BathhouseFacilityActor.h`(pending 필드·override 선언), `BathhouseFacilityPlacementDomain.cpp`(import 분리·finalize 구현).
- Service: `DrinkFridgeActor.h/.cpp`(`ValidateSpaceLayout`, CDO 검사), `ItemBoxActor.cpp`(미리보기 world), `ServiceItemDefinition.cpp`, 신규 `ServiceDisplaySettings.cpp`.
- Shop: `ShopUnboxingPlacement.h/.cpp`(shape 이동·overload 삭제), 신규 `ShopUnboxItemShape.h/.cpp`.
- 테스트: `ServiceAutomationTestProbe.h/.cpp`(constructed fixture), `ServiceAutomationTestSupport.h`, `ServiceFridgeAutomationTests.cpp`, `ServiceDisplayAutomationTests.cpp`, `ShopAutomationTests.cpp`·`ShopUnboxingScatterAutomationTests.cpp`(shape 경로), 신규 `ServiceFridgePlacementAutomationTests.cpp`, `ShopUnboxShapeTestSupport.h`.

## 클래스 크기·책임 변화

- `ShopUnboxingPlacement.cpp` 499 → 391줄. shape 표현은 별도 파일이 소유하고 무리 계산·월드 검사 규칙은 불변.
- `ABathhouseFacilityActor`: virtual override 1개와 pending 필드 2개만 추가. 냉장고 로직은 계속 `ADrinkFridgeActor`에 있다. 기존 설비 클래스(Bath·Utility·Towel machine)는 finalize 기본값(true)이거나 base의 pending 없는 경로라 동작 불변.
- `UPlayerInteractionComponent`, `UPlayerCarryComponent` 무변경.

## 설계와 문서 차이 보고

- R절과 `PROMPT_IMPLEMENTATION_R.md`는 F1 방향이 같아 충돌 없음. R절 F2가 요구한 "테스트용 `UBlueprint`로 SCS 검사"는 불가하다고 판단해 판정 함수를 component 목록으로 검사하는 대안을 썼다(R절이 허용한 경로).
- 로드 게이트: `BathhouseSim.Service.BlueprintLoad`는 인자 없이 실행하면 세 원본(`DA_ShopCatalog`, `WBP_InteractionPrompt`, `BP_Shower`)을 모두 로드한다. R절은 `BP_Shower`만 요구했지만 원본·DefaultMap 단계는 세 asset 전체가 통과했다. 복사본 단계는 `BP_Shower`만 실행했다.
- 이전 리뷰에서 있던 `PROMPT_REVIEW.md`의 "DisplayOffset 슬롯 로컬 해석" 편차는 정본이 같은 해석으로 정리돼 해소됐다.

## Blueprint·API·Core Redirect 영향

- 추가만 했다: `IPlaceableFacility` virtual 1개, `ETestFault` 값(테스트 전용), `ABathhouseFacilityActor` Transient 필드. rename·삭제·class 변경 없음, Core Redirect 없음.
- `DA_ShopCatalog`, `WBP_InteractionPrompt`, `BP_Shower` 로드 통과(아래).

## 검증 결과

- **빌드**: UE 5.8 `Build.bat BathhouseSimEditor Win64 Development` 성공. 로그 `Saved/Logs/build_r1.log`, `build_r2.log`.
- **정적**: `git diff --check -- Source` 공백 오류 없음(LF/CRLF 안내 경고만). `git status -- Content Config` 무변경. 신규 Source 미커밋 그대로.
- **copy-first load gate**(로그 `Saved/Logs/r_load_*.log`):
  1. `BP_Shower` 복사본을 `Content/Developers/MigrationCheck/`에 두고 Template 맵 로드: 성공, `Serial size mismatch`·Fatal 없음.
  2. 복사본과 빈 폴더 삭제 후 Content 무변경 확인.
  3. 원본 Template 맵 로드: 성공. 4. `/Game/Maps/DefaultMap` 로드: 성공. 5. 무변경 재확인.
- **집중**: `BathhouseSim.Service` 11개 전부 성공 — 기존 9개 + 신규 `Fridge.ConstructedPlacementTransaction`, `Fridge.LayoutRuleAndDiscard`. 로그 `Saved/Logs/auto_r.log`. Shop·Placement·BathWater·Utility 회수 테스트는 전체 회귀에 포함되어 통과.
- **전체 회귀** `Automation RunTests BathhouseSim`(headless, Template 맵): 88개 중 성공 76, 경고 포함 성공 12, 실패 0, 미실행 0(이전 86 → 88, 신규 2개). 경고는 기대된 `LogTemp` 경고와 기존 테스트 경고다. 리포트 `Saved/Automation/Reports/20260930/r_full`, 로그 `Saved/Logs/auto_r_full.log`.

## 리뷰 중점

- finalize가 `FinishSpawning` 뒤·snapshot 확정 앞에서만 불리고 실패 시 기존 import 실패와 같은 정리(item 미소모·publication 없음)를 타는지.
- base import 성공 후 finalize 실패 시 남는 부수효과(`BathWaterState->ResetEmptyForPlacement`, FacilityType 등)가 staged Actor 파괴로 함께 사라지는지.
- `PendingFacilityExtension`이 성공·실패 모두에서 비워지는지, 다른 설비 Actor(BathWaterUtility·Towel machine의 자체 Export/Import)가 이 경로를 타지 않는지.
- CDO `IsDataValid`의 SCS 수집이 부모 Blueprint의 template까지 포함하고 중복 카운트하지 않는지.

## 미검증

- **PIE 전용**: 외곽선 모양·화면 테두리, 프리뷰 반투명, 박스 안 물품 실제 모습, 0.15초 감각, 콘솔 명령 실제 동작, HUD 요약 표시. Content가 아직 없다.
- **Editor 전용**: 실제 `BP_DrinkFridge`(SCS) 배치, BP CDO `IsDataValid`의 SCS 수집 결과, `BP_ItemBox` 뷰포트 미리보기. `PROMPT_UNREAL.md`의 검증 방법으로 확인한다.
- 손님 루틴이 없어 냉장고 손님 계약은 시설 쪽 API를 테스트 사용자 Actor로만 검증했다.
