#include "Service/TelevisionActor.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Placement/FacilityPlacementComponent.h"

ATelevisionActor::ATelevisionActor()
{
	FacilityType = EBathhouseFacilityType::Television;
}

FPlayerInteractionQuery ATelevisionActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	if (!FacilityPlacement || !FacilityPlacement->IsPlacedDomainActive() || FacilityPlacement->IsStagedPlacement())
	{
		return Query;
	}
	Query.bVisible = true;
	Query.TargetName = FText::FromString(bPoweredOn ? TEXT("TV · 켜짐") : TEXT("TV · 꺼짐"));
	Query.ActionName = FText::FromString(bPoweredOn ? TEXT("끄기") : TEXT("켜기"));
	Query.bCanInteract = Context.CarryComponent && Context.CarryComponent->IsHandEmpty();
	if (!Query.bCanInteract)
	{
		Query.FailureReason = FText::FromString(TEXT("빈손으로 켜고 끌 수 있음"));
	}
	return Query;
}

FPlayerInteractionResult ATelevisionActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	const auto Query = QueryInteraction(Context);
	if (!Query.bCanInteract)
	{
		return FPlayerInteractionResult::Failed(Query.FailureReason);
	}
	bPoweredOn = !bPoweredOn;
	OnPowerChanged(bPoweredOn);
	return FPlayerInteractionResult::Succeeded();
}
