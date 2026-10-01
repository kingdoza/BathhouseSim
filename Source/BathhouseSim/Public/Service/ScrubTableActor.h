#pragma once
#include "Facility/BathhouseFacilityActor.h"
#include "TimerManager.h"
#include "ScrubTableActor.generated.h"
class UCameraComponent;
class UStaticMeshComponent;
class UPlayerScrubFocusComponent;
class ABathhouseCashPaymentActor;
DECLARE_MULTICAST_DELEGATE(FOnScrubSessionEnded);
UCLASS(Blueprintable)

class BATHHOUSESIM_API AScrubTableActor : public ABathhouseFacilityActor
{
	GENERATED_BODY()
public:

	AScrubTableActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	bool TryBeginScrubSession(UPlayerScrubFocusComponent& Scrubber);
	void EndScrubSession(UPlayerScrubFocusComponent& Scrubber);
	void AddRubDistance(UPlayerScrubFocusComponent& Scrubber, float Cm);
	bool HasOccupiedUser() const;
	UFUNCTION(BlueprintPure, Category = "Service")
	float GetScrubProgress() const;
	UFUNCTION(BlueprintPure, Category = "Service")
	float GetRemainingWaitSeconds() const;

	UBoxComponent* GetScrubArea() const
	{
		return ScrubArea;
	}

	UStaticMeshComponent* GetScrubCursor() const
	{
		return ScrubCursor;
	}

	FTransform GetExitFootTransform() const;

	float GetExitSearchRadiusCm() const
	{
		return ExitSearchRadiusCm;
	}

	float GetRubCmPerInputUnit() const
	{
		return RubCmPerInputUnit;
	}

	float GetFocusBlendInSeconds() const
	{
		return FocusBlendInSeconds;
	}

	float GetFocusBlendOutSeconds() const
	{
		return FocusBlendOutSeconds;
	}

	FOnScrubSessionEnded OnScrubSessionEnded;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Service")
	TObjectPtr<UCameraComponent> ScrubCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Service")
	TObjectPtr<UBoxComponent> ScrubArea;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Service")
	TObjectPtr<UStaticMeshComponent> ScrubCursor;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Service")
	TObjectPtr<USceneComponent> ScrubExitPoint;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Service")
	TObjectPtr<USceneComponent> CashOfferPoint;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Service")
	TObjectPtr<USceneComponent> CashStandPoint;
	UPROPERTY(EditDefaultsOnly, Category = "Service", meta = (ClampMin = "1"))
	int32 ScrubFee = 20000;
	UPROPERTY(EditDefaultsOnly, Category = "Service", meta = (ClampMin = "0.001"))
	float WaitLimitSeconds = 90;
	UPROPERTY(EditDefaultsOnly, Category = "Service", meta = (ClampMin = "0.001"))
	float RequiredRubDistanceCm = 3000;
	UPROPERTY(EditDefaultsOnly, Category = "Service", meta = (ClampMin = "0.001"))
	float RubCmPerInputUnit = 1;
	UPROPERTY(EditDefaultsOnly, Category = "Service", meta = (ClampMin = "0"))
	float ExitSearchRadiusCm = 100;
	UPROPERTY(EditDefaultsOnly, Category = "Service", meta = (ClampMin = "0"))
	float FocusBlendInSeconds = .35f;
	UPROPERTY(EditDefaultsOnly, Category = "Service", meta = (ClampMin = "0"))
	float FocusBlendOutSeconds = .25f;
	UPROPERTY(EditDefaultsOnly, Category = "Service")
	TSubclassOf<ABathhouseCashPaymentActor> CashOfferClass;

private:

	UFUNCTION()
	void HandleSlotChanged(UBathhouseFacilitySlotComponent* Slot, EBathhouseFacilitySlotState Previous,
						   EBathhouseFacilitySlotState State);
	UFUNCTION()
	void HandleUserEndPlay(AActor* User, EEndPlayReason::Type Reason);
	void BindUser(AActor* User);
	void ExpireWait();
	void CompleteScrub();
	void NotifySessionEnded();
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentUser;
	UPROPERTY(Transient)
	TWeakObjectPtr<UPlayerScrubFocusComponent> ActiveScrubber;
	UPROPERTY(Transient)
	double WaitDeadline = 0;
	UPROPERTY(Transient)
	float RubDistanceCm = 0;
	FTimerHandle WaitTimer;
	bool bCompleting = false;
};
