# 코드 리뷰 요청 — 컴퓨터 포커스 진입·이탈 수정

## 단계와 범위

- 기능 계약: `.md/PROMPT_ARCHITECTURE.md`의 컴퓨터 포커스 CMP-001~020 승인본.
- 구현 계약: `.md/PROMPT_IMPLEMENTATION.md`의 2026-09-26 컴퓨터 포커스 수정.
- 재구현 프롬의 백업, UE 5.8 Editor 빌드, Blueprint load gate, focused·full automation을 실행했다. 최초 load 시도는 DDC 초기화 Fatal로 시작 전에 중단됐으며, `-DDC-ForceMemoryCache`로 같은 gate를 재실행해 완료했다.
- 최종 자동화에서 새로 드러난 컴퓨터 세션 테스트 fixture의 회전 초기화 누락을 보완하고, 빌드와 focused·full automation을 다시 실행했다.
- Content·Config·Level은 저장하지 않았다. load gate용 임시 `.uasset` 사본은 만든 뒤 제거했다. 기존 쿨러·순환기 및 Utility 변경은 작업 전 상태 그대로 두었다.
- 사용자가 쿨러·순환기 작업 완료를 확인하고 문서 덮어쓰기를 승인했다. 이 문서와 `.md/PROMPT_UNREAL.md`가 컴퓨터 구현의 표준 결과물이다.

## 수정 요약

- `CompleteFocusIn`: widget keyboard focus를 제거하고 `FInputModeGameAndUI` → 게임 viewport focus → viewport 중앙 마우스 배치 → hit testing 활성화 순서로 바꿨다. 기존 FXAA override 위치는 유지했다.
- `RequestEndComputerUse`: FocusingIn/Active에서 timer·AA·pointer·hit test·cursor·input mode를 정리하고, 컴퓨터별 발바닥 지점과 플레이어 capsule 충돌로 고정/탐색/강제 경로를 구한다. Capsule center로 sweep 없는 `TeleportPhysics` 이동, actor yaw와 controller pitch/yaw를 설정한 뒤 owner pawn으로 blend한다.
- `FComputerFocusExitPlacement`: 10cm 간격 수평 고리, 고정 yaw 시작 각도, capsule channel/response를 사용하는 overlap·sweep, C0 초기 blocking component 무시를 구현했다. 다른 Actor는 옮기지 않는다.
- `ABathhouseComputerActor`: root 하위 `FocusExitPoint`, Editor-only child `FocusExitArrow`, 100cm 기본 반경과 값 검증 getter를 추가했다. 기존 ComputerMesh/ScreenWidget/FocusCamera/ManagedBathPlacementZone 및 blend field는 유지했다.
- `AFirstPersonCharacter`: `CancelAction`과 Started-only `CancelInput` route를 추가했다. 비컴퓨터 상태에는 전달하지 않는다. E press ownership 코드는 바꾸지 않았다.
- 재검증 중 `ComputerAutomationTests.cpp`의 두 재진입 fixture가 이전 exit의 pawn yaw를 초기화하지 않는 것을 확인해 actor rotation을 명시적으로 reset했다. runtime 구현 코드는 이 수정에서 바꾸지 않았다.

## 시나리오와 자동화 연결

| 시나리오 | 코드·자동화 |
|---|---|
| CMP-001, 003, 004, 009 | `ComputerAutomationTests.cpp`: 정상 E 이탈, FocusingIn에서 E 이탈, 세 시작 위치·방향에서 같은 고정 위치·방향. 화면 widget instance 보존도 확인 |
| CMP-002, 014, 015 | `CancelInput`: Active에서 같은 이탈, Inactive/FocusingOut 무반응. 다른 mode로 intent를 전달하지 않는 코드 경로 |
| CMP-005 | pointer down 뒤 이탈, pointer 상태 release, hit testing off, 이후 pointer press 거부 |
| CMP-006 | 기존 E press ownership 회귀 유지 |
| CMP-011~013 | `BathhouseSim.Computer.FocusExitPlacement`: fixed/free, capsule blocker 탐색, 벽 반대편 배제, 모두 막힌 Forced, 반경 0, 높이 보정 없음, 장애물 위치 불변 |
| CMP-016 | 이동 모드·interaction 복구, 예약 해제, 재진입 회귀 유지 |
| CMP-017 | `HandleComputerUnavailable`, Actor EndPlay와 controller loss에서 teleport 없이 복구 |
| CMP-018 | `BathhouseSim.Computer.BlueprintLoad`: native parent, CDO FocusExitPoint/Arrow, screen class, 반경/blend, 로드된 world instance의 FocusExitPoint/ManagedBathPlacementZone 확인 |
| CMP-019, 020 | helper 검사에서 목표 capsule center Z가 foot Z + scaled half height와 일치. 낙하·실제 바닥 수용은 PIE 인계 |
| CMP-007, 008, 010 | headless에서 실제 Slate focus, cursor 위치와 blend 외형 판정 불가. CompleteFocusIn 코드는 viewport focus·중앙 배치 호출 순서를 실행하며 수용은 PIE로 인계 |

focused 최종 실행은 `BathhouseSim.Computer` prefix로 3개 테스트를 선택해 모두 통과했다. 최초의 `BathhouseSim.Computer.*` filter는 UE 5.8에서 wildcard로 처리되지 않아 0개를 선택했고, prefix 실행으로 바로잡았다.

## 변경 파일과 클래스 성장

- `Public/Computer/BathhouseComputerActor.h` 64→81줄, `Private/Computer/BathhouseComputerActor.cpp` 161→199줄.
- `Public/Computer/PlayerComputerUseComponent.h` 101줄(변경 없음), `Private/Computer/PlayerComputerUseComponent.cpp` 427→492줄. Placement geometry는 별도 private helper에 분리했다. 종료 session owner 책임을 유지한다.
- `Public/Character/FirstPersonCharacter.h` 186→190줄, `Private/Character/FirstPersonCharacter.cpp` 449→462줄. 입력 property/binding/intent만 추가했다.
- 신규 `Private/Computer/ComputerFocusExitPlacement.h/.cpp` 34/166줄, `Private/Tests/ComputerBlueprintLoadAutomationTests.cpp` 148줄. `ComputerAutomationTests.cpp`는 구현 전 418줄에서 613줄로 늘었다.

## Blueprint/API·migration 영향

- 신규 reflected 이름은 `FocusExitPoint`, editor-only `FocusExitArrow`, `FocusExitSearchRadiusCm`, `CancelAction`이다. 기존 이름을 rename/delete하지 않았고 Core Redirect는 추가하지 않았다.
- `FocusExitArrow`는 `WITH_EDITORONLY_DATA`에 남기고 `BlueprintReadOnly`를 제거했다. 백업은 `Saved/MigrationBackup/20260926_computer/`에 완료했고 아래 재검증에서도 두 asset의 현재 hash가 백업과 일치했다.
- Module Rules, Slate/SlateCore, Config, Level은 변경하지 않았다. Content에는 기존 Utility 변경이 남아 있으며 이번 작업에서는 건드리지 않았다.

## 재구현 검증 결과

### 백업

| 파일 | SHA-256 |
|---|---|
| `Content/Bathhouse/Blueprints/Computer/BP_BathhouseComputer.uasset` | `A74BEE23F4C4A5F1B2C48379E629DC22900DEE9FD9AF75313CCB89559ADDC89E` |
| `Content/__ExternalActors__/Maps/DefaultMap/7/EH/E4FLO971KSWUJ40H7W7PHK.uasset` | `B9C38D4E99091451E9375016BDB18592790FB91AF5BEE3FAE8A6E071D2590BD0` |

### 빌드

- 재구현 후 UE 5.8 `BathhouseSimEditor Win64 Development` 빌드 성공, exit code 0. 수정된 `ComputerAutomationTests.cpp` compile, module link와 target metadata 생성이 완료됐다. UBT 로그: `%LOCALAPPDATA%/UnrealBuildTool/Log.txt`.
- 첫 빌드에서 `ComputerFocusExitPlacement.cpp`의 `FOverlapResult` 불완전 형식 오류를 확인해 `Engine/OverlapResult.h` include를 추가했다. P2 `FocusExitArrow` 수정도 UHT 오류 없이 통과했다.

### Blueprint load gate

DDC `Installed` graph에 writable node가 없는 시작 환경은 그대로다. `-DDC-ForceMemoryCache` 적용 후 engine이 memory fallback으로 시작했고 아래 세 load test가 통과했다. 성공 로그에도 DDC graph 오류와 memory fallback 메시지는 남지만, 테스트 대상 Blueprint의 Fatal·`Serial size mismatch`·`Failed to load`는 없었다.

| 단계 | 결과 | 기록 |
|---|---|---|
| Template_Default 복사본 | 통과, 1/1 성공·경고 0 | `Saved/Reimplementation/Computer/BlueprintLoad_Copy_ForceMemoryCache/BlueprintLoad_Copy.log`, `Saved/Reimplementation/Computer/BlueprintLoad_Copy_ForceMemoryCache/Report/index.json` |
| 복사본 정리 | 완료 | `Content/Developers/MigrationCheck` 및 임시 `.uasset` 제거. `git --no-optional-locks status --short -- Content Config`에서 사본이 남지 않았고 기존 Content 변경 목록만 보임 |
| Template_Default 원본 | 통과, 1/1 성공·경고 0 | `Saved/Reimplementation/Computer/BlueprintLoad_Original/BlueprintLoad_Original.log`, `Saved/Reimplementation/Computer/BlueprintLoad_Original/Report/index.json` |
| DefaultMap instance | 통과, 1/1 성공·경고 0 | `Saved/Reimplementation/Computer/BlueprintLoad_DefaultMap/BlueprintLoad_DefaultMap.log`, `Saved/Reimplementation/Computer/BlueprintLoad_DefaultMap/Report/index.json`; world instance 1개 확인 |

Copy/original test는 native parent, CDO FocusExitPoint, ScreenWidget class와 blend 값 검사를 통과했다. DefaultMap은 `-BathhouseComputerRequireWorldInstance` assertion과 instance의 FocusExitPoint·ManagedBathPlacementZone 확인을 통과했다.

### 자동화와 기타 확인

- 최초 `Automation RunTests BathhouseSim.Computer.*` 실행은 0개 선택으로 끝났다. 기록: `Saved/Reimplementation/Computer/Automation_Computer/Automation_Computer.log`. 이후 UE가 인식하는 prefix `BathhouseSim.Computer`로 최종 focused suite를 실행해 3/3 성공, 경고 0, 실패 0, 미실행 0을 기록했다. 최종 report: `Saved/Reimplementation/Computer/Automation_Computer_Final/Report/index.json`; log: `Saved/Reimplementation/Computer/Automation_Computer_Final/Automation_Computer_Final.log`.
- 수정 뒤 전체 `Automation RunTests BathhouseSim`은 Template_Default에서 58개 실행: 성공 50, 경고 포함 성공 5, 실패 3, 미실행 0. report: `Saved/Reimplementation/Computer/Automation_Full_Final/Report/index.json`; log: `Saved/Reimplementation/Computer/Automation_Full_Final/Automation_Full_Final.log`.
- 전체 실패 이름은 `BathhouseSim.Placement.ActorReplacementFailureAtomicity`, `BathhouseSim.Placement.ActorReplacementTransaction`, `BathhouseSim.Placement.SettingsZoneLeaseAndCompatibility`다. 기존 전체 로그 `Saved/Logs/BathhouseSim_Full_Final3_20260926.log`의 세 이름과 일치한다. 재구현 프롬 지시에 따라 이름만 대조했으며 무관 판정은 하지 않았다.
- Focused를 수정 전 실행했을 때 `FocusSessionSuppressionAndSampleScreen`에서 비영 blend 재진입 fixture 관련 assertion 6개가 실패했다. pawn actor yaw 초기화를 보완한 뒤 해당 테스트를 포함한 focused 3개와 full suite를 다시 실행해 컴퓨터 관련 실패가 없는 것을 확인했다.
- 백업 두 asset의 현재 SHA-256은 기록값과 각각 일치했다. 임시 load-copy는 제거됐고 기존 Content dirty 목록은 작업 전 상태와 같으며 Config status는 clean이다.
- `git diff --check`는 소스와 문서에 통과했다. PIE 전용 Slate focus, 화면 cursor 위치, blend 외형과 실제 바닥 수용은 미실행이다.

## 리뷰 중점

1. copy/original/DefaultMap report가 Blueprint parent·Widget·Zone·blend migration을 보존하는지.
2. `OverlapBlockingTestByChannel`과 sweep이 capsule object type·response를 동일하게 적용하는지, B0 component만 sweep에서 제외하는지.
3. 후보가 고리 거리순으로 선택되고 wall을 통과하지 않는지, 강제 배치에서 다른 Actor 위치가 유지되는지.
4. 정상 종료는 owner pawn으로 blend하고 abnormal cleanup 경로는 teleport하지 않는지.
5. `CompleteFocusIn` 순서와 ESC 비사용 동작을 PIE에서 수용하는지.