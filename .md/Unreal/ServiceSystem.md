# Service Editor Authoring

저장 후 새 Editor 프로세스 재로드로 확인한 상태만 기록한다. C++ 책임은 [Architecture/ServiceSystem.md](../Architecture/ServiceSystem.md)와 `Public/Service/`가 정본이다.

## Data Asset

| Asset | Class | 현재 값 |
|---|---|---|
| `/Game/Bathhouse/Data/Service/DA_ServiceItem_BananaMilk` | `/Script/BathhouseSim.ServiceItemDefinition` | `ItemId=BananaMilk`, `DisplayName=바나나우유`, 임시 `ItemMesh=/Engine/BasicShapes/Cylinder`, `BoxCapacity=12`, `BoxSlotTransforms` 12개(박스 root 로컬, 4열×3행, x∈{-12,-4,4,12} y∈{-8,0,8} z=11, scale `(0.06,0.06,0.12)`), `DisplayMesh` 비움, `DisplayOffset` scale `(0.06,0.06,0.12)`, `DisplayCategories={Display.Fridge}`, `SaleValue=2000` |
| `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_DrinkFridge` | `FacilityPlacementDefinition` | `StableId=Facility.DrinkFridge`, 태그 `Facility.Placeable`·`Facility.Discardable`, `PlacedFacilityClass=BP_DrinkFridge_C`, `RecoveryItemClass=BP_PlaceableFacilityItem_C`, `RecoveryItemMesh` 비움(native Cube fallback), `LockerSlotCount=0` |

## Blueprint (`/Game/Bathhouse/Blueprints/Service/`)

- `BP_ItemBox` (`ItemBoxActor`): 루트 `BoxMesh` = Engine Cube, scale `(0.4,0.3,0.1)`(40×30×10cm 트레이). 임시 mesh라 위가 열린 박스 외형이 아니다. `ContentsVisual` relative scale `(2.5,3.3333,10)`으로 부모 scale을 상쇄해 `BoxSlotTransforms`가 박스 root 로컬 cm로 그려진다. `EditorPreviewKind`=바나나우유, `EditorPreviewCount`=12(Editor 전용). `HeldTransform`은 기본값.
- `BP_DrinkFridge` (`DrinkFridgeActor`): 모든 추가 component는 `SceneRoot` 자식. `FridgeBody`(Cube, loc `(-10,0,90)`, scale `(0.4,0.6,1.8)`, BlockAllDynamic, Navigation 활성), `ShelfVisual0~3`(Cube, loc `(20,0,top-1)`, scale `(0.2,0.56,0.02)`, NoCollision, Navigation off, top=25/70/115/160), `DisplaySpace0~3`(`SpaceIndex` 0~3, `AcceptedCategory=Display.Fridge`, BoxExtent `(10,28,8)`, loc `(20,0,top+8)`, `SlotTransforms` 6개 = x∈{-4,4} y∈{-16,0,16} z=-2, QueryOnly·Visibility Block), `CustomerSlot`(`BathhouseFacilitySlotComponent` 1개, loc `(60,0,0)`, ApproachOffset `(40,0,0)`, yaw 180, enabled). `PlacementFootprint` loc `(0,0,90)`, extent `(30,30,90)`(60×60cm, grid 20cm 배수). `FacilityPlacement.Definition=DA_FacilityPlacement_DrinkFridge`. 전면은 +X.
- `BP_DrinkCollectionBox` (`DrinkCollectionBoxActor`): 루트 `BoxMesh` = Cube, scale `(0.4,0.4,0.3)`.

세 BP는 `warnings_as_errors` Compile 성공, 새 프로세스 재로드 후에도 위 값이 유지됐다. `BP_DrinkFridge`는 진열 공간·slot이 갖춰지기 전 Compile에서 native 검사 오류를 내므로 component를 모두 추가한 뒤 Compile한다.

## Material (`/Game/Bathhouse/Materials/Service/`)

- `M_PP_TakeHighlightOutline`: Post Process domain, **Opaque**, Blendable Location `BL_SceneColorAfterDOF`(에디터 표기 "Before Tonemapping"; `BL_BeforeTonemapping` 값은 엔진이 이 값으로 되돌린다). Emissive = `Lerp(SceneTexture:PostProcessInput0.rgb, OutlineColor, Mask)`이고 Opacity는 연결하지 않는다(마스크 0이면 원래 화면 그대로). `Mask`는 Custom 노드(HLSL) 출력: `ViewportUV`(ScreenPosition)를 `[0,1]` clamp → `ViewportUVToSceneTextureUV`로 변환해 CustomStencil이 `StencilValue`(기본 1)와 같은지만 판정(스텐실은 `.r`로 읽는다), 중심은 강조 아님 AND 이웃 8방향 중 강조이며 `CustomDepth ≤ SceneDepth + DepthBias`일 때 1. 파라미터: `Thickness`(2), `StencilValue`(1), `DepthBias`(1), `OutlineColor`(1,0.85,0.2). 등록용으로 SceneTexture 노드 3개(CustomStencil·CustomDepth·SceneDepth)를 Custom 입력에 연결했다. 재로드·Data Validation 통과, 화면 표시는 사용자 PIE 확인 대기.
- `MI_DisplayInsertPreview`: 이름은 MI지만 실제 class는 `Material`(기존 `MI_FacilityPreview_*`와 동일). Surface, Translucent, Unlit, Two Sided, Emissive `(0.2,0.85,1)`, Opacity 0.4.

## 연결된 기존 asset

- `/Game/Bathhouse/Data/Shop/DA_ShopCatalog`: 기존 7종 뒤에 `DrinkFridge`(음료 냉장고, 30000, `PlacementDefinition`)와 `BananaMilkBox`(바나나우유 박스, 12000, `ItemBoxDefinition`). 상품마다 두 정의 중 하나만 지정.
- `/Game/FirstPersonCharacter/BP_FirstPersonCharacter`: `FirstPersonCamera` PostProcessSettings `WeightedBlendables`에 `M_PP_TakeHighlightOutline` weight 1 하나. 다른 후처리 필드는 변하지 않았다.

## 미반영 (USER_UNREAL.md 참조)

DefaultMap의 `BP_DrinkCollectionBox` 배치는 저장되지 않았다(사용자 직접, USER_UNREAL.md). Config는 `ShopSettings.ItemBoxClass=BP_ItemBox_C`, `ServiceDisplaySettings.InsertPreviewMaterial=MI_DisplayInsertPreview`, `r.CustomDepth=3`으로 저장돼 새 Editor에서 읽힌다. `WBP_InteractionPrompt.HeldSummaryText`는 저장됐다([InteractionUISystem.md](InteractionUISystem.md)). 이 문서의 10개 asset(위 표·Blueprint·Material·카탈로그·캐릭터·WBP)은 Data Validation 전부 VALID.
