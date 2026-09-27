#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ComputerScreenContext.generated.h"

class ABathhouseComputerActor;
class AFacilityPlacementZoneActor;
class APlayerState;
class UBathWaterOperationsSubsystem;

USTRUCT()
struct BATHHOUSESIM_API FComputerScreenContext
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<ABathhouseComputerActor> Computer;

	UPROPERTY()
	TWeakObjectPtr<UBathWaterOperationsSubsystem> Operations;

	UPROPERTY()
	TWeakObjectPtr<AFacilityPlacementZoneActor> ManagedZone;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UComputerScreenContextReceiver : public UInterface
{
	GENERATED_BODY()
};

class BATHHOUSESIM_API IComputerScreenContextReceiver
{
	GENERATED_BODY()

public:
	virtual void InitializeComputerScreen(const FComputerScreenContext& Context) = 0;
	virtual void NotifyComputerUserChanged(APlayerState* PlayerState) = 0;
};
