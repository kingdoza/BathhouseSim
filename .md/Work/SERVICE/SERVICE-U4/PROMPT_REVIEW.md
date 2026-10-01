# 구현 리뷰 입력 — 세신 포커스 때수건 표시 복원

- 작업 ID: `SERVICE-U4`
- 단계: 구현
- 상태: 완료

## 계약과 범위

- 사용자 PIE 실패 재작업 1회차. 입력은 [PROMPT_IMPLEMENTATION_R.md](PROMPT_IMPLEMENTATION_R.md)(완료), 원인과 사용자 결정은 [버그 리포트](../../../BugReports/2026-10-01_scrub_focus_towel_visual_and_cursor_motion.md)의 현상 1이다.
- 포커스 진입부터 커서 때수건 하나만 표시한다. E/ESC, 완료·만료 자동 이탈, 손님·세신대 상실, carry 문맥 상실, 캐릭터 EndPlay에서 숨겼던 Actor의 표시를 직전 snapshot 값으로 복원한다.
- 정상 진입·이탈은 carry object/kind/attachment를 바꾸지 않는다. 사전에 hidden인 Actor는 이탈 후에도 hidden이다.
- 현상 2는 코드 수정 대상이 아니다. `AddRubInput`의 `FVector2D(Axis.Y, Axis.X)`와 `UpdateCursor`를 유지했다.
- 마스터 결정으로 기존 unity namespace 충돌 해결을 포함했다. 실제 C2872 지점의 무수식 `FFixture`만 `ServiceFacilityTest::FFixture`로 명시하며 테스트 동작·runtime·Content를 추가 변경하지 않는다.
- 추가 마스터 결정으로 SVC4-001의 네 상품 추가 계약에 맞춰 기존 BlueprintLoad의 상품 수 기대값을 16→20, 메시지를 unit-four 기준으로 갱신했다. `f15742a:.md/PROMPT_UNREAL.md`의 ProductId 네 개를 확인하고 존재 assertion만 추가했다.
- 브랜치 `work/SERVICE-U4`, 단계 시작 및 빌드 HEAD `3b0e2ca696e89a61a280a82227202427f680b435`. 커밋하지 않았다. 구현 대체 워커 사용 없음.

## 변경 파일과 책임

| 파일 | 변경 |
|---|---|
| `Source/BathhouseSim/Public/Service/PlayerScrubFocusComponent.h` | private helper 두 개, Transient weak `HiddenHeldTowel`, bool `bHiddenHeldTowelWasHidden` 추가 |
| `Source/BathhouseSim/Private/Service/PlayerScrubFocusComponent.cpp` | 승인된 진입에서 snapshot·숨김, 커서 숨김과 함께 표시 복원, 무조건 weak Reset |
| `Source/BathhouseSim/Private/Tests/ServiceAmenityScrubAutomationTests.cpp` | 기존 네 테스트 표시 assertion과 신규 snapshot·carry 상실·EndPlay 테스트 |
| `Source/BathhouseSim/Private/Tests/ServiceFacilityDisplayAutomationTests.cpp` | 세 `FFixture` 사용에 `ServiceFacilityTest::` 명시 |
| `Source/BathhouseSim/Private/Tests/ServiceFacilityPayloadAutomationTests.cpp` | 한 `FFixture` 사용에 같은 namespace 명시 |
| `Source/BathhouseSim/Private/Tests/ServiceFacilityReworkAutomationTests.cpp` | 세 `FFixture` 사용에 같은 namespace 명시 |
| `Source/BathhouseSim/Private/Tests/ServiceFacilityShopAutomationTests.cpp` | 한 `FFixture` 사용에 같은 namespace 명시 |
| `Source/BathhouseSim/Private/Tests/ServiceBlueprintLoadAutomationTests.cpp` | 상품 수 20·unit-four 메시지와 SVC4-001의 MassageChair/RestBench/Television/ScrubTable ID 존재 assertion |
| `.md/Architecture/ServiceAmenitySystem.md` | Scrub Focus Session의 표시 owner·불변식·Begin/종료·ForceCleanup 대칭 갱신 |

- `HideHeldTowel`: 유효 held Actor가 있고 저장한 weak가 비어 있을 때만 snapshot한다. 중복 호출은 기존 snapshot을 덮어쓰지 않는다.
- `RestoreHeldTowelVisibility`: 저장한 Actor가 유효할 때 직전 hidden 값으로 복원하며, Actor 유효 여부와 관계없이 weak를 Reset한다. 현재 held object를 다시 조회하지 않는다.
- Begin은 모든 진입 조건과 세신자 예약 성공 뒤, 커서 표시와 같은 전환에서 숨긴다. blend 0의 `CompleteFocusIn`보다 먼저 적용한다. 거부된 진입은 표시를 건드리지 않는다.
- RequestEnd는 커서를 숨긴 직후 복원한다. FocusingOut 시작 전이며, 유효성·이탈 위치 실패의 ForceCleanup 경로보다도 먼저다.
- ForceCleanup은 Table·이동 snapshot 유무와 관계없이 복원한다. Table이 살아 있으면 커서를 숨기고 복원한 뒤 `EndScrubSession`을 호출한다. 정상 blend 완료의 두 번째 복원은 no-op이다.
- Actor hidden만 바꾸며 collision·physics·held transform·anchor와 carry publication을 변경하지 않는다. Tick은 기존 문맥 검증만 수행하고 hidden을 재적용하지 않는다.

## 클래스 성장과 API 영향

- 컴포넌트 header 88→93줄, cpp 274→301줄. 기존 세션 snapshot·정리 책임 안의 private 표시 상태이며 독립 책임·타입·Tick·timer·delegate를 추가하지 않는다.
- 테스트 파일 343→457줄. 여러 기존 테스트에 assertion을 추가하고 응집된 비정상 종료 테스트 하나를 추가했다.
- namespace 명시만 한 네 테스트 파일은 기존 줄 수·실행·assertion을 유지한다. 총 8곳의 타입 이름만 변경했다.
- BlueprintLoad 테스트 227→235줄. 승인된 카탈로그 assertion 블록만 갱신하며 다른 asset 로드·상품 정의·가격·Widget 검증은 유지한다.
- 시스템 문서 323→325줄. 300줄 이상 분리 검토: 이번 변경은 기존 Scrub Focus Session 절 두 줄 증가에 한정되어 별도 문서 분리를 하지 않았다.
- 신규 reflected 상태는 private Transient weak 하나다. 기존 reflected 이름·상속·default subobject·Blueprint API 변경이나 삭제가 없고 Core Redirect·asset migration·재저장이 필요 없다.
- `UPlayerComputerUseComponent`, `ComputerFocusExitPlacement.*`, 공용 carry 계약과 `PhysicalCarrySystem.md`, Config, Content, Unreal 정본은 변경하지 않았다.

## 시나리오와 자동화 연결

| 테스트 (`BathhouseSim.Service.Amenity.Scrub.*`) | 검증 |
|---|---|
| `FocusClampReentryCompletionCashAndHud` | SCRB-003/004 거부 시 무변화, SCRB-005~007/011 정상·중복 진입과 이탈·재진입 표시, object/kind/attachment 불변, SCRB-008 완료 복원 |
| `ExpiryKnockdownAndUserLoss` | SCRB-009/020 만료·쓰러짐·사용자 파괴 뒤 표시 복원 |
| `CashSpawnFailureAndTableDestroyCleanup` | spawn 실패로 Active 유지 시 손 때수건 숨김·커서 표시, 세신대 파괴 즉시 복원 |
| `InputOwnershipEntryReleaseExitSearchAndTransitions` | SCRB-011/016 blend .2초 진입부터 숨김, G 차단 시 carry·숨김 유지, ESC 호출 즉시 복원·커서 숨김, blend 완료 후 visible 유지 |
| `VisibilitySnapshotCarryLossAndEndPlay` | 사전 hidden snapshot·fresh 재진입 snapshot, domain API로 비정상 carry 해제 후 원래 Actor 복원, 때수건 Destroy 후 Tick Inactive·suppression 해제, 캐릭터 Destroy 후 살아 있는 때수건 visible |

`BathhouseSim.Service.BlueprintLoad`는 SVC4-001의 카탈로그 20개와 신규 네 ProductId 존재를 검증한다. 기존 모든 상품의 ID·정의·가격 검증은 유지한다.

- 기존 `Input axes map Y->X X->Y`, `Look cursor maps input`, 게이지·입력 소유·이탈 위치·HUD assertion을 유지했다.
- 정상 G 입력 차단은 기존 입력 테스트로 검증한다. 신규 테스트의 직접 domain drop은 비정상 carry 상실을 재현하기 위한 것이며 정상 입력 동작을 바꾸지 않는다.

## 검증 결과

- 최종 UE 5.8 전체 빌드 **성공**(exit 0, DLL 링크 포함, 21.72초). Build Policy의 `Build.bat BathhouseSimEditor Win64 Development -Project=... -WaitMutex -NoHotReloadFromIDE` 명령 그대로 실행했다. 빌드 전 Editor 미실행을 확인했다.
- 빌드 시점 HEAD `3b0e2ca696e89a61a280a82227202427f680b435`, `git diff HEAD -- Source Config` 출력 SHA-256 `000f88cbfa1559b47535a2edc06aa3ec9f4a68f4a53c8c1ffb499f7f6be41f3f`. 테스트 종료 후 같은 식별값임을 확인했다.
- 파일별 hash·줄 수: `Saved/Automation/Reports/2026-10-01/SERVICE-U4-R1-Catalog/build_source_identity.json`.
- 성공 빌드 로그: `Saved/Automation/Reports/2026-10-01/SERVICE-U4-R1-Catalog/build.log`. 이전 namespace 충돌 및 상품 수 실패 기록은 [QNA_IMPLEMENTATION.md](QNA_IMPLEMENTATION.md)에 해결 과정과 함께 보존했다.
- Headless Automation Policy로 실행 완료: `/Engine/Maps/Templates/Template_Default`, `-unattended -nullrhi -NoSplash -NoSound -DDC-ForceMemoryCache`, `-ExecCmds="Automation RunTests BathhouseSim.Service+BathhouseSim.Computer+BathhouseSim.Interaction; Quit"`, `-TestExit="Automation Test Queue Empty"`.
- Automation **54/54 성공, 실패·미실행 0**, exit 0. JSON은 warning 없는 성공 49개와 warning 포함 성공 5개로 집계한다. `Service.Amenity`는 `Service`에 포함되어 중복 실행하지 않았다.
- warning은 Editor HTTP 연결 검사 실패와 기존 fixture의 수금함 없음·금액 상한·preview material 미설정 로그다. 상세 메시지는 JSON/Editor 로그에 보존했으며 assertion 실패는 없다.

| 요청 필터 | 결과 |
|---|---|
| `BathhouseSim.Service.Amenity` | 12/12 성공(신규 표시 snapshot·carry 상실·EndPlay 테스트 포함) |
| `BathhouseSim.Service` | 34/34 성공(진열·payload·rework·shop·BlueprintLoad, Amenity 12 포함) |
| `BathhouseSim.Computer` | 3/3 성공 |
| `BathhouseSim.Interaction` | 17/17 성공 |

- BlueprintLoad의 unit-two 잔존 assertion은 마스터가 확정한 unit-four 계약으로 갱신해 통과했다. 신규 상품 `MassageChair`, `RestBench`, `Television`, `ScrubTable` 존재 assertion도 통과했다. 카탈로그 Content는 수정하지 않았다.
- Automation JSON: `Saved/Automation/Reports/2026-10-01/SERVICE-U4-R1-Catalog/Automation/index.json`. Editor 전체 로그: 같은 폴더의 상위 `automation_editor.log`; 실행 stdout/stderr: `automation_console.log`.
- `git diff --check` 통과. `git diff --name-only -- Content Config` 출력 없음. 네 namespace 수정 파일은 HEAD 원문에 해당 이름 치환만 적용한 결과와 정확히 같음을 검증했다.
- 기존 사용자 파일 `.claude/settings.local.json`·`.claude/worktrees/` 및 마스터의 `CONTEXT.md`·`codex_implementation_report.md` 변경은 건드리지 않았다. 커밋하지 않았다.
- namespace와 상품 수 assertion 차단은 모두 해결됐다. 구현 단계의 빌드·Automation 완료 조건을 충족했으며 코드 리뷰 입력으로 넘긴다. 이후 단계를 직접 실행하지 않았다.
- PIE·실제 렌더 화면은 미검증이며 사용자만 수행한다. 관찰 기준은 [PROMPT_UNREAL.md](PROMPT_UNREAL.md).

## 리뷰 중점

- 숨김 기간과 커서 표시 기간의 전환 대칭, 거부·중복·즉시 blend 경로를 확인한다.
- 표시 snapshot이 현재 carry와 독립적이며 파괴된 weak와 반복 정리를 안전하게 처리하는지 확인한다.
- ForceCleanup이 Table 및 `HadSnapshot` 조건 밖에서 복원하는지, 캐릭터 component EndPlay 순서에 따라 carry가 먼저 정리되어도 표시가 복원되는지 확인한다.
- carry 상태·publication·컴퓨터 포커스·축 매핑의 diff가 없고 Content가 그대로인지 확인한다.
