# REPORT_UNREAL_EDITOR — PLACEMENT-FOOTPRINT-PREVIEW 배치 미리보기의 footprint 공간 표시

- 작업 ID: `PLACEMENT-FOOTPRINT-PREVIEW`
- 단계: Editor 작업
- 상태: 보류 — Editor asset 저장이 Claude Code 권한 분류기에 거부되어 allowlist 저장을 하나도 하지 못함, 책임 단계: Editor 작업(사용자 권한 부여 필요), 재개 조건: 사용자가 Editor asset 저장·`Config/DefaultGame.ini` 수정 권한을 허용한 뒤 Editor 워커 재개

(Editor 워커 전문을 마스터가 저장)

## 1. 입력과 실행 환경

- 입력: `PROMPT_UNREAL.md`(구현 단계, 상태 완료). 브랜치 `work/PLACEMENT-FOOTPRINT-PREVIEW`, Editor 단계 시작 커밋 `ecae280`이다.
- 사용자 결정(마스터 전달):
  - Q1~Q9 A. 메시 미리보기 외형은 유지한다.
  - Q10 B. 쿨러 2×3칸, 순환기·보일러 SceneRoot Yaw 0, Validation 보강.
  - S2: 범위 밖 asset 변경은 진행하고 보고한다(`BP_ClothesLocker` 제외).
  - S3: PIE 통과 후 병합한다.
  - 정본은 원본 참조로 쓴다.
- 빌드: `Binaries/Win64/UnrealEditor-BathhouseSim.dll`(2026-10-02 23:21:53)보다 새 Source 파일이 없어 재빌드하지 않았다.
- 실행 경로: 작업용 숨김 Editor 1개(PID 6740, `-ModelContextProtocolStartServer -NoSplash -log -ExecCmds="py …/Saved/Claude/FPV-Editor/harness.py"`)에서 공식 Python을 queue 방식으로 실행했다. 사용자 Editor는 꺼져 있었다.
- 사용자 변경: `Content/Bathhouse/Blueprints/Facility/BP_ClothesLocker.uasset`(SHA-256 `6c81ae31…5967`)은 열지도 저장하지도 않았다. 작업 전후 해시가 같다.
- 백업: `Saved/MigrationBackup/20261002_PLACEMENT-FOOTPRINT-PREVIEW/`에 `BP_Cooler`·`BP_Circulator`·`BP_Boiler`·`DefaultGame.ini`를 복사했고 `SHA256.txt`를 남겼다. 원본은 바뀌지 않았다.

## 2. 수행 결과 요약

| 항목 | 결과 |
|---|---|
| 1. `M_FacilityPlacementFootprint` | 메모리에서만 생성·graph 연결·recompile했다(parameter 6개 확인). 저장이 거부되어 디스크에 없다. Editor 종료 때 버렸다 |
| 2. `MI_FacilityPlacementFootprint` | 메모리에서만 생성했고 부모는 연결했다. scalar 설정 단계에서 스크립트 검사(반환값)로 중단됐다. 미저장, 종료 때 버렸다 |
| 3. Project Settings `FootprintPreviewMaterial` | 미수행(2항 asset이 없고 Config 수정도 같은 권한 범주) |
| 4. `BP_Cooler` footprint | 미수행. 수정 전 상태만 확인 |
| 5. `BP_Circulator`·`BP_Boiler` SceneRoot | 미수행. 수정 전 Validation 오류 확인 |
| 6. DefaultMap 세 설비 instance | override 없음을 확인. Map은 저장하지 않음(원래 계획도 저장 안 함) |
| 7. 검증(Validation·Automation) | 수정 전 Validation만 수행. Automation은 저장이 앞서야 해서 미실행 |
| 8. `.md/Unreal/PlacementSystem.md` | 저장된 변경이 없어 갱신하지 않음 |

차단 원인: `20_material.py` 재실행(MI 값 설정 후 두 asset 개별 Save)을 Claude Code auto mode 분류기가 "Modify Shared Resources"로 거부했다. 지침에 따라 다른 도구나 경로로 우회하지 않았다. 다른 모든 항목도 asset 저장이나 Config 수정이 필요해 같은 이유로 진행하지 않았다.

## 3. 수정 전 상태 확인 (읽기 전용, `Saved/Claude/FPV-Editor/10_precheck.json`)

- Data Validation(활성 Definition 16개):
  - `DA_FacilityPlacement_Circulator`·`_Boiler`는 INVALID, "PlacementFootprint가 설비 축에서 돌아 있어 그리드에 맞지 않습니다…"(구현의 축 정렬 검사 동작 확인).
  - `_Cooler`는 INVALID, "설비 배치 영역이 정의의 전역 그리드 셀 크기와 일치하지 않습니다."(root scale 반영 확인).
  - 나머지 13개는 VALID이며 경고는 공통 Cube fallback뿐이다.
- CDO:
  - `BP_Cooler`: root `PackagePhysicalRoot` scale 0.5, `SceneRoot` rotation Yaw −90·scale 1, `PlacementFootprint` extent (50,30,60), relative (0,0,60), scale 1.
  - `BP_Circulator`: `SceneRoot` Yaw −1, footprint (60,40,60).
  - `BP_Boiler`: `SceneRoot` Yaw −1, footprint (50,30,60).
- 4항 목표 계산(재개 때 적용): 합성 scale = 0.5 × 1 × 1 = 0.5, `GridSizeCm`는 Project Settings 값을 읽는다.
  - footprint local X(=Actor Y 3칸) extent = 3 × Grid ÷ (2 × 0.5)
  - local Y(=Actor X 2칸) extent = 2 × Grid ÷ (2 × 0.5)
  - 현재 Grid 20이면 (60,40), Z 60은 유지한다.
- DefaultMap instance(`BP_Cooler`·`BP_Circulator`·`BP_Boiler`, 모두 `Space_Work`):
  - `SceneRoot` rotation과 footprint extent·scale이 CDO와 같아 실질 instance override가 없다. 6항은 Map 저장이 필요 없다.
  - package: `/Game/__ExternalActors__/Maps/DefaultMap/0/EB/P4KMP0NJE6NZLCAA8FG8AB`(Cooler), `…/5/YZ/KRUXF2RNGDHA3WNR999GN3`(Circulator), `…/7/1K/M6ZXZRZAAVEJCE6LP25CQY`(Boiler).
  - 쿨러 넓힘 뒤 인접 겹침은 수정 후에 확인할 항목이라 미확인이다.
- 1항 pass 조건: `/Game/Material/M_Preview`와 `M_FacilityPlacementGrid` 모두 translucency pass After DOF, Translucent, Unlit, one-sided, depth test 켬이다. 같은 값이라 새 재질도 After DOF로 맞추면 된다.
- grid UV 대응: `M_FacilityPlacementGrid`는 `TexCoord0 × Append(ZoneSizeXCm/GridSizeCm, ZoneSizeYCm/GridSizeCm)`다. U가 `ZoneSizeXCm`(plane local X), V가 `ZoneSizeYCm`이다.
- 3항 확인값(`Config/DefaultGame.ini`, 구현이 넣은 값):
  - `FootprintPreviewMesh=/Engine/BasicShapes/Plane.Plane`
  - `FootprintPreviewFloorOffsetCm`, `FootprintPreviewTranslucencySortPriority`, `PreviewMeshTranslucencySortPriority`가 있다.
  - `FootprintPreviewMaterial` 키는 아직 없다.
  - 우선순위 관계: grid `GridVisual` Translucency Sort Priority(세 공간 instance 0, 사전 조사 확인) < footprint < 메시다. Config 값으로 grid < footprint < mesh가 성립한다(값은 Config 원본 참조).

## 4. 메모리에서 만든 재질 graph (재개 때 같은 graph로 다시 만듦, `20_material.py`)

- 속성: Surface, Translucent, Unlit, Two Sided 끔, Disable Depth Test 끔, Translucency Pass After DOF.
- graph 계산:
  - `PosCm = TexCoord0 × Append(FootprintSizeXCm, FootprintSizeYCm)`
  - `Dist = min(Mask R, Mask G)(min(PosCm, Size − PosCm))`
  - `If(Dist, OutlineThicknessCm; A>B→0, A==B→1, A<B→1)`
  - `Opacity = Lerp(FillOpacity, OutlineOpacity, 외곽선)`
  - `Emissive = PreviewColor`
- parameter 6개(`PreviewColor`, `FootprintSizeXCm`, `FootprintSizeYCm`, `FillOpacity`, `OutlineOpacity`, `OutlineThicknessCm`)가 Material에 생긴 것을 메모리에서 확인했다. recompile까지 했고 Save 전이었다.
- MI 값은 `QNA_FEATURE_SPEC.md` 기본값 표의 제안값(채움·외곽선 불투명도, 외곽선 두께)이다. native parameter 셋은 override하지 않는다.
- 스크립트 수정 필요(재개 전):
  - `MaterialEditingLibrary.set_material_instance_scalar_parameter_value`가 새로 만든 MI에서 False를 반환해 `require`가 중단시켰다.
  - 이후 진단에서 MI가 parent parameter 5개를 보이는 것을 확인했다.
  - 반환값 대신 `get_material_instance_scalar_parameter_value` readback으로 판정하도록 바꾸고, 시작 dirty 허용 목록에 두 새 asset을 넣는다. 이 수정은 같은 거부 호출에 포함되어 아직 반영되지 않았다.

## 5. 재개 순서 (권한 허용 후)

1. 작업용 Editor를 띄우고 `20_material.py`(4절 수정 반영)로 재질·MI를 만들어 개별 Save한다. 새 프로세스에서 parameter·속성을 readback한다.
2. `BP_Cooler` footprint extent X/Y(3절 계산), `BP_Circulator`·`BP_Boiler` `SceneRoot` relative rotation 0을 설정한다. 각 BP Compile(`BS_UP_TO_DATE`)을 확인하고 저장 직전 디스크 해시를 백업 해시와 비교한 뒤 개별 Save한다.
3. Data Validation 16개를 다시 돌리고 DefaultMap 세 instance readback을 한다(Map 저장 없음). 쿨러 인접 겹침 사실을 확인한다.
4. Editor를 종료한다. `Config/DefaultGame.ini` `[/Script/BathhouseSim.FacilityPlacementSettings]`에 `FootprintPreviewMaterial=/Game/Bathhouse/Materials/Placement/MI_FacilityPlacementFootprint.MI_FacilityPlacementFootprint`를 추가한다(선례: Shop 설정 직접 추가 + 백업). 새 프로세스에서 Settings 로드값을 확인한다.
5. headless Automation을 실행한다(UE_BUILD_POLICY 형식).
   - 순서: `BathhouseSim.PlacementContent.FootprintPreview` → `BathhouseSim.Placement.` → `BathhouseSim.Shop.` 3개 → 전체 `BathhouseSim`(기준선 `Saved/Automation/Reports/20261002/u3_editor3`, 실패 0).
   - 실패하면 보고하고 중단한다.
6. `.md/Unreal/PlacementSystem.md`를 갱신하고(원본 참조 형식) 이 보고서를 완료로 바꾼다.

## 6. 사용자 PIE 관찰 항목 (재개·완료 뒤 `PIE_CHECKLIST.md`용, 화면 외형은 사용자 관찰이 판정)

대표 Vanity(홀):

- FPV-001: 들고 홀 바닥을 조준하면 메시와 함께 바닥 사각형이 보이고 grid 위에 있어 묻히거나 깜빡이지 않는다. 둘 다 유효 색이다.
- FPV-002: Vanity 사각형이 메시 외곽보다 사방으로 조금 넓게 보인다. 반투명 메시 아래 부분의 외곽선이 메시를 통해 알아볼 만한지 본다. 부족하면 MI 불투명도나 두 우선순위로 조정한다.
- FPV-003: LCtrl 동안 메시·사각형이 칸 단위로 함께 이동하고 네 변이 grid 선과 겹친다. 놓으면 자유 이동, Yaw는 유지된다.
- FPV-004: Wheel 회전 때 사각형 긴 변이 메시와 같은 방향으로 돌고 수평을 유지한다.
- FPV-005: 다른 설비 옆에서 메시는 안 닿고 footprint만 닿는 자리를 조준하면 둘 다 무효 색이고 사각형이 겹쳐 들어간 것이 보인다(불투명 물체에 가려짐). LMB를 누르면 기존 문구가 나오고 설치되지 않는다.
- FPV-006: 구역 밖 걸침·불허·락커 한도·지지 부족에서 둘 다 숨지 않고 무효 색이며 사유 문구는 기존과 같다. 벽 너머 부분은 가려진다.
- FPV-007: 하늘·벽·거리 밖을 조준하면 둘 다 사라지고(footprint만 남지 않음) 복귀하면 함께 나타난다. Yaw는 유지되고 grid는 그대로다.
- FPV-008: 설치 성공 뒤 메시·사각형·grid가 모두 사라진다.
- FPV-009: G·컴퓨터 사용·손 변경 뒤 함께 사라지고 다시 들면 정상이다.

16종:

- FPV-010: footprint가 메시와 같거나 작은 설비(DrinkFridge, Boiler 등)도 메시 아래 바닥에 사각형이 보인다.
- FPV-011: 홀·목욕공간·지하 작업공간(넓힘 바닥 포함)에서 각 바닥 높이에 놓인다. Shower(뜬 판)·Bath(메시 바닥면이 바닥보다 약간 아래)에서 깜빡임이 없다.
- FPV-012: 기존 유효·무효 자리의 판정·설치 결과가 같다.
- FPV-013: 16종 모두 사각형 크기가 칸 수와 같고 메시와 함께 움직인다. 초기화 실패 설비가 새로 생기지 않는다.
- FPV-014·015: 잘못된 Definition이면 메시·footprint 모두 없고 설치가 불가하다(기존과 같음).

FPV-016(쿨러·순환기·보일러, 지하):

- 쿨러 사각형이 Actor 축 X 2칸·Y 3칸이고 snap 시 grid에 맞는다. 메시가 사각형 밖으로 나가므로 좁은 자리에서 무효가 될 수 있다.
- 순환기·보일러 메시·사각형이 grid와 평행하다(1° 틀어짐 없음).
- Level 기존 세 instance를 Q로 회수한 뒤 재배치할 수 있고, 잔량 등 기존 상태가 유지되는지 본다.

## 7. 정본·큐·dirty

- `.md/Unreal/PlacementSystem.md`: 미갱신(저장된 변경 없음). `USER_UNREAL.md`: 기록하지 않음. 권한 차단은 사용자 수동 Editor 조작이 아니라 세션 권한 문제라 마스터 보고로 처리한다.
- 종료 직전 dirty: 새 asset 2개(이번 작업이 메모리에 만든 미저장 객체)뿐이었다. Map dirty는 0이다. 저장하지 않고 `quit_editor`로 정상 종료했고(로그 `Exiting.`), 프로세스가 사라졌다. 디스크에는 두 asset 파일이 없다.
- 실행 로그: `Saved/Claude/FPV-Editor/editor_6740.log`. `10_precheck` 첫 실행은 `expression_collection` 미노출 예외였고 출력 기준 traversal로 바꿔 PASS했다. `20_material` 첫 실행은 4절 검사로 중단됐고 Save 전이었다.
- 회고 후보(UNREAL_EDITOR·ORCHESTRATOR): Editor 작업 단계를 시작하기 전에 asset 저장 권한(auto mode 분류기)이 허용되는지 확인하지 않아, 메모리 작업 뒤 저장 단계에서 막혔다.
