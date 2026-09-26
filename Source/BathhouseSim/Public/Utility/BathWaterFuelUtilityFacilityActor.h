#pragma once

#include "CoreMinimal.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "Utility/BathWaterLaborUtilityFacilityActor.h"
#include "Utility/UtilityFuelTypes.h"
#include "BathWaterFuelUtilityFacilityActor.generated.h"

class UStaticMeshComponent;
class UUtilityFuelDoorComponent;
class UUtilityFuelIntakeVolumeComponent;
class USceneComponent;

UCLASS(Abstract, Blueprintable)
class BATHHOUSESIM_API ABathWaterFuelUtilityFacilityActor : public ABathWaterLaborUtilityFacilityActor
{
	GENERATED_BODY()

public:
	ABathWaterFuelUtilityFacilityActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual bool HasValidUtilityAuthoring(FText& OutFailureReason) const override;

	UUtilityFuelIntakeVolumeComponent* GetFuelIntakeVolume() const { return FuelIntakeVolume; }
	UUtilityFuelDoorComponent* GetFuelDoorPresentation() const { return FuelDoorPresentation; }
	UStaticMeshComponent* GetFuelDoorMesh() const { return FuelDoorMesh; }
	virtual EUtilityFuelKind GetAcceptedFuelKind() const PURE_VIRTUAL(ABathWaterFuelUtilityFacilityActor::GetAcceptedFuelKind, return EUtilityFuelKind::None;);
	virtual EBathWaterCapacityKind GetRequiredCapacityKind() const PURE_VIRTUAL(ABathWaterFuelUtilityFacilityActor::GetRequiredCapacityKind, return EBathWaterCapacityKind::Heating;);
	virtual FText GetFuelFacilityDisplayName() const PURE_VIRTUAL(ABathWaterFuelUtilityFacilityActor::GetFuelFacilityDisplayName, return FText::GetEmpty(););

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Fuel")
	TObjectPtr<UUtilityFuelIntakeVolumeComponent> FuelIntakeVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Fuel")
	TObjectPtr<USceneComponent> FuelDoorPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Fuel")
	TObjectPtr<UStaticMeshComponent> FuelDoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Fuel")
	TObjectPtr<UUtilityFuelDoorComponent> FuelDoorPresentation;
};
