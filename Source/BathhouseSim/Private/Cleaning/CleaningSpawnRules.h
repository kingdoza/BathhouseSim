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
	// 조정값은 호출한 구역·director의 데이터에서 채운다. 중립 기본값은 누락 시 Find가 실패하도록 한다.
	float TraceDistance = 0;
	FName RequiredFloorTag = NAME_None;
	float MaximumSlopeDegrees = 0;
	float FloorZ = 0;
	float FloorTolerance = 0;
	float Radius = 0;
	float ClearanceHeight = 0;
	float ClearanceFloorOffset = 0;
	float Spacing = 0;
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
						   const FVector& UnscaledExtent, float HeightToleranceCm);
};
