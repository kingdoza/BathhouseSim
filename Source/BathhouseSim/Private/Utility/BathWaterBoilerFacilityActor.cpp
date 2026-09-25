#include "Utility/BathWaterBoilerFacilityActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"
#include "Utility/UtilityFuelIntakeComponent.h"
#include "Utility/UtilityGaugeComponent.h"
#include "Utility/UtilityOperationComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BathWaterBoilerFacilityActor"

ABathWaterBoilerFacilityActor::ABathWaterBoilerFacilityActor()
{
	Operation = CreateDefaultSubobject<UUtilityOperationComponent>(TEXT("Operation"));
	FuelIntake = CreateDefaultSubobject<UUtilityFuelIntakeComponent>(TEXT("FuelIntake"));
	FuelIntake->SetupAttachment(SceneRoot);

	GaugeFace = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GaugeFace"));
	GaugeFace->SetupAttachment(SceneRoot);
	GaugeFace->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GaugeFace->SetCanEverAffectNavigation(false);

	GaugeNeedlePivot = CreateDefaultSubobject<USceneComponent>(TEXT("GaugeNeedlePivot"));
	GaugeNeedlePivot->SetupAttachment(SceneRoot);
	GaugeNeedleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GaugeNeedleMesh"));
	GaugeNeedleMesh->SetupAttachment(GaugeNeedlePivot);
	GaugeNeedleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GaugeNeedleMesh->SetCanEverAffectNavigation(false);

	GaugePresentation = CreateDefaultSubobject<UUtilityGaugeComponent>(TEXT("GaugePresentation"));
	GaugePresentation->Configure(Operation, GaugeNeedlePivot);
	Capacity->RestoreCapacity(EBathWaterCapacityKind::Heating, Capacity->GetCapacityPoints());
	Capacity->SetUtilityOperation(Operation, true);
}

void ABathWaterBoilerFacilityActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (GaugePresentation && (!GetWorld() || !GetWorld()->IsGameWorld()))
	{
		GaugePresentation->ApplyConstructionPreview();
	}
}

UUtilityOperationComponent* ABathWaterBoilerFacilityActor::GetUtilityOperation() const
{
	return Operation;
}

bool ABathWaterBoilerFacilityActor::HasValidUtilityAuthoring(FText& OutFailureReason) const
{
	if (!Super::HasValidUtilityAuthoring(OutFailureReason))
	{
		return false;
	}
	if (!Capacity || Capacity->GetCapacityKind() != EBathWaterCapacityKind::Heating)
	{
		OutFailureReason = LOCTEXT("InvalidBoilerCapacityKind", "보일러 용량 종류는 Heating이어야 합니다.");
		return false;
	}
	if (!FuelIntake || !FuelIntake->HasValidAuthoring(OutFailureReason))
	{
		return false;
	}
	if (!GaugeFace || !GaugeFace->GetStaticMesh()
		|| GaugeFace->GetAttachParent() != SceneRoot
		|| GaugeFace->GetCollisionEnabled() != ECollisionEnabled::NoCollision
		|| GaugeFace->CanEverAffectNavigation())
	{
		OutFailureReason = LOCTEXT(
			"InvalidGaugeFaceAuthoring",
			"보일러 계기판은 메시를 지정하고 SceneRoot에 부착하며 collision과 navigation을 사용하지 않아야 합니다.");
		return false;
	}
	if (!GaugeNeedlePivot || GaugeNeedlePivot->GetAttachParent() != SceneRoot
		|| !GaugeNeedleMesh || !GaugeNeedleMesh->GetStaticMesh()
		|| GaugeNeedleMesh->GetAttachParent() != GaugeNeedlePivot
		|| GaugeNeedleMesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision
		|| GaugeNeedleMesh->CanEverAffectNavigation())
	{
		OutFailureReason = LOCTEXT(
			"InvalidGaugeNeedleAuthoring",
			"보일러 계기 바늘 메시와 pivot 계층, collision 또는 navigation 설정이 올바르지 않습니다.");
		return false;
	}
	if (!GaugePresentation || !GaugePresentation->HasValidAuthoring(OutFailureReason))
	{
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

#if WITH_EDITOR
EDataValidationResult ABathWaterBoilerFacilityActor::IsDataValid(FDataValidationContext& Context) const
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
