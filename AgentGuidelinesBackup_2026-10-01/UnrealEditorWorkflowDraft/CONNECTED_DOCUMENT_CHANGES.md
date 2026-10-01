# 통합 Unreal Editor 지침 연결 문서 수정안

## 상태와 적용 대상

적용 대기 수정안이다. 현재 작업은 이 폴더의 초안 네 파일 작성만 포함하며 기존 정본·Source·Config·Content와 진행 중인 결과물을 수정하지 않는다.
적용 시 `AGENT_UNREAL_MCP.md`를 메인 workflow에서 제거하고 `AGENT_UNREAL_EDITOR.md`로 전면 교체한다. 두 역할을 병행하거나 MCP 역할을 재호출하지 않는다.
기능 명세·아키텍처·C++ 구현·코드 리뷰·통합 리뷰의 단계·승인 관문은 유지하고 Editor 작업 역할·실행 수단을 통합한다.

## 승격과 파일 배치

| 이 폴더의 파일 | 채택 후 정본 위치 | 처리 |
|---|---|---|
| [AGENT_UNREAL_EDITOR.md](AGENT_UNREAL_EDITOR.md) | `.md/AGENT_UNREAL_EDITOR.md` | 신규 통합 역할 정본 |
| [UNREAL_PYTHON_API.md](UNREAL_PYTHON_API.md) | `.md/UNREAL_PYTHON_API.md` | 기존 Python 지침의 기술 절차 대체 |
| [UNREAL_MCP_CONNECTION.md](UNREAL_MCP_CONNECTION.md) | `.md/UNREAL_MCP_CONNECTION.md` | 기존 연결 문서 교체; 공통 세션 정책도 소유 |
| 이 파일 | 적용 계획 또는 검토용 초안 | 실행 역할의 필수 문서로 등록하지 않음 |

- `AGENT_UNREAL_MCP.md` 정본은 연결 참조 정리 후 삭제한다. 자동 재호출용 호환 역할을 남기지 않는다.
- `AGENT_UNREAL_PYTHON.md`도 기술 내용 이관·참조 정리 후 정본 목록에서 제거한다. 과거 승인·실행 사실은 지우지 않는다.
- 공통 세션은 새 `UNREAL_MCP_CONNECTION.md`가 소유한다. `UNREAL_EDITOR_SESSION.md`를 추가하지 않아도 된다.
- 승격 시 `../AGENT_WORKFLOW.md` 등 부모 링크는 `AGENT_WORKFLOW.md`로, `../Unreal/...`·`../Architecture/...`는 루트 기준으로 고친다.
- 승격 문서의 초안 상태를 정본 상태로 바꾸고 이 계획에 대한 필수 의존을 제거한다.
- 이 폴더를 보관하면 초안 상태를 유지한다. 정본과 다른 두 실행 규칙을 활성화하지 않는다.

## 1. AGENTS.md

- Canonical Documents에서 `.md/AGENT_UNREAL_MCP.md` 항목을 제거한다.
- 추가: `.md/AGENT_UNREAL_EDITOR.md` — 통합 Editor 조사·authoring·검증 역할.
- 추가: `.md/UNREAL_PYTHON_API.md` — 통합 역할의 Python 기술 절차.
- 추가 또는 명시: `.md/UNREAL_MCP_CONNECTION.md` — 공통 Editor 세션·MCP 연결.
- `AGENT_COMPUTERUSE.md` 설명은 “통합 Editor 역할의 명시적으로 허용된 화면 작업 절차”로 바꾼다.
- Working Rules의 MCP 전용 금지·즉시 수동 인계 문장을 아래 문장으로 교체한다.
  > Editor 작업은 AGENT_UNREAL_EDITOR.md를 따른다. 승인된 범위에서 MCP·공식 Python API와 검증된 보조 도구를 사용하고, 자동화로 완료할 수 없는 실제 작업만 USER_UNREAL.md에 인계한다. Computer Use는 명시적 허용과 native 실행 환경 지원이 필요하다.
- 엔진·tool 목록·workflow 세부사항은 라우팅 파일에 복제하지 않는다.

## 2. AGENT_WORKFLOW.md

- 기본 순서 2·7과 수직 flow의 “Unreal MCP”를 “Unreal Editor 사전 조사/작업”으로 교체한다.
- “Unreal MCP 사용 경계”는 “Unreal Editor 사용 경계”로 바꾸고 두 진입 모드는 유지한다.
- 교체 내용: 하나의 에이전트가 MCP·Python을 선택·실행한다. 기존 승인 범위 안의 도구 전환마다 별도 승인을 받지 않으며 작업별 명시 제한은 유지한다.
- MCP toolset 부족만으로 중단·인계하지 않고 Python·기존 보조 도구의 capability를 검토한다.
- `USER_UNREAL.md` 정책을 “허용된 자동화로 완료할 수 없는 실제 작업”으로 바꾸고 확인 경로·원인·재개 조건을 추가한다.
- 정기 결과물의 생산 단계를 통합 역할로 바꾸되 `REPORT_UNREAL_DISCOVERY.md`·`PROMPT_INTEGRATION_REVIEW.md` 이름은 유지한다.
- 조사 보고·Editor 정본·통합 인계는 Editor 역할, Source/승인 Config는 구현 역할이라는 소유권을 유지한다.
- Editor 스크립트·manifest·로그는 통합 역할이 소유한다. 재사용 `Scripts/Unreal/`와 작업별 `Saved/Codex/Unreal/<TaskId>/`를 구분한다.
- Computer Use는 명시적으로 허용되고 native 환경이 지원할 때 같은 역할의 화면 작업 모드로 연결한다.
- 신규 C++ helper·Config 변경은 설계/구현·코드 리뷰로 반환한다. Editor 역할의 Source·Config 직접 수정 권한을 추가하지 않는다.
- 기존 Build/Headless Policy의 엔진 진입점과 `-DDC-ForceMemoryCache`를 유지한다. Python commandlet에도 적용한다.
- 크기 규칙 유지: 통합 역할 150줄 이하, 기술 절차·이 계획 각 200줄 이하.

## 3. AGENT_FEATURE_SPEC.md

- 허용 범위·기능 명세 절차·사전 조사 제목의 MCP 표현을 “통합 Unreal Editor 읽기 전용 조사”로 교체한다.
- 요청은 exact 대상, 사실·필드·관계, 사용자 관찰 결과와 read-only 제약을 적는다. 실행 수단은 Editor 역할이 선택한다.
- 실제 Editor 사실과 조회 불가를 분리하고 Source만으로 asset 값을 확정하지 않는 원칙을 유지한다.
- 기능 명세 승인·설계 소유권·수직 판단·사용자 동작 계약은 변경하지 않는다.

## 4. AGENT_ARCHITECTURE.md

- “정확한 읽기 전용 MCP 조사 항목”을 “통합 Editor 역할의 정확한 읽기 전용 조사 항목”으로 교체한다.
- 사실 부족 시 대상·정보만 요청하고 실행 수단을 고정하지 않는다.
- 필요한 C++ authoring helper는 승인된 구현 범위·Editor 모듈 경계·검증을 설계한 뒤 구현 단계로 전달한다.
- helper가 게임 runtime 책임을 대신하거나 Editor 의존을 runtime 모듈에 넣지 않도록 기존 Core 경계를 적용한다.
- 설계 독점·정본 소유권과 `Unreal/*` 읽기 전용 규칙은 유지한다.

## 5. AGENT_IMPLEMENTATION.md

- `PROMPT_UNREAL.md` 인계 판정자를 “Unreal MCP 단계”에서 “통합 Unreal Editor 단계”로 변경한다.
- exact asset/actor/package, Parent·property·BindWidget·참조·값·단위·좌표와 유지 계약을 인계한다.
- 추가 필드: 작업 모드·기존 사용자 변경, 생성/수정/저장 allowlist, helper 설치·버전 조건, 시나리오별 검증·재로드 수준.
- “MCP 불가면 수동”을 사전에 확정하지 않는다. 실행 경로는 통합 역할이 실제 capability로 선택한다.
- 신규 C++ helper·Config는 구현 결과와 코드 리뷰 대상에 포함한다. Editor 단계가 임의 제작·설치하도록 인계하지 않는다.
- 기본 MCP·Python 사용을 작업별 예외 승인으로 작성하지 않는다. 기존 사용자의 명시적 도구·대상 제한은 유지한다.

## 6. AGENT_REVIEW.md

- 코드 승인 후 `PROMPT_UNREAL.md` 수신 역할을 통합 Unreal Editor로 교체한다.
- allowlist·native 계약·보조 도구와 Compile/Save/reload/PIE 기준의 완결성을 확인한다.
- C++ helper가 포함되면 엔진 버전·모듈 경계·실패 처리·저장 범위도 리뷰한다.
- 도구가 바뀌어도 기능 시나리오·승인 범위는 같아야 한다. 입력·Content를 직접 수정하지 않는다.

## 7. AGENT_INTEGRATION_REVIEW.md

- 필수 입력에 통합 Editor 지침과 필요한 기술 절차를 연결한다.
- “MCP 읽기 기능으로 검사”를 “MCP 또는 공식 Python의 읽기 전용 경로로 실제 asset 검사”로 교체한다.
- 실행된 Python 스크립트·manifest·요약·로그를 읽고 allowlist·idempotence·오류 누락을 확인한다.
- Compile·Validation·Save·메모리 readback·디스크 reload·새 프로세스 reload·PIE·입력·화면 결과를 각각 대조한다.
- binding 미노출·부분 world 로드·필수 검증 누락은 승인하지 않는다. 스크립트 PASS로 수용을 대체하지 않는다.
- read-only 책임·재작업 반환·사용자 수직 플레이 승인 관문은 유지한다.
- 기능 부족·권한 거부·native 화면 미지원으로 남은 필수 항목은 미완료로 유지한다. 리뷰 자체의 UI 자동 전환 권한은 추가하지 않는다.

## 8. AGENT_COMPUTERUSE.md

- 별도 에이전트에 반드시 인계하는 설명을 통합 Editor 역할의 화면 작업 모드로 고친다.
- 명시적 Computer Use 허용과 대상 작업, 현재 native 관찰·입력 지원, 최신 화면 근거 조건을 유지한다.
- MCP 실패나 자동화 일반 요청을 화면 조작 승인으로 확대하지 않는다. 기존 세션의 허용은 유효하며 같은 승인을 반복 요청하지 않는다.
- 브라우저만 지원하면 native Unreal 작업은 미지원이다. 문서 변경으로 실행 권한이 생기지 않는다.
- Compile·개별 Save·reload·PIE·정본 기록은 통합 기준을 참조하고 중복을 줄인다.
- 신규 helper·OS 입력 우회로 disabled native 기능을 대신하지 않는다. 모델·추론 설정 변경은 이 통합의 범위가 아니다.

## 9. Unreal/0_UNREAL.md 및 시스템 문서

- 생성·라우팅·갱신 owner의 “Unreal MCP 또는 Computer Use 에이전트”를 통합 Unreal Editor 역할로 변경한다.
- 사전 조사는 “통합 Editor 읽기 전용 조사”, 실제 저장 방식은 MCP·Python·허용된 보조 경로로 기술한다.
- `USER_UNREAL.md` 의미는 MCP 기능 부족이 아닌 남은 실제 자동화·수용 미완료로 고친다.
- 저장·재로드로 확인된 class·component·연결·값만 기록하는 원칙을 유지한다.
- `CustomerSystem.md`의 StateTree·`WorldSystem.md`의 external actor 등 과거 실패를 새 지침만으로 삭제·성공 처리하지 않는다.
- 실제값·미완료는 재조사 후에만 갱신한다. 모든 시스템 문서의 MCP 문자열을 일괄 치환하지 않는다.

## 10. USER_UNREAL.md

- 운영 항목의 “MCP 미지원 = 사용자 수작업”을 통합 역할의 자동화 재검토 대상으로 전환한다.
- 항목 형식: exact 대상/시나리오, 현재·목표·메모리/디스크 상태, 확인한 MCP/Python/helper, 오류·미확인, 남은 조작·검증·재개 조건.
- 분류: 수동 작업 필요 / 실행 환경 차단 / native 화면 미지원 / 승인 밖 / 필수 검증 미완료.
- 코드·Config·설계는 소유 단계로 반환하고 큐에는 의존하는 실제 Editor 미완료만 연결한다.
- 기존 승인·제외 범위·실패 로그·미저장 상태를 유지한다. 승인 범위를 소급 확대하지 않는다.
- 저장·reload·필수 검증 완료를 확인하기 전에는 기존 항목을 제거하지 않는다.

## 11. 진행 중인 프롬프트·보고서

- `PROMPT_UNREAL.md`·존재하는 `PROMPT_UNREAL_R.md`의 운영 지침은 소유 단계가 변경한다.
- 교체 문안: “승인된 범위에서 MCP·공식 Python API·기존 검증 도구로 작업한다. 완료할 수 없는 실제 항목만 USER_UNREAL.md로 인계하고 경로·차단 근거를 보고한다.”
- 기존 명시 제한·target·값·시나리오·allowlist는 유지한다. 통합이 gameplay 계약의 재승인은 아니다.
- 향후 `REPORT_UNREAL_DISCOVERY.md`에는 대상·실제값·경로·로그·미확인·dirty 기준선을 넣는다.
- 향후 `PROMPT_INTEGRATION_REVIEW.md`에는 스크립트·백업·실행 환경과 각 검증 수준·저장 상태를 넣는다.
- 기존 보고의 MCP 수행·Python 별도 승인·부분 완료·미검증은 역사적 사실이다. 수행 방식·상태를 새 명칭으로 다시 쓰지 않는다.
- `PROMPT_ARCHITECTURE.md`·`NextWork/*`의 승인 기능 계약·구현 설계는 문서 교체만으로 수정하지 않는다.

## 12. 기존 논의안

- `ORCHESTRATOR_WORKFLOW_PROPOSAL.md`의 요청 분류·역할명·flow·중단 조건·후속 책임·Unreal 경계를 통합 역할로 정렬한다.
- MCP/Python 전환만으로 다른 에이전트에 재분배하지 않는다. 기능·설계·코드·독립 리뷰 역할은 유지한다.
- Computer Use 별도 역할은 허용된 화면 모드로 맞춘다. 고정 모델·추론 설정은 자동 변경하지 않고 별도 결정 대상으로 표시한다.
- 이 정렬을 오케스트레이터 전체 제안의 채택으로 간주하지 않는다.
- `WIDGET_UI_WORKFLOW_PROPOSAL.md`의 Python 별도 역할/예외 승인·옛 링크를 통합 역할·Python 기술 절차로 연결한다.
- HTML 프로토타입·layout.json·폰트·DPI·UI 승인 흐름 등의 미도입 사항은 이 통합으로 채택하지 않는다.

## 13. Architecture 정본의 조건부 수정

- `CoreSystem.md`의 Source·Content·Config 경계를 유지한다. 재사용 tooling이 실제 도입되면 소유 경로·Editor-only 의존만 추가한다.
- `UISystem.md`의 native/WBP 책임은 유지하고 오래된 실행 역할 링크가 있을 때만 교체한다.
- `UtilityLaborSystem.md` 등의 일반 “MCP 불가면 USER_UNREAL” 문장은 “허용된 자동화로 완료할 수 없는 실제 작업을 인계”로 교체한다.
- `0_ARCHITECTURE.md`·시스템 class/API/값은 역할 통합만으로 바꾸지 않는다. helper 구현으로 구조가 바뀌면 소유 단계가 반영한다.
- `Saved/` 스크립트·백업·로그를 정본 아키텍처로 승격하지 않는다.

## 적용 순서와 확인

1. 세 초안을 정본 위치로 승격하고 상태·상대 링크를 정리한다.
2. `AGENTS.md`·`AGENT_WORKFLOW.md`와 역할 문서를 함께 정렬한다. 중간 상태를 실행 지침으로 사용하지 않는다.
3. 진행 프롬프트·큐의 운영 지침을 소유 단계에서 정리하고 논의안의 역할 참조를 맞춘다.
4. 활성 지침·링크에 `AGENT_UNREAL_MCP.md` 의존이 없음을 확인한 뒤 옛 역할 파일을 제거한다.
5. 옛 Python 역할도 이관·참조 검토 후 제거한다. 과거 보고·승인 근거·Git 이력은 보존한다.
6. `rg`로 `AGENT_UNREAL_MCP|AGENT_UNREAL_PYTHON|UNREAL_MCP_CONNECTION|USER_UNREAL|Computer Use` 참조를 검사하고 활성 규칙·과거 사실을 분류한다.
7. diff·상대 링크·제목·줄 수·소유권을 확인한다. Source·Config·Content와 승인된 기능·asset 완료 상태가 바뀌지 않았는지 확인한다.
8. 문서 채택과 실제 capability는 별개다. 이후 승인된 작업에서 조사·일괄 수정·저장·새 프로세스 검증을 실제 확인한다.
