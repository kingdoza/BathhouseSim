#include "Utility/UtilityFuelIntakeComponent.h"

#define LOCTEXT_NAMESPACE "UtilityFuelIntakeComponent"

UUtilityFuelIntakeComponent::UUtilityFuelIntakeComponent()
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCanEverAffectNavigation(false);
}

bool UUtilityFuelIntakeComponent::HasValidAuthoring(FText& OutFailureReason) const
{
	if (!GetStaticMesh())
	{
		OutFailureReason = FText::GetEmpty();
		return true;
	}
	if (GetCollisionEnabled() != ECollisionEnabled::NoCollision || CanEverAffectNavigation())
	{
		OutFailureReason = LOCTEXT(
			"InvalidIntakePresentation",
			"연료 투입구 외형 mesh는 collision과 navigation을 사용하지 않아야 합니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
