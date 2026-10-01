#pragma once
#include "Facility/BathhouseFacilityActor.h"
#include "Combat/WrenchRepairable.h"
#include "TimerManager.h"
#include "MassageChairActor.generated.h"
UCLASS(Blueprintable)

class BATHHOUSESIM_API AMassageChairActor : public ABathhouseFacilityActor, public IWrenchRepairable
{
	GENERATED_BODY()
public:

	AMassageChairActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;
	virtual bool IsAvailableForReservation() const override;
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	virtual bool IsWrenchRepairRequired() const override;

	virtual float GetWrenchRepairSeconds() const override
	{
		return RepairSeconds;
	}

	virtual bool CommitWrenchRepair(FText& OutFailure) override;
	virtual bool StagePlacedDomainUnregistration(FFacilityPlacementPublication& Publication, FText& Failure) override;
	UFUNCTION(BlueprintPure, Category = "Service")

	int32 GetCoinBalance() const
	{
		return CoinBalance;
	}
	UFUNCTION(BlueprintPure, Category = "Service")

	bool IsBroken() const
	{
		return bBroken;
	}
	UFUNCTION(BlueprintImplementableEvent, Category = "Service")
	void OnBrokenStateChanged(bool Broken);
	UFUNCTION(BlueprintImplementableEvent, Category = "Service")
	void OnCoinBalanceChanged(int32 Amount);

	static bool ShouldBreak(float RollPercent, float ChancePercent)
	{
		return RollPercent < ChancePercent;
	}
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
protected:

	UPROPERTY(EditDefaultsOnly, Category = "Service", meta = (ClampMin = "0.001"))
	float UseSeconds = 60.f;
	UPROPERTY(EditDefaultsOnly, Category = "Service", meta = (ClampMin = "0"))
	int32 UseFee = 3000;
	UPROPERTY(EditDefaultsOnly, Category = "Service", meta = (ClampMin = "0", ClampMax = "100"))
	float BreakChancePercent = 10.f;
	UPROPERTY(EditDefaultsOnly, Category = "Service", meta = (ClampMin = "0.001"))
	float RepairSeconds = 3.f;
	virtual UBathhouseFacilityPlacementInstanceData* CreateFacilityPlacementInstanceData(UObject* Outer) const override;
	virtual bool ExportFacilityExtension(UBathhouseFacilityPlacementInstanceData& Data, FText& Failure) const override;
	virtual bool ImportFacilityExtension(const UBathhouseFacilityPlacementInstanceData* Data, FText& Failure) override;

private:

	UFUNCTION()
	void HandleSlotChanged(UBathhouseFacilitySlotComponent* Slot, EBathhouseFacilitySlotState Previous,
						   EBathhouseFacilitySlotState State);
	UFUNCTION()
	void HandleUserEndPlay(AActor* User, EEndPlayReason::Type Reason);
	void CompleteUse();
	void BindUser(AActor* User);
	void PublishAvailability();
	bool IsPlacedActive() const;
	UPROPERTY(Transient)
	int32 CoinBalance = 0;
	UPROPERTY(Transient)
	bool bBroken = false;
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentUser;
	FTimerHandle UseTimer;
	bool bCollecting = false;
};
