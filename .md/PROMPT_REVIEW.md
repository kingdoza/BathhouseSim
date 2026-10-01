# 코드 리뷰 프롬프트 — 서비스 4단위

## 단계와 입력

- C++ 구현 완료, Editor 작업 전 코드 리뷰다. `.md/AGENT_WORKFLOW.md` → `.md/AGENT_REVIEW.md` 순서로 읽는다.
- 승인 기능: `.md/PROMPT_ARCHITECTURE.md` MASS-001~015, REST-001~006, SCRB-001~020, SVC4-001~003(44개).
- 구현 입력: `.md/PROMPT_IMPLEMENTATION.md`. 정본: `.md/Architecture/ServiceAmenitySystem.md` 전체와 관련 Combat·Computer·HeldTargetUse·CleaningLitter·Placement·Service·ServiceFacilityDisplay·CustomerRecovery·UI·Core 절.
- 현재 구현 프롬프트는 서비스 4단위다. 과거 3단위 `PROMPT_IMPLEMENTATION_R.md`를 이번 변경의 기준으로 쓰지 않는다.
- 다음 Editor 계약은 `.md/PROMPT_UNREAL.md`. 코드 리뷰 승인 뒤 별도 Editor 단계에서 실행한다.
- 입력 프롬프트·Architecture·Unreal 정본은 수정하지 않았다. 설계의 예정 구조를 구현했고 이번 프롬프트의 Architecture 읽기 전용 지시를 따랐다. 정본 Status의 ‘Source 미반영’ 문구도 그대로다.

## 변경 파일과 책임

경로는 `Source/BathhouseSim/` 기준이다. 기존 12개 파일 수정, 신규 29개 파일 추가다.

| 영역 | 파일 | 구현 |
|---|---|---|
| 공통 | `Public/Facility/BathhouseFacilityTypes.h`, `Public/Interaction/PhysicalCarryable.h` | enum 끝에 네 설비와 ScrubTowel append |
| 회수 수행자 | `Public/Placement/PlaceableFacility.h`, `Public/Facility/BathhouseFacilityActor.h`, `Private/Placement/PlayerFacilityPlacementComponent.cpp` | 기본 no-op virtual, base Transient weak, hold 전 owner 전달·실패/취소 clear |
| Q67 | `Private/Interaction/PlayerEquipmentUseComponent.cpp` | 비대상 장비 Apply 이유만 보존·Take 삭제, 이유 실패 1회 보고 |
| 안마의자 | `Public/Service/MassageChairActor.h`, `Private/Service/MassageChairActor.cpp`, `Public/Service/MassageChairPlacementInstanceData.h`, `Private/Service/MassageChairPlacementInstanceData.cpp` | 이용 timer·동전·고장·수금·payload·회수 publication 지급 |
| 렌치 | `Public/Combat/MonkeyWrenchActor.h`, `Private/Combat/MonkeyWrenchActor.cpp`, 신규 `Public/Combat/WrenchRepairable.h`, `Private/Combat/WrenchRepairSession.h/.cpp` | 수리 interface·private helper·장비 Hold 분기 |
| TV | `Public/Service/TelevisionActor.h`, `Private/Service/TelevisionActor.cpp` | 빈손 E 전원, transient·재설치 꺼짐 |
| 때수건 | `Public/Service/ScrubTowelActor.h`, `Private/Service/ScrubTowelActor.cpp` | carry/fixed slot/drop/recovery 전용, 장비·버리기 interface 없음 |
| 세신 | `Public/Service/ScrubTableActor.h`, `Private/Service/ScrubTableActor.cpp`, `Public/Service/PlayerScrubFocusComponent.h`, `Private/Service/PlayerScrubFocusComponent.cpp` | 설비 대기·게이지·사용자·현금, 플레이어 포커스·입력·커서·이탈 |
| 사용자 대역 | `Public/Service/ServiceAmenityTypes.h`, `Public/Service/ServiceTestUserActor.h`, `Private/Service/ServiceTestUserActor.cpp`, `Private/Service/ServiceAmenityDebugCommands.cpp` | C++ 사용자 계약, 완료/만료/현금 수금 후 제거, non-shipping 명령 4개 |
| 입력 | `Public/Character/FirstPersonCharacter.h`, `Private/Character/FirstPersonCharacter.cpp` | PlayerScrubFocus 조립, IsFocusCapturingInput, E/ESC/Look/LMB owner Scrub |
| HUD | `Public/UI/BathhouseHUD.h`, `Private/UI/BathhouseHUD.cpp`, `Public/UI/ScrubFocusHudWidget.h`, `Private/UI/ScrubFocusHudWidget.cpp` | widget class·pawn 주입·EndPlay 해제, Active 게이지·대기 표시 |
| 새 테스트 지원 | `Private/Tests/ServiceAmenityAutomationTestProbe.h/.cpp`, `ServiceAmenityAutomationTestSupport.h` | construction slot fixture, 실제 conversion·BeginPlay·HUD probe |
| 새 테스트 | `Private/Tests/ServiceAmenityChairAutomationTests.cpp`, `ServiceAmenityRestAutomationTests.cpp`, `ServiceAmenityScrubAutomationTests.cpp`, `EquipmentMergeQ67AutomationTests.cpp`, `ServiceAmenityBlueprintLoadAutomationTests.cpp` | 신규 12개 자동화 |

평상·상점에는 새 runtime 로직이 없다. 평상은 기존 base+3slot이며 Definition/catalog는 Editor 단계다. Customer·StateTree·module dependency·Config 변경은 없다.

## 시나리오 추적

테스트 경로 접두어는 별도 표기 없으면 `BathhouseSim.Service.Amenity.`다.

| 시나리오 ID | 코드 경로 | 자동화 / Editor 검증 |
|---|---|---|
| MASS-001,003,004,013 | MassageChair HandleSlotChanged/CompleteUse/ShouldBreak/IsAvailableForReservation | `Chair.TimerCollectionAndKnockdown`: 60초·3,000·0/100 확률·중간 threshold |
| MASS-002,010,014 | MassageChair Query/ExecuteInteraction | 같은 테스트: 0원 이유·물건 든 수금·반복 E·Controller wallet |
| MASS-005~007 | WrenchRepairSession + MonkeyWrench Query/Begin/Update/End/Cancel | `Chair.WrenchRepairCancelAimLossAndAttack`: 3초·release/cancel/aim loss·수리 무타격·정상 공격 |
| MASS-008,009 | MassageChair payload/StagePlacedDomainUnregistration + 기존 conversion | `Chair.RecoveryPaymentRollbackAndBrokenPayload`: 수행자 지갑+3,000·원본0·Q 취소·wallet 없는 stage 실패·Destroy 실패 rollback·고장 재설치·요약·0원 재회수; Timer 테스트 이용 중 회수 거부 |
| MASS-011 | chair 사용자 OnEndPlay·slot 전이 | Timer/Repair: Occupied→Reserved 중단·무요금·60초 재시작·Occupied/Reserved 사용자 Destroy |
| MASS-012,015, SCRB-019 | chair/Litter query + Equipment Merge | `BathhouseSim.Interaction.Equipment.Q67ReasonOnlyApplyAndOwnRows`: 빈손/박스/집게/때수건 이유, 집게 무행동·실패 intent 1회·강조/뚜껑 off·쓰레기 줍기·별도 RMB·기존 대걸레/렌치/배송 행·Facility 배치 LMB |
| REST-001~003,006 | Television Query/Execute + 기존 conversion | `Rest.TelevisionReinstallAndBenchSlots`: 빈손 toggle·held 이유·Q 취소 유지·slot0 회수/재설치 꺼짐 |
| REST-004,005 | base slot + 기존 회수 | 같은 테스트: 3석·네 번째 거부·회수 거부·Reserved/재사용·비운 뒤 회수 |
| SCRB-001~004 | ScrubTable Query/Execute, Focus Begin | `Scrub.FocusClampReentryCompletionCashAndHud`: towel/no-user 이유·대기/gauge·E 진입·세신자 중복 거부 |
| SCRB-005~007 | Focus AddRubInput/RequestEnd + Table AddRubDistance | 같은 테스트: LMB 없는 이동 무증가·축 변환·clamp 뒤 실제 거리·경계 무증가·이탈/재진입 유지·때수건 보존 |
| SCRB-008,014,015 | Table CompleteScrub + ServiceTestUser + 기존 Cash | 같은 테스트: 자동 이탈·slot 해제·인형 이동·현금 1개·다음 사용자·설비 제거 뒤 현금·E 한 번+20,000·인형 제거 |
| SCRB-009,012,018,020 | Table WaitTimer/slot/user lifecycle | `Scrub.ExpiryKnockdownAndUserLoss`: 만료 무현금·자동 이탈·Reserved/gauge0·90초 재시작·사용자 상실; Focus 테스트 누움 중 회수 거부 |
| SCRB-010,011,016 | Character 입력 + Focus + 기존 ComputerFocusExitPlacement | `Scrub.InputOwnershipEntryReleaseExitSearchAndTransitions`: entry E release 소비·blend 중 차단·이동/점프/G/Q/RMB·Look/LMB·ESC·중복 E·막힌 이탈점 근처 탐색 |
| SCRB-013,017 | ScrubTowel + 기존 fixed slot/carry | `Scrub.TowelExactSlotDropRecoveryAndNoDiscard`: exact slot·wrong-slot·held 위치 drop·physics/CCD/Pawn ignore·복구·slot 소멸·장비/버리기 interface 없음 |
| 세신 실패 정리 | Table CompleteScrub, Focus ForceCleanup | `Scrub.CashSpawnFailureAndTableDestroyCleanup`: 현금 spawn 실패 시 미완료·설비 Destroy 즉시 입력/시점/이동 복구·무 teleport |
| SVC4-001 | 기존 Shop/Placement 경로 + Editor Definition/catalog | Shop 테스트에서 4종 Definition 배송/개봉; 실제 가격·카탈로그 화면은 `PROMPT_UNREAL.md` PIE |
| SVC4-002,003 | 기존 Delivery/Discard/Collection + ServiceAmenityDebugCommands | `Shop.FourDefinitionsUnboxWorldDiscardAndDebug`: 4종 개봉·world discard·60초 수거 제거·네 명령 등록. 소스 전체 `#if !UE_BUILD_SHIPPING`; Shipping 실행 미검증 |
| 호환성 로드 | 기존 BP parent/default subobject/property | `BlueprintLoad`: 네 BP parent·Character 새/기존 component·HUD property·Wrench attack·Shower 2slot/2space/manager·package clean |

기존 테스트는 고치지 않았다. Computer·Combat·Interaction held-use·Cleaning 및 3단위 cue/생성 clearance·Placement·Facility·Service·Shop 포함 전체 회귀로 확인했다. 신규 설비 설치·회수·재설치는 실제 `FFacilityActorConversionTransaction`을 사용한다. slot은 fixture OnConstruction 중 만들고 test world에서 BeginPlay를 명시적으로 수행한다.

## 클래스 성장과 경계

줄 수는 구현 시작 시 파일 복사본과 현재 파일의 물리 줄 수다(header/cpp).

| 기존 class/file | 변경 전 → 후 | 책임 |
|---|---|---|
| MonkeyWrenchActor | h102→113, cpp375→422 | 수리 helper 소유·분기·BP 표현 event·해제; timer 없음 |
| FirstPersonCharacter | h215→231, cpp557→604 | 조립/입력 전달만. 세신 상태·timer·cursor·이탈은 새 component |
| PlayerEquipmentUseComponent.cpp | 338→358 | 기존 query 합성/실패 보고만 |
| PlayerFacilityPlacementComponent.cpp | 415→418 | 수행자 전달/clear 3줄만 |
| BathhouseFacilityActor | h176→188, cpp405→405 | 수행자 weak/getter/setter만; amenity 분기 없음 |
| BathhouseHUD | h49→56, cpp132→157 | 기존 widget 생성/주입/해제 패턴 확장 |

- **렌치 cpp 400줄 목표를 22줄 초과한다.** 수리 target/elapsed/commit은 독립 helper h29/cpp55에 분리했고 기존 carry/attack 본문을 보존했다. 단순 LOC 감소 wrapper는 만들지 않았다. 리뷰에서 분기·정리 책임의 성장 여부를 확인한다.
- 신규: MassageChair h85/cpp306, ScrubTable h119/cpp284, PlayerScrubFocus h88/cpp274, ScrubTowel h120/cpp309, Television h27/cpp38, ServiceTestUser h30/cpp44, ScrubFocusHud h29/cpp40.
- Chair는 이용 timer·현재 사용자 delegate를, Table은 대기 timer·사용자·Native OnScrubSessionEnded를 소유한다. Focus는 blend timer·Tick·table delegate·시점/이동 snapshot을 소유한다. 구독/해제는 EndPlay와 실패 정리까지 대칭이다.
- Character에는 새 PlayerScrubFocus UPROPERTY/getter, Scrub press owner, E press 소비 flag만 추가했다. HUD에는 class/runtime widget UPROPERTY 2개, base에는 Transient weak 1개다.
- ComputerFocusExitPlacement helper를 그대로 호출한다. 컴퓨터/세신 공통 부모를 만들지 않았다. Combat는 Service 구체 class를 참조하지 않고 IWrenchRepairable만 안다.

## Blueprint/API와 serialized 호환성

- 신규 enum 항목은 append했다. 기존 reflected class/property/subobject 이름·부모·Serialize를 바꾸거나 Core Redirect를 추가하지 않았다.
- Character에 `PlayerScrubFocus` native default subobject가 추가됐다. base의 수행자 필드는 Transient weak다.
- 신규 BP event: chair OnBrokenStateChanged/OnCoinBalanceChanged, TV OnPowerChanged, 렌치 OnRepairActiveChanged. 표현만 Editor graph에 둔다.
- HUD `ScrubFocusHudWidgetClass` 지정이 필요하다. BindWidget은 `ScrubGaugeBar`(ProgressBar), `ScrubWaitText`(TextBlock). host는 tick을 유지하고 Active 밖 RenderOpacity0으로 지운다.
- 고장 재설치 외형은 BP BeginPlay에서 IsBroken을 읽어 초기화하고 event로 갱신한다. fee·timer·지급·입력·게이지·domain 로직을 graph에 넣지 않는다.
- 기존 export 4개 백업: `Saved/MigrationBackup/20261001_service_unit4/`.

## 빌드·로드·정적 검사

- UE 5.8 `Build.bat BathhouseSimEditor Win64 Development -WaitMutex -NoHotReloadFromIDE` 성공. 최종 로그 `Saved/ImplementationUnit4/build_11.log`.
- copy-first 순서:
  1. 지정 4개를 `Content/Developers/MigrationCheck/`에 파일 복사. Template map + `-ServiceAmenityCopies`의 BlueprintLoad **1/1 통과**. `copy_load.log`, `copy_report/index.json`.
  2. 복사본 SHA256 확인 후 정확히 네 파일과 빈 MigrationCheck 폴더만 제거. Content/Config SHA256 동일.
  3. Template map 원본 BlueprintLoad **1/1 통과**. `original_load.log`, `original_report/index.json`.
  4. DefaultMap 원본 + 전체 BathhouseSim 회귀 성공. `full_04.log`, `full_report_04/index.json`.
  5. Content/Config/Level SHA256 재확인 동일. 네 package 비저장·clean, native parent/기존 component 유지.
- 위 로그/report 경로 접두어는 `Saved/ImplementationUnit4/`다. 모든 headless 실행은 `-DDC-ForceMemoryCache`를 사용했다. asset Serial size mismatch/Linker Failed to load 없음.
- `git diff --check` 통과. protected Computer·Interaction·Carry·HeldTargetUse 파일, 기존 테스트 전체와 입력/정본 문서는 시작 SHA256 기준 보존 확인했다.
- Source 변경은 승인 경로 안 41개 파일이다. Config/module/customer 변경 없음. 디버그 명령 파일 전체가 `#if !UE_BUILD_SHIPPING`, 자동화는 WITH_DEV_AUTOMATION_TESTS guard다.
- 진행 중 `ORCHESTRATOR_WORKFLOW_PROPOSAL.md`의 외부 변경을 감지해 그대로 보존했다. 이 파일은 구현 산출물이 아니다.

## 자동화 결과와 중간 실패

- 명령: `Automation RunTests BathhouseSim`, DefaultMap, nullrhi, memory DDC. 최종 프로세스 exit 0.
- **총 130 / 성공 121 / 경고 동반 성공 9 / 실패 0 / 미실행 0.** 신규 12개는 전부 성공(경고 없음), 기존 118개도 전부 성공.
- 경고 9개는 기존 Placement invalid authoring/startup pending, Service 판매/preview 설정 fixture, Towel presentation invalid slot/payload, Utility mobility/invalid provider 검증에서 나왔다. 새 기능의 실패를 무관 판정해 제외한 것은 없다.
- `full_01`: 새 테스트 사용자 BeginPlay/첫 timer tick 경계 누락으로 의자/Q67 실패 후 null 접근 중단. fixture BeginPlay·tick 여유·null guard 수정.
- `full_02`: 새 평상 재설치 fixture BeginPlay 누락으로 slot cache0 접근 중단. BeginPlay·slot/user 검사 수정.
- `full_03`: 129/130 성공, Shop 수거 테스트 1개 실패. 빈 test world의 free-world 아이템 낙하를 막는 실제 충돌 바닥을 fixture에 추가.
- `full_04`: 위 보완 후 130/130 성공. runtime 규칙/기존 테스트 기대값을 완화하지 않았다. 중간 실패 로그도 Saved에 보존했다.

## 리뷰 중점과 미검증

- 회수 stage 실패는 이미 성공한 Super stage를 자체 rollback해야 한다(상위 transaction은 실패한 stage를 rollback하지 않음). callback은 commit 후 1회만 지급하고 기존 publication을 이어 실행한다. cancel/rollback/0원·고장 payload를 특히 확인한다.
- 수금의 fresh 검증·재진입 guard, 사용자 Destroy와 slot EndUse/Release 순서, 현금 spawn 실패의 미완료 상태를 확인한다.
- 세신자가 focus-out을 끝낼 때까지 Table 예약을 유지한다. 이전 세션 정리가 새 세션 cursor를 숨기지 않도록 component가 예약을 해제한다.
- E Started가 새 세신 진입을 만들면 release도 세신 press로 소비한다. LMB owner·컴퓨터 우선순위와 Q67 무행동 이유 보고가 기존 입력을 바꾸지 않는지 확인한다.
- 기존 자산 비저장 로드는 확인했지만 새 BP authoring/Compile/Save/fresh reload, 실제 PIE 시점·collision/Navigation·고장 외형·TV 화면·HUD 배치·마우스 감도·상품 가격·external actor 연결은 수행하지 않았다.
- Shipping 빌드/실행은 하지 않았다. guard 정적 확인만 완료다.
- `.md/PROMPT_UNREAL.md`에 여섯 BP·WBP·네 Definition·catalog·때수건/fixed slot external package allowlist, authoring 값, 재로드·PIE·거리 결정 절차와 Unreal 정본 갱신 대상을 인계했다.
- 리뷰가 실패하면 `PROMPT_IMPLEMENTATION_R.md`를 작성해 구현 단계로 돌려보낸다. Source/Content를 리뷰 단계에서 직접 고치지 않는다.
