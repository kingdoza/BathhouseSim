#include "Cleaning/LitterSpawnZoneActor.h"

#include "Cleaning/CleaningWorldSubsystem.h"
#include "Cleaning/CleaningSpawnRules.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

ALitterSpawnZoneActor::ALitterSpawnZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SpawnBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBounds"));
	SetRootComponent(SpawnBounds);
	SpawnBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBounds->InitBoxExtent(FVector(300.0f, 300.0f, 100.0f));
	SpawnFloor = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnFloor"));
	SpawnFloor->SetupAttachment(SpawnBounds);
	SpawnFloor->SetRelativeLocation(FVector(0, 0, -100));
	SpawnBounds->SetCanEverAffectNavigation(false);
}

void ALitterSpawnZoneActor::BeginPlay()
{
	Super::BeginPlay();
	if (UCleaningWorldSubsystem* Subsystem = GetWorld()->GetSubsystem<UCleaningWorldSubsystem>())
	{
		Subsystem->RegisterLitterZone(this);
	}
}

void ALitterSpawnZoneActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UCleaningWorldSubsystem* Subsystem = World->GetSubsystem<UCleaningWorldSubsystem>())
		{
			Subsystem->UnregisterLitterZone(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

bool ALitterSpawnZoneActor::FindSpawnTransform(FRandomStream& Stream, float DefaultLitterSpacing,
											   FTransform& OutTransform, float FloorRadius, float ClearanceHeight) const
{
	UWorld* World = GetWorld();
	auto* Subsystem = World ? World->GetSubsystem<UCleaningWorldSubsystem>() : nullptr;
	if (!World || !Subsystem || !SpawnBounds || !SpawnFloor)
	{
		return false;
	}
	FCleaningFloorSpawnSettings Settings;
	Settings.BoxTransform = SpawnBounds->GetComponentTransform();
	Settings.Extent = SpawnBounds->GetUnscaledBoxExtent();
	Settings.TraceChannel = FloorTraceChannel;
	Settings.TraceDistance = FloorTraceDistance;
	Settings.RequiredFloorTag = RequiredFloorComponentTag;
	Settings.MaximumSlopeDegrees = MaximumFloorSlopeDegrees;
	Settings.FloorZ = SpawnFloor->GetComponentLocation().Z;
	Settings.FloorTolerance = FloorHeightToleranceCm;
	Settings.Radius = FloorRadius;
	Settings.ClearanceHeight = ClearanceHeight;
	Settings.Spacing = LitterSpacingOverride > 0 ? LitterSpacingOverride : DefaultLitterSpacing;
	return FCleaningFloorSpawnQuery::Find(
		*World, this, Settings, Stream,
		[Subsystem](const FVector& Location, float Spacing)
		{
			return Subsystem->IsLitterLocationClear(Location, Spacing);
		},
		OutTransform);
}
