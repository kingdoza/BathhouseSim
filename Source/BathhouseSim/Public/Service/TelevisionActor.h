#pragma once
#include "Facility/BathhouseFacilityActor.h"
#include "TelevisionActor.generated.h"
UCLASS(Blueprintable)

class BATHHOUSESIM_API ATelevisionActor : public ABathhouseFacilityActor
{
	GENERATED_BODY()
public:

	ATelevisionActor();
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	UFUNCTION(BlueprintPure, Category = "Service")

	bool IsPoweredOn() const
	{
		return bPoweredOn;
	}
	UFUNCTION(BlueprintImplementableEvent, Category = "Service")
	void OnPowerChanged(bool PoweredOn);

private:

	UPROPERTY(Transient)
	bool bPoweredOn = false;
};
