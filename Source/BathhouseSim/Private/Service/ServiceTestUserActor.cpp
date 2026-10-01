#include "Service/ServiceTestUserActor.h"
#include "Components/StaticMeshComponent.h"
#include "Economy/BathhouseCashPaymentActor.h"

AServiceTestUserActor::AServiceTestUserActor()
{
	PrimaryActorTick.bCanEverTick = false;
	WorldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorldMesh"));
	SetRootComponent(WorldMesh);
	WorldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WorldMesh->SetCanEverAffectNavigation(false);
}

void AServiceTestUserActor::HandleServiceUseEnded(AActor& Facility, EServiceUseEndReason Reason)
{
	Destroy();
}

void AServiceTestUserActor::HandleScrubCashOffered(AScrubTableActor& Table, ABathhouseCashPaymentActor& Cash,
												   const FTransform& StandTransform)
{
	if (auto* Previous = OfferedCash.Get())
	{
		Previous->OnCashClaimed.RemoveDynamic(this, &AServiceTestUserActor::HandleCashClaimed);
	}
	SetActorTransform(StandTransform, false, nullptr, ETeleportType::TeleportPhysics);
	OfferedCash = &Cash;
	Cash.OnCashClaimed.AddUniqueDynamic(this, &AServiceTestUserActor::HandleCashClaimed);
}

void AServiceTestUserActor::HandleCashClaimed(ABathhouseCashPaymentActor* Cash)
{
	Destroy();
}

void AServiceTestUserActor::EndPlay(EEndPlayReason::Type Reason)
{
	if (auto* Cash = OfferedCash.Get())
	{
		Cash->OnCashClaimed.RemoveDynamic(this, &AServiceTestUserActor::HandleCashClaimed);
	}
	OfferedCash.Reset();
	Super::EndPlay(Reason);
}
