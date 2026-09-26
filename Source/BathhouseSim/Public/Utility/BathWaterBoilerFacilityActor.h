#pragma once

#include "CoreMinimal.h"
#include "Utility/BathWaterFuelUtilityFacilityActor.h"
#include "BathWaterBoilerFacilityActor.generated.h"

class UUtilityFuelIntakeComponent;

UCLASS(Blueprintable)
class BATHHOUSESIM_API ABathWaterBoilerFacilityActor : public ABathWaterFuelUtilityFacilityActor
{
	GENERATED_BODY()

public:
	ABathWaterBoilerFacilityActor();
	virtual bool HasValidUtilityAuthoring(FText& OutFailureReason) const override;
	virtual EUtilityFuelKind GetAcceptedFuelKind() const override { return EUtilityFuelKind::Coal; }
	virtual EBathWaterCapacityKind GetRequiredCapacityKind() const override { return EBathWaterCapacityKind::Heating; }
	virtual FText GetFuelFacilityDisplayName() const override;

	UUtilityFuelIntakeComponent* GetFuelIntake() const { return FuelIntake; }

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Fuel")
	TObjectPtr<UUtilityFuelIntakeComponent> FuelIntake;
};
