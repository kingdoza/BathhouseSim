# PROMPT_REVIEW — COMPUTER-WHEEL-SCROLL 컴퓨터 화면 스크롤 영역 마우스 휠 스크롤

- 작업 ID: `COMPUTER-WHEEL-SCROLL`
- 단계: 구현
- 상태: 완료

## 기능 계약과 단계
`PROMPT_ARCHITECTURE.md`(CWS-001~020), `PROMPT_IMPLEMENTATION.md`. 단일 작업, 전체 범위. 실행 위치는 마스터 결정으로 메인 트리·브랜치 `work/COMPUTER-WHEEL-SCROLL`(시작 커밋 `5dae4d4`, BUG 병합 포함).

## 시나리오 연결
| ID | 코드 | 테스트 |
|---|---|---|
| CWS-001, 002, 004~011, 019 | `AFirstPersonCharacter::MouseWheelInput` → `ScrollPointerWheel` → 엔진 `ScrollWheel` → 엔진 `SScrollBox` | A `...WheelScrollsHoveredScrollBoxThroughSlate`(실제 UMG `UScrollBox`·`USlider`·`UButton`, 가상 창·가상 사용자, 엔진 `SScrollBox::Tick`과 `RouteMouseWheelOrGestureEvent`) |
| CWS-003, 012, 013 | `CanInjectPointerWheel` 5조건 | B `...WheelRoutesByComputerPhase` |
| CWS-016, 017, 018 | Character 분기 순서 | A(실제 사용자 focus 불변), B(커서·FocusingOut·Placement 분리·비정상 종료 복구) |
| CWS-006, 009, 010 전제, 이동량 계약 | 없음(Content) | C `...ScreenWheelContentContract`(WBP 4영역, IA, IMC, BP CDO 읽기 전용 load) |
| CWS-014, 015, 020 | 변경 없음(기존 계약) | 사용자 PIE |

## 변경 파일
- `Public/Computer/PlayerComputerUseComponent.h`, `Private/Computer/PlayerComputerUseComponent.cpp`: public `ScrollPointerWheel`, private `CanInjectPointerWheel`, friend, `UWidgetComponent` forward 선언. 새 상태·Tick·UPROPERTY·로그 없음.
- `Public/Character/FirstPersonCharacter.h`, `Private/Character/FirstPersonCharacter.cpp`: `PlacementRotateInput` → `MouseWheelInput`(분기: computer capture → 세신 소비 → Placement), friend. reflected 이름 불변.
- `Public/Placement/PlayerFacilityPlacementComponent.h`: friend 한 줄.
- `Public/Computer/BathhouseComputerActor.h`: friend 한 줄(테스트 B가 보호 멤버 접근. 설계 5.5는 이 파일을 "바꾸지 않는 것"으로 두었으나 BUG 테스트와 같은 테스트용 friend 한 줄만 추가했다. 동작 변경 없음. 리뷰 판단 요청).
- 신규 `Private/Tests/ComputerAutomationTestSupport.h`(`FScopedComputerAutomationWorld`, `BeginActorForComputerTest` 추출), `Private/Tests/ComputerAutomationTests.cpp`(추출분 using, 동작·이름 불변).
- 신규 `Private/Tests/ComputerWheelScrollAutomationTests.cpp`(세 테스트, namespace `ComputerWheelScrollTest`, virtual user 14), `Private/Tests/ComputerWheelScrollTestProbe.h`(UMG 동적 delegate 횟수 세는 test-only UCLASS).
- `.md/Architecture/ComputerSystem.md` 11행: 상태 문구만 구현 반영으로 갱신(구조는 설계 때 이미 기록됨).

## 클래스 크기·책임
`UPlayerComputerUseComponent` 487줄에서 약 520줄(주입 두 함수). Character는 분기 몇 줄 증가(함수 본문 교체). 분리 없음(설계 4.5).

## Blueprint/API/Core Redirect
reflected 추가·삭제·rename 없음. test-only UCLASS(Transient, 모듈 내부) 1개 추가. Content·Config 변경 없음.

## 검증
- 빌드: UE 5.8 `BathhouseSimEditor Win64 Development` 성공. 로그 `Saved/Logs/cws-build.log`(UTF-16).
- Source 식별값: HEAD `5dae4d4`, `git diff 5dae4d4 -- Source Config | sha256sum` = `6836e691dcf52de115bdb7ef33f370ffbbc6ec236537010c068b9107d1c67267`. 이는 추적 파일 변경분이며 신규 untracked 3개(`ComputerAutomationTestSupport.h`, `ComputerWheelScrollAutomationTests.cpp`, `ComputerWheelScrollTestProbe.h`)는 포함하지 않는다. 마스터가 커밋하면 커밋 해시로 대체한다.
- Automation(8절 필터, `-nullrhi`): 로그 `Saved/Logs/cws-auto.log`. 16개 테스트 전부 Success: 새 3개, 기존 `BathhouseSim.Computer.*`(BlueprintLoad, FocusExitPlacement, FocusSessionSuppressionAndSampleScreen, BUG의 ActiveFocusKeepsKeyboardOnGameViewport), HeldTargetUse InputOwners·BlueprintLoad, Scrub InputOwnership..., Placement 6개.
- C 결과: 4개 ScrollBox `WheelScrollMultiplier` 모두 1.000, 세로, `ConsumeMouseWheel != Never`, 애니메이션 꺼짐, Visible. IA·IMC·BP CDO 전제 성립.
- 구현 중 발견: `FindPathToWidget` 결과 경로는 `VirtualPointerPositions`가 비어 bubble 라우팅이 범위 밖 접근으로 crash한다. 테스트 helper가 엔진처럼 `FWidgetAndPointer` 배열 생성자로 경로를 만든다(production 영향 없음).
- 변경 전 코드에서는 B가 컴파일되지 않는다(신규 API). A·C는 엔진·Content 사실을 검증하므로 변경 전에도 통과 가능(정상).

## 리뷰 중점·미검증
- `PROMPT_IMPLEMENTATION.md` 11절 기준 전부.
- `BathhouseComputerActor.h` friend 한 줄 허용 여부.
- 미검증(PIE 전용): 실제 world 화면 hover 경로로의 `ScrollWheel` 주입(headless는 hit-test grid가 비어 `ScrollPointerWheel`이 false를 반환함을 B가 `AddInfo`로 기록), 실제 마우스 휠의 viewport 도달, 이동량 감각, 1 frame 한계.

## 재작업 기록
- 1회차(문서만): `PROMPT_UNREAL.md` 12행(multiplier 수치 대신 원본 위치 참조), `.md/Architecture/CharacterSystem.md` 63행(상태 문구), `.md/Architecture/ComputerSystem.md` 203행("엔진 기본 32" 제거).
- 2회차(문서만): `PROMPT_UNREAL.md` 12행("Editor 단계가 읽어 기록" 삭제, 수치 미기록·원본 위치만 기록 명시), `.md/Architecture/ComputerSystem.md` 205행(수치 기록 전제 제거), 이 절 추가.
- Source·Config·Content는 `863d964` 이후 변경 없음(문서만 변경)이라 재빌드·재Automation이 필요 없다. 위 빌드·Automation 결과와 Source 식별값이 그대로 유효하다.
