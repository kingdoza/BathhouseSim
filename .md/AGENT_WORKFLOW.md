# Agent — Common Workflow

## 목적

BathhouseSim 기술 작업의 공통 순서, 사용자 승인 관문, 작업 폴더와 결과물, 문서 소유권과 크기 제한을 정의한다.
워크플로는 기술적으로 일관된 구현뿐 아니라 사용자가 승인한 실제 게임 동작과 Editor 결과까지 일치시키는 것을 목표로 한다.

## 실행 구성

- 마스터 세션: 사용자가 오케스트레이션 Skill(`/orchestrate`)로 시작한 세션만 마스터가 된다. 단계 전환과 워커 호출은 [AGENT_ORCHESTRATOR.md](AGENT_ORCHESTRATOR.md)를 따른다.
- 일반 세션: Skill 없이 열거나 `/feature-spec`처럼 역할 Skill로 연 세션이다. 지정된 역할 하나만 하고 다음 단계를 자동으로 호출하지 않는다.
- 워커: 마스터가 호출하는 역할 에이전트다. 해당 `AGENT_*.md`와 이 문서를 따르며 모델·추론 설정은 정의 파일(`.claude/agents/*.md`)이 정한다.
- 기록·피드백 정책은 [FEEDBACK_POLICY.md](FEEDBACK_POLICY.md), C++ 빌드와 headless Editor 실행은 [UE_BUILD_POLICY.md](UE_BUILD_POLICY.md)를 따른다.

## 기본 순서

1. 기능 명세 초안
2. 필요한 경우 Unreal Editor 읽기 전용 사전 조사(질문지 전달 전)
3. 사용자 질문지 답변과 기능 명세 확정(조건 충족 시 자동 승인)
4. 아키텍처 설계
5. C++ 구현
6. 코드 리뷰
7. Unreal Editor 작업
8. 사용자 PIE 검증

- 기능 명세 승인(자동 승인 포함, [AGENT_FEATURE_SPEC.md](AGENT_FEATURE_SPEC.md)) 전에는 아키텍처와 구현을 시작하지 않는다. 승인 뒤 4~7단계는 마스터가 자동으로 잇는다.
- 앞 단계의 결과물이 `완료`가 되기 전에는 다음 단계로 진행하지 않는다.
- PIE 검증은 항상 사람이 한다. 에이전트는 PIE를 시작하거나 PIE에서 입력·시나리오를 검증하지 않는다. 별도의 통합 리뷰 단계는 없다.
- `PROMPT_UNREAL.md`가 Content 변경 없음을 선언하고 `git status`로 Content가 그대로임을 확인하면 마스터가 Editor 작업 단계를 생략할 수 있다. Content와 무관한 작은 버그는 버그 리포트를 기능 계약으로 보고 기능 명세를 생략할 수 있다. 생략 근거는 `CONTEXT.md`에 남긴다.

## 수직 구현 경로

입력·UI·Content·Actor lifecycle·물리·collision·Navigation 또는 여러 대상의 공통화가 함께 바뀌는 작업은 대표 시나리오 하나를 먼저 완성한다.

```text
기능 명세 승인 → 수직 구현 아키텍처 → 구현 → 코드 리뷰 → Unreal Editor 작업
→ 사용자 PIE 검증·수직 구현 승인 → 전체 확장 아키텍처부터 동일 순서 반복
```

- 수직 구현은 일부 계층만이 아니라 대표 대상 하나의 사용자 흐름 전체를 완성한다.
- 사용자 승인 전에는 나머지 설비·아이템·상태로 일반화하지 않는다.
- 대표 시나리오와 전체 확장은 서로 다른 작업 ID와 작업 폴더를 쓴다.
- 사용자 수직 구현 승인은 구현·Editor를 직접 수정하는 권한이 아니며 전체 확장 단계의 새 아키텍처 입력이다.

## 작업 ID와 작업 폴더

- 작업은 기능 명세부터 사용자 PIE 검증까지 한 벌의 단계 결과물을 가지는 단위다. 수직 구현 단위, 전체 확장 단계, 단순 버그 수정이 각각 하나의 작업이다.
- 여러 단위로 나뉘는 기능은 상위 작업 ID 아래 단위 작업 ID를 둔다. 예: `.md/Work/SERVICE/SERVICE-U4/`.
- 버그 수정 작업 ID는 버그 리포트 파일명에서 만든다. 예: `BUG-2026-10-01_litter_tongs_false_take_highlight`. 새 `BUG-` 작업은 완료된 작업에서 나중에 발견한 버그에만 만든다.
- 한 번 정한 작업 ID는 바꾸지 않는다. 시나리오 ID(`TRSH-001` 등)와 별개다.
- 단계 결과물은 `.md/Work/<작업 ID>/`에 둔다. 여러 작업이 동시에 진행·보류 상태일 수 있다.
- 공유 정본(`Architecture/*`, `Unreal/*`, `USER_UNREAL.md`, `BugReports/`, `FEEDBACK_BACKLOG.md`)은 작업 폴더에 두지 않는다.
- `CONTEXT.md`는 진행 상태 요약이며 결정의 정본은 QNA와 단계 결과물이다. 첫머리 형식을 적용하지 않고 다음 틀을 쓴다.

```markdown
# CONTEXT — <작업 ID> <작업 이름>
- 목표 / 상위·선행·관련 작업:
- 현재 단계와 재개 지점:
- 명세 승인 일자(자동/명시)와 사전 허용:
- 작업 브랜치, 단계별 시작 커밋, 리뷰 승인 커밋:
- 리뷰 회차, 아키텍처 자동 복귀 사용 여부, 생략한 단계와 근거:
- 복귀 기록: 복귀 대상, 변경 범위, 유지되는 완료 범위
- 사용자 지시 모델 덮어쓰기:
- 결과물 목록과 사용자 지시 요약:
```
- `CONTEXT.md`는 마스터가 갱신한다. 일반 세션은 재개 지점과 자기 단계의 기록만 갱신한다.
- 사용자 PIE 통과, 정본 반영과 병합을 확인한 뒤 작업 폴더를 제거하고 이력은 Git에 맡긴다. 상위 작업 폴더는 모든 단위가 끝난 뒤 제거한다.

## 결과물 첫머리 형식

작업 폴더의 모든 단계 결과물은 제목 바로 아래에 다음 세 줄을 같은 순서로 둔다.

```markdown
- 작업 ID: `SERVICE-U3`
- 단계: Editor 작업
- 상태: 보류 — <사유>, <책임 단계>, <재개 조건>
```

- 상태는 `진행`(작성 중, 다음 단계 입력 아님), `완료`(결론 확정, 다음 단계 입력 가능), `보류`(사유·책임 단계·재개 조건 필수)다.
- 단계 값은 기능 명세, Editor 사전 조사, Editor 진단, 아키텍처, 구현, 코드 리뷰, Editor 작업, 사용자 PIE 검증 중 하나다.
- 마스터가 쓰는 결과물의 단계 값은 `PIE_CHECKLIST.md`가 사용자 PIE 검증, `PROMPT_UNREAL_R.md`가 Editor 작업이다.
- `CONTEXT.md`는 첫머리 형식 대상이 아니다.
- 재작업 프롬프트(`PROMPT_IMPLEMENTATION_R.md`, `PROMPT_UNREAL_R.md`)는 네 번째 줄에 출처를 둔다. 예: `- 출처: 코드 리뷰 2회차`, `- 출처: 사용자 PIE (BugReports/<파일>)`.
- 소유 단계만 자기 결과물의 첫머리를 바꾼다. 에이전트는 시작할 때 입력 결과물의 작업 ID가 지시받은 작업 폴더와 같고 상태가 `완료`인지 확인한다.

## 정본 정책

| 정책 | 정본 |
|---|---|
| 승인된 사용자 동작과 수용 시나리오 | 작업 폴더의 `PROMPT_ARCHITECTURE.md` |
| 전체 시스템 지도와 의존 방향 | `.md/0_ARCHITECTURE.md` |
| 클래스 성장과 책임 분리 | `.md/Architecture/CoreSystem.md` |
| 시스템별 책임과 API | 관련 `.md/Architecture/*System.md` |
| 전체 Editor authoring 지도 | `.md/Unreal/0_UNREAL.md` |
| 시스템별 asset 구조·연결·설정 | 관련 `.md/Unreal/*System.md` |
| 실제 serialized Editor 데이터 | `Content/`의 `.uasset`, `.umap` |

Unreal 문서는 asset 전체를 복제하지 않고 C++ 계약과 기능 결과에 영향을 주는 현재 구조만 기록한다. 날짜별 진행 기록은 Git에 맡긴다.

## 정기 결과물과 소유권

| 작성자 | 결과물(작업 폴더 안) |
|---|---|
| 기능 명세 | `PROMPT_ARCHITECTURE.md`, `QNA_FEATURE_SPEC.md` |
| Editor 사전 조사 / 진단 | `REPORT_UNREAL_DISCOVERY.md` / `REPORT_UNREAL_DIAGNOSIS.md` |
| 아키텍처 | `PROMPT_IMPLEMENTATION.md`, `QNA_ARCHITECTURE.md`, Architecture 정본 |
| 구현 | Source·승인된 Config, `PROMPT_REVIEW.md`, `PROMPT_UNREAL.md`, `QNA_IMPLEMENTATION.md` |
| 코드 리뷰 | 리뷰 실패와 사용자 PIE의 구현 문제: `PROMPT_IMPLEMENTATION_R.md`, `QNA_REVIEW.md` |
| Unreal Editor 작업 | `REPORT_UNREAL_EDITOR.md`, 관련 Unreal 정본, `USER_UNREAL.md` 항목 |
| 마스터 | `CONTEXT.md`, `PIE_CHECKLIST.md`, 사용자 PIE의 Editor 설정 문제: `PROMPT_UNREAL_R.md` |

- 사전 조사가 불필요하면 `REPORT_UNREAL_DISCOVERY.md`를 만들지 않고 생략 근거를 기능 명세에 남긴다.
- 코드 리뷰 승인은 정기 결과물을 만들지 않고 보고로 종료한다.
- 리뷰 단계는 입력을 직접 고치지 않고 해당 소유 단계로 돌려보낸다.
- 커밋과 병합은 마스터가 한다. 워커는 커밋하지 않는다. 마스터가 아닌 일반 세션은 자기 단계 결과물만 기본 브랜치에 커밋할 수 있다.
- Editor 보고서 저장: Claude 하네스는 하위 에이전트의 보고서 파일 쓰기를 막는다.
  - 마스터 워커로 실행된 Editor 역할은 `REPORT_UNREAL_*.md`를 직접 쓰지 않고, 첫머리를 갖춘 전문을 저장 경로와 함께 완료 보고 뒤에 붙인다.
  - 마스터는 전문을 수정 없이 저장하고 첫머리 아래에 `(Editor 워커 전문을 마스터가 저장)`만 붙인다. 내용 소유는 Editor 역할이다.
  - 일반 세션은 직접 쓴다. Unreal 정본·`USER_UNREAL.md` 쓰기가 같은 제한에 걸리면 같은 방식으로 처리하고 회고 대상에 남긴다.
- 워커 에이전트는 `AGENT_*.md`와 `FEEDBACK_BACKLOG.md`의 일반화 항목을 수정하지 않는다.
- `AGENT_*.md`에는 작업별 클래스·에셋·수행 기록을 누적하지 않는다.

## 복귀 원칙

- 코드 리뷰나 Editor 작업에서 상위 단계 결함이 드러나면 이미 쓴 비용과 관계없이 책임 단계로 복귀한다.
- 상위 단계 결함을 Blueprint graph, asset 값이나 Level 배치로 우회하지 않는다.
- 책임이 있는 가장 낮은 단계로 복귀한다.

| 판정 | 복귀 대상 |
|---|---|
| asset이 틀렸고 C++ 계약이 맞음 | 복귀 없음. Editor 단계 안에서 수정 |
| 계약은 맞고 구현이 틀림 | 구현 |
| 계약 자체가 실제 asset·Level·엔진 동작과 맞지 않음 | 아키텍처 |
| 사용자 동작을 다시 정해야 함 | 기능 명세 |

- 복귀한 단계는 영향받는 시나리오 ID와 asset만 다시 정하고 결과물에 변경 범위를 적는다. 이후 단계는 그 범위만 재작업·재리뷰한다.
- Editor 작업 중 상위 결함이 드러나면 영향받는 asset만 저장하지 않고 멈춘다. 영향이 없는 allowlist 항목은 저장·재로드까지 마친다.

## `USER_UNREAL.md` 미완료 작업 큐

허용된 자동화(MCP·Python·기존 helper·commandlet)와 사용자가 승인한 화면 작업으로 완료할 수 없는 실제 Editor 수정 작업만 기록한다. PIE·플레이 검증은 기록하지 않고 작업 폴더의 `PIE_CHECKLIST.md`에 둔다.

- 한국어로 작성한다. 항목마다 작업 ID, exact asset/actor/package, 현재·목표 상태, 시도한 자동화 경로와 오류, 남은 조작, 먼저 끝내야 하는 PIE 시나리오와 재개 조건을 적는다.
- 분류: 수동 작업 필요 / 실행 환경 차단 / 화면 작업 미승인·미지원 / 승인 밖.
- 미완료 상태만 유지한다. 사용자가 완료를 알리면 Editor 역할이 저장·재로드를 확인한 뒤 항목을 제거한다.

## 질문과 복귀

- 사용자 동작 선택은 기능 명세 단계의 `QNA_FEATURE_SPEC.md`, 기술 설계 선택은 `QNA_ARCHITECTURE.md`, 구현 선택은 `QNA_IMPLEMENTATION.md`, 리뷰 정보 부족은 `QNA_REVIEW.md`를 쓴다.
- 구현 중 사용자 동작을 새로 결정하지 않고 기능 명세로 복귀한다.
- Editor 단계에서 코드·설계 문제가 발견되면 Blueprint 우회를 만들지 않고 소유 단계로 복귀한다.

## 문서 크기 정책

- `AGENT_*.md`: 목표 80~120줄, 최대 150줄. 단 `AGENT_WORKFLOW.md`는 공통 정책 문서로 본다.
- 공통 정책 문서: 최대 200줄
- 작업 폴더의 모든 프롬프트·질문 문서(`PROMPT_*.md`, `QNA_*.md`)와 `PIE_CHECKLIST.md`: 줄 수 상한 없음. 줄 수 때문에 동작 계약, 사용자 선택, 설계·구현 지시, exact asset·값·검증 기준, 시나리오 같은 디테일을 줄이거나 빼지 않는다. 중복만 피한다.
- 보고와 요약(`REPORT_UNREAL_DISCOVERY.md`, `REPORT_UNREAL_DIAGNOSIS.md`, `REPORT_UNREAL_EDITOR.md`, `CONTEXT.md`): 작업 하나당 최대 200줄. 상세 근거는 로그·스크립트·결과물에 두고 경로로 가리킨다.
- 시스템 문서: 300줄부터 분리 검토, 400줄 이상 성장 동결

상한을 넘으면 현재 상태만 남기고 안정적인 책임 경계로 분리한다. 같은 목록·규칙·asset 값을 여러 문서에 복제하지 않는다.

## 조정값 원본 원칙

- 동작·감각에 영향을 주는 수치(거리·여유·간격·시간·횟수·확률 등)는 코드 상수로 두지 않고 조정값 원본(Project Settings·Config, DataAsset, Blueprint 기본값)에 둔다. 코드는 원본을 읽고 순수 계산 helper는 값을 인자로 받는다. 엔진 의미상 상수(0 판정, 단위 변환 등)만 예외이며 설계에 예외 목록과 근거를 적는다.
- 문서(명세·설계·Architecture/Unreal 정본)는 수치를 복제하지 않고 원본 위치(클래스·프로퍼티, Config 파일·섹션, asset·프로퍼티)를 참조한다. 기능 명세의 기본값 표는 사용자 판단용 제안값이며 확정 뒤 정본에는 원본 위치만 남긴다.
- 자동화 테스트는 기대값을 리터럴로 복제하지 않고 같은 원본에서 읽거나 계산한다. 값 자체를 고정하는 회귀 테스트면 이유를 적는다.
- 이 원칙은 Architecture·Unreal 정본의 값 기록 규칙보다 우선한다.

## 공통 완료 조건

- 승인된 기능 시나리오와 현재 단계 범위를 벗어나지 않았다.
- 사용자 소유 변경과 무관한 파일·asset을 수정하지 않았다.
- 빌드, Compile, Save, 재로드 중 수행한 검증과 미검증을 구분했다.
- Editor 변경은 관련 `.md/Unreal/*System.md`의 현재 상태와 일치한다.
- 문서 변경 후 diff, 링크, 소유권과 줄 수를 확인했다.

## 전환 규칙 (2026-10-01 채택)

- 모든 작업은 `.md/Work/<작업 ID>/`와 결과물 첫머리 형식을 쓴다. 채택 전의 루트 결과물(`PROMPT_*.md`, `QNA_*.md`, `REPORT_UNREAL_DISCOVERY.md`, `PROMPT_INTEGRATION_REVIEW.md`)과 `.md/NextWork/`는 서비스 4단위 완료 후 삭제했으며 내용은 Git 이력에 있다.
- 정본 문서나 `USER_UNREAL.md`가 삭제된 루트 결과물을 가리키면 해당 시점 커밋의 파일로 읽는다.
- 작업 폴더가 없는 기존 작업을 이어갈 때는 마스터가 폴더와 `CONTEXT.md`를 만들고, `USER_UNREAL.md`의 PIE·플레이 검증 항목을 `PIE_CHECKLIST.md`로 옮긴다.
