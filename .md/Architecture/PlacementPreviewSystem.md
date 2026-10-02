# Placement Preview System

배치 미리보기 표현(설비 메시 미리보기, footprint 표시, 조준 없음 숨김, 호환 Zone grid)의 책임과 계약이다. 배치 판정·transaction·footprint 정본은 [PlacementSystem.md](PlacementSystem.md)에 있다. 2026-10-02 `PLACEMENT-FOOTPRINT-PREVIEW` 설계로 PlacementSystem.md(300줄 초과)에서 분리했다.

## 상태

- Generic Native Preview, Preview Without Aim, Compatible Zone Grid: Source 반영.
- Footprint Display와 Draw Order: 2026-10-02 설계, Source 미반영(`PLACEMENT-FOOTPRINT-PREVIEW`).

## Source Scope

```text
Source/BathhouseSim/Public/Placement/
  FacilityPlacementPreviewActor.h
  FacilityPlacementSettings.h          (preview·footprint 표시 설정)
Source/BathhouseSim/Private/Placement/
  FacilityPlacementPreviewActor.cpp
  FacilityPlacementPreviewSource.h/.cpp
  FacilityPlacementFootprintPreview.h/.cpp   (설계, footprint 표시 계산·재질 준비 private helper)
  FacilityPlacementZoneGrid.cpp
  PlayerFacilityPlacementComponent.cpp       (session·숨김·grid 표시 조율)
```

## Owners

| 책임 | Owner |
|---|---|
| preview session, 후보 transform, 숨김, 호환 Zone grid weak set | `UPlayerFacilityPlacementComponent` |
| 메시 미리보기 component·validity material, footprint 표시 component·DMI·색 | `AFacilityPlacementPreviewActor` |
| footprint 표시 transform·크기 계산, 표시 재질 준비·색 읽기 | private `FacilityPlacementFootprintPreview` helper |
| footprint world 크기 파생식 | `UFacilityPlacementComponent`(static, 판정과 공유) |
| preview·footprint 재질/메시 참조, footprint 높이, 그리기 우선순위 | `UFacilityPlacementSettings` |
| footprint 채움·외곽선 불투명도, 외곽선 두께 | footprint 표시 MI 기본값(Editor) |
| Zone grid 표현값과 DMI | `AFacilityPlacementZoneActor` |

## Generic Native Preview

`AFacilityPlacementPreviewActor` 하나를 직접 spawn하며 Definition별 preview class를 조회하지 않는다. 이 class는 Blueprint 파생과 domain 기능 없이 transient 표현만 담당한다.

preview 초기화는 gameplay Actor를 spawn하지 않고 `PlacedFacilityClass`의 native CDO component와 Blueprint SCS hierarchy를 함께 순회한다. inherited SCS override는 최종 generated class 기준 template을 사용하며 cooked fast-path component data가 있으면 preview Actor에 미등록 scratch component로 materialize한 뒤 표현값만 읽고 제거한다. 다음 source를 읽는다.

- 유효 mesh가 있고 class-default에서 표시되는 non-instanced `UStaticMeshComponent`
- source Actor root 기준으로 계산한 component transform
- visibility와 필요한 기본 render 속성
- placement component의 footprint relative transform과 BoxExtent snapshot

helper, hidden, editor-only, `UInstancedStaticMeshComponent`와 runtime contents/pile/water 표현은 복제하지 않는다. 각 source마다 transient `UStaticMeshComponent`를 만들고 preview root에 root-relative transform으로 붙인다. source material은 복제하지 않고 validity에 따라 모든 slot을 `ValidPreviewMaterial` 또는 `InvalidPreviewMaterial`로 설정한다.

preview Actor와 생성 component는 collision/overlap/physics/Tick과 Navigation을 항상 끈다. placed class identity, footprint root-relative transform/extent와 root scale snapshot은 매 refresh/confirm에서 authoritative CDO와 비교하며 mismatch면 preview와 confirm을 fail-closed한다. authoritative footprint는 계속 placed CDO다. eligible mesh가 없거나 hierarchy 해석, component 생성·material 적용이 실패하면 live preview가 성립하지 않으므로 confirm을 허용하지 않는다.

두 preview material은 preview 시작 시 resolve되고 translucent blend를 제공해야 한다. 누락·load·blend 검증 실패는 preview 초기화 실패이고 placement는 held item을 보존한 채 fail-closed한다.

## Footprint Display (2026-10-02 설계)

기능 계약: `PLACEMENT-FOOTPRINT-PREVIEW` FPV-001~016. 들고 있는 설비의 `PlacementFootprint` XY 사각형을 메시 미리보기와 함께 바닥에 표시한다.

구조:

- preview Actor가 transient `UStaticMeshComponent` 하나(`FootprintSurface`)를 preview root 아래에 소유한다. `PreviewMeshes` 배열에 넣지 않는다.
- 같은 Actor의 component이므로 후보 transform, Yaw, LCtrl snap, 숨김(D4), 파괴(확정·취소·실패)를 메시 미리보기와 자동으로 공유한다. `UPlayerFacilityPlacementComponent`는 바꾸지 않는다.
- 설치된 설비·Q 회수 대상에는 표시하지 않는다(preview Actor만 소유).

transform:

- 표시 메시는 Project Settings의 중심 pivot·+Z normal plane mesh 하나다. 판정 조건은 Zone grid plane과 같다(중심 원점, Z 두께 0, 양의 XY bounds).
- `FootprintRelativeToRoot`(preview 초기화 snapshot)의 축을 그대로 쓴다. plane local transform은 회전 없이 XY scale = footprint unscaled full XY ÷ plane bounds XY, Z = footprint 바닥(−Extent.Z) + 높이 설정 ÷ |합성 Z scale|이다. component relative transform = `PlaneLocal * FootprintRelativeToRoot`.
- 결과 world 사각형은 `ValidateWorldPlacement`의 `RelativeFootprint * Candidate` 바닥면과 크기·중심·방향이 같고, 설치 바닥에서 높이 설정만큼 위에 있다. 합성 scale 성분이 0 이하이거나 plane 조건이 틀리면 표시 준비 실패다.

재질과 색:

- 표시 재질은 Project Settings의 footprint MI다. preview Actor가 DMI를 한 번 만들고 native는 `PreviewColor`(vector), `FootprintSizeXCm`·`FootprintSizeYCm`(world full size, scalar)만 설정한다. 채움·외곽선 불투명도와 안쪽 외곽선 두께(cm)는 MI 기본값이 원본이며 native가 덮어쓰지 않는다.
- 색 원본은 메시 미리보기 재질 하나다(Q2 A). 초기화 때 `ValidPreviewMaterial`·`InvalidPreviewMaterial`의 vector parameter `Param` 값을 읽어 두고, `SetPlacementValidity`가 메시 재질과 같은 호출에서 `PreviewColor`를 바꾼다. 초기 색은 메시 초기 재질과 같은 무효 색이다.
- 계약 parameter 이름(`Param`, `PreviewColor`, `FootprintSizeXCm`, `FootprintSizeYCm`)은 asset 계약 이름이라 code 상수다. 표시 재질에 native parameter 셋이 없거나 preview 재질에 `Param`이 없으면 표시 준비 실패다.

표시 준비 실패:

- footprint 표시만 만들지 않고 메시 미리보기·배치 판정·확정은 그대로 진행한다(명세 7절). 판정과 다른 색의 표시는 생기지 않는다.
- preview 초기화마다 Warning 로그 한 번으로 사유를 남긴다. 자동화가 log Error를 실패로 세지 않도록 Error를 쓰지 않는다.

## Draw Order

FBK-004 권장에 따라 그리기 조건을 엔진 소스(UE 5.8)로 확인했다.

| 조건 | 근거 | 계약 |
|---|---|---|
| 숨김 | `USceneComponent::ShouldRender()`는 owner `IsHidden()`이면 거짓(Preview Without Aim) | footprint 표시는 hidden shadow·indirect·far-field flag를 켜지 않는다 |
| 반투명 순서 | `CalculateTranslucentMeshStaticSortKey`: sort key 첫 기준이 primitive `TranslucencySortPriority`(낮은 값 먼저), 다음이 거리 | grid < footprint < 메시 미리보기 순서로 그린다 |
| pass | 반투명 우선순위는 같은 translucency pass 안에서만 비교된다 | footprint 재질은 grid·preview 재질과 같은 pass(현재 After DOF)를 쓴다 |
| 가림 | 반투명은 기본 depth test를 하고 depth를 쓰지 않는다 | depth test를 끄지 않는다. 벽·설치 설비 같은 불투명 물체에 가려지고(Q3 A), 반투명끼리는 순서로만 겹친다 |
| 면 방향 | one-sided plane, +Z normal | 카메라는 항상 바닥 위다. 음수 합성 scale은 표시 준비 실패 |

- footprint 우선순위와 메시 미리보기 우선순위는 Project Settings 값이다. 메시 미리보기 component는 지금 엔진 기본 우선순위로 grid와 거리 순서를 다투므로, 설정값을 적용해 grid 위로 고정한다(재질·반투명·색은 그대로).
- grid `GridVisual` 우선순위는 Zone Blueprint 값이다. footprint 설정값은 그보다 커야 하고 메시 미리보기 설정값은 footprint보다 커야 한다. PIE에서 메시 아래 footprint가 잘 안 보이면 두 우선순위나 MI 불투명도로 조정한다(FPV-002·010·011 관찰).
- 높이 설정은 불투명 바닥과의 z-fighting만 막는다. grid·Bath 메시 바닥면과의 앞뒤는 위 우선순위가 정한다.

## Preview Without Aim (2026-10-02 EXP-U3, D4)

- 미리보기는 이번 갱신에서 후보 transform을 계산했을 때만 보인다. 후보 계산 = `TracePlacementZone` 성공(배치 거리 `PlacementTraceDistance` 안에서 배치 trace 채널 첫 hit가 `IsZoneSurfaceHit`) → `MakeCandidateTransform` → `BuildPlacedActorTransform` 성공. `ValidateCurrentPlacement`가 이를 out flag로 알린다.
- 후보가 없으면(거리 밖, 벽·천장·계단 형상, 구역 없는 바닥·하늘, 후보 계산 전 상태 이상) `RefreshPreview`는 미리보기 Actor를 옮기지 않고 `SetActorHiddenInGame(true)`로 숨긴다. 후보가 있으면 옮기고 validity material을 적용한 뒤 보이게 한다. 공간 불허·락커 한도·구역 밖·겹침·바닥 지지 부족은 후보가 있으므로 조준한 자리에 invalid material로 보인다.
- 미리보기 Actor는 파괴하지 않는다. 누적 Yaw·LCtrl·호환 Zone 격자·안내 문구(`설치 가능한 구역을 바라보세요.`)·확정 실패 시 item 보존은 그대로다. `SetActorHiddenInGame`은 값이 바뀔 때만 render state를 다시 만든다.
- 그리기 조건(UE 5.8 엔진 소스): game world에서 `USceneComponent::ShouldRender()`는 owner `IsHidden()`이면 거짓이고, primitive proxy는 `ShouldRender() || bCastHiddenShadow || bAffectIndirectLightingWhileHidden || bRayTracingFarField`일 때만 생긴다. 미리보기 mesh와 footprint 표시는 뒤 세 flag를 켜지 않는다.

## Compatible Zone Grid Presentation

`AFacilityPlacementZoneActor`는 `PlacementFloor` 아래 native `GridVisual` `UStaticMeshComponent`를 stable default subobject로 소유한다. component는 기본 hidden이며 collision, overlap, physics, Tick과 Navigation을 사용하지 않는다. Blueprint는 inherited component에 중심 pivot/+Z normal을 가진 plane mesh와 `MI_FacilityPlacementGrid` 하나만 지정한다.

Zone별 authoring 값은 `GridLineThicknessCm`, `GridZOffsetCm`, `MajorGridIntervalCells`다. Class Default와 Level instance override를 허용하되 실제 셀 크기는 계속 `UFacilityPlacementSettings.GridSizeCm` 하나가 정본이다. line thickness는 양수이면서 셀 반폭 미만으로 runtime clamp·validation하고, Z offset은 non-negative, major interval은 2 이상의 cell 수로 제한한다.

native `OnConstruction`은 plane mesh의 local bounds와 `ZoneBounds` unscaled full X/Y를 사용해 `GridVisual` relative scale을 파생하고, XY 중심을 `PlacementFloor` origin에 맞춘 뒤 `GridZOffsetCm`을 relative Z에 적용한다. Actor/Zone world scale을 값에 다시 곱하지 않는다. mesh 없음, 0/non-finite bounds와 floor/Bounds 축 불일치는 authoring 오류다.

각 runtime Zone instance는 source MI를 변경하지 않고 최초 표시 전에 DMI를 한 번 생성한다. DMI에는 정확히 `GridSizeCm`, `ZoneSizeXCm`, `ZoneSizeYCm`, `LineThicknessCm`, `MajorGridEveryNCells`를 설정한다. minor/major 색상, opacity와 major thickness ratio는 MI 기본값이며 native 코드가 덮어쓰지 않는다. 가시성은 opacity가 아니라 component visibility로 전환한다.

`SetGridVisible(bool)`은 실제 상태가 바뀔 때만 component를 전환하고 기존 `OnGridVisibilityChanged`를 그 뒤 한 번 notification한다. DMI/material 준비 실패는 같은 Zone에서 한 번만 진단하며 placement domain 상태를 변경하지 않지만 Editor 통합 완료 조건은 실패한다.

`UPlayerFacilityPlacementComponent`는 설비 preview Actor 초기화 성공 시 world의 level-authored PlacementZone을 한 번 수집하고 `IsDefinitionAllowed()`가 참인 모든 Zone을 표시한다. 조준 중인 `PreviewZone`은 후보 계산만 담당하며 grid 대상 선택에 사용하지 않는다. visible Zone은 weak array로 보관하고 held item 변경, confirm 성공, preview 실패·취소, suppression과 EndPlay에서 모두 숨긴다. tick refresh는 Zone을 다시 검색하거나 visibility event를 반복 발행하지 않는다. active session 중 runtime spawn/retag된 Zone의 hot-add는 이번 범위 밖이다.

## Settings

`UFacilityPlacementSettings`(Project Settings `Facility Placement`, `Config/DefaultGame.ini`)의 preview 항목이다. 값은 문서에 적지 않는다.

- `ValidPreviewMaterial`, `InvalidPreviewMaterial`: 메시 미리보기 재질이자 footprint 색 원본(`Param`)
- (설계) `FootprintPreviewMesh`: 중심 pivot·+Z normal plane
- (설계) `FootprintPreviewMaterial`: footprint 표시 MI
- (설계) `FootprintPreviewFloorOffsetCm`: 설치 바닥 위 높이, finite·0 이상으로 clamp
- (설계) `FootprintPreviewTranslucencySortPriority`, `PreviewMeshTranslucencySortPriority`: Draw Order 절의 순서 계약

## Blueprint/API

- preview Actor는 Blueprint 파생 없이 native class로 spawn한다. `OnPlacementValidityChanged`는 유지한다.
- `GetPreviewMeshes()`는 메시 미리보기 component만 반환한다. footprint 표시는 별도 getter로 조회한다(테스트용).
- Editor authoring 위치는 [Unreal/PlacementSystem.md](../Unreal/PlacementSystem.md)다.

## Verification

- preview 시작 시 조준과 무관하게 compatible Zone 전체만 grid가 표시되고 종료 경로마다 모두 숨겨지는지 확인한다.
- (EXP-U3) 조준 없음이면 미리보기 Actor가 유지된 채 숨고 transform이 그대로이며, 다시 구역을 조준하면 새 후보 위치에 보이고 누적 Yaw가 유지되는지, 공간 불허·겹침은 숨지 않는지 확인한다.
- GridVisual 한 개가 Bounds 전체를 덮고 전역 cell 간격, Zone별 line thickness/Z offset/major interval을 DMI와 transform에 반영하는지 확인한다.
- Zone grid가 중립색 하나를 유지하고 normal depth test로 벽·설비 뒤에서 가려지는지 확인한다.
- generic preview가 class-default 복합 mesh를 복제하고 valid/invalid material을 모든 slot에 적용하는지 확인한다.
- (설계) footprint 표시 world 사각형이 회전·root scale fixture에서 판정 footprint 바닥면 + 높이와 같은지, 표시 준비 실패 시 메시 미리보기와 판정이 그대로인지 확인한다. 화면 가림·비침·그리기 순서는 사용자 PIE로 확인한다.
