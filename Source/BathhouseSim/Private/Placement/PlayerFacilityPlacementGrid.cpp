#include "Placement/PlayerFacilityPlacementComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementZoneActor.h"

void UPlayerFacilityPlacementComponent::ShowCompatibleZoneGrids(
	const UFacilityPlacementDefinition& Definition)
{
	HideVisibleZoneGrids();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AFacilityPlacementZoneActor> It(World); It; ++It)
	{
		AFacilityPlacementZoneActor* Zone = *It;
		if (!IsValid(Zone) || !Zone->IsNetStartupActor()
			|| !Zone->IsDefinitionAllowed(Definition))
		{
			continue;
		}
		if (Zone->SetGridVisible(true))
		{
			VisibleGridZones.Add(Zone);
		}
	}
}

void UPlayerFacilityPlacementComponent::HideVisibleZoneGrids()
{
	for (const TWeakObjectPtr<AFacilityPlacementZoneActor>& WeakZone : VisibleGridZones)
	{
		if (AFacilityPlacementZoneActor* Zone = WeakZone.Get())
		{
			Zone->SetGridVisible(false);
		}
	}
	VisibleGridZones.Reset();
}
