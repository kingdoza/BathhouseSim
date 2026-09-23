#include "UI/BathWaterCapacitySummaryWidget.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

namespace
{
EBathWaterCapacityDisplayState GetDisplayState(const FBathWaterCapacitySnapshot& Snapshot)
{
	if (Snapshot.DeficitPoints > KINDA_SMALL_NUMBER) return EBathWaterCapacityDisplayState::Deficit;
	if (Snapshot.TotalPoints > KINDA_SMALL_NUMBER
		&& Snapshot.UsedPoints >= Snapshot.TotalPoints - KINDA_SMALL_NUMBER) return EBathWaterCapacityDisplayState::Full;
	return EBathWaterCapacityDisplayState::Normal;
}
}

void UBathWaterCapacitySummaryWidget::ApplyCapacitySnapshot(const FBathWaterOperationsSnapshot& Snapshot)
{
	ApplyCapacity(CirculationCapacityText, CirculationCapacityBar, CirculationCapacityStatusText,
		Snapshot.Circulation, CirculationCache, bHasCirculationCache);
	ApplyCapacity(HeatingCapacityText, HeatingCapacityBar, HeatingCapacityStatusText,
		Snapshot.Heating, HeatingCache, bHasHeatingCache);
	ApplyCapacity(CoolingCapacityText, CoolingCapacityBar, CoolingCapacityStatusText,
		Snapshot.Cooling, CoolingCache, bHasCoolingCache);
}

void UBathWaterCapacitySummaryWidget::ApplyCapacity(
	UTextBlock* Text, UProgressBar* Bar, UTextBlock* Status,
	const FBathWaterCapacitySnapshot& Snapshot, FBathWaterCapacitySnapshot& Cache, bool& bHasCache)
{
	if (bHasCache && FMath::IsNearlyEqual(Cache.TotalPoints, Snapshot.TotalPoints)
		&& FMath::IsNearlyEqual(Cache.UsedPoints, Snapshot.UsedPoints)
		&& FMath::IsNearlyEqual(Cache.DeficitPoints, Snapshot.DeficitPoints))
	{
		return;
	}
	const EBathWaterCapacityDisplayState State = GetDisplayState(Snapshot);
	const FString Value = State == EBathWaterCapacityDisplayState::Deficit
		? FString::Printf(TEXT("%.0f / %.0f  (-%.0f)"), Snapshot.UsedPoints, Snapshot.TotalPoints, Snapshot.DeficitPoints)
		: FString::Printf(TEXT("%.0f / %.0f"), Snapshot.UsedPoints, Snapshot.TotalPoints);
	if (Text) Text->SetText(FText::FromString(Value));
	if (Bar) Bar->SetPercent(Snapshot.TotalPoints > KINDA_SMALL_NUMBER
		? FMath::Clamp(Snapshot.UsedPoints / Snapshot.TotalPoints, 0.0f, 1.0f) : 0.0f);
	if (Status)
	{
		Status->SetText(State == EBathWaterCapacityDisplayState::Deficit
			? NSLOCTEXT("BathWaterUI", "CapacityDeficit", "부족")
			: State == EBathWaterCapacityDisplayState::Full
				? NSLOCTEXT("BathWaterUI", "CapacityFull", "가득 참")
				: NSLOCTEXT("BathWaterUI", "CapacityNormal", "정상"));
	}
	OnCapacityDisplayStateChanged(Snapshot.Kind, State);
	Cache = Snapshot;
	bHasCache = true;
#if WITH_DEV_AUTOMATION_TESTS
	++PresentationWriteCount;
#endif
}
