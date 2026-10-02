#include "Building/BathhouseCleaningChunkSpawner.h"

#include "Building/BathhouseBuildingSettings.h"
#include "Building/BathhouseSpaceActor.h"
#include "Cleaning/LitterSpawnZoneActor.h"
#include "Cleaning/StainSpawnZoneActor.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

void FBathhouseCleaningChunkSpawner::Spawn(
	ABathhouseSpaceActor& Space,
	const EBathhouseCleaningChunkKind Kind,
	const TArray<FBox2D>& Rects,
	const double FloorZ,
	TArray<TWeakObjectPtr<AActor>>& OutChunks)
{
	UWorld* World = Space.GetWorld();
	if (!World || Kind == EBathhouseCleaningChunkKind::None)
	{
		return;
	}
	const UBathhouseBuildingSettings* Settings = GetDefault<UBathhouseBuildingSettings>();
	UClass* ChunkClass = Kind == EBathhouseCleaningChunkKind::Litter
		? Settings->LoadLitterChunkZoneClass() : Settings->LoadStainChunkZoneClass();
	if (!ChunkClass)
	{
		UE_LOG(LogBathhouseBuilding, Error,
			TEXT("%s: cleaning chunk class for the selected kind is not set in Project Settings > Bathhouse Building."),
			*Space.GetPathName());
		return;
	}
	for (const FBox2D& Rect : Rects)
	{
		const FVector2D Center = Rect.GetCenter();
		const FVector2D Half = Rect.GetExtent();
		FTransform Transform(FQuat::Identity, FVector(Center.X, Center.Y, FloorZ));
		AActor* Chunk = World->SpawnActorDeferred<AActor>(
			ChunkClass, Transform, &Space, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Chunk)
		{
			continue;
		}
		USceneComponent* SpawnFloor = nullptr;
		if (ALitterSpawnZoneActor* Litter = Cast<ALitterSpawnZoneActor>(Chunk))
		{
			Litter->SetSpawnAreaHalfSizeXY(Half);
			SpawnFloor = Litter->GetSpawnFloor();
		}
		else if (AStainSpawnZoneActor* Stain = Cast<AStainSpawnZoneActor>(Chunk))
		{
			Stain->SetSpawnAreaHalfSizeXY(Half);
			SpawnFloor = Stain->GetSpawnFloor();
		}
		// SpawnFloor 상대 Z만큼 Actor Z를 보정해 SpawnFloor world Z가 바닥 윗면이 되게 한다.
		const double FloorRelativeZ = SpawnFloor ? SpawnFloor->GetRelativeLocation().Z : 0.0;
		Transform.SetLocation(FVector(Center.X, Center.Y, FloorZ - FloorRelativeZ));
		Chunk->FinishSpawning(Transform);
		OutChunks.Add(Chunk);
	}
}
