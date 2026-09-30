#pragma once
#include "Tests/ServiceAutomationTestSupport.h"
#include "Tests/ServiceFacilityAutomationTestProbe.h"
#include "Service/ServiceDisplayManagerComponent.h"

namespace ServiceFacilityTest
{
	using namespace ServiceTest;

	struct FFixture
	{
		FScopedServiceGrid Grid;
		UWorld* World;
		FPlayer Player;
		TArray<TObjectPtr<UServiceItemDefinition>> Kinds;
		UFacilityPlacementDefinition* Definition = nullptr;
		AFacilityPlacementZoneAutomationActor* Zone = nullptr;
		AServiceAutomationDisplayFacility* Facility = nullptr;
		FTransform Placement = FTransform(FVector(12000, 0, 100));
		TArray<TObjectPtr<UServiceItemDefinition>> SavedKinds;
		AServiceAutomationDisplayFacility* CDO = nullptr;

		explicit FFixture(UWorld* InWorld) : World(InWorld)
		{
		}

		~FFixture()
		{
			if (CDO)
			{
				CDO->FixtureKinds = SavedKinds;
			}
		}

		bool Install(FAutomationTestBase& Test, bool bShower = false)
		{
			if (!World || !BuildPlayer(Test, World, Player, true))
			{
				return false;
			}
			const TCHAR* Ids[] = {TEXT("Dryer"), TEXT("Lotion"), TEXT("Swab"), TEXT("Comb")};
			const TCHAR* Names[] = {TEXT("드라이기"), TEXT("스킨로션"), TEXT("면봉"), TEXT("빗")};
			const int32 Caps[] = {2, 6, 6, 12};
			for (int32 Index = 0; Index < (bShower ? 2 : 4); ++Index)
			{
				auto* Kind = MakeKind(bShower ? (Index == 0 ? TEXT("Shampoo") : TEXT("BodyWash")) : Ids[Index],
									  bShower ? (Index == 0 ? TEXT("샴푸") : TEXT("바디워시")) : Names[Index],
									  bShower ? 6 : Caps[Index], 0, false);
				Kind->ConsumableUses = bShower ? 30 : Index == 1 ? 10 : Index == 2 ? 30 : 0;
				Kinds.Add(Kind);
			}
			auto Class =
				bShower ? AServiceAutomationShower::StaticClass() : AServiceAutomationDisplayFacility::StaticClass();
			CDO = CastChecked<AServiceAutomationDisplayFacility>(Class->GetDefaultObject());
			SavedKinds = CDO->FixtureKinds;
			CDO->FixtureKinds = Kinds;
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
				return false;
			}
			Facility = Cast<AServiceAutomationDisplayFacility>(FFacilityActorConversionTransaction::PlaceItemAsFacility(
				*Item, Placement, *Zone, *Player.Carry, Failure));
			if (!Facility)
			{
				Test.AddError(Failure.ToString());
			}
			BeginActorPlayIfNeeded(Facility);
			return Facility != nullptr;
		}

		FPlayerInteractionContext Context(int32 Index = 0)
		{
			FPlayerInteractionContext Context;
			Context.Interactor = Player.Pawn;
			Context.CarryComponent = Player.Carry;
			Context.InteractionComponent = Player.Interaction;
			auto Center = Facility->GetSpaces()[Index]->GetSlotsWorldCenter();
			Context.HitResult.TraceStart = Center - FVector(200, 0, 0);
			Context.HitResult.TraceEnd = Center + FVector(200, 0, 0);
			return Context;
		}

		void AimAt(int32 Index)
		{
			auto Center = Facility->GetSpaces()[Index]->GetSlotsWorldCenter();
			Player.Camera->SetWorldLocationAndRotation(Center - FVector(180, 0, 0), FRotator::ZeroRotator);
			Player.Interaction->RefreshInteractionQuery();
		}
	};
} // namespace ServiceFacilityTest
