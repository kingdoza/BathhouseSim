#pragma once

#include "CoreMinimal.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Service/ServiceItemTypes.h"
#include "DrinkFridgeActor.generated.h"

class UBathhouseFacilitySlotComponent;
class UDisplaySpaceComponent;
class UServiceItemDefinition;

/** Facility with display spaces and one customer slot. Recovery keeps stock through UDrinkFridgePlacementInstanceData. */
UCLASS(Blueprintable)
class BATHHOUSESIM_API ADrinkFridgeActor : public ABathhouseFacilityActor
{
	GENERATED_BODY()

public:
	ADrinkFridgeActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	virtual bool IsAvailableForReservation() const override;
	virtual bool TryBeginFacilityRecoveryHold(FText& OutFailureReason) override;
	virtual void CancelFacilityRecoveryHold() override;

	/**
	 * Shared layout rule for instances and Blueprint templates: at least one space, SpaceIndex unique and contiguous
	 * from 0, valid space authoring, and exactly one customer slot.
	 */
	static bool ValidateSpaceLayout(
		TConstArrayView<const UDisplaySpaceComponent*> Spaces,
		int32 CustomerSlotCount,
		FText& OutFailureReason);

	/** Spaces sorted by SpaceIndex. Fails when indices are not unique/contiguous from 0 or authoring is invalid. */
	bool CollectSpaces(TArray<UDisplaySpaceComponent*>& OutSpaces, FText& OutFailureReason) const;
	int32 GetTotalStock() const;
	bool IsRecoveryHoldActive() const { return bRecoveryHoldActive; }

	/** The slot's current user takes one drink from the first non-empty space and its sale is recorded. */
	bool TryTakeDrinkForCustomer(AActor& Customer, UServiceItemDefinition*& OutKind, FText& OutFailureReason);

protected:
	virtual UBathhouseFacilityPlacementInstanceData* CreateFacilityPlacementInstanceData(UObject* Outer) const override;
	virtual bool ExportFacilityExtension(UBathhouseFacilityPlacementInstanceData& Data, FText& OutFailureReason) const override;
	virtual bool ImportFacilityExtension(const UBathhouseFacilityPlacementInstanceData* Data, FText& OutFailureReason) override;

private:
	UFUNCTION()
	void HandleFridgeSlotStateChanged(
		UBathhouseFacilitySlotComponent* Slot,
		EBathhouseFacilitySlotState PreviousState,
		EBathhouseFacilitySlotState NewState);

	UFUNCTION()
	void HandleTrackedUserEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	UFUNCTION()
	void HandleSpaceStockChanged(const FDisplaySpaceSnapshot& Snapshot);

	void BindSpaces();
	void UnbindSpaces();
	void TrackUser(AActor* User);
	bool IsPlacedDomainOperational() const;
	UBathhouseFacilitySlotComponent* GetCustomerSlot() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDisplaySpaceComponent>> BoundSpaces;

	TWeakObjectPtr<AActor> TrackedUser;
	bool bRecoveryHoldActive = false;
};
