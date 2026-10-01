#if !UE_BUILD_SHIPPING
#include "Service/ServiceTestUserActor.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	enum class EDebugAction
	{
		Spawn,
		Knockdown,
		StandUp,
		Remove
	};

	void DebugService(UWorld* World, EDebugAction Action)
	{
		auto* Controller = GEngine ? GEngine->GetFirstLocalPlayerController(World) : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		auto* Interaction = Pawn ? Pawn->FindComponentByClass<UPlayerInteractionComponent>() : nullptr;
		FHitResult Hit;
		auto* Facility = Interaction && Interaction->GetCurrentFocusHit(Hit)
							 ? Cast<ABathhouseFacilityActor>(Hit.GetActor())
							 : nullptr;
		bool Success = false;
		if (Facility && World)
		{
			for (UBathhouseFacilitySlotComponent* Slot : Facility->GetFacilitySlots())
			{
				if (Action == EDebugAction::Spawn && Facility->IsAvailableForReservation() && Slot->IsAvailable())
				{
					// Use the authored dummy when available; native diagnostics remain usable before Editor authoring.
					UClass* UserClass = LoadClass<AServiceTestUserActor>(
						nullptr, TEXT("/Game/Bathhouse/Blueprints/Service/BP_ServiceTestUser.BP_ServiceTestUser_C"),
						nullptr, LOAD_NoWarn);
					if (!UserClass)
					{
						UserClass = AServiceTestUserActor::StaticClass();
					}
					// Deferred identity is available to reserve before construction finishes at the action point.
					auto* User = World->SpawnActorDeferred<AServiceTestUserActor>(
						UserClass, Slot->GetActionTransform(), nullptr, nullptr,
						ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					if (User && Slot->TryReserve(User))
					{
						User->FinishSpawning(Slot->GetActionTransform());
						if (UserClass == AServiceTestUserActor::StaticClass())
						{
							if (auto* Mesh = User->FindComponentByClass<UStaticMeshComponent>())
							{
								Mesh->SetStaticMesh(
									LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
								Mesh->SetRelativeScale3D(FVector(.3, .3, .6));
							}
						}
						Success = IsValid(User) && Slot->BeginUse(User);
					}
					if (!Success && User)
					{
						Slot->Release(User);
						User->Destroy();
					}
					break;
				}
				auto* User = Cast<AServiceTestUserActor>(Slot->GetCurrentUser());
				if (!User)
				{
					continue;
				}
				if (Action == EDebugAction::Knockdown && Slot->GetSlotState() == EBathhouseFacilitySlotState::Occupied)
				{
					Success = Slot->EndUse(User);
				}
				else if (Action == EDebugAction::StandUp &&
						 Slot->GetSlotState() == EBathhouseFacilitySlotState::Reserved &&
						 Facility->IsAvailableForReservation())
				{
					Success = Slot->BeginUse(User);
				}
				else if (Action == EDebugAction::Remove)
				{
					Success = User->Destroy();
				}
				if (Success)
				{
					break;
				}
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("Service debug %d: %s"), int32(Action),
			   Success ? TEXT("success") : TEXT("failed (aim at an eligible placed facility)"));
	}

	FAutoConsoleCommandWithWorldAndArgs Spawn(TEXT("bathhouse.Debug.Service.SpawnTestUser"),
											  TEXT("Spawn a test user in an available aimed facility slot."),
											  FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
												  [](const TArray<FString>&, UWorld* W)
												  {
													  DebugService(W, EDebugAction::Spawn);
												  }));
	FAutoConsoleCommandWithWorldAndArgs Knockdown(TEXT("bathhouse.Debug.Service.KnockdownTestUser"),
												  TEXT("EndUse while retaining a test user's reservation."),
												  FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
													  [](const TArray<FString>&, UWorld* W)
													  {
														  DebugService(W, EDebugAction::Knockdown);
													  }));
	FAutoConsoleCommandWithWorldAndArgs StandUp(TEXT("bathhouse.Debug.Service.StandUpTestUser"),
												TEXT("Restart a reserved test user's use."),
												FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
													[](const TArray<FString>&, UWorld* W)
													{
														DebugService(W, EDebugAction::StandUp);
													}));
	FAutoConsoleCommandWithWorldAndArgs Remove(TEXT("bathhouse.Debug.Service.RemoveTestUser"),
											   TEXT("Remove an aimed facility's test user."),
											   FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
												   [](const TArray<FString>&, UWorld* W)
												   {
													   DebugService(W, EDebugAction::Remove);
												   }));
} // namespace
#endif
