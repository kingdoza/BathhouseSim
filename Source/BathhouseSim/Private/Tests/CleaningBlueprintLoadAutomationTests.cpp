#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/Blueprint.h"
#include "Cleaning/CleaningDirectorActor.h"
#include "Cleaning/StainSpawnZoneActor.h"
#include "Cleaning/LitterSpawnZoneActor.h"
#include "Cleaning/WaterStainActor.h"
#include "Components/SceneComponent.h"
#include "Service/ItemBoxActor.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Shop/BathhouseTrashBinActor.h"
#include "WorldPartition/WorldPartition.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningBlueprintLoadTest, "BathhouseSim.Cleaning.BlueprintLoad",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningBlueprintLoadTest::RunTest(const FString&)
{
	struct FSpec
	{
		const TCHAR* Name;
		const TCHAR* Folder;
		UClass* Parent;
		TArray<FName> Components;
	};

	const FSpec Specs[] = {
		{TEXT("BP_CleaningDirector"), TEXT("Cleaning"), ACleaningDirectorActor::StaticClass(), {}},
		{TEXT("BP_StainSpawnZone"),
		 TEXT("Cleaning"),
		 AStainSpawnZoneActor::StaticClass(),
		 {TEXT("SpawnBounds"), TEXT("SpawnFloor")}},
		{TEXT("BP_WaterStain"),
		 TEXT("Cleaning"),
		 AWaterStainActor::StaticClass(),
		 {TEXT("InteractionCollision"), TEXT("StainVisualRoot")}},
		{TEXT("BP_ItemBox"), TEXT("Service"), AItemBoxActor::StaticClass(), {TEXT("BoxMesh"), TEXT("ContentsVisual")}},
		{TEXT("BP_ShopDeliveryBox"), TEXT("Shop"), AShopDeliveryBoxActor::StaticClass(), {TEXT("BoxMesh")}},
		{TEXT("BP_PlaceableFacilityItem"),
		 TEXT("Placement"),
		 APlaceableFacilityItemActor::StaticClass(),
		 {TEXT("ItemRoot")}}};
	const bool bCopies = FParse::Param(FCommandLine::Get(), TEXT("BathhouseCleaningLoadCopies"));
	TestNull(TEXT("SelectionWeight removed from reflection"),
			 FindFProperty<FProperty>(AStainSpawnZoneActor::StaticClass(), TEXT("SelectionWeight")));
	TestNull(TEXT("ZoneKind removed from reflection"),
			 FindFProperty<FProperty>(AStainSpawnZoneActor::StaticClass(), TEXT("ZoneKind")));
	TestNull(TEXT("Director Pawn clearance removed from reflection"),
			 FindFProperty<FProperty>(ACleaningDirectorActor::StaticClass(), TEXT("DefaultPawnClearance")));
	TestNull(TEXT("Stain zone Pawn clearance removed from reflection"),
			 FindFProperty<FProperty>(AStainSpawnZoneActor::StaticClass(), TEXT("PawnClearanceOverride")));
	TestNull(TEXT("Litter zone Pawn clearance removed from reflection"),
			 FindFProperty<FProperty>(ALitterSpawnZoneActor::StaticClass(), TEXT("PawnClearanceOverride")));
	for (const FSpec& Spec : Specs)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("BathhouseCleaningClearanceRework")) &&
			Spec.Parent != ACleaningDirectorActor::StaticClass() && Spec.Parent != AStainSpawnZoneActor::StaticClass())
		{
			continue;
		}

		const FString Path = bCopies
								 ? FString::Printf(TEXT("/Game/Developers/MigrationCheck/%s"), Spec.Name)
								 : FString::Printf(TEXT("/Game/Bathhouse/Blueprints/%s/%s"), Spec.Folder, Spec.Name);
		auto* Package = LoadPackage(nullptr, *Path, LOAD_None);
		if (!TestNotNull(Path, Package))
		{
			return false;
		}
		UBlueprint* BP = nullptr;
		ForEachObjectWithOuter(Package,
							   [&](UObject* Object)
							   {
								   if (auto* Found = Cast<UBlueprint>(Object))
								   {
									   BP = Found;
								   }
							   });
		if (!TestNotNull(TEXT("Blueprint loads"), BP) ||
			!TestNotNull(TEXT("Generated class loads"), BP->GeneratedClass.Get()))
		{
			return false;
		}
		TestTrue(TEXT("Native parent preserved"), BP->GeneratedClass->GetSuperClass() == Spec.Parent);
		auto* CDO = Cast<AActor>(BP->GeneratedClass->GetDefaultObject());
		if (!TestNotNull(TEXT("CDO loads"), CDO))
		{
			return false;
		}
		for (FName Name : Spec.Components)
		{
			TestNotNull(FString::Printf(TEXT("Native subobject %s retained"), *Name.ToString()),
						CDO->GetDefaultSubobjectByName(Name));
		}
		if (Spec.Parent == AStainSpawnZoneActor::StaticClass())
		{
			auto* Floor = Cast<USceneComponent>(CDO->GetDefaultSubobjectByName(TEXT("SpawnFloor")));
			TestTrue(TEXT("New floor attaches to surviving bounds"),
					 Floor && Floor->GetAttachParent() == CDO->GetDefaultSubobjectByName(TEXT("SpawnBounds")));
		}
		TestFalse(TEXT("Load does not dirty package"), Package->IsDirty());
		AddInfo(Path);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("BathhouseCleaningLoadDefaultMap")))
	{
		UWorld* MapWorld = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (auto* World = Context.World())
			{
				if (World->GetPackage()->GetName() == TEXT("/Game/Maps/DefaultMap"))
				{
					MapWorld = World;
				}
			}
		}
		if (!TestNotNull(TEXT("DefaultMap loaded as requested"), MapWorld))
		{
			return false;
		}
		auto* Partition = MapWorld->GetWorldPartition();
		if (!TestNotNull(TEXT("DefaultMap world partition exists"), Partition))
		{
			return false;
		}
		int32 Directors = 0, Zones = 0, Bins = 0;
		for (UWorldPartition::TIterator<> It(Partition); It; ++It)
		{
			const auto Type = It->GetNativeClass();
			const bool bDirector = Type == ACleaningDirectorActor::StaticClass()->GetClassPathName();
			const bool bZone = Type == AStainSpawnZoneActor::StaticClass()->GetClassPathName();
			const bool bBin = Type == ABathhouseTrashBinActor::StaticClass()->GetClassPathName();
			if (!bDirector && !bZone && !bBin)
			{
				continue;
			}
			auto* Package = LoadPackage(nullptr, *It->GetActorPackage().ToString(), LOAD_None);
			if (!TestNotNull(TEXT("Existing external actor package loads"), Package))
			{
				return false;
			}
			AActor* Instance = nullptr;
			ForEachObjectWithPackage(Package,
									 [&](UObject* Object)
									 {
										 if (auto* Actor = Cast<AActor>(Object))
										 {
											 if (Actor->IsA(bDirector ? ACleaningDirectorActor::StaticClass()
															: bZone	  ? AStainSpawnZoneActor::StaticClass()
																	  : ABathhouseTrashBinActor::StaticClass()))
											 {
												 Instance = Actor;
											 }
										 }
										 return true;
									 });
			if (!TestNotNull(TEXT("Existing level actor retained"), Instance))
			{
				return false;
			}
			if (bZone)
			{
				++Zones;
				TestNotNull(TEXT("Level stain zone adds native floor"),
							Instance->GetDefaultSubobjectByName(TEXT("SpawnFloor")));
			}
			if (bDirector)
			{
				++Directors;
				const auto* P = FindFProperty<FFloatProperty>(Instance->GetClass(), TEXT("SpawnIntervalSeconds"));
				AddInfo(FString::Printf(TEXT("Director retained interval: %.3f"),
										P->GetPropertyValue_InContainer(Instance)));
			}
			if (bBin)
			{
				++Bins;
			}
			TestFalse(TEXT("External package not dirtied by load"), Package->IsDirty());
			AddInfo(Package->GetName() + TEXT(" ") + Instance->GetName());
		}
		TestTrue(TEXT("Existing director instance retained"), Directors > 0);
		TestTrue(TEXT("Existing stain zones retained"), Zones > 0);
		TestTrue(TEXT("Existing trash bin retained until Editor pass"), Bins > 0);
		AddInfo(
			FString::Printf(TEXT("DefaultMap: %d directors, %d stain zones, %d trash bins"), Directors, Zones, Bins));
	}
	return true;
}
#endif
