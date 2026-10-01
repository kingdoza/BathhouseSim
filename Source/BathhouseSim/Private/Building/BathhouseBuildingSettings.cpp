#include "Building/BathhouseBuildingSettings.h"

#include "Cleaning/LitterSpawnZoneActor.h"
#include "Cleaning/StainSpawnZoneActor.h"
#include "Engine/StaticMesh.h"

UBathhouseBuildingSettings::UBathhouseBuildingSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Bathhouse Building");
}

UStaticMesh* UBathhouseBuildingSettings::LoadShellBoxMesh() const
{
	return ShellBoxMesh.IsNull() ? nullptr
		: (ShellBoxMesh.IsValid() ? ShellBoxMesh.Get() : ShellBoxMesh.LoadSynchronous());
}

UClass* UBathhouseBuildingSettings::LoadLitterChunkZoneClass() const
{
	return LitterChunkZoneClass.IsNull() ? nullptr
		: (LitterChunkZoneClass.IsValid() ? LitterChunkZoneClass.Get() : LitterChunkZoneClass.LoadSynchronous());
}

UClass* UBathhouseBuildingSettings::LoadStainChunkZoneClass() const
{
	return StainChunkZoneClass.IsNull() ? nullptr
		: (StainChunkZoneClass.IsValid() ? StainChunkZoneClass.Get() : StainChunkZoneClass.LoadSynchronous());
}
