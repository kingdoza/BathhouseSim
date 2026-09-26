# 통합 검토 인계 — FuelIntake 로드 호환 migration

## 상태

**부분 완료, 통합 승인 불가.** 사용자가 이번 대화에서 코드 리뷰 승인 관문만 건너뛰도록 지시했다. 현재 작업의 Unreal MCP tool 목록에는 UE tool이 직접 노출되지 않았으나, UE 5.8 Editor의 로컬 Streamable HTTP MCP 서버에 연결해 정식 `initialize`/`tools/list`/`call_tool` 요청으로 작업했다. Computer Use, 임의 Python reflection, `Save All`은 사용하지 않았다.

## 실제 변경과 확인

- 변경·개별 저장한 Content는 `/Game/Bathhouse/Blueprints/Facility/BP_Boiler` 하나다. 기존 `FuelIntake : UStaticMeshComponent`의 class/이름과 Definition 참조는 유지했다. 새 `FuelIntakeVolume`은 `(0,-42,42)`/BoxExtent `(15,4,11)` cm/Scale `(1,1,1)`의 QueryOnly·Visibility Block 판정이며, `FuelIntake` Cube는 NoCollision 외형이다.
- `FuelDoorPivot`은 `(12.5,-37,42)`의 오른쪽 경첩, 자식 Cube `FuelDoorMesh`는 `(-12.5,0,0)`/Scale `(0.25,0.02,0.18)`/NoCollision이다. `FuelDoorPresentation`은 local Z축 `+90°`, 열기·닫기 `0.2s`다. 기존 계기 face/pivot/needle hierarchy와 Cube 연결은 보존했다.
- 첫 로드에서 BP parent `/Script/BathhouseSim.BathWaterBoilerFacilityActor`, 새 native component hierarchy와 기존 `/Game/Maps/DefaultMap` 보일러 instance를 확인했다. serial mismatch/load Fatal은 없었다. BP Compile 호출 뒤 관련 warning/error 로그 없이 해당 BP만 개별 Save했다. 같은 Editor에서 Level을 다시 로드하자 기존 보일러 instance가 새 Class Default를 상속했다.
- `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler`는 읽기 전용 확인했다. `PlacedFacilityClass=BP_Boiler_C`, `RecoveryItemClass=BP_PlaceableFacilityItem_C`이며 변경·저장하지 않았다. Map/World Partition external actor도 변경·저장하지 않았다.
- 짧은 자동 PIE에서 보일러 provider 등록이 성공했고 새 authoring 관련 오류는 없었다. PIE 종료 후 BP·Definition·Map dirty=false를 MCP로 확인했다. Git의 이번 Content 변경은 `BP_Boiler.uasset` 하나다. PIE teardown의 RecastNavMesh/StateTree 경고는 별도 원인 조사 대상이며 이 migration의 성공 판정 근거로 사용하지 않았다.
- `.md/Unreal/UtilityLaborSystem.md`를 저장·같은 세션 Level 재로드로 확인한 값으로 갱신했다.

## 미완료·리뷰 관문

- MCP toolset에 Data Validation 실행과 preview-open/restore-closed native 호출이 없다. 문·바늘의 실제 시각 자세도 판정하지 않았다.
- 새 Editor 프로세스로 재시작했을 때 포트는 열렸지만 MCP `initialize`가 시간 초과됐고 시작 로그가 Slate 초기화 뒤 진행되지 않았다. 새 프로세스 디스크 재로드 검증은 미완료다. 두 작업 소유 백그라운드 Editor는 종료했고 사용자 Editor는 종료하지 않았다. 복구 패키지 UI 여부는 확인되지 않아 원인을 단정하지 않는다.
- 실제 E/G/LMB/F/Q 입력, 두 보일러 독립 문 동작, 가열·소진·재투입·회수 rollback과 1024×576 UI는 직접 검증하지 않았다. 정확한 Editor 작업과 재개 조건은 `.md/USER_UNREAL.md`에 있다.
- 기존 Source·Architecture·프롬프트의 사용자 소유 변경은 수정하지 않았다. 전체 automation/빌드는 이 Editor 단계에서 다시 실행하지 않았다. Data Validation, 새 Editor 재로드와 직접 PIE 수용이 끝나야 통합 리뷰가 승인할 수 있다.

# Unreal MCP 인계 — 쿨러 전체 확장과 순환기 레버 (2026-09-26)

## 상태

**부분 완료, 통합 승인 불가.** 사용자는 코드 단계 승인을 지시했고, 현재 Editor 작업은 Unreal MCP만 사용했다. Computer Use와 Save All은 사용하지 않았다. 상세 재개 절차와 현재 instance 값은 .md/USER_UNREAL.md의 “쿨러 전체 확장과 순환기 레버” 항목에 있다.

## 저장·Compile 확인

- BP_Boiler, BP_Cooler, BP_Circulator, BP_UtilityShovel, BP_DryIceSupply를 allowlist 안에서 각각 authoring하고 개별 Save했다. 다섯 Blueprint 모두 warnings-as-errors Compile을 통과했고 Save 뒤 dirty=false였다.
- Cooler/Circulator는 지정 native parent로 reparent했다. CDO Capacity/Operation/footprint/body mesh를 유지했고 Cooler/Circulator Definition CDO 경로를 각각 설정했다. Boiler에는 GaugeFacePlate를 추가했다. Shovel CDO에는 Coal/DryIce 외형을 구성했고 DryIceSupply Blueprint를 만들었다.
- PIE 시작과 종료는 정상 동작했다. 실제 입력 상호작용은 수행하지 않았다.

## 남은 통합 관문

- 기존 DefaultMap instance는 새 CDO를 완전히 상속하지 않았다. Boiler GaugeFacePlate, Cooler/Circulator gauge/door/lever mesh·volume/presentation, Circulator CancelReturnSeconds=0, Shovel LoadAppearances가 level instance에서 stale하다. Cooler/Circulator FacilityPlacement.Definition도 각 instance에서 None이다. Instance setter/reset은 read-only 오류로 거부됐다.
- DefaultMap의 신규 DryIceSupply actor는 (450,-900,0)에 생성했으나 external actor package 저장이 실패했다. SaveActor는 두 번 모두 Asset does not exist: /Game/__ExternalActors__/Maps/DefaultMap/B/E4/YYD7GB5C924IGGI6PAL42T를 반환했다. DefaultMap 개별 Save 응답은 true였지만 map은 clean이었고 external package는 존재하지 않았다. actor 영속은 확인되지 않았다.
- Data Validation tool과 CallInEditor PreviewDownPose/RestoreUpPose 호출 tool이 없다. E 입력 주입도 없어 fuel/lever/door/visual acceptance를 실행하지 못했다.
- 작업 소유 Editor는 PID 25024, 숨김 window handle 0이다. 새 Editor 재로드를 위해 CloseMainWindow()를 호출했으나 false였고, MCP에는 정상 종료 tool이 없어 강제 종료하지 않았다. 저장 Blueprint들의 fresh-process reload는 미완료다.
- 따라서 .md/Unreal authoring 문서는 아직 갱신하지 않았다. 기존 Level actor persistence/상속, Data Validation, lever preview 복원과 직접 PIE 수용이 끝난 뒤 최종 통합 리뷰를 다시 판정한다.

## MCP instance editing and WP save retest — 2026-09-26

The Unreal MCP registry exposes generic component property setters, so instance editing is not categorically unavailable. A level-instance StaticMesh assignment succeeded, while its inherited transform/navigation/overlap values remained unchanged; Circulator LeverLabor.cancelReturnSeconds was explicitly rejected as not settable. The diagnostic mesh assignment was reverted. SceneTools.save_actor also failed for the existing Circulator because its World Partition external actor package did not exist. This leaves stale instance authoring and actor-package persistence blocked in the current MCP toolset. Blueprint CDO authoring and individual component properties remain possible where the property is editable; this does not establish a successful saved/reloaded map integration.

# Unreal MCP handoff — Computer focus CMP-001~020 (2026-09-26)

**중단 — MCP startup did not reach the server.** Code-stage approval was provided for the current computer focus task. A task-owned UE 5.8 background Editor (PID 33284) reached the Turnkey VerifySdk launch line, then stopped producing startup-log progress; no listener appeared on port 8000 and list_toolsets returned an HTTP transport connection failure. The reason Turnkey did not return is unconfirmed.

No Content, Config, Level asset authoring, Compile, Save, reload, Data Validation, or PIE was performed during this attempt. The task-owned Editor and its identified startup children were terminated after normal close failed; the port is free. Exact resume authoring and connection conditions are in .md/USER_UNREAL.md under “컴퓨터 포커스 CMP-001~020 — Unreal MCP 재시도 인계”.

## Startup comparison update — 2026-09-27

The previous successful log used the same `-NoSplash -log` command and the same Turnkey VerifySdk invocation; it opened the MCP listener and initialized a session 11 seconds after the Turnkey call. A fresh retry at 14:57:30 UTC remained at that Turnkey call for more than 60 seconds, with no Turnkey report/log files or port 8000 listener. This confirms the observed transport error was downstream of an Editor startup stall; the specific Turnkey/UAT wait cause remains unconfirmed. Retry PID 27704 and its identified UE 5.8 startup children are closed. No project assets were authored or saved.
## Authoring and fresh-process reload update — Computer focus CMP-001~020 (2026-09-27)

**부분 완료; 통합 승인 보류.** A prior task-owned MCP session successfully created IA_Cancel (Boolean), set BP_BathhouseComputer CDO FocusExitPoint.relativeLocation to (1000, 0, -228.5714285714), assigned BP_FirstPersonCharacter.CancelAction, and appended Escape → IA_Cancel while preserving the seven existing IMC_FirstPerson mappings. The controller DefaultMappingContext was verified. BP_BathhouseComputer, BP_FirstPersonCharacter, and BP_FirstPersonController compiled with warnings-as-errors without reported errors. BP_BathhouseComputer, BP_FirstPersonCharacter, IMC_FirstPerson, and IA_Cancel were individually saved and returned clean.

The exact DefaultMap external actor save failed because MCP could not resolve the external package through its asset registry, although the corresponding .uasset existed on disk. No map or external actor package was saved. Blueprint Compile propagated the class default to the actor in that session; only a fresh process can establish whether the saved CDO is reflected by the reloaded World Partition instance.

The first startup issue was an AutomationTool UnauthorizedAccessException writing user AppData logs under restricted execution. An approved elevated run then completed Turnkey and opened MCP, so that permission issue is resolved for the authoring session. A subsequent fresh-process reload attempt (task-owned PID 4284) completed Turnkey and Engine initialization and opened 127.0.0.1:8000, with an established client connection. However, no Python init_unreal.py or Editor authoring toolset registration followed; list_toolsets timed out while the log stopped after DDC maintenance. The exact startup stall cause is unknown. This is distinct from the earlier Turnkey/port startup failure. After the second unsuccessful reload attempt, PID 4284 was terminated and no UnrealEditor process or 8000 listener remained.

Data Validation was unavailable in the MCP tool registry. MCP could start/stop PIE but could not inject the input or mouse actions required by CMP-001~020 acceptance. An Editor screenshot capture was rejected by automatic approval review because it would transmit project UI/assets through MCP. Therefore fresh-process reload, Data Validation, visual inspection, and direct PIE acceptance remain outstanding; integration is not approved. See .md/USER_UNREAL.md under “컴퓨터 포커스 CMP-001~020 — authoring 및 재로드 재개 상태” for exact resume steps.
