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

바닥은 `Studio_floor`(Mobility Static, `ECC_WorldStatic`)이며 모든 지점에서 상단이 world Z=0이다.

| Actor | 위치/값 | external package |
|---|---|---|
| `BP_CleaningDirector_C_UAID_…1654552567` | 기존 instance, `SpawnIntervalSeconds=15` override 유지, `LitterClass`는 BP 상속 | `…/3/O2/M90TW0CJ9DL2TVTK3X0IZH` |
| 기존 `BP_StainSpawnZone` 두 개 (`(2050,-300,0)`, `(1000,-700,0)`) | `SpawnFloor` 상대 Z -100 → 0(world 0) | `…/6/FX/6ZGWI7FB2UN1ZMP6Q1YON1`, `…/8/Q0/Y16QBOVVFUE29TAO7F2Z2S` |
| `LitterSpawnZone_Dressing` (`BP_LitterSpawnZone`) | `(1050,-280,100)`, `SpawnFloor` world 0 | `…/0/TK/V08KR6A5F481MDJDVTC74B` |
| `LitterTongs` (`BP_LitterTongs`) | `(650,750,43)` (걸레 거치대 옆) | `…/E/P0/9P6KFKB6KF1F4ILSN6P4PY` |
| `LitterTongsSlot` (`BP_PhysicalCarryFixedSlot`) | 집게와 같은 위치, `AssignedItem`=위 집게, `bStartOccupied=true`, `SlotDisplayName=집게 보관대`, `ItemAnchor` 상대 0 | `…/9/XK/X7UUCTLXTSCHPTZJCUD5EY` |
| `TrashCollectionZone` (`BP_TrashCollectionZone`) | `(-800,450,0)` (출입구 밖) | `…/6/G0/ZUFRUBF564PZTSW1MN4J8A` |

기존 `BP_TrashBin` level instance(`(-76,-717,0)`, package `…/5/OS/BNP2Y7NJ91BPOXIJVR7FXP`)는 제거됐고 파일이 삭제됐다. BP·Source는 유지. `DefaultMap.umap`은 저장하지 않았다. `BP_TrashBag` instance는 레벨에 두지 않는다(수거 시 C++가 spawn).

저장 경로: MCP `save_actor`는 이 external package에서 `Asset does not exist` 오류로 실패해(동일 오류 재확인, 재시도 안 함) Python API(`EditorLoadingAndSavingUtils.save_packages`)를 사용했다. 스크립트·백업: `Saved/Claude/Trash/`, `Saved/MigrationBackup/20261001_service_unit3/`.
