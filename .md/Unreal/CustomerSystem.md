# Customer Editor Authoring

## 정본 범위

이 문서는 Customer routine Data Asset과 StateTree의 Unreal Editor authoring 진입점이다. C++ 상태 owner와 Bath 시간 계약은 `.md/Architecture/CustomerSystem.md`, `.md/Architecture/BathWaterSystem.md`를 따른다.

## Customer Routine Data Asset

- Asset: `/Game/Bathhouse/Data/DA_CustomerRoutine_Default`
- Class: `/Script/BathhouseSim.CustomerRoutineDefinition`
- 저장·재로드 확인값:
  - `BathStayDurationSeconds = 60.0`
  - `BathSearchTimeoutSeconds = 10.0`
  - `BathDwellMinSeconds = 10.0`
  - `BathDwellMaxSeconds = 20.0`

다른 routine 값은 이번 authoring에서 변경하지 않았다. 저장 시 native validation이 실행됐고 오류가 없었으며 새 Editor 프로세스에서 네 값과 package clean 상태를 확인했다.

## Bath 전역 임계값

`/Script/BathhouseSim.BathWaterSettings`의 native 기본 `CustomerUsableThresholdPercent`는 `80.0`이다. 현재 `Config/DefaultGame.ini`에는 별도 override가 없으므로 Project Settings는 native 기본값을 사용한다.

## Customer StateTree

- Asset: `/Game/Bathhouse/AI/ST_CustomerRoutine`
- BathLoop migration 상태: 미적용

현재 Unreal MCP toolset에는 StateTree 내부 state/task/transition/binding read/write가 없어 이번 작업에서는 StateTree를 수정·저장하지 않았다. 필요한 정확한 BathLoop authoring과 Compile/재로드 절차는 `.md/USER_UNREAL.md`의 `ST_CustomerRoutine BathLoop migration`을 따른다.
