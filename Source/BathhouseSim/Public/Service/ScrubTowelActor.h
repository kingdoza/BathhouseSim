#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PhysicalCarryable.h"
#include "Interaction/PlayerInteractable.h"
#include "ScrubTowelActor.generated.h"

class UPlayerCarryComponent;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScrubTowelHeldPresentationChanged, bool, bIsHeld);

UCLASS(Blueprintable)

class BATHHOUSESIM_API AScrubTowelActor : public AActor, public IPlayerInteractable, public IPhysicalCarryable
{
	GENERATED_BODY()

public:

	AScrubTowelActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;

	virtual EPhysicalCarryKind GetPhysicalCarryKind() const override
	{
		return EPhysicalCarryKind::ScrubTowel;
	}

	virtual FText GetPhysicalCarryDisplayName() const override;
	virtual FTransform GetHeldTransform() const override;
	virtual bool CanBeTakenBy(const UPlayerCarryComponent& Carry, FText& OutFailureReason) const override;
	virtual bool HandleTakenBy(UPlayerCarryComponent& Carry, USceneComponent* HeldAnchor) override;
	virtual bool CanFreeDrop(FText& OutFailureReason) const override;
	virtual UPrimitiveComponent* GetPhysicalCarryPrimitive() const override;

	virtual float GetThrowSpawnDistance() const override
	{
		return ThrowSpawnDistance;
	}

	virtual float GetThrowImpulseStrength() const override
	{
		return ThrowImpulseStrength;
	}

	virtual float GetUpwardThrowImpulseStrength() const override
	{
		return UpwardThrowImpulseStrength;
	}

	virtual AActor* GetAssignedPhysicalCarryFixedSlot() const override
	{
		return FixedSlot.Get();
	}

	virtual bool TryBindPhysicalCarryFixedSlot(AActor& SlotActor, FText& OutFailureReason) override;
	virtual void ClearPhysicalCarryFixedSlotBinding(AActor& ExpectedSlot) override;

	virtual void NotifyPhysicalCarryFixedSlotBindingConflict() override
	{
		bFixedSlotBindingConflict = true;
	}

	virtual bool IsStoredInAssignedPhysicalCarryFixedSlot() const override;
	virtual bool NotifyTakenFromFixedSlotCommitted(UPlayerCarryComponent& Carry, AActor& SlotActor) override;
	virtual bool NotifyStoredInFixedSlotCommitted(UPlayerCarryComponent& Carry, AActor& SlotActor) override;
	virtual bool NotifyRecoveredToFixedSlotCommitted(AActor& SlotActor) override;
	virtual void NotifyFixedSlotDestroyed(AActor& SlotActor) override;
	virtual bool NotifyPhysicalDropCommitted(UPlayerCarryComponent& Carry) override;
	virtual void PublishPhysicalCarryCommit(EPhysicalCarryCommitTransition Transition) override;
	virtual void RecoverPhysicalCarryable(UPlayerCarryComponent* PreviousCarry) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(BlueprintAssignable, Category = "Service|Presentation")
	FOnScrubTowelHeldPresentationChanged OnHeldPresentationChanged;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Service")
	TObjectPtr<UStaticMeshComponent> WorldMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Service|Carry", meta = (ClampMin = "0.0"))
	float ThrowImpulseStrength = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Service|Carry",
			  meta = (ClampMin = "0.0", DeprecatedProperty,
					  DeprecationMessage = "Held-position free drop no longer uses a camera-origin spawn distance."))
	float ThrowSpawnDistance = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Service|Carry", meta = (ClampMin = "0.0"))
	float UpwardThrowImpulseStrength = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service|Carry|Presentation")
	FTransform HeldTransform = FTransform::Identity;

private:

	void ApplyHeldTransform();
	void SetWorldPhysics(bool bEnabled);

	UPROPERTY(Transient)
	TObjectPtr<UPlayerCarryComponent> Carrier = nullptr;

	TWeakObjectPtr<AActor> FixedSlot;

	FTransform InitialTransform;
	FTransform LastSafeTransform;
	bool bEndingPlay = false;
	bool bFixedSlotBindingConflict = false;
};
