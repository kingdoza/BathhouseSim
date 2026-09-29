#include "Utility/UtilityFuelIntakeVolumeComponent.h"

#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Utility/BathWaterFuelUtilityFacilityActor.h"
#include "Utility/UtilityFuelDoorComponent.h"
#include "Utility/UtilityFuelTransaction.h"
#include "Utility/UtilityShovelActor.h"

#define LOCTEXT_NAMESPACE "UtilityFuelIntakeVolumeComponent"

UUtilityFuelIntakeVolumeComponent::UUtilityFuelIntakeVolumeComponent()
{
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetCanEverAffectNavigation(false);
	SetGenerateOverlapEvents(false);
	SetHiddenInGame(true);
}

FPlayerInteractionQuery UUtilityFuelIntakeVolumeComponent::QueryInteraction(
	const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	Query.bVisible = true;
	const ABathWaterFuelUtilityFacilityActor* FuelFacility = Cast<ABathWaterFuelUtilityFacilityActor>(GetOwner());
	const FText FacilityName = FuelFacility ? FuelFacility->GetFuelFacilityDisplayName() : FText::GetEmpty();
	const FText FuelName = FuelFacility ? GetUtilityFuelKindDisplayName(FuelFacility->GetAcceptedFuelKind()) : FText::GetEmpty();
	Query.TargetName = FText::Format(LOCTEXT("IntakeTarget", "{0} 투입구"), FacilityName);
	Query.ActionName = FText::GetEmpty();
	Query.bCanInteract = false;
	Query.FailureReason = FText::GetEmpty();
	Query.bHeldApplyVisible = true;
	Query.HeldApplyActionName = FText::Format(LOCTEXT("IntakeAction", "{0} 투입"), FuelName);
	Query.HeldApplyActivationMode = EPlayerInteractionActivationMode::Instant;
	const FUtilityFuelResult Evaluation = FUtilityFuelTransaction::EvaluateInsert(Context, *this);
	Query.bCanHeldApply = Evaluation.bSucceeded;
	Query.HeldApplyFailureReason = Evaluation.FailureReason;
	return Query;
}

FPlayerInteractionResult UUtilityFuelIntakeVolumeComponent::ExecuteInteraction(
	const FPlayerInteractionContext& Context)
{
	return FPlayerInteractionResult::Failed(FText::GetEmpty(), EPlayerInteractionIntent::Primary);
}

FPlayerInteractionResult UUtilityFuelIntakeVolumeComponent::ExecuteHeldTargetUse(
	const FPlayerInteractionContext& Context,
	const EPlayerHeldTargetUseDirection Direction)
{
	const EPlayerInteractionIntent Intent = Direction == EPlayerHeldTargetUseDirection::Apply
		? EPlayerInteractionIntent::HeldApply
		: EPlayerInteractionIntent::HeldTake;
	if (Direction != EPlayerHeldTargetUseDirection::Apply)
	{
		return FPlayerInteractionResult::Failed(FText::GetEmpty(), Intent);
	}
	AUtilityShovelActor* Shovel = Context.CarryComponent
		? Cast<AUtilityShovelActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	if (!IsValid(Shovel))
	{
		return FPlayerInteractionResult::Failed(
			LOCTEXT("InsertNeedsShovel", "삽을 들고 있어야 합니다."),
			Intent);
	}

	const FUtilityFuelResult Result = FUtilityFuelTransaction::Insert(*Shovel, *this, Context);
	return Result.bSucceeded
		? FPlayerInteractionResult::Succeeded(Intent)
		: FPlayerInteractionResult::Failed(Result.FailureReason, Intent);
}

void UUtilityFuelIntakeVolumeComponent::NotifyInteractionFocusChanged(
	const UPlayerInteractionComponent& Source,
	const FPlayerInteractionQuery& Query)
{
	if (IsValid(FuelDoorPresentation))
	{
		FuelDoorPresentation->SetSourceInsertable(
			&Source,
			Query.bHeldApplyVisible && Query.bCanHeldApply);
	}
}

void UUtilityFuelIntakeVolumeComponent::NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source)
{
	if (IsValid(FuelDoorPresentation))
	{
		FuelDoorPresentation->RemoveSource(&Source);
	}
}

void UUtilityFuelIntakeVolumeComponent::SetFuelDoorPresentation(UUtilityFuelDoorComponent* InPresentation)
{
	FuelDoorPresentation = InPresentation;
}

bool UUtilityFuelIntakeVolumeComponent::HasValidAuthoring(FText& OutFailureReason) const
{
	const FVector Extent = GetUnscaledBoxExtent();
	const FVector RelativeScale = GetRelativeScale3D();
	if (!FMath::IsFinite(Extent.X) || !FMath::IsFinite(Extent.Y) || !FMath::IsFinite(Extent.Z)
		|| Extent.X <= 0.0f || Extent.Y <= 0.0f || Extent.Z <= 0.0f
		|| !RelativeScale.Equals(FVector::OneVector, 1.0e-4f))
	{
		OutFailureReason = LOCTEXT("InvalidIntakeExtent", "연료 투입 판정 Box에는 양수 extent와 단위 상대 scale이 필요합니다.");
		return false;
	}
	const FBodyInstance* ConfiguredBodyInstance = GetBodyInstance();
	if (!ConfiguredBodyInstance || ConfiguredBodyInstance->GetCollisionEnabled(false) != ECollisionEnabled::QueryOnly)
	{
		OutFailureReason = LOCTEXT("InvalidIntakeCollision", "연료 투입 판정 Box 충돌은 QueryOnly여야 합니다.");
		return false;
	}
	if (GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block)
	{
		OutFailureReason = LOCTEXT("InvalidIntakeVisibility", "연료 투입 판정 Box는 Visibility trace를 Block해야 합니다.");
		return false;
	}
	if (CanEverAffectNavigation())
	{
		OutFailureReason = LOCTEXT("InvalidIntakeNavigation", "연료 투입 판정 Box는 Navigation에 영향을 주지 않아야 합니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
