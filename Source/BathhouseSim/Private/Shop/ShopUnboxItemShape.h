#pragma once

#include "CoreMinimal.h"

class AItemBoxActor;
class UFacilityPlacementDefinition;
class UPrimitiveComponent;

/** Collision shape source of one unboxed item, independent of whether it is a facility item or an item box. */
struct FShopUnboxItemShape
{
	using FBuildCollisionQuery = TFunction<bool(
		const FTransform& ProbeWorldTransform,
		FVector& OutLocation,
		FQuat& OutRotation,
		FCollisionShape& OutShape,
		const UPrimitiveComponent*& OutCollisionTemplate,
		FText& OutFailureReason)>;

	FVector ItemScale = FVector::OneVector;
	FBuildCollisionQuery BuildCollisionQuery;
	FString DebugName;

	static bool FromFacilityDefinition(
		UFacilityPlacementDefinition& Definition,
		FShopUnboxItemShape& OutShape,
		FText& OutFailureReason);
	static bool FromItemBoxClass(
		TSubclassOf<AItemBoxActor> BoxClass,
		FShopUnboxItemShape& OutShape,
		FText& OutFailureReason);
};
