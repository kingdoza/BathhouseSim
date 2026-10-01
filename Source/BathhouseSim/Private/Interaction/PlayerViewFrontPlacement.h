#pragma once

#include "CoreMinimal.h"

// Pure geometry shared by the unboxing and trash-bag drop placements. It owns the definition of the
// "nearest part" of a spawn cluster in front of the player camera. No world access and no tuning values:
// every distance and clearance is an argument supplied by the caller from its tuning source.

struct FViewFrontBox
{
	FVector Center = FVector::ZeroVector;     // collision shape center
	float YawDegrees = 0.0f;                  // upright pose, yaw only
	FVector HalfExtent = FVector::ZeroVector; // local half extent (>= 0)
};

namespace PlayerViewFrontPlacement
{
// Range of (p - Origin) . UnitAxis over every point of every box.
void GetProjectionRange(TConstArrayView<FViewFrontBox> Boxes, const FVector& Origin, const FVector& UnitAxis,
	double& OutMin, double& OutMax);

// Translation to add to Boxes so the view-axis projection minimum equals DistanceCm and the camera right/up
// projection ranges are centered on the camera.
FVector ComputeViewFrontTranslation(TConstArrayView<FViewFrontBox> Boxes, const FVector& CameraOrigin,
	const FVector& UnitViewDirection, float DistanceCm);

// World-space Boxes. Returns 0 when the top is at or below CameraOrigin.Z - CameraClearanceCm, or when the
// horizontal forward projection minimum is already >= CameraClearanceCm. Otherwise the push along
// UnitHorizontalForward that makes the projection minimum equal CameraClearanceCm.
float GetCameraClearancePushCm(TConstArrayView<FViewFrontBox> Boxes, const FVector& CameraOrigin,
	const FVector& UnitHorizontalForward, float CameraClearanceCm);

// Start' = max(Start, Min); Start', Start' - Step, ... while > Min, then Min once. Empty for non-finite
// input or Step <= 0.
void BuildPullDistances(float StartCm, float MinCm, float StepCm, TArray<float>& OutDistances);
}
