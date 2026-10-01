# PROMPT_REVIEW — EXP-U1 공간 건물

- 작업 ID: `EXP-U1`
- 단계: 구현
- 상태: 완료

## 1. 기능 계약과 현재 단계

- 상위 계약: [../PROMPT_ARCHITECTURE.md](../PROMPT_ARCHITECTURE.md) 4.1~4.10, 5절 [U1] EXP-001~015(대표 EXP-007·EXP-004). 결정 Q1~Q15, P1~P37, D1, S2 A, S3 A, 아키텍처 Q1 A, 통로 위치 제안 (가).
- 구현 입력: [PROMPT_IMPLEMENTATION.md](PROMPT_IMPLEMENTATION.md)(상태 완료), 구조 정본 [BuildingSystem.md](../../../Architecture/BuildingSystem.md).
- 현재 단계: 구현 완료, 코드 리뷰 대기. Content·Level은 건드리지 않았다(`git status`에 `Content/` 변경 없음). Editor 단계 입력은 [PROMPT_UNREAL.md](PROMPT_UNREAL.md).
- U2·U3(넓힘 목록 `ExpansionSteps`, 미리보기 횟수, 확장 탭·구입·가격·열쇠/한도·락커 판매)는 구현하지 않았다.

## 2. 시나리오 ID별 코드·테스트 연결

테스트는 모두 `BathhouseSim.Building.*`(파일 `Private/Tests/BathhouseBuildingAutomationTests.cpp`). 기대값은 fixture 입력(크기·두께, 테스트 입력 값)에서 계산하며 Settings·CMC는 같은 원본에서 읽는다. 값 자체를 고정한 것은 계약 문구 `이 공간에는 놓을 수 없는 설비입니다` 하나다(사용자 표시 계약).

| 시나리오 | 구현 경로 | 자동화 |
|---|---|---|
| EXP-001 벽·바닥·천장 닫힘, 밝음 | `FBathhouseSpaceLayout::BuildPlan`(바닥·천장·벽·조명), `UBathhouseSpaceShellComponent::Rebuild` | `Layout.SingleSpace`(Z 범위·바깥 직사각형·벽 4개 비겹침·벽 합집합=바깥 띠·조명 개수), `Layout.AdjacentOpenings`(맞닿은 바닥·천장 비겹침), `World.ShellChunksLifecycle`(ISM instance가 계획과 일치) |
| EXP-002 출입구 | 개구부 분할(옆 조각 + 인방) | `Layout.AdjacentOpenings`(구간 안 비어 있음, 인방, 옆 벽 유지, 서쪽 벽 3조각), `Validation.Rules`(홀 바깥 출입구 없음·막힘) |
| EXP-003 통로 | 연결 공간 벽 같은 world 구간 | `Layout.AdjacentOpenings`(두 벽 같은 구간·인방), `World.ShellChunksLifecycle`(이웃 공간 값 변경 뒤 구멍 갱신) |
| EXP-004 계단 왕복(대표) | 계단 구멍·경사로·판·벽·구멍 막이, 경사 상한 검증 | `Layout.Stairs`(위·아래 같은 구멍 R, 경사로 윗면 양 끝점, 계단 판 수, 벽 열림 구간, 구멍 막이 범위), `World.ShellChunksLifecycle`(경사로 BlockAll·Nav 비관련·숨김, 판 충돌 없음, 막이 QueryOnly), `World.SpacePlacement`(구멍 위·계단 벽 위 후보 Blocked), `Validation.Rules`(범위 밖·출입 자리·경사) |
| EXP-005 벽·천장 막힘 | 벽·천장 ISM `WorldStatic` BlockAll | `World.ShellChunksLifecycle`(Pawn·PhysicsBody·Visibility·배치 trace 채널 Block, Static) |
| EXP-006 배치·장식 정리 | Level 작업(Editor 단계) | 자동화 없음(PIE) |
| EXP-007 손님 전 루틴(대표) | 코드 변경 없음. 공간 Nav 형상(바닥 Nav 관련, 계단·천장 비관련)과 Nav 범위 검증 | `Validation.Navigation`(손님 공간 바닥·출입구 밖 지점이 범위 안, 지하는 범위 밖), `World.ShellChunksLifecycle`(Nav 관련 설정). 루틴 자체는 PIE |
| EXP-008, 009 공간별 허용 설비 | `ABathhouseSpaceActor`가 구역 base 상속, `IsDefinitionAllowed`, 문구 getter, `ValidateWorldPlacement`의 `AddIgnoredComponent(ZoneBounds)` | `World.SpacePlacement`(종류 태그 14개 × 공간 3개 허용 표, 문구 getter, 실제 `ValidateWorldPlacement` 성공·Blocked·OutsideZone, 공간 바닥 ISM 위 바닥 지지) |
| EXP-010 조각 | `FBathhouseCleaningChunkSpawner`, `SplitChunks`, `Cleaning` 두 구역에 `GetSpawnFloor`·`SetSpawnAreaHalfSizeXY` | `Layout.CleaningChunks`(축별 개수·균등 크기·전체 덮음), `World.ShellChunksLifecycle`(`Litter` 공간은 쓰레기 조각만, `Stain` 공간은 물 얼룩만, `None` 공간 0, 바닥 Z, 크기, EndPlay 파괴) |
| EXP-011, 012, 015 | 변경 없음 | 기존 회귀 |
| EXP-013 관리 탭 지도 | 공간 `ZoneBounds` extent·위치 = 안쪽 직사각형(컴퓨터는 기존 zone type 참조) | `World.ShellChunksLifecycle`(zone extent·중심 = 안쪽 직사각형) |
| EXP-014 대기 배회 구역 | 변경 없음 | 기존 회귀 |
| 검증 0.5 | `FBathhouseSpaceValidation`(`ValidateLayout`·`ValidateNavigation`·`ValidateWorld`), `IsDataValid`, BeginPlay 로그 | `Validation.Rules`(32개 규칙 케이스), `Validation.PositionSuggestions`, `Validation.Navigation` |
| 위치 제안 (가) | `SuggestTouchingLocation` + 후보 검증(`MoveFixesProblem`) | `Validation.PositionSuggestions`(떨어짐·조금 겹침·바닥 Z 차이·통로 없는 겹침, 기대값은 fixture 크기·벽 두께에서 계산, 적용 뒤 오류 0, 못 없애는 경우 제안 없음) |
| lifecycle | `Shell::Rebuild/ClearGenerated`, `FBathhouseSpaceEditorSync::RebuildNow` | `World.ShellChunksLifecycle`(두 번 재생성 뒤 component 수 불변, 동기화 반복 뒤 불변, 이웃 구멍 갱신) |

## 3. 변경 파일과 구현 요약

신규(`Source/BathhouseSim/`):

- `Public/Building/BathhouseSpaceTypes.h`: 공간 종류·벽 방향·조각 종류 enum, part enum, 재질·조명·개구부·계단 USTRUCT(tooltip 한국어, 수치 기본값은 0 = 설정 전 오류).
- `Public/Building/BathhouseBuildingSettings.h`, `Private/Building/BathhouseBuildingSettings.cpp`: Project Settings > Game > Bathhouse Building.
- `Public/Building/BathhouseSpaceActor.h`, `Private/Building/BathhouseSpaceActor.cpp`: 공간 Actor(`SpaceRoot` Static root, `ZoneBounds` 재부착, `Shell`), `OnConstruction`·`BeginPlay`·`EndPlay`·`IsDataValid`.
- `Public/Building/BathhouseSpaceShellComponent.h`, `Private/Building/BathhouseSpaceShellComponent.cpp`: part별 ISM·조명·편집용 조각 미리보기 소유와 재생성.
- `Private/Building/BathhouseSpaceLayout.h/.cpp`: snapshot, 순수 형상 계획, 직사각형 빼기, 조각·조명 분할.
- `Private/Building/BathhouseSpaceValidation.h/.cpp`: 규칙 검사, 위치 제안, Nav 검사, world 수집.
- `Private/Building/BathhouseSpaceEditorSync.h/.cpp`: WITH_EDITOR 지연 일괄 재생성.
- `Private/Building/BathhouseCleaningChunkSpawner.h/.cpp`: 조각 spawn.
- `Private/Tests/BathhouseBuildingAutomationTests.cpp`: 위 9개 테스트.

수정:

- `Public·Private/Placement/FacilityPlacementZoneActor.*`: `static FText GetDefinitionNotAllowedReason()`.
- `Private/Placement/PlayerFacilityPlacementValidation.cpp`: 거부 문구 getter, `AddIgnoredActor(&Zone)` → `AddIgnoredComponent(Zone.GetZoneBounds())`.
- `Private/Facility/BathhouseFacilityActor.cpp`, `Private/Facility/BathWaterUtilityFacilityActor.cpp`, `Private/Towel/TowelProcessingMachineActor.cpp`: 구역 거부 문구를 getter로.
- `Public·Private/Cleaning/LitterSpawnZoneActor.*`, `StainSpawnZoneActor.*`: `GetSpawnFloor()`, `SetSpawnAreaHalfSizeXY(FVector2D)`.
- `Public/Placement/PlayerFacilityPlacementComponent.h`: 테스트용 friend 한 줄(`FBathhouseSpacePlacementAutomationTest`). 배치 검증을 실제 함수로 호출하려는 것이며 동작 변경 없음.
- `Config/DefaultGameplayTags.ini`: `Facility.Type.*` 14개.
- `Config/DefaultGame.ini`: `[/Script/BathhouseSim.BathhouseBuildingSettings]`(벽·판 두께, 상자 mesh `/Engine/BasicShapes/Cube`, 조각 최대 크기, 두 구역 class). 두께 값은 구현이 정한 기본값이다.
- 기존 테스트 2개의 unity 빌드 충돌 정리(아래 6절): `Private/Tests/CleaningTowelAutomationTests.cpp`, `Private/Tests/ServiceFacilityEmptyBoxTakeAutomationTests.cpp`. 동작 변경 없음.
- Architecture 정본: `.md/Architecture/BuildingSystem.md`(구현 메모 절과 상태), `PlacementSystem.md`·`CleaningLitterSystem.md`·`.md/0_ARCHITECTURE.md`(상태 문구만).

## 4. 클래스 크기·책임 변화

- 신규 클래스라 변경 전 크기 없음. 줄 수: 공간 Actor cpp 269(헤더 99), shell cpp 235, layout cpp 400, validation cpp 1001, 테스트 cpp 1185.
- 공간 Actor는 조립·상위 flow·`IsDataValid` 호출만 한다. 계산은 순수 layout, 검증·제안은 validation, spawn은 chunk spawner, 편집 동기화는 EditorSync, 생성 component 수명은 shell이 맡는다(CoreSystem 공간 건물 항목과 일치).
- `BathhouseSpaceValidation.cpp`가 1001줄이다. 규칙 검사(약 350줄), 위치 제안 후보·검증(약 230줄), Nav·world 수집(약 250줄)이 한 파일에 있다. 설계에 이 파일의 분리 경계가 따로 없어 그대로 두었고 리뷰 판단 대상이다(분리하면 위치 제안을 별도 파일로 뺄 수 있다).
- 기존 클래스: Placement base·세 `QueryFacilityPlacement`·Cleaning 두 구역 class에는 설계 3절의 변경만 있다.

## 5. Blueprint·API·Core Redirect 영향

- 신규 reflected 계약은 BuildingSystem.md Blueprint/API Contracts 그대로다: `ABathhouseSpaceActor`(property 이름 `SpaceKind`, `FloorSizeCm`, `CeilingHeightCm`, `Surfaces`, `Lighting`, `Openings`, `Stairs`, `CleaningChunkKind`, component `SpaceRoot`·`Shell`), enum·struct, `UBathhouseBuildingSettings`. 기존 class·property 이름 변경·삭제가 없어 Core Redirect와 copy-first load gate가 필요 없다.
- 기존 Blueprint가 새로 요구하는 값은 없다. `BP_BathhouseSpace`는 Editor 단계가 만든다.
- 사용자 표시 문구가 바뀐다: 설치 거부 문구 세 곳(이전 `이 구역에는 해당 설비를 설치할 수 없습니다.`, `…기계…`)이 `이 공간에는 놓을 수 없는 설비입니다` 하나로 통일됐다.
- `AddIgnoredComponent` 전환으로 구역 Actor의 다른 충돌 component는 이제 배치 overlap·바닥 trace에 잡힌다. 기존 base zone은 `GridVisual`이 NoCollision이라 결과가 같고 `AFacilityPlacementZoneActor` base BP 테스트·기존 배치 자동화가 통과했다.

## 6. 빌드와 정적 검증

- 빌드: UE 5.8 `BathhouseSimEditor Win64 Development`(UE_BUILD_POLICY 명령) 성공. 빌드 시점 Source 식별값과 로그는 7절.
- unity 충돌 발견·수정: 새 파일 추가로 unity blob 구성이 바뀌어 기존 테스트 2개가 컴파일 실패했다(`CleaningTowelAutomationTests.cpp:837` 지역 변수가 다른 테스트 파일의 익명 namespace 상수 `StainRadiusFixtureCm`를 가려 C4459, `ServiceFacilityEmptyBoxTakeAutomationTests.cpp`의 `FFixture`가 `ServiceAmenityTest`·`ServiceFacilityTest`에서 모호). 지역 변수 이름을 바꾸고 `FFixture`를 `ServiceFacilityTest::FFixture`로 한정했다. 이 두 변경은 새 파일이 커밋돼 unity 구성이 다시 바뀌는 경우에도 안전하다(이름이 겹치지 않음). 커밋 뒤(작업 트리가 깨끗해 모든 파일이 unity에 들어가는 상태)에도 안전한지는 git 밖 복사본(`Source`·`Config`·uproject만 복사, 읽기 전용 파일로 unity 강제)에서 전체 unity 빌드(blob 9개)와 파일별 비unity 빌드(261개 컴파일)를 각각 돌려 둘 다 성공으로 확인했다(`Saved/Logs/exp_u1_build_cleanunity.log`, `…cleanunity3.log`; 복사본은 삭제). `-DisableAdaptiveUnity`·`-NoAdaptiveUnity` 인자는 UBT가 무시해 쓰지 않았다.
- `git diff --check`(Source·Config·.md): 공백 오류 없음(줄 끝 변환 경고만). focused `rg`: 옛 문구 `이 구역에는`·`AddIgnoredActor(&Zone` Source에 0건, Building 소스에 문자열 리터럴 수치 없음.
- 자동화(UnrealEditor-Cmd, 사용자 Editor 없음): `BathhouseSim.Building` 9개 통과 / 0 실패. 전체 `BathhouseSim` 필터 163개 통과 / 0 실패(회귀: Placement·Cleaning·Facility·Customer·Computer·BathWater 포함, 로그 `Saved/Logs/BathhouseSim.log`의 `Test Completed`).
- 수치 상수 점검: 11절 예외(절반 0.5, 축 부호, 축마다 최소 1개, 허용 오차 `UE_DOUBLE_KINDA_SMALL_NUMBER`, 3×3 trace 표본 비율 `{0, 0.5, 1}`)와 편집 미리보기 선 색상 외 동작 수치 상수 없음. C++ 초기값 중 Settings의 두께·조각 최대 크기는 설정이 없을 때의 예비값(Config가 정본)이고 USTRUCT·Actor property 기본값은 0이다.

## 7. 빌드 시점 Source 식별값

마지막 Source 변경 뒤 마지막 정규 빌드(UE_BUILD_POLICY 명령, 성공) 시점의 값이다. 이후 Source·Config 변경은 없다.

- HEAD: `84285122d09140b84ac40b4b22f60a107935cd8b`(단계 시작 CONTEXT 커밋 `8428512`)
- `git diff HEAD -- Source Config` 출력 SHA-256: `760d44e1103c37243ec9ff108074c01101c7fd946b8e6243ab10869f95f1db1b`
- 미추적 신규 파일은 위 diff에 들어가지 않는다. `Source`·`Config` 미추적 16개 파일(각 파일 `sha256sum` 줄을 경로순으로 이어 다시 SHA-256한 값): `59e5fc008888ccc3e1dd9a28c314705252307532ab51b28560611e715cfd94ac`
- 빌드 로그: `Saved/Logs/exp_u1_build_final.log`(마지막 정규 빌드, 성공), 소스 변경을 컴파일한 `Saved/Logs/exp_u1_build4.log`(성공). 첫 빌드 `exp_u1_build1.log`는 위 unity 충돌로 실패했고 수정 뒤 `exp_u1_build2.log`·`build3`이 성공했다.
- 자동화 로그: `Saved/Logs/exp_u1_test_building2.log`(`BathhouseSim.Building` 9개), `Saved/Logs/exp_u1_test_all.log`(전체 `BathhouseSim` 163개). 개별 `Test Completed` 줄은 같은 실행의 `Saved/Logs/BathhouseSim.log`에 있다(다음 실행이 덮어쓴다).

## 8. 리뷰 중점·전역 영향·미검증

리뷰 중점:

- 형상 계산이 다른 공간의 authored 값만 읽고 순서와 무관한가(`GatherSnapshots` 종류·이름 정렬, 계산은 snapshot만). 생성물이 `RF_Transient`이고 package를 dirty로 만들지 않는가.
- 바닥 ISM이 Static·WorldStatic이라 청소 Floor Rule을 통과하고, 벽이 Floor Rule clearance를 막는가(World.ShellChunksLifecycle이 설정을, 실제 조각 spawn은 PIE가 확인).
- `AddIgnoredActor(&Zone)` 제거가 기존 Zone 동작을 바꾸지 않는가.
- 생성 component의 `CreationMethod = UserConstructionScript` 선택이 Engine construction 재실행과 겹쳐 중복되지 않는가(자동화는 두 번 재생성 후 불변만 증명, 편집기에서 속성을 바꾸는 실제 construction 재실행은 미검증).
- 검증 파일 크기(4절).

설계 대비 차이(모두 BuildingSystem.md 구현 메모에 기록):

- 계단 통로 장애물 trace 범위를 설계의 `Zc(L)+s ~ Zf(U)−s`가 아니라 `Zc(L)+s ~ Zf(U)`(위층 바닥 윗면에서 아래로)로 했다. 지형이 판 두께 범위에 걸려도 잡히게 하기 위해서다. 그래도 지형이 판 아래에 있다면 아래 방향 trace가 위에서 지형 표면을 만난다.
- Nav 범위 검사는 `NavMeshBoundsVolume` fixture 대신 순수 함수가 bounds 상자를 받는 형태로 자동화했다(runtime에 brush 모델이 없는 volume은 bounds를 만들 수 없음). volume에서 bounds를 읽는 world 경로는 PIE·Editor 단계에서만 확인된다.
- `ValidateWorld`의 설비 소속 경고 owner는 소속이 없는 설비면 첫 공간(홀)이다.

미검증(실행하지 못했거나 PIE 대상):

- 편집 world 전체 흐름: `OnConstruction`·`RequestRebuild`(FTSTicker)·Undo·레벨 로드 순서(Editor 프로세스가 필요).
- 실제 `NavMeshBoundsVolume`에서의 `ValidateWorld`, 실제 지형(Landscape) 위 계단 통로 trace, Nav 재생성 결과, 조명 표시 밝기, 재질 모습.
- PIE: EXP-001~015 전부의 실제 플레이 확인. 관찰 항목은 [PROMPT_UNREAL.md](PROMPT_UNREAL.md) 마지막 절.
