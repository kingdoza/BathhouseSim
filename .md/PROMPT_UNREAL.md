# Unreal Editor 인계 — 컴퓨터 포커스 진입·이탈 수정

## 현재 단계와 진입 조건

- C++ 구현과 focused automation 코드는 작성했다. 코드 리뷰 승인 뒤 Editor authoring·Compile/Save·재로드·PIE를 수행한다.
- 본 구현 단계에서는 Content, Config, Level을 저장하지 않았다. 이 문서는 후속 Editor 단계용 지침이다.
- 사용자가 쿨러·순환기 작업 완료를 확인하고 기존 작업 프롬프트 덮어쓰기를 승인했다. 컴퓨터 Editor handoff는 이 표준 파일에 기록하며, 쿨러·순환기 Source/Content 변경은 이 구현에서 수정하지 않았다.

## 백업과 load gate

백업은 Editor 종료 상태에서 완료했다.

| 파일 | SHA-256 |
|---|---|
| `Content/Bathhouse/Blueprints/Computer/BP_BathhouseComputer.uasset` | `A74BEE23F4C4A5F1B2C48379E629DC22900DEE9FD9AF75313CCB89559ADDC89E` |
| `Content/__ExternalActors__/Maps/DefaultMap/7/EH/E4FLO971KSWUJ40H7W7PHK.uasset` | `B9C38D4E99091451E9375016BDB18592790FB91AF5BEE3FAE8A6E071D2590BD0` |

처음 시도한 복사본 gate는 DDC `Installed` graph에 writable node가 없어 asset load 전에 Fatal로 중단됐다. 재실행에 `-DDC-ForceMemoryCache`를 추가하자 engine이 memory fallback으로 시작해 copy·원본·DefaultMap 검증을 완료했다. 로그에는 graph 설정 오류가 fallback 메시지와 함께 남지만, load gate에서 Fatal, `Serial size mismatch`, `Failed to load`는 발생하지 않았다.

| 단계 | 결과 | 기록 |
|---|---|---|
| Template_Default 복사본 | 통과, 1/1 성공·경고 0 | `Saved/Reimplementation/Computer/BlueprintLoad_Copy_ForceMemoryCache/BlueprintLoad_Copy.log`, `Saved/Reimplementation/Computer/BlueprintLoad_Copy_ForceMemoryCache/Report/index.json` |
| 복사본 정리 | 완료 | `Content/Developers/MigrationCheck`와 임시 `.uasset` 삭제. Content status에서 임시 사본이 없음 |
| Template_Default 원본 | 통과, 1/1 성공·경고 0 | `Saved/Reimplementation/Computer/BlueprintLoad_Original/BlueprintLoad_Original.log`, `Saved/Reimplementation/Computer/BlueprintLoad_Original/Report/index.json` |
| DefaultMap | 통과, 1/1 성공·경고 0 | `Saved/Reimplementation/Computer/BlueprintLoad_DefaultMap/BlueprintLoad_DefaultMap.log`, `Saved/Reimplementation/Computer/BlueprintLoad_DefaultMap/Report/index.json`; World instance 1개 확인 |

세 단계는 native parent, CDO `FocusExitPoint`, `ScreenWidget` class, blend 값, 그리고 DefaultMap instance의 `FocusExitPoint`·`ManagedBathPlacementZone` 참조를 확인했다. `-BathhouseComputerRequireWorldInstance` 조건도 통과했다.

Blueprint load용 Content 사본은 삭제했다. Content의 작업 전 Utility asset/external actor 변경 목록은 그대로이고 Config 변경은 없다. 두 백업 asset의 현재 hash도 위 기록과 일치한다. Editor authoring·Blueprint Save·Level Save·PIE는 아직 수행하지 않았으며, 아래 Editor 작업 순서를 따른다.
## 대상 자산과 authoring

| 자산 | 작업 |
|---|---|
| `/Game/Bathhouse/Blueprints/Computer/BP_BathhouseComputer` | `FocusExitPoint` 위치·방향과 `FocusExitSearchRadiusCm`을 class default로 authoring. 새 FocusExitPoint는 모니터 앞 바닥에 발바닥 위치를 두고 모니터를 보게 한다. Editor-only Arrow의 +X를 바라보는 방향으로 확인. 고정 위치의 player capsule이 벽·책상과 겹치지 않는지도 확인한다. |
| `/Game/Maps/DefaultMap.DefaultMap:PersistentLevel.BP_BathhouseComputer_C_UAID_F02F7433CA3690F802_2051456727` | class default를 우선 사용. instance override가 필요하면 World Partition 저장 기준을 확인한 뒤 해당 external actor만 개별 저장 |
| `/Game/FirstPersonCharacter/BP_FirstPersonCharacter` | `CancelAction`에 `/Game/Input/Actions/IA_Cancel`을 지정 |
| `/Game/Input/IMC_FirstPerson` | 기존 `IA_Interact` mapping context에 Escape → `IA_Cancel` 추가. 프로젝트 serialized name 참조 확인에서 이 context와 `IA_Interact`가 확인됐으며 Editor에서 실제 mapping을 확인한 뒤 저장 |
| `/Game/FirstPersonCharacter/BP_FirstPersonController` | `DefaultMappingContext`가 `IMC_FirstPerson`을 사용하는지 확인 |

`IA_Cancel`은 Digital bool로 만든다. `CancelAction`이 null이어도 C++가 정상 동작해야 한다. 사용자 Editor의 PIE 종료 키 F2는 Editor 설정으로 두고 프로젝트 asset에서 변경하지 않는다.

기존 컴퓨터 연결 정본은 `/Game/Bathhouse/UI/WBP_BathWaterManagementScreen.WBP_BathWaterManagementScreen_C`, `ManagedBathPlacementZone`은 같은 Level의 `BP_FacilityPlacementZone_C_UAID_F02F7433CA36D1FF02_1155169559`다. Compile·저장·재로드 뒤 이 연결, ScreenWidget, 0.35/0.25초 blend가 보존되는지 확인한다.

## Compile, 개별 저장, 재로드

- `BP_BathhouseComputer`와 입력을 authoring한 Blueprint만 Compile한다. 오류가 없으면 대상 Blueprint를 개별 Save한다. Save All은 사용하지 않는다.
- DefaultMap instance override가 없다면 Level/external actor를 저장하지 않는다. override가 필요하면 위 exact external actor만 저장한다.
- 새 Editor session에서 재로드하고 Blueprint parent, FocusExitPoint/Arrow, WidgetClass, ManagedBathPlacementZone, blend 값을 재확인한다(CMP-018).

## PIE 수용

- 포커스인 완료 뒤 화면을 클릭하지 않고 E/ESC 각 한 번으로 나가는지, 진입에 사용한 E의 release는 유지되는지, FocusingIn 중 E/ESC가 종료를 시작하는지 확인한다.
- 화면 버튼 클릭 후, 슬라이더 조작 후, 슬라이더 drag 중 이탈한다. LMB가 release되고 나간 뒤 setting 값이 더 바뀌지 않아야 한다.
- 커서가 나타나는 순간 화면 중앙인지 확인한다. 구석으로 옮긴 뒤 재진입해 중앙으로 돌아오는지, 중앙 배치가 click을 만들지 않는지 본다.
- 서로 다른 세 위치·방향에서 진입해도 같은 고정 발바닥 위치·방향인지, 화면은 0.25초 동안 그 시점으로 blend하는지 본다.
- 고정 위치의 손님/물건 blocker, 벽 너머가 더 가까운 배치, 반경 내 전체 blocker에서 탐색·같은 쪽 경로·Forced 경로를 확인하고 blocker가 이동하지 않는지 확인한다.
- 고정점 30cm 위 authoring에서 낙하 후 이동되는지, 바닥 높이에서는 떨어지지 않는지 확인한다.
- 비사용 ESC가 placement, circulator lever, recovery hold를 취소하지 않는지 확인한다. 완료 후 컴퓨터에 다시 조준해 재진입한다.

## 저장 경계·정본 갱신

- Save allowlist: `BP_BathhouseComputer`, 실제 override가 필요한 경우 해당 DefaultMap external actor, 그리고 신규 `IA_Cancel`/`IMC_FirstPerson`/Character Blueprint.
- 기존 managed Zone actor와 DefaultMap.umap은 기능에 필요한 변경이 없으면 저장하지 않는다. 다른 dirty package가 보이면 Save 전에 원인을 분리한다.
- 실제 저장·재로드한 상태만 `.md/Unreal/InteractionUISystem.md`에 반영한다. Editor 작업에서 수행하지 못한 조작과 정확한 사유는 해당 단계에서 `.md/USER_UNREAL.md`에 인계한다.
