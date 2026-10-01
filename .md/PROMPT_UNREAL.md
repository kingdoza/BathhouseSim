# Unreal Editor 인계 — 버그 수정: 집게를 들면 대상의 빼기 강조가 표시됨

## 상태

- **Editor 작업 없음.** Content·Config·Level을 수정·저장하지 않았고 asset 연결·Blueprint 계약도 바뀌지 않았다(`.md/PROMPT_REVIEW.md`).
- 변경은 C++ 질의 구조(추가 필드·intent)와 `UInteractionPromptWidget`의 RMB 행 source 선택뿐이다. 기존 `HeldTakeActionNameText`, `HeldTakeFailureReasonText`, `RmbKeyText`를 그대로 쓰므로 `WBP_InteractionPrompt`에 widget을 추가하거나 바꿀 필요가 없다.
- 갱신할 `.md/Unreal/*System.md` 없음. 저장할 asset 없음.
- Editor·MCP 단계가 필요한 항목은 없다. 아래는 사용자 PIE 관찰 항목이다.

## PIE 관찰 항목

1. 집게(봉투 1개 이상)를 든 채 다음을 각각 조준한다. 모두 빼기 강조(외곽선)와 수건 기계 뚜껑 열림이 없어야 하고, 냉장고는 넣기 프리뷰도 없어야 한다. RMB 행은 `봉투 묶기`다.
   - 냉장고 진열 공간(재고 있음)
   - 수건 선반, 사용 수건통(수건 있음)
   - 세탁기·건조기(수건 있음)
2. 집게로 위 대상을 조준한 채 RMB를 누르면 봉투가 묶이고 대상 재고는 그대로다(TRSH-021). 조준하지 않고도 묶인다(TRSH-010).
3. 빈 집게(봉투 0개)에서도 강조가 없고 RMB 행은 `봉투 묶기` + 이유 `봉투가 비어 있음`이다. RMB를 눌러 실패하면 이유가 RMB 행에 1.5초 보인다.
4. 대조: 같은 대상을 품목 박스·수건바구니로 조준하면 기존대로 강조와 뚜껑·프리뷰가 보인다(DISP-006, TOWL). 이때 RMB 행은 `빼기`다.
