#pragma once

#include "CoreMinimal.h"
#include "Tests/FacilityPlacementAutomationTestProbe.h"
#include "FacilityPlacementFootprintPreviewTestProbe.generated.h"

// Reproduces Blueprint CDO layouts: relative scale/rotation set without refreshing ComponentToWorld.
UCLASS(Transient, NotBlueprintable)
class AFacilityPlacementFootprintScaleProbe final : public AFacilityPlacementAutomationActor
{
	GENERATED_BODY()

public:
	AFacilityPlacementFootprintScaleProbe();
};

UCLASS(Transient, NotBlueprintable)
class AFacilityPlacementFootprintYawPlus90Probe final : public AFacilityPlacementAutomationActor
{
	GENERATED_BODY()

public:
	AFacilityPlacementFootprintYawPlus90Probe();
};

UCLASS(Transient, NotBlueprintable)
class AFacilityPlacementFootprintYawMinus90Probe final : public AFacilityPlacementAutomationActor
{
	GENERATED_BODY()

public:
	AFacilityPlacementFootprintYawMinus90Probe();
};

UCLASS(Transient, NotBlueprintable)
class AFacilityPlacementFootprintYawSmallProbe final : public AFacilityPlacementAutomationActor
{
	GENERATED_BODY()

public:
	AFacilityPlacementFootprintYawSmallProbe();
};

// Cooler-like layout: non-integer root scale and a rotated SceneRoot.
UCLASS(Transient, NotBlueprintable)
class AFacilityPlacementFootprintHalfRootScaleProbe final : public AFacilityPlacementAutomationActor
{
	GENERATED_BODY()

public:
	AFacilityPlacementFootprintHalfRootScaleProbe();
};
