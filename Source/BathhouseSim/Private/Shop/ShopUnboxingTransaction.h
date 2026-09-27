#pragma once

#include "CoreMinimal.h"

class AShopDeliveryBoxActor;
struct FHeldEquipmentUseContext;

class FShopUnboxingTransaction
{
public:
	static bool Open(
		AShopDeliveryBoxActor& Box,
		const FHeldEquipmentUseContext& Context,
		FText& OutFailureReason);
};
