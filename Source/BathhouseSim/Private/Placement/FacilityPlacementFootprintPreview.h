#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;

// Pure surface math and material-contract helpers for the held-facility footprint display.
namespace FacilityPlacementFootprintPreview
{
	// Material contract parameter names (asset contract, not tuning values).
	extern const FName PreviewMeshColorParameter;   // vector on the valid/invalid preview mesh materials
	extern const FName SurfaceColorParameter;       // vector on the footprint material
	extern const FName SurfaceSizeXParameter;       // scalar, plane local X world size in cm
	extern const FName SurfaceSizeYParameter;       // scalar, plane local Y world size in cm

	struct FSurfaceLayout
	{
		// Component transform relative to the preview Actor root.
		FTransform RelativeToRoot = FTransform::Identity;
		// World full size along the footprint X/Y axes.
		FVector2D WorldSizeCm = FVector2D::ZeroVector;
	};

	bool ValidatePlaneBounds(const FBoxSphereBounds& PlaneBounds, FText& OutFailureReason);

	bool ComputeSurface(
		const FTransform& FootprintRelativeToRoot,
		const FVector& FootprintUnscaledExtent,
		const FVector& RootRelativeScale,
		const FBoxSphereBounds& PlaneBounds,
		float FloorOffsetCm,
		FSurfaceLayout& OutLayout,
		FText& OutFailureReason);

	bool ValidateSurfaceMaterial(UMaterialInterface* Material, FText& OutFailureReason);

	bool ReadPreviewMeshColor(
		UMaterialInterface* Material,
		FLinearColor& OutColor,
		FText& OutFailureReason);
}
