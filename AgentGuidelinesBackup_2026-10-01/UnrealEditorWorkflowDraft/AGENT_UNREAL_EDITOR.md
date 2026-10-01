# Codex Agent — Unreal Editor Mode

## 상태와 역할

통합 Editor 작업 지침 초안이다. 현재 폴더 작성만으로 기존 워크플로가 바뀌지 않으며, [연결 문서 수정안](CONNECTED_DOCUMENT_CHANGES.md)을 적용한 뒤 정본으로 사용한다.
채택 후 `AGENT_UNREAL_MCP.md`를 전면 대체한다. MCP와 Python은 같은 에이전트가 선택하는 실행 수단이며 도구별로 별도 에이전트에 인계하지 않는다.
에이전트는 실제 Editor 조사, 승인된 asset 수정, 필요한 스크립트 작성·실행, 저장·검증과 결과 인계를 끝까지 담당한다.

## 진입 모드

| 모드 | 진입 조건 | 수행 범위와 결과물 |
|---|---|---|
| 읽기 전용 조사 | 기능 명세·아키텍처의 exact 조사 요청 또는 사용자의 진단 요청 | 지정된 asset·Level·현재 runtime 사실 조회; `REPORT_UNREAL_DISCOVERY.md` |
| Editor 작업 | 코드 단계 승인과 현재 구현에 맞는 `PROMPT_UNREAL.md`; 재작업은 `PROMPT_UNREAL_R.md` | allowlist 수정·검증·저장; `PROMPT_INTEGRATION_REVIEW.md`와 관련 Unreal 정본 |

- 사용자 명시 지시로 진입 조건의 예외가 허용됐다면 승인 내용과 적용 범위를 결과물에 기록한다.
- 조사 모드에서는 `modify`, Compile, Save, asset 생성·삭제와 PIE 시작·종료·입력 등 mutation을 하지 않는다.
- 로드 중 엔진이 자동 수행한 compile·dirty와 에이전트가 요청한 작업을 구분한다. 자동 발생한 dirty도 저장하지 않는다.

## 필수 문서

- [AGENT_WORKFLOW.md](../AGENT_WORKFLOW.md): 단계·승인·결과물·빌드와 headless 실행 정책
- [UNREAL_MCP_CONNECTION.md](UNREAL_MCP_CONNECTION.md): 모든 경로의 공통 Editor 세션 운영 및 MCP 연결
- Python을 사용할 때 [UNREAL_PYTHON_API.md](UNREAL_PYTHON_API.md)
- 해당 모드의 조사 요청 또는 승인된 기능 계약과 현재 Editor 작업 프롬프트
- [0_ARCHITECTURE.md](../0_ARCHITECTURE.md), 관련 `Architecture/*System.md`; UI는 `UISystem.md`, migration은 `CoreSystem.md`
- [Unreal/0_UNREAL.md](../Unreal/0_UNREAL.md), 관련 `Unreal/*System.md`와 [USER_UNREAL.md](../USER_UNREAL.md)
- 명시적으로 허용된 화면 작업만 [AGENT_COMPUTERUSE.md](../AGENT_COMPUTERUSE.md)와 현재 Computer Use 스킬 절차

## 범위와 승인

- 승인된 exact asset·Level·property·시나리오 범위 안에서 MCP와 공식 Editor Python API를 함께 사용할 수 있다.
- MCP↔Python 전환, 허용된 조회·검증 스크립트 실행마다 도구 사용 승인을 다시 받지 않는다. 명시적인 작업별 제한은 우선한다.
- 추가 asset·기능·설정·플러그인 설치 또는 기존 승인 밖 변경은 도구 선택으로 승인된 것으로 간주하지 않는다.
- 허용된 쓰기 대상은 지정된 `Content/`, Editor 스크립트·로그·백업, 해당 단계 결과물과 저장된 상태를 설명하는 Unreal 정본이다.
- `Source/`, `Config/`, Architecture 정본, 입력 프롬프트와 `AGENT_*.md`는 Editor 작업 역할에서 읽기 전용이다.
- 신규 C++ 도구·Config 수정이 필요하면 소유 단계에 근거를 인계한다. 이미 검토·설치된 도구는 승인 범위 안에서 사용한다.
- `.uasset`·`.umap`은 공식 Editor API·검토된 Editor 도구로만 수정·저장한다. 셸·텍스트·바이너리 패치와 비공개 serialization 우회는 금지한다.
- native 계약·runtime/domain 로직을 Blueprint graph에 우회 구현하지 않는다. 도구 부족은 게임 설계를 바꿀 권한이 아니다.

## 실행 경로 선택

| 작업 | 기본 선택 | 확인 사항 |
|---|---|---|
| 소수 객체 조회·속성 변경, 기존 명시적 Editor 기능 | MCP | 현재 schema와 짧은 결과로 완료 가능한지 |
| StateTree·WidgetTree 구조 분석, 여러 asset 비교·일괄 처리 | Python | 해당 버전의 API와 실제 실행 경로가 있는지 |
| Compile·Validation·개별 Save·PIE·재로드 | 지원되는 MCP 또는 Python | 변경부터 검증까지 필요한 기능이 모두 있는지 |
| 기본 API에 없는 graph·binding·compiler 기능 | 기존 C++ Editor 보조 도구 | 설치·버전·노출 함수와 승인 범위; 없으면 구현 단계 인계 |
| 새 프로세스 asset 검증·배치 처리·기존 테스트 | Commandlet·Automation; 필요한 경우 기존 UAT 작업 | 공통 headless 정책과 소유권; 화면·입력 검증과 구분 |
| 화면 조작·시각 확인 | 명시적으로 허용된 Computer Use 모드 | native 앱 관찰·입력 지원과 작업 승인; 브라우저만 지원하면 불가 |

- 모든 작업을 MCP부터 시도하지 않는다. 알려진 기능 부족은 기존 기록과 실제 capability로 확인하고 바로 적합한 경로를 선택한다.
- 다른 허용 경로가 필요한 정보를 제공하면 같은 에이전트가 이어서 수행한다. MCP 도구 부재만으로 사용자에게 넘기지 않는다.
- Computer Use는 기본 자동 전환 경로가 아니다. 명시적으로 허용되고 실행 환경이 지원할 때만 같은 에이전트의 화면 작업 모드로 사용한다.
- 코드·문서와 asset 내용, 메모리 값과 디스크 값, class default와 instance override를 서로 대체 근거로 쓰지 않는다.

## Capability와 상태 확인

1. 공통 세션 절차로 프로젝트·엔진·PID·Editor 소유권·PIE·modal·Live Coding 상태를 확인한다.
2. `git status`, 대상 asset의 기존 변경·파일 해시와 Editor dirty package를 기준선으로 기록한다.
3. 작업에 필요한 조회·편집·Compile·Validation·Save·재로드·시나리오 검증 경로를 먼저 확인한다.
4. MCP는 최초 사용 전 schema를 읽고, Python은 모듈·class·property·signature와 검증된 선례를 확인한다.
5. 변경·저장·필수 검증을 끝낼 경로가 없으면 해당 mutation 전에 중단하고 가능한 독립 작업을 진행한다.
6. 기존 실패의 대상·버전·조건을 확인한다. 오래된 환경 기록이나 포트 개방만으로 현재 가능·불가를 확정하지 않는다.

## 조사와 토큰 효율

- 조사 질문을 필드·상태 경로·관계로 좁히고 필요한 정보만 조회한다. 구조 전체가 필요하면 요약과 상세 파일을 분리한다.
- StateTree는 필요한 subtree, Task/Condition, Transition·binding 관계를 추출하고 조회 불가 필드를 명시한다.
- 반복은 스크립트에서 처리하고 모델에는 변경 수·검사 수·차이·오류·로그 경로를 반환한다.
- 기존 검증 스크립트·helper를 재사용한다. 버전과 schema가 바뀌지 않으면 확인한 설명을 불필요하게 반복 출력하지 않는다.
- 호출 수 감소 때문에 필수 검증·근거를 생략하지 않는다. 조회 불가나 파싱 실패를 빈 값·정상 상태로 표현하지 않는다.

## Editor 작업 순서

1. 입력의 exact 대상·목표값·native 계약·시나리오와 저장 allowlist를 확인한다.
2. 기존 상태를 읽고 백업·기준선을 확보한다. 사용자 변경과 충돌하면 덮어쓰지 않는다.
3. 선택한 경로로 대표 대상 하나를 먼저 처리하고 의도한 결과를 확인한 뒤 동일 범위를 확장한다.
4. 필요한 Blueprint/StateTree Compile과 Data Validation을 수행하고 오류·경고·ensure를 판정한다.
5. allowlist package만 개별 Save하고 저장 결과·dirty 해소를 확인한다.
6. 디스크 재로드로 Parent·component·값·연결을 재검사한다. 요구된 새 프로세스 검증은 같은 세션 readback으로 대체하지 않는다.
7. 지정된 PIE·입력·수치·화면 시나리오를 검증하고 실제 수행 여부를 각각 기록한다.
8. 예상 밖 dirty와 임시 객체를 기준선과 비교하고 재로드로 확인된 상태만 관련 Unreal 정본에 반영한다.
9. 남은 항목을 분류하고 단일 `PROMPT_INTEGRATION_REVIEW.md`로 인계한 뒤 공통 세션 종료 절차를 따른다.

## 저장과 World Partition

- `Save All`, allowlist 밖 package 저장과 사용자 상태를 잃는 reload·종료를 하지 않는다.
- Level actor 수정 전 World Partition 여부와 개별 external actor/package 저장·재로드 지원을 확인한다.
- 알려진 동일 `save_actor` 실패를 다른 actor 생성으로 반복하지 않는다. 다른 검증된 API 경로는 조건·대상을 확인한 뒤 사용할 수 있다.
- 파일 존재, registry 조회, Map Save와 Map dirty=false만으로 external actor의 저장 성공을 판정하지 않는다.
- 저장 경로가 미확정이면 승인된 대표 대상 하나로 먼저 검증한다. 조사 모드에서는 생성·저장 시험을 하지 않는다.

## 실패와 복귀

- 호출/schema 오류, 기능 부족, Editor 상태, 실행 환경, 프로젝트 오류와 권한 거부를 구분한다.
- 동일 원인의 실패는 원인 확인을 포함해 두 번 이내로 제한한다. 다른 경로나 새 PID도 원인·조건 변화 없이 횟수를 초기화하지 않는다.
- 기능 부족이면 독립적으로 허용된 다른 경로를 검토한다. 명시적인 권한·승인 거부는 HTTP·다른 도구·화면 조작으로 우회하지 않는다.
- 코드·설계 문제는 소유 단계로 반환하고 기능 결과의 새로운 선택은 기능 명세로 반환한다.
- 실패한 변경은 저장하지 않는다. 이번 작업의 변경만 안전하게 되돌릴 수 있으면 복구하고 남은 메모리·디스크 상태를 보고한다.

## 미완료 큐와 결과물

- 허용된 MCP·Python·기존 보조 도구로 완료할 수 없는 실제 작업만 `USER_UNREAL.md`에 인계한다.
- exact asset/actor/package, 현재값·목표값, 확인한 경로·오류·미확인, 남은 조작과 검증·재개 조건을 한국어로 기록한다.
- 코드·설계 재작업은 소유 단계 프롬프트로 반환한다. 수동 작업·환경 차단·화면 미지원·승인 밖 항목을 구분한다.
- 기존 큐는 실제 완료·저장·필요한 재로드를 확인한 뒤에만 제거한다. 지침 교체만으로 완료 처리하지 않는다.
- 조사 보고에는 범위·방법·실제값·근거·미확정·기준선을, 작업 보고에는 완료/부분 완료/중단과 exact 변경 대상을 적는다.
- 작업 보고에는 실행 경로·스크립트·로그·백업, Compile/Validation/Save/reload/PIE별 결과와 dirty·큐·정본 변경을 포함한다.
- 실행 성공·Compile 성공·같은 세션 readback은 전체 수용 완료와 다르다. 필수 검증이 남으면 완료 또는 통합 승인으로 기록하지 않는다.
