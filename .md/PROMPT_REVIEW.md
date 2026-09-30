# 코드 리뷰 프롬프트 — 서비스 3단위: 생성 자리 clearance 재작업

## 단계와 입력

- **수직 구현(단위 확장), C++ 재작업 완료, pre-Editor 재리뷰 요청**. 현재 작업은 `.md/PROMPT_IMPLEMENTATION.md` 맨 위 2026-10-01 재작업 절과 `.md/PROMPT_IMPLEMENTATION_R.md` F1~F3다.
- `.md/AGENT_WORKFLOW.md` → `.md/AGENT_REVIEW.md`를 읽는다. 기능 계약은 `.md/PROMPT_ARCHITECTURE.md` TRSH-001~005·018·019·029·030과 대표 시나리오다. 사용자 결정은 `.md/QNA_FEATURE_SPEC.md` 2026-10-01 정정이다.
- 설계 정본: `.md/Architecture/CleaningLitterSystem.md` **Floor Rule**. 이 절과 구현 프롬프트가 일치함을 확인했다. 관련 CleaningSystem·CoreSystem 및 `.md/0_ARCHITECTURE.md`를 참조한다.
- 기존 미커밋 3단위 Source를 유지했다. 완료된 본 구현 절을 다시 수행하지 않았다. 기존 서비스 1/2단위와 사용자 소유 문서 변경도 보존했다.
- 구현 단계는 Source와 두 인계 문서만 변경했다. Architecture·Unreal 정본·입력 프롬프트·QNA·Config는 수정하지 않았다. commit/push 없음.

## 선택된 판정 기준과 변경

- F1: `FCleaningFloorSpawnQuery::Find` clearance를 `OverlapMultiByChannel(ECC_PhysicsBody)`로 바꿨다. query response는 모두 Ignore에서 WorldStatic·WorldDynamic·PhysicsBody만 Block이다.
- `bBlockingHit && IsValid(GetComponent())`인 결과만 막힘으로 센다. QueryOnly 여부·object type만으로 막지 않는다. 축 정렬 box의 반폭 R, hit 위 1cm부터 ClearanceHeight까지의 범위는 유지했다.
- F2: trace가 맞힌 **component**를 `AddIgnoredComponent`로 제외한다. Actor 전체를 제외하지 않으며 다른 mesh의 턱·벽은 계속 막는다.
- 제외 전 바닥 guard: floor height ±tolerance, 시설/휴대물 interface 기각에 object type `WorldStatic`·Mobility `Static`을 추가했다. 낮은 수건·movable 판의 윗면을 바닥으로 인정하지 않는다.
- trace의 물 얼룩·쓰레기 통과와 overlap의 두 종류 Actor 무시는 유지했다. 바닥 tag·XY·경사·같은 종류 간격 검사도 유지했다.
- TRSH-029: Pawn clearance sphere·private settings 값·zone→director 인자를 제거했다. clearance 결과에서 APawn 소유 component를 무시하므로 서 있는 capsule과 넘어진 손님 PhysicsBody가 생성을 막지 않는다.
- TRSH-030: 벽은 충돌 응답을 그대로 따라 막는다. 종류별 반경 R 안 후보를 기각하고 R 밖은 허용한다.
- F3: 집게 `TakeTongs`·`TongsNotHeldForDrop`, 봉투 `InvalidBagActivation`·`BagPhysicsFailed`·`BagNotDiscardable`·`InvalidBagAuthoring`로 LOCTEXT 키만 바꿨다. 문구와 동작은 유지했다.
- 발생 clock·손님 수 수집·RMB routing·HUD 합성·집게/봉투 lifecycle·버리기·수거·배치 이벤트와 Placement collision helper는 재작업 시작 Source와 동일하다. 집게/봉투는 키 치환 및 공백을 정규화해 비교했다.
- 설비/휴대물/Customer collision profile, 모듈 의존성과 범용 helper API는 바꾸지 않았다. 테스트 fixture만 profile과 Mobility를 설정한다.

## Blueprint/API와 미검증

- 승인된 reflected 삭제는 정확히 3개다: director `DefaultPawnClearance`, stain zone `PawnClearanceOverride`, litter zone `PawnClearanceOverride`.
- 다른 reflected 추가/삭제·기존 class/component 이름·native parent·enum 순서·Serialize·Core Redirect 변경은 없다. native `FindSpawnTransform`의 Pawn 인자는 제거했다.
- reflection 자동화에서 세 property 부재를 확인했다. 기존 `SpawnIntervalSeconds` 값과 zone bounds·tag·trace·최대 값은 보존한다.
- Blueprint Compile/Save·DefaultMap authoring·PIE는 이번 단계에서 실행하지 않았다. 삭제 property의 기존 BP/레벨 resave와 바닥 WorldStatic/Static 확인은 `.md/PROMPT_UNREAL.md`로 인계했다.

## 시나리오와 fixture 추적

신규 4개는 `Private/Tests/CleaningSpawnClearanceAutomationTests.cpp`에 둔다. 모두 실제 stain/litter zone `FindSpawnTransform` 경로를 사용한다.

| 시나리오 / 리뷰 | 자동화 | 검증 |
|---|---|---|
| F1, TRSH-001/018/019, 대표 생성 | `Cleaning.Clearance.RegionsAndDirector` | 실제 PlacementZone 위치 `(600,-100,0)`·extent `(1400,900,10)` 안의 겹친 두 spawn zone; Visibility-only 공간과 nonblocking overlap 허용; 실제 director AdvanceSpawnScheduleForTesting으로 두 종류 Actor 생성 |
| F2, TRSH-004 | `Cleaning.Clearance.SlopeAndStep` | 20° static mesh 바닥(±5cm 안 후보), 더 낮은 slope limit 기각, 하나의 static mesh component에 두 slab로 만든 1cm 단차 성공, 별도 mesh 턱 기각 |
| TRSH-004/030 | `Cleaning.Clearance.PhysicalObstaclesAndWall` | native BlockAllDynamic 사용한 수건(높이 3cm) 윗면/옆·Movable 3cm 판·WorldStatic이지만 Movable인 판·PhysicsActor 열쇠/설비 아이템·품목 박스·시설 윗면/옆 기각; 벽 R 안 기각/R 밖 성공; 낮은 욕탕 바닥 기각 |
| TRSH-029 | `Cleaning.Clearance.PawnsAndRagdoll` | 실제 FirstPersonCharacter·Customer capsule 발밑/옆 성공; SKM_Manny_Simple+PA_Mannequin의 실제 ragdoll skeletal mesh가 PhysicsBody query에 blocking overlap으로 잡힘을 확인한 뒤 두 zone 생성 성공 |
| TRSH-005, 기존 바닥 계약 | `Cleaning.Litter.FloorAndVariation` | tag·height·경사·간격·시설/박스/벽 기각, 두 종류 trace/overlap 통과, seeded mesh/yaw; 기존 Pawn 기각을 생성 허용으로 정정 |
| 기존 물걸레/zone 회귀 | `Cleaning.CarryHoldZoneAndRegistry` | 기존 carry·청소·재고 전체 검증 유지, 실제 stain zone Pawn 허용 계약과 Static floor fixture 정정 |
| native 삭제·serialized 로드 | `Cleaning.BlueprintLoad` | 세 property 부재, 복사본/원본 BP 부모·subobject·SpawnFloor 부착과 package clean; DefaultMap external director/zone instance 로드 |

- fixture의 Box helper는 기존 Movable 계약을 유지하고 **바닥 fixture만 Static**으로 설정한다. Static 지형의 fixture pose를 바꿀 때 Movable→pose 변경→Static 순서로 authoring한다.
- transient stepped mesh와 component 설정은 저장하지 않는다. ragdoll mesh/PhysicsAsset는 기존 asset을 읽기만 하며 Customer/StateTree Source는 변경하지 않았다.

## 빌드·회귀와 실패 이력

로그와 report의 공통 경로는 `Saved/ImplementationUnit3/ClearanceRework/`다.

- UE 5.8 `Build.bat BathhouseSimEditor Win64 Development -WaitMutex -NoHotReloadFromIDE` 최종 성공: `build_05.log`. 첫 build부터 성공했으며 후속 build는 fixture와 load gate 정리다.
- 전체 `Automation RunTests BathhouseSim`: **116/116 성공, 실패 0, 미실행 0**. clean success 105, 경고 포함 success 11. `full_02.log`, `Reports/Full02/index.json`.
- 기존 112개와 신규 4개를 모두 실행했다. Cleaning 전체 **17/17 경고 없는 성공**이며 물걸레·상점 TrashBin·Service·Placement·Interaction·Combat·Customer·Towel·Utility 회귀를 제외하지 않았다.
- 기존 11개 테스트의 경고는 invalid 설정/포화/수건 slot·fixture preview/수거함·startup lease·utility CDO/mobility/provider 등이며 이름과 entries는 report에 남아 있다. 신규 4개는 경고 없는 성공이다.
- 초기 `Cleaning01`은 13/17 성공, 실패 4: PawnsAndRagdoll(잘못된 skeletal asset 경로), PhysicalObstaclesAndWall(Static fixture 이동), FloorAndVariation·CarryHoldZoneAndRegistry(Static fixture 회전). 실제 mesh 경로와 pose authoring을 수정했다.
- `Cleaning02`는 17/17 성공, 3개 테스트에 Static 자식/Movable 부모 attachment 경고가 있었다. 공용 fixture Box의 기존 Mobility를 복원하고 바닥만 Static으로 지정해 최종 전체 실행에서는 청소 17개 경고가 모두 없어졌다. 이후 쓰레기 zone 15cm·얼룩 zone 30cm의 기본 반경으로 신규 fixture를 확인하고 최종 Full02를 실행했다. 실패를 제외하거나 기대값을 낮추지 않았다.
- git diff --check, focused 삭제 참조·LOCTEXT 검색, Source whitespace·인계 문서 소유권/줄 수·입력/유지 범위 SHA256 검사를 통과했다.

## copy-first load gate와 데이터 보존

- 시작 시 UnrealEditor가 모두 종료됐으며 `git --no-optional-locks status --short -- Content Config`는 비어 있었다. 종료 후 같은 결과다.
- 백업: `Saved/MigrationBackup/20261001_clearance_rework/`의 BP_CleaningDirector·BP_StainSpawnZone 두 asset.
- `Content/Developers/MigrationCheck/`에 checksum이 같은 두 복사본을 만들고 `-BathhouseCleaningLoadCopies -BathhouseCleaningClearanceRework`로 BlueprintLoad 실행: **1/1 경고 없는 성공**, `load_copies.log`, `Reports/LoadCopies/index.json`.
- 복사본 두 파일은 exact path·hash·디렉터리 contents를 확인한 뒤 제거했다. 다른 파일과 부모 디렉터리는 제거하지 않았다.
- Template 원본 6 BP는 최종 Full02 BlueprintLoad에서 성공했다. DefaultMap은 `-BathhouseCleaningLoadDefaultMap -BathhouseCleaningClearanceRework`로 별도 실행: **1/1 성공**, `load_defaultmap.log`, `Reports/LoadDefaultMap/index.json`.
- DefaultMap director 1·stain zone 2·trash bin 1과 기존 external package가 로드됐다. SpawnIntervalSeconds=15.000 유지, surviving native parent/subobject와 SpawnFloor를 확인했고 package dirty 없음.
- asset Fatal·Serial size mismatch·asset Failed to load 없음. optional DLL 부재·DDC memory fallback·Rider/HTTP 환경 메시지는 asset migration 실패와 구분한다.
- Content·Config·Level **1,405개 파일 경로와 SHA256 동일**. 시작/끝 상태 기록은 `content_config_status_before.txt`·`content_config_status_after.txt`, rework 입력 snapshot은 `before.json`이다.

## 영향 파일과 성장

아래 전/후는 재작업 시작 Source 기준이다. 경로는 `Source/BathhouseSim/` 아래다.

| 파일 | 전 | 후 |
|---|---:|---:|
| `Private/Cleaning/CleaningDirectorActor.cpp` | 163 | 163 |
| `Private/Cleaning/CleaningSpawnRules.cpp` | 127 | 125 |
| `Private/Cleaning/CleaningSpawnRules.h` | 40 | 39 |
| `Private/Cleaning/LitterSpawnZoneActor.cpp` | 73 | 71 |
| `Private/Cleaning/LitterTongsActor.cpp` | 397 | 397 |
| `Private/Cleaning/StainSpawnZoneActor.cpp` | 73 | 71 |
| `Private/Cleaning/TrashBagActor.cpp` | 378 | 378 |
| `Private/Tests/CleaningBlueprintLoadAutomationTests.cpp` | 183 | 196 |
| `Private/Tests/CleaningLitterAutomationTestSupport.h` | 143 | 142 |
| `Private/Tests/CleaningLitterSpawnAutomationTests.cpp` | 343 | 343 |
| `Private/Tests/CleaningSpawnClearanceAutomationTests.cpp` | 0 | 288 |
| `Private/Tests/CleaningTowelAutomationTests.cpp` | 1099 | 1104 |
| `Public/Cleaning/CleaningDirectorActor.h` | 71 | 69 |
| `Public/Cleaning/LitterSpawnZoneActor.h` | 68 | 65 |
| `Public/Cleaning/StainSpawnZoneActor.h` | 63 | 60 |

- 기존 14개 수정·신규 테스트 1개다. 바닥 helper cpp 127→125, 두 zone cpp 73→71로 줄고 독립 상태/lifecycle을 추가하지 않았다.
- 397줄 Tongs·378줄 Bag에는 LOCTEXT 키만 바꿨다. 기존 큰 CleaningTowel 테스트는 호출 인자와 floor fixture 정정만 반영했으며 신규 4개는 별도 288줄 테스트 파일로 분리했다.
- `.md/Architecture/*`는 변경하지 않았다. 확정 Floor Rule의 내부 버그 수정과 승인된 삭제를 구현했으며 구조를 새로 설계하지 않았다.
- Editor 인계 `.md/PROMPT_UNREAL.md`: 기존 3단위 authoring allowlist 유지, Pawn 여유 삭제 resave·DefaultMap floor object/Mobility 확인·배치 구역/물건/벽/Pawn/경사 PIE 관찰을 추가했다.

## 재리뷰 중점

- response pair와 bBlockingHit 기준이 실제 query-only bounds를 통과시키고 물리 물체/벽을 막는지 확인한다.
- floor component 제외 전 WorldStatic/Static guard와 interface·height 조건, 다른 component/mesh를 제외하지 않는 범위를 확인한다.
- Pawn 면제는 APawn owner 검사에 한정된다. clock의 손님 수 판정과 다른 시스템의 Pawn clearance를 바꾸지 않았는지 확인한다.
- 세 reflected 삭제의 복사본/원본/DefaultMap no-save gate와 Editor resave 인계, 유지 범위 checksum을 확인한다.
