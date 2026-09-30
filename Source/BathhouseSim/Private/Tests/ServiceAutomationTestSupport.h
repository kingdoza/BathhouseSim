#pragma once

#include "Tests/UtilityLaborAutomationTestSupport.h"

#include "Components/SphereComponent.h"
#include "Economy/BathhousePlayerState.h"
#include "Economy/PlayerWalletComponent.h"
#include "Engine/LocalPlayer.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/PlayerHeldTargetUseComponent.h"
#include "Placement/FacilityPlacementSettings.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Service/DisplaySpaceComponent.h"
#include "Service/DrinkFridgePlacementInstanceData.h"
#include "Service/ItemBoxActor.h"
#include "Service/ServiceDisplaySettings.h"
#include "Service/ServiceItemDefinition.h"
#include "Service/ServiceItemTypes.h"
#include "Tests/FacilityPlacementAutomationTestProbe.h"
#include "Tests/ServiceAutomationTestProbe.h"

namespace ServiceTest
{
struct FPlayer
{
	APawn* Pawn = nullptr;
	APlayerController* Controller = nullptr;
	UCameraComponent* Camera = nullptr;
	UPlayerCarryComponent* Carry = nullptr;
	UPlayerInteractionComponent* Interaction = nullptr;
	UPlayerEquipmentUseComponent* EquipmentUse = nullptr;
	UPlayerHeldTargetUseComponent* HeldUse = nullptr;
	ABathhousePlayerState* PlayerState = nullptr;
};

inline UServiceItemDefinition* MakeKind(
	const TCHAR* Id,
	const TCHAR* Name,
	const int32 Capacity,
	const int32 SaleValue,
	const bool bFridgeCategory = true)
{
	UServiceItemDefinition* Kind = NewObject<UServiceItemDefinition>(
		GetTransientPackage(),
		MakeUniqueObjectName(GetTransientPackage(), UServiceItemDefinition::StaticClass(), Id));
	Kind->ItemId = Id;
	Kind->DisplayName = FText::FromString(Name);
	Kind->ItemMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	Kind->BoxCapacity = Capacity;
	Kind->SaleValue = SaleValue;
	for (int32 Index = 0; Index < Capacity; ++Index)
	{
		Kind->BoxSlotTransforms.Add(FTransform(FVector(Index * 5.0f, 0.0f, 0.0f)));
	}
	if (bFridgeCategory)
	{
		Kind->DisplayCategories.AddTag(TAG_Display_Fridge);
	}
	return Kind;
}

inline AItemBoxActor* SpawnBox(
	UWorld* World,
	UServiceItemDefinition* Kind,
	const int32 Count,
	const FVector& Location = FVector(0.0f, 0.0f, 500.0f))
{
	FText Failure;
	AItemBoxActor* Box = AItemBoxActor::SpawnFilledBox(
		*World,
		AItemBoxActor::StaticClass(),
		Kind,
		Count,
		FTransform(FRotator::ZeroRotator, Location),
		Failure);
	if (Box)
	{
		BeginActorPlayIfNeeded(Box);
		Box->ActivateFreeWorld(FTransform(FRotator::ZeroRotator, Location), Failure);
	}
	return Box;
}

inline bool BuildPlayer(FAutomationTestBase& Test, UWorld* World, FPlayer& Out, const bool bWithPlayerState = false)
{
	APawn* User = World->SpawnActor<APawn>();
	UCameraComponent* Camera = User ? NewObject<UCameraComponent>(User, TEXT("ServiceCamera")) : nullptr;
	UPlayerCarryComponent* Carry = User ? NewObject<UPlayerCarryComponent>(User, TEXT("ServiceCarry")) : nullptr;
	UPlayerInteractionComponent* Interaction = User
		? NewObject<UPlayerInteractionComponent>(User, TEXT("ServiceInteraction")) : nullptr;
	UPlayerEquipmentUseComponent* EquipmentUse = User
		? NewObject<UPlayerEquipmentUseComponent>(User, TEXT("ServiceEquipment")) : nullptr;
	UPlayerHeldTargetUseComponent* HeldUse = User
		? NewObject<UPlayerHeldTargetUseComponent>(User, TEXT("ServiceHeldUse")) : nullptr;
	if (!User || !Camera || !Carry || !Interaction || !EquipmentUse || !HeldUse || !GEngine)
	{
		Test.AddError(TEXT("Failed to create the service automation player components."));
		return false;
	}
	User->SetRootComponent(Camera);
	User->AddInstanceComponent(Camera);
	User->AddInstanceComponent(Carry);
	User->AddInstanceComponent(Interaction);
	User->AddInstanceComponent(EquipmentUse);
	User->AddInstanceComponent(HeldUse);
	Camera->RegisterComponent();
	Carry->RegisterComponent();
	Interaction->RegisterComponent();
	EquipmentUse->RegisterComponent();
	HeldUse->RegisterComponent();
	Camera->SetWorldLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	Carry->ConfigureHeldAnchor(Camera);
	Carry->ConfigureEquipmentUse(EquipmentUse);
	Interaction->Configure(Camera, Carry);
	Interaction->ConfigureEquipmentUse(EquipmentUse);
	EquipmentUse->Configure(Camera, Carry, Interaction, nullptr);
	HeldUse->Configure(Interaction, Carry, EquipmentUse);
	BeginActorPlayIfNeeded(User);

	APlayerController* Controller = World->SpawnActor<APlayerController>();
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	if (!Controller || !LocalPlayer)
	{
		Test.AddError(TEXT("Failed to create the service automation controller."));
		return false;
	}
	BeginActorPlayIfNeeded(Controller);
	Controller->SetPlayer(LocalPlayer);
	Controller->Possess(User);
	if (bWithPlayerState)
	{
		Out.PlayerState = World->SpawnActor<ABathhousePlayerState>(
			ABathhousePlayerState::StaticClass(), FTransform::Identity);
		if (!Out.PlayerState)
		{
			Test.AddError(TEXT("Failed to spawn the service automation PlayerState."));
			return false;
		}
		User->SetPlayerState(Out.PlayerState);
	}
	Out.Pawn = User;
	Out.Controller = Controller;
	Out.Camera = Camera;
	Out.Carry = Carry;
	Out.Interaction = Interaction;
	Out.EquipmentUse = EquipmentUse;
	Out.HeldUse = HeldUse;
	return User->IsLocallyControlled();
}

inline UFacilityPlacementDefinition* MakeFridgeDefinition()
{
	UFacilityPlacementDefinition* Source = LoadObject<UFacilityPlacementDefinition>(
		nullptr,
		TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Shower.DA_FacilityPlacement_Shower"));
	if (!Source)
	{
		return nullptr;
	}
	UFacilityPlacementDefinition* Definition = DuplicateObject<UFacilityPlacementDefinition>(
		Source,
		GetTransientPackage(),
		MakeUniqueObjectName(GetTransientPackage(), UFacilityPlacementDefinition::StaticClass(), TEXT("ServiceFridgeDefinition")));
	if (!Definition)
	{
		return nullptr;
	}
	Definition->StableId = MakeUniqueObjectName(
		GetTransientPackage(), UFacilityPlacementDefinition::StaticClass(), TEXT("ServiceFridgeId"));
	Definition->PlacedFacilityClass = AServiceAutomationFridge::StaticClass();
	Definition->RecoveryItemClass = AFacilityPlacementItemAutomationActor::StaticClass();
	Definition->LockerSlotCount = 0;
	Definition->FacilityTags.AddTag(TAG_Facility_Discardable);
	return Definition;
}

/** Installs a fridge through the staged-placement path so it is a placed, domain-active facility. */
inline AServiceAutomationFridge* SpawnInstalledFridge(
	FAutomationTestBase& Test,
	UWorld* World,
	UFacilityPlacementDefinition& Definition,
	const FVector& Location,
	const FFacilityPlacementPayload* ImportPayload = nullptr,
	const APlaceableFacilityItemActor* PayloadOwner = nullptr)
{
	FText Failure;
	AServiceAutomationFridge* Fridge = World->SpawnActorDeferred<AServiceAutomationFridge>(
		AServiceAutomationFridge::StaticClass(),
		FTransform(FRotator::ZeroRotator, Location),
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Fridge)
	{
		Test.AddError(TEXT("Failed to defer-spawn the automation fridge."));
		return nullptr;
	}
	APlaceableFacilityItemActor* OwnerItem = nullptr;
	if (!PayloadOwner)
	{
		OwnerItem = World->SpawnActor<AFacilityPlacementItemAutomationActor>();
		PayloadOwner = OwnerItem;
	}
	FFacilityPlacementPayload Fresh;
	Fresh.Definition = &Definition;
	if (!Fridge->GetPlacementForTest()->PrepareForStagedPlacement(Definition, Failure)
		|| !PayloadOwner
		|| !Fridge->ImportPlacementPayload(*PayloadOwner, ImportPayload ? *ImportPayload : Fresh, Failure))
	{
		Test.AddError(FString::Printf(TEXT("Fridge staging/import failed: %s"), *Failure.ToString()));
		Fridge->Destroy();
		return nullptr;
	}
	Fridge->FinishSpawning(FTransform(FRotator::ZeroRotator, Location));
	BeginActorPlayIfNeeded(Fridge);
	if (!Fridge->FinalizePlacementPayloadAfterConstruction(Failure)
		|| !Fridge->GetPlacementForTest()->FinalizeStagedPlacementCollisionSnapshot(Failure)
		|| !Fridge->StagePlacedDomainRegistration(Failure)
		|| !Fridge->GetPlacementForTest()->CommitStagedPlacement(Failure))
	{
		Test.AddError(FString::Printf(TEXT("Fridge domain installation failed: %s"), *Failure.ToString()));
		Fridge->RollbackPlacedDomainRegistration();
		Fridge->Destroy();
		return nullptr;
	}
	Fridge->PublishPlacedDomainRegistration();
	return Fridge;
}

/** Placement definitions validate against the global grid, so pin it to the fixture grid for the test. */
struct FScopedServiceGrid
{
	UFacilityPlacementSettings* Settings = GetMutableDefault<UFacilityPlacementSettings>();
	float SavedGridSizeCm = Settings ? Settings->GridSizeCm : 10.0f;

	FScopedServiceGrid()
	{
		if (Settings)
		{
			Settings->GridSizeCm = 10.0f;
		}
	}

	~FScopedServiceGrid()
	{
		if (Settings)
		{
			Settings->GridSizeCm = SavedGridSizeCm;
		}
	}
};

/** Takes a held box out of the hand through the carry commit path and parks it away from the aim ray. */
inline void StowBox(FPlayer& Player, AItemBoxActor* Box)
{
	Player.Carry->CommitReleasePhysicalObject(Box);
	Box->NotifyPhysicalDropCommitted(*Player.Carry);
	Box->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Box->SetActorLocation(FVector(0.0f, 3000.0f, 500.0f));
}

struct FScopedDisplaySettings
{
	UServiceDisplaySettings* Settings = GetMutableDefault<UServiceDisplaySettings>();
	bool bShowTakeHighlight = Settings->bShowTakeHighlight;
	int32 StencilValue = Settings->TakeHighlightStencilValue;
	TSoftObjectPtr<UMaterialInterface> PreviewMaterial = Settings->InsertPreviewMaterial;

	~FScopedDisplaySettings()
	{
		Settings->bShowTakeHighlight = bShowTakeHighlight;
		Settings->TakeHighlightStencilValue = StencilValue;
		Settings->InsertPreviewMaterial = PreviewMaterial;
	}
};
}
