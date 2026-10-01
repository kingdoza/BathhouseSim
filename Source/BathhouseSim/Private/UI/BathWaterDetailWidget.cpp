#include "UI/BathWaterDetailWidget.h"

#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Facility/BathWaterOperationsSubsystem.h"
#include "Facility/BathWaterSettings.h"
#include "Facility/BathhouseBathFacilityActor.h"

namespace
{
void SetNumber(UTextBlock* Text, const TCHAR* Label, const float Value, const TCHAR* Suffix = TEXT(""))
{
	if (Text) Text->SetText(FText::FromString(FString::Printf(TEXT("%s %.1f%s"), Label, Value, Suffix)));
}

FText DetailStatusText(const EBathWaterThermalStatus Status)
{
	switch (Status)
	{
	case EBathWaterThermalStatus::Empty: return NSLOCTEXT("BathWaterUI", "DetailEmpty", "비어 있음");
	case EBathWaterThermalStatus::ReturningToAmbient: return NSLOCTEXT("BathWaterUI", "DetailAmbient", "실온 복귀");
	case EBathWaterThermalStatus::Stalled: return NSLOCTEXT("BathWaterUI", "DetailStalled", "정체");
	case EBathWaterThermalStatus::MovingToTarget: return NSLOCTEXT("BathWaterUI", "DetailMoving", "목표 이동");
	case EBathWaterThermalStatus::MaintainingTarget: return NSLOCTEXT("BathWaterUI", "DetailMaintaining", "목표 유지");
	case EBathWaterThermalStatus::SuspendedByCapacity: return NSLOCTEXT("BathWaterUI", "DetailSuspended", "용량 부족");
	default: return FText::GetEmpty();
	}
}

FText CapacityDeficitText(const FBathWaterBathSnapshot& Snapshot)
{
	TArray<FString, TInlineAllocator<3>> Kinds;
	if (Snapshot.bCirculationCapacityDeficit) Kinds.Add(TEXT("순환"));
	if (Snapshot.bHeatingCapacityDeficit) Kinds.Add(TEXT("가열"));
	if (Snapshot.bCoolingCapacityDeficit) Kinds.Add(TEXT("냉각"));
	return Kinds.IsEmpty()
		? NSLOCTEXT("BathWaterUI", "DetailCapacityNormal", "용량 정상")
		: FText::FromString(FString::Printf(TEXT("%s 용량 부족"), *FString::Join(Kinds, TEXT("/"))));
}

float NormalizeCirculation(const float Percent)
{
	return Percent * 0.01f;
}

float NormalizeTargetTemperature(const float TemperatureC)
{
	const UBathWaterSettings* Settings = GetDefault<UBathWaterSettings>();
	const float Min = Settings->GetMinTargetTemperatureC();
	const float Range = FMath::Max(Settings->GetMaxTargetTemperatureC() - Min, 0.001f);
	return FMath::Clamp((TemperatureC - Min) / Range, 0.0f, 1.0f);
}

const TCHAR* CapacityKindName(const EBathWaterCapacityKind Kind)
{
	switch (Kind)
	{
	case EBathWaterCapacityKind::Heating: return TEXT("가열");
	case EBathWaterCapacityKind::Cooling: return TEXT("냉각");
	default: return TEXT("순환");
	}
}
}

void UBathWaterDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (CirculationSlider)
	{
		CirculationSlider->OnValueChanged.RemoveDynamic(this, &UBathWaterDetailWidget::HandleCirculationChanged);
		CirculationSlider->OnValueChanged.AddDynamic(this, &UBathWaterDetailWidget::HandleCirculationChanged);
	}
	if (TargetTemperatureSlider)
	{
		TargetTemperatureSlider->OnValueChanged.RemoveDynamic(this, &UBathWaterDetailWidget::HandleTargetTemperatureChanged);
		TargetTemperatureSlider->OnValueChanged.AddDynamic(this, &UBathWaterDetailWidget::HandleTargetTemperatureChanged);
	}
}

void UBathWaterDetailWidget::NativeDestruct()
{
	if (CirculationSlider) CirculationSlider->OnValueChanged.RemoveDynamic(this, &UBathWaterDetailWidget::HandleCirculationChanged);
	if (TargetTemperatureSlider) TargetTemperatureSlider->OnValueChanged.RemoveDynamic(this, &UBathWaterDetailWidget::HandleTargetTemperatureChanged);
	Operations.Reset();
	SelectedBath.Reset();
	FeedbackBath.Reset();
	bHasCachedSnapshot = false;
	bHasTransientFeedback = false;
	Super::NativeDestruct();
}

void UBathWaterDetailWidget::SetOperationsContext(UBathWaterOperationsSubsystem* InOperations)
{
	if (Operations.Get() != InOperations)
	{
		FeedbackBath.Reset();
		bHasTransientFeedback = false;
		bHasCachedSnapshot = false;
		bShowingEmptyState = false;
	}
	Operations = InOperations;
}

void UBathWaterDetailWidget::ApplyBathSnapshot(const FBathWaterBathSnapshot* Snapshot)
{
	TGuardValue<bool> Guard(bApplyingSnapshot, true);
	const TWeakObjectPtr<ABathhouseBathFacilityActor> PreviousBath = SelectedBath;
	SelectedBath = Snapshot ? Snapshot->BathActor : nullptr;
	const bool bShouldEnable = Snapshot != nullptr && Operations.IsValid();
	if (CirculationSlider && CirculationSlider->GetIsEnabled() != bShouldEnable)
	{
		CirculationSlider->SetIsEnabled(bShouldEnable);
	}
	if (TargetTemperatureSlider && TargetTemperatureSlider->GetIsEnabled() != bShouldEnable)
	{
		TargetTemperatureSlider->SetIsEnabled(bShouldEnable);
	}
	const ESlateVisibility FieldVisibility = Snapshot ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	for (UTextBlock* Field : {BathNameText.Get(), WaterAmountText.Get(), ActualTemperatureText.Get(),
		TargetTemperatureText.Get(), ContaminationText.Get(), CirculationText.Get(),
		CirculationDemandText.Get(), HeatingDemandText.Get(), CoolingDemandText.Get(),
		ThermalStatusText.Get(), ThermalThresholdText.Get(), CapacityStatusText.Get()})
	{
		if (Field && Field->GetVisibility() != FieldVisibility)
		{
			Field->SetVisibility(FieldVisibility);
		}
	}
	if (!Snapshot)
	{
		if (bShowingEmptyState)
		{
			return;
		}
		bHasCachedSnapshot = false;
		bHasTransientFeedback = false;
		FeedbackBath.Reset();
		if (BathNameText) BathNameText->SetText(FText::GetEmpty());
		if (WaterAmountText) WaterAmountText->SetText(FText::GetEmpty());
		if (ActualTemperatureText) ActualTemperatureText->SetText(FText::GetEmpty());
		if (TargetTemperatureText) TargetTemperatureText->SetText(FText::GetEmpty());
		if (ContaminationText) ContaminationText->SetText(FText::GetEmpty());
		if (CirculationText) CirculationText->SetText(FText::GetEmpty());
		if (CirculationDemandText) CirculationDemandText->SetText(FText::GetEmpty());
		if (HeatingDemandText) HeatingDemandText->SetText(FText::GetEmpty());
		if (CoolingDemandText) CoolingDemandText->SetText(FText::GetEmpty());
		if (ThermalStatusText) ThermalStatusText->SetText(FText::GetEmpty());
		if (ThermalThresholdText) ThermalThresholdText->SetText(FText::GetEmpty());
		if (CapacityStatusText) CapacityStatusText->SetText(FText::GetEmpty());
		if (FeedbackText) FeedbackText->SetText(NSLOCTEXT("BathWaterUI", "NoBathSelected", "욕탕을 선택하세요"));
		bShowingEmptyState = true;
		return;
	}
	bShowingEmptyState = false;
	if (PreviousBath != Snapshot->BathActor)
	{
		bHasTransientFeedback = false;
		FeedbackBath.Reset();
		if (FeedbackText) FeedbackText->SetText(FText::GetEmpty());
	}
	else if (bHasTransientFeedback && FeedbackBath == Snapshot->BathActor && Snapshot->Revision > FeedbackRevision)
	{
		bHasTransientFeedback = false;
		FeedbackBath.Reset();
		if (FeedbackText) FeedbackText->SetText(FText::GetEmpty());
	}
	SyncSlidersToSnapshot(*Snapshot);
	if (bHasCachedSnapshot && CachedSnapshot.BathActor == Snapshot->BathActor
		&& FMath::IsNearlyEqual(CachedSnapshot.WaterPercent, Snapshot->WaterPercent)
		&& FMath::IsNearlyEqual(CachedSnapshot.CirculationPercent, Snapshot->CirculationPercent)
		&& FMath::IsNearlyEqual(CachedSnapshot.TargetTemperatureC, Snapshot->TargetTemperatureC)
		&& FMath::IsNearlyEqual(CachedSnapshot.ActualTemperatureC, Snapshot->ActualTemperatureC)
		&& FMath::IsNearlyEqual(CachedSnapshot.ContaminationPercent, Snapshot->ContaminationPercent)
		&& FMath::IsNearlyEqual(CachedSnapshot.CirculationDemandPoints, Snapshot->CirculationDemandPoints)
		&& FMath::IsNearlyEqual(CachedSnapshot.HeatingDemandPoints, Snapshot->HeatingDemandPoints)
		&& FMath::IsNearlyEqual(CachedSnapshot.CoolingDemandPoints, Snapshot->CoolingDemandPoints)
		&& FMath::IsNearlyEqual(CachedSnapshot.ThermalThresholdPercent, Snapshot->ThermalThresholdPercent)
		&& CachedSnapshot.ThermalStatus == Snapshot->ThermalStatus
		&& CachedSnapshot.bCirculationCapacityDeficit == Snapshot->bCirculationCapacityDeficit
		&& CachedSnapshot.bHeatingCapacityDeficit == Snapshot->bHeatingCapacityDeficit
		&& CachedSnapshot.bCoolingCapacityDeficit == Snapshot->bCoolingCapacityDeficit)
	{
		return;
	}
	if (BathNameText) BathNameText->SetText(FText::FromString(Snapshot->BathActor.IsValid()
		? Snapshot->BathActor->GetActorNameOrLabel() : FString()));
	SetNumber(WaterAmountText, TEXT("수위"), Snapshot->WaterPercent, TEXT("%"));
	SetNumber(ActualTemperatureText, TEXT("현재 수온"), Snapshot->ActualTemperatureC, TEXT(" °C"));
	SetNumber(TargetTemperatureText, TEXT("목표 수온"), Snapshot->TargetTemperatureC, TEXT(" °C"));
	SetNumber(ContaminationText, TEXT("오염도"), Snapshot->ContaminationPercent, TEXT("%"));
	SetNumber(CirculationText, TEXT("순환도"), Snapshot->CirculationPercent, TEXT("%"));
	SetNumber(CirculationDemandText, TEXT("순환 요구"), Snapshot->CirculationDemandPoints);
	SetNumber(HeatingDemandText, TEXT("가열 요구"), Snapshot->HeatingDemandPoints);
	SetNumber(CoolingDemandText, TEXT("냉각 요구"), Snapshot->CoolingDemandPoints);
	SetNumber(ThermalThresholdText, TEXT("임계 순환도"), Snapshot->ThermalThresholdPercent, TEXT("%"));
	if (ThermalStatusText) ThermalStatusText->SetText(FText::FromString(
		FString::Printf(TEXT("수온 상태: %s"), *DetailStatusText(Snapshot->ThermalStatus).ToString())));
	if (CapacityStatusText) CapacityStatusText->SetText(FText::FromString(
		FString::Printf(TEXT("용량: %s"), *CapacityDeficitText(*Snapshot).ToString())));
	if (TargetTemperatureSlider)
	{
		const UBathWaterSettings* Settings = GetDefault<UBathWaterSettings>();
		const float Range = FMath::Max(Settings->GetMaxTargetTemperatureC() - Settings->GetMinTargetTemperatureC(), 0.001f);
		const float Step = Settings->GetTargetTemperatureStepC() / Range;
		if (!FMath::IsNearlyEqual(TargetTemperatureSlider->GetStepSize(), Step))
		{
			TargetTemperatureSlider->SetStepSize(Step);
#if WITH_DEV_AUTOMATION_TESTS
			++SliderWriteCount;
#endif
		}
	}
	CachedSnapshot = *Snapshot;
	bHasCachedSnapshot = true;
#if WITH_DEV_AUTOMATION_TESTS
	++PresentationWriteCount;
#endif
}

void UBathWaterDetailWidget::WriteSliderValueSilently(USlider* Slider, const float NormalizedValue)
{
	if (!Slider || FMath::IsNearlyEqual(Slider->GetValue(), NormalizedValue))
	{
		return;
	}
	// USlider::SetValue re-broadcasts OnValueChanged; the guard keeps this write from becoming a new request.
	TGuardValue<bool> Guard(bWritingSliderValue, true);
	Slider->SetValue(NormalizedValue);
#if WITH_DEV_AUTOMATION_TESTS
	++SliderWriteCount;
#endif
}

void UBathWaterDetailWidget::SyncSlidersToSnapshot(const FBathWaterBathSnapshot& Snapshot)
{
	WriteSliderValueSilently(CirculationSlider, NormalizeCirculation(Snapshot.CirculationPercent));
	WriteSliderValueSilently(TargetTemperatureSlider, NormalizeTargetTemperature(Snapshot.TargetTemperatureC));
}

void UBathWaterDetailWidget::ResyncSliderAfterRequest(const bool bCirculation, const FBathWaterSettingRequestResult& Result)
{
	USlider* Slider = bCirculation ? CirculationSlider.Get() : TargetTemperatureSlider.Get();
	const auto Normalize = [bCirculation](const float Value)
	{
		return bCirculation ? NormalizeCirculation(Value) : NormalizeTargetTemperature(Value);
	};
	if (Result.bSucceeded)
	{
		WriteSliderValueSilently(Slider, Normalize(Result.CommittedValue));
		return;
	}
	FBathWaterBathSnapshot Current;
	if (Operations.IsValid() && SelectedBath.IsValid() && Operations->GetBathSnapshot(SelectedBath.Get(), Current))
	{
		WriteSliderValueSilently(Slider, Normalize(bCirculation ? Current.CirculationPercent : Current.TargetTemperatureC));
	}
	else if (bHasCachedSnapshot && CachedSnapshot.BathActor == SelectedBath)
	{
		WriteSliderValueSilently(Slider, Normalize(bCirculation ? CachedSnapshot.CirculationPercent : CachedSnapshot.TargetTemperatureC));
	}
}

void UBathWaterDetailWidget::HandleCirculationChanged(const float Value)
{
	if (!bApplyingSnapshot && !bWritingSliderValue && Operations.IsValid() && SelectedBath.IsValid())
	{
		const FBathWaterSettingRequestResult Result = Operations->RequestCirculationPercent(SelectedBath.Get(), Value * 100.0f);
		ApplyRequestFeedback(Result);
		ResyncSliderAfterRequest(true, Result);
	}
}

void UBathWaterDetailWidget::HandleTargetTemperatureChanged(const float Value)
{
	if (!bApplyingSnapshot && !bWritingSliderValue && Operations.IsValid() && SelectedBath.IsValid())
	{
		const UBathWaterSettings* Settings = GetDefault<UBathWaterSettings>();
		const float Requested = FMath::Lerp(
			Settings->GetMinTargetTemperatureC(),
			Settings->GetMaxTargetTemperatureC(),
			FMath::Clamp(Value, 0.0f, 1.0f));
		const FBathWaterSettingRequestResult Result = Operations->RequestTargetTemperature(SelectedBath.Get(), Requested);
		ApplyRequestFeedback(Result);
		ResyncSliderAfterRequest(false, Result);
	}
}

void UBathWaterDetailWidget::ApplyRequestFeedback(const FBathWaterSettingRequestResult& Result)
{
	if (!FeedbackText)
	{
		return;
	}
	if (!Result.bSucceeded)
	{
		FeedbackText->SetText(NSLOCTEXT("BathWaterUI", "RequestUnavailable", "설정을 변경할 수 없습니다"));
		FeedbackBath = SelectedBath;
		FeedbackRevision = Operations.IsValid() ? Operations->GetDataRevision() : 0;
		bHasTransientFeedback = true;
	}
	else if (Result.bWasLimited)
	{
		FeedbackText->SetText(FText::FromString(FString::Printf(
			TEXT("%s 용량 제한: %.1f 포인트 부족"), CapacityKindName(Result.LimitedKind), Result.RequiredAdditionalPoints)));
		FeedbackBath = SelectedBath;
		FeedbackRevision = Operations.IsValid() ? Operations->GetDataRevision() : 0;
		bHasTransientFeedback = true;
	}
	else
	{
		FeedbackText->SetText(FText::GetEmpty());
		FeedbackBath.Reset();
		bHasTransientFeedback = false;
	}
}
