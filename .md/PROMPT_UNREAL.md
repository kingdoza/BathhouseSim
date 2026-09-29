# Unreal Editor 인계 — held target use

## 단계와 현재 상태

- C++ 구현·UE 5.8 build·copy-first load gate·Automation 검증은 성공했다. 총 74개 전체 회귀: 성공 68, 경고 포함 성공 6, 실패 0, 미실행 0.
- C++ 구현 단계에서는 Content/Config/Level을 저장하지 않았고, 그 단계 시작 시 BP_FirstPersonCharacter/WBP_InteractionPrompt의 원본 hash가 같았다. 이후 MCP Editor authoring에서 IA_SecondaryUse와 BP_FirstPersonCharacter를 저장했다.
- 2026-09-28 MCP 작업 결과: IA_SecondaryUse 생성·Boolean 설정·개별 저장 완료. BP_FirstPersonCharacter의 SecondaryUseAction 설정·개별 저장 완료. IMC_FirstPerson의 RMB mapping과 WBP_InteractionPrompt hierarchy는 MCP 도구 한계로 미완료다.
- 사용자가 제외한 PIE와 검증은 수행하지 않았다. Compile, Data Validation, 새 Editor 프로세스 reload도 수행하지 않았으며 완료로 표시하지 않는다.
- 작업용 백그라운드 Editor는 MCP 작업 후 종료했고, 종료 확인 시 UnrealEditor 프로세스와 8000 listener가 없었다.
- Save allowlist: 아래 IA_SecondaryUse, IMC_FirstPerson, BP_FirstPersonCharacter, WBP_InteractionPrompt만 계약에 필요한 변경이 있을 때 각각 개별 저장한다. Config와 Level은 저장하지 않는다.

## Input Action 및 Mapping Context

| Asset | 현재 위치·상태 | authoring |
|---|---|---|
| Input Action | /Game/Input/Actions/IA_SecondaryUse 생성, Boolean, trigger/modifier 없음, 개별 Save 완료. 기존 /Game/Input/Actions/IA_SecondaryInteract는 F 계약으로 유지했다. | 완료. Compile·검증·reload는 제외 지시에 따라 미수행이다. |
| Mapping Context | /Game/Input/IMC_FirstPerson. MCP readback에서 유효 mapping 8개를 확인했다. `defaultKeyMappings`에는 기존 유효 row 15개와 끝의 빈 None/None row가 있었다. | 미완료. ObjectTools 배열 편집은 모호한 삽입·삭제 오류로 거부됐다. 기존 mapping을 보존하고 RightMouseButton → IA_SecondaryUse를 추가하며 빈 row를 제거해야 한다. 이번 작업에서 수정·Save하지 않았다. 시작 시점부터 Git 변경 상태였으므로 이번 변경으로 귀속하지 않는다. |

Input Action은 저장했지만 IMC mapping은 도구 오류로 미완료다. IMC를 수정·Save하지 않았으므로 이 단계의 입력 연결은 완료되지 않았다.

## Character Blueprint

- Asset: /Game/FirstPersonCharacter/BP_FirstPersonCharacter
- Parent는 /Script/BathhouseSim.FirstPersonCharacter로 유지한다.
- Class Defaults의 SecondaryUseAction에 /Game/Input/Actions/IA_SecondaryUse를 지정한다.
- PrimaryUseAction, InteractAction, SecondaryInteractAction의 기존 asset 참조는 그대로 둔다.
- PlayerHeldTargetUse default subobject 이름과 RepeatIntervalSeconds 0.15를 유지한다. interval을 조정하는 별도 지시가 없으므로 기본값은 변경하지 않는다.
- SecondaryUseAction 참조를 설정하고 BP를 개별 Save했다. Blueprint Compile과 새 Editor 프로세스 reload는 사용자의 검증 제외 지시에 따라 미수행이다. PrimaryUseAction, InteractAction, SecondaryInteractAction은 유지했다.

## Interaction Prompt Widget Blueprint

- Asset: /Game/Bathhouse/UI/WBP_InteractionPrompt
- Existing native parent와 기존 필수 BindWidget 이름·타입을 보존한다: PromptRoot, TargetNameText, ActionNameText, FailureReasonText, SecondaryActionNameText, SecondaryFailureReasonText, InteractionProgressBar, EquipmentActionNameText, EquipmentFailureReasonText, EquipmentProgressBar, PlacementActionNameText, PlacementFailureReasonText, RecoveryActionNameText, RecoveryFailureReasonText, RecoveryProgressBar.
- 다음 다섯 TextBlock을 정확한 이름으로 추가해 적절한 키 행에 배치한다: HeldTakeActionNameText, HeldTakeFailureReasonText, PrimaryKeyText, LmbKeyText, RmbKeyText. Native BindWidgetOptional이므로 표시의 visibility/enabled 상태는 C++가 관리한다.
- 새 Event Graph gameplay logic은 추가하지 않는다. E 행은 action name이 없을 때 접히고, equipment가 없으면 LMB는 held Apply를 표시한다. RMB는 held Take와 해당 방향 실패 이유를 표시한다. F는 held target use에 쓰지 않는다. 기존 고정 E/LMB/RMB 글자가 중복되면 정리하고 native key label과 중복되지 않게 한다.
- 현재 MCP에는 WidgetTree/hierarchy authoring tool이 없어 다섯 TextBlock을 추가하지 못했다. WBP는 수정·Save하지 않았다. Compile과 새 Editor reload도 검증 제외 지시에 따라 미수행이다.

## PIE 수용 시나리오

사용자 지시에 따라 이번 Editor 작업에서는 PIE 및 검증을 수행하지 않는다. 아래는 후속 수용 기준이며 이번 단계의 실행 결과가 아니다.

| CTRL | 직접 확인 |
|---|---|
| 003 | Boiler/Cooler 투입구 조준 시 문 안내가 LMB 가능 상태와 함께 보이고, LMB Apply만 연료를 넣으며 기존 clamp/refusal이 유지된다. |
| 010, 011, 014 | RMB Take 및 LMB Apply를 누르고 있을 때 약 0.15초 단위로 수건 1장씩 이동한다. 통/바구니가 비거나 대상이 차면 멈춘다. |
| 019 | RMB 반복 중 target을 바꾸면 focus 이탈 시 멈추고 새 target에서 자동 재개되지 않는다. release 후 다시 누르면 새 반복이 시작된다. |
| 020 | 반복 중 basket drop 또는 Computer 진입 시 즉시 중단된다. 먼저 이동된 수건은 유지된다. |
| 025 | 여러 target/state에서 E/LMB/RMB key label, 가능한 action, 방향별 failure, empty E 행 접힘, target 이름과 F 비표시를 확인한다. |
| 026 | Monkey wrench, mop, delivery box 등 equipment/held item에서 기존 LMB 사용은 유지되고 RMB는 held target use를 시작하지 않는다. |
| 027 | Computer 입력 capture와 placement mode가 각각 LMB/RMB held-use보다 우선한다. |

추가로 CTRL-001~018의 supply, intake, shelf/bin/machine/floor towel 방향과 수량, CTRL-021/022/028의 기존 E·F 동작 보존을 실제 입력으로 표본 확인한다. Full automation은 해당 source 경로를 검사했지만 화면/입력 수용을 대신하지 않는다.

## 완료 후 기록

- 입력 asset과 Blueprint를 저장했다면 새 Editor 프로세스에서 reload를 확인한 뒤 실제 저장 상태만 .md/Unreal/InteractionUISystem.md에 반영한다.
- Compile, 개별 Save, reload와 PIE 수행 결과를 .md/PROMPT_INTEGRATION_REVIEW.md에 기록한다. 미확인 시나리오를 통과로 표시하지 않는다.
- 변경 asset 외에는 Save하지 않는다. 현재 PIE에서 확인되지 않은 기능을 완료로 표시하지 않는다.
