#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "BathWaterBathTileWidget.generated.h"

class ABathhouseBathFacilityActor;
class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBathWaterTileSelected, ABathhouseBathFacilityActor*);

UCLASS()
class BATHHOUSESIM_API UBathWaterBathTileWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ApplyBathSnapshot(const FBathWaterBathSnapshot& Snapshot, bool bSelected);
	ABathhouseBathFacilityActor* GetBathActor() const { return BathActor.Get(); }
	FOnBathWaterTileSelected OnTileSelected;
	UFUNCTION(BlueprintImplementableEvent, Category = "Bath Water")
	void OnBathTileStateChanged(bool bSelected, bool bCirculationDeficit, bool bHeatingDeficit, bool bCoolingDeficit);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SelectButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BathNameText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActualTemperatureText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ContaminationText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ThermalStatusText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CapacityStatusText;

private:
	UFUNCTION()
	void HandleSelected();

	TWeakObjectPtr<ABathhouseBathFacilityActor> BathActor;
	FBathWaterBathSnapshot CachedSnapshot;
	bool bHasCachedSnapshot = false;
	bool bCachedSelected = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend class FBathWaterOperationsUIWidgetTest;
	int32 PresentationWriteCount = 0;
#endif
};
