#include "Utility/UtilityGaugeComponent.h"

#include "Components/SceneComponent.h"
#include "Utility/UtilityOperationComponent.h"
#include "Utility/UtilityPivotRotation.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "UtilityGaugeComponent"

UUtilityGaugeComponent::UUtilityGaugeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PivotRotation = new FUtilityPivotRotation();
}

UUtilityGaugeComponent::~UUtilityGaugeComponent()
{
	delete PivotRotation;
	PivotRotation = nullptr;
}

void UUtilityGaugeComponent::BeginPlay()
{
	Super::BeginPlay();
	bHasBegunPlay = true;
	CaptureBaselineFromPivot();
	BindOperationDelegate();
	ApplyCurrentOperation();
}

void UUtilityGaugeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(Operation))
	{
		Operation->OnOperationChanged.RemoveAll(this);
	}
	if (!bHasBegunPlay)
	{
		return;
	}
	bHasBegunPlay = false;
	Super::EndPlay(EndPlayReason);
}

void UUtilityGaugeComponent::Configure(
	UUtilityOperationComponent* InOperation,
	USceneComponent* InNeedlePivot)
{
	const bool bSamePivot = NeedlePivot == InNeedlePivot;
	if (Operation != InOperation && IsValid(Operation))
	{
		Operation->OnOperationChanged.RemoveAll(this);
	}
	Operation = InOperation;
	if (!bSamePivot)
	{
		NeedlePivot = InNeedlePivot;
		PivotRotation->Forget();
	}
	if (bHasBegunPlay)
	{
		BindOperationDelegate();
		ApplyCurrentOperation();
	}
}

void UUtilityGaugeComponent::CaptureBaselineFromPivot()
{
	PivotRotation->CaptureBaseline(NeedlePivot);
}

void UUtilityGaugeComponent::BindOperationDelegate()
{
	if (IsValid(Operation))
	{
		Operation->OnOperationChanged.RemoveAll(this);
		Operation->OnOperationChanged.AddUObject(this, &UUtilityGaugeComponent::ApplyCurrentOperation);
	}
}

void UUtilityGaugeComponent::ApplyCurrentOperation()
{
	if (!IsValid(NeedlePivot) || !IsValid(Operation))
	{
		return;
	}
	if (!PivotRotation->HasBaseline())
	{
		CaptureBaselineFromPivot();
	}
	ApplyPoints(Operation->GetRemainingPoints(), Operation->GetMaximumPoints());
}

void UUtilityGaugeComponent::ApplyConstructionPreview()
{
	CaptureBaselineFromPivot();
	if (IsValid(Operation))
	{
		ApplyPoints(0.0f, Operation->GetMaximumPoints());
	}
}

void UUtilityGaugeComponent::ApplyPoints(const float RemainingPoints, const float MaximumPoints)
{
	if (!IsValid(NeedlePivot) || !FMath::IsFinite(MaximumPoints) || MaximumPoints <= 0.0f)
	{
		return;
	}
	if (!FMath::IsFinite(LocalRotationAxis.X) || !FMath::IsFinite(LocalRotationAxis.Y)
		|| !FMath::IsFinite(LocalRotationAxis.Z) || LocalRotationAxis.IsNearlyZero()
		|| !FMath::IsFinite(ZeroAngleDegrees) || !FMath::IsFinite(MaxAngleDegrees)
		|| !FMath::IsFinite(ActiveStartRatio) || ActiveStartRatio < 0.0f || ActiveStartRatio >= 1.0f)
	{
		return;
	}
	const float DisplayFraction = CalculateDisplayFraction(RemainingPoints, MaximumPoints, ActiveStartRatio);
	const float Angle = FMath::Lerp(ZeroAngleDegrees, MaxAngleDegrees, DisplayFraction);
	PivotRotation->Apply(NeedlePivot, LocalRotationAxis, Angle);
}

bool UUtilityGaugeComponent::HasValidAuthoring(FText& OutFailureReason) const
{
	if (!IsValid(Operation) || !IsValid(NeedlePivot)
		|| !FMath::IsFinite(LocalRotationAxis.X) || !FMath::IsFinite(LocalRotationAxis.Y)
		|| !FMath::IsFinite(LocalRotationAxis.Z) || LocalRotationAxis.IsNearlyZero()
		|| !FMath::IsFinite(ZeroAngleDegrees) || !FMath::IsFinite(MaxAngleDegrees)
		|| !FMath::IsFinite(ActiveStartRatio) || ActiveStartRatio < 0.0f || ActiveStartRatio >= 1.0f)
	{
		OutFailureReason = LOCTEXT("InvalidGaugeAuthoring", "계기 연결, 회전축, 각도 또는 가동 시작 비율이 올바르지 않습니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

float UUtilityGaugeComponent::CalculateDisplayFraction(
	const float RemainingPoints,
	const float MaximumPoints,
	const float InActiveStartRatio)
{
	if (!FMath::IsFinite(RemainingPoints) || !FMath::IsFinite(MaximumPoints)
		|| MaximumPoints <= 0.0f || RemainingPoints <= 0.0f)
	{
		return 0.0f;
	}
	const float Ratio = FMath::Clamp(RemainingPoints / MaximumPoints, 0.0f, 1.0f);
	const float StartRatio = FMath::Clamp(InActiveStartRatio, 0.0f, 1.0f);
	return StartRatio + (1.0f - StartRatio) * Ratio;
}

#if WITH_EDITOR
EDataValidationResult UUtilityGaugeComponent::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FText FailureReason;
	if (!HasValidAuthoring(FailureReason))
	{
		Context.AddError(FailureReason);
		Result = EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

#undef LOCTEXT_NAMESPACE
