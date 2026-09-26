#include "Utility/BathWaterBoilerFacilityActor.h"

#include "Components/StaticMeshComponent.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"
#include "Utility/UtilityFuelIntakeComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BathWaterBoilerFacilityActor"

ABathWaterBoilerFacilityActor::ABathWaterBoilerFacilityActor()
{
	FuelIntake = CreateDefaultSubobject<UUtilityFuelIntakeComponent>(TEXT("FuelIntake"));
	FuelIntake->SetupAttachment(SceneRoot);
	Capacity->RestoreCapacity(EBathWaterCapacityKind::Heating, Capacity->GetCapacityPoints());
}

FText ABathWaterBoilerFacilityActor::GetFuelFacilityDisplayName() const
{
	return LOCTEXT("BoilerDisplayName", "보일러");
}

bool ABathWaterBoilerFacilityActor::HasValidUtilityAuthoring(FText& OutFailureReason) const
{
	if (!Super::HasValidUtilityAuthoring(OutFailureReason))
	{
		return false;
	}
	if (!FuelIntake)
	{
		OutFailureReason = LOCTEXT("MissingFuelIntakePresentation", "보일러의 기존 FuelIntake 하위 컴포넌트가 필요합니다.");
		return false;
	}
	if (FuelIntake->GetStaticMesh()
		&& (FuelIntake->GetAttachParent() != SceneRoot
			|| !FuelIntake->HasValidAuthoring(OutFailureReason)))
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT("InvalidIntakePresentation", "보일러 투입구 외형 mesh는 SceneRoot에 부착하고 collision과 navigation을 사용하지 않아야 합니다.");
		}
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
