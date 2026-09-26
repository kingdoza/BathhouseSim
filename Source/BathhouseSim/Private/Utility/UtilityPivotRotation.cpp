#include "Utility/UtilityPivotRotation.h"

#include "Components/SceneComponent.h"

void FUtilityPivotRotation::CaptureBaseline(USceneComponent* Pivot)
{
	if (!IsValid(Pivot))
	{
		return;
	}

	const FQuat CurrentRelativeRotation = Pivot->GetRelativeRotation().Quaternion();
	if (bHasBaseline && bHasAppliedRotation
		&& CurrentRelativeRotation.Equals(LastAppliedRelativeRotation, 1.0e-4f))
	{
		return;
	}

	BaselineRelativeRotation = CurrentRelativeRotation;
	bHasBaseline = true;
	bHasAppliedRotation = false;
}

void FUtilityPivotRotation::Apply(
	USceneComponent* Pivot,
	const FVector& LocalAxis,
	const float AngleDegrees)
{
	if (!IsValid(Pivot)
		|| !FMath::IsFinite(LocalAxis.X) || !FMath::IsFinite(LocalAxis.Y) || !FMath::IsFinite(LocalAxis.Z)
		|| LocalAxis.IsNearlyZero() || !FMath::IsFinite(AngleDegrees))
	{
		return;
	}
	if (!bHasBaseline)
	{
		CaptureBaseline(Pivot);
	}
	if (!bHasBaseline)
	{
		return;
	}

	const FQuat LocalAxisRotation(
		LocalAxis.GetSafeNormal(),
		FMath::DegreesToRadians(AngleDegrees));
	LastAppliedRelativeRotation = BaselineRelativeRotation * LocalAxisRotation;
	Pivot->SetRelativeRotation(LastAppliedRelativeRotation);
	bHasAppliedRotation = true;
}

void FUtilityPivotRotation::Reset(USceneComponent* Pivot)
{
	if (!IsValid(Pivot))
	{
		return;
	}
	if (!bHasBaseline)
	{
		CaptureBaseline(Pivot);
	}
	if (!bHasBaseline)
	{
		return;
	}

	LastAppliedRelativeRotation = BaselineRelativeRotation;
	Pivot->SetRelativeRotation(BaselineRelativeRotation);
	bHasAppliedRotation = true;
}

void FUtilityPivotRotation::Forget()
{
	BaselineRelativeRotation = FQuat::Identity;
	LastAppliedRelativeRotation = FQuat::Identity;
	bHasBaseline = false;
	bHasAppliedRotation = false;
}
