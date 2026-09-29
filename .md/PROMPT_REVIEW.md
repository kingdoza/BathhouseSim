# 코드 리뷰 입력 — 들고 있는 물건 조작 LMB/RMB 통일

## 단계와 범위

- 2026-09-28 승인된 기능 계약 Q1~Q7과 설계 정본을 기준으로 전체 C++ 구현을 완료했다. 구현 QNA 미해결 항목은 없다.
- 삽은 LMB held Apply 단발, 수건 이동은 LMB Apply/RMB Take 1장 단위와 Repeat로 변경했다. E의 손동작, F binding, 기존 equipment LMB는 유지했다.
- 이 구현 단계에서 Content, Config, Level, Architecture 정본을 수정·저장하지 않았다. Editor authoring과 PIE는 다음 단계다.
- 이전 작업 상태는 Character 복사본 BlueprintLoad 성공 직후였다. 첫 실행은 PowerShell 인수 인용 때문에 ReportExportPath가 문자 그대로 변수명으로 전달되어 report 파일을 만들지 못했다. 이것은 자동화 실패나 Blueprint 로드 실패가 아니며, 올바른 경로로 복사본을 다시 만들어 테스트와 report를 완료했다. 당시 남은 WBP/원본/맵/회귀 검증을 이 재개 작업에서 마쳤다.

## CTRL 요구사항 추적

| CTRL | 구현·자동화 근거 |
|---|---|
| 001~005 | FuelInteractionAtomicity, CoolerDryIceIntegration, FuelDoorFocusIntegration: supply/volume에서 기존 scoop·return·insert transaction을 Apply로 실행한다. 삽은 단발이며 초과분·재료 일치·가득 참·문 상태를 검증한다. |
| 006 | 연료 supply와 intake volume에서 E·F·RMB Take가 실패하고 삽 적재량을 보존한다. |
| 007 | InputOwners에서 빈 focus에 held target owner 동작이 시작되지 않고 기존 빈 공간 LMB fallback만 남는 것을 확인한다. |
| 008, 022, 028 | fixed-slot E, 수건바구니 거치와 기존 key/drop·trash 회귀를 보존한다. FuelDoorFocusIntegration 및 기존 Interaction/Shop 전체 회귀가 통과했다. |
| 009~018 | TowelRuleMatrix의 6개 대상 조건 × 6개 바구니 상태 × Apply/Take 표를 검증한다. AtomicTransferMachineAndRecovery와 CleaningTowelAutomationTests에서 한 장 이동, 토큰 보존, 기계 전이, 바닥 수건 소모, E/F 무변화를 확인한다. |
| 019~020 | RepeatLifecycle이 간격·한 Tick 1회·full stop·조준 이탈·자동 재개 금지·release 후 재시작·held object 변경·suppression과 이미 이동된 수량 유지를 확인한다. |
| 021~024 | 수건 대상 E/F 무변화, 거치대 E 유지, 다른 대상·작동 중 기계의 방향별 거부를 towel 회귀와 규칙 표에서 검증한다. |
| 025 | InteractionPromptPrimarySecondaryCapability, BlueprintLoad와 widget property metadata가 E 접힘, prompt root 조건, 선택적 RMB text widget, 기존 필수 BindWidget 보존을 확인한다. 실제 WBP의 신규 행·키 라벨 시각 배치는 PIE 미검증이다. |
| 026~027 | InputOwners가 equipment LMB 우선권과 RMB 무시, Computer/Placement 우선권, held-use 중 반대 버튼 무시를 확인한다. |
| 029 | RepeatLifecycle에서 interval을 0.3초로 바꿔 동일 실행 경로를 확인한다. |

## 변경 파일과 구현

- Interaction 계약/소유권: Public InteractionTypes, PlayerInteractable, PlayerInteractionComponent, PlayerEquipmentUseComponent와 구현 파일, Character의 Primary/RMB owner routing 및 SecondaryUseAction.
- 신규 lifecycle: Public/Private Interaction/PlayerHeldTargetUseComponent. Tick·press owner·반복·중단은 이 component가 보유한다. PlayerInteractionComponent에는 fresh focused interaction accessor만 추가했다.
- Target 변경: TowelHeldTransferRules helper와 clean stack, used bin, transfer port, floor towel target. FuelSupply 및 FuelIntakeVolume은 Apply query/execute와 기존 atomic transaction을 연결했다.
- HUD: InteractionPromptWidget에 E/LMB/RMB label, optional Take row, 방향별 transient failure와 visibility 규칙을 추가했다.
- 자동화: 신규 HeldTargetUseAutomationTests와 CleaningTowel, UtilityCoolerAndLever, UtilityFuelDoor, UtilityLaborFuel 및 test support를 갱신했다. ShopAutomationTests의 scoped fixture 이름 변경은 Unity 빌드 중 test helper symbol 충돌을 방지한다. TowelInventoryComponent/Computer/Placement 헤더의 test friend 변경은 테스트 접근 전용이다.

## 책임과 호환성

- 신규 PlayerHeldTargetUseComponent는 header 49줄/cpp 225줄, 순수 TowelHeldTransferRules는 49줄/154줄이다.
- FirstPersonCharacter.cpp는 485줄, InteractionPromptWidget.cpp는 436줄이다. Component tick/repeat 상태를 이미 571줄인 PlayerInteractionComponent에 추가하지 않았다.
- enum 값은 끝에만 추가했다. PlayerHeldTargetUse default subobject 이름과 기존 reflected property, PrimaryUseAction/InteractAction/SecondaryInteractAction을 유지했다. reflected rename/class 변경, native Serialize 변경, Core Redirect는 없다.
- BlueprintLoad가 native parent, default subobject와 0.15초 기본값, 기존 Character action 참조, WBP 필수 BindWidget 목록을 검사했다. 두 원본 Content asset은 저장하지 않았다.

## 빌드·복사본 로드·회귀 검증

- UE 5.8 BathhouseSimEditor Win64 Development Build.bat: 최신 테스트 변경 후 6 actions 성공.
- Copy-first load gate: BP_FirstPersonCharacter 복사본 1/1, WBP_InteractionPrompt 복사본 1/1. 복사본만 제거했다. 개별 report는 Saved/Automation/Reports/2026-09-28/HeldTargetUseCopyCharacter/index.json, HeldTargetUseCopyWBP/index.json.
- 원본 BP_FirstPersonCharacter 및 WBP_InteractionPrompt 개별 로드: 각각 1/1 성공. DefaultMap 시작 맵 로드와 BlueprintLoad: 1/1 성공. Fatal, Serial size mismatch, package Failed to load가 없었다.
- 복사본 전·후 및 최종 Content 상태는 깨끗하다. 원본 BP SHA-256 678E8D88193605449213F34993F545417BED3165C0B901DF226885B818C8A215, WBP SHA-256 41827D52BEAF385FF5D9390F4EBBF363C996E6ECC2B27D3774943DF2B166F337. 두 값은 사전 백업과도 같다.
- HeldTargetUse focused: 4/4 성공, 경고 0, 실패 0. UI prompt focused: 1/1 성공. Utility Labor focused: 8 total, 성공 5, 경고 포함 성공 3, 실패 0.
- 전체 Automation RunTests BathhouseSim: 74 total, 성공 68, 경고 포함 성공 6, 실패 0, 미실행 0. 경고 이벤트 16건은 Placement 설정/locker, towel presentation, Boiler Blueprint CDO fallback, fuel test mobility, invalid provider 회귀가 기록한 진단이다. 경고가 있는 테스트도 모두 Success다.
- 전체 report: Saved/Automation/Reports/2026-09-28/BathhouseSimFull/index.json. 집중 report: HeldTargetUseFocused, BathhouseSimUIPrompt, BathhouseSimUtilityLabor 폴더의 index.json. 로그는 Saved/Logs/HeldTargetUse_Focused.log, BathhouseSimUIPrompt.log, BathhouseSimUtilityLabor.log, BathhouseSim_Full.log 및 개별 copy/original log에 있다. 구현 산출물과 인계 문서의 diff 검사는 통과했다. 전체 working tree 검사에서 변경 상태였던 범위 밖 문서 `.md/NextWork/QNA_FEATURE_SPEC.md:503`의 EOF 빈 줄 지적 1건은 남아 있다.

## 미검증과 리뷰 중점

- 아직 IA_SecondaryUse 생성, IMC_FirstPerson RMB 연결, BP_FirstPersonCharacter SecondaryUseAction 지정, WBP optional widget authoring을 하지 않았다. PROMPT_UNREAL.md에 정확한 경로와 저장 범위를 인계했다.
- PIE 입력 감각, 방향별 HUD 행과 키 라벨, 실제 0.15초 연속 동작, target 전환·drop·Computer/Placement 우선순위는 직접 확인하지 않았다.
- 리뷰는 fresh focus 재검증/transaction, one action per Tick, release 전 재개 금지, held equipment field clearing, E/F compatibility, reflected property와 Blueprint asset load gate를 확인한다.
