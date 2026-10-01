# CONTEXT — EMPTY-BOX-TAKE-TARGET 빈 박스 빼기 대상 선택 통일
- 목표 / 상위·선행·관련 작업: 빈 품목 박스로 진열물을 뺄 때 채운 박스와 같은 진열/빼기 조준 영역을 쓰고, 그 안에서 화면 중앙 조준점에 가장 가까운 물품을 빼기 대상으로 삼는다. 상위 작업 없음. 선행: 서비스 1·2단위(`5b42a47`, `c9a1150`, DISP-023·024, VANI-011·012, SHWR-008). 관련: `BUG-2026-10-01_litter_tongs_false_take_highlight`(완료, `4111e20`).
- 현재 단계와 재개 지점: 기능 명세 `진행`(사용자 최종 승인 대기). QNA 답변(Q1 A·VANI-012 폐기 승인, Q2 A, Q3 A, P1~P9 이의 없음, S1~S3 A)과 Editor 사전 조사 결과(P1 성립, Content 조정 불필요)를 `PROMPT_ARCHITECTURE.md`에 반영 완료. 수직 구현 불필요(직접 전체 경로, 화장대·샤워기 한 번에)로 확정. 다음: 사용자가 `PROMPT_ARCHITECTURE.md`를 최종 승인하면 승인 일자를 아래에 적고 `PROMPT_ARCHITECTURE.md`·`QNA_FEATURE_SPEC.md` 상태를 `완료`로 바꾼 뒤 아키텍처 단계로 넘긴다. `REPORT_UNREAL_DISCOVERY.md`는 Editor 역할이 저장 대기 중(명세는 인계된 조사 사실로 작성).
- 명세 승인 일자와 사전 허용: 미승인(최종 승인 대기). 사전 허용 답변: S2 A(승인 범위 밖 asset 수정 필요 시 멈추고 묻기), S3 A(사용자 PIE 통과 보고 시 자동 `--no-ff` 병합).
- 작업 브랜치, 단계별 시작 커밋, 리뷰 승인 커밋: 기능 명세 시작 커밋 `fcc982d`(인계 패킷 기준). 기능 명세 워커는 별도 worktree(HEAD `ae81e98`, `fcc982d`의 부모이며 차이는 `SERVICE-U4` 작업 폴더 문서와 `USER_UNREAL.md`뿐, Source·Content 동일)에서 작성했다. 명세 확정(답변·조사 반영) 시작 커밋 `2c24374`(메인 트리, `work/SERVICE-U4`). `fcc982d`→`2c24374` Source 변경은 세신 포커스·테스트뿐이며 진열 대상 선택 코드·Content는 같다.
- 리뷰 회차, 아키텍처 자동 복귀 사용 여부, 생략한 단계와 근거: 없음
- 복귀 기록: 없음
- 워커 세션 ID, 사용자 지시 모델 덮어쓰기: 없음
- 결과물 목록과 사용자 지시 요약: `PROMPT_ARCHITECTURE.md`(진행), `QNA_FEATURE_SPEC.md`(진행, Q1~Q3·P1~P9·S1~S3 답변 기록됨), `REPORT_UNREAL_DISCOVERY.md`(Editor 역할 저장 대기). 사용자 요청 원문은 `PROMPT_ARCHITECTURE.md` 첫 절에 있다.
