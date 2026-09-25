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
#include "Placement/FacilityActorConversionTransaction.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Tests/FacilityPlacementAutomationTestProbe.h"
#include "Tests/UtilityLaborAutomationTestProbe.h"
#include "Utility/BathWaterBoilerFacilityActor.h"
#include "Utility/UtilityFuelIntakeComponent.h"
#include "Utility/UtilityFuelSupplyActor.h"
#include "Utility/UtilityFuelTransaction.h"
#include "Utility/UtilityGaugeComponent.h"
#include "Utility/UtilityOperationComponent.h"
#include "Utility/UtilityShovelActor.h"
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
	UStaticMeshComponent* GaugeFace = FindNamedMeshComponent(Boiler, TEXT("GaugeFace"));
	UStaticMeshComponent* GaugeNeedle = FindNamedMeshComponent(Boiler, TEXT("GaugeNeedleMesh"));
	if (!GaugeFace || !GaugeNeedle)
	{
		return false;
	}
	Boiler->GetFuelIntake()->SetStaticMesh(Mesh);
	Boiler->GetFuelIntake()->UpdateBounds();
	GaugeFace->SetStaticMesh(Mesh);
	GaugeFace->UpdateBounds();
	GaugeNeedle->SetStaticMesh(Mesh);
	GaugeNeedle->UpdateBounds();
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
	if (!WorldMesh || !LoadVisual)
	{
		return false;
	}
	WorldMesh->SetStaticMesh(Mesh);
	WorldMesh->UpdateBounds();
	LoadVisual->SetStaticMesh(Mesh);
	LoadVisual->UpdateBounds();
	return true;
}

}
