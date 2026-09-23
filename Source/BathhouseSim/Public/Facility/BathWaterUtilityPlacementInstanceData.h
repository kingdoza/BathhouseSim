#pragma once

#include "CoreMinimal.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "Placement/FacilityPlacementPayload.h"
#include "BathWaterUtilityPlacementInstanceData.generated.h"

UCLASS()
class BATHHOUSESIM_API UBathWaterUtilityPlacementInstanceData : public UFacilityPlacementInstanceData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	EBathWaterCapacityKind CapacityKind = EBathWaterCapacityKind::Circulation;

	UPROPERTY()
	float CapacityPoints = 100.0f;
};

