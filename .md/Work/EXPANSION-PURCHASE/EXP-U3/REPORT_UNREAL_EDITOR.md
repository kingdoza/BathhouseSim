# REPORT_UNREAL_EDITOR — EXP-U3 전체 확장 묶음 Editor 작업

- 작업 ID: `EXP-U3`
- 단계: Editor 작업
- 상태: 완료

(Editor 워커 전문을 마스터가 저장)

## 1. 범위와 입력

- 입력: [PROMPT_UNREAL.md](PROMPT_UNREAL.md)(구현 완료), 설계 [PROMPT_IMPLEMENTATION.md](PROMPT_IMPLEMENTATION.md) 13.3절, 상위 계약 [../PROMPT_ARCHITECTURE.md](../PROMPT_ARCHITECTURE.md) 4.7절(D2 가격, D3 방향·양, 락커 가격).
- 브랜치·커밋: `work/EXP-U3`, 단계 시작 커밋 `cc9a4fa`, 리뷰 승인 커밋 `34c8f54`.
- 빌드: `UnrealEditor-BathhouseSim.dll`(20:20:34)보다 새 Source 파일은 없습니다. 마지막 빌드 `Saved/Logs/agent/build_u3_7.log`가 `Result: Succeeded`입니다.
- 시나리오: 원래 U3 EXP-040~046, D2·D3 EXP-021~023·040~042·047·048, D4 EXP-050~052(D4는 Content 변경 없음).
- 적용한 개선 후보:
  - FBK-001: 지형·장애물 판정은 WorldStatic·WorldDynamic 단순 trace로 했습니다.
  - FBK-002: 띠 검사가 오탐을 내자 같은 작업을 반복하지 않고 판정 방법(띠 직사각형·component 단위)부터 바꿨습니다.
  - FBK-003: 카드 잘림·미리보기 글자는 수치·구조만 확인하고 PIE 항목으로 넘겼습니다.
- 리뷰 비차단 메모 반영: PROMPT_UNREAL 5절의 StartingMoney 임시 상향 안내는 무시했습니다(사용자가 이미 1억으로 커밋).

## 2. 실행 경로와 세션

- 작업용 숨김 Editor 2회: PID 29060(작업), PID 18684(새 프로세스 재로드와 WBP GUID 정리).
  - 실행 인자: `-ModelContextProtocolStartServer -NoSplash -log -ExecCmds="py Saved/Claude/EXP-U3/exp_harness.py"`.
  - 실행 경로: 공식 `unreal` Python API를 queue 하네스로 돌렸습니다(EXP-U2 하네스 복제). MCP 도구 호출은 필요 없었습니다.
- 시작 전 상태: BathhouseSim Editor 0개, `git status` Content 변경 없음. 다른 세션 파일(`AGENTS.md`, `.md/MODELING_*`, `ArtSource`, `.md/Work/MODEL-M1`)은 건드리지 않았습니다.
- 백업: `Saved/MigrationBackup/20261002_EXP-U3/`(원본 8개, `manifest_before.sha256`·`manifest_after.sha256`). 저장 직전마다 디스크 해시를 기준선과 대조했습니다.
- 스크립트·결과: `Saved/Claude/EXP-U3/`(README에 순서). 로그는 `editor_session1.log`, `editor_session2.log`, `auto_u3_editor.log`입니다.
- 종료: 두 Editor 모두 `SystemLibrary.quit_editor()`로 정상 종료했고 PID 소멸을 확인했습니다. 1번째 Editor에는 Recast 편집 dirty만 남아 있었고 저장하지 않았습니다(디스크 변경 없음). `PackageRestoreData.json`은 비어 있습니다.

## 3. 항목별 결과

| 항목 | 결과 |
|---|---|
| 3.1 사전 load | 새 빌드에서 Fatal·load 오류 없음. 공간 3개는 예상된 전환 상태 오류(빈 벽·가격 0)였습니다. 확장 정의의 옛 필드는 Python에서 읽히지 않았습니다. WBP 3개 `BS_UP_TO_DATE` |
| 3.2 `DA_BathhouseExpansion_Default` | 재저장(파일 해시 변경 = 지운 필드 정리). `Tiers` 3줄 그대로, Validation VALID |
| 3.3 공간 넓힘 줄 | 세 공간 모두 줄 2개. `Sides` 순서는 홀 남·북, 목욕공간 남·북·동, 작업공간 동·북. 모든 항목 `Amount Cm` = 4.7 "변마다" 값, `Price` = 4.7 가격 행의 해당 칸 |
| 3.4 끝 모습 | 미리보기 2에서 Validation에 겹침·출입구·맞닿음 문제 없음. 띠(모서리 포함)에 Level Actor 없음(컴퓨터 편집 전용 frustum만 걸침). 지상 띠 지형 최고점 0(바닥 판 아래) |
| 3.5 `NavMeshBounds` | X·Y 확대: 서쪽 끝 유지, 동쪽 X 확대, Y 양쪽 확대, Z 그대로. 미리보기 2에서 `ExpansionNavOutside`·`ExpansionNavWorkCovered` 없음 |
| 3.6 저장·재로드 | 미리보기 0으로 되돌린 뒤 4개 package를 `save_packages`로 개별 저장. 새 프로세스에서 값·미리보기 0 확인. 공간 Validation 오류 0, 경고는 `Space_Bath` opt-out 2개만 |
| 3.7 `WBP_ExpansionScreen` | `StageText`·`PriceText` 삭제, `LockerText` 왼쪽 정렬(머리: 락커·잔액 두 칸). 다른 binding 15개 이름·타입 그대로 |
| 3.8 `WBP_ExpansionSpaceOption` | `CardColumn` 순서 `NameText`→`StageText`→`SizeText`→`PriceText`→`EffectText`→`StatusText`. 새 TextBlock 2개는 카드와 같은 글꼴·팔레트. `OptionSize` 높이 196 → 236 |
| 3.9 `DA_ShopCatalog` | 22·23번째에 `ClothesLocker4`(`4칸 락커`), `ClothesLocker8`(`8칸 락커`) 추가. 판매 true, 가격 4.7, 정의 연결, ItemBox·Icon 비움. 기존 21개 순서 유지. 두 정의 `LockerSlotCount` 4·8, `Discardable` 없음 |
| 3.10 확인만 | `ExpansionAuthority`(Definition 연결, Initial Tier 0), `KeyRack`(`Pair Transforms` 8개 ≥ `Tiers` 최대 열쇠 수) 모두 Validation 오류 0 |

### 편집 world Nav 확인(Recast 미저장)

- 미리보기 2에서 손님 생성기 → 홀 남쪽 끝·북쪽 띠·북서/남동 모서리, 목욕공간 남·북·동 띠·남동/북동 모서리 경로가 모두 있습니다.
- 작업공간 바닥·동·북 띠는 Nav 투영 없음(경로 없음)입니다.
- 근거: `08_nav_probe.py` 출력.

### WBP 크기 확인(FBK-003)

- 숨김 Editor에서는 Python으로 위젯 instance를 그리게 할 수 없습니다(`WidgetBlueprintLibrary.Create`·TakeWidget이 Python에 없음, `11_measure.py` 실패 기록). 그래서 정적 수치로 확인했습니다(`11b_root_layout.py`).
- root: `RootSize` 1024×576, 탭 바 44px → 화면 영역 532px.
- 화면 높이 합 약 448px ≤ 532px. 내역: 패딩 24, 머리 41, 카드 246, 정보 97, 버튼 40.
- 카드 내용 합 약 208px ≤ 236px. 한글 줄 높이를 글꼴 크기의 약 1.93배로 보수적으로 잡았고, 홀 카드 최대 경우(단계·크기·가격·효과 두 줄)입니다.
- 실제 잘림 여부는 PIE 항목입니다.

## 4. Compile·Validation·Save·재로드

| 대상 | Compile | Validation | Save | 재로드 |
|---|---|---|---|---|
| `DA_BathhouseExpansion_Default` | - | VALID | 1번째 Editor | 2번째 Editor: `Tiers` 3줄, VALID |
| `DA_ShopCatalog` | - | VALID | 1번째 Editor | 2번째 Editor: 23개, 마지막 1·4·8칸, VALID |
| 공간 3개 | - | 오류 0 | 1번째 Editor | 2번째 Editor: 줄·가격·미리보기 0, 오류 0 |
| `NavMeshBounds` | - | VALID | 1번째 Editor | 2번째 Editor: Location·Scale 유지 |
| `WBP_ExpansionSpaceOption` | `BS_UP_TO_DATE` | VALID | 1번째 → 2번째 Editor(GUID 정리) | 2번째 Editor 순서·글꼴 확인 → 자동화 프로세스 load |
| `WBP_ExpansionScreen` | `BS_UP_TO_DATE` | VALID | 1번째 → 2번째 Editor(GUID 정리) | 2번째 Editor: 두 widget 없음, binding 일치 → 자동화 프로세스 load |

- 1번째 Editor의 ensure 4건(새 `StageText`·`PriceText` GUID 없음, 지운 두 이름의 GUID 남음)은 알려진 경우입니다. 2번째 Editor의 Compile·Save 뒤 사라졌습니다(2번째 Editor 로그와 자동화 로그 모두 ensure 0).
- 진행 중 잘못 바꾼 것 하나: font struct를 참조로 복사해 `SizeText` 글꼴 크기가 바뀌었습니다. 저장 전에 15로 되돌렸고 재로드로 확인했습니다(`10b_fix_size_font.py`, 작성 스크립트도 고침).
- 최종 `git status`: Content 변경은 allowlist 8개뿐입니다.

## 5. 자동화

- 명령: UE_BUILD_POLICY headless 형식(PowerShell), `Automation RunTests BathhouseSim`.
- 결과: 177개 중 177개 통과(155개는 경고 없이, 22개는 경고 있음), 실패 0, 미실행 0, `TEST COMPLETE. EXIT CODE: 0`.
- 보고서: `Saved/Automation/Reports/20261002/u3_editor3/`, 로그: `Saved/Claude/EXP-U3/auto_u3_editor.log`.
- 지정 항목: `Expansion.Content.ScreenContract`, `Computer.Input.ScreenWheelContentContract`, `Placement.PreviewHiddenWithoutAim`, `Expansion.*` 전부 통과.
- 앞서 Git Bash로 돌린 2회는 맵 인자가 경로 변환돼 정책 명령과 달랐습니다. 결과는 같았지만 PowerShell 실행 결과만 채택했습니다.

## 6. 정본·큐 변경

- `.md/Unreal/BuildingSystem.md`:
  - 넓힘 줄 형식(`Sides`·`Price`)과 D3 방향을 반영했습니다. 전체 구입 상한 문장을 지우고 홀 줄 + 1 ≤ `Tiers` 조건을 적었습니다.
  - 띠 검사 결과와 Validation 현재 상태를 갱신했습니다.
- `.md/Unreal/FacilitySystem.md`: 확장 정의 필드(`Tiers`만), 열쇠걸이 검사 범위(효과 표 모든 줄), 4·8칸 정의 경고.
- `.md/Unreal/InteractionUISystem.md`: 카드 `StageText`·`PriceText`, 화면 머리 구조, 카드 높이 원본, 잘림은 PIE 대상.
- `.md/Unreal/ShopSystem.md`: 4·8칸 락커 상품.
- `.md/Unreal/WorldSystem.md`: Nav 범위 계약과 편집 world 확인.
- 문서에는 수치를 복제하지 않고 원본 위치만 적었습니다.
- `USER_UNREAL.md`: 새 항목 없음, 기존 항목 변경 없음.

## 7. PIE 관찰 항목 (마스터 `PIE_CHECKLIST.md` 입력)

| ID | 관찰 | 기대 |
|---|---|---|
| EXP-021 | 확장 탭을 연다 | 탭 머리에 "현재 확장 단계"·가격이 없고 락커·잔액만 보인다. 선택지마다 `확장 단계 0/2`, 전후 크기, `다음 넓힘 N원`(공간마다 다름), 홀은 효과 두 줄. 구입 버튼은 꺼져 있고 `확장 구입`(가격 없음) |
| EXP-022 | 홀 선택 후 잔액을 홀 1번째 가격보다 낮게(Editor 값 조정) | 버튼 꺼짐, `N원 부족`(홀 가격 기준). 잔액이 오르면 즉시 켜짐. 선택 전에는 부족액 없음 |
| EXP-023 | 홀을 확인해 구입 | 홀 1번째 가격 1회 차감. 남·북 벽이 동시에 물러나고 서쪽 출입구 벽·동쪽 통로 벽은 그대로. 열쇠 4번, `확장 완료`, `설치된 락커 칸 2/4`, 홀 `확장 단계 1/2`·홀 2번째 가격, 다른 공간 `0/2` |
| EXP-040·041·047 | 목욕공간(남·북·동), 작업공간(동·북) 구입 | 여러 벽이 한 번에 물러나 모서리까지 바닥·천장 직사각형 하나. 목욕공간 모서리 이음새·틈·깜빡임 없음. 늘어난 바닥(모서리 포함)에 물 얼룩 조각(손님이 있는 곳에만 생겨 시간이 걸림), 작업공간은 조각 없음. 열쇠·한도 그대로, 가격 1회. 욕탕 미리보기가 모서리까지 초록 |
| EXP-042 | 목욕 1회·홀 1회 뒤 홀 구입 | 홀 2번째 가격, 열쇠 5~8, 한도 8칸, 홀 `2/2` |
| EXP-043·044 | 홀만 `2/2`, 이어서 모두 `2/2` | 홀만 `이 공간은 더 넓힐 수 없습니다`, 다른 공간은 그 공간 가격으로 구입 가능. 모두 상한일 때만 `최대 확장 단계입니다` |
| EXP-045·048 | Editor에서 저장하지 않고 값을 바꾼 뒤 PIE(끝나면 되돌림) | 홀 줄 3개 + `Tiers` 4줄이면 홀 3번 구입, 매번 홀의 그 번째 가격. 북 2m·남 6m는 각각 적용되고 열쇠·한도는 1회만 바뀜. 오류·경고는 `Space_Hall` 저장 또는 우클릭 Validate로 본다 |
| EXP-046 | 4·8칸 락커 구입·배송·개봉·운반·설치 | 상점 마지막에 1·4·8칸 락커. 설치 칸 수가 4·8 오른다. 홀 2회(한도 8칸)에서 1칸 락커 2개 설치 시 4칸은 설치(6/8), 8칸은 한도로 막힘. 8칸 설치는 1칸 락커를 Q 길게 눌러 회수한 뒤 |
| EXP-050·051 | 아이템을 든 채 거리 밖, 계단 쪽 벽, 천장, 출입구 밖 지형, 하늘을 차례로 본다 | 빨간 잔상 없이 미리보기가 사라지고, 돌아오면 그 자리에 즉시 보이며 회전 유지. 좌클릭해도 설치되지 않고 안내 문구. 아이템·격자 그대로 |
| EXP-052 | 공간 불허·한도·겹침 자리 조준 | 조준한 자리에 빨간 미리보기와 이유 문구(숨지 않음) |
| 회귀 | EXP-024·025·027·028·029 | 기존대로 |
| 손님 | 넓어진 홀 남·북, 목욕공간 남·북·동 바닥(모서리 포함)까지 손님이 다니는가(몇 초 뒤, Recast Dynamic) | 다닌다. 작업공간에는 손님 길 없음 |

숨김 Editor에서 판정하지 못한 화면 항목(FBK-003)입니다. 구조·수치만 확인했습니다.

| 항목 | 관찰 | 기대 |
|---|---|---|
| 카드 잘림 | 확장 탭 1024×576에서 카드 문구(`확장 단계`, 크기 변화, `다음 넓힘`, 홀 효과 두 줄, 상한 공간의 상태 문구)와 아래 정보·구입 영역 | 잘리거나 겹치지 않음(Editor는 정적 높이 합만 확인) |
| 머리 배치 | 탭 머리 | `설치된 락커 칸 N/M` 왼쪽, `잔액` 오른쪽, 빈 자리 어색함 없음 |
| 미리보기 글자 | 편집 화면에서 세 공간 `Editor Preview Expansion Count` 2 | 글자에 ` · 겹침 있음`이 없음(Validation 겹침 0은 확인함). 확인 뒤 0으로 되돌리고 저장하지 않음 |

## 8. 미검증·남은 것

- PIE 입력·화면·실제 렌더(카드 잘림, 모서리 이음새, 손님 이동, D4 잔상)는 사용자 PIE 몫입니다(7절).
- 편집 world Nav 경로는 Recast 편집 결과로 본 것이며 PIE의 Dynamic 재생성과 다를 수 있습니다(PIE "손님" 항목).
- `USER_UNREAL.md`에 넘긴 실제 Editor 작업은 없습니다.
