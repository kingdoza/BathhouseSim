#pragma once

#include "CoreMinimal.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Service/ServiceItemTypes.h"
#include "DrinkFridgeActor.generated.h"

class UBathhouseFacilitySlotComponent;
class UDisplaySpaceComponent;
class UServiceItemDefinition;
class UServiceDisplayManagerComponent;

/** Facility with display spaces and one customer slot. Recovery keeps stock through the display manager extension. */
UCLASS(Blueprintable)

class BATHHOUSESIM_API ADrinkFridgeActor : public ABathhouseFacilityActor
{
	GENERATED_BODY()

public:

	ADrinkFridgeActor();

	virtual bool IsAvailableForReservation() const override;

	/**
	 * Shared layout rule for instances and Blueprint templates: at least one space, SpaceIndex unique and contiguous
	 * from 0, valid space authoring, and exactly one customer slot.
	 */
	static bool ValidateSpaceLayout(TConstArrayView<const UDisplaySpaceComponent*> Spaces, int32 CustomerSlotCount,
									FText& OutFailureReason);

	/** Spaces sorted by SpaceIndex. Fails when indices are not unique/contiguous from 0 or authoring is invalid. */
	bool CollectSpaces(TArray<UDisplaySpaceComponent*>& OutSpaces, FText& OutFailureReason) const;
	int32 GetTotalStock() const;

	UServiceDisplayManagerComponent* GetDisplayManager() const
	{
		return DisplayManager;
	}

	/** The slot's current user takes one drink from the first non-empty space and its sale is recorded. */
	bool TryTakeDrinkForCustomer(AActor& Customer, UServiceItemDefinition*& OutKind, FText& OutFailureReason);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Service Display")
	TObjectPtr<UServiceDisplayManagerComponent> DisplayManager;

private:

	bool IsPlacedDomainOperational() const;
	UBathhouseFacilitySlotComponent* GetCustomerSlot() const;
};
