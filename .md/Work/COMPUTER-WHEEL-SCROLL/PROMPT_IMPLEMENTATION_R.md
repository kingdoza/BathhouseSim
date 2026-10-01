# PROMPT_IMPLEMENTATION_R — COMPUTER-WHEEL-SCROLL 코드 리뷰 1회차 재작업

- 작업 ID: `COMPUTER-WHEEL-SCROLL`
- 단계: 코드 리뷰
- 상태: 완료
- 출처: 코드 리뷰 1회차

## 리뷰 대상과 결론

- 범위: `git diff 5dae4d4 863d964`(구현 커밋 `863d964`). BUG 작업 변경은 범위 밖.
- Source 판정: 통과. 지적 사항 없음. production 분기·gate(`MouseWheelInput`, `ScrollPointerWheel`, `CanInjectPointerWheel` 5조건), 금지 우회 부재, reflected 이름 불변, fixture 추출(복제 없음), test-only UCLASS(기존 `*TestProbe.h` 선례), `BathhouseComputerActor.h` friend 한 줄(보호 멤버 `ComputerMesh`·`FocusBlendIn/OutSeconds`·`FocusExitPoint` 접근용, 동작 불변)을 모두 허용한다.
- 빌드·Automation: 커밋 `863d964`의 tracked 변경 해시가 `PROMPT_REVIEW.md` 식별값과 같고, 신규 3파일 mtime이 빌드 시작(23:27:58경, `Saved/Logs/cws-build.log` 23:28:05 종료) 이전이며 작업 트리가 깨끗하다. `Saved/Logs/cws-auto.log` 16개 Success, EXIT 0. 재빌드 불필요.
- 재작업 사유: Editor 단계 입력(`PROMPT_UNREAL.md`)이 조정값 원본 원칙을 어기는 정본 기록을 지시한다(F1). 문서만 고친다.

## Findings

### F1 (중요, 수정 필수) `PROMPT_UNREAL.md` 2번이 Unreal 정본에 조정값 수치 기록을 지시함

- 위치: `PROMPT_UNREAL.md` "갱신할 Unreal 정본" 2번의 "`WheelScrollMultiplier` 1.0(자동화 C가 Content에서 확인함)".
- 근거: `AGENT_WORKFLOW.md` 조정값 원본 원칙. 문서(Architecture/Unreal 정본)는 수치를 복제하지 않고 원본 위치(클래스·프로퍼티, asset·프로퍼티)를 참조한다. 이 원칙은 정본 값 기록 규칙보다 우선한다. `WheelScrollMultiplier`는 PIE 뒤 영역별로 조정하는 감각 값(`PROMPT_ARCHITECTURE.md` 4.6)이므로 정본에 1.0을 적으면 조정 때마다 두 곳이 어긋난다. 설계 9절 2번이 1.0·32를 적었지만 원칙이 우선하며, 구현은 이미 32를 뺐으므로 같은 기준으로 1.0도 뺀다.
- 수정 방향:
  - 2번을 "영역별 이동량의 원본은 각 `UScrollBox`의 `WheelScrollMultiplier`(위 4개 WBP의 해당 ScrollBox). 한 칸 = 엔진 cvar `Slate.GlobalScrollAmount` × multiplier × 휠 양, ScrollBox local unit"처럼 원본 위치와 계산식만 남긴다. 현재 값은 자동화 C의 `AddInfo` 로그로 확인한다고 적어도 된다.
  - `ConsumeMouseWheel=WhenScrollingPossible`(또는 `!= Never`), `AnimateWheelScrolling=false`는 수치 감각값이 아니라 동작 계약 설정이므로 그대로 둔다.
  - Editor 단계가 정본에 수치를 쓰지 않도록 "multiplier·cvar의 현재 수치는 정본에 적지 않는다"를 2번에 한 줄로 명시한다.

### F2 (낮음, 수정 필수) `CharacterSystem.md` 상태 문구가 Source 반영과 어긋남

- 위치: `.md/Architecture/CharacterSystem.md` 63행 끝 "(2026-10-01 `COMPUTER-WHEEL-SCROLL` 설계, Source 미반영)".
- 근거: 구현이 같은 내용의 `ComputerSystem.md` 11행 상태는 "구현(Source 반영, 자동화 통과, 사용자 PIE 대기)"로 갱신했다. 같은 작업의 정본 두 곳이 다른 상태를 말한다.
- 수정 방향: 63행 상태 괄호만 `ComputerSystem.md` 11행과 같은 상태로 맞춘다. 다른 문장은 바꾸지 않는다.

### F3 (참고, 선택) 테스트 B 단언 라벨의 시나리오 ID

- 위치: `ComputerWheelScrollAutomationTests.cpp` 596·597행 "CWS-018: Active wheel ...". CWS-018은 "컴퓨터 미사용·빈손 모니터 휠"이다. Active 중 Placement 분리는 4.2 분리 규칙(설계 7.2 5번)이다.
- 판정에 영향 없음. 고치면 Source가 바뀌므로 8절 빌드와 Automation 필터를 다시 돌리고 새 식별값을 `PROMPT_REVIEW.md`에 적는다. 고치지 않으면 Source·빌드는 그대로 둔다.

## 바꾸지 않는 것

- Source(F3를 선택하지 않는 한), Content, Config, `PROMPT_IMPLEMENTATION.md`(아키텍처 소유).
- `PROMPT_UNREAL.md`의 1·3번, PIE 관찰 항목, Blueprint 금지 절.
- `ComputerSystem.md` 203행의 "(엔진 기본 32)"는 아키텍처 소유 문장이다. 이 재작업에서 고치지 않으며 리뷰가 마스터에게 별도로 보고한다.

## 재검증 조건

- `PROMPT_UNREAL.md` 2번에 `WheelScrollMultiplier`·`Slate.GlobalScrollAmount`의 수치가 없고 원본 위치·계산식·"수치 미기록" 지시가 있다.
- `CharacterSystem.md` 63행 상태가 `ComputerSystem.md` 11행과 같다.
- `git status`에 `Content/`, `Config/` 변경이 없다. F3를 고치지 않았으면 `git diff 863d964 -- Source`가 비어 있다. 고쳤으면 빌드 성공·Automation 필터 전부 Success와 새 식별값이 있다.
- `PROMPT_REVIEW.md`에 이번 재작업 범위(F1·F2, F3 여부)를 한두 줄 추가한다.
- 재검증은 같은 리뷰어가 위 항목과 새 diff만 본다.
