#include "Utility/UtilityFuelIntakeComponent.h"

#define LOCTEXT_NAMESPACE "UtilityFuelIntakeComponent"

UUtilityFuelIntakeComponent::UUtilityFuelIntakeComponent()
{
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetCanEverAffectNavigation(false);
}

FPlayerInteractionQuery UUtilityFuelIntakeComponent::QueryInteraction(
	const FPlayerInteractionContext& Context) const
{
	(void)Context;
	FPlayerInteractionQuery Query;
	Query.bVisible = true;
	Query.TargetName = LOCTEXT("IntakeTarget", "보일러 투입구");
	Query.ActionName = LOCTEXT("IntakeAction", "삽으로 연료 투입");
	Query.FailureReason = LOCTEXT("UseHeldShovel", "삽을 들고 투입구를 향해 사용하세요.");
	return Query;
}

FPlayerInteractionResult UUtilityFuelIntakeComponent::ExecuteInteraction(
	const FPlayerInteractionContext& Context)
{
	(void)Context;
	return FPlayerInteractionResult::Failed(
		LOCTEXT("UseHeldEquipment", "삽을 들고 투입구를 향해 사용하세요."));
}

bool UUtilityFuelIntakeComponent::HasValidAuthoring(FText& OutFailureReason) const
{
	const FBodyInstance* IntakeBody = GetBodyInstance();
	if (!GetStaticMesh())
	{
		OutFailureReason = LOCTEXT("MissingIntakeMesh", "연료 투입구에 메시를 지정해야 합니다.");
		return false;
	}
	if (!IntakeBody || IntakeBody->GetCollisionEnabled(false) != ECollisionEnabled::QueryOnly)
	{
		OutFailureReason = LOCTEXT("InvalidIntakeCollision", "연료 투입구 충돌은 QueryOnly여야 합니다.");
		return false;
	}
	if (GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block)
	{
		OutFailureReason = LOCTEXT("InvalidIntakeVisibility", "연료 투입구는 Visibility trace를 Block해야 합니다.");
		return false;
	}
	if (CanEverAffectNavigation())
	{
		OutFailureReason = LOCTEXT("InvalidIntakeNavigation", "연료 투입구는 Navigation에 영향을 주지 않아야 합니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
