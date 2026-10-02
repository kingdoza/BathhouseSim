# REPORT_UNREAL_EDITOR — 욕탕 타일 Fill·Q1 A ScaleBox·정본 원본 참조 정리

- 작업 ID: `BUG-2026-10-02_bath_water_map_grid_scale_mismatch`
- 단계: Editor 작업
- 상태: 완료

(Editor 워커 전문을 마스터가 저장)

## 1. 입력과 범위

- 입력: `PROMPT_UNREAL.md`(구현, 완료), `QNA_ARCHITECTURE.md` Q1 답 A(사용자 확인), 참고 `../PLACEMENT-FOOTPRINT-PREVIEW/REPORT_UNREAL_DISCOVERY.md` 6절.
- 브랜치·시작 커밋: `work/BUG-2026-10-02_bath_water_map_grid_scale_mismatch`, `41176b2`.
- 사전 허용(마스터 인계): `WBP_BathWaterBathTile` 수정·저장, `WBP_BathWaterMap` 노출 확인(저장 없음), 정본 `Unreal/PlacementSystem.md`·`Unreal/InteractionUISystem.md` 원본 참조화.
- 금지 범위 준수:
  - Source·PIE·화면 작업·커밋을 하지 않았다.
  - `BP_ClothesLocker`는 열거나 저장하지 않았다. SHA-256 `6c81ae31…5967`이 작업 전후 같다.
  - `.md/Work/PLACEMENT-FOOTPRINT-PREVIEW/`와 worktree `BathhouseSim-FPV`는 건드리지 않았다.
- 바이너리: `UnrealEditor-BathhouseSim.dll`(22:22)이 변경 Source(최종 22:16)보다 나중에 빌드됐다. 재빌드하지 않았다(UE_BUILD_POLICY).

## 2. 실행 경로

- 작업용 숨김 Editor를 `-ModelContextProtocolStartServer -NoSplash -log -ExecCmds="py Saved/Claude/BUG-MapGrid/harness.py"`로 띄웠다. 공식 `unreal` Python을 queue로 실행했고 MCP 호출은 쓰지 않았다. 사용자 Editor는 없었다.
- 프로세스 순서: 1) PID 29524 수정·저장 → 2) PID 32204 새 프로세스 재로드 + Compile·GUID 정리 저장 → 3) PID 34812 읽기 전용 재로드. 각 프로세스는 dirty 0을 확인한 뒤 `quit_editor`로 종료했고 PID가 사라진 것을 확인했다.
- 백업: `Saved/MigrationBackup/20261002_BUG-MapGrid/WBP_BathWaterBathTile.uasset`, `manifest_before.sha256`, `manifest_after.sha256`.
- 로그: `Saved/Claude/BUG-MapGrid/editor_session1.log`, `editor_session2.log`, `editor_session3.log`.

| 순서 | 스크립트 | 결과 |
|---|---|---|
| 1 | `10_pre.py` | 읽기 전용. 세 WBP 모두 `BS_UP_TO_DATE`, 새 프로퍼티 9개 읽기 가능. hierarchy가 진단 보고와 같다(`RootOverlay > SelectButton`(Left/Top) `> TileColumn`(ButtonSlot Center/Center, padding 1,0,1,0)) |
| 2 | `11_apply_tile.py` | 메모리 수정 + Compile `BS_UP_TO_DATE`. dirty는 tile 1개뿐 |
| 3 | `12_save_tile.py` | tile만 개별 Save(디스크 해시를 기준선과 대조한 뒤). 저장 후 dirty 0 |
| 4 | `13_fresh.py` | 프로세스 2: 재로드 확인 → Compile → tile만 Save(GUID 정리) → `13_fresh.json` |
| 5 | `13_fresh.py` + `queue/no_save.flag` | 프로세스 3: 읽기 전용 재로드 → `13_fresh_ro.json` |

## 3. 변경 결과 (`/Game/Bathhouse/UI/WBP_BathWaterBathTile`)

```text
RootOverlay
└─ SelectButton        OverlaySlot H=Fill, V=Fill (이전 Left/Top), padding 0
   └─ TileTextScale    ScaleBox, Stretch=ScaleToFit, StretchDirection=DownOnly, SelfHitTestInvisible
                       ButtonSlot H=Fill, V=Fill, padding = 이전 TileColumn padding
      └─ TileColumn    ScaleBoxSlot H=Center, V=Center
         └─ BathNameText, ActualTemperatureText, ContaminationText, ThermalStatusText, CapacityStatusText
```

- BindWidget 6개(`SelectButton` Button, 글자 5개 TextBlock)의 이름·타입·도달 경로가 유지된다. `TileTextScale`은 BindWidget이 아니다.
- Class Defaults(`DeficitTileColor` 등 5개)는 바꾸지 않았다(C++ 초기값 상속).
- 클릭: ScaleBox는 버튼의 자식이고 SelfHitTestInvisible이라 `SelectButton` hit test를 가리지 않는다. 클릭 영역은 버튼 전체, 즉 footprint 사각형이다.
- Event Graph는 추가하지 않았다. 레이아웃·값만 바꿨다.

## 4. 검증 결과

| 항목 | 결과 |
|---|---|
| Compile (프로세스 1) | `BS_UP_TO_DATE`. ensure `Widget [TileTextScale] was added but did not get a GUID` 발생(새 widget 추가 시의 알려진 현상) |
| Save (프로세스 1) | tile만 저장, 해시 `31fbea99…` → `a060cd41…` |
| 새 프로세스 재로드·Compile·Save (프로세스 2) | hierarchy·slot·ScaleBox 값 유지, Compile `BS_UP_TO_DATE`, **ensure 0**, 저장 해시 `c7cbde6f92364def5ecc5450c232c0f15c279289505a2ba4c8fba096afe91894` |
| 새 프로세스 읽기 전용 재로드 (프로세스 3) | 위 해시와 구조 동일, ensure 0, dirty 0 |
| `WBP_BathWaterMap` | `BS_UP_TO_DATE`, `GridLineThicknessPx`·`BoundaryLineThicknessPx`·`GridLineColor`·`BoundaryLineColor` 읽기 가능, 저장 안 함(해시 `cc654617…` 그대로) |
| `WBP_BathWaterManagementScreen` | `BS_UP_TO_DATE`, 중첩 `BathMap`의 `BathTileWidgetClass` = `WBP_BathWaterBathTile_C` 유지, 저장 안 함(해시 `af56977d…` 그대로) |
| 예상 밖 dirty | 없음(모든 스크립트 시작·끝과 종료 직전 content/map 0, tile 저장 직전에는 tile만) |
| Data Validation | 실행하지 않음(PROMPT 요구 없음) |

## 5. 정본 변경 (직접 수정)

`.md/Unreal/PlacementSystem.md`
- `FacilityItemHeldTransform`: 저장값 수치를 지우고 `Config/DefaultGame.ini` `[/Script/BathhouseSim.FacilityPlacementSettings]`를 원본으로 적었다.
- 미리보기 재질:
  - 실제 사용 asset은 Config `ValidPreviewMaterial`·`InvalidPreviewMaterial`이 가리키는 `/Game/Material/MI_Preview_*`(부모 `M_Preview`, 색 원본 MI `Param`)임을 정리했다.
  - `MI_FacilityPreview_*`는 Material class이고 미리보기에 쓰이지 않으며 표현값은 각 asset 입력이 원본이라고 적었다. Emissive·Opacity 수치는 삭제했다.
- Definition 표: `Locker Slots` 열을 지우고 원본을 각 Definition `LockerSlotCount`로 적었다.
- footprint 표:
  - Extent·Relative Z·body scale·mesh 수치 열을 지웠다. 원본은 각 Blueprint `PlacementFootprint`(Box Extent, Relative Location)와 body component(Static Mesh, Relative Transform)다.
  - Parent Class를 정정했다: `BP_Bath` → `BathhouseBathFacilityActor`, `BP_Circulator` → `BathWaterCirculatorFacilityActor`, `BP_Cooler` → `BathWaterCoolerFacilityActor`.
  - body component를 정정했다: Bath 제어 component 2개, Washer·Dryer `LidMesh`, Circulator·Boiler·Cooler 부속 mesh, Cooler `Cooler_Body/Lid/Needle`.
  - 6종(DrinkFridge, Vanity, MassageChair, RestBench, Television, ScrubTable)을 path·parent·component만으로 추가했다. 충돌·Navigation은 이번에 확인하지 않아 "미기록"으로 적었다.
  - `SceneRoot` 회전과 Cooler root scale은 별도 판단 대기라 계약으로 적지 않는다고 명시했다.
- `MI_FacilityPlacementGrid` 표현값: 수치를 지우고 parameter 이름과 MI override 원본만 남겼다.
- 구조 사실의 근거: `PLACEMENT-FOOTPRINT-PREVIEW` 사전 조사(`Saved/Claude/FPV/01_defs.json`, 같은 커밋 기준 디스크 읽기)와 이번 진단의 BP_Bath CDO 재로드.

`.md/Unreal/InteractionUISystem.md` `Bath Water 관리 화면` 절
- 용량 요약의 pt·px 수치를 각 TextBlock `Font`·wrap 설정·slot `Padding` 원본 참조로 바꿨다.
- 지도 선 두께·색의 원본(`WBP_BathWaterMap` Class Defaults 4개)과, 두께 보정을 native가 소유한다는 점을 기록했다.
- 타일 hierarchy(3절)와 Q1 A, 상태색·불투명도 원본(Class Defaults 5개)을 기록했다.
- `ManagementSize` 1024×576 수치를 Width/Height Override 원본 참조로 바꿨다. 색 이름 나열을 색 프로퍼티 원본 참조로 바꿨다.
- 다른 절(컴퓨터 연결 등)은 범위 밖이라 그대로 뒀다.

줄 수: `PlacementSystem.md` 99줄, `InteractionUISystem.md` 87줄.

## 6. 사용자 PIE 관찰 항목

- `MAPG-001`: 컴퓨터 `관리` 탭 지도의 세로·가로 칸 선이 모두 같은 굵기·밝기로 보이고 간격이 고르다. 경계선 4개가 보이고, 화면을 나갔다 다시 들어와도 같다.
- `MAPG-002`:
  - `Bath`·`Bath2` 타일이 각각 footprint 크기(이전 작은 상자보다 가로 약 1.7배, 세로 약 4배)를 덮고, 위치가 실제 목욕 공간 배치와 같다.
  - 타일 안 어디를 눌러도 해당 욕탕이 선택된다.
  - 글자는 타일 가운데에 있고 타일 밖으로 넘치지 않는다.
- `MAPG-003`: 목욕 공간을 한 번 넓힌 뒤 칸 수가 넓어진 Zone 크기 / 굵은 칸 간격만큼 늘고 모든 칸 선이 보인다. 타일은 줄어든 비율로 footprint를 덮고, 글자가 타일보다 크면 글자 묶음이 타일에 맞게 축소된다(Q1 A).

## 7. 잔여·미검증

- 화면 판정(선 표시, 타일 크기, 글자 축소, 클릭 영역)은 숨김 Editor에서 확인할 수 없어 사용자 PIE로 넘긴다(위 6절).
- 정본의 "미기록" 항목(6종 body의 충돌·Navigation, 부속 mesh 충돌)은 확인하지 않았다.
- 범위 밖 관찰: `.md/Unreal/BathWaterSystem.md`의 utility 표(Visual scale, PlacementFootprint Extent / Relative Z)에도 수치가 복제돼 있다. 같은 원본 참조 원칙으로 정리할지 마스터가 다음 작업에서 정한다.
- `USER_UNREAL.md` 변경 없음(남은 수동 작업 없음).
