#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FacilityPlacementSettings.generated.h"

class UMaterialInterface;
class UStaticMesh;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Facility Placement"))
class BATHHOUSESIM_API UFacilityPlacementSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UFacilityPlacementSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	float GetGridSizeCm() const { return FMath::Max(1.0f, GridSizeCm); }
	float GetRotationStepDegrees() const { return FMath::Clamp(RotationStepDegrees, 1.0f, 180.0f); }
	float GetRecoveryHoldSeconds() const { return FMath::Max(0.1f, RecoveryHoldSeconds); }
	float GetRecoveryDropZOffsetCm() const { return FMath::Max(0.0f, RecoveryDropZOffsetCm); }
	float GetPlacementTraceDistance() const { return FMath::Max(1.0f, PlacementTraceDistance); }
	float GetRecoveryTraceDistance() const { return FMath::Max(1.0f, RecoveryTraceDistance); }
	FTransform GetFacilityItemHeldTransform() const
	{
		FTransform Result = FacilityItemHeldTransform;
		Result.SetScale3D(FVector::OneVector);
		return Result;
	}
	UMaterialInterface* LoadValidPreviewMaterial() const
	{
		return ValidPreviewMaterial.IsValid() ? ValidPreviewMaterial.Get() : ValidPreviewMaterial.LoadSynchronous();
	}
	UMaterialInterface* LoadInvalidPreviewMaterial() const
	{
		return InvalidPreviewMaterial.IsValid() ? InvalidPreviewMaterial.Get() : InvalidPreviewMaterial.LoadSynchronous();
	}

	float GetFootprintPreviewFloorOffsetCm() const
	{
		return FMath::IsFinite(FootprintPreviewFloorOffsetCm) ? FMath::Max(0.0f, FootprintPreviewFloorOffsetCm) : 1.0f;
	}
	int32 GetFootprintPreviewTranslucencySortPriority() const
	{
		return FMath::Clamp(FootprintPreviewTranslucencySortPriority, MIN_int16, MAX_int16);
	}
	int32 GetPreviewMeshTranslucencySortPriority() const
	{
		return FMath::Clamp(PreviewMeshTranslucencySortPriority, MIN_int16, MAX_int16);
	}
	UStaticMesh* LoadFootprintPreviewMesh() const
	{
		return FootprintPreviewMesh.IsNull() ? nullptr
			: (FootprintPreviewMesh.IsValid() ? FootprintPreviewMesh.Get() : FootprintPreviewMesh.LoadSynchronous());
	}
	UMaterialInterface* LoadFootprintPreviewMaterial() const
	{
		return FootprintPreviewMaterial.IsNull() ? nullptr
			: (FootprintPreviewMaterial.IsValid() ? FootprintPreviewMaterial.Get() : FootprintPreviewMaterial.LoadSynchronous());
	}

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (ClampMin = "1.0", UIMin = "1.0", ForceUnits = "cm"))
	float GridSizeCm = 10.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (ClampMin = "1.0", ClampMax = "180.0", UIMin = "1.0", UIMax = "180.0", ForceUnits = "deg"))
	float RotationStepDegrees = 15.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Recovery", meta = (ClampMin = "0.1", UIMin = "0.1", ForceUnits = "s"))
	float RecoveryHoldSeconds = 1.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Recovery", meta = (ClampMin = "0.0", UIMin = "0.0", ForceUnits = "cm"))
	float RecoveryDropZOffsetCm = 100.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Trace", meta = (ClampMin = "1.0", UIMin = "1.0", ForceUnits = "cm"))
	float PlacementTraceDistance = 500.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Trace", meta = (ClampMin = "1.0", UIMin = "1.0", ForceUnits = "cm"))
	float RecoveryTraceDistance = 300.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Carry")
	FTransform FacilityItemHeldTransform = FTransform::Identity;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Preview")
	TSoftObjectPtr<UMaterialInterface> ValidPreviewMaterial;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Preview")
	TSoftObjectPtr<UMaterialInterface> InvalidPreviewMaterial;

	// Center-pivot plane with +Z normal used for the held facility footprint display.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Preview|Footprint")
	TSoftObjectPtr<UStaticMesh> FootprintPreviewMesh;

	// Translucent footprint MI. Required parameters: PreviewColor (vector), FootprintSizeXCm, FootprintSizeYCm (scalar).
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Preview|Footprint")
	TSoftObjectPtr<UMaterialInterface> FootprintPreviewMaterial;

	// Height of the footprint display above the placement floor.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Preview|Footprint", meta = (ClampMin = "0.0", UIMin = "0.0", ForceUnits = "cm"))
	float FootprintPreviewFloorOffsetCm = 1.0f;

	// Draw order contract: zone grid GridVisual < footprint < preview mesh.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Preview|Footprint")
	int32 FootprintPreviewTranslucencySortPriority = 1;

	// Draw order contract: zone grid GridVisual < footprint < preview mesh.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Preview|Footprint")
	int32 PreviewMeshTranslucencySortPriority = 2;
};
