---
name: feature-spec
description: BathhouseSim 기능 명세만 진행하는 일반 세션 시작. 사용자가 직접 호출할 때만 쓴다
disable-model-invocation: true
argument-hint: "[새 기능 설명 | <작업 ID> 이어서]"
---

이 세션은 BathhouseSim 기능 명세 단계만 수행하는 일반 세션이다. 마스터가 아니며 아키텍처 이후 단계를 호출하지 않는다.

1. `.md/AGENT_WORKFLOW.md`와 `.md/AGENT_FEATURE_SPEC.md`를 읽고 그 규칙을 따른다. 형식과 절차는 두 지침서가 정본이다.
2. 사용자 입력 `$ARGUMENTS`가 새 기능이면 작업 ID를 정하고 `.md/Work/<작업 ID>/`와 `CONTEXT.md`를 만든다. `<작업 ID> 이어서`면 `CONTEXT.md`와 결과물 첫머리로 현재 상태를 확인한다.
3. 필요하면 Unreal Editor 읽기 전용 조사를 요청할 수 있다.
4. 명세가 승인(자동 승인 포함, `AGENT_FEATURE_SPEC.md`)되면 승인 일자·방식과 사전 허용을 `CONTEXT.md`에 적고 결과물 상태를 `완료`로 바꾼다. 작업 폴더의 이 단계 결과물만 기본 브랜치에 `[작업 ID] 기능 명세: 요약` 메시지로 커밋한 뒤, `/orchestrate <작업 ID> 이어서`로 마스터 세션을 시작하라고 안내하고 끝낸다.
