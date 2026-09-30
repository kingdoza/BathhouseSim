#include "Service/DisplayStockRules.h"
#include "Service/ServiceItemDefinition.h"

bool FDisplayStockRules::IsConsistent(const FServiceItemStack& Stock)
{
	return ValidateImport(Stock.Kind, Stock.Count, Stock.InUseRemaining, MAX_int32);
}

bool FDisplayStockRules::ValidateImport(const UServiceItemDefinition* Kind, int32 Count, int32 Remaining,
										int32 Capacity)
{
	return Count >= 0 && Count <= Capacity && (Count == 0) == (Kind == nullptr) && Remaining >= 0 &&
		   (Remaining == 0 || (Kind && Count > 0 && Kind->ConsumableUses > 0 && Remaining <= Kind->ConsumableUses));
}

int32 FDisplayStockRules::GetTakeableCount(const FServiceItemStack& Stock)
{
	return FMath::Max(0, Stock.Count - (Stock.InUseRemaining > 0 ? 1 : 0));
}

bool FDisplayStockRules::ConsumeOneUse(FServiceItemStack& Stock, const UServiceItemDefinition* FixedKind,
									   bool& OutDepleted)
{
	OutDepleted = false;
	if (!FixedKind || FixedKind->ConsumableUses <= 0 || Stock.Kind != FixedKind || Stock.Count <= 0 ||
		!IsConsistent(Stock))
	{
		return false;
	}
	if (Stock.InUseRemaining == 0)
	{
		Stock.InUseRemaining = FixedKind->ConsumableUses;
	}
	if (--Stock.InUseRemaining == 0)
	{
		--Stock.Count;
		if (Stock.Count == 0)
		{
			Stock.Kind = nullptr;
		}
		OutDepleted = true;
	}
	++Stock.Revision;
	return true;
}
