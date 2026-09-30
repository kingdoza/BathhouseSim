#include "Interaction/Presentation/OpeningPivotRotation.h"

#include "Components/SceneComponent.h"

namespace
{
	// SetRelativeRotation re-derives RelativeLocation from a world-space round trip, so a rotated parent
	// perturbs it by rounding error. Editor preview default propagation compares exact values, so drift
	// makes the Blueprint viewport stop following authored pivot location edits.
	void SetPivotRelativeRotationPreservingLocation(USceneComponent* Pivot, const FQuat& NewRelativeRotation)
	{
		const FVector PreservedRelativeLocation = Pivot->GetRelativeLocation();
		Pivot->SetRelativeRotation(NewRelativeRotation);
		if (Pivot->GetRelativeLocation() != PreservedRelativeLocation)
		{
			Pivot->SetRelativeLocation_Direct(PreservedRelativeLocation);
			Pivot->UpdateComponentToWorld();
		}
	}

	// The authored pose lives on the component template (Blueprint or native default subobject). The live
	// instance may still hold a displayed pose after Blueprint reinstancing, level save/load or PIE duplication,
	// so it is only used when no template exists (e.g. components created directly with NewObject).
	FQuat ResolveAuthoredRelativeRotation(const USceneComponent* Pivot)
	{
		if (!Pivot->IsTemplate())
		{
			const USceneComponent* Template = Cast<USceneComponent>(Pivot->GetArchetype());
			if (Template && Template != Pivot && !Template->HasAnyFlags(RF_ClassDefaultObject))
			{
				return Template->GetRelativeRotation().Quaternion();
			}
		}
		return Pivot->GetRelativeRotation().Quaternion();
	}
} // namespace

void FOpeningPivotRotation::CaptureBaseline(USceneComponent* Pivot)
{
	if (!IsValid(Pivot))
	{
		return;
	}

	const FQuat CurrentRelativeRotation = Pivot->GetRelativeRotation().Quaternion();
	if (bHasBaseline && bHasAppliedRotation && CurrentRelativeRotation.Equals(LastAppliedRelativeRotation, 1.0e-4f))
	{
		return;
	}

	BaselineRelativeRotation = ResolveAuthoredRelativeRotation(Pivot);
	bHasBaseline = true;
	bHasAppliedRotation = false;
}

void FOpeningPivotRotation::Apply(USceneComponent* Pivot, const FVector& LocalAxis, const float AngleDegrees)
{
	if (!IsValid(Pivot) || !FMath::IsFinite(LocalAxis.X) || !FMath::IsFinite(LocalAxis.Y) ||
		!FMath::IsFinite(LocalAxis.Z) || LocalAxis.IsNearlyZero() || !FMath::IsFinite(AngleDegrees))
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

	const FQuat LocalAxisRotation(LocalAxis.GetSafeNormal(), FMath::DegreesToRadians(AngleDegrees));
	LastAppliedRelativeRotation = BaselineRelativeRotation * LocalAxisRotation;
	SetPivotRelativeRotationPreservingLocation(Pivot, LastAppliedRelativeRotation);
	bHasAppliedRotation = true;
}

void FOpeningPivotRotation::Reset(USceneComponent* Pivot)
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
	SetPivotRelativeRotationPreservingLocation(Pivot, BaselineRelativeRotation);
	bHasAppliedRotation = true;
}

void FOpeningPivotRotation::Forget()
{
	BaselineRelativeRotation = FQuat::Identity;
	LastAppliedRelativeRotation = FQuat::Identity;
	bHasBaseline = false;
	bHasAppliedRotation = false;
}
