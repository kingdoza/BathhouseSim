#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "BathWaterCapacitySummaryWidget.generated.h"

class UTextBlock;
class UProgressBar;

UENUM(BlueprintType)
enum class EBathWaterCapacityDisplayState : uint8
{
	Normal,
	Full,
	Deficit
};

UCLASS()
class BATHHOUSESIM_API UBathWaterCapacitySummaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ApplyCapacitySnapshot(const FBathWaterOperationsSnapshot& Snapshot);

	UFUNCTION(BlueprintImplementableEvent, Category = "Bath Water")
	void OnCapacityDisplayStateChanged(EBathWaterCapacityKind Kind, EBathWaterCapacityDisplayState State);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CirculationCapacityText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HeatingCapacityText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CoolingCapacityText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> CirculationCapacityBar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> HeatingCapacityBar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> CoolingCapacityBar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CirculationCapacityStatusText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> HeatingCapacityStatusText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CoolingCapacityStatusText;

private:
	void ApplyCapacity(UTextBlock* Text, UProgressBar* Bar, UTextBlock* Status,
		const FBathWaterCapacitySnapshot& Snapshot, FBathWaterCapacitySnapshot& Cache, bool& bHasCache);
	FBathWaterCapacitySnapshot CirculationCache;
	FBathWaterCapacitySnapshot HeatingCache;
	FBathWaterCapacitySnapshot CoolingCache;
	bool bHasCirculationCache = false;
	bool bHasHeatingCache = false;
	bool bHasCoolingCache = false;
#if WITH_DEV_AUTOMATION_TESTS
	friend class FBathWaterOperationsUIWidgetTest;
	int32 PresentationWriteCount = 0;
#endif
};
