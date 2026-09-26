#pragma once

#include "CoreMinimal.h"

class USceneComponent;

/** Preserves an authored pivot baseline while applying a local axis-angle presentation. */
class FUtilityPivotRotation
{
public:
	void CaptureBaseline(USceneComponent* Pivot);
	void Apply(USceneComponent* Pivot, const FVector& LocalAxis, float AngleDegrees);
	void Reset(USceneComponent* Pivot);
	void Forget();
	bool HasBaseline() const { return bHasBaseline; }

private:
	FQuat BaselineRelativeRotation = FQuat::Identity;
	FQuat LastAppliedRelativeRotation = FQuat::Identity;
	bool bHasBaseline = false;
	bool bHasAppliedRotation = false;
};
