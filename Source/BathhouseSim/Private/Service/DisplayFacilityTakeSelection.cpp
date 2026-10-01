#include "Service/DisplayFacilityTakeSelection.h"

double FDisplayFacilityTakeSelection::GetAimAngleRadians(const FVector& TraceStart, const FVector& TraceEnd,
														 const FVector& Point)
{
	const FVector Direction = (TraceEnd - TraceStart).GetSafeNormal();
	const FVector ToPoint = Point - TraceStart;
	if (Direction.IsZero() || ToPoint.IsNearlyZero())
	{
		return 0.0;
	}
	return FMath::Atan2(FVector::CrossProduct(Direction, ToPoint).Size(), FVector::DotProduct(Direction, ToPoint));
}

int32 FDisplayFacilityTakeSelection::SelectSpaceIndex(TConstArrayView<FDisplayFacilityTakeCandidate> Spaces,
													  const FVector& TraceStart, const FVector& TraceEnd)
{
	double BestDistance = TNumericLimits<double>::Max();
	int32 Best = INDEX_NONE;
	for (const FDisplayFacilityTakeCandidate& Space : Spaces)
	{
		if (Space.TakeableCount <= 0 || Space.ItemLocations.IsEmpty())
		{
			continue;
		}
		double Distance = TNumericLimits<double>::Max();
		for (const FVector& Location : Space.ItemLocations)
		{
			Distance = FMath::Min(Distance, GetAimAngleRadians(TraceStart, TraceEnd, Location));
		}
		if (Best == INDEX_NONE || Distance < BestDistance || (Distance == BestDistance && Space.SpaceIndex < Best))
		{
			BestDistance = Distance;
			Best = Space.SpaceIndex;
		}
	}
	if (Best != INDEX_NONE)
	{
		return Best;
	}
	// No takeable group: report a stocked group (so the reason comes from its EvaluateTake), else the first group.
	int32 Stocked = INDEX_NONE;
	int32 Lowest = INDEX_NONE;
	for (const FDisplayFacilityTakeCandidate& Space : Spaces)
	{
		if (Lowest == INDEX_NONE || Space.SpaceIndex < Lowest)
		{
			Lowest = Space.SpaceIndex;
		}
		if (Space.Count > 0 && (Stocked == INDEX_NONE || Space.SpaceIndex < Stocked))
		{
			Stocked = Space.SpaceIndex;
		}
	}
	return Stocked != INDEX_NONE ? Stocked : Lowest;
}
