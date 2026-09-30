# 구현 프롬프트 — 서비스 2단위: 진열 확장(화장대·샤워 비품·수건 시스템 변경)

## 재작업 — 수건 프리뷰·강조 갱신 (2026-09-30)

서비스 2단위 구현 사이클 완료 뒤 사용자 보고로 확인한 결함의 재작업이다. 리뷰 입력은 `.md/PROMPT_IMPLEMENTATION_R.md`(수건 cue가 옮긴 뒤 따라가지 않음)다. 기능 계약은 바뀌지 않았다. 대상 시나리오: TOWL-001, 004, 005, 006, 010, 013, 016, 017, 018.

아래 `단계와 입력`과 0~10절은 완료된 2단위 본 구현 기록이며 이번에 다시 수행하지 않는다. 그 절의 clean 시작 조건은 이 재작업에 적용하지 않는다. headless 실행 형식(`.md/AGENT_WORKFLOW.md` UE 5.8 Headless Automation Policy)과 git `--no-optional-locks` 규칙은 그대로 따른다.

### 시작 조건

- UnrealEditor가 모두 종료됐는지 확인한다.
- 현재 Source·Content 변경(서비스 2단위 C++·Editor 작업물, 미커밋)은 이 단위의 결과물이므로 유지한다. 이번 재작업은 Content·Config를 수정·저장하지 않는다. 시작 전 `git --no-optional-locks status --short -- Content Config` 목록을 기록하고, 끝난 뒤 같은지 확인한다.

### 결정(정본)

- `.md/Architecture/TowelSystem.md` Service Unit 2 Display Changes의 `cue 재계산 경로`
- `.md/Architecture/InteractionSystem.md`의 `PresentationRevision` 항목
- 이 절과 정본이 다르면 멈추고 보고한다.

### 구현

1. `Public/Interaction/InteractionTypes.h` `FPlayerInteractionQuery`:
   - `UPROPERTY() int64 PresentationRevision = 0;`을 끝에 추가한다. Blueprint에 노출하지 않는다.
   - `Equals`에 `PresentationRevision == Other.PresentationRevision`을 추가한다.
2. `ACleanTowelStackActor`, `AUsedTowelBinActor`, `UTowelTransferPortComponent`의 `QueryInteraction`:
   - `PresentationRevision` = 대상 inventory `GetSnapshot().Revision` + 플레이어가 든 `ATowelBasketActor` inventory `Revision`(바구니가 아니거나 inventory가 없으면 0).
   - 모든 반환 경로(가능·불가·빈 query)에서 같은 규칙으로 채운다. 한 곳의 helper(`TowelDisplayCueUtils` 또는 기존 수건 query helper)에서 계산한다.
   - TargetName·행동명·가능 여부·이유·mode는 바꾸지 않는다.
3. `NotifyInteractionFocusChanged`·`TowelDisplayCueUtils::Update`·뚜껑 요청 로직은 바꾸지 않는다. 필요하면 뚜껑 요청이 같은 방향에서 중복 호출돼도 결과가 같은지(idempotent)만 확인한다.
4. 금지:
   - `UPlayerInteractionComponent`·`UPlayerHeldTargetUseComponent` 수정(성장 정책)
   - owner별 inventory 이벤트 구독이나 마지막 query 보관(선택지 A)
   - TargetName에 수량 추가(선택지 B, HUD 문구 변경)
   - 진열 공간·router·냉장고 query 변경
   - Towel 이동 조건·기계 상태·정원 변경

### 자동화

새 테스트는 `TowelDisplayCueAutomationTests.cpp`에 추가하고, `NotifyInteractionFocusChanged`를 직접 호출하지 않는다. 실제 경로로 검증한다.

- 경로:
  - 플레이어 fixture의 `UPlayerInteractionComponent::RefreshInteractionQuery`(조준 고정)
  - `UPlayerHeldTargetUseComponent` `BeginUse` + Tick 반복
- 선반·사용 수건통·대기 세탁기·대기 건조기 각각:
  - LMB 1장마다 프리뷰가 다음 자리로, 외곽선이 새 맨 위로 이동한다(사용 수건통은 Apply 없음, 외곽선만).
  - RMB 1장마다 외곽선이 새 맨 위로, 프리뷰가 방금 빠진 자리로 이동한다.
  - 누른 채 반복 중 매 1장마다 위 규칙을 지킨다.
  - 비교 기준은 cue transform = visual `GetIndexPresentation(새 Count / 새 Count−1)` transform이다.
- 외부 변화: 조준 중 선반에서 `UTowelTransferSubsystem`으로 손님 쪽 이동을 실행한 뒤 한 번의 `RefreshInteractionQuery`(tick 모사)로 외곽선·프리뷰가 새 Count 기준이 된다.
- query 단위:
  - 수량만 다른 두 query는 `Equals`가 false이고 HUD 필드(TargetName 등)는 같다.
  - 진열 공간·router·냉장고 query의 `PresentationRevision`은 0이다.
- 회귀(유지 필수): `BathhouseSim.Towel.Display.CuesDeterministicPileAndLid`, Towel 전체, Service 전체, Interaction held-use, Utility.
- 전체 회귀: `Automation RunTests BathhouseSim`. 수치와 실패 이름을 보고하고, 실패를 스스로 무관 판정하지 않는다.

### 빌드와 로드

- UE 5.8 `Build.bat`로 빌드한다.
- `FPlayerInteractionQuery` 필드 추가는 additive이고 asset에 저장되지 않는다. BP native parent·subobject 변경이 없으므로 copy-first gate는 DefaultMap 로드 1회로 확인한다.

### 결과물

- `.md/PROMPT_REVIEW.md`를 다시 쓴다: 선택된 경로, 변경 파일, 실제 경로 자동화 목록과 시나리오 대응, 빌드·로드·전체 회귀 수치, Content·Config 무변경 확인.
- `.md/PROMPT_UNREAL.md`의 PIE 관찰 항목에 추가한다: 선반·사용 수건통·세탁기·건조기에서 LMB·RMB를 누른 채 연속으로 옮길 때 프리뷰·외곽선이 1장마다 따라가는지, 조준 중 손님이 선반에서 가져간 뒤 외곽선이 새 맨 위로 옮겨 가는지.
- `.md/Architecture/*`는 수정하지 않는다.

## 단계와 입력

- 2026-09-30 사용자가 기능 계약 `.md/PROMPT_ARCHITECTURE.md`(서비스 2단위)의 설계를 승인했다. 대상 시나리오는 DISP-015~017·023·024, VANI-001~017, SHWR-001~011, TOWL-001~018, SHOP-S03·S04다.
- 단계: **수직 구현(단위 확장)**. 1단위는 커밋 `5b42a47`로 완료됐다.
- `.md/AGENT_WORKFLOW.md` → `.md/AGENT_IMPLEMENTATION.md`를 읽고 C++ 구현 단계만 수행한다.
- 설계 정본(전체 읽기):
  - `.md/Architecture/ServiceFacilityDisplaySystem.md`
  - `.md/Architecture/TowelSystem.md` Service Unit 2 Display Changes
  - `.md/Architecture/TowelPresentationSystem.md` Deterministic Index Layout
- 관련 절:
  - `ServiceSystem.md` 전체(1단위 구조)
  - `HeldTargetUseSystem.md` Towel Targets
  - `FacilitySystem.md`의 extension·회수 보류 항목
  - `PlacementSystem.md` staged 순서
  - `UtilityFuelSystem.md` Fuel Door Presentation
  - `CoreSystem.md` Class Growth Policy
- 이 문서와 정본이 다르면 구현을 멈추고 보고한다.
- headless 실행은 `.md/AGENT_WORKFLOW.md` UE 5.8 Headless Automation Policy 형식을 따른다. git은 `--no-optional-locks`로 실행한다.
- 시작 조건: `git --no-optional-locks status --short -- Source Content Config`가 비어 있어야 한다. `.md/` 변경은 정상이다.

## 0. 사전 조건과 백업

- UnrealEditor가 모두 종료됐는지 확인한다.
- `Saved/MigrationBackup/20260930_service_unit2/`에 파일 복사한다: `BP_DrinkFridge`, `BP_Shower`, `BP_Washer`, `BP_Dryer`, `BP_ItemBox` uasset.
- Content·Config·Level을 저장하지 않는다(아래 9의 복사본 확인만 예외).

## 1. 품목 정의

- `UServiceItemDefinition`에 `ConsumableUses`, `BoxItemOffset`을 추가한다. Validation을 확장한다.
- `AItemBoxActor::RebuildContentsVisual`: instance transform = `BoxItemOffset * BoxSlotTransforms[i]`. Editor preview도 같다.

## 2. Facility extension 계약과 회수 보류

정본 Facility Extension Contract.

- 새 파일:
  - `Public/Facility/FacilityPlacementExtension.h`: `IFacilityPlacementExtension`, `UFacilityPlacementExtensionData`
- 기존 파일 수정:
  - `UBathhouseFacilityPlacementInstanceData`: `Extensions` 추가, `GetPlacementContentsSummary()` override
  - `ABathhouseFacilityActor` 기본 hook: export 수집, finalize 전부 검증 → 전부 적용, fresh null
  - `IsDataValid`: CDO SCS 템플릿 수집 → `ValidateExtensionAuthoring`
- extension 처리 코드는 private helper(`Private/Facility/FacilityPlacementExtensionUtils.h/.cpp`)에 둔다. `BathhouseFacilityActor.cpp`와 `BathhouseFacilityPlacementDomain.cpp`를 키우지 않는다.
- 회수 보류:
  - `bRecoveryHoldActive`를 base에서 관리하고, 기본 `IsAvailableForReservation()`에 반영한다.
  - 욕탕 override가 Super를 호출하지 않으면 욕탕 기존 동결 규칙을 그대로 두고 차이를 보고한다.
  - 다른 설비 회귀를 확인한다.
- `EBathhouseFacilityType::Vanity`를 enum 끝에 append한다.

## 3. Display manager와 냉장고 이전

- `UServiceDisplayManagerComponent`, `UServiceDisplayPlacementData`: 정본 Display Manager And Payload.
- 냉장고 이전:
  - `ADrinkFridgeActor`에 manager를 native default subobject로 추가한다(`RequiredCustomerSlotCount` 1, 소모 false).
  - fridge의 export/import·SCS 검증·회수 보류·사용자 정리 override와 `UDrinkFridgePlacementInstanceData`를 제거하고 manager·base로 옮긴다.
  - `TryTakeDrinkForCustomer`, 판매 적립, 재고 기반 예약 가능 판정은 fridge에 남긴다.
  - 1단위 Service 테스트 전체가 결과 변경 없이 통과해야 한다. fixture 수정은 이전에 따른 호출 변경만 허용한다.

## 4. 진열 공간 모드·소모품·표시 도구

- `UDisplaySpaceComponent`:
  - `TargetMode`, `FixedKind`, `Stock.InUseRemaining`, `ConsumeOneUse`를 추가한다.
  - `FDisplaySpaceSnapshot`에 `InUseRemaining`을 추가한다.
  - 파일이 이미 441줄이므로 stock·소모품 계산은 private helper(`Private/Service/DisplayStockRules.h/.cpp`)로 뺀다. component에는 상태·방송·표시 연결만 둔다.
- `FServiceItemTransfer`: 꺼낼 수 있는 수 규칙과 `사용 중인 것은 꺼낼 수 없음` 이유.
- `UDisplayCueComponent`(신규): 기존 공간의 InsertPreview·TakeHighlightProxy를 이 도구로 교체한다. 1단위 프리뷰·강조 테스트 결과가 같아야 한다.

## 5. 설비 router와 held-use key

- `UDisplayFacilityTargetComponent`: 정본 Facility Target Router. 묶음 선택 함수 하나를 query·execute·focus가 공유한다. 조준선 최근접 계산은 순수 함수로 두어 테스트한다.
- `FPlayerInteractionQuery::HeldUseTargetKey`, `UPlayerHeldTargetUseComponent`의 key 비교를 추가한다. `UPlayerInteractionComponent`에는 추가하지 않는다.

## 6. 수건

정본 TowelSystem Service Unit 2 Display Changes, TowelPresentation Deterministic Index Layout.

- `FTowelHeldTransferRules`: 선반 Take, Waiting 기계 Take, 이유 문구.
- `ATowelProcessingMachineActor::AllowsInventoryTransfer`: Waiting 빼기 허용.
- 선반·사용 수건통·transfer port: focus observer와 `UDisplayCueComponent` native subobject.
- quantity visual에 `GetIndexPresentation`을 추가한다. Pile은 seed·index 결정적 계산으로 바꾸고, count 0에서 seed를 갱신한다. Stack·Pile Editor preview를 회귀 확인한다.
- `UOpeningPresentationComponent`(Interaction/Presentation, 신규): 알고리즘은 `UUtilityFuelDoorComponent`와 같다. **보일러 문 class는 수정하지 않는다.**
- 기계 native subobject `LidPivot`, `LidMesh`, `LidPresentation`을 추가하고 port focus에서 열림을 요청·해제한다.

## 7. 비 shipping 콘솔 명령

- `bathhouse.Debug.Facility.BeginUse`, `bathhouse.Debug.Facility.EndUse`(정본 Vanity And Shower). 1단위 냉장고 명령은 유지한다.

## 8. 빌드

- `.md/AGENT_WORKFLOW.md`의 UE 5.8 `Build.bat`만 사용한다.

## 9. 로드 검증 — 복사본 먼저

- `BathhouseSim.Service.BlueprintLoad` 대상에 `BP_DrinkFridge`, `BP_Shower`, `BP_Washer`, `BP_Dryer`, `BP_ItemBox`를 추가한다.
- 확인:
  - native parent
  - 새 native subobject(fridge manager, 기계 Lid 3종) 존재
  - 기존 slot·inventory·visual 유지
  - fridge SCS 공간 4개 유지
- 저장하지 않는다.
- 순서(Fatal·`Serial size mismatch`·`Failed to load`면 즉시 멈춤):
  1. 복사본을 `Content/Developers/MigrationCheck/`에 두고 Template 맵으로 로드한다.
  2. 복사본을 삭제하고 Content 무변경을 확인한다.
  3. 원본을 로드한다.
  4. DefaultMap으로 로드한다.
  5. 다시 무변경을 확인한다.

## 10. 자동화

정본 Verification 표 전체와 TOWL 항목을 구현한다. 새 테스트는 파일을 나눠 둔다:
`ServiceFacilityDisplayAutomationTests.cpp`(router·묶음·소모품), `ServiceFacilityPayloadAutomationTests.cpp`(extension·payload·회수 보류), `TowelDisplayCueAutomationTests.cpp`(수건 규칙·표시·결정적 배치·뚜껑).

추가 조건:

- 화장대·샤워기 fixture는 1단위 규칙대로 공간·router·slot을 construction 중에 만든다. 실제 `FFacilityActorConversionTransaction` 경로로 설치·회수·재설치를 검증한다.
- 소모 검증은 slot `TryReserve` → `BeginUse`(Occupied 전이)로 한다. 두 slot 샤워 fixture로 SHWR-010을 검증한다.
- router 최근접: 조준선과 묶음 중심 거리 계산을 순수 함수로 검증하고, 동거리 tie-break를 확인한다.
- DISP-024(2026-09-30 정정 계약): 빈 박스·드라이기 2·빗 3 fixture에서 드라이기 쪽 조준 RMB 반복 → 첫 1개 뒤 박스 종류가 드라이기, 조준을 빗 쪽으로 옮겨도 드라이기만 빠지고 빗 3 불변, 드라이기가 다 빠지거나 박스가 차면 멈춤, 조준이 router를 벗어나면 멈춤.
- 반복 key guard: 같은 target object에서 key가 바뀌면 멈추고 떼기 전 재개가 없어야 한다(guard 단위 검증. DISP-024 경로에서는 발동하지 않음).
- Pile: 같은 seed·index는 같은 transform이다. LIFO 제거 뒤 재삽입 위치가 같다. count 0 이후 seed가 바뀐다. 프리뷰 transform = 삽입 후 실제 instance transform이다.
- 뚜껑: Apply 또는 Take 가능 조준에서 목표 열림, 작동 시작·focus 종료·suppression에서 닫힘, 작동 중 열림 없음.
- 기존 회귀(유지 필수): Service 1단위 전체, Towel 전체(CTRL 결과 중 015·016만 의도 변경), Interaction held-use, Utility(보일러 문 포함), Facility·Placement·Customer 예약, Shop, Economy.
- 전체 회귀: `Automation RunTests BathhouseSim`. 수치와 실패 이름을 보고하고, 실패는 스스로 무관 판정하지 않는다.

## 금지

- 기존 export를 가진 class·property·subobject rename·삭제·class 변경. 단, 정본이 지정한 Transient native `UDrinkFridgePlacementInstanceData` 삭제와 `ADrinkFridgeActor` 잔재(private 빈 `UFUNCTION` 3개, Transient `BoundSpaces`) 삭제는 예외다. 기존 enum 순서 변경, native `Serialize` 변경, Core Redirect 추가도 금지다.
- `UUtilityFuelDoorComponent`의 부모·property 변경.
- Customer 코드·StateTree 변경. 샤워 연동은 slot 전이만 쓴다.
- `UPlayerInteractionComponent`에 기능 추가, Character·Interaction·Widget에 품목·설비 concrete cast.
- 수건 상태 전환·작동·정원·손님 규칙 변경.
- Config·Content·Level 저장과 Editor authoring.

## 단계 결과물

- `.md/PROMPT_REVIEW.md`: 변경 파일, 영역별 요약, 9의 결과와 로그 위치, automation 수치와 시나리오 ID별 대응, 미검증(PIE 전용), 클래스 줄 수 변화.
- `.md/PROMPT_UNREAL.md`: 정본 Editor Authoring 표를 exact path·저장 allowlist·검증 방법·갱신할 `.md/Unreal/*` 목록으로 구체화한다. 1단위와 같은 수준으로 쓴다. 반드시 포함할 것:
  - 품목 6종 박스 자리와 `BoxItemOffset`, 박스 크기 초과 시 중단
  - 화장대 묶음 배치(서로 떨어진 위치)와 router Box가 외형 전체를 덮는지
  - 샤워기 BP 변경과 레벨 instance 상속 확인(저장 대상은 BP만)
  - 세탁기·건조기 뚜껑과 Pile 범위, 열린 뚜껑으로 프리뷰가 보이는지
  - 대표 시나리오 PIE 절차와 TOWL·VANI·SHWR 관찰 방법(키, 조준 위치, 기대 HUD 문구)
- `.md/Architecture/*`는 수정하지 않는다. 설계와 달라야 하면 멈추고 보고한다.
