# World Editor Authoring

## DefaultMap

- Level: `/Game/Maps/DefaultMap`
- World Partition Level이다. Level actor는 external actor package로 저장한다. 검증된 저장 경로는 Python `EditorLoadingAndSavingUtils.save_packages`다(MCP `save_actor`는 external package에서 `Asset does not exist`로 실패한 기록이 있다). `DefaultMap.umap`은 `EXP-U1`에서 저장하지 않았다.
- 공간·배치 구역은 공간 Actor 3개(`Space_Hall`, `Space_Bath`, `Space_Work`)다. 이전 단일 `BP_FacilityPlacementZone` instance는 삭제됐다. 구조와 값 원본은 [BuildingSystem.md](BuildingSystem.md)에 있다.
- 바닥은 공간 바닥 생성 형상(지상 두 공간은 지형보다 위, 작업공간은 지하)이고, 출입구 밖 마당 바닥은 지형(world Z=0 평면)이다.
- 설비·물건 배치(`EXP-U1` 이동 결과, 위치는 각 Level instance가 정본):
  - 홀: `Counter`, `Computer`, `KeyRack`, `BP_ShopDeliveryPoint`, `BP_DrinkCollectionBox`, `ShoeLocker_1/2`, `ClothesLocker_1/2`, `MonkeyWrenchFixedSlot`+`BP_MonkeyWrench`, `WetMopFixedSlot`+`WetMop`, `LitterTongsSlot`+`LitterTongs`, `PlayerStart`(카운터 근처), 장식 `SM_Fridge`·`cleaner`, `CustomerQueueOverflowWanderVolume_Checkout_0`(카운터 옆, 크기·카운터 기준 상대 위치 유지)
  - 목욕공간: `Shower`, `Bath`, `Bath2`, `CleanTowelStack`, `UsedTowelBin`, `ScrubTowelSlot`+`ScrubTowel`, 장식 `boiler`(위치 미변경, 아래 주의)
  - 작업공간(지하): `BP_Circulator`, `BP_Cooler`, `BP_Boiler`, `CoalSupply`, `BP_DryIceSupply`, `ShovelSlot`+`BP_UtilityShovel`, `Washer`, `Dryer`, `DryingSpot`, `TowelBasketFixedSlot`+`TowelBasketCart`
  - 출입구 밖 마당(홀 서쪽): `Spawner`, `Exit`, `TrashCollectionZone`
  - 고정 거치대의 `AssignedItem`과 `ItemAnchor` 상대 0은 이동 뒤에도 유지된다.
- 삭제된 Level actor(Blueprint asset은 유지): `PlacementZone`, `LitterSpawnZone_Dressing`, `BathCleaningZone`, `DressingCleaningZone`, 북쪽 임시 벽 `Wall`, 장식 `SM_Bath_old`·`SM_Bath_01_Body`·`Plane`·`SM_Kitchen_Countertop_C/C2/C3/C4`, 빈 actor `Studio_floor`·`Cooler_Lid`·`Cooler_Needle`·`Cooler_Needle_Mesh`·`RootNode`.
- 주의: 장식 `boiler` StaticMeshActor(`/Game/Bathhouse/Meshes/boiler`)는 목욕공간 안 world Z=0에 그대로 있어 바닥 판과 겹친다(아래쪽이 바닥에 묻힘). 처리 방법은 사용자 결정 대기다(`EXP-U1` 보고).
- 선배치 Clothes Locker 두 개의 `RegistrationId`는 [FacilitySystem.md](FacilitySystem.md)에 있다.

## Navigation

- Recast actor: `/Game/Maps/DefaultMap.DefaultMap:PersistentLevel.RecastNavMesh_UAID_F02F7433CA3615F402-Default`(package `/Game/__ExternalActors__/Maps/DefaultMap/9/GO/03SG1X02WXVMF5RN4NTIOA`). 디스크에 저장된 `RuntimeGeneration`은 `Dynamic`이다(새 프로세스 재로드 확인).
- `NavMeshBounds`(`NavMeshBoundsVolume`, package `/Game/__ExternalActors__/Maps/DefaultMap/8/8U/DVJA0LL4M6BXDCMLJ35BD5`) 계약: 한 volume이 홀·목욕공간 바닥 전체, 앞으로의 넓힘 여유, 홀 서쪽 출입구 밖 마당(`Spawner`·`Exit`·`TrashCollectionZone`)을 덮고, Z 범위는 지상(마당 지형과 지상 바닥)만 덮어 작업공간 바닥을 넣지 않는다. 범위 값의 정본은 Level instance transform이며 공간 Data Validation이 계약을 검사한다.
- 편집 world 확인: 손님 생성기에서 카운터·락커·목욕공간까지 경로가 있고 작업공간 바닥과 계단에는 Nav가 없다.
- 설비 Blueprint의 body Static Mesh Navigation 활성과 helper Navigation 비활성은 저장돼 기존 Level instance에도 반영된다.

쓰레기·청소용 DefaultMap instance는 [CleaningSystem.md](CleaningSystem.md)에 있다. 세신용 `ScrubTowel`(`BP_ScrubTowel`, package `/Game/__ExternalActors__/Maps/DefaultMap/0/DK/0190JIWJICUQIWS8KMXOSL`)과 전용 `ScrubTowelSlot`(`BP_PhysicalCarryFixedSlot`, `…/E/3R/BVIBVCNJXTNW9C0FEGBWZL`)은 목욕공간에 있고 거치대의 `AssignedItem`은 그 때수건, `bStartOccupied=true`, `SlotDisplayName=때수건 거치대`다. 테스트 설비·인형·현금은 레벨에 저장하지 않는다.
