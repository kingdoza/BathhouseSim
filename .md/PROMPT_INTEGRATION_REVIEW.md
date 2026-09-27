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

# Unreal MCP integration review — Computer focus CMP-001~020 (2026-09-27)

**부분 완료; 통합 승인 보류.** MCP에서 지원하는 authoring, compile, individual save, fresh-process reload 및 PIE start/stop은 완료했다. Data Validation과 직접 keyboard/mouse·시각 수용은 남아 있다.

## MCP connection history

- 제한된 최초 UE 실행에서 AutomationTool이 사용자 AppData 로그를 쓸 때 UnauthorizedAccessException이 발생했다. 승인된 elevated 실행으로 Turnkey와 MCP가 시작돼 이 권한 문제는 해당 실행에서 해소됐다.
- 후속 fresh-process PID 20416/4284는 각각 리스너 또는 TCP 연결까지 진행했으나 Python init/toolset 호출이 응답하지 않았다. 정확한 정지 원인은 확인되지 않았다.
- 이번 PID 22480은 full Editor toolset 등록까지 성공했다. 현재 대화의 Unreal MCP wrapper 함수는 노출되지 않아 direct MCP protocol을 사용했다. initialize, tools/list, list_toolsets 및 실제 read-only level/asset 조회가 성공했고 toolset 19개를 확인했다. 프로젝트 MCP 설정 변경은 없다.

## Saved authoring and reload

- IA_Cancel Boolean을 만들었다. BP_FirstPersonCharacter.CancelAction은 IA_Cancel이며 기존 InteractAction은 IA_Interact로 유지된다. IMC_FirstPerson의 기존 7개 mapping을 보존하고 Escape → IA_Cancel을 추가했다. BP_FirstPersonController.DefaultMappingContext는 IMC_FirstPerson이다.
- BP_BathhouseComputer CDO FocusExitPoint의 fresh-process value는 relative location (1500,0,-228.5714285714), rotation yaw 180, SearchRadius 100cm다. FocusExitArrow는 이 컴포넌트의 child이며 local origin/zero rotation, editor-only, ArrowLength 80cm다.
- exact DefaultMap computer actor는 fresh process에서 같은 FocusExitPoint relative transform을 보였다. Actor transform (-470,0,160), scale (0.12,1.2,0.7)로 계산한 foot point는 (-290,0,0), facing은 world -X다. ScreenWidget과 ManagedBathPlacementZone instance reference는 보존됐다. 이전 handoff에 적힌 X=1000 값은 현재 저장 asset readback과 일치하지 않아 본 fresh-process 값을 source of truth로 기록한다.
- BP_BathhouseComputer, BP_FirstPersonCharacter, BP_FirstPersonController를 warnings-as-errors Compile했다. 변경 에셋 BP_BathhouseComputer, BP_FirstPersonCharacter, IMC_FirstPerson, IA_Cancel은 각각 Save한 뒤 fresh process에서 class/value를 재조회했고 모두 dirty=false였다. DefaultMap도 dirty=false다.
- SceneTools.save_actor는 external actor asset registry 경로를 찾지 못해 실패했다. Map/actor의 성공 저장은 없었다. 새 process의 Level instance가 CDO와 같은 위치를 읽었으므로 별도 transform override 저장 여부를 단정하지 않는다.
- Foot location 아래 world trace는 Z=0 바닥을 가리켰다. 보수적 capsule AABB overlap 조회에서는 landscape tiles와 ManagedPlacementZone이 반환됐고 컴퓨터 mesh나 별도 blocker는 없었다. 이 query는 실제 Pawn capsule collision acceptance를 대체하지 않는다.

## PIE and remaining gates

- MCP StartPIE(standard in-viewport) succeeded; PIE server logged in and world initialization completed. StopPIE succeeded, and IsPIERunning=false afterward.
- Task-specific log search showed no gameplay error/ensure. Two LogScript warnings were caused by an MCP property probe asking SceneComponent for ArrowLength; the follow-up query targeted FocusExitArrow and confirmed length 80. This was a query mistake, not a Blueprint compile/gameplay error. The Editor log also contains a MapCheck 0/0 message, which is not Data Validation.
- Data Validation is not present in registered MCP toolsets. MCP has no keyboard/mouse injection, so E/ESC, button/slider drag, cursor position, blocker and forced-route acceptance were not performed. Editor screenshot capture was rejected by automatic approval review because it could transmit project UI/assets through MCP; no alternate screen capture was attempted.
- All target assets and the map were clean after the PIE session. Task-owned background PID 22480 was closed after MCP work; no UnrealEditor process or port 8000 listener remained.

Do not mark CMP-001~020 integration complete until Data Validation, visual confirmation, and direct player input acceptance are recorded. See .md/USER_UNREAL.md under “컴퓨터 포커스 CMP-001~020 — MCP 작업 결과 및 남은 수동 수용”.

# Unreal MCP integration review — Shop orders, delivery box, trash bin (2026-09-27)

## 상태

**부분 완료, 통합 승인 불가.** 저장 가능한 Shop DataAsset과 세 actor Blueprint를 작업했다. Data Validation, Widget authoring, Project Settings 영속화, World Partition Level actor 저장, 직접 PIE 입력·시각 수용은 완료되지 않았다.

## 완료한 MCP 작업

- /Game/Bathhouse/Blueprints/Shop 폴더와 세 Blueprint를 생성했다. Parent는 각각 /Script/BathhouseSim.ShopDeliveryBoxActor, /Script/BathhouseSim.ShopDeliveryPointActor, /Script/BathhouseSim.BathhouseTrashBinActor다. 세 Blueprint를 compile하고 각각 개별 Save했다.
- BP_ShopDeliveryBox의 BoxMesh root는 Engine BasicShapes Cube, relative transform zero/unit이다. BP_ShopDeliveryPoint에는 native SceneRoot와 editor-only billboard/arrow가 있으며 MaxSearchHeightCm=1000, DropGapCm=10이다. BP_TrashBin의 BinMesh root는 Engine BasicShapes Cube, relative transform zero/unit이다. 이 세 asset은 생성 세션에서 parent/CDO를 확인했으나 새 프로세스 reload는 미확인이다.
- BP_BathhousePlayerState와 BP_PlaceableFacilityItem의 compile/load gate가 성공했고 dirty=false였다.
- DA_ShopCatalog와 일곱 Placement Definition은 저장된 파일을 fresh process에서 재조회했다. Shower entry의 가격은 10000이며 valid Definition 참조가 읽혔다. 일곱 Definition은 Facility.Placeable 및 Facility.Discardable 태그, LockerSlotCount=0이다.
- 기존 사용자의 변경은 보존했다. 이번 작업의 임시 DeliveryPoint는 제거했고 DefaultMap은 dirty=false였다. 이번 작업에서 config/Map 변경을 저장하지 않았다.

## 남은 작업과 원인

- 새 Blueprint 세 개의 fresh-process reload와 PIE Start/Stop은 가능한 MCP 호출이지만, 마지막 작업용 Editor의 toolset initialization이 두 번 시간 초과되어 수행하지 못했다. 해당 Editor process와 listener는 종료했다. 새 Editor 재연결 성공 후 이 두 항목부터 재개한다.
- Catalog save-time validation 로그는 “Every shop product requires a valid placement definition.”를 반환했다. Definition 참조 readback은 유효해 보였으나 원인은 해소되지 않았다. MCP toolset에는 Data Validation 호출이 없다.
- 현재 toolset에는 Widget hierarchy/BindWidget 편집기가 없다. 일곱 Widget 생성 및 BP_BathhouseComputer/HUD class 연결은 미완료다.
- Project Settings 값은 ObjectTools.set_properties로 메모리에 설정할 수 있었지만 config-save tool이 없어 저장되지 않았다. Catalog 임시 변경은 None으로 되돌렸고 DefaultGame.ini는 바뀌지 않았다.
- DefaultMap 배치 지점 후보 (1800,650,0)은 floor trace상 바닥 Z=0이었고 주변은 열려 있었다. 임시 DeliveryPoint 생성은 가능했으나 SceneTools.save_actor가 /Game/__ExternalActors__/Maps/DefaultMap/C/FD/PZ6HQFYUX7L4RVM1PXG25B package 부재로 실패했다. 임시 actor를 제거했으며 TrashBin/낮은 천장 geometry는 저장하지 않았다.
- keyboard/mouse 입력 주입 tool이 없어 주문 UI, E pickup/drop/re-pick, LMB unboxing, trash bin과 HUD 동작은 직접 검증하지 않았다. Data Validation과 시각 배치 판정도 완료하지 않았다.

세부 수동 Editor 단계는 .md/USER_UNREAL.md의 상점 주문·배송·쓰레기통 항목에 기록했다. 저장과 새 프로세스 재로드가 확인되지 않은 상태는 .md/Unreal/ShopSystem.md의 confirmed authoring state에 포함하지 않았다.