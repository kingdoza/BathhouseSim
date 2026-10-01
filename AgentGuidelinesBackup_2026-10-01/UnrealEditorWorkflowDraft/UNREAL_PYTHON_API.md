# Unreal Editor Python API 작업 절차

## 상태와 적용 범위

[AGENT_UNREAL_EDITOR.md](AGENT_UNREAL_EDITOR.md)의 Python 기술 절차 초안이다. 독립 Python 에이전트나 작업별 예외 승인 절차를 만들지 않는다.
채택 전에는 기존 정본이 적용된다. 채택 후에는 통합 역할의 승인된 범위에서 공식 `unreal` API와 검토·설치된 Editor 보조 도구를 사용한다.
공통 프로젝트·프로세스·PIE·종료 정책은 [UNREAL_MCP_CONNECTION.md](UNREAL_MCP_CONNECTION.md)를 따른다.

## Capability 확인

| 대상 | 사용할 수 있는 경로 | 한계와 확인 |
|---|---|---|
| Blueprint CDO·상속 native component, DataAsset | `get_editor_property/set_editor_property` | 실제 노출·쓰기 가능 여부와 저장 후 유지 확인 |
| Widget Blueprint hierarchy·slot·style | 검증된 WBP 복제·reparent와 WidgetTree API | 빈 WBP root 설정·GUID 정리는 버전별 확인 |
| StateTree 구조 읽기 | `StateTreeEditorData.get_editor_data`와 상태·Task·Transition 순회 | binding·schema·추가 필드의 노출 여부 별도 확인 |
| StateTree 편집 | 현재 공개 API와 검증된 구조체 `import_text` 선례 | ID·binding 보존; 필요한 Compile까지 가능해야 시작 |
| StateTree binding 변경·Compile, 임의 graph 편집 | 현재 설치된 C++ Editor helper 또는 제공 API | 기본 API가 부족하면 신규 구현 단계로 인계 |
| Level actor·external actor 저장 | 실제 로드된 Editor world와 지원 API | World Partition 로드·개별 package 저장·재로드 먼저 검증 |
| DeveloperSettings 영구 저장 | 제공된 저장 API·helper의 검증된 경로 | CDO 변경만으로 Config 저장을 주장하지 않음 |
| Data Validation | `EditorValidatorSubsystem` | signature·반환값, validator side effect 확인 |

- 표는 지원 보장이 아니다. 실제 UE 버전·플러그인·함수·property·signature를 확인하고 실패를 숨기지 않는다.
- `unreal`은 Unreal의 Python 환경에서 사용한다. 시스템 Python은 문법 검사·파일 처리용이며 Editor API 실행 경로가 아니다.
- Python Editor Script Plugin 활성화를 확인한다. 필요한 `.uproject`·Config·플러그인 변경은 임의로 하지 않는다.

## 읽기와 수정 분리

- 읽기 전용 스크립트에는 `modify`, `set_editor_property`, Compile, Save, asset 생성·삭제와 runtime mutation을 넣지 않는다.
- 조회는 schema·CDO·hierarchy·참조·필요 속성·현재 compile 상태를 읽는다. 검증을 위해 Compile을 실행하지 않는다.
- 수정·검증 스크립트는 파일·실행 결과를 분리한다. 재로드 검증은 변경 API를 호출하지 않는다.
- 편집 데이터 로드의 자동 compile·dirty·ensure도 로그와 기준선으로 구분하고 조사에서 저장하지 않는다.

## 스크립트 위치와 작성

- 재사용 도구는 `Scripts/Unreal/`, 작업별 스크립트·요약·상세 로그는 `Saved/Codex/Unreal/<TaskId>/`를 기본 위치로 한다.
- 기존 `Saved/Codex/`·`Saved/Claude/` 선례는 버전·대상·API를 확인해 재사용한다. 이 초안으로 기존 파일을 이동하지 않는다.
- 작업 `README.md` 또는 manifest에 모드·실행 순서·대상·저장 allowlist·기대값·로그·백업을 적는다.
- 파일명 예: `01_inspect.py`, `02_apply.py`, `03_verify.py`. 경로·BindWidget 계약·목표값은 공통 모듈이나 승인 입력 한 곳에서 읽는다.
- 재실행 시 같은 결과가 되게 작성한다. 기존 객체의 class·parent가 다르면 중단하고 무조건 재생성하지 않는다.
- `require`로 필수 조건·반환값을 확인한다. 예외를 출력한 뒤 성공처럼 계속하지 않는다.
- allowlist 검사 후 변경·저장한다. 조회용 의존 asset과 저장 대상을 구분한다.
- 필요하면 `ScopedEditorTransaction`과 `modify`를 쓴다. transaction만으로 자동 rollback이 보장된다고 가정하지 않는다.
- 로그 prefix와 `RESULT PASS/FAIL/BLOCKED`를 통일하고 오류·경고·ensure 원문은 상세 로그에 남긴다.
- 시스템 Python의 `python -m py_compile <script>`로 문법을 확인한다. 문법 통과는 Unreal API·asset 검증이 아니다.

## 백업과 동시 변경

1. exact target·목표값·필수 검증·저장 package와 현재 class·parent·참조·기존 dirty를 확인한다.
2. 디스크 기준선을 `Saved/MigrationBackup/<YYYYMMDD>_<TaskId>/`에 복사하고 원본 경로·SHA-256을 기록한다.
3. 백업은 파일 복사만 한다. 메모리와 디스크는 다르므로 기존 사용자 dirty가 있으면 충돌을 먼저 판정한다.
4. 저장 직전 디스크 해시를 최초 기준선 또는 이번 작업의 마지막 저장 해시와 비교한다. 외부 변경이면 저장을 중단한다.
5. 이미 목표 상태면 불필요한 수정·저장을 건너뛴다. 목표값 일치만으로 다른 사용자 변경을 덮어쓰지 않는다.
6. 저장 후 manifest를 갱신한다. 복구는 이번 작업 변경만 대상으로 하고 사용자 상태를 잃는 파일 복원을 하지 않는다.

## 실행 경로

에이전트가 실행과 판정까지 담당한다. 과거 환경에서 직접 실행하지 못했다는 기록을 현재 환경의 영구 제한으로 취급하지 않는다.

1. 현재 Editor에 등록된 공식 Python 실행 도구가 있으면 schema를 확인하고 같은 프로세스에서 사용한다.
2. 이미 설정·허용된 공식 Python 원격 실행 경로가 있으면 대상 PID·프로젝트·실제 지원을 확인한다. 임의 endpoint를 만들지 않는다.
3. 대상 Editor가 없고 작업이 headless를 지원하면 `UnrealEditor-Cmd.exe`로 직접 실행한다.
4. 사용자 Editor는 유지한다. 실행 도구가 없다는 이유로 종료하거나 같은 프로젝트의 commandlet를 추가하지 않는다.
5. 어떤 허용 경로도 실행할 수 없을 때만 정확한 명령·대상·제약·로그 확인법을 큐에 남긴다. 작성만으로 완료하지 않는다.

PowerShell 예시다. 동일 프로젝트 프로세스 부재, 실행 권한·플러그인·모든 경로를 먼저 확인한다.

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' `
  'C:/UnrealProjects/BathhouseSim/BathhouseSim.uproject' `
  '/Engine/Maps/Templates/Template_Default' `
  -run=pythonscript `
  -script='C:/UnrealProjects/BathhouseSim/Saved/Codex/Unreal/<TaskId>/03_verify.py' `
  -unattended -nullrhi -NoSplash -NoSound -DDC-ForceMemoryCache -stdout `
  -log='C:/UnrealProjects/BathhouseSim/Saved/Codex/Unreal/<TaskId>/verify.log'
```

- `<TaskId>`를 실제 ID로 바꾸고 로그 폴더를 준비한다. [공통 headless 정책](../AGENT_WORKFLOW.md)을 적용한다.
- 파일·맵·스크립트를 작업에 맞게 바꾼다. 예시를 실제 수행한 검증 결과로 기록하지 않는다.
- 실제 맵이 필요하면 지원되는 `LevelEditorSubsystem.load_level` 등으로 대상 Level을 명시적으로 로드한다.
- World Partition의 미로드 cell·actor를 “없음”으로 판정하지 않는다. 실제 world·trace·입력은 live 경로가 필요할 수 있다.
- `-nullrhi` 결과로 렌더링·화면·실제 입력 수용을 증명하지 않는다.
- 사용자 실행이 필요한 경우 Cmd 입력 예: `py "C:/UnrealProjects/BathhouseSim/Saved/Codex/Unreal/<TaskId>/03_verify.py"`. 화면 입력을 임의 자동화하지 않는다.

## 결과와 토큰 효율

- 반복은 스크립트에서 처리하고 요약만 모델에 반환한다. 상세 결과는 JSON·로그 파일로 남긴다.
- 요약에는 status, 검사·변경·저장 수, 대상별 실패·미확인과 상세 파일 경로를 넣는다.
- 대량 `export_text`·`dir` 덤프는 필요한 API 진단 범위로 제한한다. 필요한 필드·차이·ID 관계를 우선 반환한다.
- 파싱 실패·미노출 binding·부분 로드·요청 미완료를 빈 배열·PASS로 바꾸지 않는다.
- 종료 코드, 이번 로그의 `Traceback`·`LogPython: Error`·`Ensure condition failed`와 엔진 최종 결과를 함께 확인한다.
- 스크립트 PASS와 엔진 Failure가 충돌하면 조사한다. 에셋 로드 전 환경 Fatal은 asset 검증 결과가 아니다.

## Blueprint·DataAsset

1. 실제 class·native parent·property를 확인한다. DataAsset은 기존 항목의 식별자·순서·참조도 비교한다.
2. CDO는 `unreal.get_default_object(bp.generated_class())`로 읽고 생성 class의 유효성을 확인한다.
3. 상속 native component는 CDO property 또는 class·이름으로 식별한다. SCS component를 native component로 가정하지 않는다.
4. `bp/asset/component.modify()` 뒤 노출된 property를 변경한다. `UPROPERTY` 편집 지정자만으로 setter 지원을 확정하지 않는다.
5. 필요한 Compile 결과·status를 확인한다. 실패하면 이번 작업의 이전 값으로 안전하게 복구하고 저장하지 않는다.
6. 개별 Save·디스크 reload에서 목표값·참조·순서와 유지해야 할 관련 값을 재검사한다.

## Widget Blueprint

1. native Widget의 `BindWidget` 이름·타입, 승인 레이아웃과 복제 원본·신규 대상 allowlist를 확인한다.
2. 검증 선례는 `WBP_ComputerSampleScreen` 복제 → `BlueprintEditorLibrary.reparent_blueprint`다. exact 원본 경로·root는 실제 asset에서 확인한다.
3. `unreal.find_object`로 `WidgetTree`·root를 연다. `bp/tree/root.modify()` 후 승인된 hierarchy만 재구성한다.
4. `unreal.new_object(cls, outer=tree, name=...)`를 쓰고 같은 이름은 class가 맞을 때만 재사용한다. 기존 참조·연결을 확인해 분리한다.
5. `add_child_to_overlay/vertical_box/horizontal_box/wrap_box`, `ScrollBox.add_child`, `set_content` 등의 실제 API로 배치한다.
6. 반환 slot의 padding·alignment·size와 style을 적용한다. 재사용 panel의 `clear_children`는 승인된 자식 범위에만 쓴다.
7. 다른 WBP 포함은 generated class와 exact BindWidget 이름을 쓰고 class·트리에서의 도달 가능성을 확인한다.
8. child class 기본값은 CDO뿐 아니라 화면에 포함된 위젯 템플릿에도 확인·설정한다. 중첩 템플릿에 `None`이 남는 선례가 있다.
9. `BlueprintEditorLibrary.compile_blueprint` 후 `BS_UP_TO_DATE`와 오류·경고·ensure를 확인하고 대상만 개별 Save한다.
10. 복제에는 새 프로세스 Compile·Save로 GUID를 정리한 선례가 있다. 세션 정책에 맞춰 수행하고 다시 새 프로세스에서 읽는다.
11. `was deleted but still has a GUID` / `was added but did not get a GUID` ensure가 남으면 완료하지 않는다.
12. reparent 중 binding 경고는 최종 구조·Compile·reload가 정상인 경우에만 중간 경고로 분리 기록한다.

빈 WBP root 지정의 지원 여부를 단정하지 않는다. 검증된 복제 경로를 우선하며 원본 WBP를 수정·저장하지 않는다.
최종 검증은 parent, 필수 BindWidget 이름·타입·도달 가능성, switcher 순서, CDO·템플릿 연결과 승인 수치 대조다.
자동 구조 검증과 실제 PIE 화면·입력 검증은 별도로 기록한다.

## StateTree 조회·편집

- 조회는 `tree = unreal.load_asset(path)`, `data = unreal.StateTreeEditorData.get_editor_data(tree)`다. null·class·지원 API를 확인한다.
- `data.sub_trees`에서 `state.children/tasks/transitions`를 순회한다. schema·evaluator·condition·binding 필드는 실제 노출을 확인한다.
- State 경로, node type·ID, 필요한 Task 값, Transition 대상·조건, binding의 source/target을 질문에 맞춰 출력한다.
- `Saved/Codex/map_statetree.py` 등은 조회 선례다. 문자열 파싱이 현재 구조체 표현과 맞는지 확인하고 실패를 명시한다.
- binding의 조회·변경 지원을 각각 판정한다. 개수만으로 source/target·type 일치를 검증했다고 하지 않는다.
- 변경 전 계층·Transition·ID·binding 기준선을 수집하고 Compile·검증·Save까지 가능한지 확인한다.
- `copy()/node.import_text()/instance.import_text()` 선례는 버전·구조체 형식을 확인해 사용한다. 원시 asset 파일 편집이 아니다.
- 의미가 유지되는 노드 ID를 보존한다. Task type 교체 등 호환성이 달라지면 binding을 명시적으로 검증·이관한다.
- 예상 밖 계층·Transition·binding 변화나 Compile 실패가 있으면 저장하지 않는다.
- C++ helper는 실제 설치·노출·버전을 확인한다. 과거 `CodexAuthoring` 파일만으로 현재 사용 가능하다고 하지 않는다.

## 설정·Level·Validation

- DeveloperSettings CDO는 메모리 변경일 수 있다. 영구 저장 API·새 프로세스 reload가 확인되지 않으면 Config 반영으로 기록하지 않는다.
- Config가 필요하면 exact section·key·경로와 변경·검증을 구현 단계로 인계한다. Python으로 Source/Config 원인을 덮지 않는다.
- Level은 로드 범위·class default·instance override를 구분하고 allowlist의 정확한 map/external actor package만 저장한다.
- `EditorAssetLibrary.save_asset`을 external actor에 무조건 적용하지 않는다. 대표 대상의 저장·reload로 지원을 검증한다.
- Validation 선례는 `unreal.get_editor_subsystem(unreal.EditorValidatorSubsystem)`와 `validator.is_object_valid(asset, unreal.DataValidationUsecase.MANUAL)`다.
- result·errors·warnings를 판정한다. 상위 asset이 원인을 숨기면 하위 asset을 직접 확인하고 대상별 VALID 여부를 보고한다.

## 완료와 기록

1. 목표값·계약, 대상별 Compile·Validation·Save와 예상 밖 dirty를 확인한다.
2. 디스크 reload로 class·Parent·component·참조·Binding·수치·설정을 재검사한다.
3. 필수 fresh-process 검증은 세션 정책을 지키고 읽기 전용 스크립트로 확인한다.
4. PIE·입력·시각 수용의 실행 결과·미검증을 분리한다. 자동 PASS만으로 통합 완료하지 않는다.
5. 관련 Unreal 정본·`PROMPT_INTEGRATION_REVIEW.md`에 저장·reload 사실, 스크립트·로그·백업·미완료를 기록한다.
6. 입력 `PROMPT_UNREAL.md`는 임의로 고치지 않는다. 수정이 필요하면 소유 단계에 반환한다.
