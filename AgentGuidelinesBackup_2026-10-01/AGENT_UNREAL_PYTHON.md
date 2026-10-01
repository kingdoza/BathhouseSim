# Unreal Editor Python API 작업 절차

## 목적과 범위

Unreal MCP toolset에 없는 Editor 작업을 **Unreal Editor 공식 Python API(`unreal` 모듈)** 로 수행하는 절차다. 상점 인계(2026-09-27~28)에서 실제로 사용해 검증된 방법만 적는다.

- 대상: `.md/PROMPT_UNREAL.md` allowlist에 있는 asset과 사용자가 이 작업에서 명시적으로 승인한 추가 대상
- 전제: 사용자가 해당 작업의 Python API 사용을 승인했다. `AGENT_UNREAL_MCP.md`의 "MCP에 없는 StateTree/Widget/graph 편집 기능을 Python reflection으로 만들어내지 않는다" 규칙에 대한 예외이며, 승인 사실을 인계 문서에 기록한다.
- 읽기·조회 전용 스크립트(검증·진단)는 수정 스크립트와 분리한다.

## 가능 범위

| 작업 | Python 가능 여부 | 방법 |
|---|---|---|
| Widget Blueprint 생성·hierarchy·BindWidget·slot·style | 가능 | 샘플 WBP 복제 → reparent → WidgetTree 편집 (아래 절차) |
| Blueprint Class Default, 상속 native component 기본값 | 가능 | CDO `set_editor_property` → Compile → 개별 Save |
| DataAsset 속성, Catalog 항목 | 가능 | `set_editor_property` |
| Data Validation | 가능 | `EditorValidatorSubsystem.is_object_valid(asset, MANUAL)` |
| Project Settings(DeveloperSettings) **영구 저장** | 불가 | CDO 값은 메모리만 바뀐다. Editor를 닫고 `Config/Default*.ini`를 직접 수정한다 |
| StateTree State·Task·Transition 편집 | 부분 가능 | `StateTreeEditorData.get_editor_data(tree)` + 노드 `import_text` |
| StateTree property binding 추가·삭제, StateTree Compile | 기본 API 불가 | C++ 보조 editor plugin 필요(과거 `Saved/Codex/CodexAuthoring`). 설치는 별도 승인 |
| Level actor 배치·World Partition external actor 저장 | 이 절차 범위 밖 | 사용자 수작업 또는 별도 검증 후 도입 |

## 금지

- `.uasset`/`.umap`/바이너리를 셸·텍스트 도구로 직접 수정하지 않는다.
- `Save All`을 쓰지 않는다. 대상 package만 `EditorAssetLibrary.save_asset`으로 개별 저장한다.
- allowlist 밖 asset을 저장하지 않는다. 예상 밖 dirty package는 저장하지 않고 보고한다.
- Source(C++)·Config 수정이 필요한 원인을 Python으로 덮지 않는다. 원인과 필요한 수정만 보고하고 사용자 결정을 받는다.
- 확인하지 않은 것을 완료로 기록하지 않는다. 스크립트 성공 ≠ PIE 수용.

## 스크립트 규칙

- 위치: `Saved/Claude/<Feature>/` (예: `Saved/Claude/Shop/`). `README.md`에 실행 순서·저장 대상을 적는다.
- 파일명: 번호 + 동작 (`shop_01_author_widgets.py`, `shop_04_verify.py`). 수정 스크립트와 읽기 전용 스크립트를 섞지 않는다.
- 공통 모듈(`<feature>_common.py`)에 경로, BindWidget 계약, 레이아웃 토큰, helper를 둔다. 각 스크립트는 `unreal.Paths.project_saved_dir()` 기준으로 import하고 `importlib.reload`한다.
- **idempotent**: 다시 실행해도 같은 결과. 기존 asset은 parent가 맞을 때만 재구성하고, 이름이 같은 위젯은 재사용하되 class가 다르면 중단한다.
- 실패는 즉시 예외로 멈춘다(`require`). 조용히 건너뛰지 않는다.
- 로그 prefix를 통일한다(`SHOP_...`). 끝에 `..._COMPLETE` 또는 `..._RESULT PASS/FAIL`을 출력한다.
- 레이아웃 수치·색은 공통 모듈 상단 토큰 한 곳에만 둔다.
- 작성 후 로컬에서 `python -m py_compile`로 문법을 확인한다.

## 실행 방법

에이전트는 사용자 PC의 Editor에서 직접 실행할 수 없다(Editor의 localhost 포트에 닿지 않고, 화면 조작 권한 목록에 Unreal Editor가 잡히지 않는다). 스크립트를 폴더에 두고 사용자가 실행한 뒤, 에이전트가 `Saved/Logs`를 읽어 판정한다.

1. **Editor 안** (Editor가 열려 있고 PIE가 아닐 때)
   - Output Log 하단 입력창 왼쪽 모드를 `Cmd`로 바꾸고 `py "C:/UnrealProjects/BathhouseSim/Saved/Claude/<Feature>/<script>.py"`
   - 또는 Tools → Execute Python Script
2. **명령줄 commandlet** (Editor를 **닫은 상태**, 새 프로세스가 필요한 단계에 사용)
   ```
   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\UnrealProjects\BathhouseSim\BathhouseSim.uproject" -run=pythonscript -script="C:/UnrealProjects/BathhouseSim/Saved/Claude/<Feature>/<script>.py" -unattended -nosplash -stdout
   ```
   - PowerShell은 맨 앞 `&`가 필요하다. cmd.exe는 `&` 없이 실행한다.
   - Windows 기본 `py`/`python`으로는 실행되지 않는다(`unreal` 모듈 없음).
   - 같은 프로젝트를 연 Editor가 있으면 package 잠금이 충돌하므로 먼저 닫는다.
   - 에디터 월드 레벨 조회(액터 trace 등)는 commandlet에서 World Partition cell이 로드되지 않으므로 Editor 안에서 실행한다.
3. **로그 판정**: 가장 최근 `Saved/Logs/BathhouseSim.log`(이전 실행은 `BathhouseSim-backup-*.log`). prefix로 필터하고 `Traceback`, `LogPython: Error`, `Ensure condition failed`, 끝의 `Success/Failure - N error(s)`를 확인한다.

## 사전 준비

1. 대상 asset, 저장 allowlist, 수용 시나리오를 프롬프트에서 확정한다.
2. 수정할 기존 asset을 `Saved/MigrationBackup/<YYYYMMDD>_<topic>/`에 복사하고 SHA-256을 기록한다. Config를 고치면 ini도 백업한다.
3. 기존 asset을 수정하는 스크립트는 저장 전에 현재 파일 해시가 백업과 같은지 확인하고, 다르면 중단한다(이미 목표값이면 건너뛴다).
4. 관련 native 헤더에서 BindWidget 이름·타입, `UPROPERTY` 지정자(`EditDefaultsOnly`/`EditAnywhere`는 `set_editor_property` 가능)를 확인한다.
5. 기존 선례 스크립트(`Saved/Codex/`, `Saved/Claude/`)에서 이미 검증된 API 호출을 우선 사용한다.

## Widget Blueprint 절차

1. **생성**: 빈 WBP에는 Python으로 root를 지정할 수 없다. `/Game/Bathhouse/UI/WBP_ComputerSampleScreen`을 `EditorAssetLibrary.duplicate_asset`으로 복제하고 `BlueprintEditorLibrary.reparent_blueprint(bp, native)`로 native class에 붙인다.
2. **트리 열기**: `unreal.find_object(None, f"{path}.{name}:WidgetTree")`, root는 `...:WidgetTree.RootOverlay`. `bp/tree/root.modify()` 후 `root.clear_children()`.
3. **위젯 생성**: `unreal.new_object(cls, outer=tree, name=...)`. 재실행 시 같은 이름 객체를 찾아 `remove_from_parent()` 후 재사용하고, panel이면 `clear_children()`.
4. **배치**: `add_child_to_overlay / _vertical_box / _horizontal_box / _wrap_box`, `ScrollBox.add_child`, `set_content`. 반환 slot에 padding·alignment·size(`SlateChildSize`)를 설정한다.
5. **다른 WBP 포함**: `new_object(child_bp.generated_class(), outer=tree, name=BindWidget이름)`.
6. **native 기본값**(예: 행 위젯 클래스): Compile·Save 후 generated class CDO에 `set_editor_property` → 다시 Save. 이 WBP를 다른 WBP에 넣는 경우 **그 인스턴스 템플릿에도 같은 값을 직접 지정**한다(CDO만으로는 중첩 템플릿에 반영되지 않을 수 있다).
7. **Compile·Save**: `compile_blueprint` 후 `bp.get_editor_property("status") == BS_UP_TO_DATE` 확인, `save_asset(path, only_if_is_dirty=False)`로 강제 저장.
8. **GUID 정리 (필수)**: 복제 원본의 위젯 GUID가 `WidgetVariableNameToGuidMap`에 남는다. **1번과 다른 새 프로세스**(commandlet)에서 대상 WBP를 다시 Compile하고 강제 저장한다. 이후 새 프로세스 로드에서 `was deleted but still has a GUID` / `was added but did not get a GUID` ensure가 사라져야 한다.

알려진 정상 잡음: reparent 직후 빈 트리 Compile에서 나오는 `A required widget binding ... was not found`는 최종 Compile이 `BS_UP_TO_DATE`이면 무시한다.

## Blueprint 기본값·상속 component 절차

1. `cdo = unreal.get_default_object(bp.generated_class())`.
2. 상속 native component는 `cdo.get_editor_property("<C++ 이름>")` 또는 `get_components_by_class`에서 이름으로 찾는다. Blueprint SCS에서 추가한 component는 CDO에 없으니 구분한다.
3. `bp.modify()`, `component.modify()` 후 `set_editor_property("<C++ 속성명>", 값)`.
4. Compile 실패 시 이전 값으로 되돌리고 중단한다.
5. 개별 Save 후 CDO를 다시 읽어 값이 유지됐는지, 바꾸지 않은 관련 값(예: DrawSize, 위치)이 그대로인지 확인한다.

## Project Settings(Config)

- Python은 DeveloperSettings CDO 값을 메모리에서만 바꾼다. 저장 함수가 노출되지 않는다.
- Editor를 닫고 `Config/DefaultGame.ini` 등에 `[/Script/<Module>.<Class>]` 섹션을 추가한다. 기존 줄은 건드리지 않고 파일 줄바꿈(CRLF) 형식을 따른다.
  - soft object: `/Game/Path/Asset.Asset`, soft class: `/Game/Path/BP_X.BP_X_C`
- 열려 있던 Editor가 설정을 저장하면 덮어쓸 수 있으므로 수정 후 새로 연다.
- 검증 스크립트에서 설정 CDO 값을 읽어 확인한다.

## Data Validation과 원인 진단

- `validator = unreal.get_editor_subsystem(unreal.EditorValidatorSubsystem)`, `result, errors, warnings = validator.is_object_valid(asset, unreal.DataValidationUsecase.MANUAL)`.
- 상위 asset 오류가 원인을 숨기면(예: Catalog가 Definition의 `ValidateRuntime` 사유를 버림) 하위 asset을 직접 검증하고, native 검사 항목을 필드 단위로 다시 읽어 `DIAG_OK/FAIL`로 출력한다.
- 런타임 동작 문제(버튼 비활성, 조용한 false 반환)는 먼저 해당 native 함수의 실패 분기를 코드에서 찾고, 그 조건을 Python 진단으로 확인한다. UI가 사유를 표시하지 않으면 그 사실도 보고한다.

## 검증

1. 수정 스크립트 로그: 모든 `COMPILED ... BS_UP_TO_DATE`, `SAVED`, `DIRTY_CONTENT []`, `..._COMPLETE`.
2. **새 프로세스 read-back 스크립트**: native parent, Compile 상태, 모든 BindWidget이 root에서 도달 가능하고 타입 일치, switcher 순서, 연결값, 설정값. 결과 `VERIFY_RESULT PASS 0`과 엔진 `Success - 0 error(s), 0 warning(s)`, ensure 없음.
3. Data Validation: 대상 asset 전부 `VALID`.
4. 사용자 PIE 수용: 화면 배치·입력·클릭·시각 판정은 사용자가 확인한다. 스크립트 통과만으로 완료 처리하지 않는다.

## StateTree

- 편집 데이터: `data = unreal.StateTreeEditorData.get_editor_data(tree)`, `data.sub_trees` → `state.children / tasks / transitions`.
- Task 교체·파라미터 수정: 기존 노드를 `copy()`하고 `node.import_text(...)`, `instance.import_text(...)`로 구조체 텍스트를 넣는다. **에디터 노드 ID를 유지**해야 기존 binding이 살아남는다.
- 수정 전후 State 계층·transition을 비교해 의도하지 않은 변경이 없음을 확인한다.
- binding 추가·삭제와 Compile은 C++ 보조 plugin이 필요하다. 설치 여부와 범위를 사용자에게 먼저 확인한다.
- 텍스트 import 방식은 엔진 구조체 형식 변경에 취약하므로 매 실행 비교 검증을 함께 돌린다.

## 문제 해결

| 증상 | 원인 | 조치 |
|---|---|---|
| PowerShell `예기치 않은 토큰` | 따옴표 경로를 명령으로 인식하지 않음 | 맨 앞에 `&` |
| `No module named unreal` | 시스템 Python으로 실행 | Editor Output Log 또는 `UnrealEditor-Cmd` 사용 |
| `Ensure ... was deleted but still has a GUID` | 복제 원본 위젯 GUID 잔존 | 새 프로세스에서 Compile + 강제 저장 |
| commandlet 끝 `Failure - N error(s)` | 로그 전체 error 집계(ensure 포함) | prefix 결과와 ensure 여부로 판정, 원인 제거 후 재실행 |
| Project Settings 값이 재시작 후 사라짐 | Python은 config 저장 불가 | ini 직접 수정 |
| 기능은 연결됐는데 PIE에서 버튼이 안 눌림 | native 검사로 버튼 비활성 | 해당 `SetIsEnabled` 조건과 실패 사유를 진단 |

## 기록

- 완료 후 `.md/PROMPT_UNREAL.md`, `.md/USER_UNREAL.md`, 관련 `.md/Unreal/*System.md`에 **저장·새 프로세스 재로드로 확인된 상태만** 기록한다.
- 사용한 스크립트 경로, 백업 위치, 사용자 승인 사실, 남은 PIE 수용 항목을 함께 적는다.
- 이 절차로 해결하지 못한 항목은 원인·필요 조작·재개 조건과 함께 `USER_UNREAL.md`에 남긴다.
