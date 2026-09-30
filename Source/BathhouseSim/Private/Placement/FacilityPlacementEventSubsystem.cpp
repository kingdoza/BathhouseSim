#include "Placement/FacilityPlacementEventSubsystem.h"

void UFacilityPlacementEventSubsystem::BroadcastFacilityPlaced(const FFacilityPlacedEvent& Event)
{
	if (Event.PlacedActor.IsValid())
	{
		OnFacilityPlaced.Broadcast(Event);
	}
}
