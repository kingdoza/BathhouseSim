#pragma once

#include "CoreMinimal.h"
#include "Facility/BathWaterStateComponent.h"
#include "Facility/BathhouseFacilityActor.h"
#include "BathhouseBathFacilityActor.generated.h"

class UBathWaterControlComponent;
class UNiagaraComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class BATHHOUSESIM_API ABathhouseBathFacilityActor : public ABathhouseFacilityActor
{
	GENERATED_BODY()

public:
	ABathhouseBathFacilityActor();

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool IsAvailableForReservation() const override;
	virtual bool TryBeginFacilityRecoveryHold(FText& OutFailureReason) override;
	virtual void CancelFacilityRecoveryHold() override;
	virtual bool StagePlacedDomainUnregistration(FFacilityPlacementPublication& OutPublication, FText& OutFailureReason) override;
	virtual bool RollbackPlacedDomainUnregistration(FText& OutFailureReason) override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UFUNCTION(BlueprintPure, Category = "Bath Water|Components")
	UBathWaterControlComponent* GetFillValveControl() const { return FillValveControl; }

	UFUNCTION(BlueprintPure, Category = "Bath Water|Components")
	UBathWaterControlComponent* GetDrainLeverControl() const { return DrainLeverControl; }

	UFUNCTION(BlueprintPure, Category = "Bath Water|Components")
	USceneComponent* GetWaterSurfaceMover() const { return WaterSurfaceMover; }

	UFUNCTION(BlueprintPure, Category = "Bath Water|Components")
	UStaticMeshComponent* GetWaterSurfaceMesh() const { return WaterSurfaceMesh; }

	UFUNCTION(BlueprintPure, Category = "Bath Water|Components")
	USceneComponent* GetWaterLevelEmptyPoint() const { return WaterLevelEmptyPoint; }

	UFUNCTION(BlueprintPure, Category = "Bath Water|Components")
	USceneComponent* GetWaterLevelFullPoint() const { return WaterLevelFullPoint; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Bath Water|Presentation")
	void OnBathWaterAmountChanged(float PreviousPercent, float CurrentPercent);

	UFUNCTION(BlueprintImplementableEvent, Category = "Bath Water|Presentation")
	void OnBathCustomerUsabilityChanged(bool bUsable);

	UFUNCTION(BlueprintImplementableEvent, Category = "Bath Water|Presentation")
	void OnFillFlowPresentationChanged(bool bActive);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water|Control")
	TObjectPtr<UBathWaterControlComponent> FillValveControl;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water|Control")
	TObjectPtr<UBathWaterControlComponent> DrainLeverControl;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water|Presentation")
	TObjectPtr<UNiagaraComponent> FillFlowNiagara;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water|Presentation")
	TObjectPtr<USceneComponent> WaterPresentationRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water|Presentation")
	TObjectPtr<USceneComponent> WaterSurfaceMover;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water|Presentation")
	TObjectPtr<UStaticMeshComponent> WaterSurfaceMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water|Presentation")
	TObjectPtr<USceneComponent> WaterLevelEmptyPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water|Presentation")
	TObjectPtr<USceneComponent> WaterLevelFullPoint;

private:
	void InitializeBathWaterRuntime();
	void UnbindBathWaterDelegates();
	void HandleWaterAmountChanged(float PreviousAmount, float CurrentAmount);
	void HandleCustomerUsabilityChanged(bool bUsable);
	void HandleControlChanged(EBathWaterControlType ControlType, bool bOpen, EBathWaterControlChangeReason Reason);
	void HandleRecoveryFreezeChanged(bool bFrozen);
	void UpdateWaterSurface();
	void UpdateFillFlow();
	void RefreshReservationAvailability(bool bPublishChange);

	FDelegateHandle AmountChangedHandle;
	FDelegateHandle UsabilityChangedHandle;
	FDelegateHandle ControlChangedHandle;
	FDelegateHandle RecoveryFreezeChangedHandle;
	bool bBathRuntimeInitialized = false;
	bool bCachedReservationAvailability = false;
	bool bRecoveryHoldActive = false;
	bool bRecoveryCommitPending = false;
	bool bRecoveryFlowWasActive = false;
};
