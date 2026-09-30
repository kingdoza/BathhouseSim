#pragma once
#include "CoreMinimal.h"
#include "Interaction/HeldEquipmentUsable.h"
#include "UObject/Interface.h"
#include "HeldEquipmentSecondaryUsable.generated.h"
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))

class UHeldEquipmentSecondaryUsable : public UInterface
{
	GENERATED_BODY()
};

class BATHHOUSESIM_API IHeldEquipmentSecondaryUsable
{
	GENERATED_BODY()
public:

	virtual FHeldEquipmentUseQuery QuerySecondaryEquipmentUse(const FHeldEquipmentUseContext& Context) const = 0;
	virtual FHeldEquipmentUseResult ExecuteSecondaryEquipmentUse(const FHeldEquipmentUseContext& Context) = 0;
};
