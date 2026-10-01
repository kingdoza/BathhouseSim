# 코드 리뷰 입력 — 버그 수정: 집게를 들면 대상의 빼기 강조가 표시됨

## 단계와 범위

- 버그 `.md/BugReports/2026-10-01_litter_tongs_false_take_highlight.md`(서비스 3단위 `2c14d8f`의 설계 결함). 단순 버그 수정, 전체 구현 경로. 기능 계약은 바뀌지 않는다(DISP-006·007·013, TOWL 프리뷰·강조·뚜껑, TRSH-010·021·022·024 유지).
- 시작 조건(`Source`·`Content`·`Config` 변경 없음) 충족. Content·Config·`.md/Architecture/*` 수정 없음. 설계와 달라서 멈춘 항목 없음.

## 원인

- `UPlayerEquipmentUseComponent::MergeEquipmentQuery`가 장비 보조 사용(집게 `봉투 묶기`) query를 `HeldTake*` 필드에 채웠다.
- focus observer 세 곳(`UDisplaySpaceComponent`, `TowelDisplayCueUtils`, `UTowelTransferPortComponent`)은 합성된 query의 `bHeldTakeVisible && bCanHeldTake`로 강조·뚜껑을 켠다. 그래서 봉투가 1개 이상이면 대상과 무관하게 강조가 켜졌다.

## 변경 파일

- `Public/Interaction/InteractionTypes.h`: `EPlayerInteractionIntent` 끝에 `EquipmentSecondaryUse` append. `FPlayerInteractionQuery`에 `bEquipmentSecondaryVisible`, `bCanEquipmentSecondary`, `EquipmentSecondaryActionName`, `EquipmentSecondaryFailureReason`(BlueprintReadOnly, `Equals` 포함).
- `Private/Interaction/PlayerEquipmentUseComponent.cpp`: `MergeEquipmentQuery`는 held-use Apply·Take 비우기를 유지하고 보조 사용 query를 새 필드에 채운다. `ExecuteSecondaryEquipmentUse` 결과 intent를 `EquipmentSecondaryUse`로 변경(다른 흐름 불변).
- `Private/UI/InteractionPromptWidget.cpp`: RMB 행 source를 `bEquipmentSecondaryVisible`이면 새 필드, 아니면 held Take로 선택해 기존 `HeldTakeActionNameText`·`HeldTakeFailureReasonText`·`RmbKeyText`에 쓴다. transient failure 두 곳(발생·해제)에 `EquipmentSecondaryUse`를 같은 RMB slot으로 추가. root 표시·enabled 판정에 새 필드 포함. BindWidget·Blueprint event 불변.
- 수정하지 않은 것: focus observer 세 곳, 수건·진열 규칙, `UPlayerInteractionComponent`, `UPlayerHeldTargetUseComponent`, Character 입력 순서, 집게 행동, `HeldTake*` 의미.
- 테스트: `CleaningLitterToolAutomationTests.cpp` 단언을 새 필드로 변경. 신규 `CleaningLitterTongsCueAutomationTests.cpp`. 신규 테스트가 private 멤버를 쓰므로 `TowelQuantityVisualComponent.h`·`TowelVisualMeshProfile.h`·`TowelInventoryComponent.h`에 테스트 클래스 `friend` 선언만 추가했다(기존 cue 테스트와 같은 관례, 동작 영향 없음).

## 시나리오·버그 회귀 대응

| 항목 | 검증 |
|---|---|
| TRSH-010·012 | 기존 `Cleaning.Litter.TongsAndBag`(새 필드 단언: `bEquipmentSecondaryVisible`, 문구 `봉투 묶기`, `bHeldTakeVisible` false, 빈 봉투 이유 `봉투가 비어 있음`) |
| 버그 회귀(실제 `RefreshInteractionQuery` 경로) | 신규 `Cleaning.Litter.TongsDoNotDriveTakeCues`: 집게 봉투 3개를 들고 냉장고 진열 공간(재고 있음), 수건 선반, 사용 수건통, 대기 세탁기 port를 차례로 조준. 모두 강조 proxy·넣기 프리뷰 숨김, 세탁기 뚜껑 목표 닫힘, RMB 새 필드 `봉투 묶기` 가능이고 `bHeldTakeVisible/bCanHeldTake` false. `ExecuteSecondaryEquipmentUse`가 성공해 봉투가 묶이고 대상 재고 불변(TRSH-021), 결과 intent `EquipmentSecondaryUse`. 선반·수건통·세탁기에는 mesh profile을 지정해 cue가 실제로 그려질 수 있는 상태에서 검증(빈 profile이면 숨김 단언이 공허하다) |
| 대조(DISP-006, TOWL) | 같은 세탁기를 수건바구니로, 같은 냉장고 공간을 품목 박스로 조준하면 기존대로 강조·뚜껑·프리뷰가 켜지고 equipment secondary 행은 없음 |
| 위젯 | 위젯 Blueprint가 headless에 없어 query 단위로 대체: `Cleaning.Litter.TongsRmbRowQuery`가 `Equals` 포함, intent가 `HeldTake` 바로 뒤에 append됨, 빈 집게의 RMB 행(보이되 불가, 문구·이유), `HeldTake*` 비어 있음, 실패 result가 `EquipmentSecondaryUse`와 이유를 싣는 것을 확인. 위젯 표시 자체(RMB 행 문구, 1.5초 transient)는 PIE 확인 |

신규 회귀 테스트를 수정 전 코드에 돌려 실패를 확인하지는 않았다(수정 후 통과만 확인).

## 클래스 크기·책임

- `InteractionPromptWidget.cpp` 475 → 485줄. RMB source 선택 변수 몇 개와 case 추가만이며 책임 불변.
- `UPlayerEquipmentUseComponent`는 필드 채움 위치와 intent 값만 바뀌었다. `UPlayerInteractionComponent` 무변경.

## Blueprint·API·Core Redirect 영향

- 추가만: enum 값 append, USTRUCT 필드 4개(Transient 질의 구조, asset에 저장되지 않음). rename·삭제 없음, Core Redirect 없음. `WBP_InteractionPrompt`의 BindWidget·event 불변.

## 검증 결과

- **빌드**: UE 5.8 `Build.bat BathhouseSimEditor Win64 Development` 성공(`Saved/Logs/build_bug7.log`).
- **정적**: `git diff --check -- Source` 공백 오류 없음. `git status -- Content Config` 무변경.
- **로드**: struct·enum 추가는 asset에 저장되지 않고 BP native parent·subobject 변경이 없어 copy-first 대신 `/Game/Maps/DefaultMap`으로 `BathhouseSim.Service.BlueprintLoad`(`DA_ShopCatalog`, `WBP_InteractionPrompt`, `BP_Shower`) 1회 로드: 성공(`Saved/Logs/bug_load_defaultmap.log`).
- **집중**: `BathhouseSim.Cleaning.Litter` 전체 포함 회귀 통과. 신규 2개 성공.
- **전체 회귀** `Automation RunTests BathhouseSim`(headless, Template 맵): 118개 중 성공 108, 경고 포함 성공 10, 실패 0, 미실행 0. Cleaning·Service·Towel(`TowelDisplayCue` 포함)·Interaction held-use·Shop 모두 통과. 리포트 `Saved/Automation/Reports/20261001/bug_full`, 로그 `Saved/Logs/auto_bug_full.log`.

## 리뷰 중점

- `MergeEquipmentQuery`가 장비를 들었을 때 `HeldTake*`를 계속 비우고 보조 사용을 새 필드에만 채우는지.
- 위젯의 RMB 행이 equipment secondary와 held Take 중 하나만 쓰고, `EquipmentSecondaryUse` 실패가 같은 slot·타이머를 쓰며 `HeldTake`와 서로 지우지 않는지(해제 handler는 `HeldTake` intent로 같은 slot을 비운다).
- 다른 장비(`IHeldEquipmentSecondaryUsable`) 구현은 현재 집게뿐이라 영향 범위가 집게다.

## 미검증

- PIE: 집게로 냉장고·수건 선반·사용 수건통·세탁기·건조기를 조준할 때 강조·뚜껑 없음과 RMB 행 `봉투 묶기` 표시, 실패 시 1.5초 표시.
- 건조기는 세탁기와 같은 port 경로이며 자동화에서 직접 조준하지 않았다.
- 버그 리포트의 "미확인"(봉투 0개일 때, 집게 외 다른 물건)은 이번 수정이 `HeldTake*`를 장비 보조 사용으로 채우던 경로 자체를 없앴으로 해소된다고 판단하나 PIE에서 확인하지 않았다.
