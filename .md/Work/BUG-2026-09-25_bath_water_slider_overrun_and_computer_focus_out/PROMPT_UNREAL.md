# PROMPT_UNREAL — 욕탕 관리 슬라이더 한계 초과·컴퓨터 클릭 없는 포커스아웃

- 작업 ID: `BUG-2026-09-25_bath_water_slider_overrun_and_computer_focus_out`
- 단계: 구현
- 상태: 완료

## 판정: Content 변경 없음 (Editor 작업 단계 생략 가능)

- 생성·수정·저장 allowlist: 없음. `Content/`, `Config/`는 수정하지 않았다(`git status Content` 변경 0건).
- `WBP_BathWaterDetail`의 slider는 0~1 범위·property binding 없음 그대로 쓴다. 손잡이 동기화는 C++(`UBathWaterDetailWidget`)가 맡으므로 Blueprint graph 보정이 필요 없다.
- reflected class·property·function·BindWidget 이름 변경 없음. Core Redirect 불필요.
- 갱신할 `.md/Unreal/*System.md`: 없음.

## 사용자 PIE 관찰 항목

선행: 이 브랜치의 빌드(`BathhouseSimEditor Win64 Development`)를 사용자 Editor에 반영한다. 정본은 `PROMPT_IMPLEMENTATION.md` 9절이며 요지만 옮긴다.

| ID | 관찰 | 기대 |
|---|---|---|
| SLD-001/002 | 순환 설치 정격용량이 순환도 100% 미만. 순환도 슬라이더를 끝까지 끌고 계속 오른쪽으로 | 손잡이가 한계에서 멈추고 `순환 용량 제한: N 포인트 부족` 표시. 놓은 뒤 손잡이와 순환도 수치 일치 |
| SLD-003 | 목표 수온을 실온 위(가열 부족)·아래(냉각 부족)로 같은 방법 | 손잡이가 `UBathWaterSettings.TargetTemperatureStepC`(Project Settings > Game > Bath Water) 단위의 확정값에 멈춤 |
| SLD-004 | 설비 비가동(가동수치 0), 설치용량 안에서 올림 | 제한 없이 설정, 용량 요약·상세에 가동 부족 표시 |
| SLD-005 | 한계에서 아래로 되돌려 끌기 | 손잡이가 따라오고 제한 문구 사라짐, 다른 욕탕 불변 |
| SLD-006/CMP-005 | 한계 너머로 끄는 중 E로 나감, 재진입 | 손잡이는 확정값 |
| CMP-001 | 컴퓨터 진입 후 아무것도 클릭하지 않고 E (3회) | 고정 위치·방향으로 나옴 |
| CMP-006 | E를 누른 채 전환 대기 후 뗌, 이어서 E 한 번 | 떼도 나가지 않고, 다음 E로 나감 |
| CMP-004 | 탭·타일·슬라이더 클릭 후 E, 재진입 | 선택·값 유지 |

CMP-001이 이 빌드에서도 실패하면 `PROMPT_IMPLEMENTATION.md` 9절의 조건부 진단(Widget Reflector의 User 0 Focus 경로, `showdebug enhancedinput`)을 첨부해 아키텍처로 복귀한다.

## Blueprint에서 구현하면 안 되는 C++/domain 로직

- slider 확정값 되돌리기, 재진입 guard, 제한 피드백, 용량 제한 계산은 C++에 있다. WBP graph·property binding으로 우회하지 않는다.
- 컴퓨터 화면 widget에 `SetKeyboardFocus`·`SetUserFocus`·`SetFocusToGameViewport`·`SetInputMode`를 추가하지 않는다(Content 전체에 해당 노드 없음을 읽기 전용으로 확인함).
