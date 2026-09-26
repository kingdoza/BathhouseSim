# 구현 재작업 프롬프트 — 컴퓨터 포커스: load gate·자동화 재실행

## 재검토 결론

- 2026-09-26 두 번째 코드 리뷰 결론: **구현 재검토(검증 미완)**. 입력은 `.md/PROMPT_REVIEW.md`(13:38 UTC)와 현재 작업 트리다.
- 이전 재작업 중 완료가 확인된 항목:
  - 백업 두 파일(SHA 기록, 현재 hash 일치).
  - UE 5.8 Editor 타깃 빌드 성공. 빌드로 발견한 `Engine/OverlapResult.h` include 누락을 수정했다.
  - P2 `FocusExitArrow`의 `BlueprintReadOnly` 제거. 재확인 결과 editor-only `UPROPERTY(VisibleAnywhere)`만 남았다.
- 남은 것: Blueprint load gate(`.md/PROMPT_IMPLEMENTATION.md` 7절)와 자동화(8절). 둘 다 실행되지 않았다.
- **Source는 수정하지 않는다.** 아래 명령만 다시 실행한다. 새 실패가 나올 때만 해당 원인을 최소 수정하고 보고한다.

## 원인 — 환경 Fatal, gate 결과 아님

- `Saved/Reimplementation/Computer/BlueprintLoad_Copy/BlueprintLoad_Copy.log`의 Fatal은 `DerivedDataBackends.cpp:813` "Installed cache graph에 writable node 없음"이다. 에셋을 하나도 로드하기 전, Editor 시작 단계에서 났다.
- 이 명령에는 `-DDC-ForceMemoryCache`가 없었다. 같은 날 성공한 쿨러 전체 회귀(`Saved/Logs/BathhouseSim_Full_Final3_20260926.log`)는 이 옵션을 사용했다.
- 7절의 즉시 중단 규칙은 **Blueprint 로드 결과**(`Serial size mismatch`, `Failed to load`, 로드 중 Fatal)에 적용된다. 에셋 로드 전 환경 Fatal은 gate 실패가 아니다. 명령을 고쳐 **같은 단계부터** 다시 실행한다.

## 실행

모든 headless 명령에 `-DDC-ForceMemoryCache`를 추가한다. 나머지 인자와 순서는 `.md/PROMPT_IMPLEMENTATION.md` 7·8절 그대로다. UnrealEditor는 모두 종료한 상태여야 한다.

1. **복사본 load:**
   - `BP_BathhouseComputer.uasset`을 `Content/Developers/MigrationCheck/BP_BathhouseComputer_LoadCheck.uasset`으로 복사한다.
   - Template 맵에서 `-BathhouseComputerLoadPath=/Game/Developers/MigrationCheck/BP_BathhouseComputer_LoadCheck`로 `BathhouseSim.Computer.BlueprintLoad`를 실행한다.
   - `-ReportExportPath`와 `-abslog`는 이전과 같은 `Saved/Reimplementation/Computer/...` 아래를 쓴다.
2. **복사본 정리:** 복사본과 폴더를 삭제하고 `git --no-optional-locks status`로 Content 무변경을 확인한다.
3. **원본 load:** Template 맵, 원본 경로.
4. **DefaultMap load:** 맵 인자 `/Game/Maps/DefaultMap`.
   - 컴퓨터 instance의 `FocusExitPoint`, `ManagedBathPlacementZone`, `ScreenWidget` class, blend 값을 확인한다.
   - instance가 World Partition 로드 범위 밖이면 경고로 기록하고 Editor 단계 첫 확인 항목으로 인계한다.
5. **무변경 확인:** Content·Config 무변경, 두 백업 파일 SHA와 현재 hash 일치.
6. **자동화:**
   - 집중: `BathhouseSim.Computer.*`(신규 `FocusExitPlacement`, `BlueprintLoad` 포함).
   - 전체: `Automation RunTests BathhouseSim`(Template 맵).
   - 총/성공/경고/실패/미실행 수와 실패 이름을 보고한다.
   - 기존 Placement 3건(`ActorReplacementFailureAtomicity`, `ActorReplacementTransaction`, `SettingsZoneLeaseAndCompatibility`)은 이름만 대조한다. 그 밖의 실패는 무관 판정하지 않는다.

1·3·4 중 에셋 로드 결과로 Fatal·`Serial size mismatch`·`Failed to load`가 나오면 즉시 멈추고 로그와 함께 보고한다. `-DDC-ForceMemoryCache`로도 에셋 로드 전 환경 Fatal이 반복되면 멈추고 그 로그를 보고한다.

## 결과물

- `.md/PROMPT_REVIEW.md` 갱신: 1~6 단계별 결과와 report·log 경로, 자동화 수치. 기존 CMP 추적표·백업·빌드 기록은 유지한다.
- `.md/PROMPT_UNREAL.md`: load gate 실제 결과 반영. 이전 리뷰의 P3 authoring 확인 항목(고정 위치 capsule이 벽·책상과 겹치지 않게)이 있는지 확인한다.
- Content·Config·Level 저장, Editor authoring, Architecture 수정 금지.
