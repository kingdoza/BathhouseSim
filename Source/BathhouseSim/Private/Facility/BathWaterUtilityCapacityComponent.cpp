#include "Facility/BathWaterUtilityCapacityComponent.h"

#include "Utility/UtilityOperationComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BathWaterUtilityCapacityComponent"

UBathWaterUtilityCapacityComponent::UBathWaterUtilityCapacityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UBathWaterUtilityCapacityComponent::GetCapacityPoints() const
{
	return FMath::IsFinite(CapacityPoints) ? FMath::Max(0.0f, CapacityPoints) : 0.0f;
}

float UBathWaterUtilityCapacityComponent::GetActiveCapacityPoints() const
{
	const float InstalledPoints = GetCapacityPoints();
	if (!IsValid(UtilityOperation))
	{
		return bOperationRequired ? 0.0f : InstalledPoints;
	}
	return UtilityOperation->IsProvidingCapacity() ? InstalledPoints : 0.0f;
}

void UBathWaterUtilityCapacityComponent::SetUtilityOperation(
	UUtilityOperationComponent* InOperation,
	const bool bRequireOperation)
{
	UtilityOperation = InOperation;
	bOperationRequired = bRequireOperation;
}

bool UBathWaterUtilityCapacityComponent::HasValidAuthoring(FText& OutFailureReason) const
{
	if ((bOperationRequired && !IsValid(UtilityOperation))
		|| !StaticEnum<EBathWaterCapacityKind>()->IsValidEnumValue(static_cast<int64>(CapacityKind))
		|| !FMath::IsFinite(CapacityPoints) || CapacityPoints < 0.0f)
	{
		OutFailureReason = LOCTEXT("InvalidCapacity", "설비 용량 종류와 값은 유효하고 음수가 아니어야 합니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

void UBathWaterUtilityCapacityComponent::RestoreCapacity(
	const EBathWaterCapacityKind InKind,
	const float InPoints)
{
	CapacityKind = InKind;
	CapacityPoints = InPoints;
}

#if WITH_EDITOR
EDataValidationResult UBathWaterUtilityCapacityComponent::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FText FailureReason;
	if (!HasValidAuthoring(FailureReason))
	{
		Context.AddError(FailureReason);
		Result = EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

#undef LOCTEXT_NAMESPACE
