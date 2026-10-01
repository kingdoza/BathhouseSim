# CONTEXT — DOC-TUNING-REFS 정본 문서의 조정값 숫자 복제 정리
- 목표 / 상위·선행·관련 작업: 사용자 지시(2026-10-01) "코드 상수를 쓰지 말고 원본 데이터 위치를 참조, 값 바꾸면 문서도 바꿔야 하잖아"에 따라, UNBOX-SPAWN-VIEW 아키텍처 재작업에서 범위 밖으로 남긴 Architecture 정본의 숫자 값을 원본 위치 참조로 정리한다. 관련: `UNBOX-SPAWN-VIEW`(브랜치 `work/UNBOX-SPAWN-VIEW`가 같은 문서의 Unbox·Tie 부분과 ShopSystem Settings 표 기본값 열을 이미 정리함).
- 현재 단계와 재개 지점: 구현(Claude 구현 워커, 메인 트리). `PROMPT_IMPLEMENTATION.md` 보류 조건 충족 — 사용자 재개 지시(2026-10-01 "대기중인 작업 재개"), UNBOX-SPAWN-VIEW main 병합 완료. 구현 시작 `da8349c`(main 병합). 문서 충돌 3곳은 마스터가 계획대로 해결(ShopSystem Settings 표는 UNBOX 표 + 코드 상수 이전 예정 문장, Verification은 UNBOX 행 + 이 작업의 상자 scale 행, CleaningLitter 집게 값은 BagCapacity 정본 참조).
- 명세 승인 일자와 사전 허용: 기능 명세 생략 — 사용자 동작 변화 없음(문서 정리, 값 보존 리팩터 설계). 사용자 지시 2026-10-01. 사전 허용: 범위 밖 asset 수정 필요 시 멈추고 묻기, PIE 통과 후 병합 여부는 미정(코드 변경이 생기면 물음).
- 작업 브랜치, 단계별 시작 커밋, 리뷰 승인 커밋: `work/DOC-TUNING-REFS`(worktree `.claude/worktrees/docrefs`, main에서 분기). 아키텍처 시작 `90f014c`.
- 리뷰 회차, 아키텍처 자동 복귀 사용 여부, 생략한 단계와 근거: 0회, 미사용, 기능 명세 생략(위 근거).
- 복귀 기록: 없음
- 사용자 지시 모델 덮어쓰기: 없음
- 결과물 목록과 사용자 지시 요약: 사용자 지시 — 이 작업만 진행하고 나머지 작업은 대기.
