# 모델링 대상 스태틱 메시 목록

- 작성일: 2026-10-02
- 기준 커밋: `a59628b`(브랜치 `work/EXP-U1`). 조사 시점 Content에는 미커밋 장식 `boiler` Z 이동만 더해져 있다.
- 조사 방법:
  1. 작업용 Editor에서 읽기 전용 Python으로 수집했다. 대상은 DefaultMap actor의 StaticMeshComponent·ISM, `/Game/Bathhouse`·FirstPerson Blueprint의 component template(SubobjectDataSubsystem), CDO·DataAsset의 mesh 속성이다.
  2. 모든 StaticMesh asset의 referencer(AssetRegistry)를 대조하고, C++ 기본 mesh(`ConstructorHelpers`·`LoadObject`)와 Config를 grep했다.
  3. 결과는 `Saved/Claude/EXP-U1/41_mesh_probe.json`·`42_mesh_extra.py` 출력이다. 수치는 조사 시점 기록이며 정본이 아니다.

외부 팩(StylizedKitchen 등)·엔진 BasicShapes·이미 import한 mesh도 모두 임시 mesh로 본다. 크기는 게임 안 실제 크기(mesh bounds × component/actor scale)를 W(X)×D(Y)×H(Z) cm로 적는다. "바닥 pivot"은 actor 원점이 물건 바닥 중심이라는 뜻이다. 엔진 Cube·Cylinder·Sphere는 중심 pivot 100cm, Plane은 100×100cm, `SM_Facility_sample`은 바닥 pivot 100cm 상자다.

## 공통 교체 조건

- 대부분 Blueprint는 component scale(일부는 actor/root scale)로 임시 상자 크기를 맞췄다. 전용 mesh는 scale 1에서 맞는 크기로 만들고, 교체 때 해당 component·root scale을 1로 되돌린다.
  - root scale로 크기를 낸 actor: 손에 드는 물건 다수(아래 비고 "root scale"). 이들은 자식 component·anchor 위치도 함께 다시 확인해야 한다.
- 설비 본체 mesh는 충돌과 Navigation을 담당한다(BlockAllDynamic, Nav 활성). footprint(`PlacementFootprint` extent)와 바닥 pivot(local Z=0)을 지켜야 배치·회수가 맞는다. 근거: `.md/Unreal/PlacementSystem.md` footprint 표.
- 움직이는 부품(문짝·뚜껑·레버·밸브·바늘)은 별도 mesh다. pivot은 회전축(경첩) 위치에 둔다. component 이름과 native 역할은 유지한다.
- 손에 드는 물건(물리 운반)은 root mesh가 물리·충돌(QueryAndPhysics, CCD)을 맡는다. runtime에 simulate physics를 켜는 C++: LitterTongs, TrashBag, WetMop, MonkeyWrench, Key, ItemBox, ScrubTowel, ShopDeliveryBox, TowelBasket, UtilityShovel, PlaceableFacilityItem. 단순 convex 충돌이 필요하다.
- slot·service point·display space 위치는 component로 따로 있다. mesh가 그 위치(앉는 면·서는 자리·진열 칸)와 맞아야 한다.

## 목록

| 분류 | 물건 | 현재 임시 mesh | 사용처 | 현재 크기·pivot | 코드·asset 기대 조건 | 비고 |
|---|---|---|---|---|---|---|
| 설비 | 욕탕 | `/Game/Bathhouse/Meshes/Bath/SM_Bath_old`(convex 1, 재질 1) | `BP_Bath.FacilityVisual`(scale 3), Level `Bath`·`Bath2` | 약 281×238×77, 바닥 pivot | footprint 290×240×76(extent (145,120,38)). 손님 slot A/B/C가 Y −60/0/60, Z 2.5(탕 안). 수면 `WaterSurfaceMover`가 `WaterLevelEmptyPoint`(Z 3)~`FullPoint`(Z 70) 사이를 오르내림. 물 흐름 Niagara 노즐 `(−55,80,68)` | 수면은 아래 별행. 밸브·레버도 별행. 미사용 import `SM_Bath_01_*` 4개(`Bath_01/StaticMeshes`)는 referencer 0 |
| 설비 부품 | 욕탕 급수 밸브 | `/Engine/BasicShapes/Cube` | `BP_Bath.FillValveControl` | 30×5×30, 중심 pivot, `(−55,105,68)` | QueryOnly 상호작용(Visibility Block). local Y축 90° 회전, 0.5초. pivot = 회전축 | 근거 `.md/Unreal/BathWaterSystem.md`. 문서의 scale 기록은 asset과 다름(조사 한계) |
| 설비 부품 | 욕탕 배수 레버 | `/Engine/BasicShapes/Cube` | `BP_Bath.DrainLeverControl` | 6×6×30, 중심 pivot, `(55,105,65)` | QueryOnly. local Y축 45° 회전. 현재 중심 pivot이라 실제 mesh는 끝단(축) pivot 필요 | 같은 근거 |
| 설비 부품 | 욕탕 수면 | `/Engine/BasicShapes/Plane`(BP template) | `BP_Bath.WaterSurfaceMesh` | 250×170, rel Y −14, 게임 시작 시 숨김 | NoCollision, mover가 Z 이동. 탕 안쪽 윤곽 평면 | 물 재질 위주. 정본 문서는 `SM_Bath_01_Water`로 적어 다름(조사 한계) |
| 설비 | 샤워기 | Cube | `BP_Shower.FacilityVisual`(rel Z50, scale (0.8,1.1,0.12)), Level `Shower` | 80×110×12 판, 바닥에서 44~56 높이 | footprint 80×110×56(extent (40,55,28)). 손님 slot 2개가 local (120,±55). 비품 진열 `ShowerDisplayTarget`(Z50)·Shampoo/BodyWash 그룹 | 본체 형태(벽 부착 샤워대+수전) 새로 정의 필요 |
| 설비 | 1칸 옷 락커 | Cube | `BP_ClothesLocker.FacilityVisual`(rel Z50, (0.6,0.35,1)), Level `ClothesLocker_1/2` | 60×35×100, 바닥 pivot | footprint 60×40×100. `FacilitySlot`·`LockerSlot01` local (120,0) 앞. 문짝 열림 동작 없음 | 4·8칸은 같은 칸을 이어 붙임 |
| 설비 | 4칸·8칸 옷 락커 | Cube ×4 / ×8 | `BP_ClothesLocker_4`(`FacilityVisual`+`LockerVisual02~04`), `BP_ClothesLocker_8`(`~08`) | 칸당 60×35×100, Y 간격 35. 4칸 60×140, 8칸 60×280 | footprint 4칸 (30,70,50), 8칸 (30,140,50). `LockerSlot01~04/08`이 칸마다 있음 | 칸 모듈 1개 + 4·8칸 조립 mesh 권장 |
| 설비 | 신발장 | Cube | `BP_ShoeLocker.FacilityVisual`(rel Z50, (0.6,0.35,1)), Level `ShoeLocker_1/2` | 60×35×100, 바닥 pivot | `FacilitySlot` local (120,0) | 배치 정의 없음(고정) |
| 설비 | 카운터 | Cube | `BP_BathhouseCounter.CounterVisual`(rel Z50, (1.6,1,0.5)), Level `Counter` | 160×100×50, 상자가 바닥 위 25~75에 떠 있음 | 손님 서는 자리 CheckIn/Checkout `(220,±70,0)`, 대기열 `(320~720,±70)`, 현금 놓는 곳 `CashOfferPoint (100,70,130)`, 반납 열쇠 `ReturnedKeyPoint0~2 (50,−60/0/60,130)`·`ReturnedKeyDropPoint (20,20,115)` | 상판 높이가 위 점들(Z 115~130)과 맞아야 함 |
| 설비 | 관리 컴퓨터(모니터) | Cube | `BP_BathhouseComputer.ComputerMesh`(actor scale (0.12,1.2,0.7)), Level `Computer` | 12×120×70, 중심 pivot, actor Z 185 | `ScreenWidget` World Space 1024×576 화면이 이 면에 겹침. `FocusExitPoint`, focus camera | root scale. 화면 면과 위젯 위치 재정렬 필요 |
| 설비 | 음료 냉장고 | Cube ×5 | `BP_DrinkFridge.FridgeBody`((−10,0,90), (0.4,0.6,1.8)) + `ShelfVisual0~3`((20,0,24/69/114/159), (0.2,0.56,0.02)) | 본체 40×60×180, 선반 20×56×2(본체 앞으로 나옴) | footprint와 바닥 pivot. SelfAim 진열 공간 4개·slot 1이 선반 높이에 있음 | 문·유리 여부 결정 필요. 장식 `SM_Fridge`와 별개 |
| 설비 | 안마의자 | Cube | `BP_MassageChair.ChairBody`((0,0,50), (0.8,0.8,1)) | 80×80×100 | footprint (40,40,50). `CustomerSlot` (0,0,0), approach (80,0,0). 고장 표시 graph 있음 | 앉는 자세 기준점 확인 |
| 설비 | 평상 | Cube | `BP_RestBench.BenchBody`((0,0,25), (3.6,1,0.5)) | 360×100×50 | footprint (180,50,25). `SeatSlot0~2` X −120/0/120, approach (0,90,0), yaw 90 | |
| 설비 | TV | Cube ×2 | `BP_Television.TvBody`((0,0,90), (0.8,0.4,1.8)), `ScreenOnVisual`((21,0,105), (0.02,0.7,1.1), 기본 숨김) | 본체 80×40×180, 화면 2×70×110 | 화면 component를 켜고 끄는 graph. slot 0개 | 화면은 별도 mesh(재질 교체용) 유지 |
| 설비 | 화장대 | Cube + Plane | `BP_Vanity.VanityBody`((0,0,50), (0.6,1.2,1)), `MirrorVisual`(Plane, (−25,0,140), pitch 90, (0.8,1.2)) | 60×120×100 + 거울 80×120 | 비품 진열 4그룹(빗·드라이어·면봉·로션) 위치, slot 1 | 거울은 평면+재질 |
| 설비 | 세신대 | Cube | `BP_ScrubTable.TableBody`((0,0,45), (2,0.8,0.9)) | 200×80×90 | footprint (100,40,45). 손님 누움 `CustomerSlot` (0,0,90), approach (0,−110,−90) | 상판 Z 90 유지 |
| 설비 | 세탁기 | Cube ×2 | `BP_Washer.MachineVisual`((0,0,4), (0.6,0.5,0.08)), `LidMesh`(pivot 상대 (30,0,0), (0.6,0.5,0.02)), `LidPivot` (−30,0,80) | 받침 60×50×8 + 뚜껑 60×50×2. 본체 몸통은 없음 | footprint 60×50×80. 뚜껑은 뒤 가장자리 경첩, local Y축 −90°. 수건 표현 ISM이 runtime에 붙음 | 몸통 포함 새 모델. 뚜껑 별도 |
| 설비 | 건조기 | 위와 같음 | `BP_Dryer` 같은 구성 | 위와 같음 | 위와 같음 | 세탁기와 외형 구분 |
| 설비 | 보일러 | `/Game/Bathhouse/Meshes/SM_Facility_sample` | `BP_Boiler.VisualMesh`(scale (1,0.6,1.2)) | 100×60×120, 바닥 pivot | footprint 100×60×120. 투입구·문·계기판 위치는 아래 부품행. 투입 판정은 `FuelIntakeVolume` box(mesh 아님) | 근거 `.md/Unreal/UtilityLaborSystem.md`. 최종은 투입구·계기판 통합 본체 + 바늘·문만 분리 |
| 설비 부품 | 보일러 투입구·문·계기판·바늘 | Cube ×4 | `BP_Boiler.FuelIntake`((0,−33,42), 25×6×18), `FuelDoorMesh`(`FuelDoorPivot` (12.5,−37,42) 오른쪽 경첩, 25×2×18), `GaugeFacePlate`((0,−32,90), 30×2×30), `GaugeNeedleMesh`(`GaugeNeedlePivot` (0,−34,90), 상대 (7,0,0), 14×2×1.5) | 앞면(−Y) 부품 | 문: local Z축 +90°, 0.2초. 바늘: local Y축 −90°~90° | 바늘·문만 별도 mesh 유지 |
| 설비 | 쿨러 | `/Game/Bathhouse/Meshes/Cooler/Cooler_Body`(재질 8, convex 1) + `Cooler_Lid` + `Cooler_Needle` | `BP_Cooler.VisualMesh`, `FuelDoorMesh`(`FuelDoorPivot` 자식, 상대 (2,0,0)), `GaugeNeedleMesh`(pivot 자식). Level `BP_Cooler` actor scale 0.5 | 본체 mesh 166×109×274 → Level에서 약 83×55×137 | 뚜껑 pivot이 경첩 끝(X=0), 바늘 pivot이 회전 중심. footprint extent (50,30,60) | Level instance scale 0.5로 크기를 맞춤. 새 mesh는 scale 1 기준 |
| 설비 | 순환기 | SM_Facility_sample + Cube ×2 | `BP_Circulator.VisualMesh`(scale (1.2,0.8,1.2)), `LeverMesh`(`LeverPivot` 자식, 상대 (0,0,25), 4×2×25), `GaugeNeedleMesh`(14×2×1.5) | 120×80×120, 바닥 pivot | footprint 120×80×120. 레버는 pivot 회전(레버 노동), 바늘 회전 | 레버·바늘 별도 |
| 설비 | 석탄 공급함 | SM_Facility_sample | `BP_CoalSupply.SupplyMesh`(0.5), Level `CoalSupply` | 50×50×50, 바닥 pivot | QueryOnly 상호작용 trace 대상. 삽으로 퍼는 곳 | |
| 설비 | 드라이아이스 공급함 | SM_Facility_sample | `BP_DryIceSupply.SupplyMesh`(0.5) | 50×50×50, 바닥 pivot | 위와 같음 | 석탄 공급함과 외형 구분 |
| 설비 | 수건 공급대 | Cube | `BP_CleanTowelStack.StackVisual`((0,0,25), (0.5,0.35,0.5)), Level `CleanTowelStack` | 50×35×50 | QueryOnly. 수건 수량 표현 ISM(`TowelPresentationVisual`)이 위에 쌓임. 손님 `FacilitySlot` local (−75,0) | placement opt-out |
| 설비 | 사용 수건함 | `/Engine/BasicShapes/Cylinder` | `BP_UsedTowelBin.BinVisual`((0,0,25), (0.45,0.45,0.5)) | 지름 45×높이 50 | QueryOnly. 수건 표현 ISM, 손님 slot local (−75,0) | |
| 설비 | 수건 건조대 | Cube | `BP_DryingSpot.FacilityVisual`((0,0,50), (0.9,0.8,0.12)), Level `DryingSpot` | 90×80×12 판, 44~56 높이 | BlockAllDynamic. 수건을 거는 위치 | 건조대 형태 정의 필요 |
| 설비 | 퇴장 지점 표시 | Cube | `BP_BathhouseExit.FacilityVisual`((0,0,50), (0.12,1.2,1.2)), Level `Exit`(마당) | 12×120×120, 바닥 아래 10까지 | BlockAllDynamic이라 Nav를 깎음. 손님 slot local (120,0) | 게임에서 보일 필요(출구 표지·문짝 없는 문틀 등)부터 결정 |
| 설비 | 열쇠걸이 고리 | Cylinder | `BP_BathhouseKeyHook.HookVisual`((0,0,20), pitch 90, (0.15,0.15,0.04)) | 지름 15×두께 4 원판 | 열쇠걸이 `BP_BathhouseKeyRack` `PairTransforms`(8자리) 위치에 놓임 | 열쇠걸이 판 자체는 mesh 없음 → 판 포함 모델 후보 |
| 설비 | 설비 포장 상자(회수·구입 설비 아이템) | Cube(C++ 기본, 정의 `RecoveryItemMesh`=None → Cube fallback) | `BP_PlaceableFacilityItem.ItemRoot`(0.6), 정의 16개 `RecoveryItemMesh` | 60×60×60, 중심 pivot | 물리 운반 root. `PlaceableFacilityItemCollision`이 mesh로 충돌을 만든다. 크기 정본은 `ItemRoot` Relative Scale | 설비별 다른 상자를 원하면 정의마다 `RecoveryItemMesh` 지정 |
| 운반 물건 | 배송 상자 | SM_Facility_sample | `BP_ShopDeliveryBox.BoxMesh`(0.8, C++ 기본은 Cube) | 80×80×80, 바닥 pivot | 물리 운반, 개봉 상호작용 | 상점 주문 runtime spawn |
| 운반 물건 | 품목 박스(트레이) | Cube | `BP_ItemBox.BoxMesh`((0.4,0.3,0.1)), `ContentsVisual`(scale으로 부모 상쇄) | 40×30×10, 중심 pivot | 품목 정의 `BoxSlotTransforms`(최대 12칸) 위치에 품목 mesh가 보임 | 위가 열린 박스 필요. 근거 `.md/Unreal/ServiceSystem.md` |
| 운반 물건 | 수건 바구니 | Cube | Towel `BP_TowelBasket.WorldMesh`(Level `TowelBasketCart` actor scale (0.4,0.3,0.2)) | 40×30×20, 중심 pivot | 물리 운반. 수건 표현 ISM(`DA_TowelVisual_Basket`). 고정 거치대 `TowelBasketFixedSlot` anchor 상대 0 | root scale. `/Game/Bathhouse/Blueprints/Facility/BP_TowelBasket`(Cube 50×50×60)도 있음(조사 한계) |
| 운반 물건 | 묶은 쓰레기 봉투 | Cube(C++ 기본) | `BP_TrashBag.BagMesh`((0.3,0.3,0.4)) | 30×30×40, 중심 pivot | 물리 운반, `HeldTransform` 위치 기준 | 집게로 묶을 때 runtime spawn |
| 도구 | 쓰레기 집게 | Cube | `BP_LitterTongs.WorldMesh`(root (0.04,0.04,0.8)), Level `LitterTongs` | 4×4×80, 중심 pivot | 물리·CCD. `HeldTransform` 위치 기준, 집게 끝에서 쓰레기 줍기. 거치대 anchor 상대 0 | root scale |
| 도구 | 물걸레 | Cube ×2 | `BP_WetMop.WorldMesh`(root (0.08,0.08,0.8)) + `MopHeadVisual`(상대 (0,0,−50), (0.35,0.12,0.06)) | 자루 8×8×80 + 헤드 | 물리 운반. 헤드는 root scale을 물려받아 실제 크기가 의도와 다를 수 있음 | root scale. 자루+헤드 한 mesh 권장 |
| 도구 | 삽(석탄·드라이아이스 적재 표시 포함) | Cube ×2 | `BP_UtilityShovel.WorldMesh`(Level actor scale (0.7,0.06,0.03)), `LoadVisual`(상대 (20,0,90), (0.5,0.8,1), 비적재 시 숨김) | 70×6×3, 중심 pivot | 물리·CCD·Pawn Ignore. `LoadVisual`은 퍼 담은 연료 표시(종류별 재질) | root scale. 날 부분 위치에 적재 mesh |
| 도구 | 몽키 렌치 | `/Game/Bathhouse/Meshes/Equips/af6948267782dcbb521c7b296bce3a64`(재질 1, simple 1) | `BP_MonkeyWrench.WorldMesh`(CDO root 0.2), Level instance actor scale (0.5,0.8,0.5) | mesh 36×11×120 → Level 약 18×9×60, 회전 (0,90,−90) | 물리 운반·전투 장비 | Level instance가 비균일 scale을 덮어씀 |
| 도구 | 때수건 | Cube | `BP_ScrubTowel.WorldMesh`(root (0.25,0.18,0.03)), Level `ScrubTowel` | 25×18×3 | 물리 운반, 세신 도구. 거치대 anchor 상대 0 | root scale |
| 도구 | 열쇠 | Cylinder | `BP_BathhouseKey.WorldMesh`(pitch 90, (0.12,0.12,0.03)) | 지름 12×두께 3 원판 | 물리, 체크인 때 runtime 전달, 고리에 걸림 | 번호표 달린 열쇠 |
| 소모품·쓰레기 | 현금(지폐) | Cube | `BP_BathhouseCashPayment.WorldMesh`((0.18,0.08,0.015)) | 18×8×1.5 | 카운터 `CashOfferPoint`에 놓임 | runtime spawn |
| 소모품·쓰레기 | 바닥 쓰레기 3종 | Cylinder·Cube·Sphere | `BP_Litter.LitterMesh`((0,0,7.5), 0.15), `MeshVariants` 3개 | 각 15cm | NoCollision 시각. 상호작용은 반경 12 sphere. `FloorRadiusCm` 15(간격 계산) | 종류 수는 `MeshVariants` 배열 길이로 자유 |
| 소모품·쓰레기 | 바나나우유 | Cylinder | `DA_ServiceItem_BananaMilk.ItemMesh` | 정의 offset·scale에 따름 | 품목 박스 `BoxSlotTransforms` 12칸, 냉장고 진열 칸에 놓임 | 정의별 mesh 하나 |
| 소모품·쓰레기 | 샴푸 / 바디워시 | Cylinder | `DA_ServiceItem_Shampoo`, `_BodyWash` `.ItemMesh` | 정의 transform | 샤워기 진열 그룹(소모형) | 두 병 외형 구분 |
| 소모품·쓰레기 | 빗 / 헤어드라이어 | Cube | `DA_ServiceItem_Comb`, `_HairDryer` `.ItemMesh` | 정의 transform | 화장대 진열 그룹 | |
| 소모품·쓰레기 | 면봉 / 스킨로션 | Cylinder | `DA_ServiceItem_CottonSwab`, `_SkinLotion` `.ItemMesh` | 정의 transform | 화장대 진열 그룹 | |
| 소모품·쓰레기 | 수건(깨끗한·젖은·사용·고급 4상태) | `/Game/Bathhouse/Meshes/Towel/SM_Towel_clean_common`, `_wet_common`, `_used_common`, `_clean_good` | `DA_TowelVisual_Basket/Dryer/Washer/UsedBin/Shelf.StateVariants` → 각 설비 수건 표현 ISM | mesh 약 190×177×25(clean_good 190×190×35), 중심 pivot. 표현 component가 줄여 씀 | 같은 위치에 상태별로 바꿔 끼우는 변형. ISM 수량 표현 | 이미 import된 임시 |
| 소모품·쓰레기 | 바닥에 떨어진 사용 수건 | Plane | `BP_WorldUsedTowel.WorldMesh` | 100×100 평면 | BlockAllDynamic, runtime spawn | 납작한 수건 mesh |
| 건물 | 벽·바닥·천장 | `/Engine/BasicShapes/Cube`(Project Settings `ShellBoxMesh`) | 공간 Actor `Shell` 생성 ISM(Floor·Wall·Ceiling part, 저장 안 됨) | 공간 크기에 맞춰 늘림 | 상자 1개를 instance scale로 늘려 씀(mesh local bounds 기준), part마다 ISM 1개·재질 1개, world 기준 투영 재질(`M_Building_WorldAligned`) | 전용 mesh 불필요(재질·텍스처 작업 대상). 구조 제약은 아래 절 |
| 건물 | 계단 판 | Cube(ShellBoxMesh) | `Shell` StairStep ISM(홀 `Stairs[0]`, 20판, 충돌 없음) | 판마다 폭×깊이×두께로 늘린 상자 | 실제 걷는 면은 보이지 않는 경사로. 판은 시각만 | 모델링 후보: 계단 판 전용 mesh(모서리 nosing). 현재 구조로는 상자만 가능, 지정 슬롯 추가는 구현 작업 |
| 건물 | 계단 난간·계단 벽 | Cube(ShellBoxMesh) | `Shell` StairWall ISM | 벽 두께로 늘린 상자 | 충돌·Nav 관련 | 모델링 후보: 손잡이 난간(현재 벽형 난간만) |
| 건물 | 출입구·통로 문틀 | 없음(벽에 뚫린 구멍) | 홀 서쪽 출입구, 홀↔목욕공간 통로 | 폭·높이는 공간 `Openings` | 문짝 없음 계약 | 모델링 후보: 문틀 trim. 현재 붙일 component 없음(구현 작업 필요) |
| 건물 | 천장 조명 기구 | 없음(PointLight만) | `Shell`이 공간마다 만드는 PointLight 칸 중심 | 공간 `Lighting` 값 | 빛만 있고 기구 mesh 없음 | 모델링 후보: 천장 등. 같은 위치에 mesh를 붙이려면 구현 작업 필요 |
| 장식 | 장식 냉장고 | `/Game/StylizedKitchen/Meshes/SM_Fridge`(LOD 4, convex 4) | Level `SM_Fridge`(scale 0.5, 홀) | 약 76×61×118 | BlockAll 장식 | 음료 냉장고와 혼동 주의. 유지 여부 결정 |
| 장식 | 청소기 | `/Game/Bathhouse/Meshes/Cleaner/cleaner` | Level `cleaner`(홀) | 66×61×93, 바닥 pivot | 장식 | |
| 장식 | 장식 보일러 | `/Game/Bathhouse/Meshes/boiler` | Level `boiler`(목욕공간) | 61×43×103, 바닥 pivot | 장식(설비 `BP_Boiler`와 별개) | |
| UI·표시용 | 세신 커서 | Cube | `BP_ScrubTable.ScrubCursor`((0.18,0.12,0.03), 게임에서 숨김 시작) | 18×12×3 | NoCollision, 세신 미니게임 위치 표시 | 표시 방식(mesh/decal) 결정 후 |

## 건물(상자 생성 구조) 제약

- 벽·바닥·천장·계단 판·계단 벽은 C++ `UBathhouseSpaceShellComponent`가 part마다 `UInstancedStaticMeshComponent` 하나를 만든다. 모두 Project Settings `ShellBoxMesh` 하나를 비균일 scale로 늘려 쓴다. mesh를 바꾸면 모든 part가 같은 mesh로 늘어난다.
- 그래서 디테일(몰딩·타일 줄눈·nosing)을 mesh에 넣으면 늘어나 찌그러진다. 표면 표현은 world 기준 투영 재질로 한다(UV 늘어남 회피). 전용 mesh 조각(계단 판, 문틀, 조명 기구, 난간 손잡이, 걸레받이)을 쓰려면 별도 component·mesh 설정을 추가하는 구현·아키텍처 작업이 먼저 필요하다.
- 생성물은 Transient라 Level에서 손으로 교체·배치할 수 없다. U2 넓힘 때 벽이 이동하므로 붙이는 장식도 공간 값에서 파생돼야 한다.

## 모델링 불필요로 판단한 mesh

| mesh | 사용처 | 이유 |
|---|---|---|
| `/Engine/BasicShapes/Plane` | `BP_BathhouseSpace`·`BP_FacilityPlacementZone` `GridVisual` | 배치 격자 표시. 기본 숨김, 격자 재질(`MI_FacilityPlacementGrid`)이 표현 |
| `/Engine/BasicShapes/Plane` | `BP_TrashCollectionZone.ZoneMarker` | 수거 구역 바닥 표시(미리보기 재질). 집하장 외형을 원하면 별도 장식으로 정한다 |
| `/Engine/BasicShapes/Plane` | `BP_WaterStain.StainVisual` | 물 얼룩은 평면+재질(텍스처·decal) 작업이 맞다 |
| Cube(ShellBoxMesh) StairRamp·StairKeepClear | 공간 `Shell` | 숨김 충돌·막이 상자 |
| `/Engine/EditorMeshes/MatineeCam_SM` | `Computer` 카메라 proxy | Editor 전용(게임에서 숨김) |
| `/Engine/EngineSky/SM_SkySphere` | Level `SM_SkySphere` | 엔진 하늘 |
| `/Game/Prop/SM_Cylinder`, `/Engine/BasicShapes/Sphere` | `NS_HoneyBeam`, `NS_WaterStream` | Niagara 효과용 mesh(VFX 작업 대상) |
| Cube | `BP_ServiceTestUser` | 테스트용 actor, 레벨에 저장하지 않음 |
| Cube(C++ 기본) | `BP_TrashBin` | Level instance 제거됨, 현재 쓰지 않음 |
| 설비 placement preview | `FacilityPlacementPreviewActor` | 정의의 설비 Blueprint mesh를 미리보기 재질로 다시 씀(별도 mesh 없음) |
| `Space_*`·설비의 빈 StaticMeshComponent(mesh 없음) | Washer/Dryer/Shower/Stack/Bin의 runtime 표현 component | 수건·비품 표현 ISM 자리. mesh는 정의(DA)에서 들어옴 |

## 조사 한계

- 캐릭터(플레이어 1인칭 팔·손님)는 SkeletalMesh라 이 목록 범위 밖이다. 1인칭 손에 든 물건은 위 운반 물건 mesh를 그대로 쓴다.
- 품목 정의의 `DisplayOffset`·`BoxSlotTransforms`와 비품 진열 그룹의 정확한 칸 위치는 DataAsset·Blueprint 값이 정본이며 여기 복제하지 않았다(`.md/Unreal/ServiceSystem.md`).
- `/Game/Bathhouse/Blueprints/Facility/BP_TowelBasket`(Cube 50×50×60)과 `/Game/Bathhouse/Blueprints/Towel/BP_TowelBasket`(운반 바구니) 두 asset이 있다. Level은 Towel 쪽을 쓴다. Facility 쪽의 runtime 사용 여부는 확인하지 않았다.
- 정본 문서와 asset 값이 다른 곳이 있다. `BathWaterSystem.md`: 수면 mesh `SM_Bath_01_Water`, 밸브 scale (0.3,0.08,0.08). `UtilityLaborSystem.md`: 삽 scale (0.25,…). 이 목록은 조사 시점 asset 값을 따랐다. 정본 정정은 해당 시스템 작업 몫이다.
- 상점 상품 중 품목 박스·포장 상자로 오는 것 외에 상품 전용 mesh가 있는지는 `DA_ShopCatalog` 직접 속성에서 mesh를 찾지 못한 것으로만 판정했다(상품은 정의·class 참조를 통해 위 mesh를 쓴다).
- 열쇠걸이 판(`BP_BathhouseKeyRack`)에는 StaticMesh가 없다. 고리만 보이는 현재 모습이 의도인지 확인하지 않았다.
- runtime에만 생기는 component(수건 수량 ISM, 비품 진열 ISM, 생성 조각)의 실제 크기는 PIE에서 확인하지 않았다.
