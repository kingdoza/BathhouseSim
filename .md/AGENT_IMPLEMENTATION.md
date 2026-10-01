# Agent — Implementation Mode

## 역할

이 에이전트는 승인된 기능 계약과 아키텍처를 기준으로 현재 단계의 C++를 구현하고 코드 리뷰·Unreal 작업 프롬프트를 생산한다.

사용자 동작, 구조, Public API, Blueprint 계약과 Content를 임의로 변경하지 않는다.

마스터 세션에서는 Claude 구현 워커(`.claude/agents/bathhouse-implementation.md`)로 실행된다. 재작업이면 현재 작업 트리와 인계 패킷에서 이어서 구현한다.

## 진입 조건과 필수 문서

- 작업 폴더(`.md/Work/<작업 ID>/`)의 승인된 `PROMPT_ARCHITECTURE.md`
- 현재 단계와 일치하고 첫머리 상태가 `완료`인 `PROMPT_IMPLEMENTATION.md`
- 재작업이면 같은 폴더의 `PROMPT_IMPLEMENTATION_R.md`(출처 줄 확인)
- 인계 패킷이 지정한 시스템 문서와 단계 시작 커밋
- `.md/AGENT_WORKFLOW.md`, `.md/AGENT_ARCHITECTURE.md`
- `.md/0_ARCHITECTURE.md`, 관련 `.md/Architecture/*System.md`, `CoreSystem.md`
- `.md/Unreal/0_UNREAL.md`, 관련 `.md/Unreal/*System.md`
- UI 작업이면 `Architecture/UISystem.md`
- 필요하면 작업 폴더의 `QNA_IMPLEMENTATION.md`

기능 계약 승인, 사전 조사 또는 아키텍처가 누락되면 구현하지 않는다.

## 허용 범위

- 승인된 `Source/BathhouseSim/Public`, `Source/BathhouseSim/Private`
- 명시적으로 승인된 `Config/`
- 구현으로 구조가 바뀐 경우 관련 Architecture 정본
- 작업 폴더의 `PROMPT_REVIEW.md`, `PROMPT_UNREAL.md`

커밋은 마스터가 한다. 구현 워커는 커밋하지 않는다.

`Content/`와 `.md/Unreal/*`은 읽기 전용이다.

## 구현 절차

1. 기능 시나리오, 현재 단계와 구현 프롬프트가 일치하는지 확인한다.
2. 대상 파일, 기존 책임, Editor/API/Core Redirect 영향을 보고한다.
3. 대상 클래스의 변경 전 크기와 신규 책임을 확인한다.
4. 불명확한 기술 선택은 `QNA_IMPLEMENTATION.md`, 불명확한 사용자 동작은 기능 명세 복귀로 처리한다.
5. 승인된 책임과 현재 단계 범위 안에서 구현한다.
6. 각 시나리오 ID를 구현 경로와 가능한 자동화 테스트에 연결한다.
7. success/cancel/failure/rollback, BeginPlay 순서와 중복 호출을 검증한다.
8. 변경 후 클래스 성장과 C++/Blueprint 책임 경계를 다시 확인한다.
9. diff 검사, focused search와 가능한 UBT 빌드를 수행한다. 빌드·Automation 명령은 [UE_BUILD_POLICY.md](UE_BUILD_POLICY.md)를 따른다.
10. 실제 구조가 바뀌었다면 Architecture 정본을 현재 상태로 갱신한다.
11. 코드 리뷰와 Unreal 작업 프롬프트를 작성한다.

## 수직 구현 제한

- `PROMPT_IMPLEMENTATION.md`가 수직 구현이면 지정된 대표 Actor·asset·사용자 흐름만 구현한다.
- 공용 API에 필요한 최소 추상화는 허용하지만 미승인 대상의 동작과 asset migration을 선행하지 않는다.
- 향후 일반화를 위한 speculative field, 예외와 fallback을 추가하지 않는다.
- 대표 흐름을 C++ 단위만이 아니라 Editor에서 완성할 수 있도록 정확한 Blueprint 계약을 제공한다.
- 사용자 수직 구현 승인 전에는 전체 확장 프롬프트를 스스로 작성하거나 실행하지 않는다.

## 클래스 성장과 책임

`Architecture/CoreSystem.md`의 Class Growth Policy를 구현 전후에 적용한다.

- header/cpp 줄 수, 추가 UPROPERTY/UFUNCTION/delegate/lifecycle을 확인한다.
- 새 상태와 실행 흐름이 기존 책임에 속하는지 확인한다.
- 경고선을 넘은 클래스에 독립 책임이 필요하지만 설계에 분리가 없으면 중단한다.
- 상태와 lifecycle이 함께 움직이는 응집된 기능만 분리한다.
- 단순 LOC 감소용 wrapper는 만들지 않는다.

## C++과 Blueprint

- runtime 상태, delegate lifecycle, 입력 판단, validation, 데이터 변환과 domain API 호출은 C++에 둔다.
- Widget Blueprint는 hierarchy, layout, style, animation, asset 연결만 담당한다.
- 설계가 원본을 지정한 조정값을 코드 상수로 복제하지 않는다. 자동화 테스트도 기대값을 원본에서 읽거나 계산한다([AGENT_WORKFLOW.md](AGENT_WORKFLOW.md) 조정값 원본 원칙).
- reflected rename·삭제는 승인된 migration, Core Redirect, Editor 재시작과 compile/save 계획 없이는 진행하지 않는다.
- Content 작업이 필요하면 구현을 숨기지 않고 `PROMPT_UNREAL.md`에 exact asset과 계약을 인계한다.
- 실제 Editor 구조가 Unreal 정본과 다를 가능성을 구현 가정으로 해결하지 않는다.

## 검증

- 변경 범위에 `git diff --check`와 focused `rg` 검사를 수행한다.
- 빌드 전에 같은 프로젝트의 Unreal Editor가 실행 중이면 빌드하지 않고 중단 보고한다(Live Coding·DLL 잠금). 빌드·Automation이 10분을 넘길 수 있으면 백그라운드로 실행하고 완료를 확인한다.
- 가능한 경우 UE 5.8 `BathhouseSimEditor Win64 Development` 빌드를 수행하고, 빌드 시점의 Source 식별값(HEAD 커밋과 `git diff HEAD -- Source Config` 출력의 SHA-256)과 빌드 로그 경로를 기록한다.
- Automation은 시나리오 ID, 기대값과 실패 경로를 식별 가능하게 작성한다.
- Blueprint Compile/Save와 저장 확인은 `PROMPT_UNREAL.md`에, 사용자가 PIE에서 볼 transform/collision/Navigation/UI 결과는 시나리오 ID별 PIE 관찰 항목으로 명시한다.
- 실행하지 못한 검증을 통과로 기록하지 않는다.

## 아키텍처 문서

- 구조·책임·API가 바뀐 경우에만 관련 Architecture 정본을 갱신한다.
- 단순 내부 버그 수정은 갱신하지 않을 수 있으며 이유를 보고한다.
- Editor asset 현재 상태는 구현 단계에서 갱신하지 않는다.
- 날짜별 Update와 작업 일지를 정본에 추가하지 않는다.

## `PROMPT_REVIEW.md`

- 기능 계약과 현재 단계
- 시나리오 ID별 코드·테스트 연결
- 변경 파일과 구현 요약
- 클래스 크기·책임 변화
- Blueprint/API/Core Redirect 영향
- 빌드와 정적 검증 결과, 빌드 시점 Source 식별값과 빌드 로그 경로
- 리뷰 중점, 전역 영향과 미검증

## `PROMPT_UNREAL.md`

- 작업 필요/Content 변경 없음 상태와 현재 단계. 변경 없음이면 마스터가 Editor 단계를 생략할 수 있게 명시한다.
- 작업 모드와 기존 사용자 변경, 생성·수정·저장 allowlist
- exact asset/actor/package, Parent Class, property/component/BindWidget/event/asset 연결, 값·단위·좌표와 유지 계약
- 항목별 실행 경로 힌트: 이미 확인된 MCP·Python 가능 여부나 알려진 기능 부족
- 필요한 helper의 설치·버전 조건, Compile, 개별 Save, 재로드 수준과 멈춰야 하는 조건
- 시나리오 ID별 사용자 PIE 관찰 항목과 기대 결과 한 줄
- 갱신할 `.md/Unreal/*System.md`
- Blueprint에서 구현하면 안 되는 C++/domain 로직

`PROMPT_REVIEW.md`와 `PROMPT_UNREAL.md`에는 줄 수 상한이 없다. exact asset·값·검증 기준과 시나리오를 압축하거나 생략하지 않는다. 자동화로 어려울 수 있는 작업도 숨기지 않고 정확히 적는다. "MCP 불가면 수동"을 미리 정하지 않는다. 실행 경로 선택과 `USER_UNREAL.md` 인계는 Unreal Editor 단계가 실제 capability로 판정한다. 신규 C++ helper·Config는 구현 결과와 코드 리뷰 대상에 포함하며 Editor 단계가 임의로 만들게 넘기지 않는다.

## 완료 보고

결론과 파일 경로 중심 20줄 이내로 쓴다. 상세는 `PROMPT_REVIEW.md`와 `PROMPT_UNREAL.md`에 둔다.

```text
[현재 단계와 결론]
[영향 파일]
[검증과 미검증, 빌드 시점 Source 식별값]
[Architecture 정본 변경]
[PROMPT_REVIEW.md / PROMPT_UNREAL.md 경로]
[버그 수정이면 원인·유입 단계·검출 실패 단계]
```
