#include "Cleaning/CleaningWorldSubsystem.h"

#include "Cleaning/StainSpawnZoneActor.h"
#include "Cleaning/WaterStainActor.h"
#include "Cleaning/LitterActor.h"
#include "Cleaning/LitterSpawnZoneActor.h"
#include "Cleaning/CleaningSpawnRules.h"
#include "Placement/FacilityPlacementEventSubsystem.h"
#include "Engine/World.h"

void UCleaningWorldSubsystem::RegisterZone(AStainSpawnZoneActor* Zone)
{
	Compact();
	if (IsValid(Zone) && !Zones.Contains(Zone))
	{
		Zones.Add(Zone);
	}
}

void UCleaningWorldSubsystem::UnregisterZone(AStainSpawnZoneActor* Zone)
{
	Zones.RemoveAll([Zone](const TWeakObjectPtr<AStainSpawnZoneActor>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Zone;
	});
}

void UCleaningWorldSubsystem::RegisterStain(AWaterStainActor* Stain)
{
	Compact();
	if (IsValid(Stain) && !Stains.Contains(Stain))
	{
		Stains.Add(Stain);
	}
}

void UCleaningWorldSubsystem::UnregisterStain(AWaterStainActor* Stain)
{
	Stains.RemoveAll([Stain](const TWeakObjectPtr<AWaterStainActor>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Stain;
	});
}

TArray<AStainSpawnZoneActor*> UCleaningWorldSubsystem::GetActiveZones()
{
	Compact();
	TArray<AStainSpawnZoneActor*> Result;
	Result.Reserve(Zones.Num());
	for (const TWeakObjectPtr<AStainSpawnZoneActor>& Zone : Zones)
	{
		Result.Add(Zone.Get());
	}
	return Result;
}

int32 UCleaningWorldSubsystem::GetActiveStainCount()
{
	Compact();
	return Stains.Num();
}

int32 UCleaningWorldSubsystem::GetActiveStainCountForZone(const AStainSpawnZoneActor* Zone)
{
	Compact();
	int32 Count = 0;
	for (const TWeakObjectPtr<AWaterStainActor>& Stain : Stains)
	{
		if (Stain.IsValid() && Stain->GetSpawnZone() == Zone)
		{
			++Count;
		}
	}
	return Count;
}

bool UCleaningWorldSubsystem::IsStainLocationClear(const FVector& Location, const float MinimumSpacing)
{
	Compact();
	const float MinimumSpacingSquared = FMath::Square(FMath::Max(0.0f, MinimumSpacing));
	for (const TWeakObjectPtr<AWaterStainActor>& Stain : Stains)
	{
		if (Stain.IsValid() && FVector::DistSquared(Stain->GetActorLocation(), Location) < MinimumSpacingSquared)
		{
			return false;
		}
	}
	return true;
}

void UCleaningWorldSubsystem::RegisterLitterZone(ALitterSpawnZoneActor* Zone)
{
	Compact();
	if (IsValid(Zone) && !LitterZones.Contains(Zone))
	{
		LitterZones.Add(Zone);
	}
}

void UCleaningWorldSubsystem::UnregisterLitterZone(ALitterSpawnZoneActor* Zone)
{
	LitterZones.RemoveAll(
		[Zone](const TWeakObjectPtr<ALitterSpawnZoneActor>& Entry)
		{
			return !Entry.IsValid() || Entry.Get() == Zone;
		});
}

void UCleaningWorldSubsystem::RegisterLitter(ALitterActor* Litter)
{
	Compact();
	if (IsValid(Litter) && !LitterActors.Contains(Litter))
	{
		LitterActors.Add(Litter);
	}
}

void UCleaningWorldSubsystem::UnregisterLitter(ALitterActor* Litter)
{
	LitterActors.RemoveAll(
		[Litter](const TWeakObjectPtr<ALitterActor>& Entry)
		{
			return !Entry.IsValid() || Entry.Get() == Litter;
		});
}

TArray<ALitterSpawnZoneActor*> UCleaningWorldSubsystem::GetActiveLitterZones()
{
	Compact();
	TArray<ALitterSpawnZoneActor*> Result;
	Result.Reserve(LitterZones.Num());
	for (const TWeakObjectPtr<ALitterSpawnZoneActor>& Zone : LitterZones)
	{
		Result.Add(Zone.Get());
	}
	return Result;
}

int32 UCleaningWorldSubsystem::GetActiveLitterCount()
{
	Compact();
	return LitterActors.Num();
}

int32 UCleaningWorldSubsystem::GetActiveLitterCountForZone(const ALitterSpawnZoneActor* Zone)
{
	Compact();
	int32 Count = 0;
	for (const TWeakObjectPtr<ALitterActor>& Litter : LitterActors)
	{
		if (Litter.IsValid() && Litter->GetSpawnZone() == Zone)
		{
			++Count;
		}
	}
	return Count;
}

bool UCleaningWorldSubsystem::IsLitterLocationClear(const FVector& Location, const float MinimumSpacing)
{
	Compact();
	const float MinimumSpacingSquared = FMath::Square(FMath::Max(0.0f, MinimumSpacing));
	for (const TWeakObjectPtr<ALitterActor>& Litter : LitterActors)
	{
		if (Litter.IsValid() && FVector::DistSquared(Litter->GetActorLocation(), Location) < MinimumSpacingSquared)
		{
			return false;
		}
	}
	return true;
}

void UCleaningWorldSubsystem::Compact()
{
	LitterZones.RemoveAll(
		[](const TWeakObjectPtr<ALitterSpawnZoneActor>& Entry)
		{
			return !Entry.IsValid();
		});
	LitterActors.RemoveAll(
		[](const TWeakObjectPtr<ALitterActor>& Entry)
		{
			return !Entry.IsValid() || !Entry->IsActive();
		});
	Zones.RemoveAll([](const TWeakObjectPtr<AStainSpawnZoneActor>& Entry) { return !Entry.IsValid(); });
	Stains.RemoveAll([](const TWeakObjectPtr<AWaterStainActor>& Entry) { return !Entry.IsValid(); });
}

void UCleaningWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	auto* Events = Collection.InitializeDependency<UFacilityPlacementEventSubsystem>();
	if (Events)
	{
		PlacementHandle = Events->OnFacilityPlaced.AddUObject(this, &UCleaningWorldSubsystem::HandleFacilityPlaced);
	}
}

void UCleaningWorldSubsystem::Deinitialize()
{
	if (auto* Events = GetWorld()->GetSubsystem<UFacilityPlacementEventSubsystem>())
	{
		Events->OnFacilityPlaced.Remove(PlacementHandle);
	}
	PlacementHandle.Reset();
	Zones.Reset();
	Stains.Reset();
	LitterZones.Reset();
	LitterActors.Reset();
	Super::Deinitialize();
}

void UCleaningWorldSubsystem::HandleFacilityPlaced(const FFacilityPlacedEvent& Event)
{
	Compact();
	TArray<TWeakObjectPtr<AWaterStainActor>> RemoveStains;
	TArray<TWeakObjectPtr<ALitterActor>> RemoveLitter;
	for (auto Entry : Stains)
	{
		if (Entry.IsValid() && FCleaningFootprintOverlap::Intersects(Entry->GetActorLocation(), Entry->GetFloorRadius(),
																	 Event.FootprintTransform, Event.UnscaledExtent))
		{
			RemoveStains.Add(Entry);
		}
	}
	for (auto Entry : LitterActors)
	{
		if (Entry.IsValid() && FCleaningFootprintOverlap::Intersects(Entry->GetActorLocation(), Entry->GetFloorRadius(),
																	 Event.FootprintTransform, Event.UnscaledExtent))
		{
			RemoveLitter.Add(Entry);
		}
	}
	for (auto Entry : RemoveStains)
	{
		if (Entry.IsValid())
		{
			Entry->ClearForFacilityPlacement();
		}
	}
	for (auto Entry : RemoveLitter)
	{
		if (Entry.IsValid())
		{
			Entry->ClearForFacilityPlacement();
		}
	}
}
