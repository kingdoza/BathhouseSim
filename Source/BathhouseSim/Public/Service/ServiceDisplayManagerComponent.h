#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Facility/FacilityPlacementExtension.h"
#include "Facility/BathhouseFacilityTypes.h"
#include "Service/ServiceItemTypes.h"
#include "ServiceDisplayManagerComponent.generated.h"
class UDisplaySpaceComponent;
class UBathhouseFacilitySlotComponent;
UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))

class BATHHOUSESIM_API UServiceDisplayManagerComponent : public UActorComponent, public IFacilityPlacementExtension
{
	GENERATED_BODY()
public:

	UServiceDisplayManagerComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	virtual FName GetPlacementExtensionKey() const override
	{
		return TEXT("ServiceDisplay");
	}

	virtual UFacilityPlacementExtensionData* ExportPlacementExtension(UObject* Outer) const override;
	virtual bool ValidatePlacementExtension(const UFacilityPlacementExtensionData* Data, FText& Failure) const override;
	virtual void ApplyPlacementExtension(const UFacilityPlacementExtensionData* Data) override;
	virtual bool ValidateExtensionAuthoring(TConstArrayView<UActorComponent*> Components,
											FText& Failure) const override;
	bool CollectSpaces(TArray<UDisplaySpaceComponent*>& Out, FText& Failure) const;
	static bool ValidateSpaceLayout(TConstArrayView<const UDisplaySpaceComponent*> Spaces, int32 SlotCount,
									int32 RequiredSlots, int32 RouterCount, FText& Failure);
	int32 GetTotalStock() const;
	UFUNCTION(BlueprintPure, Category = "Service Display")

	float GetCustomerUseSeconds() const
	{
		return CustomerUseSeconds;
	}
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service Display", meta = (ClampMin = "1"))
	int32 RequiredCustomerSlotCount = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service Display")
	bool bConsumeOnCustomerUseStart = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service Display", meta = (ClampMin = "0"))
	float CustomerUseSeconds = 0;

private:

	void RefreshTrackedUsers();
	UFUNCTION()
	void HandleSlotChanged(UBathhouseFacilitySlotComponent* Slot, EBathhouseFacilitySlotState Previous,
						   EBathhouseFacilitySlotState Current);
	UFUNCTION()
	void HandleUserEndPlay(AActor* Actor, EEndPlayReason::Type Reason);
	UFUNCTION()
	void HandleStockChanged(const FDisplaySpaceSnapshot& Snapshot);
	UPROPERTY(Transient)
	TArray<TObjectPtr<UDisplaySpaceComponent>> BoundSpaces;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBathhouseFacilitySlotComponent>> BoundSlots;
	TArray<TWeakObjectPtr<AActor>> TrackedUsers;
	TSet<TWeakObjectPtr<UBathhouseFacilitySlotComponent>> ConsumedReservationSlots;
};
