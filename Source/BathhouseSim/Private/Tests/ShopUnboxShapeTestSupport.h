#pragma once

#include "CoreMinimal.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Shop/ShopSettings.h"
#include "Shop/ShopUnboxItemShape.h"
#include "Shop/ShopUnboxingPlacement.h"
#include "Shop/ShopUnboxingTuning.h"

namespace ShopUnboxTest
{
/** Converts facility definitions into unbox shapes; a definition that cannot provide one yields an empty query. */
inline TArray<FShopUnboxItemShape> MakeShapes(const TArray<UFacilityPlacementDefinition*>& Definitions)
{
	TArray<FShopUnboxItemShape> Shapes;
	Shapes.Reserve(Definitions.Num());
	for (UFacilityPlacementDefinition* Definition : Definitions)
	{
		FShopUnboxItemShape& Shape = Shapes.AddDefaulted_GetRef();
		FText Failure;
		if (!IsValid(Definition) || !FShopUnboxItemShape::FromFacilityDefinition(*Definition, Shape, Failure))
		{
			Shape = FShopUnboxItemShape();
		}
	}
	return Shapes;
}

/** Tuning read from the settings source, the same single read point the runtime uses. */
inline FShopUnboxingTuning MakeTuning()
{
	return FShopUnboxingTuning::FromSettings(*GetDefault<UShopSettings>());
}

/** Camera at the character's eye height looking along Direction (default: level, +X). */
inline FShopUnboxingPlacementRequest MakeRequest(
	const ACharacter& Player,
	const FVector& Direction = FVector::ForwardVector)
{
	FShopUnboxingPlacementRequest Request;
	Request.CameraOrigin = Player.GetActorLocation() + FVector(0.0f, 0.0f, Player.BaseEyeHeight);
	Request.CameraDirection = Direction;
	Request.FootLocation = Player.GetActorLocation()
		- FVector::UpVector * Player.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	return Request;
}

/**
 * Settings tuning with the view-front stage skipped (invalid pull step), the floor-front forward distance and the
 * overlap depth overridden. Used by tests that exercise the floor-front, overhead and final-stack stages alone.
 */
inline FShopUnboxingTuning MakeFloorStageTuning(const float ForwardDistanceCm, const float OverlapDepthCm)
{
	FShopUnboxingTuning Tuning = MakeTuning();
	Tuning.ViewPullStepCm = 0.0f;
	Tuning.ForwardDistanceCm = ForwardDistanceCm;
	Tuning.OverlapDepthCm = OverlapDepthCm;
	return Tuning;
}
}
