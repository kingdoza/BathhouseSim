# Unreal MCP 연결과 세션 운영

## 목적과 경계

이 문서는 [AGENT_UNREAL_MCP.md](AGENT_UNREAL_MCP.md)의 연결·세션 운영 절차 정본이다. Content 작업 범위, 저장 allowlist와 사전 조사/Editor 작업 구분은 해당 에이전트 지침을 따른다.

Unreal MCP 서버는 Editor 프로세스 안에서 실행된다. 백그라운드 작업도 살아 있는 Editor가 필요하며, Editor를 종료하면 해당 연결도 끊긴다. 연결을 위해 보이는 Editor를 먼저 열었다가 닫는 단계를 기본 절차로 만들지 않는다.

## 1. 대상 Editor 선택

1. 프로젝트 루트, `.uproject`의 EngineAssociation, `AGENT_WORKFLOW.md`의 UE 버전과 `.codex/config.toml`의 MCP URL을 읽는다. 연결 시도만을 이유로 설정을 수정하지 않는다.
2. 실행 중인 `UnrealEditor`/`UnrealEditor-Cmd`의 PID·실행 파일·명령줄과 대상 포트의 소유 PID를 확인한다. 프로세스명만으로 프로젝트를 판단하지 않는다.
3. 다음 분기에 따라 같은 프로젝트의 Editor 한 개만 사용한다.

| 현재 상태 | 행동 |
|---|---|
| 해당 프로젝트의 사용자 Editor가 열려 있음 | 프로젝트·엔진이 일치하는 해당 Editor에 연결하고 그 세션에서 작업한다. 작업 후에도 유지한다. |
| 해당 프로젝트의 Editor가 없음 | 작업용 백그라운드 Editor 한 개를 직접 실행하고 같은 프로세스에 연결·작업한다. 작업 후 5절에 따라 종료한다. |
| 이번 작업에서 연 백그라운드 Editor가 이미 있음 | 기록한 PID와 명령줄을 재확인하고 재사용한다. |
| 프로젝트·소유권이 불명확하거나 중복 Editor/다른 포트 소유자가 있음 | 읽기 전용으로 소유 관계를 확인한다. 사용자 프로세스를 임의 종료하거나 Editor를 추가 실행하지 않는다. |

사용자 Editor의 MCP가 응답하지 않는다는 이유로 같은 프로젝트의 백그라운드 Editor를 동시에 추가하지 않는다. 사용자 Editor의 종료·재시작은 명시적 요청이 있을 때만 수행한다.

## 2. 백그라운드 실행과 시작 확인

현재 프로젝트의 실행 예시다. 실행 전에 경로·버전을 확인하고, 기존 대상 Editor가 없을 때만 사용한다.

```powershell
$editorExe = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$projectFile = 'C:\UnrealProjects\BathhouseSim\BathhouseSim.uproject'
$editorProcess = Start-Process -FilePath $editorExe `
    -ArgumentList @(('"{0}"' -f $projectFile), '-NoSplash', '-log') `
    -WindowStyle Hidden -PassThru
$editorProcess.Id
```

- PID·시작 시각·실행 경로·프로젝트 명령줄을 작업 소유 기록으로 남긴다. 숨김 실행을 headless commandlet 실행과 혼동하지 않는다.
- 실행 환경의 권한 정책을 따른다. 자식 프로세스나 Engine 접근에 승인이 필요하면 제공되는 승인 실행 경로를 사용한다. 권한 거부를 다른 실행 방식으로 우회하지 않는다.
- 실행 전 현재 인계의 알려진 시작 실패를 확인한다. 같은 실행 환경에서 AppData 로그·Engine 접근의 권한 실패가 확인돼 있고 조건이 그대로라면, 동일한 제한 실행을 반복하지 않고 필요한 권한의 승인 실행 경로를 사용한다. 샌드박스 밖 실행 승인과 Windows 관리자 실행을 구분하며, 모든 Editor를 관리자 권한으로 실행하는 규칙으로 일반화하지 않는다.
- `Saved/Logs/`의 이번 실행 로그, 프로세스 생존과 URL 포트 리스너를 확인한다. 오래된 로그를 현재 실행의 근거로 쓰지 않는다.
- 시작이 진행 중이면 짧게 나누어 대기한다. Turnkey/RunUAT, DDC, asset load 등 마지막 진행 지점을 확인하며, 포트가 없다는 이유로 즉시 새 Editor를 띄우지 않는다.
- 포트 개방만으로 성공을 선언하지 않는다. 다음 절에서 실제 MCP 응답을 확인한다.

## 3. MCP 연결과 성공 판정

현재 저장소 설정은 `http://127.0.0.1:8000/mcp`다. 매 작업에서 설정을 다시 읽고 실제 리스너가 선택한 Editor에 속하는지 대조한다.

1. 현재 대화에서 사용 가능한 Unreal MCP 도구를 먼저 검색한다. 노출돼 있으면 현재 Level 등 읽기 전용 조회를 호출한다.
2. 도구가 노출되지 않으면 서버 연결 실패로 단정하지 않는다. 설정·프로젝트 신뢰/서버 활성화 상태와 실제 서버 응답을 별도로 확인한다. `codex mcp list`는 클라이언트 설정 확인 자료이며 Editor 연결 성공의 증거가 아니다.
3. 현재 대화의 도구 미노출에 한해, 실행·네트워크·서버 정책이 허용하면 설정된 로컬 URL에 정식 MCP 프로토콜로 접근할 수 있다. 명시적으로 비활성화되거나 거부된 서버·도구·권한을 우회하는 수단으로 사용하지 않는다.
4. 직접 MCP 접근은 `initialize` → `notifications/initialized` → `tools/list` → 읽기 전용 `tools/call` 순서로 수행한다. 서버가 반환한 protocol version과 세션을 사용한다. Editor 재시작 뒤에는 새로 초기화한다.
5. `tools/list`가 meta-tool만 반환하면 그 schema에 따라 `list_toolsets` → 필요한 `describe_toolset` → `call_tool`을 사용한다. `tools/list`의 메타 도구 수와 `list_toolsets`의 Editor toolset 수를 비교하지 않는다. 메타 도구 세 개는 정상일 수 있고, 과거의 toolset 19개도 고정 성공 기준이 아니다. 실제 조회 응답과 작업에 필요한 도구로 판정한다.
6. 반환된 schema를 읽고 현재 Level/asset 등 읽기 전용 조회 하나를 성공시킨다. HTTP 200이어도 JSON-RPC error, MCP `isError` 또는 내부 tool 오류면 성공이 아니다.

직접 Streamable HTTP 호출 시 확인 사항:

- POST Content-Type은 `application/json`, Accept는 `application/json, text/event-stream`이다. 응답 Content-Type에 맞춰 JSON 또는 SSE를 처리한다. 파싱하지 못한 응답을 성공으로 기록하지 않는다.
- 서버가 `Mcp-Session-Id`를 반환하면 후속 요청에 같은 값을 넣는다. PowerShell의 헤더 배열 전체를 문자열로 바꿔 `System.String[]`를 보내지 않는다. 예: `[string]($response.Headers['Mcp-Session-Id'] | Select-Object -First 1)`.
- 후속 요청의 `MCP-Protocol-Version`은 협상된 값으로 전달한다. 요청 ID를 구분하고 오류·시간 초과를 확인한다.
- 도구 인자용 PowerShell 변수명으로 자동 변수 `$args`를 사용하지 않는다. `$toolArguments`처럼 별도 이름을 사용한다.
- 실제로 열거된 MCP 도구만 호출한다. 임의 REST endpoint, Python reflection 또는 binary asset 편집으로 기능을 늘리지 않는다.

연결 성공은 **대상 프로젝트·엔진·서버 소유 PID가 일치하고 실제 Unreal 읽기 도구 호출이 성공한 상태**다. 연결 성공과 이번 작업에 필요한 편집·저장 기능 지원 여부는 각각 보고한다. 직접 MCP 호출로 작업했다면 Codex의 기본 도구 목록에도 노출됐다고 보고하지 않는다.

## 4. 실패 지점에 따른 대응

| 관찰 결과 | 확인 및 다음 행동 |
|---|---|
| 프로세스가 없거나 시작 직후 종료 | 이번 실행 로그와 실행 권한·프로젝트·엔진 경로를 확인한다. |
| AppData 로그 쓰기 등에 `UnauthorizedAccessException` 발생 | 명시된 접근 거부를 실행 권한 문제로 분류하고 2절을 따른다. Turnkey 정지나 종료 코드만으로 같은 원인을 단정하거나 MCP URL을 바꾸지 않는다. |
| 프로세스는 있으나 포트가 없음 | 시작 로그의 진행/정지 지점과 MCP plugin/server 시작 기록을 확인한다. Editor 중복 실행으로 해결하지 않는다. |
| 포트는 있으나 초기화/도구 호출이 시간 초과 | 시작 완료·리스너 소유 PID·최근 로그와 Python `init_unreal.py`/Editor toolset 등록 진행을 확인한다. 어느 요청이 시간 초과했는지 구분하고 마지막 로그 시각을 남긴다. 등록 로그 부재와 정지만으로 근본 원인을 확정하지 않는다. |
| 서버 조회는 성공하나 현재 대화에 도구가 없음 | 클라이언트 도구 노출 문제로 분류한다. 허용된 직접 MCP 경로가 동작하면 작업을 이어간다. 필요 시 클라이언트 갱신을 안내하되 새 대화/재시작이 반드시 해결한다고 단정하지 않는다. |
| 연결은 되나 필요한 도구가 없음 | `describe_toolset`으로 기능 부족을 확인하고 해당 Editor 작업만 `USER_UNREAL.md`에 남긴다. |
| Restore Packages 등 UI 조치가 필요함 | MCP로 지원되는 조작이 없으면 사용자에게 창 처리를 요청한다. 자동 복구·autosave 삭제·Computer Use 전환을 하지 않는다. |
| 저장 sharing violation | 저장 재시도를 멈추고 중복 Editor와 파일을 사용하는 프로세스의 소유권을 확인한다. |

동일 실패는 원인 확인을 포함해 두 번까지만 시도한다. 진행 중인 시작을 관찰하는 것과 실패 요청을 반복하는 것을 구분한다. 두 번은 허용 상한이며 무조건 재시도하라는 뜻이 아니다. 재시도에는 달라진 조건과 근거가 있어야 하며, 새 대화나 새 PID만으로 기존 실패 기록을 무시하지 않는다. 로그 정지만으로 복구 창이나 특정 engine 오류를 확정하지 않는다.

## 5. 작업 종료와 재시작

- 사용자 Editor는 유지한다. 작업이 시작한 PIE만 종료하고, 허용된 저장과 dirty 기준선 비교 결과를 보고한다. 기존 사용자 PIE는 임의 중지하지 않는다.
- 작업용 백그라운드 Editor는 이번 작업의 PIE 종료 → allowlist 개별 Save → 필요한 재로드 → dirty 기준선 비교를 마친 뒤 정상 종료하고 PID 소멸을 확인한다. 사전 조사 모드는 Save하지 않는다.
- MCP 작업 종료 때 에이전트가 시작한 백그라운드 Editor를 남기지 않는다. allowlist 저장을 시도하고, MCP 제한으로 저장할 수 없는 작업용 메모리 상태는 손실 여부를 기록한 뒤 종료한다.
- `Save All`로 dirty를 일괄 해제하지 않는다. Map의 dirty=false만으로 모든 World Partition 외부 패키지가 저장됐다고 판단하지 않는다.
- 정상 종료를 우선한다. 정상 종료가 불가능하면 에이전트가 시작한 것으로 확인된 정확한 PID만 종료하고 소멸을 확인해 보고한다. 작업용 Editor의 미저장 상태가 남으면 종료 전 그 상태와 손실 가능성을 기록한다.
- 소유권이 불명확하거나 사용자 소유 Editor인 경우 종료하지 않는다. 소유권을 확인하고, 다시 실행해 중복 Editor를 만들지 않는다.
- 재시작이 필요한 검증은 기존 작업용 Editor 종료와 PID 소멸을 확인한 뒤 새 프로세스로 수행한다. 메모리 readback, 같은 세션 Level reload, 새 프로세스 asset reload를 서로 구분해 기록한다.
- 종료 후에는 연결이 현재도 유지된다고 보고하지 않는다. 최종 보고에는 연결 성공 여부, 수행한 실제 조회, 남은 한계, Editor 유지/종료 상태를 포함한다.

## 설정 참고

Codex는 사용자 설정과 신뢰된 프로젝트의 `.codex/config.toml`에서 MCP 서버를 설정할 수 있다. 설정 변경 자체를 현재 대화의 도구 노출 성공으로 간주하지 않는다. 설정·신뢰·활성화 정책을 임의 변경하지 않는다. 근거: [OpenAI 공식 MCP 문서](https://learn.chatgpt.com/docs/extend/mcp?surface=cli).
