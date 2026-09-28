#include "Placement/PlaceableFacilityItemActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "PhysicsEngine/BodySetup.h"
#include "Placement/FacilityPlacementDefinition.h"

#define LOCTEXT_NAMESPACE "PlaceableFacilityItemCollision"

UStaticMesh* APlaceableFacilityItemActor::ResolveRecoveryMesh(
	const UFacilityPlacementDefinition& Definition)
{
	return Definition.RecoveryItemMesh
		? Definition.RecoveryItemMesh.Get()
		: LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
}

bool APlaceableFacilityItemActor::ValidateRecoveryMesh(
	const UStaticMesh& Mesh,
	FText& OutFailureReason)
{
	const UBodySetup* BodySetup = Mesh.GetBodySetup();
	if (!BodySetup || BodySetup->CollisionTraceFlag == CTF_UseComplexAsSimple)
	{
		OutFailureReason = LOCTEXT(
			"InvalidRecoveryMeshCollision",
			"설비 아이템 메시는 물리 가능한 simple collision을 가져야 합니다.");
		return false;
	}
	const FKAggregateGeom& Geometry = BodySetup->AggGeom;
	if (Geometry.BoxElems.Num() != 1 || !Geometry.SphereElems.IsEmpty()
		|| !Geometry.SphylElems.IsEmpty() || !Geometry.ConvexElems.IsEmpty()
		|| !Geometry.TaperedCapsuleElems.IsEmpty())
	{
		OutFailureReason = LOCTEXT(
			"RecoveryMeshNotSingleBox",
			"설비 아이템 메시는 하나의 simple box collision만 가져야 합니다.");
		return false;
	}
	const FKBoxElem& Box = Geometry.BoxElems[0];
	const FBoxSphereBounds Bounds = Mesh.GetBounds();
	const FVector BoxSize(Box.X, Box.Y, Box.Z);
	const FVector BoundsSize = Bounds.BoxExtent * 2.0f;
	if (BoxSize.GetMin() <= KINDA_SMALL_NUMBER
		|| !BoxSize.Equals(BoundsSize, 1.0f)
		|| !Bounds.Origin.IsNearlyZero(1.0f)
		|| !Box.Center.IsNearlyZero(1.0f)
		|| !Box.Rotation.IsNearlyZero(0.1f))
	{
		OutFailureReason = LOCTEXT(
			"RecoveryMeshBoxBoundsMismatch",
			"설비 아이템의 mesh bounds와 simple box collision은 offset/회전 없이 일치해야 합니다.");
		return false;
	}
	return true;
}

bool APlaceableFacilityItemActor::GetDefinitionItemScale(
	const UFacilityPlacementDefinition& Definition,
	FVector& OutScale,
	FText& OutFailureReason)
{
	OutScale = FVector::OneVector;
	OutFailureReason = FText::GetEmpty();
	const APlaceableFacilityItemActor* ItemCDO = Definition.RecoveryItemClass
		? Definition.RecoveryItemClass->GetDefaultObject<APlaceableFacilityItemActor>()
		: nullptr;
	const USceneComponent* Root = ItemCDO ? ItemCDO->GetRootComponent() : nullptr;
	if (!Root)
	{
		OutFailureReason = LOCTEXT(
			"InvalidRecoveryItemRoot",
			"설비 회수 아이템 클래스의 기본 루트를 확인할 수 없습니다.");
		return false;
	}

	const FVector RootScale = Root->GetRelativeScale3D();
	if (!FMath::IsFinite(RootScale.X) || !FMath::IsFinite(RootScale.Y) || !FMath::IsFinite(RootScale.Z)
		|| RootScale.X <= 0.0f || RootScale.Y <= 0.0f || RootScale.Z <= 0.0f)
	{
		OutFailureReason = LOCTEXT(
			"InvalidRecoveryItemRootScale",
			"설비 회수 아이템 클래스의 기본 루트 스케일은 유한한 양수여야 합니다.");
		return false;
	}

	OutScale = RootScale;
	return true;
}

bool APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
	const UFacilityPlacementDefinition& Definition,
	const FTransform& ItemWorldTransform,
	FVector& OutLocation,
	FQuat& OutRotation,
	FCollisionShape& OutShape,
	const UPrimitiveComponent*& OutCollisionTemplate,
	FText& OutFailureReason)
{
	const APlaceableFacilityItemActor* ItemCDO = Definition.RecoveryItemClass
		? Definition.RecoveryItemClass->GetDefaultObject<APlaceableFacilityItemActor>()
		: nullptr;
	UStaticMesh* Mesh = ResolveRecoveryMesh(Definition);
	if (!ItemCDO || !ItemCDO->ItemRoot || !Mesh
		|| !ValidateRecoveryMesh(*Mesh, OutFailureReason))
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT(
				"InvalidRecoveryItemClass",
				"설비 회수 아이템 클래스를 확인할 수 없습니다.");
		}
		return false;
	}
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const FVector Scale = ItemWorldTransform.GetScale3D().GetAbs();
	const FVector Extent = Bounds.BoxExtent * Scale;
	if (Extent.ContainsNaN() || Extent.GetMin() <= KINDA_SMALL_NUMBER)
	{
		OutFailureReason = LOCTEXT(
			"InvalidRecoveryItemScale",
			"설비 회수 아이템 크기가 올바르지 않습니다.");
		return false;
	}
	OutLocation = ItemWorldTransform.TransformPosition(Bounds.Origin);
	OutRotation = ItemWorldTransform.GetRotation();
	OutShape = FCollisionShape::MakeBox(Extent);
	OutCollisionTemplate = ItemCDO->ItemRoot;
	return true;
}

#undef LOCTEXT_NAMESPACE
