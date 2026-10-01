#pragma once

#include "CoreMinimal.h"

#include "Shop/ShopUnboxItemShape.h"

class AActor;
class AShopDeliveryBoxActor;
class UCapsuleComponent;
class UWorld;
struct FShopUnboxingTuning;

enum class EShopUnboxPlacementStage : uint8
{
	None,
	ViewFront,
	FloorFront,
	Overhead,
	FinalStack
};

struct FShopUnboxingPlacementRequest
{
	FVector CameraOrigin = FVector::ZeroVector;
	FVector CameraDirection = FVector::ForwardVector; // includes pitch, normalized internally
	FVector FootLocation = FVector::ZeroVector;
};

class FShopUnboxingPlacement
{
public:
	static bool FindSpawnTransforms(
		UWorld& World,
		AActor& Player,
		const UCapsuleComponent& PlayerCapsule,
		AShopDeliveryBoxActor& Box,
		const TArray<FShopUnboxItemShape>& Items,
		const FShopUnboxingPlacementRequest& Request,
		const FShopUnboxingTuning& Tuning,
		FRandomStream& RandomStream,
		TArray<FTransform>& OutTransforms,
		FText& OutFailureReason,
		EShopUnboxPlacementStage* OutStage = nullptr);
};
