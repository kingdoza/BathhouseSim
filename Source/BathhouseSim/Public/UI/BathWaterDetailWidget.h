#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "BathWaterDetailWidget.generated.h"

class ABathhouseBathFacilityActor;
class UBathWaterOperationsSubsystem;
class USlider;
class UTextBlock;

UCLASS()
class BATHHOUSESIM_API UBathWaterDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetOperationsContext(UBathWaterOperationsSubsystem* InOperations);
	void ApplyBathSnapshot(const FBathWaterBathSnapshot* Snapshot);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> WaterAmountText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> BathNameText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ActualTemperatureText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> TargetTemperatureText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ContaminationText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CirculationText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CirculationDemandText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> HeatingDemandText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CoolingDemandText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ThermalStatusText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ThermalThresholdText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> FeedbackText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CapacityStatusText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> CirculationSlider;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> TargetTemperatureSlider;

private:
	UFUNCTION() void HandleCirculationChanged(float Value);
	UFUNCTION() void HandleTargetTemperatureChanged(float Value);
	void ApplyRequestFeedback(const FBathWaterSettingRequestResult& Result);

	TWeakObjectPtr<UBathWaterOperationsSubsystem> Operations;
	TWeakObjectPtr<ABathhouseBathFacilityActor> SelectedBath;
	TWeakObjectPtr<ABathhouseBathFacilityActor> FeedbackBath;
	FBathWaterBathSnapshot CachedSnapshot;
	uint64 FeedbackRevision = 0;
	bool bHasCachedSnapshot = false;
	bool bHasTransientFeedback = false;
	bool bShowingEmptyState = false;
	bool bApplyingSnapshot = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend class FBathWaterOperationsUIWidgetTest;
	int32 PresentationWriteCount = 0;
	int32 SliderWriteCount = 0;
#endif
};
