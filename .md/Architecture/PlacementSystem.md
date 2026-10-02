# Placement System

2026-09-24 Utility Labor Source: utility conversion payload에 optional operation-state marker와 잔량을 추가하고 기존 actor conversion transaction에 clock stop/restart hook을 연결했다. 회수 Hold는 연료 감소를 멈추지 않고, rollback은 Hold 시작 잔량을 복원하지 않는다. 코드/PIE 검증은 미실행이며 Editor 단계가 남아 있다.

## Implementation Status

배치 설비 Actor와 전용 `APlaceableFacilityItemActor`의 staged 양방향 transaction, 공통 Held 설정, footprint 파생 cell, 명시적 zone floor, 범용 native preview, 기본 collision 기반 Dynamic Navigation, pre-placed 락커 reconciliation 및 호환 PlacementZone 전체의 native 그리드 표현은 Source와 native automation까지 구현되어 있다. 전용 grid Material과 기존 Definition/Blueprint/Level migration의 Editor·PIE 검증이 필요하다.

## Source Scope

```text
Source/BathhouseSim/Public/Placement/
  FacilityPlacementTypes.h
  FacilityPlacementPayload.h
  FacilityPlacementSettings.h
  PlaceableFacility.h
  FacilityPlacementDefinition.h
  FacilityPlacementComponent.h
  PlaceableFacilityItemActor.h
  FacilityPlacementZoneActor.h
  FacilityPlacementPreviewActor.h
  PlayerFacilityPlacementComponent.h

Source/BathhouseSim/Private/Placement/
  FacilityPlacementCollisionUtils.h/.cpp
  FacilityActorConversionTransaction.h/.cpp
  FacilityPlacementDefinition.cpp
  FacilityPlacementPayload.cpp
  FacilityPlacementSettings.cpp
  FacilityPlacementComponent.cpp
  FacilityPlacementGeometry.cpp
  FacilityPlacementZoneActor.cpp
  FacilityPlacementZoneGrid.cpp
  FacilityPlacementPreviewActor.cpp
  FacilityPlacementPreviewSource.h/.cpp
  PlaceableFacilityItemActor.cpp
  PlaceableFacilityItemCollision.cpp
  PlayerFacilityPlacementComponent.cpp
  PlayerFacilityPlacementGrid.cpp
  PlayerFacilityPlacementValidation.cpp

Source/BathhouseSim/Public|Private/Facility/
  BathhouseFacilityActor.*
  BathhouseFacilityPlacementDomain.cpp
  BathhouseFacilitySubsystem.*
  BathhouseFacilityStartup.cpp
  LockerCapacitySubsystem.*
  BathhouseExpansionAuthority.*

Source/BathhouseSim/Public|Private/Towel/
  TowelProcessingMachineActor.*

Source/BathhouseSim/Private/Tests/
  FacilityPlacementAutomationTestProbe.*
  FacilityPlacementAutomationTests.cpp
```

## Responsibilities

- 배치 설비 Actor와 전용 회수 아이템 Actor의 분리된 lifecycle 및 Definition/payload
- held 설비 아이템의 preview, zone trace, grid/snap/rotation과 Actor 교체 commit
- preview 세션 동안 호환되는 모든 PlacementZone의 중립색 grid 표시·정리
- footprint 기반 크기·바닥 정렬·구역 포함·collision·floor support 검증
- 설치 설비의 Q Hold 회수 query/progress/cancel/commit
- domain별 Q Hold begin/end hook과 cancel/실패 시 exact 상태 복원
- placed Actor collision과 Dynamic Navigation lifecycle 조율
- 확장 단계별 락커 수용량과 pre-placed 락커의 결정적 초기 등록

Placement는 설비 contents, 목욕탕 물, customer 행동, key 상태와 UI hierarchy를 변경하지 않는다. 설비별 조건은 `IPlaceableFacility`과 원래 domain owner가 판정한다.

## State Owners

| 책임 | Owner |
|---|---|
| snap grid cell 크기, Yaw 간격, 회수 시간·거리, 공통 Held 위치·회전, 설비 preview 머터리얼 | `UFacilityPlacementSettings` |
| stable id, facility tag, placed/item class, recovery item mesh, locker slot 수 | `UFacilityPlacementDefinition` |
| placed Definition, footprint, transition와 Actor collision snapshot | `UFacilityPlacementComponent` |
| payload, recovery item mesh physics와 lifecycle | `APlaceableFacilityItemActor` |
| preview session, 호환 Zone grid weak set, 누적 Yaw, Q target/경과 시간과 상위 transaction 조율 | `UPlayerFacilityPlacementComponent` |
| 범용 preview의 transient mesh·footprint 표시와 validity 표현 | `AFacilityPlacementPreviewActor`([PlacementPreviewSystem.md](PlacementPreviewSystem.md)) |
| zone bounds, 명시적 floor plane, allowed tag, native GridVisual/DMI와 Zone별 표현값 | `AFacilityPlacementZoneActor` |
| expansion readiness, pending locker와 startup reconciliation | `UBathhouseFacilitySubsystem` |
| 설치 락커 용량, lease, action-slot 후보와 bank 등록 | `ULockerCapacitySubsystem` |
| 설비별 회수 조건 | `IPlaceableFacility` 구현 Actor와 해당 domain owner |
| Q Hold 동안의 domain freeze snapshot | 해당 `IPlaceableFacility` 구현 Actor; Bath는 `UBathWaterStateComponent`와 control 표현 snapshot |
| 실제 held Actor identity | `UPlayerCarryComponent` |

## Global Settings

`UFacilityPlacementSettings : UDeveloperSettings`가 Project Settings의 공통 authoring 정본이다.

- `GridSizeCm`: 실제 값은 Project Settings(`Facility Placement > Grid Size Cm`, `Config/DefaultGame.ini`)가 정본이며 문서에 수치를 기록하지 않는다. C++ 기본값은 설정이 없을 때의 예비값이다. 값을 바꾸면 모든 Definition footprint가 새 간격의 정수배인지 Data Validation으로 확인한다. 자동화는 이 값에 의존하지 않도록 필요한 간격을 test 범위 안에서 고정한다.
- `RotationStepDegrees`
- `RecoveryHoldSeconds`
- `RecoveryDropZOffsetCm`
- `PlacementTraceDistance`, `RecoveryTraceDistance`
- `FacilityItemHeldTransform`
- `ValidPreviewMaterial`, `InvalidPreviewMaterial`
- footprint 표시 plane mesh·MI, 바닥 위 높이, 반투명 그리기 우선순위 두 개. 의미와 계약은 [PlacementPreviewSystem.md](PlacementPreviewSystem.md) Settings

`FacilityItemHeldTransform` getter는 location/rotation만 반환하고 scale을 항상 `OneVector`로 정규화한다. `APlaceableFacilityItemActor::GetHeldTransform()`과 legacy placed Actor의 fail-closed carry getter는 이 설정만 읽으며 per-Actor 값을 소유하지 않는다.

두 preview material은 config에 저장 가능한 soft asset reference다. 사용 계약은 [PlacementPreviewSystem.md](PlacementPreviewSystem.md) Generic Native Preview에 있다.

## Definition And Footprint

`UFacilityPlacementDefinition`은 다음 값만 소유한다.

- `StableId`, `FacilityTags`
- `PlacedFacilityClass`
- 공통 `APlaceableFacilityItemActor` 파생 `RecoveryItemClass`
- 설비별 `RecoveryItemMesh`
- locker인 경우 `LockerSlotCount`

`PreviewActorClass`와 `FootprintCellsX/Y`는 즉시 제거한다. 모든 Definition은 공통 native preview를 사용하고 실제 footprint는 `PlacedFacilityClass` CDO의 `UFacilityPlacementComponent`가 참조하는 `PlacementFootprint` 하나가 정본이다.

footprint cell은 저장하지 않고 요청 시 파생한다.

```text
FullSizeXY = 2 * BoxExtentXY * abs(FootprintComponentScaleXY) * abs(PlacedRootScaleXY)
CellsXY = round(FullSizeXY / GridSizeCm)
```

각 축의 full size는 양수·finite이고 `GridSizeCm`의 정수배여야 한다. 허용 오차 안에서 정수배가 아니면 Definition/Data Validation과 runtime placement가 실패한다. grid 또는 footprint를 바꾸면 cell 값을 별도로 갱신하지 않는다.

파생식 적용(2026-10-02 `PLACEMENT-FOOTPRINT-PREVIEW`, 사용자 PIE 통과 2026-10-03):

- 이전 `DeriveFootprintCells`는 `PlacementFootprint` component-to-world scale을 써서, component-to-world를 갱신하지 않는 Blueprint CDO 검사(Definition·Data Validation·preview 초기화)에서 root scale과 하위 component scale이 빠졌다(쿨러 비정수 footprint 미검출).
- 위 식의 scale 항은 `GetFootprintRelativeToRoot()` 합성 scale × root relative scale로 계산한다. CDO와 instance에서 같은 값이며(instance root relative scale = Actor scale), 판정의 `RelativeFootprint * Candidate`와 같은 FTransform 합성 규칙이다. 식 자체는 `UFacilityPlacementComponent::ComputeScaledFootprintFullSize` 하나로 두고 cell 파생과 footprint 표시가 함께 쓴다.
- Data Validation(`ValidateFootprintGridAxisAlignment`): footprint의 root 기준 회전이 pitch·roll 0, Yaw 90° 배수가 아니면 Definition 오류다. snap 중 footprint 변이 grid 선과 평행해야 하기 때문이다. runtime 판정에는 넣지 않는다.

footprint authoring 계약:

- 설비 Actor local Z=0은 실제 설치 바닥이다.
- scaled/relative transform이 적용된 `PlacementFootprint`의 네 bottom corner도 Actor local Z=0이다.
- footprint 중심을 Actor 바닥에 두지 않으며 일반적인 axis-aligned box는 relative Z가 scaled half-height가 되도록 authoring한다.
- bottom corner가 같은 plane에 놓이지 않는 pitch/roll, non-finite transform과 0 scale은 validation 실패다.

Recovery item collision과 설치 footprint는 서로 대체하지 않는다. `RecoveryItemMesh`는 기존 동일 규격 직육면체/simple-box/physics 계약을 유지한다.

`UFacilityPlacementInstanceData::GetPlacementContentsSummary()`(2026-09-30, 기본 빈 값)는 회수 아이템의 내용 요약이다. `APlaceableFacilityItemActor`는 요약이 있으면 표시 이름 뒤에 붙이고 `GetHeldSummaryText()`로 반환한다. 사용처는 음료 냉장고다([ServiceSystem.md](ServiceSystem.md) Placement Payload Extension).

활성 Definition은 하나의 공통 Blueprint 파생 클래스를 `RecoveryItemClass`로 공유할 수 있다. runtime/Data Validation은 `APlaceableFacilityItemActor` 자체 또는 그 자식 클래스만 허용한다. 선택된 class CDO의 `ItemRoot` relative scale(`APlaceableFacilityItemActor::GetDefinitionItemScale`, 유한한 양수)이 설비 회수 아이템의 공통 물리·표현 scale 정본이며, 회수 collision query와 실제 spawn이 같은 값을 사용한다. Blueprint CDO는 component-to-world를 갱신하지 않으므로 CDO `GetActorScale3D()`로 읽지 않는다. 이 scale을 담은 transform으로 spawn할 때는 `SpawnActorDeferred`와 `FinishSpawning` 모두 `ESpawnActorScaleMethod::OverrideRootScale`을 써 root scale이 두 번 곱해지지 않게 한다. `FacilityItemHeldTransform` scale은 계속 무시하고 위치·회전만 적용한다.

## Placement Zone And Candidate Transform

`AFacilityPlacementZoneActor`는 기존 `ZoneBounds` root와 stable native `PlacementFloor` `USceneComponent`를 유지한다. `PlacementFloor`의 world XY plane이 설치 높이의 유일한 정본이며 `ZoneBounds.BoxExtent.Z`와 trace impact Z는 높이에 사용하지 않는다.

`FacilityPlacementZone`은 `ECC_GameTraceChannel1`에 등록된 placement 전용 Trace Channel이며 Project 기본 응답은 `Ignore`다. `ZoneBounds`만 `QueryOnly`에서 이 채널을 `Block`하고 `Visibility`를 포함한 나머지는 `Ignore`한다. `TracePlacementZone()`만 전용 채널을 사용하며 일반 interaction과 facility recovery는 계속 `Visibility`, floor support는 기존 `WorldStatic/WorldDynamic` object query, 후보 차단은 기존 overlap 계약을 사용한다. 따라서 Zone Bounds는 바닥의 얇은 interaction 대상보다 먼저 generic focus를 차단하지 않는다.

LCtrl은 위치 quantization만 켜고 끈다. LCtrl을 놓아도 기존 누적 Yaw와 호환 Zone 그리드 가시성은 유지된다.

후보 계산은 두 단계만 가진다.

1. trace impact를 `PlacementFloor` local space로 바꾸고 Z=0으로 투영한다. LCtrl 중에만 local X/Y를 기존 grid로 quantize하고 floor 회전 기준으로 누적 Yaw를 적용해 desired footprint-bottom frame을 만든다.
2. placed CDO root scale과 footprint relative transform을 사용해 footprint local bottom center가 위 frame 원점에 오도록 Actor translation을 한 번 역산한다.

footprint local bottom center `Bf=(0,0,-Extent.Z)`, footprint-to-root transform `R`, 최종 Actor rotation/scale `Q/S`, floor point `P`일 때 개념식은 다음과 같다.

```text
BottomOffsetWorld = Q.RotateVector(S * R.TransformPosition(Bf))
ActorLocation = P - BottomOffsetWorld
```

실제 구현은 UE `FTransform` 합성 순서와 non-uniform scale을 보존하고 bottom corner 검증으로 결과를 확인한다. Zone half-height나 footprint half-height를 이후 단계에서 다시 더하지 않는다.

preview root transform, final deferred spawn transform, footprint world transform, containment와 네 모서리 floor-support trace는 모두 이 최종 Actor transform 하나를 사용한다. `ContainsFootprint`의 기존 Zone Bounds X/Y 포함 계약과 wheel rotation은 유지한다.

### Space Zones (2026-10-02 EXP-U1, Source 반영)

- 설비 배치 구역은 공간마다 하나이며 공간 Actor `ABathhouseSpaceActor`가 `AFacilityPlacementZoneActor`를 상속해 그 자체로 구역이다([BuildingSystem.md](BuildingSystem.md)). 기존 단일 PlacementZone Level instance는 제거한다. base class 계약(`ZoneBounds`, `PlacementFloor`, grid, 후보 transform, `ContainsFootprint`)은 바뀌지 않는다. 공간 subclass만 root를 `SpaceRoot`로 바꾸고 `ZoneBounds`를 그 아래에 둔다.
- 공간별 허용 설비: 각 Definition `FacilityTags`에 종류 태그 `Facility.Type.<종류>` 하나(락커 1·4·8칸은 `Facility.Type.ClothesLocker` 공유)를 두고, 공간 Actor의 상속 `AllowedFacilityTags`에 허용 종류를 나열한다. 판정은 기존 `IsDefinitionAllowed`(HasAny)다. 태그 목록은 `Config/DefaultGameplayTags.ini`, 공간별 허용 표는 공간 Level instance가 정본이다. 공간은 빈 허용 목록을 쓰지 않는다(검증 오류).
- 거부 문구는 `AFacilityPlacementZoneActor::GetDefinitionNotAllowedReason()` 한 곳(`이 공간에는 놓을 수 없는 설비입니다`)이 정본이다. player 검증과 세 `QueryFacilityPlacement` 구현(목욕탕 설비, utility, 수건 처리기)이 같은 getter를 쓴다.
- 구역 Actor가 바닥·벽 형상도 소유하므로 `ValidateWorldPlacement`의 overlap·바닥 지지 query는 구역 Actor 전체가 아니라 `ZoneBounds` component만 무시한다(`AddIgnoredComponent`). 기존 단일 Zone에서는 결과가 같다.
- 공간 벽·천장·계단 형상은 배치 trace 채널을 Block한다. 벽 너머 다른 공간의 구역은 조준되지 않고 `설치 가능한 구역을 바라보세요.`가 된다. 계단 구멍 위는 보이지 않는 QueryOnly 막이 상자가 막는다.
- 구역 인정(2026-10-02 복귀 A1): 형상 component도 구역 Actor 소유이므로 `TracePlacementZone`은 `Cast` 성공만으로 구역을 정하지 않는다. `AFacilityPlacementZoneActor::IsZoneSurfaceHit(Hit)`(hit component = `ZoneBounds`, `ImpactNormal`이 `PlacementFloor` 위쪽과 같은 쪽)일 때만 구역이다. 벽·천장·경사로·계단 벽 hit와 `ZoneBounds` 아랫면 hit는 구역 없음(`설치 가능한 구역을 바라보세요.`)이다. trace는 한 번이고 가려진 뒤쪽 구역을 다시 찾지 않는다. 기존 단일 Zone의 결과는 같다.

## Preview Presentation

메시 미리보기(Generic Native Preview), footprint 표시(2026-10-02), 조준 없음 숨김(EXP-U3 D4), 호환 Zone grid와 반투명 그리기 순서는 [PlacementPreviewSystem.md](PlacementPreviewSystem.md)가 정본이다. 이 문서의 후보 transform·footprint·판정 계약을 그대로 쓰며 표시는 판정을 바꾸지 않는다.

## Collision And Navigation

별도 navigation geometry를 만들지 않는다. `PlacementNavModifier`, `FailsafeExtent`, primitive reference 배열과 navigation tag를 제거하고 다음 Unreal 기본 계약을 사용한다.

- 실제 몸체 `UStaticMeshComponent`의 기존 Simple Collision, collision response와 `CanEverAffectNavigation`이 정본이다.
- `RecastNavMesh.RuntimeGeneration = Dynamic`이 component 등록·collision 변경·Actor 제거의 dirty area를 갱신한다.
- `PlacementFootprint`, `PackagePhysicalRoot`, 상호작용 Box, towel presentation, 슬롯과 Action/Approach Point는 `CanEverAffectNavigation=false`다.
- non-mesh helper primitive가 Navigation relevant이면 Data Validation 실패다.

배치 시스템은 몸체 primitive를 수집하지 않는다. `UFacilityPlacementComponent`는 `None -> PendingConstruction -> Captured -> None`의 단방향 placement snapshot과 `None -> Captured -> None` recovery snapshot을 구분한다. deferred Actor는 `FinishSpawning` 전에 Actor collision을 끄고, Construction이 명시적으로 다시 활성화한 authored 결과를 snapshot에 합친 직후 다시 끈다. snapshot 준비·확정·pre-commit 검증·복원 실패는 item consume와 publication 전에 transaction 실패로 전파한다. 개별 component collision 설정은 변경하지 않는다.

- staged placement: deferred Actor를 `FinishSpawning`하기 전에 collision snapshot 후 비활성화
- placement success: silent domain 등록과 held item 소비가 끝난 뒤 collision 복원, 그 다음 publication
- placement failure: collision이 꺼진 staged Actor 제거
- recovery stage: 원본 collision 비활성화 후 silent domain unregister
- recovery rollback: domain 재등록 후 이전 collision 복원
- recovery success: collision이 꺼진 원본을 마지막에 제거
- pending/거부된 startup locker: collision과 facility/capacity/Navigation domain 비활성 유지

`SetActorEnableCollision`은 일반 primitive의 query collision과 Navigation relevancy 갱신을 UE에 전달한다. collision 없이도 nav data를 내보내는 modifier/custom exporter는 이 계약에서 허용하지 않는다.

## Placement And Recovery Transaction

기존 E pickup, LMB confirm, LCtrl snap, Mouse Wheel Yaw, G free drop과 Q Hold recovery 입력은 유지한다. LMB owner 우선순위는 `Computer > Placement > Equipment`다.

placement는 후보를 같은 frame에 재검증하고 새 placed Actor를 collision/domain 비활성 staged 상태로 만든다. payload import와 silent facility/locker 등록 후 held item을 소비하며, 그 뒤 Actor collision을 복원하고 held/facility/capacity event를 한 번 publish한다. `StagePlacedDomainRegistration()`은 collision을 복원하거나 staged flag를 commit하지 않는다. 최종 commit API만 이를 수행한다.

recovery hold 시작은 side-effect-free query 성공 뒤 `TryBeginFacilityRecoveryHold()`을 정확히 한 번 호출한다. 일반 설비의 default hook은 base `bRecoveryHoldActive`만 관리해 보류 중 예약을 막고 취소 때 가용성을 알린다([FacilitySystem.md](FacilitySystem.md) 회수 보류 알림). Bath는 0% 수위·control motion·Niagara를 snapshot한 뒤 동결한다. Q release, gaze/target 변경, suppression과 조건 변경은 `CancelFacilityRecoveryHold()`로 복원한다.

실제 recovery transaction은 staged item 준비와 collision 확인 후 원본 Actor collision/domain을 silent 비활성화한다. Bath의 domain-unregistration override는 control 닫힘을 commit-pending으로 적용하되 hold snapshot은 유지한다. item physics 활성화와 원본 파괴가 성공하면 EndPlay에서 snapshot을 폐기하고 publication한다. 파괴 전 실패는 원본 domain/collision 뒤 hold snapshot까지 복원하고 item을 제거한다. Player의 후속 cancel과 target EndPlay는 idempotent하다.

Actor collision restore는 실패 가능한 domain rollback 뒤에 수행한다. callback 재진입과 Actor 파괴 보상, held identity/Root scale/payload와 기존 회수 조건은 현재 transaction 계약을 유지한다.

placement staged 순서(2026-09-30 확정): `SpawnActorDeferred` → `PrepareForStagedPlacement` → `ImportPlacementPayload`(construction 전, native subobject만 존재) → `FinishSpawning`(Blueprint SCS component 생성) → `IPlaceableFacility::FinalizePlacementPayloadAfterConstruction(OutFailure)` → collision snapshot 확정·검증 → `QueryFacilityPlacement` → `BeginTransition` → domain 등록 → item 소비 → publication. 새 단계는 default `true`이며 기존 설비의 순서·결과를 바꾸지 않는다. 실패하면 기존 import 실패와 같이 staged Actor를 제거하고 item 소비·publication 없이 실패를 반환한다. Blueprint component에 의존하는 payload 적용은 이 단계에서만 한다([ServiceSystem.md](ServiceSystem.md) Placement Payload Extension).

회수 수행자(2026-10-01 서비스 4단위 설계): `IPlaceableFacility::SetFacilityRecoveryInstigator(AActor*)`(기본 no-op)를 `UPlayerFacilityPlacementComponent`가 Hold 시작 직전 owner pawn으로, 취소 시 nullptr로 호출한다. 회수 commit 뒤의 지급(안마의자 동전함)은 설비가 publication callback에서 한다. Placement는 지갑을 모른다([ServiceAmenitySystem.md](ServiceAmenitySystem.md) Recovery Instigator).

배치 확정 이벤트(2026-10-01 서비스 3단위 설계):

- `UFacilityPlacementEventSubsystem : UWorldSubsystem`(Public/Placement, 신규)은 C++ 전용 `FOnFacilityPlacedNative`를 소유한다.
  - 이벤트 값 `FFacilityPlacedEvent`(non-reflected struct): placed Actor weak, `PlacementFootprint` world transform, unscaled box extent.
  - `BroadcastFacilityPlaced(const FFacilityPlacedEvent&)`로 발행한다.
- `FFacilityActorConversionTransaction::PlaceItemAsFacility`가 `PublishPlacedDomainRegistration()`과 `EndTransition()` 뒤에 한 번 발행한다. placed Actor와 footprint가 아직 유효할 때만 발행한다.
  - preview, 취소, 모든 실패 경로와 회수에서는 발행하지 않는다.
  - 신규 설치와 회수 아이템 재배치 모두 이 함수를 지난다.
- Placement는 구독자를 모른다. Cleaning이 구독해 겹치는 쓰레기·물 얼룩을 지운다([CleaningLitterSystem.md](CleaningLitterSystem.md) Footprint Clear On Placement). 쓰레기·물 얼룩은 preview validity에 관여하지 않는다.
- 이 파일은 이미 500줄을 넘었다. 추가는 발행 호출 몇 줄로 한정하고 footprint 판정·구독 로직을 넣지 않는다.

## Fresh Install Payload And Discard Tag

- 상점 구매 아이템은 `Definition`만 있고 `InstanceData`가 null인 **신규 설치 payload**를 가진다. `FFacilityPlacementPayload::Validate`는 null `InstanceData`를 허용하고 `IsFreshInstall()`로 구분한다. 회수 export는 항상 domain data를 채우므로 둘이 섞이지 않는다.
- 세 import 구현(목욕탕 설비, utility, 수건 처리기)은 신규 설치 payload면 domain import를 건너뛰고 class 기본값을 유지한다. utility는 잔량 0 신규 상태, 목욕탕 설비는 번호 없음·기본 가중치다.
- 생성은 Placement 소유 factory `APlaceableFacilityItemActor::SpawnFreshItem(World, Definition, Transform, OutFailure)`만 사용한다. 회수 경로와 같은 deferred spawn → `InitializeStaged` → payload 설정 → `FinishSpawning` → 검증 순서이며, free-world 활성화는 호출자가 한다.
- native gameplay tag `Facility.Discardable`(`TAG_Facility_Discardable`)을 `FacilityTags`에 둔 Definition의 아이템만 버릴 수 있다. `LockerSlotCount > 0`인 Definition에 이 태그가 있으면 Data Validation 오류다. 아이템은 `IPhysicalCarryDiscardable`을 구현한다. 규칙과 사용처는 [ShopSystem.md](ShopSystem.md)에 있다.

## Preserved Domain Contracts

- `IPlaceableFacility`은 side-effect-free placement/recovery query, default no-op recovery-hold begin/cancel, typed payload export/import와 silent domain stage/rollback을 제공한다. `IPhysicalCarryable`은 별도 recovery item만 구현한다.
- `FFacilityPlacementPayload`는 Definition과 item-outer domain instance data만 보관한다. contents, bath water, processing progress, slot/customer와 registry reference는 전달하지 않는다.
- E는 free-world facility item pickup, LMB는 confirm, LCtrl은 snap, Mouse Wheel은 Yaw, G는 held-position free drop, Q Hold는 placed facility recovery다. LMB 우선순위는 `Computer > Placement > Equipment`다.
- Washer/Dryer는 inventory 0과 `Waiting`, Bath는 모든 slot `Available`과 normalized water amount가 정확히 `0`, locker bank는 모든 action slot `Available`과 lease-safe capacity일 때만 회수된다.
- recovery item은 footprint world bottom 기준 drop Z offset에 생성되고 원본 facility scale을 복사하지 않는다. free-world physics/CCD/Pawn Ignore와 무충격 생성 계약을 유지한다.
- Bath 회수 gate와 Q Hold freeze/restore는 [BathWaterSystem.md](BathWaterSystem.md)의 native normalized amount/control snapshot이 정본이며 Blueprint 물 표현은 상태를 소유하지 않는다.
- `InstalledLockerCapacity`는 등록된 action slot 합계이고 provisional/committed lease는 check-in과 함께 원자적으로 commit/rollback한다. 탈의·착의는 서로 다른 random slot을 사용할 수 있다.
- Expansion tier의 `KeyPoolSize`와 `MaxInstalledLockerSlots`는 독립 정본이다. locker 변경은 key 수·번호나 이미 배정된 customer key를 바꾸지 않는다.
- `ACleanTowelStackActor`와 `AUsedTowelBinActor`는 towel token owner라 Actor 변환과 Q recovery에서 제외된다.
- 순환기·보일러·쿨러는 독립 `ABathWaterUtilityFacilityActor`가 같은 placeable/carry/recovery transaction을 구현한다. utility 용량과 종류는 typed `UBathWaterUtilityPlacementInstanceData`로 round-trip하며 총용량 원장은 [BathWaterOperationsSystem.md](BathWaterOperationsSystem.md)가 소유한다.
- utility recovery query는 물리 item 후보 생성 전에 post-removal capacity를 검증한다. stage에서는 provider를 silent unregister하고 rollback은 같은 identity를 한 번 복원하며 최종 publication은 transaction 종료에 한 번만 발생한다.
- 욕탕 회수 hold는 물 양뿐 아니라 condition Tick도 freeze하지만 예약 demand는 hold 동안 유지한다. 실제 domain-unregistration stage에서만 demand를 제거하고 rollback에서 condition snapshot과 registry를 함께 복원한다.

## Locker Startup Reconciliation

`UBathhouseFacilitySubsystem`이 Expansion Authority readiness와 pre-placed locker pending set을 소유한다. `ULockerCapacitySubsystem`은 topology·한도 검증과 bank/capacity 등록만 소유한다. 락커 Actor별 `OnExpansionAuthorityChanged` 재시도는 제거한다.

pre-placed locker는 `BeginPlay`에서 idempotent하게 slot delegate를 준비하고 Actor collision을 끈 뒤 Facility Subsystem에 pending 등록한다. Authority가 먼저 시작했더라도 즉시 개별 등록하지 않는다. Facility Subsystem은 world의 post-Actor-BeginPlay lifecycle event에서 한 번 reconciliation하며, Authority가 그 이후 runtime에 등록되면 그 등록 시점에 한 번 수행한다. 지연 Tick은 사용하지 않는다.

각 pre-placed locker instance는 cooked build에도 저장되는 `FGuid RegistrationId`를 가진다. Editor가 기존 instance와 duplicate/import에 고유 ID를 생성하고 Data Validation은 invalid/duplicate ID를 거부한다. UE Editor-only `AActor::ActorGuid`는 runtime 순서에 사용하지 않는다.

reconciliation:

1. invalid weak actor와 이미 등록된 actor를 제거하고 `RegistrationId`로 정렬한다.
2. Authority가 없으면 capacity 오류를 만들지 않고 pending을 유지하며 transient readiness 진단만 한 번 기록한다.
3. structural topology를 검증하고 남은 tier slot 수에 들어가는 bank만 silent 등록한다. 큰 bank가 들어가지 않으면 거부하되 뒤의 작은 bank 검사는 계속한다.
4. bank별 facility+capacity 등록이 모두 성공한 뒤에만 Actor collision을 복원한다. 부분 실패는 해당 bank 내부 등록만 rollback하고 fail-closed한다.
5. accepted bank가 하나 이상이면 모든 내부 변경 후 ClothesLocker facility publication 한 번과 capacity publication 한 번만 발행한다.

accepted ID는 owner Actor weak reference와 함께 보관한다. 동일 actor의 pending/accepted/registered 재제출은 no-op이며, 같은 ID를 제출한 다른 actor만 duplicate로 거부한다. active reconciliation의 publication callback이 pending 또는 Authority revision을 바꾸면 guard 해제 전 즉시 tail pass를 수행해 유실 없이 다음 batch를 처리한다. 실제 tier 초과·잘못된 topology·invalid/duplicate ID는 actor별 영구 오류를 한 번만 기록하고 이번 startup reconciliation에서 재시도하지 않는다. runtime tier 상승, streaming 정책과 자동 재활성화는 이번 범위 밖이다.

## Expansion, Capacity And Key Pool

`InstalledLockerCapacity`는 성공 등록된 action slot 합계이고 `ActiveLeaseCount`와 provisional lease 계약을 유지한다. `CanInstallLockerSlots`는 Authority 미준비와 실제 한도 초과를 구조적으로 구분한다. player placement 중 Authority가 미준비면 transient 실패로 item/preview를 유지하며 `ExpansionLimit` 오류로 기록하지 않는다.

물리 key 수는 Expansion tier의 `KeyPoolSize`에만 종속된다. locker registration, 거부, 회수와 재배치는 key 번호, 수량과 customer key를 변경하지 않는다.

2026-10-02 `EXP-U2` 설계: Expansion tier index는 **홀 넓힘 횟수**(표 끝을 넘으면 마지막 줄)이고, 컴퓨터 확장 구입 transaction이 홀을 넓힐 때만 `TryAdvanceToTier`로 올린다([ExpansionPurchaseSystem.md](ExpansionPurchaseSystem.md)). 홀 넓힘 한 번에 여러 벽이 물러나도 한 번이다(EXP-U3 D3). 한도는 `CanInstallLockerSlots`가 매번 현재 tier를 읽으므로 상승 즉시 다음 미리보기 검증부터 적용된다. startup에서 거부된 locker의 자동 재활성화는 계속 범위 밖이다.

## Compatibility And Migration

사용자가 즉시 제거를 승인한 reflected/native 계약:

- `APlaceableFacilityItemActor::HeldTransform`
- `UFacilityPlacementComponent::HeldTransform`
- `UFacilityPlacementDefinition::FootprintCellsX/Y`
- `UFacilityPlacementDefinition::PreviewActorClass`
- placed facility와 towel machine의 `PlacementNavModifier` default subobject/property/configuration

rename이 아니라 property/component 삭제이므로 Core Redirect로 대체하지 않는다. Source compile 뒤 영향받는 Definition과 Blueprint를 같은 Editor migration에서 load, compile, resave하고 stale property/component reference를 검사한다. migration 전후를 섞은 Content 상태는 지원하지 않는다.

보존 계약:

- `EPlaceableFacilityMode` ordinal, legacy `Mode`, release 값, `PackagePhysicalRoot` 이름과 fail-closed placed carry API
- placed/item class 분리, payload와 Root scale 계약
- zone tag, input, 회수 gate와 atomic Actor 교체 rollback
- Clean Towel Stack/Used Towel Bin placement opt-out
- 기존 utility 설비가 없으므로 신규 actor/payload는 rename migration 없이 추가한다. 기존 facility payload와 recovery item 계약은 변경하지 않는다.

## Blueprint/API And Editor Contracts

- Project Settings: 공통 Held Transform과 valid/invalid preview material 지정
- 공통 설비 아이템 Blueprint: `/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem`, parent `APlaceableFacilityItemActor`; `ItemRoot` scale과 carry 표현 기본값만 authoring
- PlacementZone: `PlacementFloor`를 실제 바닥 plane에 배치하고 inherited `GridVisual`에 중심 pivot Plane과 공통 `MI_FacilityPlacementGrid` 지정
- PlacementZone Class Default/instance: `GridLineThicknessCm`, `GridZOffsetCm`, `MajorGridIntervalCells`만 authoring; scale/DMI/visibility graph는 만들지 않음
- placed facility Blueprint: footprint bottom을 Actor local Z=0에 맞추고 실제 body mesh collision/Nav relevance를 검증
- helper primitive와 모든 Action/Approach Point: Navigation 비관련
- Level RecastNavMesh: Runtime Generation `Dynamic`
- 기존 per-facility preview class와 NavModifier authoring 제거
- locker instance: 자동 생성된 persistent RegistrationId가 유효·고유한지 확인

Blueprint는 설비 preview mesh 복제, Zone grid DMI·크기·가시성, 후보 transform, Dynamic Navigation 전환, pending reconciliation과 publication을 변경하지 않는다.

## Dependencies

- Building -> Placement zone base class·`IPlaceableFacility`·배치 trace 채널(EXP-U1)
- Shop -> Placement Definition·fresh item factory·collision helper
- Cleaning -> Placement 배치 확정 이벤트 subsystem·collision helper(3단위)
- Placement -> Interaction carry/query/result contract
- Facility/Towel -> Placement placeable-facility contract
- Bath Water Operations utility -> Placement placeable-facility/typed-payload contract
- Facility -> Placement collision/domain activation API
- Customer -> Facility locker capacity/slot API
- Placement/Facility -> Engine NavigationSystem과 GameplayTags
- 기존 `DeveloperSettings` dependency만 사용하며 새 module/plugin은 추가하지 않는다.

## Verification

- 공통 Held transform이 모든 facility item에 같고 Scale은 보존되는지 확인한다.
- 회수 collision query와 회수 spawn이 CDO `ItemRoot` relative scale을 한 번만 쓰는지, component-to-world가 갱신되지 않은 Blueprint식 CDO에서도 확인한다. 수치는 단언하지 않고 asset·fixture의 authored 값과 비교한다.
- grid/footprint 변경 시 파생 cell과 non-multiple validation을 확인한다.
- root scale·하위 component scale이 있는 CDO fixture에서 파생 cell과 비정수 검출, Yaw 90° 배수가 아닌 footprint의 Data Validation 오류를 확인한다(2026-10-02).
- preview 표현 검증 항목은 [PlacementPreviewSystem.md](PlacementPreviewSystem.md) Verification에 있다.
- bath/washer/dryer/locker footprint bottom이 같은 floor plane에 놓이는지 확인한다.
- preview/stage에서 collision/Nav가 없고 commit/rollback 뒤 Actor collision snapshot이 복원되는지 확인한다.
- PIE 전후 대형 `NavArea_Null`이 없고 recovery/replacement 위치의 Dynamic NavMesh가 갱신되는지 확인한다.
- Facility/Queue Approach Point가 agent radius를 고려한 생성 NavMesh 위에 남는지 확인한다.
- Authority/locker BeginPlay 순서 permutation에서 accepted set, capacity와 publication 횟수가 같은지 확인한다.
- utility 회수 거부가 provider/actor/item 상태를 바꾸지 않고, stage rollback과 placement commit이 capacity를 정확히 한 번 변경하는지 확인한다.
- total slot 초과 시 stable ID 순서로 fitting bank만 등록되고 오류가 한 번만 발생하는지 확인한다.
