#include "Tests/FacilityPlacementFootprintPreviewTestProbe.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

AFacilityPlacementFootprintScaleProbe::AFacilityPlacementFootprintScaleProbe()
{
	PackagePhysicalRoot->SetRelativeScale3D_Direct(FVector(2.0f, 1.0f, 1.0f));
	SceneRoot->SetRelativeScale3D_Direct(FVector(1.0f, 3.0f, 1.0f));
}

AFacilityPlacementFootprintYawPlus90Probe::AFacilityPlacementFootprintYawPlus90Probe()
{
	SceneRoot->SetRelativeRotation_Direct(FRotator(0.0f, 90.0f, 0.0f));
}

AFacilityPlacementFootprintYawMinus90Probe::AFacilityPlacementFootprintYawMinus90Probe()
{
	SceneRoot->SetRelativeRotation_Direct(FRotator(0.0f, -90.0f, 0.0f));
}

AFacilityPlacementFootprintYawSmallProbe::AFacilityPlacementFootprintYawSmallProbe()
{
	SceneRoot->SetRelativeRotation_Direct(FRotator(0.0f, 7.0f, 0.0f));
}
