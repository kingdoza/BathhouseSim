#pragma once

#include "CoreMinimal.h"

#include "Shop/ShopUnboxItemShape.h"

class AActor;
class AShopDeliveryBoxActor;
class UCapsuleComponent;
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
		const TArray<FShopUnboxItemShape>& Items,
		float ForwardDistanceCm,
		FRandomStream& RandomStream,
		float OverlapDepthCm,
		TArray<FTransform>& OutTransforms,
		FText& OutFailureReason);

};
