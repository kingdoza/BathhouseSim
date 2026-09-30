# Shop Editor Authoring

## 확인된 저장 상태

/Game/Bathhouse/Data/Shop/DA_ShopCatalog은 UShopCatalog asset이다. 재로드 확인된 Products에는 ProductId=Shower, bForSale=true, DisplayName=샤워기, Price=10000, PlacementDefinition=/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Shower가 있다. 상품 배열 순서는 UI 카드 순서다.

다음 Placement Definition 일곱 개는 저장 후 새 프로세스에서 확인했다. 모두 Facility.Placeable과 Facility.Discardable 태그가 있으며 LockerSlotCount=0이다.

- /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Bath
- /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Shower
- /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Washer
- /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Dryer
- /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler
- /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Cooler
- /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Circulator

## 서비스 상품

`DA_ShopCatalog`에는 기존 7종 뒤에 `DrinkFridge`(30,000, `DA_FacilityPlacement_DrinkFridge`)와 `BananaMilkBox`(12,000, `DA_ServiceItem_BananaMilk`를 `ItemBoxDefinition`으로 지정)가 있다. 저장·재로드 확인됨. `ShopSettings.ItemBoxClass`(`BP_ItemBox`)는 Config에 아직 저장되지 않았다([USER_UNREAL.md](../USER_UNREAL.md)). 상세는 [ServiceSystem.md](ServiceSystem.md).

## 아직 authoring 정본에 기록되지 않은 범위

Shop Actor Blueprint 세 개는 개별 compile/save와 생성 세션 readback까지 끝났지만 새 Editor 프로세스 재로드는 아직 확인하지 못했다. 해당 asset의 parent/component hierarchy와 CDO 값은 재로드를 확인한 뒤 이 문서에 반영한다. Widget, Project Settings, Level instance 연결도 저장된 Editor 상태가 확인된 뒤에만 추가한다.

Shop 기능 책임과 API는 [Architecture/ShopSystem.md](../Architecture/ShopSystem.md)에 있다. MCP 미지원 Editor 작업은 [USER_UNREAL.md](../USER_UNREAL.md)의 상점 후속 항목을 참조한다.