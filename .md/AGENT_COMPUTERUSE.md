# Agent — Unreal Editor 화면 작업 모드 (Computer Use)

## 역할

[AGENT_UNREAL_EDITOR.md](AGENT_UNREAL_EDITOR.md) 역할의 화면 작업 절차다. 별도 에이전트가 아니며 같은 Unreal Editor 에이전트가 이 모드로 전환한다.

화면 작업은 마지막 수단이다. 스크린샷 기반 조작은 MCP·Python보다 토큰과 시간이 수십 배 들고 오조작 위험이 크다. 저장 결과와 미검증 항목은 `REPORT_UNREAL_EDITOR.md`로 인계한다.

## 진입 조건

- MCP, Python, 기존 helper, commandlet 등 허용된 자동화 경로를 모두 시도했고, 화면 작업 없이 끝낼 수 있는 항목은 이미 모두 끝냈다.
- 남은 후보를 대상 asset, 필요한 조작, 자동화 경로의 실패 근거, 예상 범위로 정리해 보고했고, 사용자가 작업 직전에 승인했다. 승인은 마스터가 받아 이 에이전트를 재개하며 전달한다.
- 명세 승인 때의 사전 허용, 이전 작업의 승인, `USER_UNREAL.md` 항목의 존재, MCP·Python 실패를 화면 작업 승인으로 간주하지 않는다.
- 현재 실행 환경이 native Unreal 앱의 관찰과 입력을 지원한다. 브라우저만 지원하면 진입하지 않는다.
- 사용자가 진단이나 일부 조작만 승인하면 그 범위와 완료 기준을 먼저 고정한다.

## 필수 문서

- `.md/AGENT_WORKFLOW.md`, `.md/AGENT_UNREAL_EDITOR.md`, `.md/UNREAL_MCP_CONNECTION.md`
- 작업 폴더의 `PROMPT_UNREAL.md`, 재작업이면 `PROMPT_UNREAL_R.md`
- 관련 `.md/Architecture/*System.md`와 `.md/Unreal/*System.md`; UI 작업이면 `Architecture/UISystem.md`
- 현재 작업과 관련된 `.md/USER_UNREAL.md` 항목
- 현재 설치된 Computer Use 스킬의 `SKILL.md`와 필수 참조 문서

## 허용 범위

- 승인된 항목의 `Content/` asset과 Level 화면 기반 조회·수정
- Details, Blueprint/Widget/StateTree editor, Compile, Data Validation과 개별 Save
- 저장·재로드한 현재 상태의 관련 `.md/Unreal/*System.md`, `REPORT_UNREAL_EDITOR.md`와 완료된 `USER_UNREAL.md` 항목 정리

PIE 시작·입력·플레이 시나리오 검증은 하지 않는다. `Source/`, `Config/`, `AGENT_*.md`, Architecture 정본과 입력 프롬프트는 수정하지 않는다. `.uasset`과 `.umap`은 Unreal Editor 밖에서 직접 편집하지 않는다.

## 실행 환경 Preflight

1. `.uproject` EngineAssociation과 workflow의 UE 버전을 확인한다.
2. `git status`와 대상 asset의 기존 변경을 기록한다.
3. 실행 중인 Editor와 commandlet를 확인하고 동일 project를 중복 실행하지 않는다.
4. 사용자 visible Editor가 있으면 project path와 version이 맞는 해당 세션을 우선한다.
5. 사용자 PIE/SIE, Live Coding, modal과 기존 dirty package를 확인한다. 사용자 PIE는 종료하지 않는다.
6. 현재 Computer Use 스킬의 초기화·관찰 API를 사용하고 과거 세션의 window ID나 좌표를 재사용하지 않는다.
7. 화면을 관찰할 수 없으면 입력을 시작하지 않는다.

## 화면 관찰과 입력

- 최신 화면 상태에서 대상 창, tab, panel과 control을 확인한 뒤 입력한다. 클릭·드래그 좌표는 최신 screenshot에 근거해야 한다.
- scroll, popup, tab 전환, compile과 modal 이후 화면을 다시 확인한다. 텍스트 입력 전 편집 영역의 focus를 확인한다.
- 접근성 text만으로 시각 결과를 추측하지 않는다. target을 식별할 수 없으면 임의 좌표를 누르지 않는다.
- 예상과 다른 modal, compile error 또는 dirty asset이 나타나면 입력을 중단하고 분류한다.

## 토큰 절약

- 스크린샷은 축소 비율을 지정해 작게 찍고, 작은 글자는 필요한 영역만 확대해 읽는다. 전체 화면을 반복해서 찍지 않는다.
- 결과를 예측할 수 있는 연속 조작은 일괄 실행으로 묶어 중간 확인 스크린샷을 줄인다.
- 값 확인은 화면이 아니라 저장 뒤 MCP·Python 재로드 readback으로 한다.

## 에셋 작업과 저장

1. exact asset path, Parent Class, 기존 hierarchy와 native 계약을 확인한다.
2. 승인된 항목의 property, component, binding, layout과 asset 연결만 수정한다.
3. Blueprint에 C++ runtime/domain 로직을 우회 구현하지 않는다.
4. 대상 Blueprint를 Compile하고 오류·경고를 확인한다.
5. 지정 allowlist asset만 개별 Save하며 `Save All`을 사용하지 않는다.
6. 저장 후 MCP·Python으로 asset 또는 map을 재로드해 값과 연결이 유지되는지 확인한다.
7. World Partition은 지정된 external actor만 저장하고 예상 밖 package를 보존한다.
8. 사용자 visible Editor는 명시적 종료 요청 없이 종료하지 않는다.

## Unreal 정본 갱신

- Compile, 개별 Save와 재로드가 성공한 asset만 관련 `.md/Unreal/*System.md`에 반영한다.
- exact path, Parent, 핵심 component/binding, 기능 관련 default·override·collision·transform과 전역 설정만 기록한다.
- 날짜별 작업 이력, transient 상태와 저장되지 않은 예정값은 기록하지 않는다.
- 새 시스템 문서를 만들면 `.md/Unreal/0_UNREAL.md`에 실제 링크를 추가한다.

## `USER_UNREAL.md`

- 승인된 항목을 저장·재로드한 뒤 해당 미완료 항목을 제거한다.
- 실패하거나 일부만 완료한 항목은 현재 상태, 남은 조작과 재개 조건으로 갱신한다.
- 완료 이력을 누적하지 않고 미완료 작업만 유지한다.

## 실패 처리

- 프로젝트 오류, Editor 상태 오류, 실행 환경 오류와 화면 식별 실패를 구분한다.
- 같은 화면 동작 실패는 원인 확인을 포함해 두 번까지만 시도한다.
- 보이지 않는 control을 추측 입력하거나 별도 helper/protocol을 만들어 우회하지 않는다.
- 코드/API 문제가 원인이면 구현 단계로, 설계 문제면 아키텍처로 돌려보낸다.

## 결과 기록

`REPORT_UNREAL_EDITOR.md`의 화면 작업 부분에 승인 범위, 수정·저장한 exact asset과 변경 내용, Compile·Save·재로드 결과, 갱신한 Unreal 정본, 예상 밖 dirty package, 미검증과 남은 `USER_UNREAL.md` 항목을 적는다.
