# Building System

## Status And Scope

- 2026-10-02 `EXP-U1` 설계·구현(공간 건물). Source·Editor 반영, 2026-10-02 사용자 PIE 통과·병합(`8ca6a24`). 같은 날 사후 결정 D1(공간별 조각 종류 하나), 아키텍처 Q1 A(지형 구멍), 넓힘 목록 형태 확정(U2 구현)을 반영했다. 입력은 `.md/Work/EXPANSION-PURCHASE/PROMPT_ARCHITECTURE.md`(EXP-001~015)이고 구현 지시는 병합 커밋 `8ca6a24`의 `.md/Work/EXPANSION-PURCHASE/EXP-U1/PROMPT_IMPLEMENTATION.md`(작업 폴더 제거됨, Git 이력)다.
- 가게를 홀·목욕공간·작업공간(지하) 세 공간으로 나누고, 공간마다 직사각형 바닥 경계를 따라 벽·바닥·천장·조명·출입구·통로·계단을 만든다. 공간은 그 바닥의 설비 배치 구역이자 생성 조각의 생성자다(홀 = 쓰레기 조각, 목욕공간 = 물 얼룩 조각, 작업공간 = 없음, D1).
- 2026-10-02 `EXP-U2` 설계·구현, 사용자 PIE 통과·병합(`3c17e41`): 아래 Expansion 절의 넓힘 목록·runtime 넓힘 적용·편집 미리보기·넓힘 검증. 구입·확장 탭·열쇠·한도·락커 판매는 [ExpansionPurchaseSystem.md](ExpansionPurchaseSystem.md)다. 구현 지시는 병합 커밋 `3c17e41`의 `.md/Work/EXPANSION-PURCHASE/EXP-U2/PROMPT_IMPLEMENTATION.md`(작업 폴더 제거됨, Git 이력)다.
- 2026-10-02 `EXP-U3` 설계·구현, 사용자 PIE 통과: 사용자 결정 D2(넓힘 줄마다 가격, 전체 상한 없음)와 D3(넓힘 한 번에 여러 벽, 벽별 양)를 Expansion 절·Validation (U2·U3) 표에 합쳤다. 구현 지시는 병합 커밋의 `.md/Work/EXPANSION-PURCHASE/EXP-U3/PROMPT_IMPLEMENTATION.md`(Git 이력) 5·6절이다.
- `EXPANSION-PURCHASE` 완료(2026-10-02, U3 병합 `201b2d0`). 작업 폴더는 제거됐고, 이 문서의 `.md/Work/EXPANSION-PURCHASE/…` 경로는 해당 병합 커밋 이력에서 읽는다.

## Source Scope

```text
Public/Building/
  BathhouseSpaceTypes.h            공간 종류·벽 방향 enum, 재질·조명·개구부·계단 USTRUCT
  BathhouseSpaceActor.h            ABathhouseSpaceActor : AFacilityPlacementZoneActor
  BathhouseSpaceShellComponent.h   생성 형상·조명 component의 소유·재생성
  BathhouseBuildingSettings.h      UBathhouseBuildingSettings : UDeveloperSettings
Private/Building/
  (위 .cpp)
  BathhouseSpaceLayout.h/.cpp      순수 계산: 공간 snapshot → 공간별 형상 계획, 직사각형 빼기, 조각 분할
  BathhouseSpaceValidation.h/.cpp  규칙 검사(`CollectProblems`, `ValidateLayout`)
  BathhouseSpacePositionSuggestion.cpp  위치 제안(`SuggestTouchingLocation`, `ApplyMove`, 후보 검증, 문구 부착)
  BathhouseSpaceWorldValidation.cpp  snapshot 수집, Settings 입력, Nav 범위, 계단 통로 장애물, 설비 소속(`ValidateWorld`)
  BathhouseSpaceValidationInternal.h  세 파일이 공유하는 내부 helper(이름 있는 namespace `BathhouseSpaceValidationDetail`)
  BathhouseSpaceEditorSync.h/.cpp  WITH_EDITOR: 편집 world의 공간 형상 지연 일괄 재생성
  BathhouseCleaningChunkSpawner.h/.cpp  runtime 생성 조각 Actor spawn
  BathhouseSpaceExpansion.cpp      (U2) 공간 Actor 넓힘 적용·되돌림·효과 횟수·미리보기 글자
  BathhouseSpaceExpansionLayout.cpp  (U2) 순수: 넓힌 안쪽 직사각형·넓힘 띠
  BathhouseSpaceExpansionValidation.cpp  (U2) 넓힘 검사(`ValidateExpansion`)
  BathhouseSpacePreviewLabelWidget.h/.cpp  (U2 복귀 R1) 편집 미리보기 글자 native widget
Private/Tests/
  BathhouseBuildingAutomationTests.cpp
  BathhouseExpansionAutomationTests.cpp  (U2)
```

`Building`은 새 Source 하위 폴더다([CoreSystem.md](CoreSystem.md) System Documents).

## Ownership

| 대상 | 상태·authoring owner | 실행 owner |
|---|---|---|
| 공간 종류, 바닥 크기·위치, 천장 높이, 재질, 조명, 개구부, 계단, 허용 설비, 생성 조각 종류 | 각 `ABathhouseSpaceActor` Level instance | 공간 Actor |
| 벽 두께, 바닥·천장 두께, 형상용 상자 mesh, 생성 조각 최대 크기, 조각 종류별 구역 class | `UBathhouseBuildingSettings`(Project Settings) | 읽기 전용 |
| 공간별 형상 계획(벽·바닥·천장·계단 상자, 조각 직사각형) | 파생값, 저장하지 않음 | `FBathhouseSpaceLayout`(순수) |
| 생성 component(ISM·조명·편집용 조각 미리보기) | `UBathhouseSpaceShellComponent`(Transient 목록) | shell component |
| 생성 조각 Actor | 공간 Actor(Transient weak 목록) | `FBathhouseCleaningChunkSpawner` |
| 설비 배치 구역 bounds·바닥·grid | 공간 Actor가 상속한 `AFacilityPlacementZoneActor` | [PlacementSystem.md](PlacementSystem.md) |
| 조각 안 쓰레기·물 얼룩 생성 | 조각 Actor와 `ACleaningDirectorActor` | [CleaningLitterSystem.md](CleaningLitterSystem.md) |
| 손님 길 | Level `NavMeshBoundsVolume` + Recast `Dynamic` 재생성 | UE Navigation |
| (U2·U3) 넓힘 목록(줄마다 벽·양 목록과 가격) | 공간 Level instance `ExpansionSteps` | 공간 Actor |
| (U2) 넓힌 횟수(runtime) / 미리보기 횟수(편집) | 공간 Actor `AppliedExpansionCount`(Transient) / `EditorPreviewExpansionCount`(Transient, editor-only) | 구입 transaction([ExpansionPurchaseSystem.md](ExpansionPurchaseSystem.md)) / Details |

## Space Actor

`ABathhouseSpaceActor`는 `AFacilityPlacementZoneActor`의 자식이다. 공간 바닥 = 설비 배치 구역이므로 구역 동기화가 없다. 기존 Computer `ManagedBathPlacementZone`(type `AFacilityPlacementZoneActor`)에 목욕공간 Actor를 그대로 연결한다.

- component: 새 root `SpaceRoot`(`USceneComponent`, Static) 아래에 상속 `ZoneBounds`(Movable)를 다시 붙이고, `SpaceRoot` 아래 `Shell`(`UBathhouseSpaceShellComponent`)을 둔다. root를 바꾸는 이유는 U2 넓힘에서 Actor 위치(0회 기준점)를 고정한 채 `ZoneBounds` 상대 위치만 옮기기 위해서다.
- Actor transform 계약: Location XY = 0회 바닥 직사각형 중심, Location Z = 바닥 윗면 높이. Rotation은 0, Scale은 1이어야 한다(검증 오류, runtime은 형상을 만들지 않고 오류 로그).
- `ZoneBounds` XY extent = 안쪽 바닥 크기의 절반, 상대 위치 = 현재 직사각형 중심 offset(U1은 0). Z extent와 `PlacementFloor` identity는 Blueprint 값을 유지한다. 그래서 설치 바닥 = 공간 바닥이다.
- WP: 생성자에서 `bIsSpatiallyLoaded=false`(WITH_EDITORONLY_DATA). 형상 계산이 다른 공간의 authored 값을 읽으므로 세 공간은 항상 함께 로드돼야 한다.

공간 Actor property(Category `Bathhouse Space`, 모두 EditAnywhere, 값은 Level instance가 정본):

| property | 의미 |
|---|---|
| `SpaceKind` | `EBathhouseSpaceKind`: `Hall`(홀), `Bath`(목욕공간), `Work`(작업공간) |
| `FloorSizeCm` | 안쪽 바닥 X×Y (벽 안쪽 면 사이) |
| `CeilingHeightCm` | 바닥 윗면 → 천장 아랫면 |
| `Surfaces` | `WallMaterial`, `FloorMaterial`, `CeilingMaterial` |
| `Lighting` | `SpacingCm`, `IntensityCandela`, `AttenuationRadiusCm`, `Color`, `bCastShadows`, `CeilingOffsetCm` |
| `Openings` | `FBathhouseSpaceOpening`: `Side`, `CenterOffsetCm`, `WidthCm`, `HeightCm`, `ConnectedSpace`(없음 = 바깥 출입구) |
| `Stairs` | `FBathhouseStairSpec`: `LowerSpace`, `TopEdgeCenterOffsetCm`, `DownSide`, `WidthCm`, `RunCm`, `StepCount`, `GuardHeightCm`, `StepMaterial`, `StairWallMaterial` |
| `CleaningChunkKind` | `EBathhouseCleaningChunkKind`: `None`, `Litter`(쓰레기), `Stain`(물 얼룩). C++ 기본 `None`. 한 공간에 한 종류 |
| (상속) `AllowedFacilityTags` | 이 공간에 놓을 수 있는 설비 종류 태그(`Facility.Type.*`) |
| (U2·U3) `ExpansionSteps` | 넓힘 목록(Category `Bathhouse Space|Expansion`, 줄 = 넓힘 한 번의 `Sides`·`Price`, Expansion 절) |

- `EBathhouseSpaceSide`: `East`(+X), `West`(−X), `North`(+Y), `South`(−Y). world 축 기준이다.
- `CenterOffsetCm`: 개구부 중심의 벽 길이 방향 좌표를 공간 Actor 위치 기준으로 잰 값(동·서 벽은 Y, 남·북 벽은 X). Actor 위치는 넓힘에도 고정되므로 벽이 물러나도 개구부가 벽을 따라 미끄러지지 않는다.
- `TopEdgeCenterOffsetCm`: 위층 공간 Actor 위치 → 계단 맨 위 가장자리 중심 XY. `DownSide`가 내려가는 방향이다.
- 통로는 한쪽 공간의 `Openings`에만 적는다(`ConnectedSpace` 지정). 맞닿은 공간의 벽도 같은 world 구간으로 뚫린다. 계단은 위층 공간(홀)의 `Stairs`에 적는다.

## Geometry Rules

좌표는 모두 world, 단위 cm. `t` = `WallThicknessCm`, `s` = `SlabThicknessCm`. 안쪽 직사각형 `I`, 바깥 직사각형 `O` = `I`를 사방 `t`만큼 넓힌 것, 바닥 윗면 `Zf`, 천장 아랫면 `Zc = Zf + CeilingHeightCm`.

- 바닥: `O`에서 이 공간을 위층으로 하는 계단의 판 구멍을 뺀 직사각형들, Z `[Zf − s, Zf]`.
- 천장: `O`에서 이 공간을 아래층으로 하는 계단의 판 구멍을 뺀 직사각형들, Z `[Zc, Zc + s]`.
- 벽: 동·서 벽은 Y로 `O` 전체 길이, 남·북 벽은 X로 `I` 길이(모서리 겹침 없음). 두께 `t`는 `I` 밖으로, Z `[Zf, Zc]`.
- 개구부: 벽 길이 구간 `[c − w/2, c + w/2]` × Z `[Zf, Zf + h]`를 벽에서 뺀다. 나머지는 개구부 사이 전체 높이 조각 + 개구부 위 인방 조각이다. 통로는 `ConnectedSpace` 벽에서도 같은 world 구간을 뺀다.
- 맞닿은 두 공간은 바깥 직사각형 변이 정확히 닿는다. 그래서 두 벽은 등을 맞대고, 바닥·천장은 변끼리 붙어 겹치지 않는다.
- 직사각형 빼기는 guillotine 분할로 겹치지 않는 직사각형 목록을 만든다.

계단(위층 `U`, 아래층 `L`, 높이 `H = Zf(U) − Zf(L)`). 계단 frame 원점 = 위층 Actor 위치 + `TopEdgeCenterOffsetCm`(Z = `Zf(U)`), +x = `DownSide`, y = 폭 방향.

- 구멍 `R` = x `[0, RunCm]` × y `[−W/2, W/2]`(걷는 통로). 판 구멍 = x `[0, RunCm]` × y `[−W/2 − t, W/2 + t]`(`R` + 옆 벽 발자국)이고 `U` 바닥과 `L` 천장에서 뺀다(2026-10-02 복귀 A2).
- 경사로(`StairRamp`): 윗면이 (0, `Zf(U)`)와 (`RunCm`, `Zf(L)`)를 잇는 두께 `s` 회전 상자. 보이지 않고 충돌만 한다. 실제 걷는 면이다.
- 계단 판(`StairStep`): `StepCount`개 시각 상자, 충돌 없음. 각 판 윗면은 경사로와 한 단 높이 이내다.
- 계단 벽(`StairWall`, 두께 `t`, `R` 바깥): 양옆 Z `[Zf(L), Zf(U) + GuardHeightCm]`. 위쪽 끝(x=0 바깥, 폭은 옆 벽 바깥까지) Z `[Zf(L), Zc(L)]`와 `[Zc(L) + s, Zf(U) − s]`(위층 쪽은 입구로 열림). 아래쪽 끝(x=Run 바깥) Z `[Zc(L) + s, Zf(U) − s]`와 `[Zf(U), Zf(U) + GuardHeightCm]`(아래층 출구로 열림). 판 두께 구간에서는 판이 우선해 끝 벽이 건너뛰고, 높이 0 이하 조각은 만들지 않는다.
- 면 겹침 불변식: 보이는 part(Floor·Wall·Ceiling·StairWall·StairStep) 상자끼리 양의 부피로 겹치지 않는다. 그래서 같은 방향 동일 평면 면이 없고 접면은 등을 맞댄 면뿐이다. 위 입구 앞 띠 윗면은 Floor, 아래 출구 위 띠 아랫면은 Ceiling이다.
- 위층 구멍 막이(`StairKeepClear`): `R` 위 Z `[Zf(U), Zc(U)]`의 보이지 않는 QueryOnly 상자. 경사로 꼭대기가 바닥 지지로 잡혀 구멍 위에 설비가 놓이는 것을 막는다.

조명: `I`를 `SpacingCm` 이하 간격으로 균등 분할한 칸 중심마다 Movable `UPointLightComponent` 하나, Z = `Zc − CeilingOffsetCm`. 축마다 최소 한 개.

생성 component와 충돌·Navigation:

| part | 표시 | 충돌 | object type | Nav | 배치 trace 채널 | 재질 |
|---|---|---|---|---|---|---|
| Floor | 표시 | BlockAll | WorldStatic | 관련 | Block | `FloorMaterial` |
| Wall | 표시 | BlockAll | WorldStatic | 관련 | Block | `WallMaterial` |
| Ceiling | 표시 | BlockAll | WorldStatic | 비관련 | Block | `CeilingMaterial` |
| StairStep | 표시 | 없음 | — | 비관련 | — | `StepMaterial` |
| StairRamp | 숨김 | BlockAll | WorldStatic | 비관련 | Block | 없음 |
| StairWall | 표시 | BlockAll | WorldStatic | 관련 | Block | `StairWallMaterial` |
| StairKeepClear | 숨김 | QueryOnly, WorldStatic·WorldDynamic·PhysicsBody만 Block | WorldStatic | 비관련 | Ignore | 없음 |

- part마다 `UInstancedStaticMeshComponent` 하나(상자 mesh = Settings `ShellBoxMesh`, instance scale은 mesh local bounds로 파생)를 쓰고 Mobility는 Static이다. 청소 바닥 규칙(WorldStatic + Static)을 바닥이 만족한다.
- "배치 trace 채널 Block"은 벽 너머 다른 공간의 배치 구역을 조준하지 못하게 한다. 형상 hit는 구역 hit가 아니다. 구역은 `AFacilityPlacementZoneActor::IsZoneSurfaceHit`(hit component = `ZoneBounds`, hit 면이 바닥 위쪽을 향함)일 때만 인정한다([PlacementSystem.md](PlacementSystem.md) Space Zones, 2026-10-02 복귀 A1).
- 생성 component와 조명은 `RF_Transient`이며 저장하지 않는다. 재생성은 항상 전부 파괴 후 새로 만든다(Static component를 옮기지 않음).

## Lifecycle

- 형상 계획은 `FBathhouseSpaceLayout::Build(Snapshots, Values)` 하나로 계산한다. snapshot은 같은 world의 모든 공간 Actor authored 값만 읽는다(다른 공간의 생성 component는 읽지 않음). 그래서 로드·BeginPlay 순서와 무관하게 결과가 같다.
- `OnConstruction`: `ZoneBounds` 갱신 → `Super::OnConstruction`(grid) → 자기 shell 재생성 → editor world면 `FBathhouseSpaceEditorSync::RequestRebuild(World)`.
- `FBathhouseSpaceEditorSync`(WITH_EDITOR): world별로 다음 tick 한 번 모든 공간의 shell만 재생성한다(coalesce, `FTSTicker`). OnConstruction을 다시 부르지 않아 재귀가 없다. 로드 순서, 다른 공간 편집, Undo 뒤에도 통로·계단 구멍이 맞는다. transaction·Modify를 하지 않는다(생성물은 Transient).
- `BeginPlay`: `ZoneBounds` 갱신 → shell 재생성 → `CleaningChunkKind`가 `None`이 아니면 그 종류의 생성 조각 spawn → 이 공간이 owner인 검증 오류를 `LogBathhouseBuilding` Error로 한 번 기록. runtime 정본은 BeginPlay 재생성이다(PIE 복제·cook 직렬화에 의존하지 않음).
- `EndPlay`: 생성 조각 Actor 파괴.
- (U2) `BeginPlay` 끝에 구입 subsystem에 등록하고, 목록 끝 바깥 직사각형 × Z `[Zf − s, Zc + s]`를 Nav dirty area로 한 번 등록한다(편집 미리보기 상태로 만든 Nav가 저장·PIE 복제돼도 게임 형상으로 다시 만들게 함). `EndPlay`에서 등록 해제.
- 검증 오류가 있어도 계산 가능한 형상은 만든다. Rotation·Scale 위반과 Settings 상자 mesh 누락만 형상 생략이다.

## Cleaning Chunks

- `I`를 Settings `CleaningChunkMaxSizeCm` 이하의 같은 크기 칸으로 나눈다(축마다 `ceil(크기/최대)`개, 칸 = 크기/개수). 넓힘으로 늘어난 바닥(넓힌 뒤 직사각형 − 넓히기 전, 모서리 포함, `ExpansionBandRects`)은 직사각형마다 같은 규칙으로 나눠 추가한다(기존 조각 불변).
- 공간 Actor `CleaningChunkKind`가 종류를 정하고 Settings가 그 종류의 class를 준다(`Litter` → `LitterChunkZoneClass`, `Stain` → `StainChunkZoneClass`, `None` → 조각 없음). 기능 계약 D1의 값(홀 `Litter`, 목욕공간 `Stain`, 작업공간 `None`)은 Level instance 값이다. 공간 종류로 조각 종류를 추론하지 않는다. U2 넓힘 띠도 같은 종류로 추가한다.
- 칸마다 그 class 하나를 deferred spawn → `SetSpawnAreaHalfSizeXY(칸/2)` → `SpawnFloor` world Z가 `Zf`가 되게 Actor Z 보정 → `FinishSpawning`. 조각별 최대 개수는 class 기본값이다.
- 편집 화면에서는 같은 칸을 editor-only 선 상자(게임에서 없음)로 보여 준다. `None`이면 미리보기도 없다.

## Validation

`IsDataValid`는 같은 world의 공간 전체 layout을 검사하고 이 Actor가 관련된 문제를 보고한다. 문제마다 owner 공간이 있고 runtime은 owner만 로그를 남긴다.

| 검사 | 결과 |
|---|---|
| 종류마다 공간이 정확히 하나 | Error |
| Rotation 0, Scale 1 | Error |
| `FloorSizeCm`·`CeilingHeightCm`·조명 간격 양수·유한, `AllowedFacilityTags` 비어 있지 않음 | Error |
| 두 공간의 부피(바깥 직사각형 × Z `[Zf − s, Zc + s]`)가 겹침(닿음은 허용) | Error |
| 개구부: 폭·높이 양수, 높이 ≤ 천장, 벽 안쪽 길이 안, 같은 벽에서 서로 겹치지 않음 | Error |
| 통로: `ConnectedSpace`가 다른 공간이고 그 변에서 바깥 직사각형이 닿고 구간이 두 벽 안쪽 길이 안, 바닥 Z 같음, 높이 ≤ 두 천장 | Error |
| 바깥 출입구 구간에 다른 공간이 닿음 | Error |
| 홀에 바깥 출입구가 하나도 없음(손님은 홀 출입구로만 드나든다. 목욕공간·작업공간에는 요구하지 않음) / Work로 내려가는 계단이 없음 | Error |
| 계단: `LowerSpace` 유효·다름·더 낮음, 폭·길이·판 수 양수, 계단 벽까지 포함한 발자국이 두 공간 안쪽 안, 위 입구 앞과 아래 출구 앞에 계단 폭만큼의 빈 바닥, 경사 ≤ `UCharacterMovementComponent` 기본 `GetWalkableFloorAngle()` | Error |
| 계단 통로(구멍 `R` 안, Z `Zc(L)`~`Zf(U)`, 두 판 두께 구간 포함)를 공간이 아닌 blocking 물체(지형 등)가 막음 | Error(편집 world trace, 2026-10-02 복귀 A3) |
| 손님 공간 바닥 직사각형이 어떤 `NavMeshBoundsVolume`에 들지 않음, 바깥 출입구 앞 바깥 지점(개구부 폭만큼 떨어진 곳)이 Nav 범위 밖, Work 바닥이 Nav 범위 안 | Error |
| Settings 상자 mesh 없음, 공간이 고른 조각 종류의 Settings class 없음 / 공간 재질 없음, Work에 `CleaningChunkKind` ≠ `None`(손님이 없어 생성 없음) | Error / Warning |
| 배치된 `IPlaceableFacility`가 어느 공간에도 없거나 그 공간이 허용하지 않는 종류 | Warning |

부동소수 비교의 허용 오차만 엔진 상수(`UE_KINDA_SMALL_NUMBER`)를 쓴다.

Validation (U2·U3): `ValidateWorld`는 위 표를 0회 복사본에 적용하고(위치 제안도 0회 기준) 이어서 순수 `ValidateExpansion(…, HallEffectRowCount, …)`을 돌린다. `HallEffectRowCount`는 world 첫 유효 Authority Definition의 `Tiers.Num()`이고 없으면 `INDEX_NONE`(효과 표 검사 생략). 아래는 `EXP-U3` 설계 상태다(Status 참조).

| 검사 | 코드 | 결과 |
|---|---|---|
| 줄 `Sides`가 비어 있음 | `ExpansionSidesEmpty` | Error |
| 같은 줄에 같은 `Side`가 두 번 이상(벽마다 한 번 보고, 뒤쪽 중복 항목은 다른 검사 생략) | `ExpansionSideDuplicate` | Error |
| 줄 `Price` ≤ 0 | `ExpansionPriceInvalid` | Error |
| 벽 항목 `AmountCm` ≤ 0·비유한(항목마다) | `ExpansionAmountInvalid` | Error |
| 0회에 다른 공간과 바깥 직사각형이 닿은 변(부피 Z 구간 겹침 + 변 일치 + 길이 구간 겹침, 통로 벽 포함)을 넓히는 벽 항목 | `ExpansionTouchingSide` | Error |
| 홀 줄 수 + 1 > `HallEffectRowCount`(owner 홀) | `ExpansionHallEffectShort` | Error |
| 목록 끝까지 넓힌 부피끼리 겹침 / 끝 모습에서 바깥 출입구 구간에 다른 공간이 닿음 | `ExpansionOverlap` / `ExpansionOutsideOpeningBlocked` | Error |
| 끝 모습 손님 공간 바닥·출입구 앞 바깥 지점이 Nav 범위 밖, 끝 모습 Work 바닥이 Nav 범위 안 | `ExpansionNavOutside` / `ExpansionNavWorkCovered` | Error |
| 바깥 출입구가 있는 변을 넓히는 벽 항목 | `ExpansionEntranceSide` | Warning |

직사각형은 커지기만 하므로(여러 벽이어도 같음) 끝 모습끼리 검사가 모든 중간 조합을 덮는다. 넓힘 문제에는 위치 제안을 붙이지 않는다. 문구는 "{공간} {k}번째 넓힘 줄"과 벽 방향을 밝힌다. 전체 구입 상한 합계 경고는 없다(D2).

### Position Suggestions (2026-10-02 사용자 (가))

공간 위치는 계속 Actor Location 직접 입력이다. 아래 오류는 문구 뒤에 맞닿게 하는 Location 값을 제안한다. 자동으로 옮기지 않는다.

- 대상 오류: 통로의 두 공간이 그 변에서 닿지 않음(떨어짐 또는 겹침), 같은 높이 띠의 두 공간 부피 겹침, 통로 두 공간의 바닥 Z 다름.
- 맞닿음 축 값: 변의 법선 축에서 두 바깥 직사각형 변이 같아지는 Actor 위치. 예: 홀 동쪽 통로면 `목욕공간 X = 홀 X + 홀 안쪽 X/2 + 목욕공간 안쪽 X/2 + 2 × 벽 두께`(현재 넓힘 0, 직사각형 중심 = Actor XY). 같은 식을 풀어 통로를 적은 공간의 값도 구한다.
- 순서: 연결 대상 공간(`ConnectedSpace`)을 옮기는 값을 먼저, 통로를 적은 공간을 옮기는 값을 `또는`으로 다음에 적는다. 통로 없는 겹침은 겹침이 작은 축으로 각 공간을 떼는 값을 둘 다 적는다. 바닥 Z 차이는 연결 대상 Z를 통로 쪽 Z로 맞추는 값이다.
- 통로 구간이 연결 대상 벽 안쪽 길이를 벗어나면 맞닿음 축 값에 더해 벽 길이 방향 축 값(구간이 벽 안쪽에 들어오는 가장 가까운 위치)도 제안한다.
- 제안 값을 적용한 snapshot으로 같은 검증을 다시 돌려 그 오류가 사라지고 새 겹침이 생기지 않을 때만 제안한다.
- 값은 두 공간 값과 Settings 벽 두께에서만 계산한다. 표시는 cm(Details 단위)와 괄호 안 m다.

## Navigation

- 손님 길은 Level `NavMeshBoundsVolume`이 넓게(홀·목욕공간과 앞으로의 넓힘, 출입구 밖 마당 경로 포함) 덮고, Z 범위는 지상만 덮어 지하 바닥을 넣지 않는다. 공간은 Nav 범위를 움직이지 않는다.
- Recast `RuntimeGeneration = Dynamic`이 생성 형상의 등록·파괴를 반영한다. 계단 판·경사로·천장은 Nav 비관련이라 계단과 구멍 위에는 Nav가 없다. 손님은 지하에 갈 길이 없다.
- 범위가 넓힘을 다 덮는지는 위 Validation이 알린다.

## Blueprint/API Contracts

- 신규 reflected: 위 class·enum·struct와 property·component 이름(`SpaceRoot`, `Shell`), `UBathhouseBuildingSettings` 값. (U2) `FBathhouseSpaceExpansionStep`, `ExpansionSteps`, editor-only `EditorPreviewExpansionCount`, `AppliedExpansionCount`, Settings `EditorPreviewLabelWorldSizeCm`·`EditorPreviewLabelHeightCm`·`EditorPreviewLabelFontSize`, 편집 전용 native widget class `UBathhouseSpacePreviewLabelWidget`. 모두 추가라 redirect가 필요 없다.
- (U3) 신규 `FBathhouseSpaceExpansionSide`(`Side`, `AmountCm`), `FBathhouseSpaceExpansionStep::Sides`(TitleProperty `Side`)·`Price`. 삭제 `FBathhouseSpaceExpansionStep::Side`·`AmountCm`. struct 이름 유지·property 삭제라 redirect가 필요 없고, 옛 Level 값은 load 때 건너뛰므로 Editor 단계가 세 공간 줄을 다시 입력·저장한다.
- Blueprint `BP_BathhouseSpace`(parent `ABathhouseSpaceActor`)는 상속 `GridVisual`에 Plane과 `MI_FacilityPlacementGrid`만 지정한다. 형상·조명·조각을 Blueprint graph로 만들지 않는다.
- 공간 Actor는 C++ public으로 `GetSpaceKind()`, `GetInteriorRect()`(현재 안쪽 바닥 world XY), `GetFloorZ()`, `GetCeilingZ()`를 제공한다.
- 기존 class 이름 변경·삭제가 없어 Core Redirect가 필요 없다.
- 지형(Q1 A): 계단 통로가 지형을 지나므로 구멍을 지원하는 프로젝트 지형 재질(지금 엔진 격자 재질과 같은 모습)을 지형에 지정하고, 0회 홀 안쪽 바닥 아래에 지형 구멍을 둔다. 계단이 그 범위를 벗어나면 편집 world 검증이 알린다. Editor 정본은 `.md/Unreal/`이 기록한다.

## Expansion (U1 형태 확정, U2·U3 구현)

넓힘 데이터·편집 미리보기 형태는 2026-10-02 사용자 승인 형태에 D2(줄마다 가격)·D3(줄마다 여러 벽)를 더한 것이다. 구입 상태 owner·tier는 [ExpansionPurchaseSystem.md](ExpansionPurchaseSystem.md)가 정한다.

- `ExpansionSteps`: `TArray<FBathhouseSpaceExpansionStep>`(EditAnywhere, Level instance 정본). 줄 하나 = 그 공간의 넓힘 한 번(한 단계). index `k` = `k+1`번째 넓힘. 공간별 넓힘 횟수 상한 = 줄 수. 전체 구입 상한은 없다(D2).
  - `Sides`: `TArray<FBathhouseSpaceExpansionSide>`, 항목 = 함께 물러날 벽 하나(`Side`, world 축)와 양(`AmountCm`, 양수). 같은 벽은 한 줄에 한 번(D3).
  - `Price`: 이 넓힘의 구입 가격(원, 양수). 그 공간의 몇 번째 넓힘인지별이며 전체 구입 순번과 무관하다(D2). 가격은 넓힘 단위이고 벽별 가격은 없다.
- 넓힌 직사각형: 횟수 `n`이면 0회 안쪽 직사각형에 index `0..n-1` 줄을 순서대로 적용한다. 줄 하나는 그 줄의 벽 항목마다 그 변을 바깥으로 양만큼 옮긴다(같은 직사각형에 동시에, 항목 순서 무관). 양이 0 이하·비유한인 항목과 같은 줄의 뒤쪽 중복 항목은 형상에서 건너뛴다(검증 오류). 두 변이 함께 물러나면 그 사이 모서리도 새 직사각형에 든다. Actor 위치는 고정이고 `ZoneBounds` 상대 위치·extent만 바뀐다. 개구부 `CenterOffsetCm`은 Actor 기준이라 불변이고 물러난 벽의 개구부는 벽과 함께 옮겨진다. 계단은 움직이지 않는다.
- 효과 횟수: `EWorldType::Editor` world는 `EditorPreviewExpansionCount`(`[0, 줄 수]` clamp), 그 밖은 `AppliedExpansionCount`(시작 0). `GetInteriorRect()`·`FillSnapshot`·`ApplyZoneGeometry`·shell이 효과 횟수를 쓴다. `GetBaseInteriorRect()`(0회), `GetInteriorRectForCount(n)`, (U3) `GetNextExpansionPrice()`(다음 줄 가격, 상한이면 0), `IsAtExpansionLimit()`도 제공한다.
- snapshot: `BaseInterior`, `Steps`(줄마다 `Sides`·`Price`), `ExpansionCount`를 담고 `Interior` = 넓힌 직사각형. 순수 `ExpandInterior`, `IsStepApplicable`(벽 1개 이상·모든 양 유효·중복 없음), `ExpansionBandRects`(k번째 줄이 늘린 영역 = 넓힌 뒤 − 넓히기 전을 `SubtractRects`로 나눈 겹치지 않는 직사각형 목록, 모서리 포함), `WithExpansionCount`.
- runtime 적용(구입 transaction만 호출): `CanApplyNextExpansion`(부작용 없음: game world·begun play·다음 줄 `IsStepApplicable`·다음 줄 `Price` > 0·transform·상자 mesh·두께) → `ApplyNextExpansion(Undo)`: 횟수 +1 → `ApplyZoneGeometry` → 이 공간 shell만 한 번 재생성(실패면 되돌림, 벽을 하나씩 차례로 적용하지 않음) → 조각 종류가 있으면 `ExpansionBandRects`의 직사각형마다 같은 분할 규칙으로 나눠 조각 추가(기존 조각 불변). `UndoExpansion(Undo)`는 추가 조각 파괴, 횟수 복원, 구역·shell 재생성.
- 이웃 공간 shell은 다시 만들지 않는다. `BuildPlan`은 이웃의 Actor 위치·개구부·계단·바닥 Z만 읽고 안쪽 직사각형을 읽지 않으며, 맞닿은 변은 넓힐 수 없기 때문이다(여러 벽이어도 벽마다 같은 규칙).
- 재생성은 한 함수 호출 안에서 파괴 후 생성이라 바닥이 비는 물리 step이 없다. 새 벽은 옛 바깥 직사각형 밖에만 생기고 이미 놓인 Actor는 건드리지 않는다. 배치 격자는 다음 `SetGridVisible(true)`가 새 `ZoneBounds`로 다시 계산한다.
- 편집 미리보기: 미리보기 횟수를 바꾸면 OnConstruction과 편집 동기화가 모든 공간을 다시 짓는다. 횟수 > 0이면 shell이 `넓힘 미리보기 N회`를 띄우고, 효과 횟수 snapshot에서 이 공간 부피 겹침이 있으면 ` · 겹침 있음`을 덧붙인다(2026-10-02 복귀 R1·RET-003로 표시 방식 재설계).
  - component: editor-only Transient `UWidgetComponent`(World space, 충돌 없음, Nav 비관련, `bDrawAtDesiredSize`)에 코드로 만든 native `UBathhouseSpacePreviewLabelWidget`(root `UTextBlock` 하나, WBP 없음)을 띄운다. 글꼴은 `UTextBlock` 기본 글꼴(프로젝트 WBP와 같은 엔진 기본 UMG 글꼴, 한글은 엔진 fallback)이며 크기만 Settings `EditorPreviewLabelFontSize`로 정한다. `UTextRenderComponent`는 offline 글꼴만 그려 한글을 못 쓰므로 쓰지 않는다.
  - 위치: 순수 `FBathhouseSpaceLayout::PreviewLabelPlacement`. XY = 효과 횟수 안쪽 중심, Z = 모든 공간 중 가장 높은 천장 판 윗면 + Settings `EditorPreviewLabelHeightCm`(모든 공간 같은 높이라 지하 글자도 위에서 보임). XY가 겹치는 다른 공간 중 바닥이 더 높은 것이 있으면(아래층) 글자를 중심 남쪽에, 아니면 북쪽에 붙인다.
  - 방향·크기: 앞면 world +Z, 글자 위쪽 world +Y. component scale = `EditorPreviewLabelWorldSizeCm / EditorPreviewLabelFontSize`.
  - 그리기(2026-10-02 복귀 RET-004): hidden-in-game을 쓰지 않는다(엔진 `IsVisible()`이 false가 되어 `UWidgetComponent`가 그리지 않음). game world 비노출은 편집 world에서만 만든다는 생성 조건이 지킨다. `WidgetClass`·World space를 등록 전에 지정하고, `TickMode` Enabled, `TickWhenOffscreen` true, 자동 redraw, `DrawSize`는 엔진 기본값(양수), 문구 지정 뒤 `RequestRedraw()`. default subobject를 추가하지 않는다. 직렬화된 `ZoneBounds`(미리보기 결과)는 BeginPlay가 다시 계산하므로 게임에 영향이 없다.
- 검증은 미리보기와 무관하게 0회 복사본(기존 규칙)과 목록 끝 복사본(Validation (U2·U3) 표)을 검사한다.

## Implementation Notes (2026-10-02 구현)

- 입력 snapshot(`FBathhouseSpaceSnapshot`)·형상 계획·순수 계산은 `Private/Building/BathhouseSpaceLayout.*`, 규칙 검사는 `BathhouseSpaceValidation.cpp`, 위치 제안은 `BathhouseSpacePositionSuggestion.cpp`, world 수집·Nav·계단 trace는 `BathhouseSpaceWorldValidation.cpp`다(공개 API는 `BathhouseSpaceValidation.h`의 `FBathhouseSpaceValidation` 하나). 개구부·계단 위치는 Actor 기준 상대값으로 담고 world 값은 helper가 계산한다. 위치 제안의 이동 후보는 snapshot 복사본에 적용해 같은 검사를 다시 돌려 확인한 것만 문구에 붙는다.
- 계단 구현 치수: 경사로 상자 두께 = 판 두께. 계단 판 윗면 = 그 판 중앙 위치의 경사로 윗면 높이. 계단 옆 벽은 `R`의 길이 방향 `[0, Run]`, 위·아래 끝 벽은 두께 `t`에 폭 방향으로 옆 벽 두께까지 포함한다. 판 구멍은 옆 벽 발자국까지 넓고(`StairSlabHole`) 위·아래 끝 벽은 판 두께 구간을 건너뛰는 두 조각이다(Geometry Rules, 복귀 A2). 구멍 막이·trace는 `R`(`StairHole`)을 쓴다. 위 입구 앞과 아래 출구 앞의 출입 자리는 계단 벽 바깥 `t` 지점부터 계단 폭만큼이다.
- 계단 통로 장애물 trace는 구멍 안 3×3 지점에서 위층 바닥 윗면에서 아래로 쏜다(공간 Actor가 아닌 WorldStatic·WorldDynamic blocking만 오류). 아래 끝은 Validation 표대로 아래층 천장 아랫면 `Zc(L)`이다(복귀 A3).
- 검사는 `ValidateLayout`(순수, 위치 제안 포함), `ValidateNavigation`(순수, Nav bounds 상자를 받음), `ValidateWorld`(world에서 snapshot·`NavMeshBoundsVolume`·계단 trace·배치된 설비를 모아 위 둘을 합침)로 나뉜다. 편집 world의 `IsDataValid`와 BeginPlay 로그가 `ValidateWorld`를 쓴다.
- 구역 인정(복귀 A1): `AFacilityPlacementZoneActor::IsZoneSurfaceHit`가 hit component = `ZoneBounds`이고 hit 법선이 구역 바닥 위쪽일 때만 참이다. `TracePlacementZone`은 그때만 구역을 채운다.
- Project Settings 두께·조각 최대 크기는 숨은 하한 없이 그대로 읽고(getter에 `Max` 대체값 없음) 0 이하·비유한이면 `ValueInvalid` Error로 알린다(재작업 I3).
- 계단 재질이 비었거나(판·벽) 둘째 이후 계단 항목의 재질이 무시되면 `MaterialMissing` Warning이다(재작업 I1).
- 생성 component는 `CreationMethod = UserConstructionScript` + `RF_Transient`다. Engine construction 재실행이 이전 생성물을 파괴해도 shell이 `IsValid`로 걸러 다시 만든다.
- 자동화는 접근용 friend(`FBathhouseBuildingAutomationAccess`, 공간 Actor)와 배치 검증용 friend(`FBathhouseSpacePlacementAutomationTest`, `UPlayerFacilityPlacementComponent`)를 쓴다.

## Dependencies

- Building → Placement(`AFacilityPlacementZoneActor`, `IPlaceableFacility`, 배치 trace 채널), Cleaning(조각 zone class와 크기 API), NavigationSystem(`ANavMeshBoundsVolume` 검증, U2 dirty area), DeveloperSettings, Engine(ISM·PointLight·CharacterMovement CDO 읽기), UMG(U2 편집 world 미리보기 글자 `UWidgetComponent`·`UTextBlock`만, 기존 module 의존)
- (U2) Building → Facility(Authority Definition 읽기: 효과 표 줄 수, 구입 subsystem), Economy(구입 subsystem의 wallet). [ExpansionPurchaseSystem.md](ExpansionPurchaseSystem.md)
- Placement·Cleaning·Customer·Facility·Economy는 Building을 모른다. Computer는 기존 zone type으로만 참조한다. UI(확장 탭)는 구입 subsystem을 쓴다.
- 새 module은 없다.

## Verification

- 순수 layout: 단일 공간 상자 Z·XY, 모서리 겹침 없음, 개구부 분할, 통로 양쪽 같은 구간, 계단 판 구멍·경사로 양 끝·계단 벽 열림 구간, 보이는 part 상자 부피 비겹침(계단 포함), 조각 균등 분할·전체 덮음. 기대값은 같은 입력 값에서 계산한다.
- 검증: 겹침/닿음, 통로 비인접(위치 제안 값 적용 시 오류 소멸 포함), 출입구 막힘, 계단 범위 밖·경사 초과, 종류 중복·누락, 회전.
- world: BeginPlay 뒤 part별 component 수와 충돌 설정, 두 번 재생성해도 component 수 불변, `Litter` 공간은 쓰레기 조각만·`Stain` 공간은 물 얼룩 조각만·`None` 공간은 조각 없음, 조각 `SpawnFloor` Z = 바닥, 공간 Actor를 zone으로 쓴 배치 바닥 지지·벽 겹침 거부, 실제 `TracePlacementZone`에서 형상 hit와 `ZoneBounds` 아랫면 hit는 구역 없음.
- (U2·U3) 넓힘: `ExpandInterior`(여러 벽·벽별 양·중복·잘못된 양), `IsStepApplicable`, `ExpansionBandRects`(겹침 없음·넓이 합·모서리 덮음)·띠 조각, 넓힌 뒤 이웃 `BuildPlan` 불변, Validation (U2·U3) 표 코드별, 미리보기 횟수와 무관한 `ValidateWorld`, game world 효과 횟수 = 적용 횟수, 적용·되돌림 뒤 구역·shell·조각 원상, 이웃 shell component identity 유지.
