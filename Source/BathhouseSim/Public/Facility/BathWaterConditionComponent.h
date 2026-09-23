#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "Facility/BathWaterStateComponent.h"
#include "BathWaterConditionComponent.generated.h"

class UBathWaterOperationsSubsystem;
class UBathWaterStateComponent;
class UCustomerSessionComponent;

DECLARE_MULTICAST_DELEGATE(FOnBathWaterConditionChangedNative);

UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UBathWaterConditionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBathWaterConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	float GetCirculationPercent() const { return CirculationPercent; }
	float GetTargetTemperatureC() const { return TargetTemperatureC; }
	float GetActualTemperatureC() const { return ActualTemperatureC; }
	float GetContaminationPercent() const { return ContaminationPercent; }
	float GetMaxCirculationDemandPoints() const { return GetSafeNonNegative(MaxCirculationDemandPoints); }
	float GetHeatingDemandPointsPerC() const { return GetSafeNonNegative(HeatingDemandPointsPerC); }
	float GetCoolingDemandPointsPerC() const { return GetSafeNonNegative(CoolingDemandPointsPerC); }
	float GetThermalThresholdPercent() const;
	float GetDemandPoints(EBathWaterCapacityKind Kind) const;
	EBathWaterThermalStatus GetThermalStatus() const;
	int32 GetActiveBatherCount();
	bool IsRecoveryFrozen() const { return bRecoveryFrozen; }

	bool RegisterActiveBather(UCustomerSessionComponent* Session);
	bool UnregisterActiveBather(UCustomerSessionComponent* Session);
	void ResetForPlacement();
	bool BeginRecoveryFreeze(FText& OutFailureReason);
	void CancelRecoveryFreeze();
	void PrepareRecoveryCommit();
	bool HasValidAuthoring(FText& OutFailureReason) const;

	FOnBathWaterConditionChangedNative OnConditionChangedNative;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water|Capacity", meta = (ClampMin = "0.0"))
	float MaxCirculationDemandPoints = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water|Capacity", meta = (ClampMin = "0.0"))
	float HeatingDemandPointsPerC = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water|Capacity", meta = (ClampMin = "0.0"))
	float CoolingDemandPointsPerC = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water|Condition", meta = (ClampMin = "0.0"))
	float CleaningRateAtFullCirculationPercentPointsPerSecond = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water|Condition", meta = (ClampMin = "0.0"))
	float ContaminationPerBatherPercentPointsPerSecond = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water|Temperature", meta = (ClampMin = "0.0"))
	float MaxTargetControlRateCPerSecond = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water|Temperature", meta = (ClampMin = "0.0"))
	float NaturalReturnRateCPerSecond = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bath Water|Temperature", meta = (ClampMin = "0.000001"))
	float TemperatureEpsilonC = 0.001f;

private:
	friend class UBathWaterOperationsSubsystem;
	friend class FBathWaterOperationsFlowAndConditionTest;
	friend class FBathWaterOperationsFacilityTransactionTest;

	struct FRecoverySnapshot
	{
		float Circulation = 0.0f;
		float Target = 20.0f;
		float Actual = 20.0f;
		float Contamination = 0.0f;
		bool bTickEnabled = false;
	};

	static float GetSafeNonNegative(float Value);
	void SetCirculationPercentFromSubsystem(float NewPercent);
	void SetTargetTemperatureFromSubsystem(float NewTargetC);
	void EnsureWaterStateBound();
	void HandleFlowStep(const FBathWaterFlowStep& Step);
	void HandleWaterAmountChanged(float PreviousAmount, float CurrentAmount);
	void RefreshTickState();
	void IntegrateTemperature(float DeltaTime);
	float ComputeThermalVelocity(float TemperatureC, bool bActiveAvailable) const;
	bool IsCirculationCapacitySatisfied() const;
	bool IsThermalCapacitySatisfied() const;
	void BroadcastIfChanged(float OldActual, float OldContamination);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Bath Water|Condition", meta = (AllowPrivateAccess = "true"))
	float CirculationPercent = 0.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Bath Water|Condition", meta = (AllowPrivateAccess = "true"))
	float TargetTemperatureC = 20.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Bath Water|Condition", meta = (AllowPrivateAccess = "true"))
	float ActualTemperatureC = 20.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Bath Water|Condition", meta = (AllowPrivateAccess = "true"))
	float ContaminationPercent = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UBathWaterStateComponent> WaterState = nullptr;
	TSet<TWeakObjectPtr<UCustomerSessionComponent>> ActiveBathers;
	TOptional<FRecoverySnapshot> RecoverySnapshot;
	FDelegateHandle FlowStepHandle;
	FDelegateHandle AmountChangedHandle;
	bool bRecoveryFrozen = false;
};
