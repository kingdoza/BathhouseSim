#include "Placement/FacilityPlacementFootprintPreview.h"

#include "MaterialShared.h"
#include "Materials/MaterialInterface.h"
#include "Placement/FacilityPlacementComponent.h"

#define LOCTEXT_NAMESPACE "FacilityPlacementFootprintPreview"

namespace FacilityPlacementFootprintPreview
{
const FName PreviewMeshColorParameter(TEXT("Param"));
const FName SurfaceColorParameter(TEXT("PreviewColor"));
const FName SurfaceSizeXParameter(TEXT("FootprintSizeXCm"));
const FName SurfaceSizeYParameter(TEXT("FootprintSizeYCm"));

bool ValidatePlaneBounds(const FBoxSphereBounds& PlaneBounds, FText& OutFailureReason)
{
	// Same rule as AFacilityPlacementZoneActor::QueryGridGeometry's plane check.
	const FVector Extent = PlaneBounds.BoxExtent;
	const FVector Origin = PlaneBounds.Origin;
	if (Extent.ContainsNaN() || Origin.ContainsNaN()
		|| !FMath::IsFinite(Extent.X) || !FMath::IsFinite(Extent.Y)
		|| Extent.X * 2.0f <= UE_SMALL_NUMBER || Extent.Y * 2.0f <= UE_SMALL_NUMBER
		|| !Origin.IsNearlyZero(UE_KINDA_SMALL_NUMBER)
		|| !FMath::IsNearlyZero(Extent.Z, UE_KINDA_SMALL_NUMBER))
	{
		OutFailureReason = LOCTEXT("InvalidPlane", "Footprint 표시 mesh는 중심 pivot의 평평한 plane이어야 합니다.");
		return false;
	}
	return true;
}

bool ComputeSurface(
	const FTransform& FootprintRelativeToRoot,
	const FVector& FootprintUnscaledExtent,
	const FVector& RootRelativeScale,
	const FBoxSphereBounds& PlaneBounds,
	const float FloorOffsetCm,
	FSurfaceLayout& OutLayout,
	FText& OutFailureReason)
{
	if (!ValidatePlaneBounds(PlaneBounds, OutFailureReason))
	{
		return false;
	}
	const FVector Composed = FootprintRelativeToRoot.GetScale3D() * RootRelativeScale;
	if (Composed.ContainsNaN()
		|| !FMath::IsFinite(Composed.X) || !FMath::IsFinite(Composed.Y) || !FMath::IsFinite(Composed.Z)
		|| Composed.X <= UE_KINDA_SMALL_NUMBER || Composed.Y <= UE_KINDA_SMALL_NUMBER
		|| Composed.Z <= UE_KINDA_SMALL_NUMBER
		|| FootprintUnscaledExtent.ContainsNaN() || FootprintUnscaledExtent.X <= 0.0f
		|| FootprintUnscaledExtent.Y <= 0.0f || !FMath::IsFinite(FloorOffsetCm))
	{
		OutFailureReason = LOCTEXT("InvalidFootprintScale", "Footprint 표시의 합성 scale이 올바르지 않습니다.");
		return false;
	}
	const FVector PlaneSize = PlaneBounds.BoxExtent * 2.0f;
	const FTransform PlaneLocal(
		FQuat::Identity,
		FVector(0.0f, 0.0f, -FootprintUnscaledExtent.Z + FloorOffsetCm / Composed.Z),
		FVector(
			2.0f * FootprintUnscaledExtent.X / PlaneSize.X,
			2.0f * FootprintUnscaledExtent.Y / PlaneSize.Y,
			1.0f));
	OutLayout.RelativeToRoot = PlaneLocal * FootprintRelativeToRoot;
	const FVector FullSize = UFacilityPlacementComponent::ComputeScaledFootprintFullSize(
		FootprintRelativeToRoot, FootprintUnscaledExtent, RootRelativeScale);
	OutLayout.WorldSizeCm = FVector2D(FullSize.X, FullSize.Y);
	return true;
}

bool ValidateSurfaceMaterial(UMaterialInterface* Material, FText& OutFailureReason)
{
	FLinearColor Color;
	float Scalar = 0.0f;
	if (!Material || !IsTranslucentBlendMode(Material->GetBlendMode()))
	{
		OutFailureReason = LOCTEXT("InvalidSurfaceMaterial", "Footprint 표시 머티리얼이 없거나 반투명이 아닙니다.");
		return false;
	}
	if (!Material->GetVectorParameterValue(FHashedMaterialParameterInfo(SurfaceColorParameter), Color)
		|| !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(SurfaceSizeXParameter), Scalar)
		|| !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(SurfaceSizeYParameter), Scalar))
	{
		OutFailureReason = LOCTEXT("MissingSurfaceParameters", "Footprint 표시 머티리얼에 PreviewColor, FootprintSizeXCm, FootprintSizeYCm parameter가 필요합니다.");
		return false;
	}
	return true;
}

bool ReadPreviewMeshColor(
	UMaterialInterface* Material,
	FLinearColor& OutColor,
	FText& OutFailureReason)
{
	if (!Material
		|| !Material->GetVectorParameterValue(FHashedMaterialParameterInfo(PreviewMeshColorParameter), OutColor))
	{
		OutFailureReason = LOCTEXT("MissingPreviewColor", "미리보기 머티리얼에서 Param 색을 읽을 수 없습니다.");
		return false;
	}
	return true;
}
}

#undef LOCTEXT_NAMESPACE
