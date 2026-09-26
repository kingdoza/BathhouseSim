#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utility/UtilityFuelTypes.h"
#include "UtilityOperationComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnUtilityOperationChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUtilityOperatingChanged, bool);

UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UUtilityOperationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUtilityOperationComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	float GetRemainingPoints() const;
	FUtilityOperationSnapshot GetOperationSnapshot() const;
	bool IsOperating() const { return GetRemainingPoints() > 0.0f; }
	bool IsProvidingCapacity() const;
	bool IsPlacedClockActive() const { return bPlacedClockActive; }
	bool IsLaborBlocked() const { return bLaborBlocked; }
	bool HasImportedOperationState() const { return bOperationStateImported; }
	float GetMaximumPoints() const { return MaxOperationPoints; }
	float GetDecayPointsPerSecond() const { return DecayPointsPerSecond; }
	bool HasValidAuthoring(FText& OutFailureReason) const;
	bool CanAcceptFuel(const FUtilityFuelLoad& Load, FText& OutFailureReason) const;
	bool ApplyLaborReward(float Points, FText& OutFailureReason);
	bool ImportOperationState(float RemainingPoints, FText& OutFailureReason);
	bool StartPlacedClock(bool bPublishOperatingTransition = true);
	void StopPlacedClock(bool bPublishOperatingTransition = true);
	void SetLaborBlocked(bool bBlocked) { bLaborBlocked = bBlocked; }

	static float CalculateRemainingPoints(float AnchorPoints, float DecayRate, double ElapsedSeconds);

	FOnUtilityOperationChanged OnOperationChanged;
	FOnUtilityOperatingChanged OnOperatingChanged;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Utility Operation", meta = (ClampMin = "0.01"))
	float MaxOperationPoints = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Utility Operation", meta = (ClampMin = "0.0"))
	float DecayPointsPerSecond = 1.0f;

private:
	friend class FUtilityFuelTransaction;

	double GetGameTimeSeconds() const;
	float GetRemainingPointsAt(double GameTimeSeconds) const;
	float SettleToCurrentTimeSilently(double& OutGameTimeSeconds);
	bool TryAcquireMutationGuard();
	void ReleaseMutationGuard();
	void SetRemainingPointsSilently(float Points, double GameTimeSeconds);
	void PublishCommittedChanges(float PreviousPoints, bool bPreviousOperating);
	void UpdateTickEnabled();

	float AnchorPoints = 0.0f;
	double AnchorGameTimeSeconds = 0.0;
	float LastPublishedPoints = 0.0f;
	int64 Revision = 0;
	bool bPlacedClockActive = false;
	bool bLaborBlocked = false;
	bool bOperationStateImported = false;
	bool bMutationInProgress = false;
	bool bLastPublishedOperating = false;
};
