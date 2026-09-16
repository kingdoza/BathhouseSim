#include "Placement/FacilityPlacementZoneActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/DataValidation.h"
#include "Placement/FacilityPlacementSettings.h"

#define LOCTEXT_NAMESPACE "FacilityPlacementZoneGrid"

void AFacilityPlacementZoneActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!bGridVisible && GridVisual)
	{
		GridVisual->SetVisibility(false, true);
		GridVisual->SetHiddenInGame(true, true);
	}
	float ZoneSizeXCm = 0.0f;
	float ZoneSizeYCm = 0.0f;
	FText FailureReason;
	RefreshGridGeometry(ZoneSizeXCm, ZoneSizeYCm, FailureReason);
}

float AFacilityPlacementZoneActor::GetGridLineThicknessCm() const
{
	const float GridSizeCm = GetDefault<UFacilityPlacementSettings>()->GetGridSizeCm();
	const float Maximum = FMath::Max(
		UE_SMALL_NUMBER * 2.0f,
		GridSizeCm * 0.5f - UE_SMALL_NUMBER);
	const float SafeValue = FMath::IsFinite(GridLineThicknessCm)
		&& GridLineThicknessCm > 0.0f
		? GridLineThicknessCm : 1.0f;
	return FMath::Clamp(SafeValue, UE_SMALL_NUMBER, Maximum);
}

float AFacilityPlacementZoneActor::GetGridZOffsetCm() const
{
	return FMath::IsFinite(GridZOffsetCm)
		? FMath::Max(0.0f, GridZOffsetCm) : 0.0f;
}

int32 AFacilityPlacementZoneActor::GetMajorGridIntervalCells() const
{
	return FMath::Max(2, MajorGridIntervalCells);
}

bool AFacilityPlacementZoneActor::QueryGridGeometry(
	float& OutZoneSizeXCm,
	float& OutZoneSizeYCm,
	float& OutScaleX,
	float& OutScaleY,
	FText& OutFailureReason) const
{
	OutZoneSizeXCm = 0.0f;
	OutZoneSizeYCm = 0.0f;
	OutScaleX = 1.0f;
	OutScaleY = 1.0f;
	if (!ZoneBounds || !PlacementFloor || !GridVisual)
	{
		OutFailureReason = LOCTEXT(
			"MissingGridComponents",
			"Placement Zone grid components are incomplete.");
		return false;
	}

	const FVector ZoneExtent = ZoneBounds->GetUnscaledBoxExtent();
	OutZoneSizeXCm = ZoneExtent.X * 2.0f;
	OutZoneSizeYCm = ZoneExtent.Y * 2.0f;
	if (!FMath::IsFinite(OutZoneSizeXCm) || !FMath::IsFinite(OutZoneSizeYCm)
		|| OutZoneSizeXCm <= UE_SMALL_NUMBER || OutZoneSizeYCm <= UE_SMALL_NUMBER)
	{
		OutFailureReason = LOCTEXT(
			"InvalidGridZoneBounds",
			"Placement Zone grid requires positive finite ZoneBounds X/Y dimensions.");
		return false;
	}

	const UStaticMesh* Mesh = GridVisual->GetStaticMesh();
	if (!Mesh)
	{
		OutFailureReason = LOCTEXT(
			"MissingGridMesh",
			"Placement Zone GridVisual requires a plane Static Mesh.");
		return false;
	}
	const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
	const FVector MeshExtent = MeshBounds.BoxExtent;
	const FVector MeshOrigin = MeshBounds.Origin;
	const float MeshSizeX = MeshExtent.X * 2.0f;
	const float MeshSizeY = MeshExtent.Y * 2.0f;
	if (MeshExtent.ContainsNaN() || MeshOrigin.ContainsNaN()
		|| !FMath::IsFinite(MeshSizeX) || !FMath::IsFinite(MeshSizeY)
		|| MeshSizeX <= UE_SMALL_NUMBER || MeshSizeY <= UE_SMALL_NUMBER)
	{
		OutFailureReason = LOCTEXT(
			"InvalidGridMeshBounds",
			"Placement Zone grid plane requires positive finite local X/Y bounds.");
		return false;
	}
	if (!MeshOrigin.IsNearlyZero(KINDA_SMALL_NUMBER)
		|| !FMath::IsNearlyZero(MeshExtent.Z, KINDA_SMALL_NUMBER))
	{
		OutFailureReason = LOCTEXT(
			"InvalidGridMeshAxis",
			"Placement Zone grid mesh must be a centered local XY plane with its normal on local +Z.");
		return false;
	}

	OutScaleX = OutZoneSizeXCm / MeshSizeX;
	OutScaleY = OutZoneSizeYCm / MeshSizeY;
	if (!FMath::IsFinite(OutScaleX) || !FMath::IsFinite(OutScaleY)
		|| OutScaleX <= UE_SMALL_NUMBER || OutScaleY <= UE_SMALL_NUMBER)
	{
		OutFailureReason = LOCTEXT(
			"InvalidGridScale",
			"Placement Zone grid scale could not be derived from its bounds.");
		return false;
	}
	return true;
}

bool AFacilityPlacementZoneActor::RefreshGridGeometry(
	float& OutZoneSizeXCm,
	float& OutZoneSizeYCm,
	FText& OutFailureReason)
{
	float ScaleX = 1.0f;
	float ScaleY = 1.0f;
	if (!QueryGridGeometry(
		OutZoneSizeXCm,
		OutZoneSizeYCm,
		ScaleX,
		ScaleY,
		OutFailureReason))
	{
		return false;
	}
	GridVisual->SetRelativeTransform(FTransform(
		FQuat::Identity,
		FVector(0.0f, 0.0f, GetGridZOffsetCm()),
		FVector(ScaleX, ScaleY, 1.0f)));
	return true;
}

bool AFacilityPlacementZoneActor::HasValidAuthoredGridValues(
	FText& OutFailureReason) const
{
	const float GridSizeCm = GetDefault<UFacilityPlacementSettings>()->GetGridSizeCm();
	if (!FMath::IsFinite(GridLineThicknessCm) || GridLineThicknessCm <= 0.0f
		|| GridLineThicknessCm >= GridSizeCm * 0.5f)
	{
		OutFailureReason = LOCTEXT(
			"InvalidGridLineThickness",
			"GridLineThicknessCm must be finite, positive, and less than half GridSizeCm.");
		return false;
	}
	if (!FMath::IsFinite(GridZOffsetCm) || GridZOffsetCm < 0.0f)
	{
		OutFailureReason = LOCTEXT(
			"InvalidGridZOffset",
			"GridZOffsetCm must be finite and non-negative.");
		return false;
	}
	if (MajorGridIntervalCells < 2)
	{
		OutFailureReason = LOCTEXT(
			"InvalidMajorGridInterval",
			"MajorGridIntervalCells must be at least two.");
		return false;
	}
	return true;
}

#if WITH_EDITOR
EDataValidationResult AFacilityPlacementZoneActor::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto Invalidate = [&](const FText& Message)
	{
		Context.AddError(Message);
		Result = EDataValidationResult::Invalid;
	};

	FText FailureReason;
	if (!HasValidAuthoredGridValues(FailureReason))
	{
		Invalidate(FailureReason);
	}
	const bool bNativeBaseCDO = HasAnyFlags(RF_ClassDefaultObject)
		&& GetClass() == AFacilityPlacementZoneActor::StaticClass();
	if (!bNativeBaseCDO)
	{
		float ZoneSizeXCm = 0.0f;
		float ZoneSizeYCm = 0.0f;
		float ScaleX = 1.0f;
		float ScaleY = 1.0f;
		if (!QueryGridGeometry(
			ZoneSizeXCm,
			ZoneSizeYCm,
			ScaleX,
			ScaleY,
			FailureReason))
		{
			Invalidate(FailureReason);
		}
		if (!GridVisual || GridVisual->GetNumOverrideMaterials() <= 0
			|| !GridVisual->OverrideMaterials[0])
		{
			Invalidate(LOCTEXT(
				"GridMaterialValidation",
				"GridVisual material element 0 must be assigned."));
		}
	}
	return Result == EDataValidationResult::NotValidated
		? EDataValidationResult::Valid : Result;
}
#endif

#undef LOCTEXT_NAMESPACE
