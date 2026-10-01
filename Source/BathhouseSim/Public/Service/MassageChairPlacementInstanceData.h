#pragma once
#include "Facility/BathhouseFacilityPlacementInstanceData.h"
#include "MassageChairPlacementInstanceData.generated.h"
UCLASS(Transient)

class BATHHOUSESIM_API UMassageChairPlacementInstanceData : public UBathhouseFacilityPlacementInstanceData
{
	GENERATED_BODY()
public:

	UPROPERTY(Transient)
	bool bBroken = false;
	virtual FText GetPlacementContentsSummary() const override;
};
