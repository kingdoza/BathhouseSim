#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FacilityPlacementEventSubsystem.generated.h"

struct BATHHOUSESIM_API FFacilityPlacedEvent
{
	TWeakObjectPtr<AActor> PlacedActor;
	FTransform FootprintTransform = FTransform::Identity;
	FVector UnscaledExtent = FVector::ZeroVector;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFacilityPlacedNative, const FFacilityPlacedEvent&);

UCLASS()

class BATHHOUSESIM_API UFacilityPlacementEventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:

	FOnFacilityPlacedNative OnFacilityPlaced;
	void BroadcastFacilityPlaced(const FFacilityPlacedEvent& Event);
};
