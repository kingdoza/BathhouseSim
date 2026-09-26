#pragma once

#include "CoreMinimal.h"
#include "Utility/BathWaterFuelUtilityFacilityActor.h"
#include "BathWaterCoolerFacilityActor.generated.h"

UCLASS(Blueprintable)
class BATHHOUSESIM_API ABathWaterCoolerFacilityActor : public ABathWaterFuelUtilityFacilityActor
{
	GENERATED_BODY()

public:
	ABathWaterCoolerFacilityActor();
	virtual EUtilityFuelKind GetAcceptedFuelKind() const override { return EUtilityFuelKind::DryIce; }
	virtual EBathWaterCapacityKind GetRequiredCapacityKind() const override { return EBathWaterCapacityKind::Cooling; }
	virtual FText GetFuelFacilityDisplayName() const override;
};
