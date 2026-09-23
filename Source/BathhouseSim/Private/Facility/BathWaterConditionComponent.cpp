#include "Facility/BathWaterConditionComponent.h"

#include "Customer/CustomerSessionComponent.h"
#include "Facility/BathWaterOperationsSubsystem.h"
#include "Facility/BathWaterSettings.h"
#include "Facility/BathhouseBathFacilityActor.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BathWaterConditionComponent"

UBathWaterConditionComponent::UBathWaterConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;
}

void UBathWaterConditionComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureWaterStateBound();
	RefreshTickState();
}

void UBathWaterConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (WaterState)
	{
		WaterState->OnFlowStepNative.Remove(FlowStepHandle);
		WaterState->OnWaterAmountChangedNative.Remove(AmountChangedHandle);
	}
	FlowStepHandle.Reset();
	AmountChangedHandle.Reset();
	ActiveBathers.Reset();
	RecoverySnapshot.Reset();
	WaterState = nullptr;
	OnConditionChangedNative.Clear();
	SetComponentTickEnabled(false);
	Super::EndPlay(EndPlayReason);
}

void UBathWaterConditionComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	EnsureWaterStateBound();
	if (bRecoveryFrozen || !WaterState || WaterState->GetNormalizedAmount() <= 0.0f
		|| !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f)
	{
		RefreshTickState();
		return;
	}

	const float OldActual = ActualTemperatureC;
	const float OldContamination = ContaminationPercent;
	const int32 BatherCount = GetActiveBatherCount();
	const float IncreaseRate = BatherCount * GetSafeNonNegative(ContaminationPerBatherPercentPointsPerSecond);
	const float CleaningRate = IsCirculationCapacitySatisfied()
		? GetSafeNonNegative(CleaningRateAtFullCirculationPercentPointsPerSecond)
			* FMath::Clamp(CirculationPercent, 0.0f, 100.0f) * 0.01f
		: 0.0f;
	ContaminationPercent = FMath::Clamp(
		ContaminationPercent + (IncreaseRate - CleaningRate) * DeltaTime,
		0.0f,
		100.0f);
	IntegrateTemperature(DeltaTime);
	BroadcastIfChanged(OldActual, OldContamination);
}

float UBathWaterConditionComponent::GetSafeNonNegative(const float Value)
{
	return FMath::IsFinite(Value) ? FMath::Max(0.0f, Value) : 0.0f;
}

float UBathWaterConditionComponent::GetThermalThresholdPercent() const
{
	const float MaxRate = GetSafeNonNegative(MaxTargetControlRateCPerSecond);
	if (MaxRate <= KINDA_SMALL_NUMBER)
	{
		return 100.0f;
	}
	return FMath::Clamp(GetSafeNonNegative(NaturalReturnRateCPerSecond) / MaxRate * 100.0f, 0.0f, 100.0f);
}

float UBathWaterConditionComponent::GetDemandPoints(const EBathWaterCapacityKind Kind) const
{
	const float Ambient = GetDefault<UBathWaterSettings>()->GetAmbientTemperatureC();
	switch (Kind)
	{
	case EBathWaterCapacityKind::Circulation:
		return GetMaxCirculationDemandPoints() * FMath::Clamp(CirculationPercent, 0.0f, 100.0f) * 0.01f;
	case EBathWaterCapacityKind::Heating:
		return FMath::Max(TargetTemperatureC - Ambient, 0.0f) * GetHeatingDemandPointsPerC();
	case EBathWaterCapacityKind::Cooling:
		return FMath::Max(Ambient - TargetTemperatureC, 0.0f) * GetCoolingDemandPointsPerC();
	default:
		return 0.0f;
	}
}

EBathWaterThermalStatus UBathWaterConditionComponent::GetThermalStatus() const
{
	if (!WaterState || WaterState->GetNormalizedAmount() <= 0.0f)
	{
		return EBathWaterThermalStatus::Empty;
	}
	const float Ambient = GetDefault<UBathWaterSettings>()->GetAmbientTemperatureC();
	const float Epsilon = FMath::Max(GetSafeNonNegative(TemperatureEpsilonC), UE_SMALL_NUMBER);
	const bool bTargetIsAmbient = FMath::IsNearlyEqual(TargetTemperatureC, Ambient, Epsilon);
	const bool bNeedsActive = !bTargetIsAmbient && CirculationPercent > 0.0f;
	if (bNeedsActive && (!IsCirculationCapacitySatisfied() || !IsThermalCapacitySatisfied()))
	{
		return EBathWaterThermalStatus::SuspendedByCapacity;
	}
	if (FMath::IsNearlyEqual(ActualTemperatureC, TargetTemperatureC, Epsilon))
	{
		return bTargetIsAmbient || FMath::IsNearlyZero(ComputeThermalVelocity(ActualTemperatureC, bNeedsActive), Epsilon)
			? EBathWaterThermalStatus::MaintainingTarget
			: EBathWaterThermalStatus::ReturningToAmbient;
	}
	const float Velocity = ComputeThermalVelocity(ActualTemperatureC, bNeedsActive
		&& IsCirculationCapacitySatisfied() && IsThermalCapacitySatisfied());
	if (FMath::IsNearlyZero(Velocity, Epsilon))
	{
		return EBathWaterThermalStatus::Stalled;
	}
	return FMath::Sign(Velocity) == FMath::Sign(TargetTemperatureC - ActualTemperatureC)
		? EBathWaterThermalStatus::MovingToTarget
		: EBathWaterThermalStatus::ReturningToAmbient;
}

int32 UBathWaterConditionComponent::GetActiveBatherCount()
{
	for (auto It = ActiveBathers.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
	return ActiveBathers.Num();
}

bool UBathWaterConditionComponent::RegisterActiveBather(UCustomerSessionComponent* Session)
{
	if (!IsValid(Session))
	{
		return false;
	}
	const int32 Before = ActiveBathers.Num();
	ActiveBathers.Add(Session);
	if (ActiveBathers.Num() != Before)
	{
		OnConditionChangedNative.Broadcast();
	}
	return true;
}

bool UBathWaterConditionComponent::UnregisterActiveBather(UCustomerSessionComponent* Session)
{
	if (!Session)
	{
		return false;
	}
	const bool bRemoved = ActiveBathers.Remove(Session) > 0;
	if (bRemoved)
	{
		OnConditionChangedNative.Broadcast();
	}
	return bRemoved;
}

void UBathWaterConditionComponent::ResetForPlacement()
{
	const float Ambient = GetDefault<UBathWaterSettings>()->GetAmbientTemperatureC();
	CirculationPercent = 0.0f;
	TargetTemperatureC = Ambient;
	ActualTemperatureC = Ambient;
	ContaminationPercent = 0.0f;
	ActiveBathers.Reset();
	RecoverySnapshot.Reset();
	bRecoveryFrozen = false;
	SetComponentTickEnabled(false);
	OnConditionChangedNative.Broadcast();
}

bool UBathWaterConditionComponent::BeginRecoveryFreeze(FText& OutFailureReason)
{
	if (bRecoveryFrozen || RecoverySnapshot.IsSet())
	{
		OutFailureReason = LOCTEXT("ConditionAlreadyFrozen", "욕탕 상태 회수 동결이 이미 시작되었습니다.");
		return false;
	}
	RecoverySnapshot.Emplace(FRecoverySnapshot{
		CirculationPercent,
		TargetTemperatureC,
		ActualTemperatureC,
		ContaminationPercent,
		IsComponentTickEnabled() });
	bRecoveryFrozen = true;
	SetComponentTickEnabled(false);
	return true;
}

void UBathWaterConditionComponent::CancelRecoveryFreeze()
{
	if (!RecoverySnapshot.IsSet())
	{
		return;
	}
	const FRecoverySnapshot Snapshot = RecoverySnapshot.GetValue();
	RecoverySnapshot.Reset();
	CirculationPercent = Snapshot.Circulation;
	TargetTemperatureC = Snapshot.Target;
	ActualTemperatureC = Snapshot.Actual;
	ContaminationPercent = Snapshot.Contamination;
	bRecoveryFrozen = false;
	SetComponentTickEnabled(Snapshot.bTickEnabled);
	RefreshTickState();
	OnConditionChangedNative.Broadcast();
}

void UBathWaterConditionComponent::PrepareRecoveryCommit()
{
	if (bRecoveryFrozen && RecoverySnapshot.IsSet())
	{
		SetComponentTickEnabled(false);
	}
}

bool UBathWaterConditionComponent::HasValidAuthoring(FText& OutFailureReason) const
{
	const float Values[] = {
		MaxCirculationDemandPoints,
		HeatingDemandPointsPerC,
		CoolingDemandPointsPerC,
		CleaningRateAtFullCirculationPercentPointsPerSecond,
		ContaminationPerBatherPercentPointsPerSecond,
		MaxTargetControlRateCPerSecond,
		NaturalReturnRateCPerSecond };
	for (const float Value : Values)
	{
		if (!FMath::IsFinite(Value) || Value < 0.0f)
		{
			OutFailureReason = LOCTEXT("InvalidConditionRate", "욕탕 수질·수온 수치와 요구량은 유한한 음이 아닌 값이어야 합니다.");
			return false;
		}
	}
	if (!FMath::IsFinite(TemperatureEpsilonC) || TemperatureEpsilonC <= 0.0f)
	{
		OutFailureReason = LOCTEXT("InvalidTemperatureEpsilon", "TemperatureEpsilonC는 유한한 양수여야 합니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

#if WITH_EDITOR
EDataValidationResult UBathWaterConditionComponent::IsDataValid(FDataValidationContext& Context) const
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

void UBathWaterConditionComponent::SetCirculationPercentFromSubsystem(const float NewPercent)
{
	const float Safe = FMath::Clamp(FMath::IsFinite(NewPercent) ? NewPercent : 0.0f, 0.0f, 100.0f);
	if (!FMath::IsNearlyEqual(CirculationPercent, Safe))
	{
		CirculationPercent = Safe;
		OnConditionChangedNative.Broadcast();
	}
}

void UBathWaterConditionComponent::SetTargetTemperatureFromSubsystem(const float NewTargetC)
{
	const float Safe = GetDefault<UBathWaterSettings>()->ClampAndQuantizeTargetTemperature(NewTargetC);
	if (!FMath::IsNearlyEqual(TargetTemperatureC, Safe))
	{
		TargetTemperatureC = Safe;
		OnConditionChangedNative.Broadcast();
	}
}

void UBathWaterConditionComponent::EnsureWaterStateBound()
{
	if (WaterState || !GetOwner())
	{
		return;
	}
	WaterState = GetOwner()->FindComponentByClass<UBathWaterStateComponent>();
	if (!WaterState)
	{
		return;
	}
	AddTickPrerequisiteComponent(WaterState);
	FlowStepHandle = WaterState->OnFlowStepNative.AddUObject(this, &UBathWaterConditionComponent::HandleFlowStep);
	AmountChangedHandle = WaterState->OnWaterAmountChangedNative.AddUObject(
		this, &UBathWaterConditionComponent::HandleWaterAmountChanged);
}

void UBathWaterConditionComponent::HandleFlowStep(const FBathWaterFlowStep& Step)
{
	if (bRecoveryFrozen || !FMath::IsFinite(Step.CurrentAmount)
		|| !FMath::IsFinite(Step.IncomingAmount) || !FMath::IsFinite(Step.OutgoingAmount))
	{
		return;
	}
	const float OldActual = ActualTemperatureC;
	const float OldContamination = ContaminationPercent;
	const float Ambient = GetDefault<UBathWaterSettings>()->GetAmbientTemperatureC();
	if (Step.CurrentAmount <= KINDA_SMALL_NUMBER)
	{
		ActualTemperatureC = Ambient;
		ContaminationPercent = 0.0f;
	}
	else if (Step.IncomingAmount > 0.0f)
	{
		const float RetainedOld = FMath::Max(0.0f, Step.PreviousAmount - Step.OutgoingAmount);
		ActualTemperatureC = (RetainedOld * ActualTemperatureC + Step.IncomingAmount * Ambient)
			/ Step.CurrentAmount;
		ContaminationPercent = FMath::Clamp(
			RetainedOld * ContaminationPercent / Step.CurrentAmount,
			0.0f,
			100.0f);
	}
	BroadcastIfChanged(OldActual, OldContamination);
	RefreshTickState();
}

void UBathWaterConditionComponent::HandleWaterAmountChanged(const float PreviousAmount, const float CurrentAmount)
{
	(void)PreviousAmount;
	if (CurrentAmount <= 0.0f)
	{
		const float OldActual = ActualTemperatureC;
		const float OldContamination = ContaminationPercent;
		ActualTemperatureC = GetDefault<UBathWaterSettings>()->GetAmbientTemperatureC();
		ContaminationPercent = 0.0f;
		BroadcastIfChanged(OldActual, OldContamination);
	}
	RefreshTickState();
}

void UBathWaterConditionComponent::RefreshTickState()
{
	SetComponentTickEnabled(!bRecoveryFrozen && WaterState && WaterState->GetNormalizedAmount() > 0.0f);
}

void UBathWaterConditionComponent::IntegrateTemperature(float DeltaTime)
{
	const float Ambient = GetDefault<UBathWaterSettings>()->GetAmbientTemperatureC();
	const float Epsilon = FMath::Max(GetSafeNonNegative(TemperatureEpsilonC), UE_SMALL_NUMBER);
	const bool bActiveAvailable = CirculationPercent > 0.0f
		&& IsCirculationCapacitySatisfied() && IsThermalCapacitySatisfied()
		&& !FMath::IsNearlyEqual(TargetTemperatureC, Ambient, Epsilon);
	for (int32 Iteration = 0; Iteration < 8 && DeltaTime > UE_SMALL_NUMBER; ++Iteration)
	{
		const float Velocity = ComputeThermalVelocity(ActualTemperatureC, bActiveAvailable);
		if (!FMath::IsFinite(Velocity) || FMath::IsNearlyZero(Velocity, Epsilon))
		{
			break;
		}
		float Boundary = Velocity > 0.0f ? TNumericLimits<float>::Max() : TNumericLimits<float>::Lowest();
		auto ConsiderBoundary = [&](const float Candidate)
		{
			if ((Velocity > 0.0f && Candidate > ActualTemperatureC + Epsilon && Candidate < Boundary)
				|| (Velocity < 0.0f && Candidate < ActualTemperatureC - Epsilon && Candidate > Boundary))
			{
				Boundary = Candidate;
			}
		};
		ConsiderBoundary(Ambient);
		ConsiderBoundary(TargetTemperatureC);
		const bool bHasBoundary = Velocity > 0.0f
			? Boundary != TNumericLimits<float>::Max()
			: Boundary != TNumericLimits<float>::Lowest();
		if (bHasBoundary)
		{
			const float TimeToBoundary = FMath::Abs((Boundary - ActualTemperatureC) / Velocity);
			if (TimeToBoundary <= DeltaTime)
			{
				ActualTemperatureC = Boundary;
				DeltaTime -= TimeToBoundary;
				continue;
			}
		}
		ActualTemperatureC += Velocity * DeltaTime;
		DeltaTime = 0.0f;
	}
	if (FMath::IsNearlyEqual(ActualTemperatureC, Ambient, Epsilon))
	{
		ActualTemperatureC = Ambient;
	}
	if (FMath::IsNearlyEqual(ActualTemperatureC, TargetTemperatureC, Epsilon))
	{
		ActualTemperatureC = TargetTemperatureC;
	}
}

float UBathWaterConditionComponent::ComputeThermalVelocity(
	const float TemperatureC,
	const bool bActiveAvailable) const
{
	const float Ambient = GetDefault<UBathWaterSettings>()->GetAmbientTemperatureC();
	const float Epsilon = FMath::Max(GetSafeNonNegative(TemperatureEpsilonC), UE_SMALL_NUMBER);
	const float NaturalRate = GetSafeNonNegative(NaturalReturnRateCPerSecond);
	const float ActiveRate = bActiveAvailable
		? GetSafeNonNegative(MaxTargetControlRateCPerSecond) * FMath::Clamp(CirculationPercent, 0.0f, 100.0f) * 0.01f
		: 0.0f;
	if (FMath::IsNearlyEqual(TargetTemperatureC, Ambient, Epsilon) || !bActiveAvailable)
	{
		return FMath::IsNearlyEqual(TemperatureC, Ambient, Epsilon)
			? 0.0f : FMath::Sign(Ambient - TemperatureC) * NaturalRate;
	}
	const float TargetDirection = FMath::Sign(TargetTemperatureC - TemperatureC);
	const float AmbientDirection = FMath::Sign(Ambient - TemperatureC);
	if (FMath::IsNearlyEqual(TemperatureC, Ambient, Epsilon))
	{
		return FMath::Sign(TargetTemperatureC - Ambient) * FMath::Max(0.0f, ActiveRate - NaturalRate);
	}
	if (FMath::IsNearlyEqual(TemperatureC, TargetTemperatureC, Epsilon))
	{
		return AmbientDirection * FMath::Max(0.0f, NaturalRate - ActiveRate);
	}
	return TargetDirection * ActiveRate + AmbientDirection * NaturalRate;
}

bool UBathWaterConditionComponent::IsCirculationCapacitySatisfied() const
{
	UBathWaterOperationsSubsystem* Operations = GetWorld()
		? GetWorld()->GetSubsystem<UBathWaterOperationsSubsystem>() : nullptr;
	return Operations && Operations->IsCapacitySatisfied(EBathWaterCapacityKind::Circulation);
}

bool UBathWaterConditionComponent::IsThermalCapacitySatisfied() const
{
	const float Ambient = GetDefault<UBathWaterSettings>()->GetAmbientTemperatureC();
	UBathWaterOperationsSubsystem* Operations = GetWorld()
		? GetWorld()->GetSubsystem<UBathWaterOperationsSubsystem>() : nullptr;
	if (!Operations)
	{
		return false;
	}
	if (TargetTemperatureC > Ambient)
	{
		return Operations->IsCapacitySatisfied(EBathWaterCapacityKind::Heating);
	}
	if (TargetTemperatureC < Ambient)
	{
		return Operations->IsCapacitySatisfied(EBathWaterCapacityKind::Cooling);
	}
	return true;
}

void UBathWaterConditionComponent::BroadcastIfChanged(
	const float OldActual,
	const float OldContamination)
{
	if (!FMath::IsNearlyEqual(OldActual, ActualTemperatureC)
		|| !FMath::IsNearlyEqual(OldContamination, ContaminationPercent))
	{
		OnConditionChangedNative.Broadcast();
	}
}

#undef LOCTEXT_NAMESPACE
