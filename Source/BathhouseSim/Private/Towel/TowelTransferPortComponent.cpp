#include "Towel/TowelTransferPortComponent.h"

#include "Interaction/PlayerCarryComponent.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Towel/TowelBasketActor.h"
#include "Towel/TowelInventoryComponent.h"
#include "Towel/TowelProcessingMachineActor.h"
#include "Towel/TowelTransferSubsystem.h"
#include "Towel/TowelHeldTransferRules.h"
#include "Towel/TowelDisplayCueUtils.h"
#include "Towel/Presentation/TowelPileVisualComponent.h"
#include "Service/DisplayCueComponent.h"
#include "Interaction/Presentation/OpeningPresentationComponent.h"

#define LOCTEXT_NAMESPACE "TowelTransferPortComponent"

UTowelTransferPortComponent::UTowelTransferPortComponent()
{
	InitBoxExtent(FVector(30.0f));
	SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetCanEverAffectNavigation(false);
}

FPlayerInteractionQuery UTowelTransferPortComponent::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	const ATowelProcessingMachineActor* Machine = Cast<ATowelProcessingMachineActor>(GetOwner());
	Query.PresentationRevision = TowelDisplayCueUtils::GetPresentationRevision(
		Context, Machine ? Machine->GetInventory() : nullptr);
	if (!Machine || (Machine->GetFacilityPlacementComponent()
		&& !Machine->GetFacilityPlacementComponent()->IsPlacedDomainActive()))
	{
		return Query;
	}

	Query.bVisible = true;
	Query.TargetName = LOCTEXT("MachinePort", "수건 투입구");
	const ATowelBasketActor* Basket = Context.CarryComponent
		? Cast<ATowelBasketActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	const FTowelInventorySnapshot BasketSnapshot = Basket && Basket->GetInventory()
		? Basket->GetInventory()->GetSnapshot()
		: FTowelInventorySnapshot();
	const FTowelHeldTransferQuery HeldUseQuery = FTowelHeldTransferRules::Build(
		ETowelHeldTransferTargetKind::MachinePort,
		Query.TargetName,
		Machine->GetInventory() ? Machine->GetInventory()->GetSnapshot() : FTowelInventorySnapshot(),
		Basket && Basket->GetInventory(),
		BasketSnapshot,
		Machine->GetMachineState(),
		Machine->GetInputState(),
		Machine->GetOutputState());
	FTowelHeldTransferRules::ApplyToQuery(Query, HeldUseQuery);
	return Query;
}

FPlayerInteractionResult UTowelTransferPortComponent::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	return FPlayerInteractionResult::Failed(FText::GetEmpty(), EPlayerInteractionIntent::Primary);
}

FPlayerInteractionResult UTowelTransferPortComponent::ExecuteHeldTargetUse(
	const FPlayerInteractionContext& Context,
	const EPlayerHeldTargetUseDirection Direction)
{
	return Transfer(Context, Direction);
}

FPlayerInteractionResult UTowelTransferPortComponent::Transfer(
	const FPlayerInteractionContext& Context,
	const EPlayerHeldTargetUseDirection Direction)
{
	const EPlayerInteractionIntent Intent = Direction == EPlayerHeldTargetUseDirection::Apply
		? EPlayerInteractionIntent::HeldApply
		: EPlayerInteractionIntent::HeldTake;
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

	ATowelProcessingMachineActor* Machine = Cast<ATowelProcessingMachineActor>(GetOwner());
	ATowelBasketActor* Basket = Context.CarryComponent
		? Cast<ATowelBasketActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	UTowelTransferSubsystem* TransferSubsystem = GetWorld()
		? GetWorld()->GetSubsystem<UTowelTransferSubsystem>()
		: nullptr;
	if (!Machine || !Basket || !TransferSubsystem || !Machine->GetInventory() || !Basket->GetInventory()
		|| (Machine->GetFacilityPlacementComponent()
			&& !Machine->GetFacilityPlacementComponent()->IsPlacedDomainActive()))
	{
		return FPlayerInteractionResult::Failed(
			LOCTEXT("PortUnavailable", "현재 수건을 옮길 수 없습니다."),
			Intent);
	}

	FTowelTransferRequest Request;
	Request.Source = Direction == EPlayerHeldTargetUseDirection::Apply
		? Basket->GetInventory() : Machine->GetInventory();
	Request.Destination = Direction == EPlayerHeldTargetUseDirection::Apply
		? Machine->GetInventory() : Basket->GetInventory();
	Request.RequestedCount = 1;
	Request.ExpectedSourceRevision = Request.Source->GetSnapshot().Revision;
	Request.ExpectedDestinationRevision = Request.Destination->GetSnapshot().Revision;
	const FTowelTransferResult Result = TransferSubsystem->TryTransfer(Request);
	return Result.bSucceeded
		? FPlayerInteractionResult::Succeeded(Intent)
		: FPlayerInteractionResult::Failed(
			LOCTEXT("TransferFailed", "수건을 옮길 수 없습니다."),
			Intent);
}

#undef LOCTEXT_NAMESPACE

void UTowelTransferPortComponent::NotifyInteractionFocusChanged(const UPlayerInteractionComponent& Source,
																const FPlayerInteractionQuery& Query)
{
	if (auto* Machine = Cast<ATowelProcessingMachineActor>(GetOwner()))
	{
		TowelDisplayCueUtils::Update(Source, Query, Machine->GetInventory(), Machine->GetTowelVisual(),
									 Machine->GetDisplayCue());
		const bool bCanOpen =
			Machine->GetMachineState() != ETowelMachineState::Processing &&
			((Query.bHeldApplyVisible && Query.bCanHeldApply) || (Query.bHeldTakeVisible && Query.bCanHeldTake));
		Machine->GetLidPresentation()->SetSourceInsertable(this, bCanOpen);
	}
}

void UTowelTransferPortComponent::NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source)
{
	if (auto* Machine = Cast<ATowelProcessingMachineActor>(GetOwner()))
	{
		Machine->GetDisplayCue()->HideAll();
		Machine->GetLidPresentation()->RemoveSource(this);
	}
}

void UTowelTransferPortComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* Machine = Cast<ATowelProcessingMachineActor>(GetOwner()))
	{
		Machine->GetDisplayCue()->HideAll();
		Machine->GetLidPresentation()->RemoveSource(this);
	}
	Super::EndPlay(Reason);
}
