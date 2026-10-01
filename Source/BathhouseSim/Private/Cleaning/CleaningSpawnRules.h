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
	// 호출자(구역)가 모든 필드를 채운다. 0 기본값은 조정값 복제를 피하기 위한 것이며,
	// Extent·Radius·ClearanceHeight 누락만 입력 검사로 실패한다.
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
