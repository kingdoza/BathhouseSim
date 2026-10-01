#pragma once
#include "GameFramework/Actor.h"
#include "Service/ServiceAmenityTypes.h"
#include "ServiceTestUserActor.generated.h"
class UStaticMeshComponent;
UCLASS(Blueprintable)

class BATHHOUSESIM_API AServiceTestUserActor : public AActor, public IServiceFacilityUser
{
	GENERATED_BODY()
public:

	AServiceTestUserActor();
	virtual void EndPlay(EEndPlayReason::Type Reason) override;
	virtual void HandleServiceUseEnded(AActor& Facility, EServiceUseEndReason Reason) override;
	virtual void HandleScrubCashOffered(AScrubTableActor& Table, ABathhouseCashPaymentActor& Cash,
										const FTransform& StandTransform) override;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Service")
	TObjectPtr<UStaticMeshComponent> WorldMesh;

private:

	UFUNCTION()
	void HandleCashClaimed(ABathhouseCashPaymentActor* Cash);
	UPROPERTY(Transient)
	TWeakObjectPtr<ABathhouseCashPaymentActor> OfferedCash;
};
