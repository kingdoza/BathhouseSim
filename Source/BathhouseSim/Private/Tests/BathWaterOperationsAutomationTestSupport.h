#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"
#include "GameFramework/Actor.h"
#include "Utility/UtilityOperationComponent.h"

namespace BathWaterOperationsTestSupport
{
class FScopedBathWaterOperationsWorld
{
public:
	explicit FScopedBathWaterOperationsWorld(const TCHAR* BaseName)
	{
		if (!GEngine) return;
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
	}
	~FScopedBathWaterOperationsWorld()
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

inline UBathWaterUtilityCapacityComponent* AddProvider(
	UWorld& World,
	const TCHAR* Name,
	const EBathWaterCapacityKind Kind,
	const float Points)
{
	AActor* Owner = World.SpawnActor<AActor>();
	UUtilityOperationComponent* Operation = NewObject<UUtilityOperationComponent>(Owner, FName(*FString::Printf(TEXT("%sOperation"), Name)));
	Operation->RegisterComponent();
	FText FailureReason;
	if (!Operation->ImportOperationState(Points, FailureReason) || !Operation->StartPlacedClock(false))
	{
		return nullptr;
	}
	UBathWaterUtilityCapacityComponent* Provider = NewObject<UBathWaterUtilityCapacityComponent>(Owner, Name);
	Provider->RestoreCapacity(Kind, Points);
	Provider->SetUtilityOperation(Operation);
	Provider->RegisterComponent();
	return Provider;
}
}

#endif
