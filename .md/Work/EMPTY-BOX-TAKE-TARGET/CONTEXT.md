# CONTEXT — EMPTY-BOX-TAKE-TARGET 빈 박스 빼기 대상 선택 통일
- 목표 / 상위·선행·관련 작업: 빈 품목 박스로 진열물을 뺄 때 채운 박스와 같은 진열/빼기 조준 영역을 쓰고, 그 안에서 화면 중앙 조준점에 가장 가까운 물품을 빼기 대상으로 삼는다. 상위 작업 없음. 선행: 서비스 1·2단위(`5b42a47`, `c9a1150`, DISP-023·024, VANI-011·012, SHWR-008). 관련: `BUG-2026-10-01_litter_tongs_false_take_highlight`(완료, `4111e20`).
- 현재 단계와 재개 지점: 구현(Claude 구현 워커, 메인 트리). PROMPT_IMPLEMENTATION의 worktree 경로 지시는 사용자 결정(빌드는 메인 트리 브랜치 전환)으로 무효.
- 명세 승인 일자와 사전 허용: 2026-10-01 사용자 승인("셋다 승인"). 사전 허용: 승인 범위 밖 asset 수정이 필요하면 멈추고 묻는다(S2 A), 사용자 PIE 통과 보고 시 자동 `--no-ff` 병합(S3 A).
- 작업 브랜치, 단계별 시작 커밋, 리뷰 승인 커밋: `work/EMPTY-BOX-TAKE-TARGET`(worktree `.claude/worktrees/emptybox`, main에서 분기). 아키텍처 시작 `fc5b59f`. worktree 제거 후 메인 트리, 구현 시작 `31fc8d0`(main 지침 병합).
- 리뷰 회차, 아키텍처 자동 복귀 사용 여부, 생략한 단계와 근거: 없음
- 복귀 기록: 없음
- 사용자 지시 모델 덮어쓰기: 없음
- 결과물 목록과 사용자 지시 요약: `PROMPT_ARCHITECTURE.md`(진행), `QNA_FEATURE_SPEC.md`(진행, Q1~Q3·P1~P9·S1~S3 답변 기록됨), `REPORT_UNREAL_DISCOVERY.md`(Editor 역할 저장 대기). 사용자 요청 원문은 `PROMPT_ARCHITECTURE.md` 첫 절에 있다.
