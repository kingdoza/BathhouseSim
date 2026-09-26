#include "Utility/UtilityLeverOperatingVolumeComponent.h"

#include "PhysicsEngine/BodyInstance.h"
#include "Utility/UtilityLeverLaborComponent.h"

#define LOCTEXT_NAMESPACE "UtilityLeverOperatingVolumeComponent"

UUtilityLeverOperatingVolumeComponent::UUtilityLeverOperatingVolumeComponent()
{
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetCanEverAffectNavigation(false);
	SetGenerateOverlapEvents(false);
	SetHiddenInGame(true);
}

FPlayerInteractionQuery UUtilityLeverOperatingVolumeComponent::QueryInteraction(
	const FPlayerInteractionContext& Context) const
{
	return IsValid(LeverLabor) ? LeverLabor->QueryInteraction(Context) : FPlayerInteractionQuery();
}

FPlayerInteractionResult UUtilityLeverOperatingVolumeComponent::ExecuteInteraction(
	const FPlayerInteractionContext& Context)
{
	FText FailureReason;
	return IsValid(LeverLabor) && LeverLabor->TryStartStroke(Context, FailureReason)
		? FPlayerInteractionResult::Succeeded()
		: FPlayerInteractionResult::Failed(FailureReason.IsEmpty()
			? LOCTEXT("LeverUnavailable", "레버를 조작할 수 없습니다.") : FailureReason);
}

void UUtilityLeverOperatingVolumeComponent::NotifyInteractionFocusChanged(
	const UPlayerInteractionComponent& Source,
	const FPlayerInteractionQuery& Query)
{
	(void)Source;
	(void)Query;
}

void UUtilityLeverOperatingVolumeComponent::NotifyInteractionFocusEnded(
	const UPlayerInteractionComponent& Source)
{
	if (IsValid(LeverLabor))
	{
		LeverLabor->NotifySourceFocusEnded(Source);
	}
}

void UUtilityLeverOperatingVolumeComponent::SetLeverLabor(UUtilityLeverLaborComponent* InLeverLabor)
{
	LeverLabor = InLeverLabor;
	if (IsValid(LeverLabor))
	{
		LeverLabor->SetOperatingVolume(this);
	}
}

bool UUtilityLeverOperatingVolumeComponent::HasValidAuthoring(FText& OutFailureReason) const
{
	const FVector Extent = GetUnscaledBoxExtent();
	if (!FMath::IsFinite(Extent.X) || !FMath::IsFinite(Extent.Y) || !FMath::IsFinite(Extent.Z)
		|| Extent.X <= 0.0f || Extent.Y <= 0.0f || Extent.Z <= 0.0f
		|| !GetRelativeScale3D().Equals(FVector::OneVector, 1.0e-4f))
	{
		OutFailureReason = LOCTEXT("InvalidLeverExtent", "레버 조작 Box에는 양수 extent와 단위 상대 scale이 필요합니다.");
		return false;
	}
	const FBodyInstance* Body = GetBodyInstance();
	if (!Body || Body->GetCollisionEnabled(false) != ECollisionEnabled::QueryOnly
		|| GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block
		|| CanEverAffectNavigation())
	{
		OutFailureReason = LOCTEXT("InvalidLeverCollision", "레버 조작 Box는 QueryOnly Visibility Block이며 Navigation에 영향이 없어야 합니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
