#pragma once
#include "CoreMinimal.h"
#include "Facility/FacilityPlacementExtension.h"
#include "Service/ServiceItemTypes.h"
#include "ServiceDisplayPlacementData.generated.h"
UCLASS(Transient, NotBlueprintable)

class BATHHOUSESIM_API UServiceDisplayPlacementData : public UFacilityPlacementExtensionData
{
	GENERATED_BODY()
public:

	UPROPERTY(Transient)
	TArray<FDisplaySpaceSnapshot> Spaces;
	/** Fridge summaries retain the existing bottle suffix. Other facilities use items. */
	UPROPERTY(Transient)
	bool bBottleSummary = false;
	virtual FText GetContentsSummary() const override;
};
