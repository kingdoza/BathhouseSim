# UE 5.8 Build·Headless 실행 정책

C++ 빌드와 headless Editor 실행의 고정 명령과 옵션이다. 구현, 코드 리뷰, Unreal Editor 역할과 Editor Python 절차가 이 문서를 따른다. 단계·승인·결과물 규칙은 [AGENT_WORKFLOW.md](AGENT_WORKFLOW.md)를 따른다.

- 빌드 전에 같은 프로젝트의 Unreal Editor가 실행 중이면 빌드하지 않는다. Live Coding·모듈 DLL 잠금으로 실패한다.
- 빌드·Automation·commandlet이 10분을 넘길 수 있으면 백그라운드로 실행하고 완료를 확인한 뒤 결과를 읽는다.

## Build

BathhouseSim의 C++ build는 항상 다음 진입점을 사용한다.

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  BathhouseSimEditor Win64 Development `
  -Project='C:\UnrealProjects\BathhouseSim\BathhouseSim.uproject' `
  -WaitMutex `
  -NoHotReloadFromIDE
```

에이전트 shell에서는 UBT 자식 프로세스와 Engine/Uba 접근을 위해 첫 시도부터 필요한 권한으로 실행한다. 승인 실행이 불가능하면 시도하지 않고 차단 사유를 보고한다. system `dotnet`, MSBuild 직접 실행, `UnrealBuildTool.exe` 또는 `.dll` 직접 실행은 사용하지 않는다.

## Headless Editor 실행

Automation, Blueprint load gate 등 모든 headless Editor 실행은 다음 형식을 사용하며 `-DDC-ForceMemoryCache`를 항상 붙인다. Agent sandbox는 사용자 공용 DDC 경로(`%LOCALAPPDATA%\UnrealEngine\Common\DerivedDataCache`)에 쓸 수 없어, 이 옵션이 없으면 에셋 로드 전에 DDC 초기화 Fatal로 종료된다.

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  'C:\UnrealProjects\BathhouseSim\BathhouseSim.uproject' /Engine/Maps/Templates/Template_Default `
  -unattended -nullrhi -NoSplash -NoSound -DDC-ForceMemoryCache `
  -ExecCmds="Automation RunTests <TestFilter>; Quit" -TestExit="Automation Test Queue Empty" `
  -ReportExportPath='C:\UnrealProjects\BathhouseSim\Saved\Automation\Reports\<날짜>\<이름>' -log
```

- 맵 인자, test filter, 작업별 명령행 인자(예: load path)만 바꾼다. 사용자가 직접 여는 Editor에는 이 옵션을 쓰지 않는다.
- 에셋 로드 전 환경 Fatal은 검증 결과가 아니다. 명령을 고쳐 같은 단계부터 다시 실행하고, 반복되면 로그와 함께 보고한다.
