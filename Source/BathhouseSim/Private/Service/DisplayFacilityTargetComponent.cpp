#include "Service/DisplayFacilityTargetComponent.h"
#include "Service/ServiceDisplayManagerComponent.h"
#include "Service/DisplaySpaceComponent.h"
#include "Service/ItemBoxActor.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#define LOCTEXT_NAMESPACE "DisplayFacilityTarget"

UDisplayFacilityTargetComponent::UDisplayFacilityTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InitBoxExtent(FVector(50));
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
}

bool UDisplayFacilityTargetComponent::ValidateAuthoring(FText& OutFailureReason) const
{
	if (FacilityDisplayName.IsEmptyOrWhitespace())
	{
		OutFailureReason = LOCTEXT("MissingFacilityDisplayName", "설비 표시 이름을 지정하십시오.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

#if WITH_EDITOR
EDataValidationResult UDisplayFacilityTargetComponent::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	FText Failure;
	if (!ValidateAuthoring(Failure))
	{
		Context.AddError(Failure);
		return EDataValidationResult::Invalid;
	}
	return ParentResult == EDataValidationResult::Invalid ? ParentResult : EDataValidationResult::Valid;
}
#endif

bool UDisplayFacilityTargetComponent::CollectSpaces(TArray<UDisplaySpaceComponent*>& Out) const
{
	const auto* Manager = GetOwner() ? GetOwner()->FindComponentByClass<UServiceDisplayManagerComponent>() : nullptr;
	FText Failure;
	return Manager && Manager->CollectSpaces(Out, Failure) && !Out.IsEmpty() && Out[0]->IsOperational();
}

int32 UDisplayFacilityTargetComponent::SelectClosestSpace(TConstArrayView<FVector> Centers,
														  TConstArrayView<int32> Indices, const FVector& Start,
														  const FVector& End)
{
	if (Centers.Num() != Indices.Num())
	{
		return INDEX_NONE;
	}
	double BestDistance = TNumericLimits<double>::Max();
	int32 Best = INDEX_NONE;
	for (int32 Index = 0; Index < Centers.Num(); ++Index)
	{
		const double Distance = FMath::PointDistToSegmentSquared(Centers[Index], Start, End);
		if (Distance < BestDistance || (Distance == BestDistance && (Best == INDEX_NONE || Indices[Index] < Best)))
		{
			BestDistance = Distance;
			Best = Indices[Index];
		}
	}
	return Best;
}

UDisplaySpaceComponent* UDisplayFacilityTargetComponent::SelectSpace(
	const FPlayerInteractionContext& Context, const TArray<UDisplaySpaceComponent*>& Spaces) const
{
	const auto* Box = Context.CarryComponent ? Cast<AItemBoxActor>(Context.CarryComponent->GetHeldObject()) : nullptr;
	if (!IsValid(Box) || !Box->IsHeld())
	{
		return nullptr;
	}
	if (Box->GetContents().Kind)
	{
		for (auto* Space : Spaces)
		{
			if (Space->GetTargetMode() == EDisplaySpaceTargetMode::FacilityRouted &&
				Space->GetFixedKind() == Box->GetContents().Kind)
			{
				return Space;
			}
		}
		return nullptr;
	}
	TArray<FVector> Centers;
	TArray<int32> Indices;
	for (const auto* Space : Spaces)
	{
		if (Space->GetTargetMode() == EDisplaySpaceTargetMode::FacilityRouted)
		{
			Centers.Add(Space->GetSlotsWorldCenter());
			Indices.Add(Space->GetSpaceIndex());
		}
	}
	const int32 Selected =
		SelectClosestSpace(Centers, Indices, Context.HitResult.TraceStart, Context.HitResult.TraceEnd);
	for (auto* Space : Spaces)
	{
		if (Space->GetSpaceIndex() == Selected)
		{
			return Space;
		}
	}
	return nullptr;
}

FPlayerInteractionQuery UDisplayFacilityTargetComponent::QueryInteraction(
	const FPlayerInteractionContext& Context) const
{
	TArray<UDisplaySpaceComponent*> Spaces;
	if (!CollectSpaces(Spaces))
	{
		return {};
	}
	FPlayerInteractionQuery Query;
	Query.bVisible = true;
	TArray<FString> Lines;
	Lines.Add(FacilityDisplayName.ToString());
	for (const auto* Space : Spaces)
	{
		Lines.Add(Space->GetStockSummary().ToString());
	}
	auto* Selected = SelectSpace(Context, Spaces);
	const auto* Box = Context.CarryComponent ? Cast<AItemBoxActor>(Context.CarryComponent->GetHeldObject()) : nullptr;
	if (IsValid(Box) && Box->IsHeld())
	{
		if (Selected)
		{
			Query = Selected->BuildHeldUseQuery(Context);
			Query.HeldUseTargetKey = Selected->GetSpaceIndex();
		}
		else
		{
			Query.bHeldApplyVisible = Query.bHeldTakeVisible = true;
			Query.HeldApplyActionName = LOCTEXT("Apply", "넣기");
			Query.HeldTakeActionName = LOCTEXT("Take", "빼기");
			Query.HeldApplyFailureReason = Query.HeldTakeFailureReason =
				LOCTEXT("Rejected", "여기에 넣을 수 없는 물건");
			Query.HeldApplyActivationMode = Query.HeldTakeActivationMode = EPlayerInteractionActivationMode::Repeat;
		}
	}
	Query.TargetName = FText::FromString(FString::Join(Lines, TEXT("\n")));
	return Query;
}

FPlayerInteractionResult UDisplayFacilityTargetComponent::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	return FPlayerInteractionResult::Failed(FText::GetEmpty());
}

FPlayerInteractionResult UDisplayFacilityTargetComponent::ExecuteHeldTargetUse(const FPlayerInteractionContext& Context,
																			   EPlayerHeldTargetUseDirection Direction)
{
	TArray<UDisplaySpaceComponent*> Spaces;
	auto* Space = CollectSpaces(Spaces) ? SelectSpace(Context, Spaces) : nullptr;
	const auto Intent = Direction == EPlayerHeldTargetUseDirection::Apply ? EPlayerInteractionIntent::HeldApply
																		  : EPlayerInteractionIntent::HeldTake;
	return Space ? Space->ExecuteRoutedHeldTargetUse(Context, Direction)
				 : FPlayerInteractionResult::Failed(LOCTEXT("Rejected", "여기에 넣을 수 없는 물건"), Intent);
}

void UDisplayFacilityTargetComponent::NotifyInteractionFocusChanged(const UPlayerInteractionComponent& Source,
																	const FPlayerInteractionQuery& Query)
{
	TArray<UDisplaySpaceComponent*> Spaces;
	GetOwner()->GetComponents(Spaces);
	for (auto* Space : Spaces)
	{
		if (Space->GetSpaceIndex() == Query.HeldUseTargetKey)
		{
			Space->NotifyInteractionFocusChanged(Source, Query);
		}
		else
		{
			Space->NotifyInteractionFocusEnded(Source);
		}
	}
}

void UDisplayFacilityTargetComponent::NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source)
{
	TArray<UDisplaySpaceComponent*> Spaces;
	if (GetOwner())
	{
		GetOwner()->GetComponents(Spaces);
	}
	for (auto* Space : Spaces)
	{
		Space->NotifyInteractionFocusEnded(Source);
	}
}

#undef LOCTEXT_NAMESPACE
