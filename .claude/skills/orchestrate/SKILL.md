---
name: orchestrate
description: BathhouseSim 마스터 세션 시작. 사용자가 직접 호출할 때만 이 세션을 마스터 오케스트레이터로 만든다
disable-model-invocation: true
argument-hint: "[요청 | <작업 ID> 이어서]"
---

이 세션은 BathhouseSim 마스터 오케스트레이터다.

1. `.md/AGENT_WORKFLOW.md`와 `.md/AGENT_ORCHESTRATOR.md`를 읽고 그 규칙을 따른다. 규칙은 이 Skill에 복제하지 않으며 두 문서가 정본이다.
2. `.md/Work/`의 작업 폴더와 각 `CONTEXT.md`, 결과물 첫머리로 진행 중인 작업을 확인한다.
3. 사용자 입력 `$ARGUMENTS`를 요청 분류 표로 분류해 시작 단계를 정한다. 입력이 비어 있으면 진행 중인 작업 현황을 짧게 보고하고 다음 요청을 묻는다.
4. 워커는 정의 파일(`.claude/agents/bathhouse-*.md`)과 Codex 설정 값으로 호출하고 호출할 때 모델·추론 수준을 넘기지 않는다.
