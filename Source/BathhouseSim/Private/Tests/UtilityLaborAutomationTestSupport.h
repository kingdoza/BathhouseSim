#pragma once
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Facility/BathWaterConditionComponent.h"
#include "Facility/BathWaterOperationsSubsystem.h"
#include "Facility/BathWaterUtilityPlacementInstanceData.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"
#include "Facility/BathhouseBathFacilityActor.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PhysicalCarryFixedSlotActor.h"
#include "Interaction/PlayerEquipmentUseComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Interaction/PlayerInteractable.h"
#include "Placement/FacilityActorConversionTransaction.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Tests/FacilityPlacementAutomationTestProbe.h"
#include "Tests/UtilityLaborAutomationTestProbe.h"
#include "Utility/BathWaterBoilerFacilityActor.h"
#include "Utility/BathWaterCoolerFacilityActor.h"
#include "Utility/BathWaterCirculatorFacilityActor.h"
#include "Utility/UtilityLeverOperatingVolumeComponent.h"
#include "Utility/UtilityFuelIntakeVolumeComponent.h"
#include "Utility/UtilityFuelSupplyActor.h"
#include "Utility/UtilityFuelTransaction.h"
#include "Utility/UtilityGaugeComponent.h"
#include "Utility/UtilityOperationComponent.h"
#include "Utility/UtilityShovelActor.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UnrealType.h"

#include <limits>

namespace
{
void TickWorldForDuration(UWorld* World, const double DurationSeconds, const float MaxStepSeconds = 0.25f)
{
	if (!World || !FMath::IsFinite(DurationSeconds) || DurationSeconds <= 0.0
		|| !FMath::IsFinite(MaxStepSeconds) || MaxStepSeconds <= 0.0f)
	{
		return;
	}
	double RemainingSeconds = DurationSeconds;
	while (RemainingSeconds > UE_DOUBLE_SMALL_NUMBER)
	{
		const float StepSeconds = static_cast<float>(FMath::Min(RemainingSeconds, static_cast<double>(MaxStepSeconds)));
		++GFrameCounter;
		World->Tick(LEVELTICK_All, StepSeconds);
		RemainingSeconds -= StepSeconds;
	}
}

void BeginActorPlayIfNeeded(AActor* Actor)
{
	if (Actor && !Actor->HasActorBegunPlay())
	{
		Actor->DispatchBeginPlay();
	}
}

FPlayerInteractionResult ExecuteFocusedHeldTargetUse(
	UPlayerInteractionComponent* Interaction,
	const EPlayerHeldTargetUseDirection Direction)
{
	const EPlayerInteractionIntent Intent = Direction == EPlayerHeldTargetUseDirection::Apply
		? EPlayerInteractionIntent::HeldApply
		: EPlayerInteractionIntent::HeldTake;
	if (!Interaction)
	{
		return FPlayerInteractionResult::Failed(FText::GetEmpty(), Intent);
	}

	FPlayerInteractionContext Context;
	IPlayerInteractable* Interactable = nullptr;
	UObject* TargetObject = nullptr;
	if (!Interaction->ResolveFocusedInteraction(Context, Interactable, TargetObject)
		|| !Interactable || !IsValid(TargetObject))
	{
		return FPlayerInteractionResult::Failed(FText::GetEmpty(), Intent);
	}

	const FPlayerInteractionQuery Query = Interactable->QueryInteraction(Context);
	const bool bVisible = Direction == EPlayerHeldTargetUseDirection::Apply
		? Query.bHeldApplyVisible : Query.bHeldTakeVisible;
	const bool bCanUse = Direction == EPlayerHeldTargetUseDirection::Apply
		? Query.bCanHeldApply : Query.bCanHeldTake;
	const FText& FailureReason = Direction == EPlayerHeldTargetUseDirection::Apply
		? Query.HeldApplyFailureReason : Query.HeldTakeFailureReason;
	if (!bVisible && FailureReason.IsEmpty())
	{
		return FPlayerInteractionResult::Failed(FText::GetEmpty(), Intent);
	}
	if (!bCanUse)
	{
		return Interaction->ReportExternalInteractionAttempt(
			FPlayerInteractionResult::Failed(FailureReason, Intent));
	}

	FPlayerInteractionResult Result = Interactable->ExecuteHeldTargetUse(Context, Direction);
	Result.Intent = Intent;
	Interaction->RefreshInteractionQuery();
	return Interaction->ReportExternalInteractionAttempt(Result);
}

class FScopedUtilityLaborWorld
{
public:
	explicit FScopedUtilityLaborWorld(const TCHAR* BaseName)
	{
		if (!GEngine)
		{
			return;
		}
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), BaseName);
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World)
		{
			GEngine->DestroyWorldContext(World);
			return;
		}
		World->AddToRoot();
		Context.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}

	~FScopedUtilityLaborWorld()
	{
		if (World && GEngine)
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}

	UWorld* Get() const { return World; }

private:
	UWorld* World = nullptr;
};

UStaticMeshComponent* FindNamedMeshComponent(AActor* Actor, const FName ComponentName)
{
	if (!Actor)
	{
		return nullptr;
	}
	TArray<UStaticMeshComponent*> Components;
	Actor->GetComponents(Components);
	for (UStaticMeshComponent* Component : Components)
	{
		if (Component && Component->GetFName() == ComponentName)
		{
			return Component;
		}
	}
	return nullptr;
}

bool SetBoilerTestMeshes(ABathWaterBoilerFacilityActor* Boiler, UStaticMesh* Mesh)
{
	if (!Boiler || !Mesh)
	{
		return false;
	}
	UStaticMeshComponent* GaugeNeedle = Boiler->GetGaugeNeedleMesh();
	UStaticMeshComponent* FuelDoorMesh = Boiler->GetFuelDoorMesh();
	UUtilityFuelIntakeVolumeComponent* FuelIntakeVolume = Boiler->GetFuelIntakeVolume();
	if (!GaugeNeedle || !FuelDoorMesh || !FuelIntakeVolume)
	{
		return false;
	}
	FuelIntakeVolume->SetBoxExtent(FVector(15.0f));
	FuelIntakeVolume->SetRelativeScale3D(FVector::OneVector);
	GaugeNeedle->SetStaticMesh(Mesh);
	GaugeNeedle->UpdateBounds();
	FuelDoorMesh->SetStaticMesh(Mesh);
	FuelDoorMesh->UpdateBounds();
	return true;
}

bool SetCoolerTestMeshes(ABathWaterCoolerFacilityActor* Cooler, UStaticMesh* Mesh)
{
	if (!Cooler || !Mesh || !Cooler->GetFuelIntakeVolume() || !Cooler->GetFuelDoorMesh()
		|| !Cooler->GetGaugeNeedleMesh())
	{
		return false;
	}
	Cooler->GetFuelIntakeVolume()->SetBoxExtent(FVector(15.0f));
	Cooler->GetFuelIntakeVolume()->SetRelativeScale3D(FVector::OneVector);
	Cooler->GetFuelDoorMesh()->SetStaticMesh(Mesh);
	Cooler->GetGaugeNeedleMesh()->SetStaticMesh(Mesh);
	Cooler->GetFuelDoorMesh()->UpdateBounds();
	Cooler->GetGaugeNeedleMesh()->UpdateBounds();
	return true;
}

bool SetCirculatorTestMeshes(ABathWaterCirculatorFacilityActor* Circulator, UStaticMesh* Mesh)
{
	if (!Circulator || !Mesh || !Circulator->GetLeverOperatingVolume() || !Circulator->GetLeverMesh())
	{
		return false;
	}
	Circulator->GetLeverOperatingVolume()->SetBoxExtent(FVector(15.0f));
	Circulator->GetLeverOperatingVolume()->SetRelativeScale3D(FVector::OneVector);
	Circulator->GetLeverMesh()->SetStaticMesh(Mesh);
	Circulator->GetLeverMesh()->UpdateBounds();
	Circulator->GetGaugeNeedleMesh()->SetStaticMesh(Mesh);
	Circulator->GetGaugeNeedleMesh()->UpdateBounds();
	return true;
}

bool SetShovelTestMeshes(AUtilityShovelActor* Shovel, UStaticMesh* Mesh)
{
	if (!Shovel || !Mesh)
	{
		return false;
	}
	UStaticMeshComponent* WorldMesh = FindNamedMeshComponent(Shovel, TEXT("WorldMesh"));
	UStaticMeshComponent* LoadVisual = FindNamedMeshComponent(Shovel, TEXT("LoadVisual"));
	UMaterialInterface* DryIceMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	if (!WorldMesh || !LoadVisual || !DryIceMaterial)
	{
		return false;
	}
	WorldMesh->SetStaticMesh(Mesh);
	WorldMesh->UpdateBounds();
	LoadVisual->SetStaticMesh(Mesh);
	LoadVisual->UpdateBounds();
	FUtilityShovelLoadAppearance CoalAppearance;
	CoalAppearance.Mesh = Mesh;
	FUtilityShovelLoadAppearance DryIceAppearance;
	DryIceAppearance.Material = DryIceMaterial;
	Shovel->SetLoadAppearance(EUtilityFuelKind::Coal, CoalAppearance);
	Shovel->SetLoadAppearance(EUtilityFuelKind::DryIce, DryIceAppearance);
	return true;
}

}
