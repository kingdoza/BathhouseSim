#pragma once

#include "CoreMinimal.h"

#include "Shop/ShopUnboxingTuning.h"

struct FShopUnboxClusterItem
{
	FVector Center = FVector::ZeroVector;
	float YawDegrees = 0.0f;
};

class FShopUnboxingCluster final
{
public:
	static bool BuildLayout(
		const TArray<FVector>& HalfExtents,
		float DepthCm,
		const FShopUnboxClusterTuning& Tuning,
		FRandomStream& RandomStream,
		TArray<FShopUnboxClusterItem>& OutItems);

	static float ComputePenetrationDepth(
		const FVector& CenterA,
		float YawA,
		const FVector& HalfExtentA,
		const FVector& CenterB,
		float YawB,
		const FVector& HalfExtentB);
};
