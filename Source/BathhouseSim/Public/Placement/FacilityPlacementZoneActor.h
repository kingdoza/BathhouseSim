#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "FacilityPlacementZoneActor.generated.h"

class UBoxComponent;
class UFacilityPlacementDefinition;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class BATHHOUSESIM_API AFacilityPlacementZoneActor : public AActor
{
	GENERATED_BODY()

public:
	AFacilityPlacementZoneActor();
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	bool IsDefinitionAllowed(const UFacilityPlacementDefinition& Definition) const;
	FTransform MakeCandidateTransform(const FVector& WorldPoint, float YawDegrees, bool bSnap) const;
	bool ContainsFootprint(const FTransform& CandidateTransform, const FVector& WorldHalfExtent) const;
	bool SetGridVisible(bool bVisible);
	bool IsGridVisible() const { return bGridVisible; }
	UBoxComponent* GetZoneBounds() const { return ZoneBounds; }
	USceneComponent* GetPlacementFloor() const { return PlacementFloor; }
	UStaticMeshComponent* GetGridVisual() const { return GridVisual; }
	float GetGridLineThicknessCm() const;
	float GetGridZOffsetCm() const;
	int32 GetMajorGridIntervalCells() const;

	static float QuantizeLocalCoordinate(float Value, float GridSize);
	static float NormalizePlacementYaw(float YawDegrees);

	UFUNCTION(BlueprintImplementableEvent, Category = "Facility Placement|Presentation")
	void OnGridVisibilityChanged(bool bVisible);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Facility Placement")
	TObjectPtr<UBoxComponent> ZoneBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Facility Placement")
	TObjectPtr<USceneComponent> PlacementFloor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Facility Placement|Grid")
	TObjectPtr<UStaticMeshComponent> GridVisual;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Facility Placement")
	FGameplayTagContainer AllowedFacilityTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Facility Placement|Grid", meta = (ClampMin = "0.01", ForceUnits = "cm"))
	float GridLineThicknessCm = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Facility Placement|Grid", meta = (ClampMin = "0.0", ForceUnits = "cm"))
	float GridZOffsetCm = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Facility Placement|Grid", meta = (ClampMin = "2", UIMin = "2"))
	int32 MajorGridIntervalCells = 10;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GridMID;

	UPROPERTY(Transient)
	bool bGridVisible = false;

private:
	bool RefreshGridGeometry(float& OutZoneSizeXCm, float& OutZoneSizeYCm, FText& OutFailureReason);
	bool QueryGridGeometry(float& OutZoneSizeXCm, float& OutZoneSizeYCm,
		float& OutScaleX, float& OutScaleY, FText& OutFailureReason) const;
	bool EnsureGridMaterial(FText& OutFailureReason);
	bool HasValidAuthoredGridValues(FText& OutFailureReason) const;
	void DiagnoseGridFailureOnce(bool bMaterialFailure, const FText& FailureReason);

	bool bDiagnosedGridGeometryFailure = false;
	bool bDiagnosedGridMaterialFailure = false;
};
