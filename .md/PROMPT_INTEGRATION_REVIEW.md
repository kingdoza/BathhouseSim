# Unreal MCP 단계 보고 — held target use (2026-09-28)

## 판정

**부분 authoring 완료; 통합 완료 판정 보류.** MCP로 `IA_SecondaryUse`를 생성·저장하고 `BP_FirstPersonCharacter.SecondaryUseAction`을 연결·저장했다. `IMC_FirstPerson`의 RMB binding과 `WBP_InteractionPrompt` hierarchy는 미완료다. 사용자가 PIE와 검증을 제외했으므로 Compile, Data Validation, reload, PIE 결과는 없다.

## 연결과 세션 종료

- 제한 실행의 첫 시도는 Turnkey 단계에서 실패·정지했고 MCP 요청까지 도달하지 못했다. 종료한 뒤 저장소의 MCP 연결 절차에 맞춘 elevated Editor 실행으로 재시도했다. Turnkey 오류의 근본 원인은 확정하지 않았다.
- UE 5.8.3 작업 Editor PID 24128이 `127.0.0.1:8000` listener를 소유한 것을 확인했다. MCP `initialize`와 protocol `2025-11-25` 협상, `tools/list`, `list_toolsets`, read-only asset query가 성공했고 19개 toolset을 확인했다.
- 작업 후 정상 창 종료 요청은 실패했다. 작업 소유 PID 24128만 종료했고, 종료 후 UnrealEditor 프로세스와 8000 listener가 없음을 확인했다.
- Computer Use, 화면 캡처, Python reflection, asset binary 편집, Compile, Data Validation, 새 프로세스 reload와 PIE는 수행하지 않았다.

## Authoring 결과

| Asset | 결과 |
|---|---|
| `/Game/Input/Actions/IA_SecondaryUse` | 생성 후 `ValueType=Boolean`으로 설정했다. 불필요한 trigger/modifier는 추가하지 않았다. 개별 Save 성공. |
| `/Game/FirstPersonCharacter/BP_FirstPersonCharacter` | `SecondaryUseAction`에 위 InputAction을 지정하고 개별 Save 성공. `PrimaryUseAction`, `InteractAction`, `SecondaryInteractAction` 참조는 보존했다. |
| `/Game/Input/IMC_FirstPerson` | 변경·저장하지 않았다. 유효 `Mappings` 8개는 IA_Interact/E, IA_SecondaryInteract/F, IA_DropCarry/G, IA_PrimaryUse/LeftMouseButton, IA_RecoverFacility/Q, IA_PlacementSnap/LeftControl, IA_PlacementRotate/MouseWheelAxis, IA_Cancel/Escape였다. `defaultKeyMappings`는 유효 15개와 끝의 빈 None/None row를 반환했다. |
| `/Game/Bathhouse/UI/WBP_InteractionPrompt` | 변경·저장하지 않았다. 현 19개 toolset에 WidgetTree/hierarchy authoring tool이 없어 요구된 다섯 TextBlock을 추가할 수 없었다. |

- `IMC_FirstPerson`의 배열 변경 두 호출은 각각 `ArrayRemove: elements changed alongside the size change; removed elements are ambiguous` 및 `ArrayAdd: elements changed alongside the size change; insertion points are ambiguous`로 실패했다. 실패 뒤 package는 dirty=false였다. 따라서 이번 MCP 작업에서 해당 에셋의 부분 변경이나 저장은 없었다. 이 결과는 현재 ObjectTools 배열 경로의 제한이며 모든 MCP 버전의 보편적 불가를 뜻하지 않는다.
- `IMC_FirstPerson.uasset`은 이 작업의 MCP 편집 전부터 Git 변경 상태였다. 이번 작업은 해당 에셋을 저장하지 않았다. 앞선 별도 Editor 로그에는 빈 Input Action mapping 저장 시 `A mapping cannot have an empty input action!` 오류가 기록되어 있어, 이 빈 row를 수동 정리 인계에 포함했다. 그 별도 변경의 작성 주체는 확인하지 않았다.
- Save 후 MCP dirty-state 조회에서 IA, BP, IMC, WBP 모두 dirty=false였다. 이는 현재 Editor 내 dirty 상태만 확인하며 새 프로세스 reload나 디스크 재로드 검증은 아니다.

## 미완료 및 후속 조건

- `IMC_FirstPerson`: 기존 8개 binding을 보존하고 RightMouseButton → IA_SecondaryUse를 추가한다. `defaultKeyMappings`의 기존 유효 15개를 보존하면서 끝의 빈 row를 제거하고 개별 Save한다.
- `WBP_InteractionPrompt`: 프롬프트에 지정된 다섯 TextBlock과 키 행 계층을 authoring한다. 기존 native parent 및 필수 BindWidget 이름·타입을 보존한다.
- 위 MCP 미지원 authoring은 `.md/USER_UNREAL.md`에 정확한 수동 단계로 인계했다. 새 저장 상태가 reload로 확인되기 전에는 `.md/Unreal/InteractionUISystem.md`를 갱신하지 않는다.
- Compile·Data Validation·reload·PIE는 사용자가 이번 요청에서 제외했다. 실행하거나 통과했다고 기록하지 않았다. 남은 authoring을 포함해 이번 통합 단계는 완료로 판정하지 않는다.
