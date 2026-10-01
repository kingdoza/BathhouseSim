#pragma once
#include "CoreMinimal.h"

struct FDisplayFacilityTakeCandidate
{
	int32 SpaceIndex = INDEX_NONE;
	/** Visible plus in-use count. */
	int32 Count = 0;
	/** FDisplayStockRules::GetTakeableCount. */
	int32 TakeableCount = 0;
	/** World positions of the visible display items (in-use included, empty slots excluded). */
	TArray<FVector> ItemLocations;
};

/** Pure rule for choosing the display group an empty item box takes from. No world or UObject dependency. */
class FDisplayFacilityTakeSelection
{
public:

	/** Angle (rad, 0..PI) between the screen-center direction and Point. 0 when the direction or Point-Origin is zero. */
	static double GetAimAngleRadians(const FVector& TraceStart, const FVector& TraceEnd, const FVector& Point);
	/** Selected SpaceIndex, or INDEX_NONE when Spaces is empty. Does not assume input order. */
	static int32 SelectSpaceIndex(TConstArrayView<FDisplayFacilityTakeCandidate> Spaces, const FVector& TraceStart,
								  const FVector& TraceEnd);
};
