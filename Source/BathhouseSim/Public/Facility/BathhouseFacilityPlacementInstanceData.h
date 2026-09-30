#pragma once

#include "CoreMinimal.h"
#include "Facility/BathhouseFacilityTypes.h"
#include "Placement/FacilityPlacementPayload.h"
#include "Facility/FacilityPlacementExtension.h"
#include "BathhouseFacilityPlacementInstanceData.generated.h"

UCLASS(Transient, NotBlueprintable)
class BATHHOUSESIM_API UBathhouseFacilityPlacementInstanceData
	: public UFacilityPlacementInstanceData
{
	GENERATED_BODY()

public:

	virtual FText GetPlacementContentsSummary() const override;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UFacilityPlacementExtensionData>> Extensions;
	EBathhouseFacilityType FacilityType = EBathhouseFacilityType::Bath;
	int32 FacilityNumber = INDEX_NONE;
	float SelectionWeight = 1.0f;
	bool bEnabled = true;
};
