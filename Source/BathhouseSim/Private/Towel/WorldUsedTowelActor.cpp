#include "Towel/WorldUsedTowelActor.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Towel/TowelBasketActor.h"
#include "Towel/TowelCirculationSubsystem.h"
#include "Towel/TowelInventoryComponent.h"
#include "Towel/TowelTransferSubsystem.h"
#include "Towel/TowelHeldTransferRules.h"

#define LOCTEXT_NAMESPACE "WorldUsedTowelActor"

AWorldUsedTowelActor::AWorldUsedTowelActor()
{
	PrimaryActorTick.bCanEverTick = false;
	WorldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorldMesh"));
	SetRootComponent(WorldMesh);
	WorldMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Inventory = CreateDefaultSubobject<UTowelInventoryComponent>(TEXT("TowelInventory"));
	Inventory->ConfigureDefaults(ETowelState::Used, 1, 1);
	Inventory->SetRecoverContentsOnEndPlay(false);
}

void AWorldUsedTowelActor::BeginPlay()
{
	Super::BeginPlay();
	WorldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UTowelCirculationSubsystem* Subsystem = GetWorld()->GetSubsystem<UTowelCirculationSubsystem>())
	{
		Subsystem->RegisterWorldTowel(this);
	}
}

void AWorldUsedTowelActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UTowelCirculationSubsystem* Subsystem = World->GetSubsystem<UTowelCirculationSubsystem>())
		{
			Subsystem->UnregisterWorldTowel(this);
			if (bTokenCommitted && !bConsumed && !bRecoveryCommitted && Inventory->GetSnapshot().Count > 0)
			{
				Subsystem->RecoverInventory(Inventory);
				bRecoveryCommitted = true;
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

FPlayerInteractionQuery AWorldUsedTowelActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	if (!bTokenCommitted || bConsumed)
	{
		return Query;
	}
	Query.bVisible = true;
	Query.TargetName = LOCTEXT("UsedTowel", "사용한 수건");
	const ATowelBasketActor* Basket = Context.CarryComponent
		? Cast<ATowelBasketActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	const FTowelInventorySnapshot BasketSnapshot = Basket && Basket->GetInventory()
		? Basket->GetInventory()->GetSnapshot()
		: FTowelInventorySnapshot();
	const FTowelHeldTransferQuery HeldUseQuery = FTowelHeldTransferRules::Build(
		ETowelHeldTransferTargetKind::WorldUsedTowel,
		Query.TargetName,
		Inventory ? Inventory->GetSnapshot() : FTowelInventorySnapshot(),
		Basket && Basket->GetInventory(),
		BasketSnapshot);
	FTowelHeldTransferRules::ApplyToQuery(Query, HeldUseQuery);
	return Query;
}

void AWorldUsedTowelActor::CommitStagedToken()
{
	if (bTokenCommitted || bConsumed)
	{
		return;
	}
	bTokenCommitted = true;
	Inventory->SetRecoverContentsOnEndPlay(true);
	WorldMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

FPlayerInteractionResult AWorldUsedTowelActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	return FPlayerInteractionResult::Failed(FText::GetEmpty(), EPlayerInteractionIntent::Primary);
}

FPlayerInteractionResult AWorldUsedTowelActor::ExecuteHeldTargetUse(
	const FPlayerInteractionContext& Context,
	const EPlayerHeldTargetUseDirection Direction)
{
	const EPlayerInteractionIntent Intent = Direction == EPlayerHeldTargetUseDirection::Apply
		? EPlayerInteractionIntent::HeldApply
		: EPlayerInteractionIntent::HeldTake;
	const FPlayerInteractionQuery Query = QueryInteraction(Context);
	if (Direction != EPlayerHeldTargetUseDirection::Take || !Query.bCanHeldTake)
	{
		const FText& FailureReason = Direction == EPlayerHeldTargetUseDirection::Apply
			? Query.HeldApplyFailureReason
			: Query.HeldTakeFailureReason;
		return FPlayerInteractionResult::Failed(FailureReason, Intent);
	}

	ATowelBasketActor* Basket = Context.CarryComponent
		? Cast<ATowelBasketActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	UTowelTransferSubsystem* Transfer = GetWorld()
		? GetWorld()->GetSubsystem<UTowelTransferSubsystem>()
		: nullptr;
	if (!Basket || !Transfer || !bTokenCommitted || bConsumed
		|| !Inventory || !Basket->GetInventory())
	{
		return FPlayerInteractionResult::Failed(
			LOCTEXT("CollectFailed", "수건을 주울 수 없습니다."),
			Intent);
	}

	FTowelTransferRequest Request;
	Request.Source = Inventory;
	Request.Destination = Basket->GetInventory();
	Request.RequestedCount = 1;
	Request.ExpectedSourceRevision = Inventory->GetSnapshot().Revision;
	Request.ExpectedDestinationRevision = Basket->GetInventory()->GetSnapshot().Revision;
	const FTowelTransferResult Result = Transfer->TryTransfer(Request);
	if (!Result.bSucceeded)
	{
		return FPlayerInteractionResult::Failed(
			LOCTEXT("CollectFailed", "수건을 주울 수 없습니다."),
			Intent);
	}
	bConsumed = true;
	WorldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetLifeSpan(0.05f);
	return FPlayerInteractionResult::Succeeded(Intent);
}

#undef LOCTEXT_NAMESPACE
