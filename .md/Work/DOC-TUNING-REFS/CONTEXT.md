# CONTEXT — DOC-TUNING-REFS 정본 문서의 조정값 숫자 복제 정리
- 목표 / 상위·선행·관련 작업: 사용자 지시(2026-10-01) "코드 상수를 쓰지 말고 원본 데이터 위치를 참조, 값 바꾸면 문서도 바꿔야 하잖아"에 따라, UNBOX-SPAWN-VIEW 아키텍처 재작업에서 범위 밖으로 남긴 Architecture 정본의 숫자 값을 원본 위치 참조로 정리한다. 관련: `UNBOX-SPAWN-VIEW`(브랜치 `work/UNBOX-SPAWN-VIEW`가 같은 문서의 Unbox·Tie 부분과 ShopSystem Settings 표 기본값 열을 이미 정리함).
- 현재 단계와 재개 지점: 사용자 PIE 대기(`PIE_CHECKLIST.md`). 통과 시 main `--no-ff` 병합 → 작업 폴더 제거 → `git push origin main`.
- 명세 승인 일자(자동/명시)와 사전 허용: 기능 명세 생략(사용자 동작 변화 없음, 값 보존). 사전 허용: 사용자 지시(2026-10-01) "DOC-TUNING-REFS 끝나면 이 작업까지 포함해서 메인으로 푸시해" — PIE 통과 후 main `--no-ff` 병합과 `git push origin main`(원격에 새 커밋이 있으면 멈추고 묻기).
- 작업 브랜치, 단계별 시작 커밋, 리뷰 승인 커밋: `work/DOC-TUNING-REFS`(worktree `.claude/worktrees/docrefs`, main에서 분기). 아키텍처 시작 `90f014c`. 구현 시작 `fc8e438`, 구현 `1e0bc27`, 재작업 `4b32f94` = 리뷰 승인 커밋.
- 리뷰 회차, 아키텍처 자동 복귀 사용 여부, 생략한 단계와 근거: 리뷰 2회(1회차 테스트 설정 복원·주석·정본 문구, 2회차 승인), 아키텍처 자동 복귀 미사용. 기능 명세 생략(동작 불변), Editor 작업 생략(Content·Config 무변경, `BP_UtilityShovel`에 던지기 값 override 없음 — 리뷰가 asset 이름표로 확인, 삽 놓기 속도는 PIE 4번으로 대체).
- 복귀 기록: 없음
- 사용자 지시 모델 덮어쓰기: 없음
- 결과물 목록과 사용자 지시 요약: 사용자 지시 — 이 작업만 진행하고 나머지 작업은 대기.
