# Cleaning·Litter Editor Authoring

서비스 3단위(쓰레기·수거)에서 저장·새 프로세스 재로드로 확인한 현재 상태다. 개수·spawn·청소·carry·수거·RMB 분기·설비 배치 정리는 모두 C++ 소유이며 Blueprint graph는 없다. C++ 계약은 `.md/Architecture/CleaningLitterSystem.md`다.

## Blueprint (`/Game/Bathhouse/Blueprints/Cleaning/`)

| Asset | Parent | 현재 값 |
|---|---|---|
| `BP_Litter` | `LitterActor` | native `InteractionCollision` 12cm sphere(QueryOnly, Navigation off) 루트, `LitterMesh` 자식(NoCollision, Navigation off, scale `(0.15,0.15,0.15)`, Z 7.5). `MeshVariants` = Cylinder·Cube·Sphere, `FloorRadiusCm=15` |
| `BP_LitterSpawnZone` | `LitterSpawnZoneActor` | `SpawnBounds` extent `(300,300,100)`(NoCollision, Navigation off), `SpawnFloor` 상대 Z -100, 최대 5, `LitterSpacingOverride=0`, 높이 허용 5. Pawn 여유 property 없음 |
| `BP_LitterTongs` | `LitterTongsActor` | `WorldMesh`=Cube scale `(0.04,0.04,0.8)`(QueryAndPhysics, CCD), `HeldTransform` `(45,15,-35)`, `BagCapacity=20`, 묶기 거리 60/30, `TiedBagClass=BP_TrashBag_C` |
| `BP_TrashBag` | `TrashBagActor` | `BagMesh`=Cube scale `(0.3,0.3,0.4)`(QueryAndPhysics, CCD, 상대 위치·회전 0), `HeldTransform` `(50,15,-25)` |
| `BP_TrashCollectionZone` | `TrashCollectionZoneActor` | `CollectionBounds` extent `(150,150,100)`(NoCollision, Navigation off), `CollectionIntervalSeconds=300`, 자식 `ZoneMarker`(Plane, 상대 Z 1, scale `(3,3,1)`, 프로파일·충돌 모두 NoCollision, Navigation off, material `MI_FacilityPreview_Valid`) |
| `BP_CleaningDirector` | `CleaningDirectorActor` | `LitterClass=BP_Litter_C`, 쓰레기 평균 120초/인, 최대 20, 간격 40, update 0.25초, clearance 30. 기존 `SpawnIntervalSeconds=15`·얼룩 최대 8·`StainClass` 유지. `DefaultPawnClearance` 제거 |
| `BP_WaterStain` | `WaterStainActor` | `FloorRadiusCm=37.5`(시각 Plane 100cm×0.75의 반경; 런타임 = 이 값 × 최대 XY scale 1.3). material·scale/yaw 범위·청소 2초 유지 |
| `BP_StainSpawnZone` | `StainSpawnZoneActor` | `SelectionWeight`·`ZoneKind`·`PawnClearanceOverride` 제거 후 재저장. 기존 최대 4·경사 10·trace 유지 |

모든 Blueprint는 `warnings_as_errors` Compile·Data Validation(VALID)·새 프로세스 재로드를 통과했다. 충돌은 `bodyInstance.collisionProfileName`을 `NoCollision`으로 **함께** 지정해야 로드 후에도 유지된다(충돌 값만 바꾸면 로드 때 프로파일이 덮는다).

## DefaultMap instance (World Partition external actor)

바닥은 공간 바닥 생성 형상(Mobility Static, `ECC_WorldStatic`, [BuildingSystem.md](BuildingSystem.md))이고 출입구 밖 마당은 지형(world Z=0)이다. 이전 기록의 `Studio_floor`는 mesh가 없는 빈 actor였고 삭제됐다.

쓰레기·물 얼룩 생성 구역은 Level에 직접 두지 않는다. 공간 Actor가 BeginPlay에 `CleaningChunkKind`(홀 쓰레기, 목욕공간 물 얼룩, 작업공간 없음)에 맞는 조각을 spawn한다(class는 Project Settings Bathhouse Building). 이전 `LitterSpawnZone_Dressing`, `BathCleaningZone`, `DressingCleaningZone` instance는 삭제됐다(`EXP-U1`).

| Actor | 위치/값 | external package |
|---|---|---|
| `BP_CleaningDirector_C_UAID_…1654552567` | 기존 instance, `SpawnIntervalSeconds=15` override 유지, `LitterClass`는 BP 상속. 위치 무관 | `…/3/O2/M90TW0CJ9DL2TVTK3X0IZH` |
| `LitterTongs` (`BP_LitterTongs`) | 홀 안(걸레 거치대 옆), 위치는 Level instance 정본 | `…/E/P0/9P6KFKB6KF1F4ILSN6P4PY` |
| `LitterTongsSlot` (`BP_PhysicalCarryFixedSlot`) | 집게와 같은 위치, `AssignedItem`=위 집게, `bStartOccupied=true`, `SlotDisplayName=집게 보관대`, `ItemAnchor` 상대 0 | `…/9/XK/X7UUCTLXTSCHPTZJCUD5EY` |
| `TrashCollectionZone` (`BP_TrashCollectionZone`) | 홀 서쪽 출입구 밖 마당(지형 위), Nav 범위 안 | `…/6/G0/ZUFRUBF564PZTSW1MN4J8A` |

기존 `BP_TrashBin` level instance는 제거됐다. `DefaultMap.umap`은 저장하지 않았다. `BP_TrashBag` instance는 레벨에 두지 않는다(수거 시 C++가 spawn).

저장 경로: MCP `save_actor`는 external package에서 `Asset does not exist` 오류로 실패해 Python API(`EditorLoadingAndSavingUtils.save_packages`)를 사용한다. 스크립트·백업: `Saved/Claude/Trash/`, `Saved/Claude/EXP-U1/`, `Saved/MigrationBackup/20261002_EXP-U1/`.
