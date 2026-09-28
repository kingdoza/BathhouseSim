#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "Interaction/HeldEquipmentUsable.h"
#include "Interaction/PhysicalCarryable.h"
#include "Interaction/PhysicalCarryDiscardable.h"
#include "Interaction/PlayerInteractable.h"
#include "Shop/ShopTypes.h"
#include "ShopDeliveryBoxActor.generated.h"

class UPlayerCarryComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class USceneComponent;

UCLASS(Blueprintable)
class BATHHOUSESIM_API AShopDeliveryBoxActor
	: public AActor
	, public IPlayerInteractable
	, public IPhysicalCarryable
	, public IHeldEquipmentUsable
	, public IPhysicalCarryDiscardable
{
	GENERATED_BODY()

public:
	AShopDeliveryBoxActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool InitializeContents(int64 InOrderId, const TArray<FShopOrderLine>& InContents);
	bool ActivateFreeWorld(const FTransform& WorldTransform, FText& OutFailureReason);
	FVector GetBoxHalfExtent() const;
	UStaticMeshComponent* GetBoxMesh() const { return BoxMesh; }
	const TArray<FShopOrderLine>& GetContents() const { return Contents; }
	int64 GetOrderId() const { return OrderId; }
	FText GetContentsSummary() const;

	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	virtual EPhysicalCarryKind GetPhysicalCarryKind() const override { return EPhysicalCarryKind::DeliveryBox; }
	virtual FText GetPhysicalCarryDisplayName() const override { return GetContentsSummary(); }
	virtual EPhysicalCarryCapability GetPhysicalCarryCapabilities() const override { return EPhysicalCarryCapability::FreeDrop; }
	virtual FTransform GetHeldTransform() const override;
	virtual bool CanBeTakenBy(const UPlayerCarryComponent& Carry, FText& OutFailureReason) const override;
	virtual bool HandleTakenBy(UPlayerCarryComponent& Carry, USceneComponent* HeldAnchor) override;
	virtual bool CanFreeDrop(FText& OutFailureReason) const override;
	virtual UPrimitiveComponent* GetPhysicalCarryPrimitive() const override;
	virtual float GetThrowImpulseStrength() const override { return ThrowImpulseStrength; }
	virtual float GetUpwardThrowImpulseStrength() const override { return UpwardThrowImpulseStrength; }
	virtual bool NotifyPhysicalDropCommitted(UPlayerCarryComponent& Carry) override;
	virtual void PublishPhysicalCarryCommit(EPhysicalCarryCommitTransition Transition) override;
	virtual void RecoverPhysicalCarryable(UPlayerCarryComponent* PreviousCarry) override;
	virtual bool CanDiscardCarriedObject(FText& OutFailureReason) const override;
	virtual void HandleDiscardCommitted() override;

	virtual FHeldEquipmentUseQuery QueryEquipmentUse(const FHeldEquipmentUseContext& Context) const override;
	virtual FHeldEquipmentUseResult BeginEquipmentUse(const FHeldEquipmentUseContext& Context) override;
	virtual FHeldEquipmentUseUpdate UpdateEquipmentUse(const FHeldEquipmentUseContext& Context, float DeltaTime) override;
	virtual FHeldEquipmentUseResult EndEquipmentUse(const FHeldEquipmentUseContext& Context) override;
	virtual void CancelEquipmentUse(const FHeldEquipmentUseContext& Context) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UStaticMeshComponent> BoxMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry|Presentation")
	FTransform HeldTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry", meta = (ClampMin = "0.0"))
	float ThrowImpulseStrength = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry", meta = (ClampMin = "0.0"))
	float UpwardThrowImpulseStrength = 15.0f;

private:
	friend class FShopUnboxingTransaction;

	enum class ELifecycle : uint8
	{
		Staged,
		FreeWorld,
		Held,
		Consumed
	};

	void SetFreeWorldPhysics(bool bEnabled);

	UPROPERTY(Transient)
	int64 OrderId = 0;

	UPROPERTY(Transient)
	TArray<FShopOrderLine> Contents;

	UPROPERTY(Transient)
	bool bContentsInitialized = false;

bool bOpening = false;

	TWeakObjectPtr<UPlayerCarryComponent> Carrier;
	FTransform LastSafeTransform = FTransform::Identity;
	ELifecycle Lifecycle = ELifecycle::Staged;
};
