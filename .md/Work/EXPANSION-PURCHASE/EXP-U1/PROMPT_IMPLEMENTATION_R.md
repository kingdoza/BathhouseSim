# PROMPT_IMPLEMENTATION_R — EXP-U1 코드 리뷰 1회차 재작업

- 작업 ID: `EXP-U1`
- 단계: 코드 리뷰
- 상태: 완료
- 출처: 코드 리뷰 1회차

## 0. 판정·실행 순서·범위

- 리뷰 대상: `git diff d0e231a b5f4c41`(브랜치 `work/EXP-U1`). 입력 `PROMPT_REVIEW.md`·`PROMPT_UNREAL.md`·`PROMPT_IMPLEMENTATION.md`·`QNA_ARCHITECTURE.md`·상위 `PROMPT_ARCHITECTURE.md` 모두 작업 ID 일치, 상태 `완료`.
- 빌드 식별값: `PROMPT_REVIEW.md` 7절과 같다(`git diff 8428512 b5f4c41 --diff-filter=M -- Source Config` SHA-256 `760d44e1…`, 신규 16파일 SHA-256 `59e5fc00…`). 재빌드하지 않았다. `Saved/Logs/exp_u1_build_final.log` 성공, `Saved/Logs/BathhouseSim.log` 163개 Success·실패 0 확인.
- 판정: **아키텍처 재검토(A1·A2, 국소) + 구현 재작업(I1~I4)**.
- 실행 순서: 마스터가 A1·A2(·A3)를 아키텍처로 복귀한다 → 아키텍처가 `PROMPT_IMPLEMENTATION.md` 15절(복귀 재설계)·`BuildingSystem.md`·`PlacementSystem.md`를 갱신한다 → 구현이 그 결정과 이 문서 I1~I4를 함께 반영한다 → 같은 리뷰어가 재검증한다. A 항목은 구현이 단독으로 고치지 않는다.
- 영향 시나리오: A1 EXP-008·009(배치 조준), A2 EXP-001·004(계단 통로 면), A3 0.5 검증, I1 0.5 검증(경고), I2 없음(파일 구조), I3 조정값 원본, I4 PIE 인계.
- 유지되는 완료 범위: 위 항목 밖의 형상 계산·shell 수명·조각 spawn·Nav 검사·위치 제안·Placement/Cleaning 문구·API 변경, Config 두 파일, 기존 테스트 2개 이름 정리. 재작업 범위 밖 파일을 바꾸지 않는다.

## A. 아키텍처 복귀 항목(구현 단독 수정 금지)

### A1 [중요] 공간 형상 hit가 배치 구역 hit로 취급된다

- 사실
  - `Private/Building/BathhouseSpaceShellComponent.cpp:39`: Floor·Wall·Ceiling·StairRamp·StairWall ISM이 `BathhousePlacementCollision::ZoneTraceChannel`을 Block하고 Owner는 공간 Actor(`AFacilityPlacementZoneActor` 자식)다.
  - `Private/Placement/PlayerFacilityPlacementValidation.cpp:57` `TracePlacementZone`은 `Cast<AFacilityPlacementZoneActor>(Hit.GetActor())`로 구역을 정한다. `Private/Placement/FacilityPlacementZoneActor.cpp:149-150` `MakeCandidateTransform`은 hit 점을 바닥면에 투영한다(`Local.Z = 0`).
- 결과
  - 천장을 조준하면 그 아래 바닥 점에 후보가 생기고 설치된다(이전에는 구역 없음).
  - 벽을 조준하면 `설치 가능한 구역을 바라보세요.` 대신 `설비 전체가 하나의 설치 구역 안에 있어야 합니다.`나 겹침 문구가 나온다.
  - 지하 계단 아래에서 구멍 너머 홀 천장을 조준하면(배치 trace 거리 안이면) 홀 구역 후보가 생긴다.
- 어긋난 계약: 상위 계약 4.2 "배치 격자 표시·스냅·회전·확정 등 다른 배치 규칙은 지금과 같다", `Architecture/PlacementSystem.md:168` "벽 너머 다른 공간의 구역은 조준되지 않고 `설치 가능한 구역을 바라보세요.`가 된다".
- 구현이 못 고치는 이유: `PROMPT_IMPLEMENTATION.md` 12절이 Placement 후보 계산 변경을 3절 표 밖에서 금지한다. 설계 표(공간 형상의 배치 trace 채널 Block)가 "형상 Owner = 구역 Actor"를 고려하지 않았다.
- 아키텍처가 정할 것: 구역 판정 기준(예: blocking hit component가 그 구역의 `ZoneBounds`일 때만 구역으로 인정, 또는 형상 component 소유 분리)과 3절 수정 파일 표. 자동화 기준에 실제 `TracePlacementZone` 경로(벽·천장·경사로 hit → 구역 없음, `ZoneBounds` hit → 구역)를 넣는다. 현재 `World.SpacePlacement`는 `ValidateWorldPlacement`만 직접 호출해 이 경로를 검사하지 않는다.

### A2 [중요] 계단 벽과 바닥·천장 판의 같은 방향 동일 평면 면(z-fighting)

- 근거: `Architecture/BuildingSystem.md` Geometry Rules는 판에서 구멍 `R`만 빼고(83행), 계단 벽은 `R` 바깥 두께 `t`, Z `[Zf(L), …]`로 둔다(86행). 구현(`Private/Building/BathhouseSpaceLayout.cpp:228`·`249` 판 빼기, `:360` 이하 계단 벽 4개)이 그대로 따랐다.
- 겹치는 면(같은 평면, 같은 방향, 다른 재질)
  1. 위쪽 끝 벽 윗면(Z = `Zf(U)`)과 홀 바닥 윗면: 계단 입구 앞 `t` × (`W` + 2`t`) 띠. 플레이어가 밟는 곳이다.
  2. 옆 벽·위쪽 끝 벽·아래쪽 끝 벽의 안쪽 면과 홀 바닥 판 구멍 가장자리 면(Z `[Zf(U)−s, Zf(U)]`), 작업공간 천장 판 구멍 가장자리 면(Z `[Zc(L), Zc(L)+s]`): 계단 통로 안에서 보이는 `s` 높이 띠.
  3. 아래쪽 끝 벽 아랫면(Z = `Zc(L)`)과 작업공간 천장 아랫면: 아래 출구 위 `t` 띠.
- 어긋난 계약: EXP-001 "겹쳐 깜빡이는 면이 없다". EXP-004 대표 경로에서 보인다. 재질이 Floor·Ceiling과 StairWall로 달라 깜빡임이 드러난다.
- 아키텍처가 정할 것: 판 구멍과 계단 벽 Z 범위 규칙(예: 판에서 계단 벽 발자국까지 빼기, 또는 계단 벽을 판 두께 구간에서 자르기)과 자동화 기준(계단 부품과 판 사이 같은 방향 동일 평면 겹침 없음). 충돌·보행 결과(경사로, 위 입구·아래 출구 열림, 구멍 막이)는 유지한다.

### A3 [낮음, A1·A2와 함께] 계단 통로 trace 범위의 정본 불일치

- 구현은 `Zf(U)` → `Zc(L)+s`(`BuildingSystem.md` Implementation Notes)인데 같은 문서 Validation 표 138행은 `Zc(L)+s`~`Zf(U)−s`다.
- 리뷰 판단(구현 보고 판단 요청 1): 위쪽 확장은 타당하다. 아래층 천장 판 두께 구간 `[Zc(L), Zc(L)+s]`도 구멍 안 보행 경로이므로 아래 끝을 `Zc(L)`까지 넓힐지 함께 정하고 표를 고친다. 정해지면 구현이 `Private/Building/BathhouseSpaceValidation.cpp` `ValidateWorld`의 `Bottom`을 맞춘다.

## I. 구현 재작업

### I1 [보통] 계단 재질 누락 경고 없음

- `ABathhouseSpaceActor::FillSnapshot`이 `bHasStepMaterial`·`bHasStairWallMaterial`을 채우지만 `CollectProblems`(`BathhouseSpaceValidation.cpp:206` 부근 `MaterialMissing`)가 쓰지 않는다. `PROMPT_UNREAL.md` 5절은 계단 재질도 "비우면 Data Validation 경고"라고 인계했다.
- 수정: 계단 항목의 `StepMaterial`·`StairWallMaterial`이 비면 `MaterialMissing` Warning(문구에 계단 번호와 어느 재질인지)을 낸다. `Validation.Rules`에 케이스를 더한다.
- shell은 `Stairs[0]` 재질만 쓴다(`BathhouseSpaceActor.cpp:134-135`). 계단 항목이 둘 이상이면 2번째 이후 재질이 무시됨을 Warning으로 알리거나 `Stairs` tooltip에 적는다(둘 중 하나, 동작 변경 없음).

### I2 [보통] `BathhouseSpaceValidation.cpp` 1001줄 분리(구현 보고 판단 요청 2)

- 규칙 검사·위치 제안·Nav/world 수집 세 책임이 한 파일에 있고 U2가 넓힘 검증을 더한다. CoreSystem의 400~500줄 경고선을 이미 두 배 넘었다.
- 수정: `FBathhouseSpaceValidation` 공개 API(헤더)는 유지하고 translation unit만 나눈다.
  - `BathhouseSpaceValidation.cpp`: 규칙 검사(`CollectProblems`, `ValidateLayout`)
  - 위치 제안 파일(예: `BathhouseSpacePositionSuggestion.cpp`): `SuggestTouchingLocation`, `ApplyMove`, 후보 수집·`MoveFixesProblem`·`AppendSuggestions`
  - world 파일(예: `BathhouseSpaceWorldValidation.cpp`): `GatherSnapshots`, `ReadInputs`, `ValidateNavigation`, `ValidateWorld`
  - 공유 helper(`AddProblem`, `EdgeCoordinate`, `AlongMin/Max`, `VolumeZ`, `IsUsable` 등)는 Private 내부 헤더로 옮긴다. 이름 있는 namespace를 쓰고, unity blob에서 다른 파일의 익명 namespace 이름과 겹치지 않게 한다(`PROMPT_REVIEW.md` 6절 충돌 사례).
- 동작 변경 없음. 기존 `BathhouseSim.Building.*` 테스트가 그대로 통과해야 한다. `BuildingSystem.md` Source Scope·Implementation Notes에 파일을 반영한다.

### I3 [낮음] Settings getter의 숨은 하한

- `Public/Building/BathhouseBuildingSettings.h:22-26`의 `FMath::Max(1.0f, …)`는 `PROMPT_IMPLEMENTATION.md` 11절 상수 예외 목록에 없는 대체값이다(ClampMin 메타로 Editor 입력은 이미 막힌다).
- 수정(하나 선택): 하한을 없애고 0 이하·비유한 Settings 값을 검증 Error로 알리거나, 하한을 유지하고 그 근거(0 나눗셈 방지용 엔진 의미 상수)를 `BuildingSystem.md` Implementation Notes의 예외로 적는다.

### I4 [낮음] `PROMPT_UNREAL.md` 보강

- 14절 EXP-004에 관찰 항목 추가: 계단 구멍 위에서 든 물건 놓기는 `손에 든 위치가 막혀 있어 물건을 내려놓을 수 없습니다.`로 거부될 수 있다(`StairKeepClear`가 WorldStatic으로 PhysicsBody를 막고 `UPlayerCarryComponent::IsHeldPoseClear`가 겹침으로 본다). 들고 오르내리기에는 영향이 없다. 사용자가 이 거부를 원하지 않으면 기능 명세 사안이다.
- A1·A2 결정 결과를 14절에 반영: EXP-001 "계단 통로 안·입구 앞·출구 위 면이 깜빡이지 않는다", EXP-008 "천장·벽을 조준하면 `설치 가능한 구역을 바라보세요.`".
- 5절 계단 재질 경고 서술을 I1 결과와 맞춘다.

## 재검증 조건

- UE_BUILD_POLICY 빌드 성공, `BathhouseSim` 전체 자동화 통과(현재 163개 + 추가분). 추가 테스트: A1 실제 `TracePlacementZone` 경로, A2 동일 평면 겹침 없음, A3 trace 범위(정해진 경우), I1 계단 재질 경고.
- `PROMPT_REVIEW.md` 갱신: 새 빌드 식별값, 변경 파일, 설계 대비 차이, 이 문서 항목별 처리 결과.
- 재검증은 이 리뷰어가 `b5f4c41` 이후 diff와 위 항목만 본다.

## 구현 보고의 리뷰 판단 요청에 대한 답

1. 계단 trace 범위 확장: 수용. 정본 표 정리와 아래 끝은 A3.
2. 검증 파일 분리: 분리한다(I2).
3. `PlayerFacilityPlacementComponent.h` 테스트 friend: 수용. 같은 클래스의 기존 테스트 friend 패턴과 같고 동작 변경이 없다.
4. 설치 거부 문구 3곳 통일: 수용. 세 경로 모두 `IsDefinitionAllowed` 거부(같은 상황)이고 계약 4.2 문구와 같다.
5. unity 충돌로 기존 테스트 2개 이름 정리: 수용. 테스트 전용 변경이다.
6. 벽·판 두께 기본값 Config: 수용. 설계가 구현에 맡겼다. Editor 단계의 `Zg` 조건(판 두께 < `Zg` ≤ 문턱 높이)과 멈춤 조건이 `PROMPT_UNREAL.md` 4절에 이미 있다.
