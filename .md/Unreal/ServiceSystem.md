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

## 서비스 2단위 (화장대·비품 여섯 종)

저장·새 프로세스 재로드로 확인한 상태다. 임시 mesh는 `/Engine/BasicShapes`의 Cube/Cylinder/Plane뿐이다.

- **품목 정의 여섯 개** (`/Game/Bathhouse/Data/Service/DA_ServiceItem_*`, `ServiceItemDefinition`, `DisplayCategories` 비움, `SaleValue=0`, `DisplayMesh` 비움, `BoxItemOffset=DisplayOffset`, 자리 위치는 `BoxSlotTransforms`): HairDryer(Cube, 정원 2·횟수 0), SkinLotion(Cylinder, 6·10), CottonSwab(Cylinder, 6·30), Comb(Cube, 12·0, 회전 yaw 90), Shampoo(Cylinder, 6·30), BodyWash(Cylinder, 6·30). 배열 순서는 x가 빠른 행 우선, 모든 자리가 `BP_ItemBox` 트레이(40×30cm) 안이다. 바나나우유 정의는 수정하지 않았다.
- `BP_Vanity`(`BathhouseFacilityActor`, `FacilityType=Vanity`): `VanityBody`(Cube, `(0,0,50)`, scale `(0.6,1.2,1)`, BlockAllDynamic·Navigation 활성), `MirrorVisual`(Plane, `(-25,0,140)`, Pitch 90, scale `(0.8,1.2,1)`, NoCollision·Navigation off), `DisplayTarget`(`DisplayFacilityTargetComponent`, `화장대`, `(0,0,100)`, extent `(45,75,100)`, QueryOnly), 공간 `DryerGroup/LotionGroup/SwabGroup/CombGroup`(SpaceIndex 0~3, FacilityRouted, FixedKind 각 품목, 자리 2/4/4/6, y `-45/-15/15/45`·z 105, NoCollision·Navigation off, 모두 SceneRoot 자식), `CustomerSlot` 1개(`(70,0,0)`, ApproachOffset `(30,0,0)`, yaw 180), Actor component `DisplayManager`(RequiredCustomerSlotCount 1, ConsumeOnCustomerUseStart true, CustomerUseSeconds 20, attachment 없음). Footprint 위치 `(0,0,90)`, extent `(40,70,90)`. 전면은 +X.
- `DA_FacilityPlacement_Vanity`: `StableId=Facility.Vanity`, 태그 Placeable·Discardable, `PlacedFacilityClass=BP_Vanity_C`, 공통 회수 아이템 class, 잠금 칸 0. `BP_Vanity.FacilityPlacement.Definition`에 연결.
- `BP_Shower`: 기존 parent·slot 2개·Definition·footprint·몸체 유지. 추가: `DisplayManager`(슬롯 2, 소모 true, 0초), `ShowerDisplayTarget`(`샤워기`, `(0,0,50)`, extent `(45,60,30)`), `ShampooGroup`(0)/`BodyWashGroup`(1), FacilityRouted·자리 2개·NoCollision·y `-25/25`. DefaultMap 기존 샤워기 instance는 이를 상속한다(override 없음).
- `BP_Washer`/`BP_Dryer`: `LidMesh`=Cube(pivot 상대 `(30,0,0)`, scale `(0.6,0.5,0.02)`, NoCollision), `LidPivot` `(-30,0,80)`, `LidPresentation` 축 `(0,1,0)`·`-90°`·0.2/0.2초, `MachineVisual` 위치 `(0,0,4)`·scale `(0.6,0.5,0.08)`(asset·충돌·Navigation 유지). `TowelPresentationVisual`: 위치 `(0,0,12)`, ItemsPerLayer 5, MeshProfile·RandomSeed·회전 범위 유지. 컴포넌트 scale `(0.16,0.12,0.025)`이 pile 좌표에 곱해지므로(수건 mesh 원본 190×177cm → 약 30×21cm) `PileHalfExtent` `(62.5,83.33,80)`·`LayerSpacing` 80·`MaxZJitter` 12는 월드 약 `(10,10,2)cm`·간격 2cm·지터 0.3cm에 해당한다. 프롬프트의 `(22,18)`/8/2를 그대로 쓰면 수건이 한 점에 겹치고, 기존 y=150은 footprint(±30)를 넘어 조정했다.
- `BP_CleanTowelStack`/`BP_UsedTowelBin`: native `DisplayCue` 1개와 `TowelPresentationVisual` 상속 확인(저장 변경 없음). `BP_DrinkFridge`: native `DisplayManager`(슬롯 1, 소모 false)와 SelfAim 공간 4개·slot 1개 상속 확인(변경 없음).
- `DisplaySpaceComponent` collision 설정은 `bodyInstance.collisionProfileName`만 바꾸면 `collisionEnabled`가 그대로다. FacilityRouted는 `bodyInstance.collisionEnabled=NoCollision`을 명시해야 native 검증을 통과한다.
- `DisplaySpaceComponent.SlotTransforms`처럼 기본값이 있는 배열은 개수를 바꾸면서 값을 바꾸는 요청이 MCP에서 거부된다. 기존 값 그대로 개수만 줄이는 요청 뒤 값을 지정한다.

## 서비스 4단위 (안마의자·평상·TV·세신대)

저장·새 프로세스 재로드로 확인한 상태다. 임시 mesh는 `/Engine/BasicShapes/Cube.Cube`뿐이고 새 재질은 없다. 네 설비 Actor scale은 모두 `(1,1,1)`, grid는 20cm다.

| Blueprint (`/Game/Bathhouse/Blueprints/Service/`) | Parent / FacilityType | body (SceneRoot 자식, BlockAllDynamic·Navigation on) | footprint (extent, 위치 Z) | slot (모두 `BathhouseFacilitySlotComponent`, enabled) |
|---|---|---|---|---|
| `BP_MassageChair` | `MassageChairActor` / MassageChair | `ChairBody` `(0,0,50)` scale `(.8,.8,1)` | `(40,40,50)`, Z50 | `CustomerSlot` 1: 위치 `(0,0,0)`, approach `(80,0,0)`, yaw 0 |
| `BP_RestBench` | `BathhouseFacilityActor` / **RestBench**(부모 기본 Bath를 변경) | `BenchBody` `(0,0,25)` scale `(3.6,1,.5)` | `(180,50,25)`, Z25 | `SeatSlot0~2`: X -120/0/120, approach `(0,90,0)`, yaw 90 |
| `BP_Television` | `TelevisionActor` / Television | `TvBody` `(0,0,90)` scale `(.8,.4,1.8)` | `(40,20,90)`, Z90 | 없음(0개) |
| `BP_ScrubTable` | `ScrubTableActor` / ScrubTable | `TableBody` `(0,0,45)` scale `(2,.8,.9)` | `(100,40,45)`, Z45 | `CustomerSlot` 1: 위치 `(0,0,90)`, approach `(0,-110,-90)`(바닥), yaw 90 |

- 모든 footprint는 NoCollision·Navigation off이고 full X/Y가 20cm의 정수배다. 각 설비의 `FacilityPlacement.Definition`은 아래 Definition이다.
- `BP_MassageChair`: `UseSeconds 60`, `UseFee 3000`, `BreakChancePercent 10`, `RepairSeconds 3`. `BrokenLabel`(TextRender "고장", 위치 `(0,0,125)`, NoCollision·Navigation off, 기본 hidden). Event Graph: `BeginPlay`에서 `IsBroken()`을 읽어 `BrokenLabel` Visibility 설정, `OnBrokenStateChanged(Broken)`에서 Visibility만 반전. 돈·고장 판정은 graph에 없다.
- `BP_Television`: `ScreenOnVisual`(Cube `(21,0,105)` scale `(.02,.7,1.1)`, `MI_FacilityPreview_Valid` 재질, NoCollision·Navigation off, 기본 hidden). Event Graph: `OnPowerChanged(PoweredOn)`에서 Visibility만 설정.
- `BP_ScrubTable` 기본값: `ScrubFee 20000`, `WaitLimitSeconds 90`, `RequiredRubDistanceCm 3000`, `RubCmPerInputUnit 1`, `ExitSearchRadiusCm 100`, 블렌드 in/out 0.35/0.25, `CashOfferClass=BP_BathhouseCashPayment_C`. 여섯 native component(이름 유지, 중복 SCS 없음): `ScrubCamera` `(0,-170,240)` pitch -45/yaw 90, `ScrubArea` `(0,0,95)` **회전 Pitch 180·Yaw 90·Roll 0(Details 패널 `(Roll,Pitch,Yaw)` 표기로 `(0,180,90)`), extent `(35,90,1)`**(NoCollision·Navigation off; 사용자 PIE에서 확정·저장, 새 프로세스 디스크 읽기로 확인), `ScrubCursor` Cube scale `(.18,.12,.03)`(NoCollision·Navigation off·HiddenInGame), `ScrubExitPoint` `(0,-120,0)` yaw 90, `CashOfferPoint` `(0,-160,110)`, `CashStandPoint` **`(0,-200,0)`** yaw 90(프롬프트 초기값 -180에서 조정: 이탈점과 60cm 간격이면 캡슐이 겹쳐 80cm로 벌렸다).
- `ScrubArea` 축 규약: 코드가 마우스 오른쪽(+)을 영역 로컬 Y로, 마우스 아래(+, 입력 Y 반전 때문)를 로컬 X로 보낸다. 위 회전에서 로컬 X = 월드 -Y(카메라 쪽, 화면 아래), 로컬 Y = 월드 -X(화면 오른쪽), **로컬 Z는 아래를 향한다**("+Z가 표면 법선" 계약과 다르며 코드에서 법선은 커서 높이 계산에만 쓰인다). 그래서 extent는 X(깊이)=35, Y(가로)=90이다. 프롬프트 초기값(회전 0, extent `(90,35,1)`)은 마우스 방향이 90° 어긋났다.
- 카메라 기하(초기 extent 기준, FOV 90·16:9 계산): 네 모서리와 커서가 모두 화면 안에 있고 몸체에 가려지지 않는다. 이탈점은 몸체 앞면에서 80cm 앞이며 바닥 Z=0이다. 실제 포커스 화면 판정은 PIE 확인 대기.
- `BP_ScrubTowel`: `WorldMesh` Cube scale `(.25,.18,.03)`(QueryAndPhysics, CCD, Pawn Ignore), `HeldTransform` `(45,15,-30)` scale 1. `BP_ServiceTestUser`: `WorldMesh` Cube scale `(.5,.5,1.7)`, 상대 Z 85(발 기준 원점), NoCollision·Navigation off, 물리 없음.
- Definition `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_{MassageChair,RestBench,Television,ScrubTable}`: `StableId=Facility.<이름>`, 태그 Placeable·Discardable, `PlacedFacilityClass=BP_<이름>_C`, 공통 회수 아이템 class, `RecoveryItemMesh` 비움, 잠금 칸 0.
- 상속 확인만(변경 없음): `BP_Shower`(슬롯 2·진열 공간·manager), `BP_MonkeyWrench`(`WorldMesh`·`MeleeAttack`), `BP_FirstPersonCharacter`(`PlayerScrubFocus` 정확히 1개, 컴퓨터 component 유지).
- DefaultMap의 때수건과 전용 거치대는 [WorldSystem.md](WorldSystem.md)에 있다.

