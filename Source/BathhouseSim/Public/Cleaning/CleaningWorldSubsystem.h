#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CleaningWorldSubsystem.generated.h"

class AStainSpawnZoneActor;
class AWaterStainActor;
class ALitterActor;
class ALitterSpawnZoneActor;
struct FFacilityPlacedEvent;

UCLASS()
class BATHHOUSESIM_API UCleaningWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	void RegisterLitterZone(ALitterSpawnZoneActor* Zone);
	void UnregisterLitterZone(ALitterSpawnZoneActor* Zone);
	void RegisterLitter(ALitterActor* Litter);
	void UnregisterLitter(ALitterActor* Litter);
	TArray<ALitterSpawnZoneActor*> GetActiveLitterZones();
	int32 GetActiveLitterCount();
	int32 GetActiveLitterCountForZone(const ALitterSpawnZoneActor* Zone);
	bool IsLitterLocationClear(const FVector& Location, float MinimumSpacing);
	void RegisterZone(AStainSpawnZoneActor* Zone);
	void UnregisterZone(AStainSpawnZoneActor* Zone);
	void RegisterStain(AWaterStainActor* Stain);
	void UnregisterStain(AWaterStainActor* Stain);

	TArray<AStainSpawnZoneActor*> GetActiveZones();
	int32 GetActiveStainCount();
	int32 GetActiveStainCountForZone(const AStainSpawnZoneActor* Zone);
	bool IsStainLocationClear(const FVector& Location, float MinimumSpacing);

private:
	void Compact();
	void HandleFacilityPlaced(const FFacilityPlacedEvent& Event);
	FDelegateHandle PlacementHandle;
	TArray<TWeakObjectPtr<ALitterSpawnZoneActor>> LitterZones;
	TArray<TWeakObjectPtr<ALitterActor>> LitterActors;

	TArray<TWeakObjectPtr<AStainSpawnZoneActor>> Zones;
	TArray<TWeakObjectPtr<AWaterStainActor>> Stains;
};
