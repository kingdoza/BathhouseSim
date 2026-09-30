#pragma once

#include "CoreMinimal.h"
#include "Facility/BathhouseFacilityPlacementInstanceData.h"
#include "Service/ServiceItemTypes.h"
#include "DrinkFridgePlacementInstanceData.generated.h"

/** Recovery payload of a drink fridge: every display space's kind and count. */
UCLASS(Transient, NotBlueprintable)
class BATHHOUSESIM_API UDrinkFridgePlacementInstanceData : public UBathhouseFacilityPlacementInstanceData
{
	GENERATED_BODY()

public:
	virtual FText GetPlacementContentsSummary() const override;

	UPROPERTY()
	TArray<FDisplaySpaceSnapshot> Spaces;
};
