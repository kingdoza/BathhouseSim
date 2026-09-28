#pragma once

#include "CoreMinimal.h"

class AActor;
class AShopDeliveryBoxActor;
class UCapsuleComponent;
class UFacilityPlacementDefinition;
class UWorld;

class FShopUnboxingPlacement
{
public:
	static bool FindSpawnTransforms(
		UWorld& World,
		AActor& Player,
		const UCapsuleComponent& PlayerCapsule,
		AShopDeliveryBoxActor& Box,
		const FVector& FootLocation,
		float ViewYaw,
		const TArray<UFacilityPlacementDefinition*>& Definitions,
		float ForwardDistanceCm,
		FRandomStream& RandomStream,
		float OverlapDepthCm,
		TArray<FTransform>& OutTransforms,
		FText& OutFailureReason);
};
