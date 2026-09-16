#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BathWaterStateComponent.generated.h"

UENUM(BlueprintType)
enum class EBathWaterState : uint8
{
	Empty,
	Filling,
	Filled,
	Draining,
	Holding
};

UENUM(BlueprintType)
enum class EBathWaterControlType : uint8
{
	FillValve,
	DrainLever
};

UENUM(BlueprintType)
enum class EBathWaterControlChangeReason : uint8
{
	PlayerInteraction,
	FullAutoClose,
	RecoveryCommit,
	Reset
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnBathWaterStateChanged,
	EBathWaterState, PreviousState,
	EBathWaterState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnBathWaterAmountChanged,
	float, PreviousNormalizedAmount,
	float, NewNormalizedAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBathCustomerUsabilityChanged, bool, bCustomerUsable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnBathWaterControlChanged,
	EBathWaterControlType, ControlType,
	bool, bOpen,
	EBathWaterControlChangeReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBathWaterRecoveryFreezeChanged, bool, bFrozen);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBathWaterAmountChangedNative, float, float);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBathWaterStateChangedNative, EBathWaterState, EBathWaterState);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBathCustomerUsabilityChangedNative, bool);
DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FOnBathWaterControlChangedNative,
	EBathWaterControlType,
	bool,
	EBathWaterControlChangeReason);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBathWaterRecoveryFreezeChangedNative, bool);

UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UBathWaterStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBathWaterStateComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UFUNCTION(BlueprintPure, Category = "Bath Water")
	EBathWaterState GetWaterState() const { return WaterState; }

	UFUNCTION(BlueprintPure, Category = "Bath Water")
	bool IsEmpty() const { return NormalizedAmount == 0.0f; }

	UFUNCTION(BlueprintCallable, Category = "Bath Water",
		meta = (DeprecatedFunction, DeprecationMessage = "Use the native bath-water request API."))
	bool SetWaterState(EBathWaterState NewState);

	UFUNCTION(BlueprintPure, Category = "Bath Water")
	float GetNormalizedAmount() const { return NormalizedAmount; }

	UFUNCTION(BlueprintPure, Category = "Bath Water")
	float GetWaterPercent() const { return NormalizedAmount * 100.0f; }

	UFUNCTION(BlueprintCallable, Category = "Bath Water",
		meta = (DeprecatedFunction, DeprecationMessage = "Use the native bath-water request API."))
	void SetNormalizedAmount(float NewAmount);

	UFUNCTION(BlueprintPure, Category = "Bath Water")
	bool IsCustomerUsable() const;

	UFUNCTION(BlueprintPure, Category = "Bath Water")
	bool IsControlOpen(EBathWaterControlType ControlType) const;

	UFUNCTION(BlueprintPure, Category = "Bath Water")
	bool IsRecoveryFrozen() const { return bRecoveryFrozen; }

	float GetFillRatePercentPerSecond() const;
	float GetDrainRatePercentPerSecond() const;
	bool CanSetControlOpen(EBathWaterControlType ControlType, bool bOpen, FText& OutFailureReason) const;
	bool RequestSetControlOpen(
		EBathWaterControlType ControlType,
		bool bOpen,
		EBathWaterControlChangeReason Reason,
		FText& OutFailureReason);
	void ResetEmptyForPlacement();
	bool BeginRecoveryFreeze(FText& OutFailureReason);
	void CancelRecoveryFreeze();
	void PrepareRecoveryCommit();

	UPROPERTY(BlueprintAssignable, Category = "Bath Water|Presentation")
	FOnBathWaterStateChanged OnWaterStateChanged;
	UPROPERTY(BlueprintAssignable, Category = "Bath Water|Presentation")
	FOnBathWaterAmountChanged OnWaterAmountChanged;
	UPROPERTY(BlueprintAssignable, Category = "Bath Water|Presentation")
	FOnBathCustomerUsabilityChanged OnCustomerUsabilityChanged;
	UPROPERTY(BlueprintAssignable, Category = "Bath Water|Presentation")
	FOnBathWaterControlChanged OnControlChanged;
	UPROPERTY(BlueprintAssignable, Category = "Bath Water|Presentation")
	FOnBathWaterRecoveryFreezeChanged OnRecoveryFreezeChanged;

	FOnBathWaterAmountChangedNative OnWaterAmountChangedNative;
	FOnBathWaterStateChangedNative OnWaterStateChangedNative;
	FOnBathCustomerUsabilityChangedNative OnCustomerUsabilityChangedNative;
	FOnBathWaterControlChangedNative OnControlChangedNative;
	FOnBathWaterRecoveryFreezeChangedNative OnRecoveryFreezeChangedNative;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water|Flow", meta = (ClampMin = "0.0"))
	float FillRatePercentPerSecond = 6.666667f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water|Flow", meta = (ClampMin = "0.0"))
	float DrainRatePercentPerSecond = 10.0f;

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Bath Water")
	EBathWaterState WaterState = EBathWaterState::Empty;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Bath Water",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NormalizedAmount = 0.0f;

private:
	struct FRecoverySnapshot
	{
		float Amount = 0.0f;
		EBathWaterState State = EBathWaterState::Empty;
		bool bFillOpen = false;
		bool bDrainOpen = false;
		bool bCustomerUsable = false;
		bool bTickEnabled = false;
	};

	float GetNetPercentPerSecond() const;
	EBathWaterState DeriveWaterState() const;
	void CommitAmount(float NewAmount);
	void CommitDerivedState();
	void CommitControlState(EBathWaterControlType ControlType, bool bOpen, EBathWaterControlChangeReason Reason);
	void RefreshTickState();
	void BroadcastUsabilityIfChanged();
	void SetRecoveryFrozen(bool bFrozen);

	TOptional<FRecoverySnapshot> RecoverySnapshot;
	bool bFillValveOpen = false;
	bool bDrainLeverOpen = false;
	bool bCachedCustomerUsable = false;
	bool bRecoveryFrozen = false;
};
