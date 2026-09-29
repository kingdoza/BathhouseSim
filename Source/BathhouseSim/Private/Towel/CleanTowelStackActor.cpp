#include "Towel/CleanTowelStackActor.h"
#include "Placement/FacilityPlacementComponent.h"

#include "Interaction/PlayerCarryComponent.h"
#include "Towel/TowelBasketActor.h"
#include "Towel/TowelInventoryComponent.h"
#include "Towel/Presentation/TowelStackVisualComponent.h"
#include "Towel/TowelTransferSubsystem.h"
#include "Towel/TowelHeldTransferRules.h"

#define LOCTEXT_NAMESPACE "CleanTowelStackActor"

ACleanTowelStackActor::ACleanTowelStackActor()
{
	FacilityType = EBathhouseFacilityType::TowelShelf;
	Inventory = CreateDefaultSubobject<UTowelInventoryComponent>(TEXT("TowelInventory"));
	Inventory->ConfigureDefaults(ETowelState::Clean, 20, 30);
	TowelPresentationVisual = CreateDefaultSubobject<UTowelStackVisualComponent>(TEXT("TowelPresentationVisual"));
	TowelPresentationVisual->SetupAttachment(GetRootComponent());
}

void ACleanTowelStackActor::BeginPlay()
{
	Super::BeginPlay();
	TowelPresentationVisual->BindInventorySource(Inventory);
}

void ACleanTowelStackActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TowelPresentationVisual->UnbindInventorySource();
	Super::EndPlay(EndPlayReason);
}

FPlayerInteractionQuery ACleanTowelStackActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	if (GetFacilityPlacementComponent()
		&& !GetFacilityPlacementComponent()->IsPlacedDomainActive())
	{
		return ABathhouseFacilityActor::QueryInteraction(Context);
	}

	FPlayerInteractionQuery Query;
	Query.bVisible = true;
	Query.TargetName = LOCTEXT("CleanStack", "깨끗한 수건 선반");
	const ATowelBasketActor* Basket = Context.CarryComponent
		? Cast<ATowelBasketActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	const FTowelInventorySnapshot BasketSnapshot = Basket && Basket->GetInventory()
		? Basket->GetInventory()->GetSnapshot()
		: FTowelInventorySnapshot();
	const FTowelHeldTransferQuery HeldUseQuery = FTowelHeldTransferRules::Build(
		ETowelHeldTransferTargetKind::CleanShelf,
		Query.TargetName,
		Inventory ? Inventory->GetSnapshot() : FTowelInventorySnapshot(),
		Basket && Basket->GetInventory(),
		BasketSnapshot);
	FTowelHeldTransferRules::ApplyToQuery(Query, HeldUseQuery);
	return Query;
}

FPlayerInteractionResult ACleanTowelStackActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	if (GetFacilityPlacementComponent()
		&& !GetFacilityPlacementComponent()->IsPlacedDomainActive())
	{
		return ABathhouseFacilityActor::ExecuteInteraction(Context);
	}
	return FPlayerInteractionResult::Failed(FText::GetEmpty(), EPlayerInteractionIntent::Primary);
}

FPlayerInteractionResult ACleanTowelStackActor::ExecuteHeldTargetUse(
	const FPlayerInteractionContext& Context,
	const EPlayerHeldTargetUseDirection Direction)
{
	return TransferFromHeldBasket(Context, Direction);
}

FPlayerInteractionResult ACleanTowelStackActor::TransferFromHeldBasket(
	const FPlayerInteractionContext& Context,
	const EPlayerHeldTargetUseDirection Direction)
{
	const EPlayerInteractionIntent Intent = Direction == EPlayerHeldTargetUseDirection::Apply
		? EPlayerInteractionIntent::HeldApply
		: EPlayerInteractionIntent::HeldTake;
	if (GetFacilityPlacementComponent()
		&& !GetFacilityPlacementComponent()->IsPlacedDomainActive())
	{
		return FPlayerInteractionResult::Failed(FText::GetEmpty(), Intent);
	}

	const FPlayerInteractionQuery Query = QueryInteraction(Context);
	const bool bCanTransfer = Direction == EPlayerHeldTargetUseDirection::Apply
		? Query.bCanHeldApply
		: Query.bCanHeldTake;
	const FText& FailureReason = Direction == EPlayerHeldTargetUseDirection::Apply
		? Query.HeldApplyFailureReason
		: Query.HeldTakeFailureReason;
	if (!bCanTransfer)
	{
		return FPlayerInteractionResult::Failed(FailureReason, Intent);
	}

	ATowelBasketActor* Basket = Context.CarryComponent
		? Cast<ATowelBasketActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	UTowelTransferSubsystem* Transfer = GetWorld()->GetSubsystem<UTowelTransferSubsystem>();
	if (!Basket || !Transfer || !Basket->GetInventory() || !Inventory)
	{
		return FPlayerInteractionResult::Failed(
			LOCTEXT("DepositFailed", "수건을 선반으로 옮길 수 없습니다."),
			Intent);
	}

	FTowelTransferRequest Request;
	Request.Source = Direction == EPlayerHeldTargetUseDirection::Apply
		? Basket->GetInventory() : Inventory.Get();
	Request.Destination = Direction == EPlayerHeldTargetUseDirection::Apply
		? Inventory.Get() : Basket->GetInventory();
	Request.RequestedCount = 1;
	Request.ExpectedSourceRevision = Request.Source->GetSnapshot().Revision;
	Request.ExpectedDestinationRevision = Request.Destination->GetSnapshot().Revision;
	const FTowelTransferResult Result = Transfer->TryTransfer(Request);
	return Result.bSucceeded
		? FPlayerInteractionResult::Succeeded(Intent)
		: FPlayerInteractionResult::Failed(
			LOCTEXT("DepositFailed", "수건을 선반으로 옮길 수 없습니다."),
			Intent);
}

#undef LOCTEXT_NAMESPACE
