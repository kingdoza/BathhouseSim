# Unreal 인계 — 세신 포커스 때수건 표시 재작업

- 작업 ID: `SERVICE-U4`
- 단계: 구현
- 상태: 완료

## Content 변경 없음

- 이번 재작업은 C++ 세션 표시 상태와 자동화 테스트·Architecture 정본만 변경한다. Content 변경·재저장 없음.
- 생성·수정·저장 allowlist는 비어 있다. `BP_ScrubTowel`, `BP_ScrubTable`, `BP_FirstPersonCharacter`, DefaultMap을 저장하지 않는다. Blueprint graph 우회도 하지 않는다.
- private Transient weak 추가이며 Parent Class, component 이름·hierarchy, asset 연결·authoring 값·BindWidget·Blueprint event·기존 API와 Core Redirect는 바뀌지 않는다.
- Blueprint Compile/Save, asset migration, Editor Python/MCP/helper 실행이나 Unreal 정본 갱신이 필요 없다. 마스터가 Content diff 없음 확인 후 Editor 작업 단계를 생략할 수 있다.
- [ServiceSystem.md](../../../Unreal/ServiceSystem.md)의 저장된 authoring 값을 유지한다. 현상 2의 축 매핑·`UpdateCursor`와 `ScrubArea`/카메라 값은 이번 수정 대상이 아니다.
- UE 5.8 전체 빌드와 Automation 54/54가 통과했다(Service 34, Computer 3, Interaction 17; 세신 Amenity 12 포함). namespace·상품 수 assertion 차단 해결은 [QNA_IMPLEMENTATION.md](QNA_IMPLEMENTATION.md), 검증 식별값·로그는 [PROMPT_REVIEW.md](PROMPT_REVIEW.md)에 기록했다. Content 작업 없이 사용자 PIE 관찰로 넘길 수 있다.

## 사용자 PIE 관찰 항목

기존 [PIE_CHECKLIST.md](PIE_CHECKLIST.md) 10행의 현상 1에 해당한다. 에이전트는 PIE를 시작하지 않았다.

| 경로 | 관찰·기대 결과 |
|---|---|
| SCRB-005~007/011/016 진입 | 때수건을 들고 누운 손님이 있는 세신대에 E 진입. 진입 순간부터 손의 때수건은 사라지고 커서 때수건 하나만 보인다. G로 내려놓을 수 없고 carry 상태는 유지된다. |
| SCRB-007/011 E·ESC 이탈·재진입 | 이탈 입력 직후 손에 때수건이 다시 보이고 커서가 숨겨진다. 재진입하면 다시 커서 하나만 보인다. |
| SCRB-008 완료 자동 이탈 | 게이지 완료 직후 커서는 숨겨지고 손에 때수건이 다시 보인다. 현금·slot·손님 처리는 기존 계약을 유지한다. |
| SCRB-009 만료 자동 이탈 | 대기 만료 이탈 직후 손에 때수건이 다시 보이고 커서가 숨겨진다. |
| 사용자 상실·쓰러짐 | 기존 테스트 인형 Remove/Knockdown 명령으로 이탈할 때 손의 때수건이 다시 보인다. |
| SCRB-013 이탈 후 | G drop과 전용 거치대 E store가 정상이며 월드·거치대의 때수건이 보인다. |

실제 화면에서 하나만 보이는지와 이탈 직후 렌더 결과는 headless Automation으로 대체할 수 없다. 현상 2는 기존 체크리스트 11행의 별도 사용자 관찰로 유지하며 코드 수정 결과로 통과 처리하지 않는다.
