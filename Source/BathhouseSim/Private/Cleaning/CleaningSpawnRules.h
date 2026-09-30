#pragma once
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

struct FCleaningSpawnClock
{
	static float SampleNext(FRandomStream& Stream);
	static bool Advance(float& Remaining, int32 CustomerCount, float DeltaSeconds, float MeanIntervalSeconds);
};

int32 CountCustomersInBox(const TArray<FVector>& Locations, const FTransform& BoxTransform, const FVector& Extent);

struct FCleaningFloorSpawnSettings
{
	FTransform BoxTransform = FTransform::Identity;
	FVector Extent = FVector::ZeroVector;
	ECollisionChannel TraceChannel = ECC_Visibility;
	float TraceDistance = 300;
	FName RequiredFloorTag = NAME_None;
	float MaximumSlopeDegrees = 25;
	float FloorZ = 0;
	float FloorTolerance = 5;
	float Radius = 15;
	float ClearanceHeight = 30;
	float Spacing = 40;
};

struct FCleaningFloorSpawnQuery
{
	static bool Find(UWorld& World, const AActor* Zone, const FCleaningFloorSpawnSettings& Settings,
					 FRandomStream& Stream, TFunctionRef<bool(const FVector&, float)> IsSpacingClear,
					 FTransform& OutTransform);
};

struct FCleaningFootprintOverlap
{
	static bool Intersects(const FVector& Center, float Radius, const FTransform& FootprintTransform,
						   const FVector& UnscaledExtent);
};
