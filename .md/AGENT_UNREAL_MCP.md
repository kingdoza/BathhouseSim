# Codex Agent — Unreal MCP Mode

## 역할

이 에이전트는 연결된 Unreal MCP toolset으로 Editor의 실제 asset과 runtime 상태를 조사하거나, 코드 리뷰가 승인한 Blueprint·DataAsset·Level·StateTree·UI 작업을 수행한다.

MCP가 제공하지 않는 기능을 다른 UI 자동화로 우회하지 않는다. 실제 Editor 변경 후에는 관련 Unreal 정본을 저장된 현재 상태로 갱신한다.

## 절대 금지

- Computer Use 스킬, CUA tool, 화면 좌표 클릭, 마우스·키보드 자동화를 호출하지 않는다.
- MCP 실패나 기능 부족을 이유로 Computer Use에 자동 전환하지 않는다.
- `.uasset`과 `.umap`을 셸, 바이너리 패치, 텍스트 도구 또는 임의 commandlet 편집으로 수정하지 않는다.
- MCP에 없는 StateTree/Widget/graph 편집 기능을 Python reflection이나 asset serialization 우회로 만들어내지 않는다.
- 지원되지 않는 작업을 완료했다고 기록하지 않는다.

Computer Use는 사용자가 별도 작업으로 명시적으로 요청했을 때만 `.md/AGENT_COMPUTERUSE.md`에 따라 다른 단계에서 실행한다.

## 두 가지 진입 모드

### 사전 조사 모드

- 기능 명세 에이전트가 exact asset과 확인할 사실을 지정해야 한다.
- 코드 리뷰 승인은 필요하지 않다.
- Editor와 asset을 읽기 전용으로 조사하며 수정, Compile로 인한 dirty, Save와 PIE mutation을 만들지 않는다.
- 결과는 `.md/REPORT_UNREAL_DISCOVERY.md`에 기록하고 기능 명세 단계로 돌려보낸다.

### Editor 작업 모드

- 코드 리뷰 결론이 `코드 단계 승인`이어야 한다.
- 최초 작업은 `.md/PROMPT_UNREAL.md`, 재작업은 `.md/PROMPT_UNREAL_R.md`가 현재 구현과 일치해야 한다.
- 프롬프트의 exact asset allowlist와 수용 시나리오만 수정·검증·저장한다.
- 완료 후 `.md/PROMPT_INTEGRATION_REVIEW.md`와 관련 `.md/Unreal/*System.md`를 작성한다.

## 필수 문서

- `.md/AGENT_WORKFLOW.md`
- [UNREAL_MCP_CONNECTION.md](UNREAL_MCP_CONNECTION.md): Editor 선택·실행, MCP 연결·진단, 세션 종료 절차
- 사전 조사 모드는 기능 명세 초안 또는 exact 조사 요청과 관련 `.md/PROMPT_ARCHITECTURE.md`
- Editor 작업 모드는 승인된 `.md/PROMPT_ARCHITECTURE.md`와 현재 `.md/PROMPT_UNREAL.md` 또는 `.md/PROMPT_UNREAL_R.md`
- `.md/0_ARCHITECTURE.md`, 관련 `.md/Architecture/*System.md`
- `.md/Unreal/0_UNREAL.md`, 관련 `.md/Unreal/*System.md`
- UI 작업이면 `Architecture/UISystem.md`, rename/migration이면 `CoreSystem.md`
- 미완료 작업이 있으면 `.md/USER_UNREAL.md`

## 허용 범위

- 연결된 Unreal MCP가 명시적으로 지원하는 asset 조회·편집·Compile·validation·Save·PIE·로그 작업
- 프롬프트가 지정한 `Content/` asset과 Level
- Editor 구조 갱신을 위한 관련 `.md/Unreal/*System.md`
- 현재 단계 결과물과 미완료 작업을 위한 `.md/REPORT_UNREAL_DISCOVERY.md`, `.md/PROMPT_INTEGRATION_REVIEW.md`, `.md/USER_UNREAL.md`
- 세션 확인을 위한 read-only 파일·프로세스·로그 검사

`Source/`, `Config/`, `AGENT_*.md`, Architecture 정본과 입력 프롬프트는 수정하지 않는다.

## MCP Capability Preflight

1. [UNREAL_MCP_CONNECTION.md](UNREAL_MCP_CONNECTION.md)의 절차로 대상 Editor를 선택하고 실제 MCP 조회 호출까지 확인한다.
2. 연결 문서의 성공 기준을 충족한 세션에서 PIE 상태를 확인한다.
3. 현재 작업에 필요한 MCP tool을 열거하고 조회·수정·저장·검증 가능 여부를 작업 전에 판정한다. 현재 인계와 관련 Unreal 정본에서 동일 도구·대상의 알려진 실패 및 재개 조건을 먼저 확인한다.
4. `git status`, 대상 asset의 기존 변경과 Editor dirty package를 기준선으로 기록한다.
5. startup log의 Blueprint compile, missing component/property와 load error를 확인한다.
6. 필요한 tool이 없으면 가능한 항목과 불가능한 항목을 즉시 분리한다.

연결 실패와 현재 대화의 도구 미노출은 연결 문서에 따라 구분한다. 연결 확인 후에도 필요한 Editor 기능이 없으면 해당 작업을 `USER_UNREAL.md`로 인계한다. 읽기 전용 Source 분석으로 Editor 결과를 추측하지 않는다.

### 호출 전 스키마 확인

- 각 편집·생성·저장 도구의 최초 사용 전에 현재 세션의 `describe_toolset` 설명과 인자 스키마를 읽는다. 재연결·도구 변경 시 다시 확인한다. 인자명만으로 asset 유형, 부모 클래스, object/class 참조와 경로 형식을 추측하지 않는다.
- 인자 오류는 정확한 요청·오류와 스키마를 대조해 수정한 뒤 제한 내에서 재시도한다. 스키마에 맞는 호출의 내부 실패와 구분하며, 인자를 무작위로 바꾸거나 지원 여부를 모르는 요청을 묶어 실행하지 않는다.

## 세션 정책

Editor 소유권, 중복 실행 방지, 재시작·종료와 sharing violation 대응은 [UNREAL_MCP_CONNECTION.md](UNREAL_MCP_CONNECTION.md)를 따른다. 연결과 종료 절차를 이 문서에 중복 작성하지 않는다.

## 사전 조사 절차

1. 기능 명세가 지정한 질문과 관련 Unreal 정본을 확인한다.
2. MCP capability와 dirty 기준선을 기록한다.
3. Blueprint CDO, component hierarchy, asset binding, Level override, Project/World 설정과 현재 runtime 사실만 조회한다.
4. 사용자 기대, 실제값, 불일치와 MCP로 확인하지 못한 항목을 분리한다.
5. asset을 저장하거나 현재 상태를 교정하지 않는다.
6. `REPORT_UNREAL_DISCOVERY.md`에 exact 경로, 실제값, 근거와 미확정을 기록한다.

## Editor 작업 절차

1. `PROMPT_UNREAL.md`의 대상 asset, 시나리오와 저장 allowlist를 확정한다.
2. MCP capability와 dirty 기준선을 기록한다.
3. native Parent/API/component가 Editor에 실제 노출되는지 확인한다.
4. MCP가 지원하는 명시된 작업만 수행한다.
5. 대상 Blueprint Compile, Data Validation과 개별 Save를 수행한다.
6. 같은 세션에서 MCP가 지원하는 시나리오별 PIE·로그·수치 검증을 수행한다.
7. 저장 후 MCP reload로 asset 계약을 다시 확인한다.
8. 관련 `.md/Unreal/*System.md`를 재로드로 확인된 현재 상태로 갱신한다.
9. 불가능한 조작과 시각 판정은 `USER_UNREAL.md`에 남긴다.
10. dirty 기준선과 비교하고 `PROMPT_INTEGRATION_REVIEW.md`를 작성한다.

`Save All`을 사용하지 않고 allowlist asset만 개별 저장한다. 임시 actor/asset은 MCP로 안전하게 생성·제거할 수 있을 때만 사용하며 사용자 map 변경과 합쳐 저장하지 않는다.

### Level actor·World Partition 저장

- 배치·수정 전 대상 Level의 World Partition 여부, 개별 actor/package 저장 도구와 기존 실패 조건을 확인한다. 도구 존재만으로 저장 지원을 확정하지 않는다. 지원이 미확인이고 알려진 동일 차단점이 없다면 승인된 대상 하나로 저장·재로드를 확인한 뒤 나머지를 진행한다. 읽기 전용 조사에서는 생성·저장 시험을 하지 않는다.
- `save_actor`의 `Asset does not exist: /Game/__ExternalActors__/...` 오류는 과거 신규·기존 actor 모두에서 관찰됐다. 현재 도구·저장 경로가 같은 경우 다른 actor를 계속 생성해 재시험하지 않는다. 도구 수정, 지원되는 다른 개별 저장 경로, 해당 패키지 상태 변화 등 관련 조건이 바뀐 근거가 있을 때만 재확인한다.
- 오류에 나온 package 경로, 디스크 파일 존재, registry 조회 결과와 dirty 상태를 가능한 범위에서 구분한다. registry 조회 실패만으로 디스크 파일 부재나 MCP 구현 결함을 확정하지 않는다. 조회 불가는 미확인으로 기록한다.
- 저장 완료는 해당 external actor package의 저장 결과·dirty 해소와 디스크에서 재로드한 actor의 존재·핵심값 유지로 확인한다. 파일 존재, 메모리 readback, Map Save 성공 또는 Map dirty=false만으로 완료 처리하지 않는다. 미저장 사용자 상태를 잃는 reload는 수행하지 않는다.
- 저장 실패 시 배치 확대를 중단하고 현재 스키마가 지원하는 allowlist 내 개별 저장 경로를 확인한다. 지원 경로가 없거나 실패가 지속되면 exact actor/package, 도구·오류, 메모리/디스크 상태, 필요한 저장 조작과 재개 조건을 `USER_UNREAL.md`에 남긴다. 현재 실패를 모든 MCP 버전의 영구적인 저장 불가로 일반화하지 않는다.
- 이번 작업이 만든 미저장 임시 actor만 참조·기준선을 확인해 안전하게 정리한다. 사용자 actor·기존 변경을 삭제하거나 `Save All`로 해결하지 않는다. 실제 작업 상태의 유지·손실 및 작업용 Editor 종료는 연결 문서의 세션 정책을 따른다.

## Unreal 정본 갱신

- exact asset path, Parent Class, 핵심 component hierarchy와 C++ 계약 연결을 기록한다.
- 기능에 영향을 주는 Class Default, DataAsset, StateTree binding, Widget hierarchy, collision, transform과 World/Project 설정만 기록한다.
- Class Default와 Level instance override를 구분한다.
- 저장·재로드가 완료된 상태만 기록하며 예정값이나 날짜별 이력을 누적하지 않는다.
- 새 시스템 문서를 만들면 `.md/Unreal/0_UNREAL.md`의 라우팅 표에 실제 링크를 추가한다.
- 실제 asset과 문서가 충돌하면 asset을 자동으로 맞추지 않고 작업 범위와 승인 계약에 따라 판정한다.

## `USER_UNREAL.md` 인계

MCP가 수행할 수 없는 각 항목을 한국어로 작성한다.

- exact asset path와 현재 확인 상태
- 사용자가 수행할 클릭·선택·값 입력
- 기대 결과와 Compile/Save 절차
- 완료 확인 방법과 워크플로 재개 조건
- 자동화하지 못한 이유와 필요한 Editor 기능

미완료 항목만 유지한다. Computer Use 실행을 제안하거나 자동 호출하지 않는다. MCP 가능 작업까지 함께 넘기지 않는다.

## 실패 분류

- 프로젝트 오류: native/Blueprint 계약, compile, validation, PIE와 gameplay 문제
- Editor 상태 오류: stale class, dirty package, World Partition과 sharing violation
- 실행 환경 오류: engine/MCP mismatch, timeout, DDC/Zen과 세션 문제
- 호출 오류: 현재 스키마와 다른 인자·참조 형식. 올바른 요청으로 정정하기 전에는 기능 부족으로 판정하지 않는다.
- 권한·승인 거부: 실행·네트워크·화면 캡처 등 요청이 정책 또는 승인 검토에서 차단됨. 연결 실패나 도구 부재와 구분한다.
- 기능 부족: 현재 MCP toolset에 필요한 조회·편집·시각 기능이 없음

같은 실패는 원인 확인을 포함해 두 번까지만 시도하되, 재시도에는 바뀐 조건과 근거가 필요하다. 프로젝트 오류는 저장하지 않고 소유 단계로 돌려보내며, 기능 부족은 `USER_UNREAL.md`로 인계한다.

승인 거부는 동일 요청 반복·직접 HTTP·다른 캡처 경로로 우회하지 않는다. 차단된 요청과 반환된 사유를 보고하고 독립적으로 허용된 작업을 계속한다. MCP 캡처 도구가 있어도 호출이 거부됐다면 시각 확인은 미완료로 남긴다. 향후 정식 권한 변경 또는 정책상 허용된 승인 경로가 있을 때 재개하며, 지침 문서가 실행 권한을 대신하지 않는다.

## 결과물

사전 조사 결과는 `REPORT_UNREAL_DISCOVERY.md`에, Editor 작업 결과는 `PROMPT_INTEGRATION_REVIEW.md`에 기록한다. 후자에는 완료/부분 완료/중단, exact 변경 asset, Compile/Save/reload/PIE 결과, Unreal 정본 변경, dirty package와 `USER_UNREAL.md` 항목을 포함한다.

MCP가 확인하지 못한 PIE 또는 시각 수용 기준이 필수이면 상태를 완료로 기록하지 않는다.
