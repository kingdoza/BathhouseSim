#if !UE_BUILD_SHIPPING

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Service/DrinkFridgeActor.h"
#include "Service/ServiceItemDefinition.h"

namespace
{
struct FFridgeDebugTarget
{
	ADrinkFridgeActor* Fridge = nullptr;
	AActor* User = nullptr;
};

bool ResolveAimedFridge(UWorld* World, FFridgeDebugTarget& OutTarget)
{
	APlayerController* Controller = GEngine ? GEngine->GetFirstLocalPlayerController(World) : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	UPlayerInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UPlayerInteractionComponent>() : nullptr;
	if (!Interaction)
	{
		UE_LOG(LogTemp, Warning, TEXT("DrinkFridge debug: no local player interaction component."));
		return false;
	}
	AActor* HitOwner = nullptr;
	FPlayerInteractionContext Context;
	IPlayerInteractable* Interactable = nullptr;
	UObject* TargetObject = nullptr;
	if (Interaction->ResolveFocusedInteraction(Context, Interactable, TargetObject))
	{
		HitOwner = Context.HitActor;
	}
	else
	{
		FHitResult Hit;
		if (Interaction->GetCurrentFocusHit(Hit))
		{
			HitOwner = Hit.GetActor();
		}
	}
	OutTarget.Fridge = Cast<ADrinkFridgeActor>(HitOwner);
	OutTarget.User = Pawn;
	if (!OutTarget.Fridge)
	{
		UE_LOG(LogTemp, Warning, TEXT("DrinkFridge debug: the aimed target is not a drink fridge."));
		return false;
	}
	return true;
}

UBathhouseFacilitySlotComponent* FindSlot(const ADrinkFridgeActor& Fridge)
{
	const TArray<TObjectPtr<UBathhouseFacilitySlotComponent>>& Slots = Fridge.GetFacilitySlots();
	return Slots.Num() == 1 ? Slots[0].Get() : nullptr;
}

FAutoConsoleCommandWithWorldAndArgs GDrinkFridgeReserve(
	TEXT("bathhouse.Debug.DrinkFridge.Reserve"),
	TEXT("Reserves and begins use of the aimed drink fridge's slot with the local player pawn."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		FFridgeDebugTarget Target;
		if (!ResolveAimedFridge(World, Target))
		{
			return;
		}
		UBathhouseFacilitySlotComponent* Slot = FindSlot(*Target.Fridge);
		const bool bReserved = Slot && Target.Fridge->IsAvailableForReservation()
			&& Slot->TryReserve(Target.User) && Slot->BeginUse(Target.User);
		UE_LOG(LogTemp, Log, TEXT("DrinkFridge debug Reserve on %s: %s"),
			*Target.Fridge->GetName(), bReserved ? TEXT("reserved and in use") : TEXT("failed"));
	}));

FAutoConsoleCommandWithWorldAndArgs GDrinkFridgeTake(
	TEXT("bathhouse.Debug.DrinkFridge.Take"),
	TEXT("Takes one drink from the aimed drink fridge as its current slot user."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		FFridgeDebugTarget Target;
		if (!ResolveAimedFridge(World, Target))
		{
			return;
		}
		UServiceItemDefinition* Kind = nullptr;
		FText Failure;
		if (Target.Fridge->TryTakeDrinkForCustomer(*Target.User, Kind, Failure))
		{
			UE_LOG(LogTemp, Log, TEXT("DrinkFridge debug Take on %s: took %s (stock %d)"),
				*Target.Fridge->GetName(), *Kind->DisplayName.ToString(), Target.Fridge->GetTotalStock());
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("DrinkFridge debug Take on %s failed: %s"),
				*Target.Fridge->GetName(), *Failure.ToString());
		}
	}));

FAutoConsoleCommandWithWorldAndArgs GDrinkFridgeRelease(
	TEXT("bathhouse.Debug.DrinkFridge.Release"),
	TEXT("Releases the aimed drink fridge's slot from the local player pawn."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		FFridgeDebugTarget Target;
		if (!ResolveAimedFridge(World, Target))
		{
			return;
		}
		UBathhouseFacilitySlotComponent* Slot = FindSlot(*Target.Fridge);
		const bool bReleased = Slot && Slot->Release(Target.User);
		UE_LOG(LogTemp, Log, TEXT("DrinkFridge debug Release on %s: %s"),
			*Target.Fridge->GetName(), bReleased ? TEXT("released") : TEXT("failed"));
	}));
}

#endif
