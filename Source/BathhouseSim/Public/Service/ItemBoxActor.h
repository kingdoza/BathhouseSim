#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "Interaction/PhysicalCarryable.h"
#include "Interaction/PhysicalCarryDiscardable.h"
#include "Interaction/PlayerInteractable.h"
#include "Service/ServiceItemTypes.h"
#include "ItemBoxActor.generated.h"

class UInstancedStaticMeshComponent;
class UPlayerCarryComponent;
class UPrimitiveComponent;
class UServiceItemDefinition;
class UStaticMeshComponent;
class USceneComponent;
class UWorld;

/** Open-top carryable box holding a homogeneous stack of service items. Not equipment; contents change only through FServiceItemTransfer. */
UCLASS(Blueprintable)
class BATHHOUSESIM_API AItemBoxActor
	: public AActor
	, public IPlayerInteractable
	, public IPhysicalCarryable
	, public IPhysicalCarryDiscardable
{
	GENERATED_BODY()

public:
	AItemBoxActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** Spawn-time only (before BeginPlay). Count is 0..BoxCapacity of Kind; Count 0 leaves an unlocked empty box. */
	bool InitializeContents(UServiceItemDefinition* Kind, int32 Count);
	bool ActivateFreeWorld(const FTransform& WorldTransform, FText& OutFailureReason);

	const FServiceItemStack& GetContents() const { return Contents; }
	/** Mutable access for FServiceItemTransfer callers. Call NotifyContentsChanged after a successful commit. */
	FServiceItemStack& GetMutableContents() { return Contents; }
	void NotifyContentsChanged();
	bool IsFreeWorld() const { return Lifecycle == ELifecycle::FreeWorld; }
	bool IsHeld() const { return Lifecycle == ELifecycle::Held; }
	UStaticMeshComponent* GetBoxMesh() const { return BoxMesh; }
	UInstancedStaticMeshComponent* GetContentsVisual() const { return ContentsVisual; }
	FText GetContentsSummary() const;

	/** Deferred-spawns a box, fills it and finishes spawning. Caller still activates it with ActivateFreeWorld. */
	static AItemBoxActor* SpawnFilledBox(
		UWorld& World,
		TSubclassOf<AItemBoxActor> BoxClass,
		UServiceItemDefinition* Kind,
		int32 Count,
		const FTransform& WorldTransform,
		FText& OutFailureReason);

	static bool BuildClassCollisionQuery(
		TSubclassOf<AItemBoxActor> BoxClass,
		const FTransform& WorldTransform,
		FVector& OutLocation,
		FQuat& OutRotation,
		FCollisionShape& OutShape,
		const UPrimitiveComponent*& OutCollisionTemplate,
		FText& OutFailureReason);

	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	virtual EPhysicalCarryKind GetPhysicalCarryKind() const override { return EPhysicalCarryKind::ItemBox; }
	virtual FText GetPhysicalCarryDisplayName() const override { return GetContentsSummary(); }
	virtual FText GetHeldSummaryText() const override { return GetContentsSummary(); }
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
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Editor Preview")
	TObjectPtr<UServiceItemDefinition> EditorPreviewKind;

	UPROPERTY(EditAnywhere, Category = "Editor Preview", meta = (ClampMin = "0"))
	int32 EditorPreviewCount = 0;
#endif

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "Editor Preview")
	void RefreshEditorPreview();
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Box")
	TObjectPtr<UStaticMeshComponent> BoxMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Box")
	TObjectPtr<UInstancedStaticMeshComponent> ContentsVisual;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry|Presentation")
	FTransform HeldTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry", meta = (ClampMin = "0.0"))
	float ThrowImpulseStrength = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry", meta = (ClampMin = "0.0"))
	float UpwardThrowImpulseStrength = 15.0f;

private:
	enum class ELifecycle : uint8
	{
		Staged,
		FreeWorld,
		Held,
		Consumed
	};

	void SetFreeWorldPhysics(bool bEnabled);
	void RebuildContentsVisual(const UServiceItemDefinition* Kind, int32 Count);

	UPROPERTY(Transient)
	FServiceItemStack Contents;

	bool bContentsInitialized = false;

	TWeakObjectPtr<UPlayerCarryComponent> Carrier;
	FTransform LastSafeTransform = FTransform::Identity;
	ELifecycle Lifecycle = ELifecycle::Staged;
};
