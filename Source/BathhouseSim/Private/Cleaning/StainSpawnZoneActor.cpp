#include "Cleaning/StainSpawnZoneActor.h"

#include "Cleaning/CleaningWorldSubsystem.h"
#include "Cleaning/CleaningSpawnRules.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

AStainSpawnZoneActor::AStainSpawnZoneActor()
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

void AStainSpawnZoneActor::BeginPlay()
{
	Super::BeginPlay();
	if (UCleaningWorldSubsystem* Subsystem = GetWorld()->GetSubsystem<UCleaningWorldSubsystem>())
	{
		Subsystem->RegisterZone(this);
	}
}

void AStainSpawnZoneActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UCleaningWorldSubsystem* Subsystem = World->GetSubsystem<UCleaningWorldSubsystem>())
		{
			Subsystem->UnregisterZone(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

bool AStainSpawnZoneActor::FindSpawnTransform(FRandomStream& Stream, float DefaultStainSpacing,
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
	Settings.Spacing = StainSpacingOverride > 0 ? StainSpacingOverride : DefaultStainSpacing;
	return FCleaningFloorSpawnQuery::Find(
		*World, this, Settings, Stream,
		[Subsystem](const FVector& Location, float Spacing)
		{
			return Subsystem->IsStainLocationClear(Location, Spacing);
		},
		OutTransform);
}
