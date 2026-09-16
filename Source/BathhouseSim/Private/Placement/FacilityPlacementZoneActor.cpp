#include "Placement/FacilityPlacementZoneActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementSettings.h"
#include "Placement/FacilityPlacementTypes.h"

#define LOCTEXT_NAMESPACE "FacilityPlacementZoneActor"

namespace
{
	const FName GridSizeParameter(TEXT("GridSizeCm"));
	const FName ZoneSizeXParameter(TEXT("ZoneSizeXCm"));
	const FName ZoneSizeYParameter(TEXT("ZoneSizeYCm"));
	const FName LineThicknessParameter(TEXT("LineThicknessCm"));
	const FName MajorIntervalParameter(TEXT("MajorGridEveryNCells"));
}

AFacilityPlacementZoneActor::AFacilityPlacementZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;
	ZoneBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBounds"));
	SetRootComponent(ZoneBounds);
	ZoneBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneBounds->SetCollisionResponseToChannel(
		BathhousePlacementCollision::ZoneTraceChannel,
		ECR_Block);
	ZoneBounds->SetCanEverAffectNavigation(false);
	PlacementFloor = CreateDefaultSubobject<USceneComponent>(TEXT("PlacementFloor"));
	PlacementFloor->SetupAttachment(ZoneBounds);
	PlacementFloor->SetCanEverAffectNavigation(false);
	GridVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GridVisual"));
	GridVisual->SetupAttachment(PlacementFloor);
	GridVisual->SetVisibility(false, true);
	GridVisual->SetHiddenInGame(true, true);
	GridVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GridVisual->SetCollisionResponseToAllChannels(ECR_Ignore);
	GridVisual->SetGenerateOverlapEvents(false);
	GridVisual->SetSimulatePhysics(false);
	GridVisual->SetCanEverAffectNavigation(false);
	GridVisual->PrimaryComponentTick.bCanEverTick = false;
	GridVisual->SetComponentTickEnabled(false);
}

bool AFacilityPlacementZoneActor::EnsureGridMaterial(FText& OutFailureReason)
{
	if (GridMID)
	{
		return true;
	}
	if (!GridVisual || GridVisual->GetNumOverrideMaterials() <= 0
		|| !GridVisual->OverrideMaterials[0])
	{
		OutFailureReason = LOCTEXT("MissingGridMaterial", "Placement Zone GridVisual requires a material in element 0.");
		return false;
	}
	GridMID = GridVisual->CreateDynamicMaterialInstance(0);
	if (!GridMID)
	{
		OutFailureReason = LOCTEXT("GridMaterialCreationFailed", "Placement Zone could not create its grid dynamic material instance.");
		return false;
	}
	return true;
}

bool AFacilityPlacementZoneActor::SetGridVisible(const bool bVisible)
{
	if (!bVisible)
	{
		if (!bGridVisible)
		{
			return true;
		}
		GridVisual->SetVisibility(false, true);
		GridVisual->SetHiddenInGame(true, true);
		bGridVisible = false;
		OnGridVisibilityChanged(false);
		return true;
	}
	if (bGridVisible)
	{
		return true;
	}

	float ZoneSizeXCm = 0.0f;
	float ZoneSizeYCm = 0.0f;
	FText FailureReason;
	if (!RefreshGridGeometry(ZoneSizeXCm, ZoneSizeYCm, FailureReason))
	{
		DiagnoseGridFailureOnce(false, FailureReason);
		return false;
	}
	if (!EnsureGridMaterial(FailureReason))
	{
		DiagnoseGridFailureOnce(true, FailureReason);
		return false;
	}

	const float GridSizeCm = GetDefault<UFacilityPlacementSettings>()->GetGridSizeCm();
	GridMID->SetScalarParameterValue(GridSizeParameter, GridSizeCm);
	GridMID->SetScalarParameterValue(ZoneSizeXParameter, ZoneSizeXCm);
	GridMID->SetScalarParameterValue(ZoneSizeYParameter, ZoneSizeYCm);
	GridMID->SetScalarParameterValue(LineThicknessParameter, GetGridLineThicknessCm());
	GridMID->SetScalarParameterValue(MajorIntervalParameter,
		static_cast<float>(GetMajorGridIntervalCells()));
	GridVisual->SetVisibility(true, true);
	GridVisual->SetHiddenInGame(false, true);
	bGridVisible = true;
	OnGridVisibilityChanged(true);
	return true;
}

void AFacilityPlacementZoneActor::DiagnoseGridFailureOnce(
	const bool bMaterialFailure,
	const FText& FailureReason)
{
	bool& bDiagnosed = bMaterialFailure
		? bDiagnosedGridMaterialFailure : bDiagnosedGridGeometryFailure;
	if (bDiagnosed)
	{
		return;
	}
	bDiagnosed = true;
	UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *GetPathName(), *FailureReason.ToString());
}

bool AFacilityPlacementZoneActor::IsDefinitionAllowed(const UFacilityPlacementDefinition& Definition) const
{
	return AllowedFacilityTags.IsEmpty() || Definition.FacilityTags.HasAny(AllowedFacilityTags);
}

FTransform AFacilityPlacementZoneActor::MakeCandidateTransform(
	const FVector& WorldPoint,
	const float YawDegrees,
	const bool bSnap) const
{
	const FTransform FloorTransform = PlacementFloor
		? PlacementFloor->GetComponentTransform()
		: GetActorTransform();
	FVector Local = FloorTransform.InverseTransformPosition(WorldPoint);
	Local.Z = 0.0f;
	if (bSnap)
	{
		const float Grid = GetDefault<UFacilityPlacementSettings>()->GetGridSizeCm();
		Local.X = QuantizeLocalCoordinate(Local.X, Grid);
		Local.Y = QuantizeLocalCoordinate(Local.Y, Grid);
	}
	const FQuat LocalYaw(FVector::UpVector, FMath::DegreesToRadians(NormalizePlacementYaw(YawDegrees)));
	return FTransform(FloorTransform.GetRotation() * LocalYaw, FloorTransform.TransformPosition(Local));
}

bool AFacilityPlacementZoneActor::ContainsFootprint(
	const FTransform& CandidateTransform,
	const FVector& WorldHalfExtent) const
{
	if (!ZoneBounds || WorldHalfExtent.ContainsNaN())
	{
		return false;
	}
	const FTransform ZoneTransform = ZoneBounds->GetComponentTransform();
	const FVector Bounds = ZoneBounds->GetUnscaledBoxExtent();
	const FVector AxisX = CandidateTransform.GetUnitAxis(EAxis::X);
	const FVector AxisY = CandidateTransform.GetUnitAxis(EAxis::Y);
	for (const FVector2D Corner : {
		FVector2D(-1.0f, -1.0f),
		FVector2D(-1.0f, 1.0f),
		FVector2D(1.0f, -1.0f),
		FVector2D(1.0f, 1.0f) })
	{
		const FVector WorldCorner = CandidateTransform.GetLocation()
			+ AxisX * (Corner.X * WorldHalfExtent.X)
			+ AxisY * (Corner.Y * WorldHalfExtent.Y);
		const FVector LocalCorner = ZoneTransform.InverseTransformPosition(WorldCorner);
		if (FMath::Abs(LocalCorner.X) > Bounds.X + KINDA_SMALL_NUMBER
			|| FMath::Abs(LocalCorner.Y) > Bounds.Y + KINDA_SMALL_NUMBER)
		{
			return false;
		}
	}
	return true;
}

float AFacilityPlacementZoneActor::QuantizeLocalCoordinate(const float Value, const float GridSize)
{
	return FMath::GridSnap(Value, FMath::Max(1.0f, GridSize));
}

float AFacilityPlacementZoneActor::NormalizePlacementYaw(const float YawDegrees)
{
	return FRotator::NormalizeAxis(YawDegrees);
}

#undef LOCTEXT_NAMESPACE
