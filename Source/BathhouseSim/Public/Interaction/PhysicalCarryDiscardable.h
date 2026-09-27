#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PhysicalCarryDiscardable.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UPhysicalCarryDiscardable : public UInterface
{
	GENERATED_BODY()
};

class BATHHOUSESIM_API IPhysicalCarryDiscardable
{
	GENERATED_BODY()

public:
	virtual bool CanDiscardCarriedObject(FText& OutFailureReason) const = 0;
	virtual void HandleDiscardCommitted() = 0;
};
