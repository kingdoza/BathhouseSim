#include "UI/BathWaterCapacitySummaryWidget.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

namespace
{
EBathWaterCapacityDisplayState GetDisplayState(const FBathWaterCapacitySnapshot& Snapshot)
{
	if (Snapshot.DeficitPoints > KINDA_SMALL_NUMBER
		|| Snapshot.InstalledDeficitPoints > KINDA_SMALL_NUMBER)
	{
		return EBathWaterCapacityDisplayState::Deficit;
	}
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
		&& FMath::IsNearlyEqual(Cache.ActivePoints, Snapshot.ActivePoints)
		&& FMath::IsNearlyEqual(Cache.UsedPoints, Snapshot.UsedPoints)
		&& FMath::IsNearlyEqual(Cache.DeficitPoints, Snapshot.DeficitPoints)
		&& FMath::IsNearlyEqual(Cache.InstalledDeficitPoints, Snapshot.InstalledDeficitPoints))
	{
		return;
	}
	const EBathWaterCapacityDisplayState State = GetDisplayState(Snapshot);
	const FString Value = FString::Printf(
		TEXT("예약 %.0f / 가동 %.0f / 설치 %.0f"),
		Snapshot.UsedPoints,
		Snapshot.ActivePoints,
		Snapshot.TotalPoints);
	if (Text) Text->SetText(FText::FromString(Value));
	if (Bar) Bar->SetPercent(Snapshot.TotalPoints > KINDA_SMALL_NUMBER
		? FMath::Clamp(Snapshot.UsedPoints / Snapshot.TotalPoints, 0.0f, 1.0f) : 0.0f);
	if (Status)
	{
		TArray<FString> DeficitMessages;
		if (Snapshot.InstalledDeficitPoints > KINDA_SMALL_NUMBER)
		{
			DeficitMessages.Add(FString::Printf(
				TEXT("설치 용량 %.0f 부족"), Snapshot.InstalledDeficitPoints));
		}
		if (Snapshot.DeficitPoints > KINDA_SMALL_NUMBER)
		{
			DeficitMessages.Add(FString::Printf(TEXT("가동 용량 %.0f 부족"), Snapshot.DeficitPoints));
		}
		if (DeficitMessages.Num() > 0)
		{
			Status->SetText(FText::FromString(FString::Join(DeficitMessages, TEXT(" · "))));
		}
		else
		{
			Status->SetText(State == EBathWaterCapacityDisplayState::Full
				? NSLOCTEXT("BathWaterUI", "CapacityFull", "예약 용량 사용 중")
				: NSLOCTEXT("BathWaterUI", "CapacityNormal", "정상"));
		}
	}
	OnCapacityDisplayStateChanged(Snapshot.Kind, State);
	Cache = Snapshot;
	bHasCache = true;
#if WITH_DEV_AUTOMATION_TESTS
	++PresentationWriteCount;
#endif
}
