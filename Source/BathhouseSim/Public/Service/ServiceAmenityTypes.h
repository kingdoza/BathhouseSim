#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ServiceAmenityTypes.generated.h"
class AScrubTableActor;
class ABathhouseCashPaymentActor;
enum class EServiceUseEndReason : uint8
{
	Completed,
	Abandoned
};
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))

class UServiceFacilityUser : public UInterface
{
	GENERATED_BODY()
};

class BATHHOUSESIM_API IServiceFacilityUser
{
	GENERATED_BODY()
public:

	virtual void HandleServiceUseEnded(AActor& Facility, EServiceUseEndReason Reason) = 0;
	virtual void HandleScrubCashOffered(AScrubTableActor& Table, ABathhouseCashPaymentActor& Cash,
										const FTransform& StandTransform) = 0;
};
