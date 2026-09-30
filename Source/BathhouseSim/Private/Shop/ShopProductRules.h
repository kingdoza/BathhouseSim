#pragma once

#include "CoreMinimal.h"
#include "Shop/ShopTypes.h"

/** Shared validity rules for shop products and order lines: exactly one of a facility or an item-box definition. */
class FShopProductRules
{
public:
	static bool ValidateDefinitions(
		const UFacilityPlacementDefinition* Facility,
		const UServiceItemDefinition* ItemBox,
		FText& OutFailureReason);

	static bool ValidateProduct(const FShopProductEntry& Product, FText& OutFailureReason);
	static bool ValidateOrderLine(const FShopOrderLine& Line, FText& OutFailureReason);
};
