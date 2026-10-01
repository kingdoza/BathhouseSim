# Building System

## Status And Scope

- 2026-10-02 `EXP-U1` 설계(공간 건물). Source 미반영, 사용자 승인 대기. 입력은 `.md/Work/EXPANSION-PURCHASE/PROMPT_ARCHITECTURE.md`(EXP-001~015)이고 구현 지시는 `.md/Work/EXPANSION-PURCHASE/EXP-U1/PROMPT_IMPLEMENTATION.md`다.
- 가게를 홀·목욕공간·작업공간(지하) 세 공간으로 나누고, 공간마다 직사각형 바닥 경계를 따라 벽·바닥·천장·조명·출입구·통로·계단을 만든다. 공간은 그 바닥의 설비 배치 구역이자 쓰레기·물 얼룩 생성 조각의 생성자다.
- 비대상(U1): 확장 구입·넓힘 적용, 확장 탭, 열쇠·한도 변경, 락커 판매. 확장 지점은 아래 U2 Extension Points에만 적는다.

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
  BathhouseSpaceValidation.h/.cpp  layout 규칙 검사 + world 검사(Nav 범위, 계단 통로 장애물, 설비 소속)
  BathhouseSpaceEditorSync.h/.cpp  WITH_EDITOR: 편집 world의 공간 형상 지연 일괄 재생성
  BathhouseCleaningChunkSpawner.h/.cpp  runtime 생성 조각 Actor spawn
Private/Tests/
  BathhouseBuildingAutomationTests.cpp
```

`Building`은 새 Source 하위 폴더다([CoreSystem.md](CoreSystem.md) System Documents).

## Ownership

| 대상 | 상태·authoring owner | 실행 owner |
|---|---|---|
| 공간 종류, 바닥 크기·위치, 천장 높이, 재질, 조명, 개구부, 계단, 허용 설비 | 각 `ABathhouseSpaceActor` Level instance | 공간 Actor |
| 벽 두께, 바닥·천장 두께, 형상용 상자 mesh, 생성 조각 최대 크기·class | `UBathhouseBuildingSettings`(Project Settings) | 읽기 전용 |
| 공간별 형상 계획(벽·바닥·천장·계단 상자, 조각 직사각형) | 파생값, 저장하지 않음 | `FBathhouseSpaceLayout`(순수) |
| 생성 component(ISM·조명·편집용 조각 미리보기) | `UBathhouseSpaceShellComponent`(Transient 목록) | shell component |
| 생성 조각 Actor | 공간 Actor(Transient weak 목록) | `FBathhouseCleaningChunkSpawner` |
| 설비 배치 구역 bounds·바닥·grid | 공간 Actor가 상속한 `AFacilityPlacementZoneActor` | [PlacementSystem.md](PlacementSystem.md) |
| 조각 안 쓰레기·물 얼룩 생성 | 조각 Actor와 `ACleaningDirectorActor` | [CleaningLitterSystem.md](CleaningLitterSystem.md) |
| 손님 길 | Level `NavMeshBoundsVolume` + Recast `Dynamic` 재생성 | UE Navigation |

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
| (상속) `AllowedFacilityTags` | 이 공간에 놓을 수 있는 설비 종류 태그(`Facility.Type.*`) |

- `EBathhouseSpaceSide`: `East`(+X), `West`(−X), `North`(+Y), `South`(−Y). world 축 기준이다.
- `CenterOffsetCm`: 개구부 중심의 벽 길이 방향 좌표를 공간 Actor 위치 기준으로 잰 값(동·서 벽은 Y, 남·북 벽은 X). Actor 위치는 넓힘에도 고정되므로 벽이 물러나도 개구부가 벽을 따라 미끄러지지 않는다.
- `TopEdgeCenterOffsetCm`: 위층 공간 Actor 위치 → 계단 맨 위 가장자리 중심 XY. `DownSide`가 내려가는 방향이다.
- 통로는 한쪽 공간의 `Openings`에만 적는다(`ConnectedSpace` 지정). 맞닿은 공간의 벽도 같은 world 구간으로 뚫린다. 계단은 위층 공간(홀)의 `Stairs`에 적는다.

## Geometry Rules

좌표는 모두 world, 단위 cm. `t` = `WallThicknessCm`, `s` = `SlabThicknessCm`. 안쪽 직사각형 `I`, 바깥 직사각형 `O` = `I`를 사방 `t`만큼 넓힌 것, 바닥 윗면 `Zf`, 천장 아랫면 `Zc = Zf + CeilingHeightCm`.

- 바닥: `O`에서 이 공간을 위층으로 하는 계단 구멍을 뺀 직사각형들, Z `[Zf − s, Zf]`.
- 천장: `O`에서 이 공간을 아래층으로 하는 계단 구멍을 뺀 직사각형들, Z `[Zc, Zc + s]`.
- 벽: 동·서 벽은 Y로 `O` 전체 길이, 남·북 벽은 X로 `I` 길이(모서리 겹침 없음). 두께 `t`는 `I` 밖으로, Z `[Zf, Zc]`.
- 개구부: 벽 길이 구간 `[c − w/2, c + w/2]` × Z `[Zf, Zf + h]`를 벽에서 뺀다. 나머지는 개구부 사이 전체 높이 조각 + 개구부 위 인방 조각이다. 통로는 `ConnectedSpace` 벽에서도 같은 world 구간을 뺀다.
- 맞닿은 두 공간은 바깥 직사각형 변이 정확히 닿는다. 그래서 두 벽은 등을 맞대고, 바닥·천장은 변끼리 붙어 겹치지 않는다.
- 직사각형 빼기는 guillotine 분할로 겹치지 않는 직사각형 목록을 만든다.

계단(위층 `U`, 아래층 `L`, 높이 `H = Zf(U) − Zf(L)`). 계단 frame 원점 = 위층 Actor 위치 + `TopEdgeCenterOffsetCm`(Z = `Zf(U)`), +x = `DownSide`, y = 폭 방향.

- 구멍 `R` = x `[0, RunCm]` × y `[−W/2, W/2]`. `U` 바닥과 `L` 천장에서 뺀다.
- 경사로(`StairRamp`): 윗면이 (0, `Zf(U)`)와 (`RunCm`, `Zf(L)`)를 잇는 두께 `s` 회전 상자. 보이지 않고 충돌만 한다. 실제 걷는 면이다.
- 계단 판(`StairStep`): `StepCount`개 시각 상자, 충돌 없음. 각 판 윗면은 경사로와 한 단 높이 이내다.
- 계단 벽(`StairWall`, 두께 `t`, `R` 바깥): 양옆 Z `[Zf(L), Zf(U) + GuardHeightCm]`, 위쪽 끝(x=0 바깥) Z `[Zf(L), Zf(U)]`(위층 쪽은 입구로 열림), 아래쪽 끝(x=Run 바깥) Z `[Zc(L), Zf(U) + GuardHeightCm]`(아래층 출구로 열림).
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
- "배치 trace 채널 Block"은 벽 너머 다른 공간의 배치 구역을 조준하지 못하게 한다.
- 생성 component와 조명은 `RF_Transient`이며 저장하지 않는다. 재생성은 항상 전부 파괴 후 새로 만든다(Static component를 옮기지 않음).

## Lifecycle

- 형상 계획은 `FBathhouseSpaceLayout::Build(Snapshots, Values)` 하나로 계산한다. snapshot은 같은 world의 모든 공간 Actor authored 값만 읽는다(다른 공간의 생성 component는 읽지 않음). 그래서 로드·BeginPlay 순서와 무관하게 결과가 같다.
- `OnConstruction`: `ZoneBounds` 갱신 → `Super::OnConstruction`(grid) → 자기 shell 재생성 → editor world면 `FBathhouseSpaceEditorSync::RequestRebuild(World)`.
- `FBathhouseSpaceEditorSync`(WITH_EDITOR): world별로 다음 tick 한 번 모든 공간의 shell만 재생성한다(coalesce, `FTSTicker`). OnConstruction을 다시 부르지 않아 재귀가 없다. 로드 순서, 다른 공간 편집, Undo 뒤에도 통로·계단 구멍이 맞는다. transaction·Modify를 하지 않는다(생성물은 Transient).
- `BeginPlay`: `ZoneBounds` 갱신 → shell 재생성 → 손님 공간(Hall·Bath)이면 생성 조각 spawn → 이 공간이 owner인 검증 오류를 `LogBathhouseBuilding` Error로 한 번 기록. runtime 정본은 BeginPlay 재생성이다(PIE 복제·cook 직렬화에 의존하지 않음).
- `EndPlay`: 생성 조각 Actor 파괴.
- 검증 오류가 있어도 계산 가능한 형상은 만든다. Rotation·Scale 위반과 Settings 상자 mesh 누락만 형상 생략이다.

## Cleaning Chunks

- `I`를 Settings `CleaningChunkMaxSizeCm` 이하의 같은 크기 칸으로 나눈다(축마다 `ceil(크기/최대)`개, 칸 = 크기/개수). U2의 넓힘 띠는 따로 같은 규칙으로 나눠 추가한다(기존 조각 불변).
- 칸마다 Settings `LitterChunkZoneClass`, `StainChunkZoneClass`를 하나씩 deferred spawn → `SetSpawnAreaHalfSizeXY(칸/2)` → `SpawnFloor` world Z가 `Zf`가 되게 Actor Z 보정 → `FinishSpawning`. 조각별 최대 개수는 class 기본값이다.
- Work 공간은 조각이 없다. 편집 화면에서는 같은 칸을 editor-only 선 상자(게임에서 없음)로 보여 준다.

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
| 손님 공간(Hall·Bath)에 바깥 출입구가 없음 / Work로 내려가는 계단이 없음 | Error |
| 계단: `LowerSpace` 유효·다름·더 낮음, 폭·길이·판 수 양수, 계단 벽까지 포함한 발자국이 두 공간 안쪽 안, 위 입구 앞과 아래 출구 앞에 계단 폭만큼의 빈 바닥, 경사 ≤ `UCharacterMovementComponent` 기본 `GetWalkableFloorAngle()` | Error |
| 계단 통로(구멍 안, `Zc(L)+s`~`Zf(U)−s`)를 공간이 아닌 blocking 물체(지형 등)가 막음 | Error(편집 world trace) |
| 손님 공간 바닥 직사각형이 어떤 `NavMeshBoundsVolume`에 들지 않음, 바깥 출입구 앞 바깥 지점(개구부 폭만큼 떨어진 곳)이 Nav 범위 밖, Work 바닥이 Nav 범위 안 | Error |
| Settings 상자 mesh·조각 class 없음, 공간 재질 없음 | Error / Warning |
| 배치된 `IPlaceableFacility`가 어느 공간에도 없거나 그 공간이 허용하지 않는 종류 | Warning |

부동소수 비교의 허용 오차만 엔진 상수(`UE_KINDA_SMALL_NUMBER`)를 쓴다.

## Navigation

- 손님 길은 Level `NavMeshBoundsVolume`이 넓게(홀·목욕공간과 앞으로의 넓힘, 출입구 밖 마당 경로 포함) 덮고, Z 범위는 지상만 덮어 지하 바닥을 넣지 않는다. 공간은 Nav 범위를 움직이지 않는다.
- Recast `RuntimeGeneration = Dynamic`이 생성 형상의 등록·파괴를 반영한다. 계단 판·경사로·천장은 Nav 비관련이라 계단과 구멍 위에는 Nav가 없다. 손님은 지하에 갈 길이 없다.
- 범위가 넓힘을 다 덮는지는 위 Validation이 알린다.

## Blueprint/API Contracts

- 신규 reflected: 위 class·enum·struct와 property·component 이름(`SpaceRoot`, `Shell`), `UBathhouseBuildingSettings` 값.
- Blueprint `BP_BathhouseSpace`(parent `ABathhouseSpaceActor`)는 상속 `GridVisual`에 Plane과 `MI_FacilityPlacementGrid`만 지정한다. 형상·조명·조각을 Blueprint graph로 만들지 않는다.
- 공간 Actor는 C++ public으로 `GetSpaceKind()`, `GetInteriorRect()`(현재 안쪽 바닥 world XY), `GetFloorZ()`, `GetCeilingZ()`를 제공한다.
- 기존 class 이름 변경·삭제가 없어 Core Redirect가 필요 없다.
- 지형: 계단 통로가 지형을 지나므로 Level 지형에 구멍이 필요하다(편집 world 검증이 알림). Editor 정본은 `.md/Unreal/`이 기록한다.

## U2 Extension Points (U1 구현 금지)

- 공간 Actor에 넓힘 목록과 현재 넓힘 횟수가 생기면 `GetInteriorRect()`와 `ZoneBounds` 상대 위치가 바뀌고 같은 shell 재생성·조각 추가 경로를 쓴다.
- 확장 구입 상태 owner, 가격·상한·홀 효과 표는 U2 설계가 정한다(예정 원본은 `PROMPT_IMPLEMENTATION.md` 0절).

## Dependencies

- Building → Placement(`AFacilityPlacementZoneActor`, `IPlaceableFacility`, 배치 trace 채널), Cleaning(조각 zone class와 크기 API), NavigationSystem(`ANavMeshBoundsVolume` 검증), DeveloperSettings, Engine(ISM·PointLight·CharacterMovement CDO 읽기)
- Placement·Cleaning·Customer는 Building을 모른다. Computer는 기존 zone type으로만 참조한다.
- 새 module은 없다.

## Verification

- 순수 layout: 단일 공간 상자 Z·XY, 모서리 겹침 없음, 개구부 분할, 통로 양쪽 같은 구간, 계단 구멍·경사로 양 끝·계단 벽 열림 구간, 조각 균등 분할·전체 덮음. 기대값은 같은 입력 값에서 계산한다.
- 검증: 겹침/닿음, 통로 비인접, 출입구 막힘, 계단 범위 밖·경사 초과, 종류 중복·누락, 회전.
- world: BeginPlay 뒤 part별 component 수와 충돌 설정, 두 번 재생성해도 component 수 불변, Work 조각 없음, 조각 `SpawnFloor` Z = 바닥, 공간 Actor를 zone으로 쓴 배치 바닥 지지·벽 겹침 거부.
