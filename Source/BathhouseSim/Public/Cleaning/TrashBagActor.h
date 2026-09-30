#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PhysicalCarryable.h"
#include "Interaction/PhysicalCarryDiscardable.h"
#include "Interaction/PlayerInteractable.h"
#include "TrashBagActor.generated.h"
class UStaticMeshComponent;
UCLASS(Blueprintable)

class BATHHOUSESIM_API ATrashBagActor : public AActor,
										public IPlayerInteractable,
										public IPhysicalCarryable,
										public IPhysicalCarryDiscardable
{
	GENERATED_BODY()
public:

	ATrashBagActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;
	bool InitializeCount(int32 Count);
	bool ActivateFreeWorld(const FTransform& Transform, FText& Failure);

	bool IsFreeWorld() const
	{
		return Lifecycle == ELifecycle::FreeWorld;
	}

	int32 GetLitterCount() const
	{
		return LitterCount;
	}

	FText GetBagSummary() const;
	static ATrashBagActor* SpawnTiedBag(UWorld& World, TSubclassOf<ATrashBagActor> BagClass, int32 Count,
										const FTransform& Transform);
	static bool BuildClassCollisionQuery(TSubclassOf<ATrashBagActor> BagClass, const FTransform& WorldTransform,
										 FVector& OutLocation, FQuat& OutRotation, FCollisionShape& OutShape,
										 const UPrimitiveComponent*& OutCollisionTemplate, FText& OutFailureReason);
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;

	virtual EPhysicalCarryKind GetPhysicalCarryKind() const override
	{
		return EPhysicalCarryKind::TrashBag;
	}

	virtual EPhysicalCarryCapability GetPhysicalCarryCapabilities() const override
	{
		return EPhysicalCarryCapability::FreeDrop;
	}

	virtual FText GetPhysicalCarryDisplayName() const override
	{
		return GetBagSummary();
	}

	virtual FText GetHeldSummaryText() const override
	{
		return GetBagSummary();
	}

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

	virtual bool NotifyPhysicalDropCommitted(UPlayerCarryComponent& Carry) override;
	virtual void PublishPhysicalCarryCommit(EPhysicalCarryCommitTransition Transition) override;
	virtual void RecoverPhysicalCarryable(UPlayerCarryComponent* PreviousCarry) override;
	virtual bool CanDiscardCarriedObject(FText& OutFailureReason) const override;
	virtual void HandleDiscardCommitted() override;
	virtual bool CanDiscardFromWorld(FText& OutFailureReason) const override;
	virtual void HandleDiscardFromWorldCommitted() override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trash Bag")
	TObjectPtr<UStaticMeshComponent> BagMesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry|Presentation")
	FTransform HeldTransform = FTransform::Identity;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry", meta = (ClampMin = "0.0"))
	float ThrowImpulseStrength = 120;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry", meta = (ClampMin = "0.0"))
	float UpwardThrowImpulseStrength = 15;

private:

	enum class ELifecycle : uint8
	{
		Staged,
		FreeWorld,
		Held,
		Consumed
	};

	bool CanDiscardKind() const
	{
		return bCountInitialized;
	}

	void SetFreeWorldPhysics(bool bEnabled);
	UPROPERTY(Transient)
	int32 LitterCount = 0;
	bool bCountInitialized = false;
	TWeakObjectPtr<UPlayerCarryComponent> Carrier;
	FTransform LastSafeTransform = FTransform::Identity;
	ELifecycle Lifecycle = ELifecycle::Staged;
};
