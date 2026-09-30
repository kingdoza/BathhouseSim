#include "Cleaning/TrashCollectionZoneActor.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Interaction/PhysicalCarryable.h"
#include "Interaction/PhysicalCarryDiscardable.h"
#include "TimerManager.h"

ATrashCollectionZoneActor::ATrashCollectionZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;
	CollectionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("CollectionBounds"));
	SetRootComponent(CollectionBounds);
	CollectionBounds->InitBoxExtent(FVector(150, 150, 100));
	CollectionBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollectionBounds->SetCanEverAffectNavigation(false);
}

void ATrashCollectionZoneActor::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimer(CollectionTimer, this, &ATrashCollectionZoneActor::CollectNow,
									FMath::Max(1.0f, CollectionIntervalSeconds), true);
}

void ATrashCollectionZoneActor::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(CollectionTimer);
	Super::EndPlay(Reason);
}

void ATrashCollectionZoneActor::CollectNow()
{
	UWorld* World = GetWorld();
	if (!World || !CollectionBounds)
	{
		return;
	}
	TArray<FOverlapResult> Hits;
	World->OverlapMultiByObjectType(Hits, CollectionBounds->GetComponentLocation(),
									CollectionBounds->GetComponentQuat(), FCollisionObjectQueryParams::AllObjects,
									FCollisionShape::MakeBox(CollectionBounds->GetScaledBoxExtent()));
	TArray<TWeakObjectPtr<AActor>> Targets;
	for (const auto& Hit : Hits)
	{
		AActor* Actor = Hit.GetActor();
		auto* Discard = Cast<IPhysicalCarryDiscardable>(Actor);
		auto* Carryable = Cast<IPhysicalCarryable>(Actor);
		auto* Root = Carryable ? Carryable->GetPhysicalCarryPrimitive() : nullptr;
		FText Failure;
		if (!IsValid(Actor) || !Discard || !Root || !Discard->CanDiscardFromWorld(Failure))
		{
			continue;
		}
		const FVector Local = CollectionBounds->GetComponentTransform().InverseTransformPosition(Root->Bounds.Origin);
		const FVector Extent = CollectionBounds->GetUnscaledBoxExtent();
		if (FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z)
		{
			Targets.AddUnique(Actor);
		}
	}
	for (auto Target : Targets)
	{
		if (Target.IsValid())
		{
			if (auto* Discard = Cast<IPhysicalCarryDiscardable>(Target.Get()))
			{
				Discard->HandleDiscardFromWorldCommitted();
			}
		}
	}
}
