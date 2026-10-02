# REPORT_UNREAL_EDITOR — EXP-U2 확장 구입 수직(홀 1회) Editor 작업

- 작업 ID: `EXP-U2`
- 단계: Editor 작업
- 상태: 보류 — 구현 복귀 2건(R1 미리보기 글자 표시, R2 `Service.BlueprintLoad` 상품 수 고정), 책임 단계 구현(R1의 Z 위치 문구는 설계 6.4와 함께 판단), 재개 조건: 구현 수정·빌드 뒤 Editor가 편집 world 미리보기 글자 화면만 다시 확인(asset 변경 없음)과 자동화 재실행. allowlist Content는 모두 저장·재로드 완료이며 게임 PIE 시나리오는 막히지 않는다.

(Editor 워커 전문을 마스터가 저장)

## 1. 범위와 실행 환경

- 입력: `PROMPT_UNREAL.md`(구현, 완료), 시작 커밋 `337f901`, 브랜치 `work/EXP-U2`. 바이너리 `UnrealEditor-BathhouseSim.dll` 17:55가 `Source`의 모든 파일보다 새로웠다(리뷰 승인 소스로 빌드된 상태).
- 시작 기준선: `git status` 깨끗함, Editor dirty 0. 대상 7개 파일을 `Saved/MigrationBackup/20261002_EXP-U2/`에 백업하고 SHA-256을 기록했다(`manifest_before.sha256`, 저장 뒤 `manifest_after.sha256`). 저장 직전마다 디스크 해시를 기준선과 대조했다.
- 실행 경로:
  - 작업용 숨김 Editor `-ModelContextProtocolStartServer`와 queue harness(`Saved/Claude/EXP-U2/exp_harness.py`, 공식 `unreal` Python API).
  - MCP(포트 8000, 소유 PID 확인)는 `CaptureViewport` 화면 캡처에만 썼다.
  - 새 프로세스 headless Automation.
  - PIE와 Computer Use는 쓰지 않았다. BeekeepingSim Editor(PID 30900)는 건드리지 않았다.
- 비저장 로드 확인(새 빌드): `DA_BathhouseExpansion_Default`, `WBP_ComputerScreenRoot`(`BS_UP_TO_DATE`), 공간 3개, `DA_ShopCatalog` 모두 오류 없이 로드되고 VALID였다. MapCheck 오류 0.

## 2. 변경 대상과 결과 (모두 개별 Save, 새 프로세스 재로드 확인)

| 대상 | 변경 | 검증 |
|---|---|---|
| `/Game/Bathhouse/Data/Expansion/DA_BathhouseExpansion_Default` | `Max Purchase Count` 0→2, `Purchase Prices` []→[100000, 300000]. `Tiers` 3줄(3/2, 4/4, 8/8)은 확인만 | VALID, ScreenContract의 `ValidatePurchaseData` 통과 |
| `/Game/Bathhouse/Data/Shop/DA_ShopCatalog` | 맨 뒤 21번째 `ClothesLocker1`, `bForSale` true, `1칸 락커`, 8000, `DA_FacilityPlacement_ClothesLocker_1`, ItemBox·Icon 비움. 기존 20개 순서 유지 | VALID. 정의에 `Facility.Discardable` 없음, `LockerSlotCount` 1 |
| `Space_Hall` `…/8/7N/V36YOPHA8C46E77EZIX78C` | `Expansion Steps` 남 400, 북 400 | 오류 0·경고 0 |
| `Space_Bath` `…/6/85/2UD4T92UQSBNFKC3N8BZFK` | 북 400, 남 400 | 오류 0, 기존 opt-out 경고 2 |
| `Space_Work` `…/9/0G/5Z3D27MZMK7HDNFHFAEYU2` | 서 400, 동 400 | 오류 0·경고 0 |
| `NavMeshBounds` `…/8/8U/DVJA0LL4M6BXDCMLJ35BD5` (조건부) | scale Y 10→13. 범위 Y ±1000→±1300, X·Z와 위치는 유지 | VALID. 아래 3절 |
| `/Game/Bathhouse/UI/WBP_ExpansionSpaceOption` (신규) | parent `ExpansionSpaceOptionWidget`, BindWidget 6개 | `BS_UP_TO_DATE`, VALID |
| `/Game/Bathhouse/UI/WBP_ExpansionScreen` (신규) | parent `ExpansionScreenWidget`, BindWidget 17개 | `BS_UP_TO_DATE`, VALID |
| `/Game/Bathhouse/UI/WBP_ComputerScreenRoot` | `ExpansionTabSize > ExpansionTabButton > ExpansionTabLabel`(`확장`)을 `ShopTabSize` 뒤에 추가(크기 132×32, 간격·style·글꼴은 상점 탭 복사), Switcher [2] `ExpansionScreen`(Fill). 기존 child 0·1과 계층·이름 유지 | `BS_UP_TO_DATE`, VALID, 탭·Switcher 순서 재로드 확인 |

- 저장 상태의 `Editor Preview Expansion Count`는 세 공간 모두 0이다. 재로드에서 글자 component 0개.
- 확인만 한 항목:
  - `ExpansionAuthority`: `Expansion Definition` = `DA_BathhouseExpansion_Default`, `Initial Tier Index` 0, VALID.
  - `KeyRack`: `Pair Transforms` 8개 ≥ 도달 가능한 최대 열쇠 8, VALID.
- Settings `Editor Preview Label World Size Cm`: C++ 기본값을 유지했다. Config 변경 없음. 크기 문제가 아니라 R1이 원인이다.
- `git status`: 위 9개 Content 파일(신규 2)과 Unreal 정본 5개만 바뀌었다. `DefaultMap.umap`과 allowlist 밖 asset은 저장하지 않았다.

## 3. 넓힘 방향 판단과 끝 모습 점검

- 방향은 지시서 11절과 계약 4.7의 제안을 그대로 썼다. 홀은 동(목욕공간 맞닿음)·서(출입구)를 빼고, 목욕공간은 서(홀)를 뺐다. 작업공간 천장 윗면은 Z −55라 지형(Z 0)보다 55cm 아래다. 서쪽 띠(홀 서쪽 벽 밖 마당 아래 50cm 포함)와 동쪽 띠(목욕공간 바닥 아래)는 모두 땅속에 묻혀 보이지 않고 충돌하지도 않는다.
- 끝 모습 바깥 상자(cm, world):
  - 홀 X −820~1320, Y −1120~1120
  - 목욕공간 X 1320~2360, Y −1070~1070
  - 작업공간 X −870~1370, Y −670~270, Z −395~−55
- 넓힘 띠의 Level Actor: 걸리는 것은 `Computer`의 DrawFrustum과 `BP_ShopDeliveryPoint`의 EditorBillboard·Arrow뿐이다. 모두 편집 전용이고 충돌이 없다. 실제 설비·장식·마당 물건은 없다(`05_band_check.py`, `06_actor_comps.py`).
- 지형: 지상 띠마다 9×5 지점에서 WorldStatic·WorldDynamic object trace를 쐈다. 가장 높은 hit가 Landscape Z 0이고 지상 바닥 판 아래면(Z 5)보다 낮다.
- `NavMeshBounds`:
  - 넓힘 목록을 넣자 홀·목욕공간에 오류 `목록 끝까지 넓힌 모습: …바닥이 하나의 손님 길 범위(NavMeshBoundsVolume)에 다 들어오지 않습니다`가 나와 조건부 수정을 적용했다. Y만 늘렸고 Z 범위(−100~600)는 그대로라 작업공간 바닥은 계속 범위 밖이다.
  - 편집 world에서 미리보기 2회와 `RebuildNavigation`으로 확인했다. 홀 남·북 띠와 목욕공간 남·북 띠 지점이 Nav 위로 투영되고, 생성기에서 각 띠까지 경로가 있다. 작업공간과 작업공간 띠에는 Nav가 없다(`09_nav_probe.py`).
  - 이때 dirty가 된 `RecastNavMesh` package(`…/9/GO/03SG1X02WXVMF5RN4NTIOA`)는 allowlist 밖이라 저장하지 않았다. runtime Dynamic 생성이므로 영향 없다.
- 미리보기 확인:
  - 1회·2회에서 `ZoneBounds` extent와 상대 위치, 공간 bounds가 넓힌 모습을 따랐다.
  - 3을 넣으면 2로 clamp됐다.
  - 글자 문구는 `넓힘 미리보기 N회`이고 겹침 표시는 없다(겹침 없음과 일치).
  - 화면 캡처(`cap_top_p2.png`, `cap_hall_p2_oblique.png`)에서 넓힌 벽·천장 모습을 확인했다.

## 4. 복귀 항목 (구현)

- **R1 편집 world 넓힘 미리보기 글자가 보이지 않음**(`PROMPT_UNREAL.md` 4절 멈춤 조건 "미리보기 글자 방향·크기 이상(코드 문제)").
  - 근거: `10_label_props.py`. component는 world Z = 천장 판 윗면(홀 395 = Zf 25 + 천장 350 + 판 20), forward +Z, up +Y, 글꼴 `/Engine/EngineFonts/RobotoDistanceField`, 재질 `DefaultTextMaterialOpaque`, `bHiddenInGame` true, `IsEditorOnly` true다.
  - (a) 천장 판 윗면과 같은 평면이다. `cap_hall_label_close.png`에서는 보이지 않고, 5cm 올린 `cap_hall_label_lift.png`에서는 보인다.
  - (b) 이 글꼴에 한글 glyph가 없어 숫자 `2` 말고는 네모로 나온다.
  - (c) 작업공간 글자(Z −55)는 홀 바닥과 지형 아래라 위에서 볼 수 없다.
  - 방향(위에서 읽힘, 위쪽 북)은 맞다.
  - 설계 근거: `PROMPT_IMPLEMENTATION.md` 6.4 "Z = 천장 판 윗면"과 `Architecture/BuildingSystem.md` 205행.
  - 영향 asset: 없음. 글자는 Transient 생성물이고 넓힘 데이터는 수정이 필요 없다.
- **R2 `BathhouseSim.Service.BlueprintLoad` 실패.** `ServiceBlueprintLoadAutomationTests.cpp:145`가 `Catalog->Products.Num()`을 리터럴 20과 비교하는데, 계약대로 21개가 되어 실패한다. 테스트 기대값을 고치는 구현 문제다. Content는 계약과 맞다.

## 5. Compile / Validation / Save / Reload / 자동화

- Compile: 새 WBP 2개와 root 모두 `BS_UP_TO_DATE`.
  - 생성 세션의 "added but did not get a GUID" ensure와 첫 재로드의 "deleted but still has a GUID"(복제 원본 `WBP_ComputerSampleScreen`의 위젯 5개)는 선례 절차대로 처리했다. 새 프로세스에서 Compile과 강제 Save를 했다.
  - 그 뒤 headless Automation 실행 로그에서 GUID 관련 메시지 0, `Ensure condition failed` 0.
- Data Validation(`15_validate_all.py`, 재로드 `17_fresh_readback.py`): 대상 asset 7개와 Level actor 7개 모두 오류 0. 경고는 기존 `Space_Bath` opt-out 2개와 `DA_FacilityPlacement_ClothesLocker_1`의 `Recovery Item Mesh` 미지정 1개(allowlist 밖, 기존 상태)다.
- Save: `EditorAssetLibrary.save_asset`(데이터·WBP), `EditorLoadingAndSavingUtils.save_packages`(external actor 4개). Save All은 쓰지 않았다. 저장 뒤 dirty는 Recast(미저장)뿐이었다.
- Reload: 새 Editor 프로세스(PID 29892)에서 2절의 모든 값, BindWidget 이름·타입, parent, 탭·Switcher 순서, Validation을 확인했다(`17_fresh_readback.json`). 같은 세션 readback으로 대신하지 않았다.
- 자동화(UE_BUILD_POLICY headless, `-DDC-ForceMemoryCache`, 필터 `BathhouseSim`): 173개 중 172개 통과.
  - 실패 1: `Service.BlueprintLoad`(R2).
  - `Expansion.Content.ScreenContract` 통과(구현 단계에서는 실패하던 테스트).
  - `Computer.Input.ScreenWheelContentContract` 통과, `Expansion.*` 11개 모두 통과.
  - 로그 `Saved/Logs/auto_exp_u2_editor.log`, 리포트 `Saved/Automation/Reports/20261002/exp_u2_editor`.

## 6. 세션·프로세스 기록

| PID | 용도 | 종료 |
|---|---|---|
| 23616 | 작업(수정·저장) | Recast dirty(미저장) 때문에 `QUIT_EDITOR`가 저장 창에서 멈출 수 있었다. Python `Package`에 dirty 해제 API가 없어, 명령줄·시작 시각을 확인한 이 PID만 강제 종료했다. allowlist는 그 전에 모두 저장했다 |
| 22836 | 재로드 시도 | 앞선 강제 종료로 남은 autosave 복원 데이터(Recast)의 Restore Packages 창 때문에 숨김 창이 frame 0에서 멈췄다. 이 PID만 종료했다. `Saved/Autosaves/PackageRestoreData.json`과 Recast `_Auto1.uasset`은 `Saved/MigrationBackup/20261002_EXP-U2/autosave_discard/`로 옮겼다(삭제 아님) |
| 29892 | 새 프로세스 재로드·GUID 정리 | dirty 0 확인 뒤 `QUIT_EDITOR`로 정상 종료. 복원 데이터 `RestoreEnabled=false`, 비어 있음 |

- 최종 상태: BathhouseSim Editor 없음, 포트 8000 리스너 없음, BeekeepingSim(30900)은 그대로다.
- 교훈(회고 입력): 버릴 dirty가 있는 작업용 Editor를 강제 종료하면 autosave 복원 창이 다음 숨김 실행을 막는다. 편집 world에서 Nav를 다시 만들면 Recast external package가 dirty가 된다.

## 7. 정본 갱신

- `.md/Unreal/BuildingSystem.md`: 넓힘 목록 원본 위치와 현재 방향, 끝 모습 점검, 미리보기 사용법과 글자 현재 상태(R1), 글자 크기 Settings 위치, Validation 상태.
- `.md/Unreal/ShopSystem.md`: 21번째 `ClothesLocker1` 줄(가격 원본 = 그 줄 `Price`).
- `.md/Unreal/InteractionUISystem.md`: 탭 3개와 Switcher [2], `WBP_ExpansionScreen`·`WBP_ExpansionSpaceOption` 구조, 확인 패널 배치, 스크롤 영역 없음. 같은 문서의 `ScreenWidget.WidgetClass` 기록(탭 root)은 현재 상태와 맞아 고치지 않았다.
- `.md/Unreal/FacilitySystem.md`: 확장 정의 필드 원본 위치, `ExpansionAuthority`·`KeyRack` 검증 결과.
- `.md/Unreal/WorldSystem.md`: `NavMeshBounds`가 끝 모습을 덮도록 넓힌 사실과 편집 world Nav 확인.
- 수치는 정본에 복제하지 않았다. 원본 asset·instance 위치만 적었다. 이 보고서의 수치는 수행 기록이다.
- `USER_UNREAL.md`: 추가할 항목이 없다. 남은 실제 Editor 수동 작업이 없다. 참고로 `USER_UNREAL.md` 109행의 오래된 `WidgetClass` 기록은 이번 범위에서 읽어 확인하지 않아 그대로 두었다.

## 8. 시나리오별 사용자 PIE 관찰 항목

| 시나리오 | 관찰 항목과 기대 결과 |
|---|---|
| EXP-020 | 처음 쓰면 `관리` 탭이 기본, 탭 순서 `관리 · 상점 · 확장`, 확장 탭 버튼이 다른 탭과 같은 크기·색 |
| EXP-021 | `현재 확장 단계: 0`, `설치된 락커 칸 2/2`, `이번 구입 가격 100,000원`, 잔액, 세 카드 크기 `현재 → 다음`, 홀 카드에 열쇠 3→4·한도 2→4 두 줄이 잘리지 않음, 구입 버튼 꺼짐 |
| EXP-022 | 잔액 부족이면 `N원 부족`, 버튼 꺼짐. 손님 결제로 충분해지면 탭을 다시 열지 않아도 켜짐 |
| EXP-023 | 홀 → 구입 → 확인: 돈 한 번 감소, 홀 남쪽 벽 4m 후퇴, 바닥·천장·조명 확장, 4번 열쇠, `확장 완료`·`현재 확장 단계: 1`·`2/4` |
| EXP-024 | 넓어진 남쪽 바닥 걷기, 홀 설비 미리보기·설치, 몇 초 뒤 손님이 넓은 바닥 이용(Nav 범위는 이제 띠를 덮음) |
| EXP-025 | 넓히는 순간 옛 남쪽 벽 근처의 플레이어·손님·물건이 끼이거나 사라지지 않음 |
| EXP-026 | 확인 대기 중 `취소`·다른 탭·컴퓨터 이탈 → 변화 없음, 다시 오면 구입 버튼부터 |
| EXP-027 | `확인` 연타 → 돈 변화·열쇠 추가 한 번. 구입 버튼 자리에는 확인 대기 때 안내 문구가 오므로 구입 버튼 더블클릭이 확인으로 이어지지 않음 |
| EXP-028 | 확장 1회 뒤 상점 맨 끝 `1칸 락커`(8,000원) 구입·배송·개봉·설치 → `3/4`, 손님 3명 이상 동시 사용 |
| EXP-029 | 0회에서 1칸 락커 구입은 되고 설치는 한도 초과 문구로 막혀 손에 남음. 홀 확장 뒤 설치됨 |
| EXP-030 | 락커 아이템은 목욕공간·지하에서 공간 불가 문구, 쓰레기 수거 구역에서 사라지지 않음 |
| EXP-031 | `ExpansionAuthority`의 `Expansion Definition`을 저장하지 않고 비운 채 PIE → `확장을 사용할 수 없습니다`, 돈 불변, `LogBathhouseExpansion` Error 한 번. 끝나면 되돌림 |
| EXP-032 | 관리·상점 탭 동작과 휠 스크롤이 이전과 같음 |

## 9. 산출물 경로

- 스크립트·결과·캡처: `Saved/Claude/EXP-U2/`(README.md, 01~18, `01_inspect.json`, `03_spaces.json`, `17_fresh_readback.json`, `cap_*.png`, queue 출력)
- 백업·해시: `Saved/MigrationBackup/20261002_EXP-U2/`
- 자동화: `Saved/Logs/auto_exp_u2_editor.log`, `Saved/Automation/Reports/20261002/exp_u2_editor/`
