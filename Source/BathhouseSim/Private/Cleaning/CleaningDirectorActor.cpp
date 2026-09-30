#include "Cleaning/CleaningDirectorActor.h"
#include "Cleaning/CleaningSpawnRules.h"
#include "Cleaning/CleaningWorldSubsystem.h"
#include "Cleaning/StainSpawnZoneActor.h"
#include "Cleaning/LitterSpawnZoneActor.h"
#include "Cleaning/WaterStainActor.h"
#include "Cleaning/LitterActor.h"
#include "Components/BoxComponent.h"
#include "Customer/BathhouseCustomerCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

ACleaningDirectorActor::ACleaningDirectorActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACleaningDirectorActor::BeginPlay()
{
	Super::BeginPlay();
	SpawnRandom.Initialize(FMath::Rand());
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ACleaningDirectorActor::UpdateSpawnSchedule,
									FMath::Max(0.05f, SpawnUpdateIntervalSeconds), true);
}

void ACleaningDirectorActor::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	StainClocks.Reset();
	LitterClocks.Reset();
	Super::EndPlay(Reason);
}

void ACleaningDirectorActor::UpdateSpawnSchedule()
{
	TArray<FVector> Locations;
	for (TActorIterator<ABathhouseCustomerCharacter> It(GetWorld()); It; ++It)
	{
		Locations.Add(It->GetActorLocation());
	}
	AdvanceSpawnSchedule(FMath::Max(0.05f, SpawnUpdateIntervalSeconds), Locations);
}

void ACleaningDirectorActor::AdvanceSpawnSchedule(float DeltaSeconds, const TArray<FVector>& Locations)
{
	auto* Subsystem = GetWorld()->GetSubsystem<UCleaningWorldSubsystem>();
	if (!Subsystem)
	{
		return;
	}
	auto AdvanceZones = [&](auto& Clocks, const auto& Zones, float Mean, auto Spawn)
	{
		for (auto It = Clocks.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid() || !Zones.Contains(It.Key().Get()))
			{
				It.RemoveCurrent();
			}
		}
		for (auto* Zone : Zones)
		{
			auto* Bounds = Zone->GetSpawnBounds();
			if (!Bounds)
			{
				continue;
			}
			float* Remaining = Clocks.Find(Zone);
			if (!Remaining)
			{
				Remaining = &Clocks.Add(Zone, FCleaningSpawnClock::SampleNext(SpawnRandom));
			}
			const int32 Count =
				CountCustomersInBox(Locations, Bounds->GetComponentTransform(), Bounds->GetUnscaledBoxExtent());
			if (FCleaningSpawnClock::Advance(*Remaining, Count, DeltaSeconds, Mean))
			{
				*Remaining = FCleaningSpawnClock::SampleNext(SpawnRandom);
				Spawn(*Zone);
			}
		}
	};
	AdvanceZones(StainClocks, Subsystem->GetActiveZones(), SpawnIntervalSeconds,
				 [this](AStainSpawnZoneActor& Zone)
				 {
					 TrySpawnStain(Zone);
				 });
	AdvanceZones(LitterClocks, Subsystem->GetActiveLitterZones(), LitterMeanIntervalPerCustomerSeconds,
				 [this](ALitterSpawnZoneActor& Zone)
				 {
					 TrySpawnLitter(Zone);
				 });
}

void ACleaningDirectorActor::TrySpawnStain(AStainSpawnZoneActor& Zone)
{
	auto* S = GetWorld()->GetSubsystem<UCleaningWorldSubsystem>();
	if (!StainClass || !S || S->GetActiveStainCount() >= MaxActiveStains ||
		S->GetActiveStainCountForZone(&Zone) >= Zone.GetMaxActiveStains())
	{
		return;
	}
	const auto* CDO = StainClass->GetDefaultObject<AWaterStainActor>();
	for (int32 Attempt = 0; Attempt < MaxPlacementAttemptsPerInterval; ++Attempt)
	{
		FTransform Transform;
		if (!Zone.FindSpawnTransform(SpawnRandom, DefaultStainSpacing, Transform, CDO->GetMaximumFloorRadius(),
									 SpawnClearanceHeightCm))
		{
			continue;
		}
		auto* Stain = GetWorld()->SpawnActorDeferred<AWaterStainActor>(StainClass, Transform, this, nullptr,
																	   ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Stain)
		{
			Stain->ConfigureVisualVariationSeed(SpawnRandom.RandHelper(MAX_int32));
			Stain->SetSpawnZone(&Zone);
			Stain->FinishSpawning(Transform);
		}
		return;
	}
}

void ACleaningDirectorActor::TrySpawnLitter(ALitterSpawnZoneActor& Zone)
{
	auto* S = GetWorld()->GetSubsystem<UCleaningWorldSubsystem>();
	if (!LitterClass || !S || S->GetActiveLitterCount() >= MaxActiveLitter ||
		S->GetActiveLitterCountForZone(&Zone) >= Zone.GetMaxActiveLitter())
	{
		return;
	}
	const auto* CDO = LitterClass->GetDefaultObject<ALitterActor>();
	for (int32 Attempt = 0; Attempt < MaxPlacementAttemptsPerInterval; ++Attempt)
	{
		FTransform Transform;
		if (!Zone.FindSpawnTransform(SpawnRandom, DefaultLitterSpacing, Transform, CDO->GetFloorRadius(),
									 SpawnClearanceHeightCm))
		{
			continue;
		}
		auto* Litter = GetWorld()->SpawnActorDeferred<ALitterActor>(LitterClass, Transform, this, nullptr,
																	ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Litter)
		{
			Litter->ConfigureVisualVariationSeed(SpawnRandom.RandHelper(MAX_int32));
			Litter->SetSpawnZone(&Zone);
			Litter->FinishSpawning(Transform);
		}
		return;
	}
}
#if WITH_DEV_AUTOMATION_TESTS
void ACleaningDirectorActor::AdvanceSpawnScheduleForTesting(float DeltaSeconds, const TArray<FVector>& Locations)
{
	AdvanceSpawnSchedule(DeltaSeconds, Locations);
}

void ACleaningDirectorActor::SetSpawnRandomSeedForTesting(int32 Seed)
{
	SpawnRandom.Initialize(Seed);
	StainClocks.Reset();
	LitterClocks.Reset();
}
#endif
