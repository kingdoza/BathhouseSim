#include "Utility/BathWaterLaborUtilityFacilityActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"
#include "Utility/UtilityGaugeComponent.h"
#include "Utility/UtilityOperationComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BathWaterLaborUtilityFacilityActor"

ABathWaterLaborUtilityFacilityActor::ABathWaterLaborUtilityFacilityActor()
{
	Operation = CreateDefaultSubobject<UUtilityOperationComponent>(TEXT("Operation"));
	GaugeNeedlePivot = CreateDefaultSubobject<USceneComponent>(TEXT("GaugeNeedlePivot"));
	GaugeNeedlePivot->SetupAttachment(SceneRoot);
	GaugeNeedleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GaugeNeedleMesh"));
	GaugeNeedleMesh->SetupAttachment(GaugeNeedlePivot);
	GaugeNeedleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GaugeNeedleMesh->SetCanEverAffectNavigation(false);
	GaugePresentation = CreateDefaultSubobject<UUtilityGaugeComponent>(TEXT("GaugePresentation"));
	GaugePresentation->Configure(Operation, GaugeNeedlePivot);
	Capacity->SetUtilityOperation(Operation);
}

void ABathWaterLaborUtilityFacilityActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		if (GaugePresentation)
		{
			GaugePresentation->ApplyConstructionPreview();
		}
	}
}

UUtilityOperationComponent* ABathWaterLaborUtilityFacilityActor::GetUtilityOperation() const
{
	return Operation;
}

bool ABathWaterLaborUtilityFacilityActor::HasValidUtilityAuthoring(FText& OutFailureReason) const
{
	if (!Super::HasValidUtilityAuthoring(OutFailureReason))
	{
		return false;
	}
	if (!GaugeNeedlePivot || GaugeNeedlePivot->GetAttachParent() != SceneRoot
		|| !GaugeNeedleMesh || !GaugeNeedleMesh->GetStaticMesh()
		|| GaugeNeedleMesh->GetAttachParent() != GaugeNeedlePivot
		|| GaugeNeedleMesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision
		|| GaugeNeedleMesh->CanEverAffectNavigation())
	{
		OutFailureReason = LOCTEXT("InvalidGaugeNeedle", "계기 바늘 메시와 pivot 계층, collision 또는 navigation 설정이 올바르지 않습니다.");
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
EDataValidationResult ABathWaterLaborUtilityFacilityActor::IsDataValid(FDataValidationContext& Context) const
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
