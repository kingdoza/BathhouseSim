#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PhysicalCarryable.h"
#include "Interaction/HeldEquipmentUsable.h"
#include "Interaction/HeldEquipmentSecondaryUsable.h"
#include "Interaction/PlayerInteractable.h"
#include "LitterTongsActor.generated.h"

class UPlayerCarryComponent;
class USceneComponent;
class UStaticMeshComponent;
class ALitterActor;
class ATrashBagActor;
struct FTrashBagDropRequest;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLitterTongsHeldPresentationChanged, bool, bIsHeld);

UCLASS(Blueprintable)

class BATHHOUSESIM_API ALitterTongsActor : public AActor,
										   public IPlayerInteractable,
										   public IPhysicalCarryable,
										   public IHeldEquipmentUsable,
										   public IHeldEquipmentSecondaryUsable
{
	GENERATED_BODY()

public:

	ALitterTongsActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;

	virtual EPhysicalCarryKind GetPhysicalCarryKind() const override
	{
		return EPhysicalCarryKind::LitterTongs;
	}

	virtual FText GetPhysicalCarryDisplayName() const override;
	virtual FText GetHeldSummaryText() const override;

	int32 GetBagCount() const
	{
		return BagCount;
	}

	// Fills the tie drop request: camera values from the use context, every tuning value from this CDO.
	// The only reader of the Tie* tuning properties; the execution path and automation both use it.
	void BuildTieDropRequest(const FHeldEquipmentUseContext& Context, FTrashBagDropRequest& OutRequest) const;

	virtual FHeldEquipmentUseQuery QuerySecondaryEquipmentUse(const FHeldEquipmentUseContext& Context) const override;
	virtual FHeldEquipmentUseResult ExecuteSecondaryEquipmentUse(const FHeldEquipmentUseContext& Context) override;
	virtual FTransform GetHeldTransform() const override;
	virtual bool CanBeTakenBy(const UPlayerCarryComponent& Carry, FText& OutFailureReason) const override;
	virtual bool HandleTakenBy(UPlayerCarryComponent& Carry, USceneComponent* HeldAnchor) override;
	virtual bool CanFreeDrop(FText& OutFailureReason) const override;
	virtual UPrimitiveComponent* GetPhysicalCarryPrimitive() const override;

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
	virtual FHeldEquipmentUseQuery QueryEquipmentUse(const FHeldEquipmentUseContext& Context) const override;
	virtual FHeldEquipmentUseResult BeginEquipmentUse(const FHeldEquipmentUseContext& Context) override;
	virtual FHeldEquipmentUseUpdate UpdateEquipmentUse(const FHeldEquipmentUseContext& Context,
													   float DeltaTime) override;
	virtual FHeldEquipmentUseResult EndEquipmentUse(const FHeldEquipmentUseContext& Context) override;
	virtual void CancelEquipmentUse(const FHeldEquipmentUseContext& Context) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(BlueprintAssignable, Category = "Cleaning|Presentation")
	FOnLitterTongsHeldPresentationChanged OnHeldPresentationChanged;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter Tongs", meta = (ClampMin = "1"))
	int32 BagCapacity = 20;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter Tongs")
	TSubclassOf<ATrashBagActor> TiedBagClass;
	/** 1단계(시선 앞) 시작 거리: 카메라에서 봉투의 가장 가까운 부분까지(시선 방향) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter Tongs", meta = (ClampMin = "0.0"))
	float TieViewDistanceCm = 60.0f;
	/** 1단계 시선 당김 하한 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter Tongs", meta = (ClampMin = "0.0"))
	float TieViewMinDistanceCm = 30.0f;
	/** 1단계 시선 당김 간격 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter Tongs", meta = (ClampMin = "0.01"))
	float TieViewPullStepCm = 10.0f;
	/** 2단계(바닥 정면)의 발바닥 기준 수평 거리 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter Tongs", meta = (ClampMin = "0.0"))
	float TieForwardDistanceCm = 60;
	/** 2단계 당김 하한 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter Tongs", meta = (ClampMin = "0.0"))
	float TieMinForwardDistanceCm = 30;
	/** 2단계 당김 간격 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter Tongs", meta = (ClampMin = "0.01"))
	float TieForwardPullStepCm = 10.0f;
	/** 2단계 봉투 밑면의 발바닥 위 높이 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter Tongs", meta = (ClampMin = "0.0"))
	float TieFloorClearanceCm = 5.0f;
	/** 바닥 정면 단계 봉투와 카메라의 최소 분리 여유 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter Tongs", meta = (ClampMin = "0.0"))
	float TieCameraClearanceCm = 10.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cleaning")
	TObjectPtr<UStaticMeshComponent> WorldMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning|Carry", meta = (ClampMin = "0.0"))
	float ThrowImpulseStrength = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning|Carry", meta = (ClampMin = "0.0"))
	float UpwardThrowImpulseStrength = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning|Carry|Presentation")
	FTransform HeldTransform = FTransform::Identity;

private:

	UPROPERTY(Transient)
	int32 BagCount = 0;

	void ApplyHeldTransform();
	void SetWorldPhysics(bool bEnabled);

	UPROPERTY(Transient)
	TObjectPtr<UPlayerCarryComponent> Carrier = nullptr;

	TWeakObjectPtr<AActor> FixedSlot;

	FTransform LastSafeTransform;
	bool bEndingPlay = false;
	bool bFixedSlotBindingConflict = false;
};
