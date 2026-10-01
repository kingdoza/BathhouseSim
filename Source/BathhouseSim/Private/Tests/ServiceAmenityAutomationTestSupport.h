#pragma once
#include "Tests/ServiceAutomationTestSupport.h"
#include "Tests/ServiceAmenityAutomationTestProbe.h"
#include "Service/PlayerScrubFocusComponent.h"
#include "Service/ServiceTestUserActor.h"
#include "Service/ScrubTowelActor.h"
#include "Character/FirstPersonCharacter.h"
#include "Character/FirstPersonMovementComponent.h"
#include "Components/CapsuleComponent.h"

namespace ServiceAmenityTest
{
	using namespace ServiceTest;

	struct FFixture
	{
		FScopedServiceGrid Grid;
		UWorld* World;
		FPlayer Player;
		AFacilityPlacementZoneAutomationActor* Zone = nullptr;
		UFacilityPlacementDefinition* Definition = nullptr;
		ABathhouseFacilityActor* Facility = nullptr;
		FTransform Transform = FTransform(FVector(12000, 0, 0));

		explicit FFixture(UWorld* W) : World(W)
		{
		}

		bool Install(FAutomationTestBase& Test, UClass* Class)
		{
			if (!World || !BuildPlayer(Test, World, Player, true))
			{
				return false;
			}
			Definition = MakeFridgeDefinition();
			if (!Definition)
			{
				return false;
			}
			Definition->PlacedFacilityClass = Class;
			Zone = World->SpawnActor<AFacilityPlacementZoneAutomationActor>(
				AFacilityPlacementZoneAutomationActor::StaticClass(), FTransform(FVector(12000, 0, 0)));
			for (auto Tag : Definition->FacilityTags)
			{
				Zone->AddAllowedTag(Tag);
			}
			Zone->GetZoneBounds()->SetBoxExtent(FVector(500));
			FText Failure;
			auto* Item = APlaceableFacilityItemActor::SpawnFreshItem(*World, *Definition,
																	 FTransform(FVector(11000, 0, 300)), Failure);
			if (!Item || !Item->ActivateFreeWorld(Item->GetActorTransform(), Failure) ||
				!Player.Carry->TryTakePhysicalObject(Item, Failure))
			{
				Test.AddError(Failure.ToString());
				return false;
			}
			Facility = Cast<ABathhouseFacilityActor>(FFacilityActorConversionTransaction::PlaceItemAsFacility(
				*Item, Transform, *Zone, *Player.Carry, Failure));
			if (!Facility)
			{
				Test.AddError(Failure.ToString());
			}
			BeginActorPlayIfNeeded(Facility);
			return Facility != nullptr;
		}

		FPlayerInteractionContext Context() const
		{
			FPlayerInteractionContext C;
			C.Interactor = Player.Pawn;
			C.CarryComponent = Player.Carry;
			C.InteractionComponent = Player.Interaction;
			return C;
		}

		AServiceTestUserActor* Occupy(int32 Index = 0)
		{
			if (!Facility || !Facility->GetFacilitySlots().IsValidIndex(Index))
			{
				return nullptr;
			}
			auto* User = World->SpawnActor<AServiceTestUserActor>();
			if (!User)
			{
				return nullptr;
			}
			BeginActorPlayIfNeeded(User);
			auto* Slot = Facility->GetFacilitySlots()[Index].Get();
			if (!Slot->TryReserve(User) || !Slot->BeginUse(User))
			{
				User->Destroy();
				return nullptr;
			}
			return User;
		}

		AFirstPersonCharacter* Character()
		{
			auto* Pawn = World->SpawnActor<AFirstPersonCharacter>();
			BeginActorPlayIfNeeded(Pawn);
			Player.Controller->Possess(Pawn);
			Player.Controller->SetViewTarget(Pawn);
			Pawn->SetPlayerState(Player.PlayerState);
			return Pawn;
		}
	};

	inline AScrubTowelActor* HoldTowel(UWorld& World, UPlayerCarryComponent& Carry)
	{
		auto* Towel = World.SpawnActor<AScrubTowelActor>();
		BeginActorPlayIfNeeded(Towel);
		auto* Mesh = Towel->FindComponentByClass<UStaticMeshComponent>();
		Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Mesh->SetWorldScale3D(FVector(.1));
		FText Failure;
		return Carry.TryTakePhysicalObject(Towel, Failure) ? Towel : nullptr;
	}

	inline USphereComponent* AddAimShape(AActor& Actor)
	{
		auto* Shape = NewObject<USphereComponent>(&Actor);
		Shape->SetupAttachment(Actor.GetRootComponent());
		Shape->SetSphereRadius(28);
		Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Shape->SetCollisionResponseToAllChannels(ECR_Ignore);
		Shape->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Shape->SetCanEverAffectNavigation(false);
		Actor.AddInstanceComponent(Shape);
		Shape->RegisterComponent();
		return Shape;
	}
} // namespace ServiceAmenityTest
