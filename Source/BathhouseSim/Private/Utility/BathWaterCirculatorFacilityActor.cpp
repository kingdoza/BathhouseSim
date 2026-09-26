#include "Utility/BathWaterCirculatorFacilityActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"
#include "Utility/UtilityLeverLaborComponent.h"
#include "Utility/UtilityLeverOperatingVolumeComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BathWaterCirculatorFacilityActor"

ABathWaterCirculatorFacilityActor::ABathWaterCirculatorFacilityActor()
{
	LeverOperatingVolume = CreateDefaultSubobject<UUtilityLeverOperatingVolumeComponent>(TEXT("LeverOperatingVolume"));
	LeverOperatingVolume->SetupAttachment(SceneRoot);
	LeverPivot = CreateDefaultSubobject<USceneComponent>(TEXT("LeverPivot"));
	LeverPivot->SetupAttachment(SceneRoot);
	LeverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeverMesh"));
	LeverMesh->SetupAttachment(LeverPivot);
	LeverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeverMesh->SetCanEverAffectNavigation(false);
	LeverLabor = CreateDefaultSubobject<UUtilityLeverLaborComponent>(TEXT("LeverLabor"));
	LeverLabor->Configure(Operation, LeverPivot);
	LeverOperatingVolume->SetLeverLabor(LeverLabor);
	Capacity->RestoreCapacity(EBathWaterCapacityKind::Circulation, Capacity->GetCapacityPoints());
}

void ABathWaterCirculatorFacilityActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (LeverLabor)
	{
		LeverLabor->RestoreUpPose();
	}
}

void ABathWaterCirculatorFacilityActor::OnFacilityRecoveryHoldStarted()
{
	if (LeverLabor)
	{
		LeverLabor->CancelForRecoveryStart();
	}
}

bool ABathWaterCirculatorFacilityActor::HasValidUtilityAuthoring(FText& OutFailureReason) const
{
	if (!Super::HasValidUtilityAuthoring(OutFailureReason))
	{
		return false;
	}
	if (!Capacity || Capacity->GetCapacityKind() != EBathWaterCapacityKind::Circulation
		|| !LeverOperatingVolume || LeverOperatingVolume->GetAttachParent() != SceneRoot
		|| !LeverOperatingVolume->HasValidAuthoring(OutFailureReason)
		|| !LeverPivot || LeverPivot->GetAttachParent() != SceneRoot
		|| !LeverMesh || !LeverMesh->GetStaticMesh() || LeverMesh->GetAttachParent() != LeverPivot
		|| LeverMesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision
		|| LeverMesh->CanEverAffectNavigation()
		|| !LeverLabor || !LeverLabor->HasValidAuthoring(OutFailureReason))
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT("InvalidCirculatorLever", "순환기 레버 메시, pivot, 조작 영역 또는 노동 component 구성이 올바르지 않습니다.");
		}
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

#if WITH_EDITOR
EDataValidationResult ABathWaterCirculatorFacilityActor::IsDataValid(FDataValidationContext& Context) const
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
