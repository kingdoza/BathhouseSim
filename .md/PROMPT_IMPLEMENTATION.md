# 구현 프롬프트 — 서비스 4단위: 배치형 서비스(안마의자·휴게 공간·세신)

## 단계와 입력

- 기능 계약: `.md/PROMPT_ARCHITECTURE.md`(서비스 4단위). 대상 시나리오는 MASS-001~015, REST-001~006, SCRB-001~020, SVC4-001~003이다. 설계 중 사용자 결정 Q66 A, Q67 A와 G3·G4 정정이 반영돼 있다.
- 기술 질문: `.md/QNA_ARCHITECTURE.md` — 없음.
- 단계: **수직 구현(단위 확장)**. 1~3단위와 버그 수정은 커밋 `2c14d8f`, `4111e20`까지 완료됐다.
- `.md/AGENT_WORKFLOW.md` → `.md/AGENT_IMPLEMENTATION.md`를 읽고 C++ 구현 단계만 수행한다.
- 설계 정본(전체 읽기): `.md/Architecture/ServiceAmenitySystem.md`
- 관련 절:
  - `CombatSystem.md` 전체(렌치·근접 공격)
  - `ComputerSystem.md`(포커스 진입·이탈·이탈 위치 helper)
  - `HeldTargetUseSystem.md` Input Ownership·HUD Data
  - `CleaningLitterSystem.md` Input Routing(장비 보조 필드)
  - `PlacementSystem.md`(staged 순서, 회수 수행자)
  - `ServiceSystem.md`, `ServiceFacilityDisplaySystem.md`(facility payload hook, 사용자 정리, 비 shipping 명령)
  - `CustomerRecoverySystem.md`(slot EndUse/BeginUse 중단 규칙)
  - `UISystem.md`, `CoreSystem.md` Class Growth Policy
- 이 문서와 정본이 다르면 구현을 멈추고 보고한다.
- 시작 조건: `git --no-optional-locks status --short -- Source Content Config`가 비어 있어야 한다. `.md/` 변경은 정상이다.
- headless 실행은 `.md/AGENT_WORKFLOW.md` UE 5.8 Headless Automation Policy 형식을 따른다.

## 0. 사전 조건과 백업

- UnrealEditor가 모두 종료됐는지 확인한다.
- `Saved/MigrationBackup/20261001_service_unit4/`에 다음 uasset을 파일 복사한다.
  - `/Game/FirstPersonCharacter/BP_FirstPersonCharacter`
  - `/Game/Bathhouse/Blueprints/Game/BP_BathhouseHUD`
  - `/Game/Bathhouse/Blueprints/Combat/BP_MonkeyWrench`
  - `/Game/Bathhouse/Blueprints/Facility/BP_Shower`
- Content·Config·Level을 저장하지 않는다(9의 복사본 확인만 예외).

## 1. 공통 계약과 검증 수단

- `EBathhouseFacilityType` 끝에 `MassageChair`, `RestBench`, `Television`, `ScrubTable`을 append한다.
- `EPhysicalCarryKind` 끝에 `ScrubTowel`을 append한다. 기존 순서는 바꾸지 않는다.
- `Public/Service/ServiceAmenityTypes.h`: `IServiceFacilityUser`(C++ 전용), `EServiceUseEndReason { Completed, Abandoned }`
- `AServiceTestUserActor`: 정본 Test User 절. collision·물리·Tick 없음.
- `ServiceAmenityDebugCommands.cpp`(`#if !UE_BUILD_SHIPPING`): 명령 네 개. 대상 해석은 `FacilityDisplayDebugCommands.cpp`의 조준 해석을 따른다.
  - 공용 helper가 private이면 같은 방식으로 작성한다. 기존 파일은 수정하지 않는다.
  - 인형 spawn이나 slot 전이가 실패하면 인형을 제거하고 로그를 남긴다.

## 2. Q67 장비 합성 규칙

- `UPlayerEquipmentUseComponent::MergeEquipmentQuery`: 정본 Equipment Merge.
  - 장비 LMB query visible: 기존대로 held-use 전부 지움.
  - `!bVisible`: Take를 지우고, Apply는 visible·이유만 남긴다(`bCanHeldApply=false`, 행동명 비움).
  - 장비 보조 필드(`EquipmentSecondary*`)는 기존대로 유지한다.
- `BeginEquipmentUse` 무보고 종료 분기: 합성 focus query에 Apply 이유가 있으면 그 이유로 실패 result를 보고한다(intent `EquipmentUse`).
- 다른 장비(대걸레·렌치·배송 상자)는 항상 visible이라 결과가 같아야 한다. 기존 테스트로 확인한다.

## 3. 안마의자·렌치 수리·회수 수행자

- `IPlaceableFacility::SetFacilityRecoveryInstigator(AActor*)`: 기본 no-op.
  - `ABathhouseFacilityActor`: Transient weak 보관과 getter.
  - `UPlayerFacilityPlacementComponent`: `TryBeginFacilityRecoveryHold` 직전에 owner pawn을, `CancelRecovery`에서 nullptr를 넘긴다. 다른 흐름은 바꾸지 않는다.
- `AMassageChairActor`, `UMassageChairPlacementInstanceData`: 정본 Massage Chair 절 그대로다.
  - timer는 world timer(game time)다.
  - 사용자 `OnEndPlay` 구독·해제는 대칭으로 한다.
  - E 수금은 `ADrinkCollectionBoxActor`의 wallet 해석·재진입 guard 방식을 따른다.
  - 동전함 지급은 회수 publication callback에서 한다. wallet은 stage에서 해석·검증한다.
  - Data Validation: CDO SCS slot 정확히 1개(1단위 냉장고 SCS 검사 방식), 값 범위.
- `IWrenchRepairable`(Public/Combat), `FWrenchRepairSession`(Private/Combat): 정본 Wrench Repair 절.
- `AMonkeyWrenchActor`: query·Begin·Update·End·Cancel 분기만 추가하고 세션 로직은 helper에 둔다. 변경 후 cpp 400줄 이하를 목표로 한다. 넘으면 보고한다.
  - 수리 중 `StartAttack`·피격은 없다.
  - BP event `OnRepairActiveChanged(bool)`

## 4. TV와 평상

- `ATelevisionActor`: 정본 Television 절. slot이 없어도 facility 등록·회수가 되는지 확인한다.
- 평상은 코드가 없다. 테스트는 slot 3개 fixture로 기존 규칙을 확인한다.

## 5. 세신

- `AScrubTowelActor`: 정본 Scrub Towel 절. carry·fixed slot·복구는 `AWetMopActor` 구조를 따르되 장비 interface는 없다.
- `AScrubTableActor`: 정본 Scrub Table 절.
  - native subobject 여섯 개와 slot(Blueprint) 1개
  - 대기 timer, 게이지, 완료·만료·쓰러짐·사용자 상실
  - 세신자 예약, `OnScrubSessionEnded`
  - 현금 spawn 실패 시 완료하지 않음
  - Data Validation: slot 1개, `ScrubArea` extent > 0, 값 범위, `CashOfferClass` 지정
- `UPlayerScrubFocusComponent`: 정본 Scrub Focus Session 절.
  - 이탈 위치는 `FComputerFocusExitPlacement::Resolve`를 그대로 호출한다.
  - `UPlayerComputerUseComponent`와 `ComputerFocusExitPlacement.*`는 수정하지 않는다.
- `AFirstPersonCharacter`: 정본 Character Input Routing 절.
  - `IsFocusCapturingInput()`으로 기존 컴퓨터 차단 검사를 바꾼다. 컴퓨터 동작은 결과가 같아야 한다.
  - E·ESC·Look·LMB 분기를 추가하고, `EPrimaryUsePressOwner`에 `Scrub`을 append한다.
- `UScrubFocusHudWidget`과 `ABathhouseHUD`: money widget 생성·pawn 주입·EndPlay 해제 방식을 따른다. `ScrubFocusHudWidgetClass`(EditDefaultsOnly)가 없으면 경고 로그만 남기고 생성하지 않는다.

## 6. 상점

- 코드 변경은 없다. 네 설비 Definition과 catalog 항목은 Editor 단계다. 상점 테스트는 fixture Definition으로 개봉·world 버리기(SVC4-002)를 확인한다.

## 7. 비 shipping 확인

- 콘솔 명령과 테스트 인형 spawn 경로가 `#if !UE_BUILD_SHIPPING` 안에 있는지 확인한다(SVC4-003).

## 8. 빌드

- `.md/AGENT_WORKFLOW.md`의 UE 5.8 `Build.bat`만 사용한다.

## 9. 로드 검증 — 복사본 먼저

- 대상: 0의 네 asset.
- 확인:
  - native parent
  - `BP_FirstPersonCharacter`의 새 `PlayerScrubFocus` subobject
  - HUD 새 property
  - 렌치·샤워기 기존 component 유지
- 저장하지 않는다.
- 순서(Fatal·`Serial size mismatch`·`Failed to load`면 즉시 멈춤):
  1. 복사본을 `Content/Developers/MigrationCheck/`에 두고 Template 맵으로 로드한다.
  2. 복사본을 삭제하고 Content 무변경을 확인한다.
  3. 원본을 로드한다.
  4. DefaultMap으로 로드한다.
  5. 다시 무변경을 확인한다.

## 10. 자동화

정본 Verification 표 전체를 구현한다. 새 테스트는 파일을 나눠 둔다.

- `ServiceAmenityChairAutomationTests.cpp`: 안마의자·수리·회수 지급
- `ServiceAmenityRestAutomationTests.cpp`: TV·평상
- `ServiceAmenityScrubAutomationTests.cpp`: 세신대·포커스·때수건·인형
- `EquipmentMergeQ67AutomationTests.cpp`: 합성 규칙

추가 조건:

- 설비 fixture는 Service 규칙대로 slot을 construction 중에 만든다. 실제 `FFacilityActorConversionTransaction` 경로로 설치·회수·재설치를 검증한다.
- timer 검증은 test world tick 또는 timer manager 진행으로 결정적으로 한다. 고장 확률은 0·100으로 검증하고, 중간 확률은 판정 함수만 단위 검증한다.
- 회수 지급(MASS-008):
  - 수행자 pawn의 PlayerState wallet이 +금액이고 동전함 0이다.
  - Q 취소·stage 실패(wallet 없음)에서는 무변화다.
  - 재설치 뒤 고장이 유지되고, 아이템 요약에 `고장`이 있다.
- 쓰러짐 대역(MASS-011, SCRB-020): slot `EndUse` → 요금·고장 없음, Reserved 유지 → `BeginUse` 재시작 60초 / 게이지 0·대기 90초 다시.
- 세신 포커스(SCRB-005~011, 016):
  - `AddRubInput`에 정해진 축 값을 넣는다. LMB 없이 넣으면 게이지 불변이다.
  - 누른 채 넣으면 누적 = clamp 뒤 이동 거리이고, 경계에서 밀어도 늘지 않는다.
  - 이탈 위치가 막히면 근처 빈자리를 쓴다(컴퓨터 이탈 테스트 fixture 재사용).
  - 입력 차단: 이동·점프·G·Q·RMB가 무변화이고, 진입 E release로는 이탈하지 않는다.
- Q67(MASS-012, 015, SCRB-019):
  - 집게로 고장 안마의자·쓰레기 아닌 수건 선반을 조준하면 LMB 이유가 보이고 강조·뚜껑은 꺼져 있다. 3단위 버그 회귀 테스트는 유지된다.
  - 집게로 쓰레기를 조준하면 `줍기`다.
  - 대걸레·렌치·배송 상자는 기존 행이다.
- 기존 회귀(유지 필수):
  - Computer 전체(포커스·이탈·ESC)
  - Combat 전체
  - Interaction held-use
  - Cleaning·3단위 버그 회귀
  - Service·Shop·Placement·Facility 전체
- 전체 회귀: `Automation RunTests BathhouseSim`. 수치와 실패 이름을 보고하고, 실패는 스스로 무관 판정하지 않는다.

## 금지

- 기존 export를 가진 class·property·subobject의 rename·삭제·class 변경, 기존 enum 순서 변경, native `Serialize` 변경, Core Redirect 추가.
- `UPlayerComputerUseComponent`, `ComputerFocusExitPlacement.*`, `UPlayerInteractionComponent`, `UPlayerCarryComponent`, `UPlayerHeldTargetUseComponent` 수정.
- Customer 코드·StateTree 변경. 손님 쪽 연결은 `IServiceFacilityUser` 계약까지만 한다.
- 범용 Facility base에 안마의자·세신·TV 분기 추가(base에는 회수 수행자 필드만).
- 컴퓨터·세신 포커스 공통 부모 class 도입.
- Config·Content·Level 저장과 Editor authoring.

## 단계 결과물

- `.md/PROMPT_REVIEW.md`:
  - 변경 파일, 영역별 요약
  - 9의 결과와 로그 위치
  - automation 수치와 시나리오 ID별 대응
  - 미검증(PIE 전용)
  - 클래스 줄 수 변화. 특히 `MonkeyWrenchActor.cpp`, `FirstPersonCharacter.cpp`, `PlayerEquipmentUseComponent.cpp`, `PlayerFacilityPlacementComponent.cpp`, `BathhouseFacilityActor.*`
- `.md/PROMPT_UNREAL.md`: 정본 Editor 계약을 exact path·Parent Class·저장 allowlist·검증 방법·갱신할 `.md/Unreal/*` 목록으로 구체화한다. 1~3단위와 같은 수준으로 쓴다. 반드시 포함할 것:
  - 신규 BP 여섯 개(임시 mesh). 각 BP의 slot·authoring 값.
  - `BP_ScrubTable`의 카메라·범위·커서·이탈점·현금 두 점 배치. 카메라가 `ScrubArea` 전체를 보는지, 이탈점이 바닥 위 빈 곳인지 확인한다.
  - `BP_MassageChair` 고장 외형 BP event 연결, `BP_Television` 화면 켜짐·꺼짐 BP event 연결
  - `WBP_ScrubFocusHud`, `BP_BathhouseHUD.ScrubFocusHudWidgetClass`
  - Definition 네 개와 `DA_ShopCatalog` 4상품
  - DefaultMap의 때수건과 fixed slot instance(`AssignedItem`). external actor package만 저장한다.
  - 대표 시나리오 PIE 절차와 MASS·REST·SCRB 관찰 방법(키, 조준 위치, 콘솔 명령, 기대 HUD 문구). `RequiredRubDistanceCm` 결정 절차를 포함한다.
- `.md/Architecture/*`는 수정하지 않는다. 설계와 달라야 하면 멈추고 보고한다.
