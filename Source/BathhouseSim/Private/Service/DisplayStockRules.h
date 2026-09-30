#pragma once
#include "CoreMinimal.h"
#include "Service/ServiceItemTypes.h"

class FDisplayStockRules
{
public:

	static bool IsConsistent(const FServiceItemStack& Stock);
	static int32 GetTakeableCount(const FServiceItemStack& Stock);
	static bool ValidateImport(const UServiceItemDefinition* Kind, int32 Count, int32 Remaining, int32 Capacity);
	static bool ConsumeOneUse(FServiceItemStack& Stock, const UServiceItemDefinition* FixedKind, bool& OutDepleted);
};
