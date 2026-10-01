# 오케스트레이션 제안서 채택 대조 결과

- 대상: `ORCHESTRATOR_WORKFLOW_PROPOSAL.md`, `QNA_ORCHESTRATOR_WORKFLOW_PROPOSAL.md`
- 대조 문서: `AGENTS.md`, `.md/AGENT_*.md`, `.md/FEEDBACK_POLICY.md`, `.md/FEEDBACK_BACKLOG.md`, `.md/USER_UNREAL.md`, `.claude/skills/*/SKILL.md`, `.claude/agents/*.md`, `~/.codex/config.toml`
- 작성일: 2026-10-01
- 상태: 검토 결과만 기록한다. 아직 어떤 문서도 수정하지 않았다.

## 요약

제안서의 큰 구조는 대부분 반영돼 있다. 마스터 Skill, 작업 폴더와 결과물 첫머리, 구현·코드 리뷰 루프, 실행 도구 분담, 구현 대체, PIE 인계, 버그 리포트·복귀 기록(RET)·Feedback 정책이 여기에 해당한다. 다만 실제로 돌려 보면 막힐 누락 4건과 덜 급한 누락·잔재가 있고, 제안서 자체에도 빠진 운용 공백이 있다.

| 구분 | 건수 | 우선순위 |
|---|---|---|
| 1. 실제 운용에서 바로 막힐 누락 | 4 | 높음 |
| 2. 반영이 덜 된 항목 | 8 | 중간 |
| 3. 잔재와 불일치 | 5 | 낮음 |
| 4. 제안서에도 없던 보완점 | 6 | 중간 |
| 5. 의도한 변경인지 확인할 것 | 2 | 사용자 확인 |

## 1. 실제 운용에서 바로 막힐 누락

### A. 워커 정의 파일에 도구 제한이 없다

- 제안서: 정의 파일에서 `tools`·`mcpServers`로 역할에 필요한 도구만 연다. 예: 코드 리뷰는 수정 도구 제외, MCP는 Unreal Editor 역할만 (제안서 138행).
- 현재: `.claude/agents/*.md` 여섯 개 모두 `tools` 지정이 없어 모든 도구를 받는다. 회고·코드 리뷰 워커도 Computer Use와 Edit를 쓸 수 있다.
- 주의: 제안서의 "코드 리뷰는 수정 도구 제외"는 코드 리뷰가 `PROMPT_IMPLEMENTATION_R.md`와 `QNA_REVIEW.md`를 쓰는 소유권과 충돌한다. `tools`로는 경로 단위 제한을 할 수 없다. 그래서 다음과 같이 정리해야 한다.
  - Computer Use MCP는 Unreal Editor 역할에만 둔다.
  - 회고는 읽기 전용 도구만 둔다.
  - 코드 리뷰는 Write는 허용하고 Source 수정 금지는 규칙에 맡기거나 hook으로 막는다.

### B. 에이전트를 재개할 수 없을 때의 대체 규칙이 없다

- 제안서: 실행 도구가 재개를 지원하지 않으면 작업 폴더와 `CONTEXT.md`로 새 에이전트에 인계한다 (제안서 292행).
- 현재: [AGENT_ORCHESTRATOR.md:67-71](../../.md/AGENT_ORCHESTRATOR.md)의 "재개와 신규"에 이 대체 규칙이 없다.
- 영향: Claude 하위 에이전트 ID는 세션 안에서만 유효하다. 작업마다 새 세션을 열기로 했으므로(Q7), 세션이 바뀌면 "원래 리뷰어 재개", "자기 구현의 후속 수정" 규칙을 지킬 수 없다. `CONTEXT.md`의 "워커 세션 ID"도 Codex에만 실효가 있다.

### C. 즉시 피드백 패킷 형식이 빠졌다

- 제안서 "즉시 피드백 형식" 절의 패킷 항목:
  - 버그 리포트 경로, 작업·시나리오 ID
  - 승인된 계약과 실제 관찰·재현 조건
  - 로그·PIE·코드·asset 근거
  - 유입 단계와 검출 실패 단계
  - 확정 원인 또는 추가 조사 범위
  - 요청 수정과 **금지할 임시 우회**
  - **회귀 검증 조건과 복귀 지점**
- 현재: [AGENT_ORCHESTRATOR.md:75](../../.md/AGENT_ORCHESTRATOR.md)의 인계 패킷에는 금지 범위만 있다. 재작업 지시용 패킷은 정의돼 있지 않다.

### D. 진행 중인 작업이 작업 폴더로 옮겨지지 않았다

- 제안서: 진행 중인 결과물을 작업별 폴더로 옮긴다 (채택 후속 작업 4번).
- 현재: `.md/Work/` 폴더가 없다. 그런데 [USER_UNREAL.md](../../.md/USER_UNREAL.md)에는 서비스 2·3·4단위가 "PIE 수용 대기"로 남아 있다.
  - 이 작업들은 `/orchestrate <작업 ID> 이어서`로 재개할 방법이 없다.
  - `.md/BugReports/`의 2026-10-01 버그 두 건(집게 오강조, 세신 포커스)도 대응하는 작업 폴더가 없다.
- [AGENT_WORKFLOW.md:149](../../.md/AGENT_WORKFLOW.md)는 "해당 작업을 재개할 때 `PIE_CHECKLIST.md`로 옮긴다"고 하지만, 재개할 작업 ID와 `CONTEXT.md`가 먼저 있어야 한다.

## 2. 반영이 덜 된 항목

| 항목 | 제안서 | 현재 |
|---|---|---|
| `CONTEXT.md` 항목 목록 | 결과물 첫머리 또는 `CONTEXT.md`에 단계 시작 커밋; 복귀 대상·변경 범위·유지되는 완료 범위를 `CONTEXT.md`에 (389행) | [AGENT_WORKFLOW.md:53](../../.md/AGENT_WORKFLOW.md)는 항목을 정하고 "…만 둔다"고 한다. 그런데 다른 규칙이 목록에 없는 값을 적으라고 한다: 단계 시작 커밋([AGENT_ORCHESTRATOR.md:101](../../.md/AGENT_ORCHESTRATOR.md)), 사용자 지시 모델 덮어쓰기(:63), 작업 브랜치. 복귀 범위 기록 규칙은 아예 빠졌다. |
| 역할별 요구 수준 표 | 추론 수준·에이전트 연속성·필요한 실행 능력 표, "난이도에 따른 자동 추론 상·하향 금지", 요구 수준 변경 절차 | 정본에 없다. 모델 연결(정의 파일)만 있고, 그 연결이 맞춰야 할 요구 수준 기록이 없다. |
| 모델 이름 미기록 원칙 | 원칙 7, 128행: 정책 문서는 모델 이름을 기록하지 않는다 | [AGENT_ORCHESTRATOR.md:9,56,58](../../.md/AGENT_ORCHESTRATOR.md)에 `Opus 5.5`, `Opus, high`, `Sonnet 5.5, medium`이 있다. |
| 후속 문제 분류표 | "코드와 Editor가 각각 맞지만 함께 실패하면 마스터가 분류하고 Editor 읽기 전용 조사로 양쪽 계약 추적", "기존 범위 밖의 개선 요구는 기능 명세 신규 작업" | 두 행 모두 없다. |
| 요청 분류표 | "화면 조작 명시 요청 → 화면 작업 모드 진입 조건 확인" | 행이 없다. |
| PIE 실패 시 커밋 비교 | 코드 단계 커밋과 Editor 단계 커밋을 나눠 비교해 책임 단계 분류; 실패한 단계만 되돌림 | 없다. |
| Editor 단계 생략 조건 | `PROMPT_UNREAL.md`의 "Content 변경 없음" 선언 **그리고** `git status`로 Content가 그대로임을 확인 | [AGENT_WORKFLOW.md:29](../../.md/AGENT_WORKFLOW.md)에 `git status` 확인 조건이 없다. |
| 하위 에이전트 동기화 | 이미 호출된 하위 에이전트는 이후 마스터 대화를 자동으로 동기화하지 않는다 | 없다. 재개할 때 새 결정을 메시지로 넘겨야 한다는 근거 문장이다. |

## 3. 잔재와 불일치

- [AGENT_UNREAL_EDITOR.md:114](../../.md/AGENT_UNREAL_EDITOR.md): "완료 또는 통합 승인으로 기록하지 않는다"에 통합 리뷰가 남아 있다.
- [AGENT_UNREAL_EDITOR.md:63](../../.md/AGENT_UNREAL_EDITOR.md): "…재로드·시나리오 검증 경로를 먼저 확인"은 PIE 제외 원칙과 맞지 않는다.
- [USER_UNREAL.md](../../.md/USER_UNREAL.md):
  - "완료 전에는 통합 리뷰를 승인하지 않는다" 문구 (5·18·28행)
  - `PROMPT_INTEGRATION_REVIEW.md` 참조 (240행)
  - PIE·플레이 검증 항목. 새 규칙상 `PIE_CHECKLIST.md`로 가야 한다.
- [WIDGET_UI_WORKFLOW_PROPOSAL.md](../../.md/WIDGET_UI_WORKFLOW_PROPOSAL.md): 폐기된 `AGENT_UNREAL_MCP.md`, `AGENT_UNREAL_PYTHON.md`, `AGENT_INTEGRATION_REVIEW.md`를 참조한다.
- [bathhouse-retrospective.md](../../.claude/agents/bathhouse-retrospective.md): 공통 문구를 그대로 복사해 회고에 맞지 않는다. 회고에는 "작업 ID·작업 폴더·단계 시작 커밋"이나 "자기 역할 태그"가 없다. 회고 기준점, 입력 범위, 읽기 전용 제약으로 바꿔야 한다.

## 4. 제안서에도 없던 보완점

1. **Codex 워커에 역할 정의가 없다.** Claude 워커에는 `.claude/agents` 파일이 있지만 Codex는 `AGENTS.md`만 자동으로 읽는다. 그래서 인계 패킷에 다음을 직접 넣어야 한다.
   - 구현 역할 지정과 `AGENT_IMPLEMENTATION.md` 읽기
   - 커밋 금지, `AGENT_*.md` 수정 금지
   - 20줄 완료 보고

   이 내용은 `AGENT_ORCHESTRATOR.md`에 고정 템플릿으로 두는 편이 안전하다.
2. **`codex exec` 호출 방법이 덜 정해졌다.**
   - 마스터의 Bash 도구는 최대 10분이라, 구현과 UE 빌드를 하면 시간 초과가 난다. 백그라운드 실행과 완료 확인 규칙이 필요하다.
   - `-o` 파일에는 세션 ID가 남지 않는다. 세션 ID를 얻는 방법(`--json` 출력 또는 `codex exec resume --last`)을 정해야 한다.
   - `-o` 보고 파일(`codex_<단계>.md`)은 결과물 소유권 표와 첫머리 형식 적용 여부가 정의돼 있지 않다.
3. **Editor가 열린 상태에서 빌드하는 경우를 다루는 규칙이 없다.** 제안서 10번은 사용자 Editor를 유지하라고 하는데, 구현 단계의 `Build.bat`은 Live Coding이 켜져 있거나 모듈 DLL이 잠겨 있으면 실패한다.
   - 리뷰 쪽에는 규칙이 있다([AGENT_REVIEW.md:43](../../.md/AGENT_REVIEW.md)).
   - 구현 단계와 마스터 쪽에는 없다. 구현 단계 진입 전에 Editor 상태를 확인하는 단계가 없으면 자동 구간이 여기서 멈춘다.
4. **같은 작업 폴더에서 결과물 이름이 겹친다.**
   - PIE 실패 진단이나 "함께 실패" 진단용 Editor 읽기 전용 조사가 같은 폴더의 사전 조사 `REPORT_UNREAL_DISCOVERY.md`를 덮어쓰게 된다.
   - 이때 단계 값도 "Editor 사전 조사"로 맞지 않는다.
   - [AGENT_UNREAL_EDITOR.md](../../.md/AGENT_UNREAL_EDITOR.md)의 읽기 전용 조사 진입 조건에도 "마스터의 진단 요청"이 명시돼 있지 않다.
5. **브랜치를 언제 만들고 기능 명세 커밋을 누가 하는지 정해지지 않았다.**
   - `work/<작업 ID>` 브랜치 생성 시점이 없다.
   - `/feature-spec` 일반 세션이 만든 `Work/<작업 ID>/`가 main 작업 트리에 생기는지 별도 worktree에 생기는지 불명확하다.
   - "커밋은 마스터가 한다"는 규칙이 있는데, 일반 세션은 마스터가 아니므로 명세 결과물 커밋 주체가 비어 있다.
6. **재개 근거가 기록되지 않는다.**
   - 코드 리뷰 승인은 결과물을 남기지 않는다. 그래서 새 세션이 "리뷰 통과"를 확인할 근거가 `CONTEXT.md` 한 줄뿐이다. 승인 커밋을 함께 적는 규칙이 있으면 안전하다.
   - 마스터가 쓰는 결과물(`PIE_CHECKLIST.md`, `PROMPT_UNREAL_R.md`)의 첫머리 "단계" 값이 정의돼 있지 않다.
   - `CONTEXT.md`와 `PIE_CHECKLIST.md`의 기본 틀이 없다. 사용자가 체크리스트에 통과·실패를 표시하는 위치도 정해져 있지 않다.

## 5. 의도한 변경인지 확인할 것

| 항목 | 제안서 | 현재 | 판단 |
|---|---|---|---|
| 저위험 Feedback 승인 | 525·529행: 요청된 회고 안의 검증 항목 명확화와 저위험 Feedback은 마스터가 승인하고 보고할 수 있다 | [FEEDBACK_POLICY.md:106-108](../../.md/FEEDBACK_POLICY.md)는 항상 사용자 승인으로 읽힌다 | 제안서 Q8의 답(A)과 `AGENTS.md`는 엄격한 쪽이라 현재 쪽이 맞아 보인다. 다만 114행의 "반드시 사용자 승인" 목록과 함께 읽으면 그 밖의 변경은 승인이 필요 없는 것처럼 읽혀 모호하다. 한쪽으로 확정해야 한다. |
| 기존 FB 항목 처리 | 역할 태그·상태를 붙인 일반화 항목으로 바꾼다 | "이전 형식 기록"으로 보관하고 회고 때 처리한다 | 합리적인 변경으로 보인다. 의도한 것이면 그대로 둔다. |

## 반영 위치 제안

| 대상 | 줄 수 (현재 / 상한) | 넣을 항목 |
|---|---|---|
| `.md/AGENT_ORCHESTRATOR.md` | 110 / 150 | 1-B, 1-C, 2(분류표·커밋 비교·동기화·모델 이름 제거), 4-1, 4-2, 4-3 |
| `.claude/agents/*.md` | — | 1-A, 3(회고 정의 파일) |
| `.md/AGENT_WORKFLOW.md` | 178 / 200 | 2(`CONTEXT.md` 항목 목록, 단계 생략 조건), 4-4, 4-5, 4-6. 여유가 적으므로 최소 문장만 넣는다 |
| `.md/AGENT_UNREAL_EDITOR.md` | 114 / 150 | 3(잔재 문구), 4-4(진단 조사 진입 조건과 결과물 이름) |
| `.md/USER_UNREAL.md`, `.md/Work/` | — | 1-D 이관과 3(잔재 문구) |
| 역할별 요구 수준 표 | — | `AGENT_ORCHESTRATOR.md` 또는 `AGENT_WORKFLOW.md` 중 한 곳에만 둔다 |
