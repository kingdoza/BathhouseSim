# REPORT_UNREAL_EDITOR — PLACEMENT-FOOTPRINT-PREVIEW 배치 미리보기의 footprint 공간 표시

- 작업 ID: `PLACEMENT-FOOTPRINT-PREVIEW`
- 단계: Editor 작업
- 상태: 보류 — Editor asset 저장·BP 수정 스크립트 작성이 Claude Code auto mode 권한 분류기에 거부됨(마스터 전달 승인 후 재개에서도 같음), 책임 단계: Editor 작업(사용자 권한 설정 필요), 재개 조건: 사용자가 권한 규칙을 추가하거나 사용자가 직접 승인하는 세션에서 Editor 단계를 실행

(Editor 워커 전문을 마스터가 저장)

## 1. 입력과 실행 환경

- 입력: `PROMPT_UNREAL.md`(구현 단계, 상태 완료). 브랜치 `work/PLACEMENT-FOOTPRINT-PREVIEW`, Editor 단계 시작 커밋 `ecae280`이다.
- 사용자 결정(마스터 전달):
  - Q1~Q9 A. 메시 미리보기 외형은 유지한다.
  - Q10 B. 쿨러 2×3칸, 순환기·보일러 SceneRoot Yaw 0, Validation 보강.
  - S2: 범위 밖 asset 변경은 진행하고 보고한다(`BP_ClothesLocker` 제외).
  - S3: PIE 통과 후 병합한다.
  - 정본은 원본 참조로 쓴다.
- 사용자 "허용"(2026-10-02, 마스터 전달)의 범위:
  - 새 재질·MI 생성·저장
  - `BP_Cooler`·`BP_Circulator`·`BP_Boiler` 수정·저장
  - `DefaultGame.ini` `FootprintPreviewMaterial` 키 추가
  - Validation·Automation 실행, 정본 갱신
- 빌드: `Binaries/Win64/UnrealEditor-BathhouseSim.dll`(2026-10-02 23:21:53)보다 새 Source 파일이 없어 재빌드하지 않았다.
- 실행 경로(1차): 작업용 숨김 Editor 1개(PID 6740, `-ModelContextProtocolStartServer -NoSplash -log -ExecCmds="py …/Saved/Claude/FPV-Editor/harness.py"`)에서 공식 Python을 queue 방식으로 실행했다. 사용자 Editor는 꺼져 있었다. 2차 재개에서는 Editor를 띄우기 전에 막혔다.
- 사용자 변경: `Content/Bathhouse/Blueprints/Facility/BP_ClothesLocker.uasset`(SHA-256 `6c81ae31…5967`)은 열지도 저장하지도 않았다. 작업 전후 해시가 같다.
- 백업: `Saved/MigrationBackup/20261002_PLACEMENT-FOOTPRINT-PREVIEW/`에 `BP_Cooler`·`BP_Circulator`·`BP_Boiler`·`DefaultGame.ini`를 복사했고 `SHA256.txt`를 남겼다. 원본은 바뀌지 않았다.

## 2. 수행 결과 요약

| 항목 | 결과 |
|---|---|
| 1. `M_FacilityPlacementFootprint` | 1차: 메모리에서만 생성·graph 연결·recompile, 저장 거부, Editor 종료 때 버림. 디스크에 없음 |
| 2. `MI_FacilityPlacementFootprint` | 1차: 메모리 생성 후 스크립트 검사로 중단, 미저장. 2차: 스크립트 수정만 반영(미실행) |
| 3. Project Settings `FootprintPreviewMaterial` | 미수행 |
| 4. `BP_Cooler` footprint | 미수행. 2차에서 수정 스크립트 작성이 거부됨 |
| 5. `BP_Circulator`·`BP_Boiler` SceneRoot | 미수행(같은 스크립트). 수정 전 Validation 오류는 확인함 |
| 6. DefaultMap 세 설비 instance | override 없음 확인. Map 저장 불필요 |
| 7. 검증(Validation·Automation) | 수정 전 Validation만 수행. Automation 미실행 |
| 8. `.md/Unreal/PlacementSystem.md` | 저장된 변경이 없어 갱신하지 않음 |

차단 기록:
- 1차: `20_material.py` 재실행(MI 값 설정 + 두 asset 개별 Save)이 거부됐다.
- 2차(사용자 허용 전달 후): BP 수정·저장 스크립트 `30_bp.py` 작성이 거부됐다. 모두 사유는 "Modify Shared Resources"다.
- 거부는 같은 결과를 다른 경로로 추진하는 것도 금지하므로, 재질 저장을 포함한 저장 작업을 다시 시도하지 않았다.
- 마스터가 전달한 사용자 승인은 이 하위 세션의 권한 시스템이 인정하지 않는다. 사용자가 권한 규칙을 추가하거나 직접 승인하는 세션에서 실행해야 한다.

## 3. 수정 전 상태 확인 (읽기 전용, `Saved/Claude/FPV-Editor/10_precheck.json`)

- Data Validation(활성 Definition 16개):
  - `DA_FacilityPlacement_Circulator`·`_Boiler`는 INVALID, "PlacementFootprint가 설비 축에서 돌아 있어 그리드에 맞지 않습니다…"(구현의 축 정렬 검사 동작 확인).
  - `_Cooler`는 INVALID, "설비 배치 영역이 정의의 전역 그리드 셀 크기와 일치하지 않습니다."(root scale 반영 확인).
  - 나머지 13개는 VALID이며 경고는 공통 Cube fallback뿐이다.
- CDO:
  - `BP_Cooler`: root `PackagePhysicalRoot` scale 0.5, `SceneRoot` Yaw −90·scale 1, `PlacementFootprint` extent (50,30,60), relative (0,0,60), scale 1.
  - `BP_Circulator`: `SceneRoot` Yaw −1, footprint (60,40,60).
  - `BP_Boiler`: `SceneRoot` Yaw −1, footprint (50,30,60).
- DefaultMap instance(`BP_Cooler`·`BP_Circulator`·`BP_Boiler`, 모두 `Space_Work`):
  - `SceneRoot` rotation과 footprint extent·scale이 CDO와 같아 실질 instance override가 없다. 6항은 Map 저장이 필요 없다.
  - package: `/Game/__ExternalActors__/Maps/DefaultMap/0/EB/P4KMP0NJE6NZLCAA8FG8AB`(Cooler), `…/5/YZ/KRUXF2RNGDHA3WNR999GN3`(Circulator), `…/7/1K/M6ZXZRZAAVEJCE6LP25CQY`(Boiler).
  - 쿨러 넓힘 뒤 인접 겹침은 수정 후에 확인할 항목이라 미확인이다.
- 1항 pass 조건: `/Game/Material/M_Preview`와 `M_FacilityPlacementGrid` 모두 translucency pass After DOF, Translucent, Unlit, one-sided, depth test 켬이다. 같은 값이라 새 재질도 After DOF로 맞춘다.
- grid UV 대응: `M_FacilityPlacementGrid`는 `TexCoord0 × Append(ZoneSizeXCm/GridSizeCm, ZoneSizeYCm/GridSizeCm)`다. U가 `ZoneSizeXCm`(plane local X), V가 `ZoneSizeYCm`이다.
- 3항 확인값(`Config/DefaultGame.ini`, 구현이 넣은 값):
  - `FootprintPreviewMesh=/Engine/BasicShapes/Plane.Plane`
  - `FootprintPreviewFloorOffsetCm`, `FootprintPreviewTranslucencySortPriority`, `PreviewMeshTranslucencySortPriority`가 있다.
  - `FootprintPreviewMaterial` 키는 아직 없다.
  - 우선순위 관계: grid `GridVisual` Translucency Sort Priority(세 공간 instance 0) < footprint < 메시다. Config 값으로 성립한다(값은 Config 원본 참조).

## 4. 재개용 스크립트와 계산

- `Saved/Claude/FPV-Editor/20_material.py`(2차에서 수정 반영, 문법 통과, 미실행):
  - 재질 속성: Surface, Translucent, Unlit, Two Sided 끔, depth test 켬, After DOF.
  - graph 계산:
    - `PosCm = TexCoord0 × Append(FootprintSizeXCm, FootprintSizeYCm)`
    - `Dist = min(R, G)(min(PosCm, Size − PosCm))`
    - `If(Dist, OutlineThicknessCm; A>B→0, A==B→1, A<B→1)`
    - `Opacity = Lerp(FillOpacity, OutlineOpacity, 외곽선)`
    - `Emissive = PreviewColor`
  - MI에는 `QNA_FEATURE_SPEC.md` 기본값 표 제안값(채움·외곽선 불투명도, 외곽선 두께)만 설정한다.
  - MI 값 판정은 `set_*` 반환값이 아니라 readback으로 한다(1차에서 새 MI의 set이 False를 반환했으나 parent parameter 5개는 보였다).
  - 시작 dirty 허용은 두 새 asset으로 한정하고, allowlist 밖 dirty가 있으면 저장하지 않는다.
- BP 수정(`30_bp.py`, 작성 거부로 파일 없음. 재개 때 같은 내용으로 작성):
  - 대상 `/Game/Bathhouse/Blueprints/Facility/BP_Cooler`·`BP_Circulator`·`BP_Boiler`.
  - 수정 직전과 저장 직전에 디스크 SHA-256을 백업 해시와 비교한다. 다르면 중단한다.
  - `BP_Cooler`: `SceneRoot` Yaw가 ±90인지 확인한다. footprint local X extent = Actor Y 3칸 × `GridSizeCm`(Project Settings) ÷ (2 × |root·SceneRoot·footprint 합성 scale X|), local Y extent = Actor X 2칸 × Grid ÷ (2 × |합성 scale Y|)다. Z extent, relative transform, root scale은 유지한다(현재 Grid·scale이면 (60,40,60)).
  - `BP_Circulator`·`BP_Boiler`: CDO `SceneRoot` relative rotation을 (0,0,0)으로 한다. 하위 값은 유지한다.
  - `BlueprintEditorLibrary.compile_blueprint` 뒤 `status`가 UP_TO_DATE이고 dirty가 세 BP뿐일 때만 `save_asset`으로 개별 저장한다.

## 5. 재개 순서 (권한 허용 후)

1. 작업용 Editor를 띄우고 `20_material.py`로 재질·MI를 만들어 개별 Save한다. 새 프로세스에서 parameter·속성을 readback한다.
2. 4절 BP 스크립트로 세 BP를 수정·Compile·Save한다.
3. Data Validation 16개를 다시 돌리고 DefaultMap 세 instance readback을 한다(Map 저장 없음). 쿨러 인접 겹침 사실을 확인한다.
4. Editor를 종료한다. `Config/DefaultGame.ini` `[/Script/BathhouseSim.FacilityPlacementSettings]`에 `FootprintPreviewMaterial=/Game/Bathhouse/Materials/Placement/MI_FacilityPlacementFootprint.MI_FacilityPlacementFootprint`를 추가한다(선례: Shop 설정 직접 추가 + 백업). 새 프로세스에서 Settings 로드값을 확인한다.
5. headless Automation을 실행한다(UE_BUILD_POLICY 형식).
   - 순서: `BathhouseSim.PlacementContent.FootprintPreview` → `BathhouseSim.Placement.` → `BathhouseSim.Shop.` 3개 → 전체 `BathhouseSim`(기준선 `Saved/Automation/Reports/20261002/u3_editor3`, 실패 0).
   - 실패하면 보고하고 중단한다.
6. `.md/Unreal/PlacementSystem.md`를 갱신하고(원본 참조 형식) 이 보고서를 완료로 바꾼다.

## 6. 사용자 PIE 관찰 항목 (완료 뒤 `PIE_CHECKLIST.md`용, 화면 외형은 사용자 관찰이 판정)

대표 Vanity(홀):

- FPV-001: 들고 홀 바닥을 조준하면 메시와 함께 바닥 사각형이 보이고 grid 위에 있어 묻히거나 깜빡이지 않는다. 둘 다 유효 색이다.
- FPV-002: 사각형이 메시 외곽보다 사방으로 조금 넓다. 반투명 메시 아래 외곽선이 알아볼 만한지 본다. 부족하면 MI 불투명도나 두 우선순위로 조정한다.
- FPV-003: LCtrl 동안 메시·사각형이 칸 단위로 함께 이동하고 네 변이 grid 선과 겹친다. 놓으면 자유 이동, Yaw는 유지된다.
- FPV-004: Wheel 회전 때 사각형 긴 변이 메시와 같은 방향으로 돌고 수평을 유지한다.
- FPV-005: 메시는 안 닿고 footprint만 닿는 자리에서 둘 다 무효 색이고 겹침이 보인다(불투명 물체에 가려짐). LMB를 눌러도 설치되지 않고 문구는 기존과 같다.
- FPV-006: 구역 밖·불허·한도·지지 부족에서 숨지 않고 무효 색이다. 벽 너머 부분은 가려진다.
- FPV-007: 하늘·벽·거리 밖을 조준하면 둘 다 사라지고(footprint만 남지 않음) 복귀하면 함께 나타난다. Yaw는 유지된다.
- FPV-008: 설치 성공 뒤 메시·사각형·grid가 모두 사라진다.
- FPV-009: G·컴퓨터·손 변경 뒤 함께 사라지고 다시 들면 정상이다.

16종:

- FPV-010: 같거나 작은 footprint(DrinkFridge, Boiler 등)도 메시 아래 사각형이 보인다.
- FPV-011: 세 공간(넓힘 바닥 포함) 높이에 놓인다. Shower·Bath에서 깜빡임이 없다.
- FPV-012: 기존 자리의 판정·설치 결과가 같다.
- FPV-013: 16종 사각형 크기가 칸 수와 같고 초기화 실패가 새로 생기지 않는다.
- FPV-014·015: 잘못된 Definition이면 메시·footprint 모두 없고 설치가 불가하다.

FPV-016(지하 쿨러·순환기·보일러):

- 쿨러가 Actor X 2칸·Y 3칸이고 snap이 맞는다(메시가 밖으로 나가 좁은 자리는 무효일 수 있음).
- 순환기·보일러가 grid와 평행하다.
- Level 기존 세 instance를 회수·재배치할 수 있고 잔량 등 상태가 유지된다.

## 7. 정본·큐·dirty

- `.md/Unreal/PlacementSystem.md`: 미갱신. `USER_UNREAL.md`: 기록하지 않음(세션 권한 문제라 마스터 보고로 처리).
- 1차 종료 직전 dirty: 이번 작업이 메모리에 만든 미저장 새 asset 2개뿐이었고 Map dirty는 0이었다. 저장 없이 `quit_editor`로 정상 종료했다(로그 `Exiting.`). 2차는 Editor를 띄우지 않았다. 현재 작업용 Editor는 0개다.
- 로그: `Saved/Claude/FPV-Editor/editor_6740.log`
- 회고 후보(UNREAL_EDITOR·ORCHESTRATOR):
  - Editor 작업 단계 시작 전에 하위 에이전트 세션의 asset 저장 권한(auto mode 분류기)을 확인하지 않았다.
  - 마스터가 전달한 사용자 승인으로는 하위 세션의 분류기 거부가 풀리지 않았다.
