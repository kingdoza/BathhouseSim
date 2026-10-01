# Unreal MCP 연결과 공통 Editor 세션 운영

## 상태와 책임

[AGENT_UNREAL_EDITOR.md](AGENT_UNREAL_EDITOR.md)의 연결·세션 운영 절차다. asset 승인·도구 선택·결과물은 Unreal Editor 역할 정본을 따른다.
공통 세션 정책은 MCP, Python, 기존 C++ helper와 headless 검증에 모두 적용한다. 별도 세션 문서를 필수로 추가하지 않는다.
Python 절차는 [UNREAL_PYTHON_API.md](UNREAL_PYTHON_API.md), 버전·빌드·headless 기본 옵션은 [AGENT_WORKFLOW.md](AGENT_WORKFLOW.md)를 따른다.

## 1. 대상과 소유권 확인

1. 프로젝트 절대 경로, `.uproject` EngineAssociation과 공통 workflow의 엔진 경로를 확인한다.
2. `UnrealEditor/UnrealEditor-Cmd`의 PID·실행 파일·명령줄·시작 시각을 읽는다. 이름만으로 프로젝트를 판단하지 않는다.
3. MCP 사용 시 현재 설정의 URL·포트 소유 PID를 확인한다. 연결 시도만으로 설정·신뢰·서버 활성화를 수정하지 않는다.
4. 사용자 세션과 이번 작업의 세션을 구분하고 PID·프로젝트·모드·소유권·로그 경로를 기록한다.
5. 사용자 PIE/SIE 실행 여부, Live Coding, modal·복구 창, 대상 asset·기존 dirty·startup 오류를 확인한다. 에이전트는 PIE를 시작하지 않는다.

| 현재 상태 | 행동 |
|---|---|
| 같은 프로젝트의 사용자 Editor가 열려 있음 | 그 세션을 우선; MCP·등록된 Python 실행 경로 확인; 작업 후 유지 |
| 같은 프로젝트의 작업용 Editor가 이미 있음 | PID·명령줄·소유권 확인 후 재사용 |
| 대상 Editor 없음; live world·MCP 필요 | 작업용 숨김 Editor 한 개 실행 |
| 대상 Editor 없음; headless 작업 지원 | commandlet 한 개 실행 후 종료 결과 확인 |
| 소유권 불명확, 중복 실행, 다른 포트 소유자 | 읽기 전용 확인; 추가 실행·사용자 프로세스 종료 금지 |

- 사용자 Editor에 실행 경로가 없다는 이유로 같은 프로젝트를 다른 Editor/commandlet에서 동시에 열지 않는다.
- 사용자 Editor의 종료·재시작은 명시적인 해당 작업 권한이 있을 때만 한다. 사용자 PIE는 종료하지 않는다.
- Python 조회가 가능하면 MCP 연결 성공을 불필요한 선행 조건으로 요구하지 않는다. 공통 소유권·기준선은 항상 확인한다.
- 세션 소유권, asset 승인과 sandbox 실행 권한은 별개의 조건이다. 하나를 다른 권한으로 간주하지 않는다.

## 2. 작업용 live Editor 시작

기존 대상 프로세스가 없고 live Editor가 필요한 경우에만 실행한다. 경로는 현재 프로젝트·버전을 확인한 뒤 사용한다.

```powershell
$editorExe = 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe'
$projectFile = 'C:/UnrealProjects/BathhouseSim/BathhouseSim.uproject'
$editorProcess = Start-Process -FilePath $editorExe `
    -ArgumentList @(('"{0}"' -f $projectFile), '-ModelContextProtocolStartServer', '-NoSplash', '-log') `
    -WindowStyle Hidden -PassThru
$editorProcess.Id
```

- `-ModelContextProtocolStartServer`를 빼면 Editor 개인 설정의 자동 시작이 꺼져 있을 때 MCP 포트가 열리지 않으며 로그에도 드러나지 않는다. 포트는 보통 시작 후 20초 안팎에 열린다. 60초 안에 열리지 않으면 계속 기다리지 말고 `Saved/Logs/BathhouseSim.log`를 읽는다.
- 숨김 Editor와 headless commandlet는 다르다. `-nullrhi` 검증을 화면·렌더링·실제 입력 검증으로 기록하지 않는다.
- 이번 PID·시작 시각·명령줄을 기록하고 해당 실행의 로그·생존·리스너를 확인한다.
- 자식 프로세스·Engine·AppData 접근에 권한이 필요하면 정식 승인 실행 경로를 쓴다. 동일 제한 실행을 반복하지 않는다.
- sandbox 밖 승인과 Windows 관리자 권한을 구분한다. 항상 관리자 실행하는 정책을 만들지 않는다.
- 시작 중에는 짧게 나누어 진행을 확인한다. Turnkey/UAT, DDC, asset load의 마지막 로그 지점을 기록한다.
- 포트 부재나 일시적인 로그 정지를 이유로 다른 Editor를 추가하지 않는다.
- 시작 중 자동 compile·dirty를 기준선에 구분 기록한다. 조사 모드에서는 해당 package를 저장하지 않는다.

## 3. MCP 연결

기존 문서의 URL 예시는 `http://127.0.0.1:8000/mcp`다. 현재 설정을 읽고 선택한 Editor가 리스너 소유자인지 대조한다.

1. 대화에 노출된 Unreal MCP 도구를 검색하고 있으면 Level·asset 등 읽기 전용 조회를 수행한다.
2. 도구 미노출과 서버 연결 실패를 구분한다. 설정·신뢰·활성화와 서버 실제 응답을 별도로 확인한다.
3. 도구 미노출에 한해 네트워크·실행·서버 정책이 허용하면 설정 URL에 정식 MCP 프로토콜로 접근할 수 있다.
4. 직접 접근은 `initialize` → `notifications/initialized` → `tools/list` → 읽기 전용 `tools/call` 순서다.
5. 반환된 protocol version·session을 사용한다. Editor 재시작 뒤에는 이전 session을 재사용하지 않는다.
6. meta-tool만 있으면 실제 schema의 `list_toolsets` → 필요한 `describe_toolset` → `call_tool` 경로를 사용한다.
7. 필요한 도구·schema를 읽고 안전한 조회 응답을 확인한다. 도구 수를 고정 성공 기준으로 쓰지 않는다.

### Streamable HTTP

- POST `Content-Type`은 `application/json`, `Accept`는 `application/json, text/event-stream`이다.
- 응답 Content-Type에 맞춰 JSON/SSE를 파싱하고 JSON-RPC error·MCP `isError`·내부 Editor 오류를 확인한다.
- 반환된 `Mcp-Session-Id`를 후속 요청에 전달한다. 헤더 배열을 `System.String[]` 문자열로 변환하지 않는다.
- PowerShell 예: `[string]($response.Headers['Mcp-Session-Id'] | Select-Object -First 1)`.
- 후속 `MCP-Protocol-Version`은 협상된 값으로 보낸다. 요청 ID·timeout·실패 응답을 구분한다.
- 인자 변수는 `$toolArguments`처럼 짓고 자동 변수 `$args`·`$HOME`·`$CODEX_HOME`을 재정의하지 않는다.
- 현재 열거·설명된 도구만 호출한다. 없는 endpoint·도구명을 추측하지 않는다.
- Python 실행 도구가 실제 제공되면 그 schema로 호출하고 Python 절차를 적용한다. 미등록 기능은 별도 허용 Python 경로에서 판단한다.

연결 성공은 프로젝트·엔진·리스너 소유 PID가 맞고 실제 읽기 호출이 성공한 상태다.
HTTP 200, 포트 개방, `codex mcp list`·클라이언트 설정만으로 Editor 연결 성공을 선언하지 않는다.
직접 MCP 작업을 대화 기본 도구 목록 노출로 기록하지 않는다. 연결 성공과 필요한 편집 기능 지원은 별도로 판정한다.

## 4. 기능 확인과 도구 전환

- 최초 호출 전 asset 유형·Parent·property·object/class 참조·경로 형식·인자 schema를 확인한다.
- 재연결·버전·도구 변경 시 재확인한다. 필요한 toolset만 읽고 확인한 schema를 불필요하게 반복 출력하지 않는다.
- 인자 오류는 요청·schema·응답을 대조해 정정한다. 올바른 호출의 내부 실패와 구분한다.
- MCP 기능 부족이면 같은 에이전트가 Python·검증된 helper의 조회·편집·저장·검증 경로를 확인한다.
- 전환 전 메모리 변경·dirty·저장 상태를 확인한다. 작업을 중복 적용하거나 외부 변경을 덮어쓰지 않는다.
- 기능 부족과 권한·승인 거부를 구분한다. 거부된 작업은 직접 HTTP·다른 실행·다른 캡처 경로로 우회하지 않는다.
- Python 선택은 플러그인 설치·설정 변경이나 화면 조작의 자동 승인이 아니다.

## 5. 실패 지점별 대응

| 관찰 결과 | 대응 |
|---|---|
| 프로세스 부재·시작 직후 종료 | 이번 로그와 경로·플러그인·권한 확인 |
| AppData·Engine 접근 거부 | 정식 권한 경로 확인; network·tool 부재와 구분 |
| 프로세스 생존, 포트 없음 | 시작·서버 등록 로그 확인; 추가 Editor 실행 금지 |
| 포트 존재, 초기화·조회 timeout | 소유권·시작 완료·이번 로그 확인; 실패 요청 단계 기록 |
| 조회 성공, 대화 도구 미노출 | 허용된 직접 MCP 재사용; 클라이언트 재시작 성공을 보장하지 않음 |
| 연결 성공, 필요 도구 없음 | Python·기존 helper capability 확인 후 남은 항목만 인계 |
| 모달·Restore Packages 등 UI 필요 | 제공 API·명시적으로 허용된 native 경로 확인; 불가하면 조작 인계 |
| 저장 sharing violation | 반복 Save 중단; 중복 프로세스·파일 소유권 확인 |
| startup Fatal·DDC 실패 | 공통 옵션·권한 정정; asset 검증 결과로 기록하지 않음 |

동일 원인의 재시도 제한은 통합 역할을 따른다. 시작 진행 관찰을 실패 재호출과 혼동하지 않는다.
조건 변화와 근거가 있어야 재시도한다. 새 대화·새 PID·다른 도구명만으로 과거 실패를 무시하지 않는다.
복구 창·engine 결함을 로그 정지만으로 확정하지 않고 비활성화된 서버·도구·권한을 우회하지 않는다.

## 6. 새 프로세스·headless 검증

- 모든 headless Editor 실행은 `AGENT_WORKFLOW.md`의 `-DDC-ForceMemoryCache` 등 공통 정책을 적용한다.
- Python commandlet는 Python 절차를, 기존 Automation·UAT는 실제 task·test filter·출력을 확인한다.
- 종료 코드·이번 로그·검증 결과를 함께 확인한다. 오래된 PASS를 이번 실행에 사용하지 않는다.
- 새 프로세스 전에는 기존 작업용 Editor를 안전하게 종료하고 PID 소멸을 확인한다.
- 사용자 Editor가 열려 있고 종료 권한이 없으면 fresh-process 검증을 미완료로 기록한다. 임의 종료·중복 실행하지 않는다.
- 메모리 readback, 같은 세션 asset/Level reload와 새 프로세스 디스크 reload를 결과물에서 구분한다.
- commandlet의 world·World Partition 로드 범위는 live와 다를 수 있다. 미조회 actor를 삭제·부재로 판단하지 않는다.

## 7. 종료와 상태 보존

- 사용자 Editor는 유지한다. 에이전트는 PIE를 시작하지 않으며 사용자 PIE를 종료하지 않는다.
- 작업용 Editor는 allowlist 개별 Save → 필요한 reload → dirty 기준선 비교 후 정상 종료하고 PID 소멸을 확인한다.
- 조사 모드는 Save하지 않는다. 임시 객체 정리는 이번 작업이 만든 대상으로만 제한한다.
- `Save All`로 dirty를 해제하지 않는다. Map dirty=false는 external actor package의 clean·저장을 증명하지 않는다.
- 저장 실패 시 exact package·메모리/디스크 상태·미저장 작업·예상 손실·복구 조건을 종료 전에 기록한다.
- 작업용 미저장 상태를 종료하며 잃었다면 손실을 명시하고 완료하지 않는다. 사용자 변경·소유권이 불명확하면 임의 종료하지 않는다.
- 정상 종료 불가 시 소유권·프로젝트·시작 시각을 재확인한 작업용 exact PID만 정책이 허용하는 경로로 정리한다.
- 종료·재시작 후 포트·PID 상태를 확인한다. 종료된 연결을 현재도 유지되는 것으로 보고하지 않는다.
- 최종 보고에는 조회·실행 경로, 저장·reload 결과, 미검증·dirty·잔여 프로세스와 Editor 유지/종료 상태를 적는다.
