# World Editor Authoring

## DefaultMap

- Level: `/Game/Maps/DefaultMap`
- World Partition Level이다.
- Placement Zone은 한 개다.
  - Actor: `/Game/Maps/DefaultMap.DefaultMap:PersistentLevel.BP_FacilityPlacementZone_C_UAID_F02F7433CA36D1FF02_1155169559`
  - Transform: Location `(600,-100,0)`, Rotation `(0,0,0)`, Scale `(1,1,1)`
  - `ZoneBounds` Extent: `(1400,900,10)`
  - `AllowedFacilityTags`: `Facility.Placeable`
  - `PlacementFloor` world plane: `Z=0`
- 선배치 Clothes Locker 두 개는 [FacilitySystem.md](FacilitySystem.md)의 고유 `RegistrationId`를 가진다.

## Navigation

- Recast actor: `/Game/Maps/DefaultMap.DefaultMap:PersistentLevel.RecastNavMesh_UAID_F02F7433CA3615F402-Default`
- 현재 디스크에 저장된 `RuntimeGeneration`은 `DynamicModifiersOnly`다.
- 설비 Blueprint의 body Static Mesh Navigation 활성과 helper Navigation 비활성은 저장돼 기존 Level instance에도 반영된다.
- 삭제된 `PlacementNavModifier`는 조사한 Bath 2개, Clothes Locker 2개, Washer 1개, Dryer 1개 external actor instance에 존재하지 않는다.

현재 MCP의 World Partition actor 저장 경로가 external actor package를 저장하지 못하므로, Recast를 `Dynamic`으로 바꾸는 작업은 [USER_UNREAL.md](../USER_UNREAL.md)에 남아 있다.

쓰레기·청소용 DefaultMap instance(쓰레기 구역·집게·거치대·수거 구역, stain zone/director 재저장)와 바닥(world Z=0) 조사 결과는 [CleaningSystem.md](CleaningSystem.md)에 있다.

세신용 DefaultMap instance: `ScrubTowel`(`BP_ScrubTowel`)과 전용 `ScrubTowelSlot`(`BP_PhysicalCarryFixedSlot`)이 `(300,900,43)`에 있다(배치 구역 y≤800 밖, 벽 y=1025과 이격, 다른 거치대와 겹치지 않음). 거치대의 `AssignedItem`은 그 때수건 instance, `bStartOccupied=true`, `SlotDisplayName=때수건 거치대`, `ItemAnchor` 상대 0이다. external package: 때수건 `/Game/__ExternalActors__/Maps/DefaultMap/0/DK/0190JIWJICUQIWS8KMXOSL`, 거치대 `…/E/3R/BVIBVCNJXTNW9C0FEGBWZL`(Python `save_packages`로 저장; `DefaultMap.umap`·기존 actor는 저장하지 않음). 테스트 설비·인형·현금은 레벨에 저장하지 않는다.
