#include "Utility/BathWaterFuelUtilityFacilityActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"
#include "Utility/UtilityFuelDoorComponent.h"
#include "Utility/UtilityFuelIntakeVolumeComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BathWaterFuelUtilityFacilityActor"

ABathWaterFuelUtilityFacilityActor::ABathWaterFuelUtilityFacilityActor()
{
	FuelIntakeVolume = CreateDefaultSubobject<UUtilityFuelIntakeVolumeComponent>(TEXT("FuelIntakeVolume"));
	FuelIntakeVolume->SetupAttachment(SceneRoot);
	FuelDoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("FuelDoorPivot"));
	FuelDoorPivot->SetupAttachment(SceneRoot);
	FuelDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FuelDoorMesh"));
	FuelDoorMesh->SetupAttachment(FuelDoorPivot);
	FuelDoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FuelDoorMesh->SetCanEverAffectNavigation(false);
	FuelDoorPresentation = CreateDefaultSubobject<UUtilityFuelDoorComponent>(TEXT("FuelDoorPresentation"));
	FuelDoorPresentation->Configure(FuelDoorPivot);
	FuelIntakeVolume->SetFuelDoorPresentation(FuelDoorPresentation);
}

void ABathWaterFuelUtilityFacilityActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		if (FuelDoorPresentation)
		{
			FuelDoorPresentation->ApplyClosedImmediately();
		}
	}
}

bool ABathWaterFuelUtilityFacilityActor::HasValidUtilityAuthoring(FText& OutFailureReason) const
{
	if (!Super::HasValidUtilityAuthoring(OutFailureReason))
	{
		return false;
	}
	if (!Capacity || Capacity->GetCapacityKind() != GetRequiredCapacityKind())
	{
		OutFailureReason = LOCTEXT("InvalidFuelFacilityCapacity", "연료 설비 용량 종류가 설비 계약과 일치하지 않습니다.");
		return false;
	}
	if (!FuelIntakeVolume || FuelIntakeVolume->GetAttachParent() != SceneRoot
		|| !FuelIntakeVolume->HasValidAuthoring(OutFailureReason))
	{
		return false;
	}
	if (!FuelDoorPivot || FuelDoorPivot->GetAttachParent() != SceneRoot
		|| !FuelDoorMesh || !FuelDoorMesh->GetStaticMesh()
		|| FuelDoorMesh->GetAttachParent() != FuelDoorPivot
		|| FuelDoorMesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision
		|| FuelDoorMesh->CanEverAffectNavigation()
		|| !FuelDoorPresentation || !FuelDoorPresentation->HasValidAuthoring(OutFailureReason))
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT("InvalidFuelDoor", "연료 투입구 문 mesh와 pivot 계층, collision 또는 navigation 설정이 올바르지 않습니다.");
		}
		return false;
	}
	if (GetAcceptedFuelKind() == EUtilityFuelKind::None)
	{
		OutFailureReason = LOCTEXT("MissingAcceptedFuel", "연료 설비가 받을 연료 종류를 지정해야 합니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

#if WITH_EDITOR
EDataValidationResult ABathWaterFuelUtilityFacilityActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FText FailureReason;
	if (!HasValidUtilityAuthoring(FailureReason))
	{
		Context.AddError(FailureReason);
		Result = EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

#undef LOCTEXT_NAMESPACE
