#include "Facility/BathhouseExpansionDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BathhouseExpansion"

bool UBathhouseExpansionDefinition::TryGetPurchasePrice(const int32 PurchaseIndex, int32& OutPrice) const
{
	if (!PurchasePrices.IsValidIndex(PurchaseIndex) || PurchasePrices[PurchaseIndex] <= 0)
	{
		return false;
	}
	OutPrice = PurchasePrices[PurchaseIndex];
	return true;
}

int32 UBathhouseExpansionDefinition::GetHallEffectIndex(const int32 HallExpansionCount) const
{
	return Tiers.IsEmpty() ? INDEX_NONE : FMath::Clamp(HallExpansionCount, 0, Tiers.Num() - 1);
}

const FBathhouseExpansionTier* UBathhouseExpansionDefinition::GetHallEffect(const int32 HallExpansionCount) const
{
	const int32 Index = GetHallEffectIndex(HallExpansionCount);
	return Index == INDEX_NONE ? nullptr : &Tiers[Index];
}

void UBathhouseExpansionDefinition::CollectDataProblems(TArray<FText>& OutReasons) const
{
	if (Tiers.IsEmpty())
	{
		OutReasons.Add(LOCTEXT("MissingTiers", "효과 표(Tiers)에 줄이 하나 이상 있어야 합니다."));
		return;
	}
	int32 PreviousKeys = -1;
	int32 PreviousSlots = -1;
	for (int32 Index = 0; Index < Tiers.Num(); ++Index)
	{
		const FBathhouseExpansionTier& Tier = Tiers[Index];
		if (Tier.KeyPoolSize < Tier.MaxInstalledLockerSlots || Tier.KeyPoolSize < PreviousKeys || Tier.MaxInstalledLockerSlots < PreviousSlots)
		{
			OutReasons.Add(FText::Format(
				LOCTEXT("InvalidTier", "효과 표 {0}번 줄은 앞 줄보다 줄어들 수 없고 열쇠 수가 락커 칸 한도 이상이어야 합니다."),
				Index));
		}
		PreviousKeys = Tier.KeyPoolSize;
		PreviousSlots = Tier.MaxInstalledLockerSlots;
	}
	if (PurchasePrices.Num() < MaxPurchaseCount)
	{
		OutReasons.Add(FText::Format(
			LOCTEXT("TooFewPrices", "구입 가격 줄({0})이 전체 구입 횟수 상한({1})보다 적습니다."),
			PurchasePrices.Num(), MaxPurchaseCount));
	}
	for (int32 Index = 0; Index < FMath::Min(PurchasePrices.Num(), MaxPurchaseCount); ++Index)
	{
		if (PurchasePrices[Index] <= 0)
		{
			OutReasons.Add(FText::Format(
				LOCTEXT("InvalidPrice", "{0}번째 구입 가격은 0보다 커야 합니다."), Index + 1));
		}
	}
	if (Tiers.Num() < MaxPurchaseCount + 1)
	{
		OutReasons.Add(FText::Format(
			LOCTEXT("TooFewTiers", "효과 표 줄({0})이 전체 구입 횟수 상한 + 1({1})보다 적습니다."),
			Tiers.Num(), MaxPurchaseCount + 1));
	}
}

bool UBathhouseExpansionDefinition::ValidatePurchaseData(FText& OutFailureReason) const
{
	TArray<FText> Reasons;
	CollectDataProblems(Reasons);
	OutFailureReason = Reasons.IsEmpty() ? FText::GetEmpty() : Reasons[0];
	return Reasons.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UBathhouseExpansionDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	TArray<FText> Reasons;
	CollectDataProblems(Reasons);
	for (const FText& Reason : Reasons)
	{
		Context.AddError(Reason);
		Result = EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

#undef LOCTEXT_NAMESPACE
