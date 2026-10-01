# CONTEXT — EMPTY-BOX-TAKE-TARGET 빈 박스 빼기 대상 선택 통일
- 목표 / 상위·선행·관련 작업: 빈 품목 박스로 진열물을 뺄 때 채운 박스와 같은 진열/빼기 조준 영역을 쓰고, 그 안에서 화면 중앙 조준점에 가장 가까운 물품을 빼기 대상으로 삼는다. 상위 작업 없음. 선행: 서비스 1·2단위(`5b42a47`, `c9a1150`, DISP-023·024, VANI-011·012, SHWR-008). 관련: `BUG-2026-10-01_litter_tongs_false_take_highlight`(완료, `4111e20`).
- 현재 단계와 재개 지점: 기능 명세 `진행`. `QNA_FEATURE_SPEC.md` 사용자 답변 대기. 답변 뒤 `PROMPT_ARCHITECTURE.md`의 조건부 문장(Q1~Q3 연쇄)을 확정하고, Editor 사전 조사(`PROMPT_ARCHITECTURE.md` "Editor 사전 조사 요청") 결과를 반영한 뒤 사용자 승인을 받는다.
- 명세 승인 일자와 사전 허용: 미승인
- 작업 브랜치, 단계별 시작 커밋, 리뷰 승인 커밋: 기능 명세 시작 커밋 `fcc982d`(인계 패킷 기준). 기능 명세 워커는 별도 worktree(HEAD `ae81e98`, `fcc982d`의 부모이며 차이는 `SERVICE-U4` 작업 폴더 문서와 `USER_UNREAL.md`뿐, Source·Content 동일)에서 작성했다.
- 리뷰 회차, 아키텍처 자동 복귀 사용 여부, 생략한 단계와 근거: 없음
- 복귀 기록: 없음
- 워커 세션 ID, 사용자 지시 모델 덮어쓰기: 없음
- 결과물 목록과 사용자 지시 요약: `PROMPT_ARCHITECTURE.md`(진행), `QNA_FEATURE_SPEC.md`(진행, Q1~Q3·P1~P9). 사용자 요청 원문은 `PROMPT_ARCHITECTURE.md` 첫 절에 있다.
