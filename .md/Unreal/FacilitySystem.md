# Facility Editor Authoring

## 설비 Blueprint

| Asset | Parent Class | 역할 |
|---|---|---|
| `/Game/Bathhouse/Blueprints/Facility/BP_Bath` | `/Script/BathhouseSim.BathhouseBathFacilityActor` | Bath 시설, 3개 Facility Slot과 물 표현 composition root |
| `/Game/Bathhouse/Blueprints/Facility/BP_Shower` | `/Script/BathhouseSim.BathhouseFacilityActor` | Shower 시설과 2개 Facility Slot, 비품 manager·router·공간 2개([ServiceSystem.md](ServiceSystem.md)) |
| `/Game/Bathhouse/Blueprints/Service/BP_MassageChair` | `/Script/BathhouseSim.MassageChairActor` | 안마의자(slot 1, 고장 표시 graph, [ServiceSystem.md](ServiceSystem.md)) |
| `/Game/Bathhouse/Blueprints/Service/BP_RestBench` | `/Script/BathhouseSim.BathhouseFacilityActor` | 평상(`SeatSlot0~2`, FacilityType RestBench) |
| `/Game/Bathhouse/Blueprints/Service/BP_Television` | `/Script/BathhouseSim.TelevisionActor` | TV(slot 0, 화면 표현 graph) |
| `/Game/Bathhouse/Blueprints/Service/BP_ScrubTable` | `/Script/BathhouseSim.ScrubTableActor` | 세신대(slot 1, native 세신 component 6개) |
| `/Game/Bathhouse/Blueprints/Service/BP_Vanity` | `/Script/BathhouseSim.BathhouseFacilityActor` | 화장대(비품 진열 4그룹, Facility Slot 1개, [ServiceSystem.md](ServiceSystem.md)) |
| `/Game/Bathhouse/Blueprints/Facility/BP_ClothesLocker` | `/Script/BathhouseSim.BathhouseFacilityActor` | 1칸 Clothes Locker |
| `/Game/Bathhouse/Blueprints/Facility/BP_ClothesLocker_4` | `/Script/BathhouseSim.BathhouseFacilityActor` | 4칸 Clothes Locker |
| `/Game/Bathhouse/Blueprints/Facility/BP_ClothesLocker_8` | `/Script/BathhouseSim.BathhouseFacilityActor` | 8칸 Clothes Locker |
| `/Game/Bathhouse/Blueprints/Towel/BP_Washer` | `/Script/BathhouseSim.TowelProcessingMachineActor` | Washer, native 뚜껑(`LidPivot`/`LidMesh`/`LidPresentation`)·`DisplayCue`, 값은 ServiceSystem.md |
| `/Game/Bathhouse/Blueprints/Towel/BP_Dryer` | `/Script/BathhouseSim.TowelProcessingMachineActor` | Dryer, 뚜껑·`DisplayCue` 동일 |
| `/Game/Bathhouse/Blueprints/Towel/BP_CleanTowelStack` | `/Script/BathhouseSim.CleanTowelStackActor` | placement opt-out 수건 공급대 |
| `/Game/Bathhouse/Blueprints/Towel/BP_UsedTowelBin` | `/Script/BathhouseSim.UsedTowelBinActor` | placement opt-out 사용 수건함 |

설비 Definition의 종류 태그(`Facility.Type.*`)와 공간별 허용 표는 [BuildingSystem.md](BuildingSystem.md), DefaultMap의 공간별 설비 위치는 [WorldSystem.md](WorldSystem.md)에 있다.

설비 Blueprint 9개는 삭제된 inherited `PlacementNavModifier`를 보유하지 않는다. 배치 대상 7개의 body Static Mesh만 collision과 Navigation을 담당하고, `PlacementFootprint`, `PackagePhysicalRoot`, slot, water/contents/presentation helper는 Navigation 비활성이다. 상세 extent와 body 설정은 [PlacementSystem.md](PlacementSystem.md)에 있고, `BP_Bath`의 물·조작부 표현은 [BathWaterSystem.md](BathWaterSystem.md)가 정본이다.

## Clothes Locker authoring

- `BP_ClothesLocker`, `BP_ClothesLocker_4`, `BP_ClothesLocker_8`의 Definition `LockerSlotCount`는 각각 1, 4, 8이다.
- 4칸 Blueprint는 `LockerSlot01~04`, 8칸 Blueprint는 `LockerSlot01~08`을 가진다.
- Level에 선배치된 1칸 Locker 두 개(홀)의 저장 ID는 유효하고 서로 다르다. 이전 디스크 package에는 `RegistrationId`가 저장돼 있지 않아 로드마다 native가 새로 만들었고, `EXP-U1` 이동 저장 때 그 생성값이 처음 디스크에 저장됐다(새 프로세스 재로드 확인).
  - `BP_ClothesLocker_C_UAID_F02F7433CA3615F402_1440894859`(`ClothesLocker_1`): `F3598070-4FAAA372-1FDC72AE-266D85D7`
  - `BP_ClothesLocker_C_UAID_F02F7433CA3615F402_1441212860`(`ClothesLocker_2`): `906CB953-4713224C-A9D6A9AC-CE3ACA7A`
- 새로 배치하거나 Editor에서 복제한 Locker는 native `RegistrationId` 생성 계약을 따른다. PIE duplicate는 저장된 ID를 바꾸지 않는다.

## 확장 정의와 관리자

- `/Game/Bathhouse/Data/Expansion/DA_BathhouseExpansion_Default`(`UBathhouseExpansionDefinition`): `Max Purchase Count`(전체 구입 횟수 상한), `Purchase Prices`(줄 k = k+1번째 구입 가격, 상한과 같은 줄 수), `Tiers`(줄 = 홀 넓힘 횟수, 열쇠 수·락커 칸 한도, 상한 + 1줄). 값의 정본은 이 asset이다. `ValidatePurchaseData`와 Data Validation 오류 0(`EXP-U2` 저장·새 프로세스 재로드).
- DefaultMap `ExpansionAuthority`(`BP_BathhouseExpansionAuthority_C`, package `/Game/__ExternalActors__/Maps/DefaultMap/1/T8/BSY81Z2446YMM4DNR4DC1B`): `Expansion Definition` = 위 asset, `Initial Tier Index` 0. Data Validation 오류 0(확인만, 저장하지 않음).
- DefaultMap `KeyRack`(`BP_BathhouseKeyRack_C`, package `/Game/__ExternalActors__/Maps/DefaultMap/8/3G/CDTHV2ALC56NYO6HJ2VG2Q`): `Pair Transforms` 개수가 도달 가능한 `Tiers` 줄의 최대 열쇠 수 이상이다. Data Validation 오류 0(확인만).
- `DA_FacilityPlacement_ClothesLocker_1`은 상점 상품이다([ShopSystem.md](ShopSystem.md)). Data Validation 경고 1개(`Recovery Item Mesh` 미지정, Engine Cube 대체)는 기존 상태다.

현재 저장 상태를 불러온 깨끗한 PIE 2회에서 duplicate ID, expansion limit, capacity 중복 합산 또는 startup locker 오류가 발생하지 않았다. 개별 Level actor Data Validation과 World Partition 저장이 필요한 항목은 [USER_UNREAL.md](../USER_UNREAL.md)에 남아 있다.
