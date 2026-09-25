#pragma once

#include "CoreMinimal.h"
#include "Facility/BathWaterUtilityFacilityActor.h"
#include "BathWaterBoilerFacilityActor.generated.h"

class UStaticMeshComponent;
class UUtilityFuelIntakeComponent;
class UUtilityGaugeComponent;
class UUtilityOperationComponent;
class USceneComponent;

UCLASS(Blueprintable)
class BATHHOUSESIM_API ABathWaterBoilerFacilityActor : public ABathWaterUtilityFacilityActor
{
	GENERATED_BODY()

public:
	ABathWaterBoilerFacilityActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual UUtilityOperationComponent* GetUtilityOperation() const override;
	virtual bool RequiresLaborOperation() const override { return true; }
	virtual bool HasValidUtilityAuthoring(FText& OutFailureReason) const override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UUtilityFuelIntakeComponent* GetFuelIntake() const { return FuelIntake; }
	UUtilityOperationComponent* GetOperation() const { return Operation; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Operation")
	TObjectPtr<UUtilityOperationComponent> Operation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Fuel")
	TObjectPtr<UUtilityFuelIntakeComponent> FuelIntake;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Gauge")
	TObjectPtr<UStaticMeshComponent> GaugeFace;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Gauge")
	TObjectPtr<USceneComponent> GaugeNeedlePivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Gauge")
	TObjectPtr<UStaticMeshComponent> GaugeNeedleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Gauge")
	TObjectPtr<UUtilityGaugeComponent> GaugePresentation;
};
