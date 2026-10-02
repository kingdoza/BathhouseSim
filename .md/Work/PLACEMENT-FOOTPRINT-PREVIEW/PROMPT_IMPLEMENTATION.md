# PROMPT_IMPLEMENTATION — PLACEMENT-FOOTPRINT-PREVIEW 배치 미리보기의 footprint 공간 표시

- 작업 ID: `PLACEMENT-FOOTPRINT-PREVIEW`
- 단계: 아키텍처
- 상태: 완료

## 0. 입력과 기준

- 기능 계약: 같은 폴더 `PROMPT_ARCHITECTURE.md`(완료, 2026-10-02 자동 승인), 결정 `QNA_FEATURE_SPEC.md`(Q1~Q9 A, Q10 B, S2·S3), 조사 `REPORT_UNREAL_DISCOVERY.md`. 조사 3절의 "미리보기 사실상 불투명"은 사용자 관찰("반투명이야")로 정정된 추론이라 설계 근거로 쓰지 않았다.
- 설계 기준 Source: `42eee21`(main과 같음). 구현은 `BUG-2026-10-02_bath_water_map_grid_scale_mismatch` 병합 뒤 원래 폴더에서 한다. 그 BUG 작업은 Placement Source를 바꾸지 않았다(Unreal 정본 형식만). 시작 전에 아래 대상 파일이 `42eee21`과 같은지 `git diff 42eee21 -- Source/BathhouseSim/*/Placement Config/DefaultGame.ini`로 확인하고, 다르면 차이를 `QNA_IMPLEMENTATION.md`에 적는다.
- 정본(이번 갱신): [PlacementPreviewSystem.md](../../Architecture/PlacementPreviewSystem.md)(신규, PlacementSystem에서 미리보기 표현 분리), [PlacementSystem.md](../../Architecture/PlacementSystem.md) Definition And Footprint, [0_ARCHITECTURE.md](../../0_ARCHITECTURE.md), [CoreSystem.md](../../Architecture/CoreSystem.md) Class Growth.
- 이 문서는 조정값 수치를 적지 않는다. 원본 위치는 9절 표다.

## 1. 기능 계약과 현재 단계

- 시나리오: FPV-001~016 전체.
- 현재 단계: 수직 구현과 전체 확장을 나누지 않는 **한 작업 공통 구현**이다(Q9 A). 배치 미리보기는 이미 모든 설비가 쓰는 공통 native 경로이고 설비별 코드·asset이 없다. 사용자 PIE는 대표 설비 Vanity로 FPV-001~009, 이어서 FPV-010~015, FPV-016 순서다.
- 목적: 들고 있는 설비의 `PlacementFootprint` XY 사각형을 메시 미리보기와 같은 후보 transform·Yaw·숨김·유효 색으로 바닥에 표시한다. 함께 쿨러 비정수 footprint, 순환기·보일러 −1° 회전을 고치고 Data Validation이 root scale·하위 scale을 반영한 비정수 footprint와 grid 축에서 돈 footprint를 오류로 잡게 한다.

## 2. 수용 기준과 비목표

수용 기준(관찰 결과는 `PROMPT_ARCHITECTURE.md` 3·5·7절 그대로):

- 표시 world 사각형 = 판정 world footprint 바닥면(`ValidateWorldPlacement`의 `RelativeFootprint * Candidate`)과 크기·중심·방향이 같고 설치 바닥에서 높이 설정만큼 위다.
- 메시 미리보기와 같은 Actor에 있어 이동·회전·snap·숨김(D4)·파괴를 함께 한다.
- 색은 메시 미리보기 재질의 유효·무효 색과 같고 같은 호출에서 바뀐다.
- 불투명 물체에 가려지고, 반투명 순서는 grid < footprint < 메시 미리보기로 고정된다.
- 표시 준비 실패는 메시 미리보기·판정·확정을 바꾸지 않고 footprint만 없으며 Warning 로그를 남긴다.
- 판정 결과는 Q10 B 세 설비 외에는 바뀌지 않는다(FPV-013).
- CDO footprint cell 파생이 root scale과 하위 component scale을 반영한다. 활성 16종은 수정 뒤 Data Validation 오류가 없다.

비목표: `PROMPT_ARCHITECTURE.md` 9절 그대로. 추가로 이번 구현은 `UPlayerFacilityPlacementComponent`, conversion transaction, Zone grid 코드, 설비 Actor class를 바꾸지 않는다.

## 3. 설계 결정 (명세의 "설계에 맡김")

| 항목 | 결정 | 근거·거부한 대안 |
|---|---|---|
| 표시 수단·소유자 | `AFacilityPlacementPreviewActor`가 소유한 transient plane `UStaticMeshComponent` 하나(`FootprintSurface`) + footprint MI의 DMI | 같은 Actor라 transform·Yaw·숨김·파괴·validity 갱신이 코드 추가 없이 공유된다. decal은 바닥 외 표면(설치 설비 아랫면, 벽)에도 투사돼 Q3 A의 "grid와 같은 가림"과 다르다. 별도 Actor는 숨김·파괴 동기화 코드가 생긴다. |
| 위치·숨김·유효성 공유 | preview Actor component라 자동 공유. 색은 `SetPlacementValidity`가 메시 재질과 같은 호출에서 DMI `PreviewColor` 설정 | `UPlayerFacilityPlacementComponent`(430줄, 400줄 경고선 초과) 무변경 |
| 색 연동 | 초기화 때 `ValidPreviewMaterial`·`InvalidPreviewMaterial`의 vector parameter `Param` 값을 읽어 둔다 | 색 원본이 메시 미리보기 재질 하나로 남는다(Q2 A). footprint 전용 색 설정은 두 번째 원본이라 거부 |
| 높이 | Project Settings 높이 설정(설치 바닥 기준). footprint 바닥면 + 높이 | 높이는 불투명 바닥과의 z-fighting만 막는다. 반투명끼리는 depth를 쓰지 않으므로 grid·Bath 메시 바닥면과의 앞뒤는 높이로 정해지지 않는다 |
| 반투명 그리기 순서 | `TranslucencySortPriority`: grid(Zone BP 값) < footprint(설정) < 메시 미리보기(설정). footprint 재질은 grid·preview 재질과 같은 translucency pass | 엔진 sort key 첫 기준이 priority이고 같은 pass 안에서만 비교된다(PlacementPreviewSystem Draw Order 표). 메시가 footprint 위에 그려져야 "반투명 메시를 통해 보인다"(명세 3절 7항)와 같다. 메시 미리보기는 지금 grid와 거리 순서를 다투므로 설정값으로 grid 위에 고정한다(재질·반투명·색 불변). PIE에서 비침이 부족하면 두 설정값이나 MI 불투명도로 조정한다 |
| 면 방향 | one-sided plane(+Z normal). 합성 scale 성분이 0 이하면 표시 준비 실패 | 카메라는 항상 바닥 위다. 음수 scale은 면을 뒤집는다 |
| 표시 준비 실패 | footprint만 만들지 않고 메시 미리보기·판정·확정은 진행. preview 초기화마다 `Warning` 한 번 | 명세 7절 "판정·확정 결과를 바꾸지 않는다"를 모두 만족한다. `Error`는 automation이 실패로 세어, 표시 재질 없는 기존 preview 테스트가 깨진다 |
| 조정값 저장 | 채움·외곽선 불투명도, 외곽선 두께 = footprint MI 기본값. 높이·우선순위·mesh·MI 참조 = Project Settings | grid와 같은 분담(표현 parameter는 MI, transform 값은 native). 모두 전역 한 벌(Q8 A) |
| 쿨러 정수 칸 authoring | `BP_Cooler` `PlacementFootprint` BoxExtent XY만 바꾼다. root scale·`SceneRoot` Yaw·relative location·Z는 그대로 | 메시와 상대 위치·방향 유지(FPV-016). root scale을 바꾸면 메시 크기가 바뀐다 |
| 비정수 검사 | cell 파생 scale = `GetFootprintRelativeToRoot()` 합성 scale × root relative scale. static 함수 하나로 판정·표시 공유 | CDO component-to-world는 갱신되지 않아 지금 root scale이 빠진다. instance에서는 지금 값과 같다 |
| 축 정렬 검사 | Definition Data Validation에만 "root 기준 pitch·roll 0, Yaw 90° 배수" 오류 추가. runtime 판정에는 넣지 않음 | FPV-014·016 snap 정렬 회귀 방지. runtime에 넣으면 판정 결과가 바뀌는 범위가 명세 밖으로 넓어진다 |

## 4. 대상 파일과 책임 변화

| 파일 | 변경 |
|---|---|
| `Public/Placement/FacilityPlacementSettings.h` | footprint 표시 설정 5개와 getter·loader 추가(5.1) |
| `Public/Placement/FacilityPlacementComponent.h`, `Private/Placement/FacilityPlacementGeometry.cpp` | static footprint full size 함수, `DeriveFootprintCells` scale 항 교체, 축 정렬 검사 함수(5.2) |
| `Private/Placement/FacilityPlacementDefinition.cpp` | `IsDataValid`에 축 정렬 오류 추가(5.2) |
| `Private/Placement/FacilityPlacementFootprintPreview.h/.cpp`(신규) | 순수 surface transform 계산, plane mesh 검사, 표시 재질·색 준비(5.3) |
| `Public/Placement/FacilityPlacementPreviewActor.h`, `Private/Placement/FacilityPlacementPreviewActor.cpp` | footprint component·DMI·색 소유, 메시 우선순위 적용, validity 색 갱신, 테스트 getter(5.4) |
| `Config/DefaultGame.ini` | 새 설정 키(5.5) |
| `Private/Tests/` 신규 파일 | 10절 |

책임 변화:

| 항목 | 판단 |
|---|---|
| 기존 책임 | preview Actor: 메시 복제·validity material·geometry snapshot. Placement component: footprint cell 파생·판정 transform |
| 신규 책임 | footprint 표시 component·DMI·색(preview Actor), surface 계산·재질 준비(private helper), CDO 정확 파생·축 정렬 검사(Placement component) |
| 상태 owner | 표시 component·DMI·두 색: preview Actor transient. 저장 상태 없음 |
| 실행 owner | 기존 preview session(`UPlayerFacilityPlacementComponent`) 그대로. 새 Tick·timer·delegate 없음 |
| authoring owner | 9절 표 |
| 의존 방향 | Placement 내부만. 새 module·plugin 없음(Engine Materials·StaticMesh는 기존 의존) |
| 분리 | preview Actor(176줄)는 component 조립·상위 flow만, 계산·검사는 helper |

## 5. 상세 구현 지시

### 5.1 Settings (`UFacilityPlacementSettings`, Category `Preview|Footprint` 권장)

- `TSoftObjectPtr<UStaticMesh> FootprintPreviewMesh`: 중심 pivot·+Z normal plane.
- `TSoftObjectPtr<UMaterialInterface> FootprintPreviewMaterial`: footprint 표시 MI.
- `float FootprintPreviewFloorOffsetCm`: `ForceUnits="cm"`, ClampMin 0. getter는 non-finite면 C++ 예비값, 음수면 0으로 clamp.
- `int32 FootprintPreviewTranslucencySortPriority`, `int32 PreviewMeshTranslucencySortPriority`: 설명 tooltip에 "grid GridVisual < footprint < 메시 미리보기" 순서 계약을 쓴다. getter 범위 clamp는 엔진 허용 범위(int16)로만 한다.
- loader는 기존 preview material loader와 같은 형태(`IsValid ? Get : LoadSynchronous`). null soft ptr이면 load하지 않는다.
- C++ 기본값은 설정 누락 시 예비값이다. 우선순위 예비값은 footprint < mesh 관계를 지킨다.

### 5.2 footprint 크기 파생과 Data Validation

- `UFacilityPlacementComponent`에 public static 함수(이름 예: `ComputeScaledFootprintFullSize(const FTransform& FootprintRelativeToRoot, const FVector& UnscaledExtent, const FVector& RootRelativeScale)`)를 둔다. 결과 = `2 * UnscaledExtent * (FootprintRelativeToRoot.GetScale3D() * RootRelativeScale).GetAbs()`(footprint 축 기준). 이 식이 판정 `RelativeFootprint * Candidate` 합성과 같은 per-axis scale 규칙임을 주석으로 남긴다.
- `DeriveFootprintCells`: `GetFootprintRelativeToRoot()` 실패 시 실패를 그대로 반환하고, owner root의 `GetRelativeScale3D()`와 위 static 함수로 full size를 구한다. component-to-world scale은 더 쓰지 않는다. 기존 finite·양수·정수배 허용 오차 판정과 문구는 유지한다. root scale이 0·non-finite면 실패.
- 영향 확인: placed instance(`IsOperational`)는 root relative scale = Actor scale이라 결과가 같다. CDO 경로(Definition `ValidateRuntime`·`IsDataValid`, preview 초기화, `ValidateWorldPlacement`의 `ValidateFootprintContractForDefinition`)에서 root scale이 새로 반영된다. 조사 기준으로 root scale이 1이 아닌 활성 설비는 쿨러 하나라 다른 15종 결과는 같다.
- 축 정렬: `UFacilityPlacementComponent`에 `ValidateFootprintGridAxisAlignment(FText&) const`(이름 자유)를 둔다. `GetFootprintRelativeToRoot()` 회전의 pitch·roll이 0이고 Yaw가 90° 배수인지 각도 허용 오차 안에서 본다. 실패 문구는 footprint가 설비 축에서 돌아 있어 grid에 맞지 않는다는 뜻과 원인 component 계층(`SceneRoot` 등) 확인 안내를 담는다.
- `UFacilityPlacementDefinition::IsDataValid`: 기존 footprint cell 검사가 성공한 뒤 축 정렬 검사를 호출해 실패면 `Invalidate`. `ValidateRuntime`과 `DeriveFootprintCells`에는 넣지 않는다.

### 5.3 private helper `FacilityPlacementFootprintPreview`

`Private/Placement/FacilityPlacementFootprintPreview.h/.cpp`, non-UObject namespace. 역할:

1. 순수 계산(엔진 world 불필요):
   - 입력: `FootprintRelativeToRoot`, footprint unscaled extent, placed root relative scale, plane mesh local bounds(`FBoxSphereBounds`), 높이 설정.
   - plane 검사: bounds origin ≈ 0, extent Z ≈ 0, XY 크기 양수·finite. 조건은 Zone grid plane 검사(`AFacilityPlacementZoneActor::QueryGridGeometry`)와 같다. grid 코드는 바꾸지 말고 같은 규칙을 helper에 둔다.
   - 합성 scale `Sf = FootprintRelativeToRoot.GetScale3D() * RootRelativeScale`의 모든 성분이 finite·양수가 아니면 실패.
   - `PlaneLocal` = 회전 identity, translation `(0, 0, -Extent.Z + FloorOffsetCm / Sf.Z)`, scale `(2*Extent.X / PlaneSizeX, 2*Extent.Y / PlaneSizeY, 1)`.
   - 출력: component relative transform `PlaneLocal * FootprintRelativeToRoot`와 world full size XY(5.2 static 함수 결과의 XY).
2. 재질 준비: `FootprintPreviewMaterial` load, translucent blend 확인, `PreviewColor` vector와 `FootprintSizeXCm`·`FootprintSizeYCm` scalar parameter 존재 확인(`GetVectorParameterValue`/`GetScalarParameterValue`가 false면 실패).
3. 색 읽기: 메시 미리보기 valid/invalid 재질에서 `Param` vector 값을 읽는다. 하나라도 없으면 실패.
4. 실패는 `FText` 사유로 반환한다. 로그는 preview Actor가 한 번 남긴다.

계약 parameter 이름 네 개(`Param`, `PreviewColor`, `FootprintSizeXCm`, `FootprintSizeYCm`)는 helper의 named constant로 한 곳에 둔다.

### 5.4 `AFacilityPlacementPreviewActor`

- `InitializeFromPlacedClass`:
  - 메시 component 생성 시 `RegisterComponent` 전에 `SetTranslucentSortPriority(Settings->PreviewMeshTranslucencySortPriority getter)`를 적용한다.
  - 메시 component와 `ApplyPreviewMaterial(InvalidMaterial)`가 성공한 뒤 footprint 표시를 준비한다. 순서: helper 계산(이미 가진 `SourceFootprintRelative`, `SourceFootprintExtent`, `SourceRootScale` snapshot 사용) → 재질 준비·색 읽기 → DMI 생성(`UMaterialInstanceDynamic::Create(Material, this)`) → `FootprintSizeXCm/YCm` 설정 → `PreviewColor` = 무효 색 → component 생성.
  - footprint component: `NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(...FootprintSurface...), RF_Transient)`, `AddInstanceComponent`, `SetupAttachment(SceneRoot)`, mesh·relative transform·material slot 0 = DMI, collision `NoCollision`, overlap·physics 끔, `SetCanEverAffectNavigation(false)`, Tick 끔, `SetCastShadow(false)`, `SetReceivesDecals(false)`, `SetTranslucentSortPriority(footprint 설정)`, `bCastHiddenShadow`·`bAffectIndirectLightingWhileHidden`·`bRayTracingFarField`는 기본(false) 유지, `RegisterComponent`.
  - 표시 준비가 어느 단계에서 실패하면 만든 것을 정리하고(component 미생성 또는 `DestroyComponent`) `UE_LOG(<Placement preview 로그 category>, Warning, ...)`로 사유를 한 번 남긴 뒤 **초기화는 성공으로 계속**한다.
  - 함수 시작의 reset에 footprint component·DMI 포인터 reset을 더한다.
- `SetPlacementValidity`: 기존 메시 재질 적용 뒤, footprint DMI가 있으면 `PreviewColor`를 `bValid ? ValidColor : InvalidColor`로 설정한다.
- `ValidateSourceGeometry`: 변경 없음. 기존 snapshot 비교가 footprint 표시의 근거도 보호한다.
- 헤더: `UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> FootprintSurface`, `UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FootprintMaterial`, 두 `FLinearColor`, const getter `GetFootprintSurface()`. `GetPreviewMeshes()`에는 footprint를 넣지 않는다(기존 테스트가 이 배열로 메시 재질을 검사한다).
- `OnPlacementValidityChanged`, `SceneRoot`, public API는 유지한다.

### 5.5 Config (`Config/DefaultGame.ini` `[/Script/BathhouseSim.FacilityPlacementSettings]`)

- `FootprintPreviewMesh`: Zone grid와 같은 engine plane(`/Engine/BasicShapes/Plane.Plane`, [Unreal/PlacementSystem.md](../../Unreal/PlacementSystem.md) Placement Zone의 GridVisual mesh).
- 높이: `QNA_FEATURE_SPEC.md` 기본값 표의 "바닥 위 높이" 제안(원본 grid 표시 높이 `BP_FacilityPlacementZone`·`BP_BathhouseSpace` Class Default `GridZOffsetCm` 기준)으로 계산한 값.
- 우선순위: footprint = grid `GridVisual` Translucency Sort Priority(조사 보고 4절, Zone Blueprint 값) + 1, 메시 미리보기 = footprint + 1.
- `FootprintPreviewMaterial`은 **쓰지 않는다**. MI가 아직 없어 load 실패 로그가 생기기 때문이다. Editor 단계가 MI를 만든 뒤 Project Settings로 지정한다. 그 전에는 null → 표시 준비 실패 Warning, 메시 미리보기는 정상.

## 6. Lifecycle과 실패 타임라인

| 시점 | 메시 미리보기 | footprint 표시 |
|---|---|---|
| 아이템 듦 → preview spawn·초기화 성공 | 생성, 무효 재질 | 준비 성공 시 생성·무효 색. 실패 시 없음 + Warning |
| 초기화 실패(FPV-015) | Actor 파괴, 설치 불가 | 같은 Actor라 함께 없음 |
| refresh, 후보 있음 | transform·validity 재질·보임 | 같은 transform(자식), 같은 호출에서 색, 보임 |
| refresh, 후보 없음(D4) | 이동 없이 숨김 | owner hidden이라 함께 숨김 |
| geometry mismatch | fail-closed(기존) | Actor와 함께 |
| 확정 성공·G·컴퓨터·손 변경·EndPlay | Actor 파괴 | 함께 파괴. 설치된 설비에는 원래 없음 |
| 확정 실패 | 유지 | 유지 |

새 Tick·timer·delegate·transaction은 없다. rollback 대상 상태가 없다.

## 7. 전역 설정·Blueprint/API·Core Redirect·migration

- 전역 설정: `UFacilityPlacementSettings` 새 키만 추가. Collision·Input·Nav·Project 전역값 변경 없음.
- 메시 미리보기 sort priority 고정은 미리보기 표시에만 영향이 있다. 다른 Actor·grid의 우선순위는 바꾸지 않는다.
- reflected 변경: preview Actor에 `Transient` UPROPERTY 두 개, Settings에 Config UPROPERTY 다섯 개. rename·삭제 없음 → Core Redirect 불필요. preview Actor는 spawn 전용이고 저장된 Blueprint 파생이 없다(옛 `BP_FacilityPreview_*` 삭제됨).
- `UFacilityPlacementComponent`·`UFacilityPlacementDefinition`: UPROPERTY 변경 없음. serialized layout 불변.
- 동작 영향: CDO 파생이 root scale을 반영하므로 Editor 단계 전까지 쿨러는 preview 초기화(=배치)와 Data Validation이 실패한다. 순환기·보일러는 Data Validation만 오류이고 runtime은 지금과 같다. 같은 작업의 Editor 단계가 해소한다.
- (추론, 미검증) 지금 Level의 쿨러 instance는 component-to-world scale 0.5로 `IsOperational`의 cell 검사가 이미 실패할 수 있다. 수정 뒤에는 해소된다. 사용자 PIE FPV-016에서 회수·재배치를 함께 본다.

## 8. Editor 단계 인계 (`PROMPT_UNREAL.md` 작성 기준)

구현 단계가 이 절을 바탕으로 `PROMPT_UNREAL.md`를 쓴다. S2: 범위 밖 asset 변경은 진행하고 보고에 asset·값·이유를 적는다. `Content/Bathhouse/Blueprints/Facility/BP_ClothesLocker.uasset`은 저장·커밋하지 않는다.

1. `/Game/Bathhouse/Materials/Placement/M_FacilityPlacementFootprint`(Material)
   - Surface, Translucent, Unlit, one-sided, depth test 켬, translucency pass는 `M_FacilityPlacementGrid`·`/Game/Material/M_Preview`와 같은 값.
   - parameter: `PreviewColor`(vector), `FootprintSizeXCm`·`FootprintSizeYCm`(scalar, native 설정), `FillOpacity`·`OutlineOpacity`·`OutlineThicknessCm`(scalar, MI 원본).
   - graph: plane UV에서 가장 가까운 변까지의 거리를 cm로 구한다(UV 축↔plane local X/Y 대응은 같은 plane을 쓰는 grid material의 `ZoneSizeXCm/YCm` 연결과 같게, `FootprintSizeXCm`이 plane local X). 그 거리가 `OutlineThicknessCm`보다 작으면 외곽선(사각형 안쪽). Opacity = 외곽선 ? `OutlineOpacity` : `FillOpacity`, Emissive = `PreviewColor` RGB.
2. `/Game/Bathhouse/Materials/Placement/MI_FacilityPlacementFootprint`(MI, 부모 위 Material): `FillOpacity`·`OutlineOpacity`·`OutlineThicknessCm`만 `QNA_FEATURE_SPEC.md` 기본값 표 제안값으로 설정. native parameter 셋은 override하지 않는다.
3. Project Settings `Facility Placement` `FootprintPreviewMaterial` = 위 MI(DefaultGame.ini 저장 확인). 나머지 새 설정은 구현이 넣은 값 확인만.
4. `BP_Cooler`: `PlacementFootprint` BoxExtent X/Y만 바꿔 world footprint를 Actor 축 X 2칸·Y 3칸으로 한다(FPV-016). `SceneRoot` Yaw −90° 때문에 footprint local X가 Actor Y다. 각 local 축 extent = 그 축 칸 수 × `GridSizeCm` ÷ (2 × |root scale × 하위 합성 scale|). Z extent, relative location·rotation, root scale은 그대로.
5. `BP_Circulator`·`BP_Boiler`: native `SceneRoot` relative rotation을 0으로(Yaw −1° 제거). 하위 component relative 값은 그대로.
6. `DefaultMap`의 세 설비 instance: `SceneRoot` rotation·`PlacementFootprint` extent instance override가 있으면 Class Default로 되돌리고 보고한다. 없으면 Map 저장하지 않는다. 쿨러 footprint가 넓어져 인접 물체와 겹치면 사실만 보고한다(위치 이동은 하지 않음).
7. Data Validation: 활성 16개 Definition 모두 오류 없음(기존 Cube fallback 경고는 허용). 순환기·보일러는 수정 전 축 정렬 오류, 쿨러는 비정수 오류가 나는지 먼저 보고 수정 뒤 사라지는지 확인한다.
8. content 테스트 `BathhouseSim.PlacementContent.FootprintPreview`(10절 T7) 실행.
9. `.md/Unreal/PlacementSystem.md` 갱신: 새 재질·MI·설정 위치, 세 BP footprint·`SceneRoot` 현재 상태, 미리보기 재질 실제 속성(조사 보고 6절 불일치 포함). 수치는 원본 위치 참조로.
10. 화면 판정(가림·비침·순서·깜빡임)은 하지 않고 `PIE_CHECKLIST.md` 대상으로 넘긴다(FBK-003).

## 9. 조정값 원본과 code 상수 예외

| 값 | 원본 |
|---|---|
| grid 간격 | Project Settings `Facility Placement > Grid Size Cm` |
| footprint 크기 | 각 배치 설비 Blueprint `PlacementFootprint` extent와 상위 component·root scale |
| 유효·무효 색 | Project Settings `ValidPreviewMaterial`·`InvalidPreviewMaterial`가 가리키는 재질의 `Param` |
| 채움·외곽선 불투명도, 외곽선 두께(cm) | `MI_FacilityPlacementFootprint` 기본값 |
| 바닥 위 높이 | Project Settings `FootprintPreviewFloorOffsetCm` |
| 그리기 우선순위 | Project Settings 두 우선순위 값, grid는 Zone Blueprint `GridVisual` Translucency Sort Priority |
| plane mesh·표시 MI | Project Settings `FootprintPreviewMesh`·`FootprintPreviewMaterial` |

code 상수 예외(엔진·asset 계약 의미, 조정값 아님):

- 계약 parameter 이름 4개(5.3).
- full size의 `2 *`(half extent → full), plane local Z scale 1, 회전 identity.
- 수치 비교 허용 오차: plane 중심·두께 판정, 축 정렬 각도 판정. 기존 cell 정수배 허용 오차는 그대로 둔다.
- Settings C++ 기본값은 Config 누락 시 예비값이다.

## 10. 자동화 테스트

새 파일 `Private/Tests/FacilityPlacementFootprintPreviewAutomationTests.cpp`(필요하면 probe header 신규). 1860줄 `FacilityPlacementAutomationTests.cpp`에는 추가하지 않는다. 기대값은 fixture에 넣은 값에서 계산하고 리터럴 결과를 복제하지 않는다. 전역 Settings를 바꾸는 테스트는 저장·복원한다.

- T1 `FootprintCellsIncludeRootScale`: 등록하지 않은(CDO처럼 component-to-world 미갱신) fixture에 root relative scale과 하위(`SceneRoot`) scale을 주고 cell이 `round(static full size / grid)`와 같은지. 기존 component-to-world 방식이면 실패했을 조합이어야 한다.
- T2 비정수 검출: unscaled는 정수배, root scale 적용 뒤 비정수인 fixture(쿨러 형태)가 `DeriveFootprintCells`와 Definition `IsDataValid`에서 실패.
- T3 축 정렬: footprint 합성 Yaw 0·90·−90은 통과, 90° 배수가 아닌 작은 Yaw는 `IsDataValid` Invalid. 같은 class의 `ValidateRuntime`은 cell이 정수면 여전히 성공(runtime 불변). Definition 검사는 class CDO를 쓰므로 생성자에서 회전을 주는 probe class가 필요하다.
- T4 surface 순수 계산: footprint relative에 Yaw −90°·위치 offset, root uniform scale ≠ 1인 fixture. 임의 후보 Actor transform에서 plane 네 모서리 world 위치가 `RelativeFootprint * Candidate` 바닥 네 모서리 + floor normal × 높이와 같은지, world size 출력이 static 함수와 같은지. 실패 경우: 중심이 어긋난 plane bounds, 두께 있는 bounds, 0 크기, 합성 scale 0 이하.
- T5 준비 실패 fallback: 기존 preview 테스트처럼 `Param` 없는 transient 반투명 재질과 null `FootprintPreviewMaterial`로 `InitializeFromPlacedClass` 성공, `GetFootprintSurface() == nullptr`, `GetPreviewMeshes()` 각 component sort priority = 설정값, `SetPlacementValidity` 정상.
- T6 준비 성공(가능하면): Editor context에서 `UMaterialExpressionVectorParameter`·`ScalarParameter`를 가진 transient Material을 만들어(`UpdateCachedExpressionData` 등) parameter 조회가 되면 footprint component 존재, sort priority, `SetActorTransform` 뒤 world 모서리(T4 기준), `SetPlacementValidity(true/false)`의 DMI `PreviewColor` = 각 재질 `Param`, `SetActorHiddenInGame(true)` 뒤 component `ShouldRender()` 거짓을 확인한다. 엔진에서 transient parameter 조회가 안정적이지 않으면 T6를 만들지 않고 `PROMPT_REVIEW.md`에 근거를 적는다(T7이 대신한다).
- T7 content `BathhouseSim.PlacementContent.FootprintPreview`(구현 단계는 작성·컴파일만, Editor 단계 8항에서 실행): Asset Registry의 `UFacilityPlacementDefinition` 중 `PlacedFacilityClass`가 있는 것마다 임시 Game world에서 preview Actor를 초기화해 footprint component가 있고, world 크기 = `DeriveFootprintCells` × `GridSizeCm`, 색 = 실제 preview 재질 `Param`인지 확인한다. `BP_ClothesLocker`는 디스크 상태 그대로 읽기만 한다.
- 회귀: 기존 `BathhouseSim.Placement.*` 전부 통과(특히 `PreviewHiddenWithoutAim`, preview material·Blueprint preview, DeriveFootprintCells 기존 검사).

## 11. 빌드·리뷰 기준

- 빌드와 headless automation은 [UE_BUILD_POLICY.md](../../UE_BUILD_POLICY.md) 고정 명령을 따른다. 필터 `BathhouseSim.Placement`(T7 제외).
- 리뷰 확인:
  - `UPlayerFacilityPlacementComponent`·transaction·Zone grid·설비 Actor class 무변경.
  - footprint 표시가 판정 경로(`ValidateCurrentPlacement`/`ValidateWorldPlacement`)에 관여하지 않는다.
  - 표시 준비 실패가 preview 초기화 실패로 번지지 않고 Warning 한 번.
  - cell 파생과 표시 크기가 같은 static 함수를 쓴다.
  - 조정값 code 상수 없음(9절 예외만).
  - footprint component가 collision·Nav·Tick·shadow를 쓰지 않는다.
- `PROMPT_REVIEW.md`에 7절 동작 영향(쿨러 일시 배치 불가)과 T6 수행 여부를 적는다.

## 12. 사용자 PIE에서 관찰할 시나리오

- Vanity(홀): FPV-001~009. 특히 FPV-002 메시 아래 footprint 외곽선이 반투명 메시를 통해 알아볼 만한지, FPV-003 snap 시 네 변과 grid 선, FPV-005 벽·설치 설비 발밑까지 닿은 여백과 가림, FPV-007 숨김에서 footprint만 남지 않음.
- FPV-010~015: DrinkFridge·Boiler, Shower·Bath(바닥면과 깜빡임 없음), 세 공간 높이, 판정 회귀, 16종, 초기화 실패.
- FPV-016: 쿨러 2×3칸·snap 정렬·좁은 자리 무효 가능, 순환기·보일러 반듯함, Level 기존 instance와 회수·재배치·잔량.
- 비침이 부족하면 조정 위치: MI 불투명도, 두 우선순위 설정(3절).

## 13. 구현 금지 범위

- Content·`.uasset`·`.umap` 수정, Editor 실행으로 asset 저장(Editor 단계 몫). `BP_ClothesLocker` 작업 트리 변경 건드리지 않음.
- `UPlayerFacilityPlacementComponent`, `FacilityActorConversionTransaction`, Zone grid(`FacilityPlacementZoneGrid.cpp`), 설비 Actor class, 판정 규칙·안내 문구 변경.
- 메시 미리보기 재질·색·반투명 변경, 설치 설비·회수 대상 footprint 표시, 표시 on/off 입력.
- `FootprintPreviewMaterial` Config 값 기록(5.5).
- 조정값을 code 상수로 두기, Error 로그로 표시 실패 알리기.
