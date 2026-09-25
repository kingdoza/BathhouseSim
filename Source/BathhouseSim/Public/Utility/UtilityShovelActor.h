#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/HeldEquipmentUsable.h"
#include "Interaction/PhysicalCarryable.h"
#include "Interaction/PlayerInteractable.h"
#include "Utility/UtilityFuelTypes.h"
#include "UtilityShovelActor.generated.h"

class UPlayerCarryComponent;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUtilityShovelLoadChanged, FUtilityFuelLoad, Load);

UCLASS(Blueprintable)
class BATHHOUSESIM_API AUtilityShovelActor
	: public AActor
	, public IPlayerInteractable
	, public IPhysicalCarryable
	, public IHeldEquipmentUsable
{
	GENERATED_BODY()

public:
	AUtilityShovelActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	virtual EPhysicalCarryKind GetPhysicalCarryKind() const override { return EPhysicalCarryKind::Shovel; }
	virtual EPhysicalCarryCapability GetPhysicalCarryCapabilities() const override
	{
		return EPhysicalCarryCapability::FreeDrop | EPhysicalCarryCapability::FixedSlot;
	}
	virtual FText GetPhysicalCarryDisplayName() const override;
	virtual FTransform GetHeldTransform() const override;
	virtual bool CanBeTakenBy(const UPlayerCarryComponent& Carry, FText& OutFailureReason) const override;
	virtual bool HandleTakenBy(UPlayerCarryComponent& Carry, USceneComponent* HeldAnchor) override;
	virtual bool CanFreeDrop(FText& OutFailureReason) const override;
	virtual UPrimitiveComponent* GetPhysicalCarryPrimitive() const override;
	virtual AActor* GetAssignedPhysicalCarryFixedSlot() const override { return FixedSlot.Get(); }
	virtual bool TryBindPhysicalCarryFixedSlot(AActor& SlotActor, FText& OutFailureReason) override;
	virtual void ClearPhysicalCarryFixedSlotBinding(AActor& ExpectedSlot) override;
	virtual void NotifyPhysicalCarryFixedSlotBindingConflict() override { bFixedSlotBindingConflict = true; }
	virtual bool IsStoredInAssignedPhysicalCarryFixedSlot() const override;
	virtual bool NotifyTakenFromFixedSlotCommitted(UPlayerCarryComponent& Carry, AActor& SlotActor) override;
	virtual bool NotifyStoredInFixedSlotCommitted(UPlayerCarryComponent& Carry, AActor& SlotActor) override;
	virtual bool NotifyRecoveredToFixedSlotCommitted(AActor& SlotActor) override;
	virtual void NotifyFixedSlotDestroyed(AActor& SlotActor) override;
	virtual bool NotifyPhysicalDropCommitted(UPlayerCarryComponent& Carry) override;
	virtual void PublishPhysicalCarryCommit(EPhysicalCarryCommitTransition Transition) override;
	virtual void RecoverPhysicalCarryable(UPlayerCarryComponent* PreviousCarry) override;
	virtual FHeldEquipmentUseQuery QueryEquipmentUse(const FHeldEquipmentUseContext& Context) const override;
	virtual FHeldEquipmentUseResult BeginEquipmentUse(const FHeldEquipmentUseContext& Context) override;
	virtual FHeldEquipmentUseUpdate UpdateEquipmentUse(const FHeldEquipmentUseContext& Context, float DeltaTime) override;
	virtual FHeldEquipmentUseResult EndEquipmentUse(const FHeldEquipmentUseContext& Context) override;
	virtual void CancelEquipmentUse(const FHeldEquipmentUseContext& Context) override;

	FUtilityFuelLoad GetFuelLoad() const { return FuelLoad; }
	bool IsLoadEmpty() const { return FuelLoad.IsEmpty(); }
	bool HasValidAuthoring(FText& OutFailureReason) const;
	bool CanAcceptLoad(const FUtilityFuelLoad& Load, FText& OutFailureReason) const;
	UStaticMeshComponent* GetWorldMesh() const { return WorldMesh; }

	UPROPERTY(BlueprintAssignable, Category = "Utility|Fuel")
	FOnUtilityShovelLoadChanged OnFuelLoadChanged;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Shovel")
	TObjectPtr<UStaticMeshComponent> WorldMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Shovel")
	TObjectPtr<UStaticMeshComponent> LoadVisual;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Utility|Carry|Presentation")
	FTransform HeldTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Utility|Carry", meta = (ClampMin = "0.0"))
	float ThrowImpulseStrength = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Utility|Carry", meta = (ClampMin = "0.0"))
	float UpwardThrowImpulseStrength = 15.0f;

private:
	friend class FUtilityFuelTransaction;

	bool TryAcquireMutationGuard();
	void ReleaseMutationGuard();
	void SetFuelLoadSilently(const FUtilityFuelLoad& NewLoad);
	void PublishFuelLoadChanged();
	void ApplyLoadPresentation();
	void SetWorldPhysics(bool bEnabled);
	void ApplyHeldTransform();

	UPROPERTY(Transient)
	TObjectPtr<UPlayerCarryComponent> Carrier = nullptr;

	UPROPERTY(Transient)
	FUtilityFuelLoad FuelLoad;

	TWeakObjectPtr<AActor> FixedSlot;
	FTransform LastSafeTransform;
	int64 FuelLoadRevision = 0;
	bool bFuelMutationInProgress = false;
	bool bEndingPlay = false;
	bool bFixedSlotBindingConflict = false;
};
