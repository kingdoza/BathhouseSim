# REPORT_UNREAL_EDITOR — PLACEMENT-FOOTPRINT-PREVIEW 배치 미리보기의 footprint 공간 표시

- 작업 ID: `PLACEMENT-FOOTPRINT-PREVIEW`
- 단계: Editor 작업
- 상태: 완료

## 1. 수행 주체와 범위

- 하위 Editor 워커가 두 번 보류했다(1차: 재질 저장, 2차: BP 수정 스크립트 작성이 Claude Code auto mode 권한 분류기에 "Modify Shared Resources"로 거부). 하위 세션의 분류기는 사용자 채팅을 보지 못해 마스터가 전달한 사용자 허용을 인정하지 않았다. 수정 전 상태 확인과 재개 순서는 그 워커가 남긴 기록(Git `c5b8e3d`의 이 파일)과 `Saved/Claude/FPV-Editor/10_precheck.json`에 있다.
- 사용자 지시(2026-10-02 "ㅇㅇ 니가 직접해")로 마스터가 이 세션에서 Editor 역할을 직접 수행했다. AGENT_ORCHESTRATOR "마스터는 Content를 직접 고치지 않는다"의 사용자 지시 예외이며, 범위는 워커 보고서 5절로 한정했다.
- 사용자 허용 범위(2026-10-02 "허용"): 재질·MI 2개 생성·저장, `BP_Cooler`·`BP_Circulator`·`BP_Boiler` 수정·저장, `Config/DefaultGame.ini` `FootprintPreviewMaterial` 한 줄, Validation·Automation, Unreal 정본 갱신.
- 브랜치 `work/PLACEMENT-FOOTPRINT-PREVIEW`, Editor 단계 시작 커밋 `ecae280`. 바이너리가 최신이라 재빌드하지 않았다.
- `BP_ClothesLocker`(사용자 작업 트리 변경)는 열거나 저장하지 않았다. SHA-256 `6c81ae31…5967`이 작업 전후 같다.
- 백업: `Saved/MigrationBackup/20261002_PLACEMENT-FOOTPRINT-PREVIEW/`(BP 3개, `DefaultGame.ini`, `SHA256.txt`). 각 BP는 수정 직전과 저장 직전에 디스크 해시가 백업과 같은지 확인했다.

## 2. 실행 경로

- 작업용 Editor(`UnrealEditor.exe … -ModelContextProtocolStartServer -NoSplash -log -ExecCmds="py Saved/Claude/FPV-Editor/harness.py"`)에서 공식 `unreal` Python을 queue 방식으로 실행했다. 사용자 Editor는 없었다.
- 프로세스 1(PID 4712): `20_material.py` → `22_mi_check.py` → `30_bp.py` → `31_bp_save.py` → `29_quit_nosave.py`
- `DefaultGame.ini`는 Editor 종료 뒤 한 줄을 추가했다(CRLF 유지, 작업 전 해시 = 백업 해시 확인).
- 프로세스 2(PID 9524): `40_verify.py`(읽기 전용 새 프로세스 재로드) → `29_quit_nosave.py`
- headless Automation: UE_BUILD_POLICY 형식, `Automation RunTests BathhouseSim`, 리포트 `Saved/Automation/Reports/20261002/fpv_editor_full`, 로그 `Saved/Automation/Logs/fpv_editor_full.log`
- 결과 JSON: `Saved/Claude/FPV-Editor/20_material.json`, `30_bp.json`(compile 단계까지), `31_bp_save.json`, `40_verify.json`

## 3. 변경 결과

| 대상 | 변경 | 검증 |
|---|---|---|
| `/Game/Bathhouse/Materials/Placement/M_FacilityPlacementFootprint` (신규) | Surface, Translucent, Unlit, one-sided, depth test 켬, After DOF. graph는 워커 보고서 4절과 같음 | 새 프로세스 재로드에서 속성 일치 |
| `/Game/Bathhouse/Materials/Placement/MI_FacilityPlacementFootprint` (신규) | 부모 위 Material, `FillOpacity`·`OutlineOpacity`·`OutlineThicknessCm` override(값은 질문지 기본값 표 제안값, 원본은 이 MI) | 새 프로세스에서 parent·override 3개 확인. `set_*` 반환값은 False였으나 override 목록 readback으로 판정 |
| `BP_Cooler` | `PlacementFootprint` Box Extent X/Y를 합성 scale·`SceneRoot` Yaw ±90°에 맞춰 Actor X 2칸·Y 3칸이 되게 변경(Z·relative transform·root scale 유지) | Compile UP_TO_DATE, 저장, 새 프로세스 CDO·instance 일치 |
| `BP_Circulator`·`BP_Boiler` | CDO `SceneRoot` relative rotation → 0 | 같음 |
| `Config/DefaultGame.ini` | `[/Script/BathhouseSim.FacilityPlacementSettings]` `FootprintPreviewMaterial` 추가 | 새 프로세스에서 Settings가 위 MI를 로드 |
| `DefaultMap` 세 utility instance | 변경·저장 없음 | Compile 재인스턴스화로 external actor 3개가 자동 dirty였으나 저장하지 않음. 새 프로세스에서 세 instance가 새 Class Default 값을 그대로 가짐(override 없음) |

## 4. 검증 결과

- Data Validation(활성 Definition 16개, 새 프로세스): 16개 모두 VALID(작업 전 Circulator·Boiler·Cooler INVALID).
- 새 프로세스 dirty: 시작·끝 모두 0.
- Automation 전체 `BathhouseSim`: 184개, 실패 0, Warning 포함 성공 24(기준선 `u3_editor3` 177개, 실패 0, Warning 22).
  - 늘어난 7개는 이번 작업 6개(`Placement.FootprintPreview.*` 5, `PlacementContent.FootprintPreview`)와 지도 버그 1개다.
  - 리뷰 때 실패하던 Shop 3개(`UnboxViewFront.RoomPhysics`, `SevenDefinitionSpawn`, `UnboxingPhysics`)와 `PlacementContent.FootprintPreview`가 통과했다.
  - 새 Warning 테스트 2개(`Placement.ActorReplacementTransaction`, `Placement.PreviewHiddenWithoutAim`): 테스트가 미리보기 재질을 `Param` 없는 임시 재질로 바꿔 끼워 footprint 표시만 빠지는 fail-open Warning이다. 실제 재질은 `PlacementContent.FootprintPreview`가 `Param` 읽기와 Settings 계약을 확인했다.
- 화면 외형(비침·깜빡임·가림·외곽선 가독성)은 숨김 Editor로 판정하지 않았다. 사용자 PIE로 넘긴다.

## 5. Unreal 정본

`.md/Unreal/PlacementSystem.md`(원본 참조 형식 유지):
- footprint 표시 재질·mesh·높이·우선순위의 원본(Config 키), 재질 계약과 native DMI 입력, MI 표현값 원본을 추가했다.
- `M_Preview`가 graph상 Opacity 미연결이지만 화면에서는 반투명이라는 사용자 관찰을 함께 적었다.
- footprint Validation 계약에 합성 scale과 Yaw 90° 배수 검사를 반영했다.
- Circulator·Boiler `SceneRoot` 회전 0, Cooler footprint authoring 기준, DefaultMap instance가 Class Default를 따른다는 사실을 "판단 대기" 문구 대신 적었다.

## 6. 관찰과 잔여

- `DefaultMap`의 쿨러 instance 위치는 grid 칸 경계에 맞춰져 있지 않다(작업 전부터). 판정에는 영향이 없고, 회수·재배치하면 snap 위치로 놓을 수 있다.
- 쿨러 footprint가 넓어졌지만 지하 작업공간의 인접 순환기·보일러와 겹치지 않는다(새 프로세스 instance 위치·크기로 확인).
- `USER_UNREAL.md` 변경 없음. 작업용 Editor 2개는 dirty 확인 뒤 정상 종료했다.
- 회고 대상(ORCHESTRATOR·UNREAL_EDITOR): Editor 단계 전에 하위 세션의 저장 권한을 확인하지 않았고, 전달 승인으로 하위 세션 거부가 풀리지 않았다.

## 7. 사용자 PIE 관찰 항목

`PIE_CHECKLIST.md`로 옮겼다(시나리오 FPV-001~016, 워커 보고서 6절 기준).
