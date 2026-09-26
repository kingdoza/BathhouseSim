#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PhysicalCarryable.h"
#include "Interaction/PlayerInteractable.h"
#include "Interaction/SupplementalInteractionIntentSource.h"
#include "Placement/PlaceableFacility.h"
#include "BathWaterUtilityFacilityActor.generated.h"

class APlaceableFacilityItemActor;
class UBathWaterUtilityCapacityComponent;
class UBoxComponent;
class UFacilityPlacementComponent;
class UPlayerCarryComponent;
class USceneComponent;
class UStaticMeshComponent;
class UUtilityOperationComponent;

UCLASS(Blueprintable)
class BATHHOUSESIM_API ABathWaterUtilityFacilityActor
	: public AActor
	, public IPlayerInteractable
	, public ISupplementalInteractionIntentSource
	, public IPlaceableFacility
	, public IPhysicalCarryable
{
	GENERATED_BODY()

public:
	ABathWaterUtilityFacilityActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	virtual FPlayerInteractionQuery MergeSupplementalInteractionQuery(const FPlayerInteractionQuery& BaseQuery) const override;
	virtual UFacilityPlacementComponent* GetFacilityPlacementComponent() const override { return FacilityPlacement; }
	virtual FFacilityPlacementTransactionResult QueryFacilityPlacement(
		const FTransform& CandidateTransform,
		const class AFacilityPlacementZoneActor& Zone) const override;
	virtual FFacilityPlacementTransactionResult QueryFacilityRecovery() const override;
	virtual bool TryBeginFacilityRecoveryHold(FText& OutFailureReason) override;
	virtual void CancelFacilityRecoveryHold() override;
	virtual bool ExportPlacementPayload(APlaceableFacilityItemActor& Item,
		FFacilityPlacementPayload& OutPayload, FText& OutFailureReason) const override;
	virtual bool ImportPlacementPayload(const APlaceableFacilityItemActor& Item,
		const FFacilityPlacementPayload& Payload, FText& OutFailureReason) override;
	virtual bool StagePlacedDomainRegistration(FText& OutFailureReason) override;
	virtual void RollbackPlacedDomainRegistration() override;
	virtual bool StagePlacedDomainUnregistration(FFacilityPlacementPublication& OutPublication,
		FText& OutFailureReason) override;
	virtual bool RollbackPlacedDomainUnregistration(FText& OutFailureReason) override;
	virtual void PublishPlacedDomainRegistration() override;
	virtual bool CommitPlaceableFacilityMode(EPlaceableFacilityMode NewMode, FText& OutFailureReason) override;

	virtual EPhysicalCarryKind GetPhysicalCarryKind() const override { return EPhysicalCarryKind::Facility; }
	virtual EPhysicalCarryCapability GetPhysicalCarryCapabilities() const override { return EPhysicalCarryCapability::None; }
	virtual FText GetPhysicalCarryDisplayName() const override;
	virtual FTransform GetHeldTransform() const override;
	virtual bool CanBeTakenBy(const UPlayerCarryComponent& Carry, FText& OutFailureReason) const override;
	virtual bool HandleTakenBy(UPlayerCarryComponent& Carry, USceneComponent* HeldAnchor) override;
	virtual bool CanFreeDrop(FText& OutFailureReason) const override;
	virtual UPrimitiveComponent* GetPhysicalCarryPrimitive() const override;
	virtual float GetThrowImpulseStrength() const override;
	virtual float GetUpwardThrowImpulseStrength() const override;
	virtual AActor* GetAssignedPhysicalCarryFixedSlot() const override;
	virtual bool TryBindPhysicalCarryFixedSlot(AActor& SlotActor, FText& OutFailureReason) override;
	virtual void ClearPhysicalCarryFixedSlotBinding(AActor& ExpectedSlot) override;
	virtual void NotifyPhysicalCarryFixedSlotBindingConflict() override;
	virtual bool IsStoredInAssignedPhysicalCarryFixedSlot() const override;
	virtual bool NotifyTakenFromFixedSlotCommitted(UPlayerCarryComponent& Carry, AActor& SlotActor) override;
	virtual bool NotifyStoredInFixedSlotCommitted(UPlayerCarryComponent& Carry, AActor& SlotActor) override;
	virtual bool NotifyRecoveredToFixedSlotCommitted(AActor& SlotActor) override;
	virtual void NotifyFixedSlotDestroyed(AActor& SlotActor) override;
	virtual bool NotifyPhysicalDropCommitted(UPlayerCarryComponent& Carry) override;
	virtual void PublishPhysicalCarryCommit(EPhysicalCarryCommitTransition Transition) override;
	virtual void RecoverPhysicalCarryable(UPlayerCarryComponent* PreviousCarry) override;

	UBathWaterUtilityCapacityComponent* GetCapacityComponent() const { return Capacity; }
	virtual UUtilityOperationComponent* GetUtilityOperation() const;
	virtual bool RequiresLaborOperation() const { return true; }
	virtual bool HasValidUtilityAuthoring(FText& OutFailureReason) const;

protected:
	virtual void OnFacilityRecoveryHoldStarted() {}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water Utility")
	TObjectPtr<UBoxComponent> PackagePhysicalRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water Utility")
	TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water Utility")
	TObjectPtr<UStaticMeshComponent> VisualMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Facility Placement")
	TObjectPtr<UBoxComponent> PlacementFootprint;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Facility Placement")
	TObjectPtr<UFacilityPlacementComponent> FacilityPlacement;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bath Water Utility")
	TObjectPtr<UBathWaterUtilityCapacityComponent> Capacity;

private:
	friend class FBathWaterOperationsFacilityTransactionTest;
	void HandleUtilityOperatingChanged(bool bIsOperating);
	void BindUtilityOperation();
	void UnbindUtilityOperation();
	TWeakObjectPtr<UUtilityOperationComponent> BoundUtilityOperation;
	bool bProviderRegistered = false;
	bool bRecoveryHoldActive = false;
};
