#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Placement/FacilityPlacementTypes.h"
#include "PlaceableFacility.generated.h"

class AFacilityPlacementZoneActor;
class APlaceableFacilityItemActor;
class UFacilityPlacementComponent;
struct FFacilityPlacementPayload;

struct BATHHOUSESIM_API FFacilityPlacementPublication
{
	TFunction<void()> Callback;

	void Publish()
	{
		if (Callback)
		{
			Callback();
		}
	}
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UPlaceableFacility : public UInterface
{
	GENERATED_BODY()
};

class BATHHOUSESIM_API IPlaceableFacility
{
	GENERATED_BODY()

public:
	/** Native domain opt-out for fixed endpoints that must never enter Actor conversion. */
	virtual bool SupportsFacilityActorConversion() const { return true; }
	virtual UFacilityPlacementComponent* GetFacilityPlacementComponent() const = 0;
	virtual FFacilityPlacementTransactionResult QueryFacilityPlacement(
		const FTransform& CandidateTransform,
		const AFacilityPlacementZoneActor& Zone) const = 0;
	virtual FFacilityPlacementTransactionResult QueryFacilityRecovery() const = 0;
	virtual bool TryBeginFacilityRecoveryHold(FText& OutFailureReason)
	{
		OutFailureReason = FText::GetEmpty();
		return true;
	}
	virtual void CancelFacilityRecoveryHold() {}
	virtual bool ExportPlacementPayload(
		APlaceableFacilityItemActor& PayloadOwner,
		FFacilityPlacementPayload& OutPayload,
		FText& OutFailureReason) const = 0;
	virtual bool ImportPlacementPayload(
		const APlaceableFacilityItemActor& PayloadOwner,
		const FFacilityPlacementPayload& Payload,
		FText& OutFailureReason) = 0;
	/**
	 * Called by the placement transaction right after FinishSpawning (Blueprint components exist) and before the
	 * collision snapshot is finalized. Payload state that depends on Blueprint components is applied here.
	 * Returning false fails the placement like an import failure.
	 */
	virtual bool FinalizePlacementPayloadAfterConstruction(FText& OutFailureReason)
	{
		OutFailureReason = FText::GetEmpty();
		return true;
	}
	virtual bool StagePlacedDomainRegistration(FText& OutFailureReason) = 0;
	virtual void RollbackPlacedDomainRegistration() = 0;
	virtual bool StagePlacedDomainUnregistration(
		FFacilityPlacementPublication& OutPublication,
		FText& OutFailureReason) = 0;
	virtual bool RollbackPlacedDomainUnregistration(FText& OutFailureReason) = 0;
	virtual void PublishPlacedDomainRegistration() = 0;
	virtual bool CommitPlaceableFacilityMode(EPlaceableFacilityMode NewMode, FText& OutFailureReason) = 0;
};
