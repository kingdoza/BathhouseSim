#include "Utility/BathWaterCoolerFacilityActor.h"

#include "Facility/BathWaterOperationsTypes.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"

#define LOCTEXT_NAMESPACE "BathWaterCoolerFacilityActor"

ABathWaterCoolerFacilityActor::ABathWaterCoolerFacilityActor()
{
	Capacity->RestoreCapacity(EBathWaterCapacityKind::Cooling, Capacity->GetCapacityPoints());
}

FText ABathWaterCoolerFacilityActor::GetFuelFacilityDisplayName() const
{
	return LOCTEXT("CoolerDisplayName", "냉각기");
}

#undef LOCTEXT_NAMESPACE
