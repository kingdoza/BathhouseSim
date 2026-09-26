#pragma once

#include "CoreMinimal.h"
#include "Facility/BathWaterUtilityFacilityActor.h"
#include "BathWaterLaborUtilityFacilityActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UUtilityGaugeComponent;
class UUtilityOperationComponent;

UCLASS(Abstract, Blueprintable)
class BATHHOUSESIM_API ABathWaterLaborUtilityFacilityActor : public ABathWaterUtilityFacilityActor
{
	GENERATED_BODY()

public:
	ABathWaterLaborUtilityFacilityActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual UUtilityOperationComponent* GetUtilityOperation() const override;
	virtual bool HasValidUtilityAuthoring(FText& OutFailureReason) const override;

	UUtilityOperationComponent* GetOperation() const { return Operation; }
	USceneComponent* GetGaugeNeedlePivot() const { return GaugeNeedlePivot; }
	UStaticMeshComponent* GetGaugeNeedleMesh() const { return GaugeNeedleMesh; }
	UUtilityGaugeComponent* GetGaugePresentation() const { return GaugePresentation; }

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Operation")
	TObjectPtr<UUtilityOperationComponent> Operation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Gauge")
	TObjectPtr<USceneComponent> GaugeNeedlePivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Gauge")
	TObjectPtr<UStaticMeshComponent> GaugeNeedleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Gauge")
	TObjectPtr<UUtilityGaugeComponent> GaugePresentation;
};
