#if !UE_BUILD_SHIPPING
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Facility/BathhouseFacilitySlotComponent.h"

namespace
{
	void UseAimedFacility(UWorld* World, bool bBegin)
	{
		auto* Controller = GEngine ? GEngine->GetFirstLocalPlayerController(World) : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		auto* Interaction = Pawn ? Pawn->FindComponentByClass<UPlayerInteractionComponent>() : nullptr;
		FHitResult Hit;
		auto* Facility = Interaction && Interaction->GetCurrentFocusHit(Hit)
							 ? Cast<ABathhouseFacilityActor>(Hit.GetActor())
							 : nullptr;
		if (!Facility || !Pawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("Facility debug: aim at a placed facility."));
			return;
		}
		bool bSuccess = false;
		for (UBathhouseFacilitySlotComponent* Slot : Facility->GetFacilitySlots())
		{
			if (bBegin && Facility->IsAvailableForReservation() && Slot->IsAvailable())
			{
				bSuccess = Slot->TryReserve(Pawn) && Slot->BeginUse(Pawn);
				if (!bSuccess)
				{
					Slot->Release(Pawn);
				}
				break;
			}
			if (!bBegin && Slot->GetCurrentUser() == Pawn)
			{
				bSuccess = Slot->EndUse(Pawn) && Slot->Release(Pawn);
				break;
			}
		}
		UE_LOG(LogTemp, Log, TEXT("Facility debug %s on %s: %s"), bBegin ? TEXT("BeginUse") : TEXT("EndUse"),
			   *Facility->GetName(), bSuccess ? TEXT("success") : TEXT("failed"));
	}

	FAutoConsoleCommandWithWorldAndArgs GFacilityBeginUse(
		TEXT("bathhouse.Debug.Facility.BeginUse"),
		TEXT("Reserve and begin the first available slot of the aimed facility with the local Pawn."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& Args, UWorld* World)
			{
				UseAimedFacility(World, true);
			}));
	FAutoConsoleCommandWithWorldAndArgs GFacilityEndUse(
		TEXT("bathhouse.Debug.Facility.EndUse"), TEXT("End and release the local Pawn's slot in the aimed facility."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& Args, UWorld* World)
			{
				UseAimedFacility(World, false);
			}));
} // namespace
#endif
