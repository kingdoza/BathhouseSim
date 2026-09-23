#include "UI/BathWaterBathTileWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Facility/BathhouseBathFacilityActor.h"

namespace
{
FText MakeThermalStatusText(const EBathWaterThermalStatus Status)
{
	switch (Status)
	{
	case EBathWaterThermalStatus::Empty: return NSLOCTEXT("BathWaterUI", "StatusEmpty", "비어 있음");
	case EBathWaterThermalStatus::ReturningToAmbient: return NSLOCTEXT("BathWaterUI", "StatusAmbient", "실온 복귀");
	case EBathWaterThermalStatus::Stalled: return NSLOCTEXT("BathWaterUI", "StatusStalled", "정체");
	case EBathWaterThermalStatus::MovingToTarget: return NSLOCTEXT("BathWaterUI", "StatusMoving", "목표 이동");
	case EBathWaterThermalStatus::MaintainingTarget: return NSLOCTEXT("BathWaterUI", "StatusMaintaining", "목표 유지");
	case EBathWaterThermalStatus::SuspendedByCapacity: return NSLOCTEXT("BathWaterUI", "StatusSuspended", "용량 부족");
	default: return FText::GetEmpty();
	}
}

FText MakeCapacityStatusText(const FBathWaterBathSnapshot& Snapshot)
{
	TArray<FString, TInlineAllocator<3>> Deficits;
	if (Snapshot.bCirculationCapacityDeficit) Deficits.Add(TEXT("순환"));
	if (Snapshot.bHeatingCapacityDeficit) Deficits.Add(TEXT("가열"));
	if (Snapshot.bCoolingCapacityDeficit) Deficits.Add(TEXT("냉각"));
	return Deficits.IsEmpty()
		? NSLOCTEXT("BathWaterUI", "BathCapacityNormal", "용량 정상")
		: FText::FromString(FString::Printf(TEXT("%s 용량 부족"), *FString::Join(Deficits, TEXT("/"))));
}
}

void UBathWaterBathTileWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (SelectButton)
	{
		SelectButton->OnClicked.RemoveDynamic(this, &UBathWaterBathTileWidget::HandleSelected);
		SelectButton->OnClicked.AddDynamic(this, &UBathWaterBathTileWidget::HandleSelected);
	}
}

void UBathWaterBathTileWidget::NativeDestruct()
{
	if (SelectButton)
	{
		SelectButton->OnClicked.RemoveDynamic(this, &UBathWaterBathTileWidget::HandleSelected);
	}
	OnTileSelected.Clear();
	BathActor.Reset();
	bHasCachedSnapshot = false;
	Super::NativeDestruct();
}

void UBathWaterBathTileWidget::ApplyBathSnapshot(
	const FBathWaterBathSnapshot& Snapshot,
	const bool bSelected)
{
	const bool bUnchanged = bHasCachedSnapshot
		&& CachedSnapshot.BathActor == Snapshot.BathActor
		&& FMath::IsNearlyEqual(CachedSnapshot.ActualTemperatureC, Snapshot.ActualTemperatureC)
		&& FMath::IsNearlyEqual(CachedSnapshot.ContaminationPercent, Snapshot.ContaminationPercent)
		&& CachedSnapshot.ThermalStatus == Snapshot.ThermalStatus
		&& CachedSnapshot.bCirculationCapacityDeficit == Snapshot.bCirculationCapacityDeficit
		&& CachedSnapshot.bHeatingCapacityDeficit == Snapshot.bHeatingCapacityDeficit
		&& CachedSnapshot.bCoolingCapacityDeficit == Snapshot.bCoolingCapacityDeficit
		&& bCachedSelected == bSelected;
	if (bUnchanged)
	{
		return;
	}
	BathActor = Snapshot.BathActor;
	if (BathNameText) BathNameText->SetText(FText::FromString(BathActor.IsValid()
		? BathActor->GetActorNameOrLabel() : FString()));
	if (ActualTemperatureText) ActualTemperatureText->SetText(FText::FromString(
		FString::Printf(TEXT("%.1f °C"), Snapshot.ActualTemperatureC)));
	if (ContaminationText) ContaminationText->SetText(FText::FromString(
		FString::Printf(TEXT("%.1f%%"), Snapshot.ContaminationPercent)));
	if (ThermalStatusText) ThermalStatusText->SetText(MakeThermalStatusText(Snapshot.ThermalStatus));
	if (CapacityStatusText) CapacityStatusText->SetText(MakeCapacityStatusText(Snapshot));
	if (SelectButton)
	{
		const bool bCapacityDeficit = Snapshot.bCirculationCapacityDeficit
			|| Snapshot.bHeatingCapacityDeficit || Snapshot.bCoolingCapacityDeficit;
		SelectButton->SetBackgroundColor(bCapacityDeficit
			? FLinearColor(0.88f, 0.26f, 0.18f)
			: bSelected ? FLinearColor(0.24f, 0.88f, 0.96f)
			: FLinearColor(0.12f, 0.62f, 0.68f));
	}
	SetRenderOpacity(bSelected ? 1.0f : 0.85f);
	OnBathTileStateChanged(bSelected, Snapshot.bCirculationCapacityDeficit,
		Snapshot.bHeatingCapacityDeficit, Snapshot.bCoolingCapacityDeficit);
	CachedSnapshot = Snapshot;
	bHasCachedSnapshot = true;
	bCachedSelected = bSelected;
#if WITH_DEV_AUTOMATION_TESTS
	++PresentationWriteCount;
#endif
}

void UBathWaterBathTileWidget::HandleSelected()
{
	if (BathActor.IsValid())
	{
		OnTileSelected.Broadcast(BathActor.Get());
	}
}
