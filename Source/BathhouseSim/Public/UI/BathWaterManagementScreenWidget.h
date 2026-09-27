#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Computer/ComputerScreenContext.h"
#include "BathWaterManagementScreenWidget.generated.h"

class ABathhouseBathFacilityActor;
class AFacilityPlacementZoneActor;
class UBathWaterCapacitySummaryWidget;
class UBathWaterDetailWidget;
class UBathWaterMapWidget;
class UBathWaterOperationsSubsystem;

UCLASS()
class BATHHOUSESIM_API UBathWaterManagementScreenWidget : public UUserWidget, public IComputerScreenContextReceiver
{
	GENERATED_BODY()

public:
	virtual void InitializeComputerScreen(const FComputerScreenContext& Context) override;
	virtual void NotifyComputerUserChanged(APlayerState* PlayerState) override { (void)PlayerState; }

	void InitializeManagementContext(
		UBathWaterOperationsSubsystem* InOperations,
		AFacilityPlacementZoneActor* InZone);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBathWaterCapacitySummaryWidget> CapacitySummary;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBathWaterMapWidget> BathMap;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBathWaterDetailWidget> BathDetail;

private:
	void BindContext();
	void UnbindContext();
	void HandleOperationsChanged();
	void HandleSelectionChanged(ABathhouseBathFacilityActor* Bath);
	void RefreshSnapshot();

	TWeakObjectPtr<UBathWaterOperationsSubsystem> Operations;
	TWeakObjectPtr<AFacilityPlacementZoneActor> PlacementZone;
	TWeakObjectPtr<ABathhouseBathFacilityActor> SelectedBath;
	FDelegateHandle OperationsChangedHandle;
};
