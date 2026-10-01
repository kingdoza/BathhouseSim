#pragma once
#include "CoreMinimal.h"
class APawn;
class ATrashBagActor;

enum class ETrashBagDropStage : uint8
{
	None,
	ViewFront,
	FloorFront
};

// Camera input and every tuning value. Members have no meaningful defaults: ALitterTongsActor::BuildTieDropRequest
// is the only filler. A stage whose values are invalid (non-finite, step <= 0, minimum <= 0, distance < minimum)
// is skipped.
struct FTrashBagDropRequest
{
	FVector CameraOrigin = FVector::ZeroVector;
	FVector CameraDirection = FVector::ForwardVector;
	float ViewDistanceCm = 0.0f;
	float ViewMinDistanceCm = 0.0f;
	float ViewPullStepCm = 0.0f;
	float FloorForwardDistanceCm = 0.0f;
	float FloorMinForwardDistanceCm = 0.0f;
	float FloorPullStepCm = 0.0f;
	float FloorClearanceCm = 0.0f;
	float CameraClearanceCm = 0.0f;
};

struct FTrashBagDropPlacement
{
	static bool Find(UWorld& World, APawn* UserPawn, const TArray<AActor*>& IgnoredActors,
					 TSubclassOf<ATrashBagActor> BagClass, const FTrashBagDropRequest& Request,
					 FTransform& OutTransform, ETrashBagDropStage* OutStage = nullptr);
};
