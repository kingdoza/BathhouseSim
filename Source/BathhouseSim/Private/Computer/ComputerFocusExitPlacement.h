#pragma once

#include "CoreMinimal.h"

class AActor;
class UCapsuleComponent;
class UWorld;

struct FComputerFocusExitPlacementResult
{
	enum class EPath : uint8
	{
		Fixed,
		Searched,
		Forced
	};

	FVector CapsuleCenter = FVector::ZeroVector;
	EPath Path = EPath::Forced;
	float SearchDistanceCm = 0.0f;
};

class FComputerFocusExitPlacement
{
public:
	static bool Resolve(
		UWorld* World,
		UCapsuleComponent* PlayerCapsule,
		AActor* PlayerActor,
		const FVector& FootLocation,
		float FixedYawDegrees,
		float SearchRadiusCm,
		FComputerFocusExitPlacementResult& OutResult);
};
