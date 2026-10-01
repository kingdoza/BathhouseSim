#pragma once
#include "Tests/ServiceFacilityAutomationTestSupport.h"
#include "Cleaning/CleaningDirectorActor.h"
#include "Cleaning/CleaningSpawnRules.h"
#include "Cleaning/CleaningWorldSubsystem.h"
#include "Cleaning/LitterActor.h"
#include "Cleaning/LitterSpawnZoneActor.h"
#include "Cleaning/LitterTongsActor.h"
#include "Cleaning/StainSpawnZoneActor.h"
#include "Cleaning/TrashBagDropPlacement.h"
#include "Cleaning/TrashBagActor.h"
#include "Cleaning/TrashCollectionZoneActor.h"
#include "Cleaning/WaterStainActor.h"
#include "Cleaning/WetMopActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Placement/FacilityPlacementEventSubsystem.h"
#include "GameFramework/WorldSettings.h"

namespace CleaningLitterTest
{
	using namespace ServiceTest;

	template <class P, class V> void Set(UObject* Object, const TCHAR* Name, V Value)
	{
		auto* Property = FindFProperty<P>(Object->GetClass(), Name);
		check(Property);
		Property->SetPropertyValue_InContainer(Object, Value);
	}

	inline UBoxComponent* Box(UWorld* World, const FVector& Center, const FVector& Extent, AActor* Owner = nullptr)
	{
		if (!Owner)
		{
			Owner = World->SpawnActor<AActor>();
		}
		auto* Result = NewObject<UBoxComponent>(Owner);
		Owner->AddInstanceComponent(Result);
		if (!Owner->GetRootComponent())
		{
			Owner->SetRootComponent(Result);
		}
		else
		{
			Result->SetupAttachment(Owner->GetRootComponent());
		}
		Result->SetBoxExtent(Extent);
		Result->SetCollisionObjectType(ECC_WorldStatic);
		Result->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Result->SetCollisionResponseToAllChannels(ECR_Block);
		Result->RegisterComponent();
		Result->SetWorldLocation(Center);
		return Result;
	}

	inline void Mesh(AActor* Actor, const FVector& Scale = FVector(.2))
	{
		auto* Mesh = Actor->FindComponentByClass<UStaticMeshComponent>();
		check(Mesh);
		Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Mesh->SetWorldScale3D(Scale);
		Mesh->UpdateBounds();
	}

	inline void AddCapsule(FPlayer& Player)
	{
		auto* Capsule = NewObject<UCapsuleComponent>(Player.Pawn);
		Capsule->InitCapsuleSize(34, 88);
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Player.Pawn->AddInstanceComponent(Capsule);
		Capsule->RegisterComponent();
		Capsule->SetWorldLocation(FVector(0, 0, 88));
		Player.Camera->SetWorldLocation(FVector(0, 0, 160));
	}

	inline FHeldEquipmentUseContext Context(FPlayer& Player, AActor* Equipment, AActor* Focus = nullptr)
	{
		FHeldEquipmentUseContext Result;
		Result.User = Player.Pawn;
		Result.Equipment = Equipment;
		Result.CarryComponent = Player.Carry;
		Result.InteractionComponent = Player.Interaction;
		Result.Camera = Player.Camera;
		Result.CameraOrigin = Player.Camera->GetComponentLocation();
		Result.CameraDirection = Player.Camera->GetForwardVector().GetSafeNormal();
		if (Focus)
		{
			Result.FocusHit = FHitResult(Focus, Focus->FindComponentByClass<UPrimitiveComponent>(),
										 Focus->GetActorLocation(), FVector::UpVector);
		}
		return Result;
	}

	inline ALitterActor* Litter(UWorld* World, const FVector& Location, ALitterSpawnZoneActor* Zone = nullptr,
								int32 Seed = 17)
	{
		auto* Result =
			World->SpawnActorDeferred<ALitterActor>(ALitterActor::StaticClass(), FTransform(Location), nullptr, nullptr,
													ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Result)
		{
			return nullptr;
		}
		auto* Property = FindFProperty<FArrayProperty>(Result->GetClass(), TEXT("MeshVariants"));
		auto* Variants = Property->ContainerPtrToValuePtr<TArray<TObjectPtr<UStaticMesh>>>(Result);
		Variants->Add(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Variants->Add(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
		Variants->Add(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
		Result->SetSpawnZone(Zone);
		Result->ConfigureVisualVariationSeed(Seed);
		Result->FinishSpawning(FTransform(Location));
		BeginActorPlayIfNeeded(Result);
		return Result;
	}

	inline void BeginCleaning(UWorld* World)
	{
		for (TActorIterator<ALitterActor> I(World); I; ++I)
		{
			BeginActorPlayIfNeeded(*I);
		}
		for (TActorIterator<AWaterStainActor> I(World); I; ++I)
		{
			BeginActorPlayIfNeeded(*I);
		}
	}

	inline void ConfigureDirector(ACleaningDirectorActor* Director)
	{
		Set<FClassProperty>(Director, TEXT("LitterClass"), ALitterActor::StaticClass());
		Set<FClassProperty>(Director, TEXT("StainClass"), AWaterStainActor::StaticClass());
		Set<FFloatProperty>(Director, TEXT("DefaultStainSpacing"), 0.f);
		Set<FFloatProperty>(Director, TEXT("DefaultLitterSpacing"), 0.f);
		Director->SetSpawnRandomSeedForTesting(103);
	}

	inline ALitterTongsActor* Tongs(UWorld* World)
	{
		auto* Result = World->SpawnActor<ALitterTongsActor>();
		Mesh(Result);
		Set<FClassProperty>(Result, TEXT("TiedBagClass"), ATrashBagActor::StaticClass());
		BeginActorPlayIfNeeded(Result);
		return Result;
	}

	/** Tie drop request exactly as the tongs build it: camera from the use context, tuning from the CDO values. */
	inline FTrashBagDropRequest TieRequest(UWorld* World, FPlayer& Player)
	{
		auto* Equipment = Tongs(World);
		FTrashBagDropRequest Request;
		Equipment->BuildTieDropRequest(Context(Player, Equipment), Request);
		Equipment->Destroy();
		return Request;
	}

	/** Tie request with the view-front stage skipped (invalid pull step) so only the floor-front stage runs. */
	inline FTrashBagDropRequest TieFloorOnlyRequest(UWorld* World, FPlayer& Player)
	{
		FTrashBagDropRequest Request = TieRequest(World, Player);
		Request.ViewPullStepCm = 0.0f;
		return Request;
	}
} // namespace CleaningLitterTest
