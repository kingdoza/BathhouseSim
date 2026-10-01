#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WrenchRepairable.generated.h"
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))

class UWrenchRepairable : public UInterface
{
	GENERATED_BODY()
};

class BATHHOUSESIM_API IWrenchRepairable
{
	GENERATED_BODY()
public:

	virtual bool IsWrenchRepairRequired() const = 0;
	virtual float GetWrenchRepairSeconds() const = 0;
	virtual bool CommitWrenchRepair(FText& OutFailure) = 0;
};
