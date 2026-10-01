# PROMPT_REVIEW — 욕탕 관리 슬라이더 한계 초과·컴퓨터 클릭 없는 포커스아웃

- 작업 ID: `BUG-2026-09-25_bath_water_slider_overrun_and_computer_focus_out`
- 단계: 구현
- 상태: 완료

## 기능 계약과 단계

- 단순 버그 수정(기능 명세 생략). 계약은 `PROMPT_IMPLEMENTATION.md` 1절의 SLD-001~006, CMP-001·004·006.
- 실행 위치: 메인 작업 트리, 브랜치 `work/BUG-2026-09-25_bath_water_slider_overrun_and_computer_focus_out`(마스터 결정으로 worktree 지시 대체).

## 시나리오별 코드·테스트 연결

| ID | 코드 | 테스트 |
|---|---|---|
| SLD-001/002 | `UBathWaterDetailWidget::HandleCirculationChanged` → `ResyncSliderAfterRequest` → `WriteSliderValueSilently` | `BathWater.ManagementUI.SliderPointerDragHoldsCommittedValue`(실제 Slate RoutePointer* 경로), `SliderCommittedSyncAndFeedback` |
| SLD-003 | `HandleTargetTemperatureChanged` 동일 경로 | `SliderCommittedSyncAndFeedback`(가열 설치 한계·냉각 설치 한계, 반복 요청) |
| SLD-004 | 변경 없음(domain 규칙) | `SliderCommittedSyncAndFeedback`(운영 시계 없는 설치 전용 provider, 제한 없음) |
| SLD-005 | 같은 callback | `SliderPointerDragHoldsCommittedValue` 마지막 구간, 욕탕 B 불변 |
| SLD-006 | 변경 없음(기존 `ReleasePointerIfNeeded` + 확정값 쓰기) | 직접 테스트 없음. 손잡이가 매 이벤트 확정값이라 PIE 관찰 항목으로 둠 |
| CMP-001/004/006 | production 변경 없음 | `Computer.Input.ActiveFocusKeepsKeyboardOnGameViewport` |
| 실패 경로 | `ResyncSliderAfterRequest` 실패 분기(domain 값, 없으면 `CachedSnapshot`) | `SliderCommittedSyncAndFeedback`(욕탕 등록 해제) |
| polling 안전망 | `ApplyBathSnapshot`의 `SyncSlidersToSnapshot`이 cache gate 앞 | `SliderCommittedSyncAndFeedback`(손잡이 임의 이동 후 같은 snapshot 1회로 복구, `PresentationWriteCount` 불변) |

## 변경 파일과 요약

- `Source/BathhouseSim/Public/UI/BathWaterDetailWidget.h`, `Private/UI/BathWaterDetailWidget.cpp`: `NormalizeCirculation`/`NormalizeTargetTemperature`(파일 내부, `UBathWaterSettings`에서 읽음), `WriteSliderValueSilently`(`bWritingSliderValue` 재진입 guard, 값이 다를 때만 `SetValue`), `SyncSlidersToSnapshot`(cache gate 앞), `ResyncSliderAfterRequest`. 두 handler는 `bApplyingSnapshot || bWritingSliderValue`이면 반환하고 요청 뒤 같은 callback 안에서 해당 slider만 다시 쓴다. step size 쓰기와 `PresentationWriteCount`는 cache gate 뒤에 둠. test friend `FBathWaterSliderTestAccess`.
- `Source/BathhouseSim/BathhouseSim.Build.cs`: private 의존 `Slate`, `SlateCore`(테스트 use site: `SViewport`, `SVirtualWindow`, `FSlateApplication`, `FSceneViewport`).
- `Private/Tests/BathWaterOperationsAutomationTestSupport.h`(신규): `FScopedBathWaterOperationsWorld`·`AddProvider`를 named namespace `BathWaterOperationsTestSupport`로 이동. `BathWaterOperationsAutomationTests.cpp`는 이를 include해 `using`한다(복제 없음).
- `Private/Tests/BathWaterSliderInputAutomationTests.cpp`, `ComputerKeyboardFocusAutomationTests.cpp`(신규). helper는 named namespace(`BathWaterSliderInputTest`, `ComputerKeyboardFocusTest`)에 둠.
- `Public/Computer/BathhouseComputerActor.h`, `Public/Character/FirstPersonCharacter.h`: 테스트 friend `FComputerKeyboardFocusTest` 한 줄씩(기존 friend 패턴). production 동작 변경 없음.
- `.md/Architecture/BathWaterManagementUISystem.md` 9행: "Source 미반영" 표기를 "Source 반영, 사용자 PIE 대기"로 갱신. 구조·API 변경이 없어 다른 Architecture 정본은 갱신하지 않음.

## 클래스 크기·책임

- `UBathWaterDetailWidget`: cpp 269 → 약 335줄, 책임 추가 없음(손잡이를 domain 확정값과 일치시키는 표시 동기화). 경고선 아래, 새 타입 없음. reflected 변경 없음.
- 조정값: 새 수치 상수 없음. 온도 범위는 `UBathWaterSettings`(Min/Max/Step)에서 읽는다. 테스트도 용량·온도 기대값을 condition getter(`GetMaxCirculationDemandPoints`, `GetHeatingDemandPointsPerC`, `GetCoolingDemandPointsPerC`)와 settings(`GetAmbientTemperatureC` 등)에서 계산한다. 테스트 리터럴은 fixture 비율(설치 60%, 여유 10/5°C, 슬라이더 위치 비율)과 허용오차뿐이며 튜닝값이 아니다.

## Blueprint/API/Core Redirect 영향

- 없음. Content 변경 없음(`PROMPT_UNREAL.md`).

## 빌드와 검증

- 빌드: `BathhouseSimEditor Win64 Development`, 성공(exit 0). 마지막 빌드 로그 `C:/Users/kdowo/AppData/Local/Temp/claude/C--UnrealProjects-BathhouseSim/fe09b558-f6ab-4905-a7e9-d23d125f4a9f/scratchpad/build8.log`.
- Source 식별값: HEAD `cd47d47b84d7aab67e36997cf9989d1640fc781f`, `git diff HEAD -- Source Config`와 untracked `Source`/`Config` 파일 내용을 이어 붙인 SHA-256 `97090aa8296dc0e5cd180dc0bed31705e24b120a6d6509664d73241157130ef2`(마지막 빌드·Automation 직후 계산, 이후 Source 변경 없음).
- Automation(필터 `BathhouseSim.BathWater+BathhouseSim.Computer+BathhouseSim.Interaction.HeldTargetUse.InputOwners`, headless): 새 3개 포함 16개 전부 Success, Fail 0. 로그 `Saved/Logs/BathhouseSim.log`(마지막 실행), 리포트 경로 `Saved/Automation/Reports/2026-10-01/bug-0925`.
- 변경 전 실패 확인: `ResyncSliderAfterRequest`를 임시로 무력화(수정 전 동작과 동일)하고 `BathhouseSim.BathWater.ManagementUI`를 실행했다. `SliderPointerDragHoldsCommittedValue`는 "계속 끌기 시 손잡이가 한계 유지"와 "놓은 뒤 확정값 위치"에서, `SliderCommittedSyncAndFeedback`는 두 번째 한계 초과·반복 가열/냉각·실패 경로에서 실패했다. 이후 원복해 위 최종 결과를 얻었다.
- `git diff --check`는 LF 경고(저장소 autocrlf) 외 없음. Content 변경 0건.

## 리뷰 중점·전역 영향·미검증

- 중점: 같은 callback 안 확정값 쓰기, guard의 재요청 차단, polling 동기화가 cache gate 앞, 피드백 유지·해제 규칙 불변, 다른 욕탕 불변, 컴퓨터 production 경로 불변과 음성 대조(`ActiveFocusKeepsKeyboardOnGameViewport` 1단계), Build.cs 의존이 테스트 use site와 일치.
- 전역 영향: Build.cs 의존 2개뿐(Engine이 이미 public 의존하는 모듈).
- 테스트 harness 메모:
  - pointer 테스트는 `FindPathToWidget`의 cached geometry가 headless에서 비어 있어 `FWeakWidgetPath::ToWidgetPath`로 재배치한 path를 쓴다(window prepass 기준 실제 layout). hover move를 먼저 보낸다(`UWidgetInteractionComponent`와 같음).
  - `FPointerEvent`는 user index 생성자를 쓴다(device id 생성자는 `ApplicationCore` 링크가 필요).
  - game viewport harness는 `FSceneViewport`를 widget만으로 만들고 `UGameViewportClient::Viewport`와 test world context의 `GameViewport`에만 연결한다(`AddAssociation`과 전역 viewport 미사용).
- 미검증: 실제 키보드 이벤트와 `ProcessLocalPlayerSlateOperations`(전역 game viewport 필요), SLD-006의 직접 테스트, 사용자 PIE 전부. 실제 CMP-001 확인은 PIE 항목이다.
- 테스트 한계: SLD-001~005 슬라이더 테스트는 실제 Slate 드래그 경로를 쓰지만 렌더·paint는 없다.
