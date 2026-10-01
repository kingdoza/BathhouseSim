#pragma once

#include "CoreMinimal.h"

class UShopSettings;

// Cluster shape values read by FShopUnboxingCluster::BuildLayout.
struct FShopUnboxClusterTuning
{
	int32 PlacementAttempts = 0;
	float MinElevationDegrees = 0.0f;
	float MaxElevationDegrees = 0.0f;
	float DepthToleranceCm = 0.0f;
	float PairDepthExtentRatio = 0.0f;
};

// Every unboxing placement value. UShopSettings is the only source; FromSettings is the single read point.
struct FShopUnboxingTuning
{
	float OverlapDepthCm = 0.0f;

	float ViewDistanceCm = 0.0f;
	float ViewMinDistanceCm = 0.0f;
	float ViewPullStepCm = 0.0f;
	int32 ViewLayoutAttempts = 0;

	float ForwardDistanceCm = 0.0f;
	float MinForwardDistanceCm = 0.0f;
	float ForwardPullStepCm = 0.0f;
	int32 ForwardLayoutAttempts = 0;
	float ForwardFloorClearanceCm = 0.0f;
	float CameraClearanceCm = 0.0f;

	float OverheadClearanceCm = 0.0f;
	float OverheadStepCm = 0.0f;
	int32 OverheadStepCount = 0;
	int32 OverheadLayoutAttempts = 0;

	FShopUnboxClusterTuning Cluster;

	static FShopUnboxingTuning FromSettings(const UShopSettings& Settings);
};
