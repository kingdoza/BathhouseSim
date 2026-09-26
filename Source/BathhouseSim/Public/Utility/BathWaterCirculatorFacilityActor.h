#pragma once

#include "CoreMinimal.h"
#include "Utility/BathWaterLaborUtilityFacilityActor.h"
#include "BathWaterCirculatorFacilityActor.generated.h"

class UStaticMeshComponent;
class UUtilityLeverLaborComponent;
class UUtilityLeverOperatingVolumeComponent;
class USceneComponent;

UCLASS(Blueprintable)
class BATHHOUSESIM_API ABathWaterCirculatorFacilityActor : public ABathWaterLaborUtilityFacilityActor
{
	GENERATED_BODY()

public:
	ABathWaterCirculatorFacilityActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual bool HasValidUtilityAuthoring(FText& OutFailureReason) const override;

	UUtilityLeverOperatingVolumeComponent* GetLeverOperatingVolume() const { return LeverOperatingVolume; }
	USceneComponent* GetLeverPivot() const { return LeverPivot; }
	UStaticMeshComponent* GetLeverMesh() const { return LeverMesh; }
	UUtilityLeverLaborComponent* GetLeverLabor() const { return LeverLabor; }

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	virtual void OnFacilityRecoveryHoldStarted() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Lever")
	TObjectPtr<UUtilityLeverOperatingVolumeComponent> LeverOperatingVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Lever")
	TObjectPtr<USceneComponent> LeverPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Lever")
	TObjectPtr<UStaticMeshComponent> LeverMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Lever")
	TObjectPtr<UUtilityLeverLaborComponent> LeverLabor;
};
