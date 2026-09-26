#include "Utility/UtilityOperationComponent.h"

#include "Facility/BathWaterUtilityFacilityActor.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Misc/ScopeExit.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "UtilityOperationComponent"

UUtilityOperationComponent::UUtilityOperationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UUtilityOperationComponent::BeginPlay()
{
	Super::BeginPlay();
	// Imported state is established before FinishSpawning. BeginPlay deliberately
	// does not reset the state, so a staged recovery item remains authoritative.
	AnchorGameTimeSeconds = GetGameTimeSeconds();
	LastPublishedPoints = AnchorPoints;
	bLastPublishedOperating = bPlacedClockActive && AnchorPoints > 0.0f;
	UpdateTickEnabled();
}

void UUtilityOperationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bPlacedClockActive = false;
	bLaborBlocked = true;
	SetComponentTickEnabled(false);
	OnOperationChanged.Clear();
	OnOperatingChanged.Clear();
	Super::EndPlay(EndPlayReason);
}

void UUtilityOperationComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bPlacedClockActive || bMutationInProgress)
	{
		return;
	}

	const float CurrentPoints = GetRemainingPoints();
	const float PreviousPublished = LastPublishedPoints;
	if (CurrentPoints <= 0.0f)
	{
		AnchorPoints = 0.0f;
		AnchorGameTimeSeconds = GetGameTimeSeconds();
		SetComponentTickEnabled(false);
	}
	if (CurrentPoints != PreviousPublished)
	{
		++Revision;
		const TWeakObjectPtr<UUtilityOperationComponent> WeakThis(this);
		TGuardValue<bool> MutationGuard(bMutationInProgress, true);
		PublishCommittedChanges(PreviousPublished, bLastPublishedOperating);
		if (!WeakThis.IsValid())
		{
			return;
		}
	}
	UpdateTickEnabled();
}

float UUtilityOperationComponent::GetRemainingPoints() const
{
	return GetRemainingPointsAt(GetGameTimeSeconds());
}

FUtilityOperationSnapshot UUtilityOperationComponent::GetOperationSnapshot() const
{
	FUtilityOperationSnapshot Snapshot;
	Snapshot.RemainingPoints = GetRemainingPoints();
	Snapshot.MaximumPoints = MaxOperationPoints;
	Snapshot.DecayPointsPerSecond = DecayPointsPerSecond;
	Snapshot.bPlacedClockActive = bPlacedClockActive;
	Snapshot.bIsOperating = IsProvidingCapacity();
	Snapshot.Revision = Revision;
	return Snapshot;
}

bool UUtilityOperationComponent::IsProvidingCapacity() const
{
	return bPlacedClockActive && GetRemainingPoints() > 0.0f;
}

bool UUtilityOperationComponent::HasValidAuthoring(FText& OutFailureReason) const
{
	if (!FMath::IsFinite(MaxOperationPoints) || MaxOperationPoints <= 0.0f
		|| !FMath::IsFinite(DecayPointsPerSecond) || DecayPointsPerSecond < 0.0f)
	{
		OutFailureReason = LOCTEXT("InvalidOperationAuthoring", "설비의 최대 가동수치와 감소 속도가 올바르지 않습니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UUtilityOperationComponent::CanAcceptFuel(
	const FUtilityFuelLoad& Load,
	FText& OutFailureReason) const
{
	if (!HasValidAuthoring(OutFailureReason) || !Load.IsValid() || Load.IsEmpty())
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT("InvalidFuelLoad", "삽의 재료와 양이 올바르지 않습니다.");
		}
		return false;
	}
	if (!bPlacedClockActive)
	{
		OutFailureReason = LOCTEXT("OperationNotInstalled", "설치된 설비에만 재료를 넣을 수 있습니다.");
		return false;
	}
	if (bLaborBlocked)
	{
		OutFailureReason = LOCTEXT("OperationRecoveryBlocked", "회수 중에는 재료를 넣을 수 없습니다.");
		return false;
	}
	if (bMutationInProgress)
	{
		OutFailureReason = LOCTEXT("OperationBusy", "재료 이동이 이미 처리 중입니다.");
		return false;
	}
	if (GetRemainingPoints() >= MaxOperationPoints)
	{
		OutFailureReason = LOCTEXT("OperationAtCapacity", "설비가 가득 차 있습니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UUtilityOperationComponent::ApplyLaborReward(const float Points, FText& OutFailureReason)
{
	if (!FMath::IsFinite(Points) || Points <= 0.0f)
	{
		OutFailureReason = LOCTEXT("InvalidLaborReward", "노동 보상은 유한한 양수여야 합니다.");
		return false;
	}
	const ABathWaterUtilityFacilityActor* Facility = Cast<ABathWaterUtilityFacilityActor>(GetOwner());
	const UFacilityPlacementComponent* Placement = Facility
		? Facility->GetFacilityPlacementComponent() : nullptr;
	if (!Placement || Placement->GetMode() != EPlaceableFacilityMode::Placed
		|| Placement->IsStagedPlacement() || !Placement->IsPlacedDomainActive() || !bPlacedClockActive)
	{
		OutFailureReason = LOCTEXT("LaborRewardFacilityInactive", "설치되어 가동 중인 설비에만 노동 보상을 적용할 수 있습니다.");
		return false;
	}
	if (bLaborBlocked)
	{
		OutFailureReason = LOCTEXT("LaborRewardBlocked", "회수 중에는 노동 보상을 적용할 수 없습니다.");
		return false;
	}
	if (!HasValidAuthoring(OutFailureReason))
	{
		return false;
	}
	if (!TryAcquireMutationGuard())
	{
		OutFailureReason = LOCTEXT("LaborRewardBusy", "설비 가동 상태를 변경하는 중입니다.");
		return false;
	}
	ON_SCOPE_EXIT
	{
		ReleaseMutationGuard();
	};

	const float PreviousPublishedPoints = LastPublishedPoints;
	const bool bWasProvidingCapacity = bPlacedClockActive && GetRemainingPoints() > 0.0f;
	double GameTimeSeconds = 0.0;
	const float CurrentPoints = SettleToCurrentTimeSilently(GameTimeSeconds);
	const float RewardedPoints = FMath::Min(MaxOperationPoints, CurrentPoints + Points);
	SetRemainingPointsSilently(RewardedPoints, GameTimeSeconds);
	PublishCommittedChanges(PreviousPublishedPoints, bWasProvidingCapacity);
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UUtilityOperationComponent::ImportOperationState(
	const float RemainingPoints,
	FText& OutFailureReason)
{
	if (bPlacedClockActive || bMutationInProgress || bOperationStateImported || !HasValidAuthoring(OutFailureReason)
		|| !FMath::IsFinite(RemainingPoints) || RemainingPoints < 0.0f
		|| RemainingPoints > MaxOperationPoints)
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT("InvalidImportedOperationState", "회수된 설비의 가동수치가 유효하지 않습니다.");
		}
		return false;
	}
	AnchorPoints = RemainingPoints;
	AnchorGameTimeSeconds = GetGameTimeSeconds();
	LastPublishedPoints = RemainingPoints;
	bOperationStateImported = true;
	++Revision;
	UpdateTickEnabled();
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UUtilityOperationComponent::StartPlacedClock(const bool bPublishOperatingTransition)
{
	FText FailureReason;
	if (!HasValidAuthoring(FailureReason))
	{
		return false;
	}
	if (bPlacedClockActive)
	{
		return true;
	}
	const float PreviousPublishedPoints = LastPublishedPoints;
	const bool bWasProvidingCapacity = IsProvidingCapacity();
	AnchorGameTimeSeconds = GetGameTimeSeconds();
	bPlacedClockActive = true;
	const float CurrentPoints = GetRemainingPoints();
	if (bPublishOperatingTransition)
	{
		PublishCommittedChanges(PreviousPublishedPoints, bWasProvidingCapacity);
	}
	else
	{
		LastPublishedPoints = CurrentPoints;
		bLastPublishedOperating = CurrentPoints > 0.0f;
	}
	UpdateTickEnabled();
	return true;
}

void UUtilityOperationComponent::StopPlacedClock(const bool bPublishOperatingTransition)
{
	if (!bPlacedClockActive)
	{
		return;
	}
	const float PreviousPublished = LastPublishedPoints;
	const bool bWasProvidingCapacity = IsProvidingCapacity();
	const double Now = GetGameTimeSeconds();
	AnchorPoints = GetRemainingPointsAt(Now);
	AnchorGameTimeSeconds = Now;
	bPlacedClockActive = false;
	SetComponentTickEnabled(false);
	++Revision;
	if (bPublishOperatingTransition)
	{
		PublishCommittedChanges(PreviousPublished, bWasProvidingCapacity);
	}
}

float UUtilityOperationComponent::CalculateRemainingPoints(
	const float InAnchorPoints,
	const float InDecayRate,
	const double ElapsedSeconds)
{
	if (!FMath::IsFinite(InAnchorPoints) || InAnchorPoints <= 0.0f
		|| !FMath::IsFinite(InDecayRate) || InDecayRate <= 0.0f
		|| !FMath::IsFinite(ElapsedSeconds) || ElapsedSeconds <= 0.0)
	{
		return FMath::IsFinite(InAnchorPoints) ? FMath::Max(0.0f, InAnchorPoints) : 0.0f;
	}
	const double Remaining = static_cast<double>(InAnchorPoints)
		- static_cast<double>(InDecayRate) * ElapsedSeconds;
	return static_cast<float>(FMath::Max(0.0, Remaining));
}

double UUtilityOperationComponent::GetGameTimeSeconds() const
{
	return GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : AnchorGameTimeSeconds;
}

float UUtilityOperationComponent::GetRemainingPointsAt(const double GameTimeSeconds) const
{
	if (!bPlacedClockActive)
	{
		return FMath::IsFinite(AnchorPoints) ? FMath::Clamp(AnchorPoints, 0.0f, MaxOperationPoints) : 0.0f;
	}
	return CalculateRemainingPoints(
		AnchorPoints,
		DecayPointsPerSecond,
		FMath::Max(0.0, GameTimeSeconds - AnchorGameTimeSeconds));
}

float UUtilityOperationComponent::SettleToCurrentTimeSilently(double& OutGameTimeSeconds)
{
	OutGameTimeSeconds = GetGameTimeSeconds();
	const float CurrentPoints = GetRemainingPointsAt(OutGameTimeSeconds);
	if (bPlacedClockActive)
	{
		AnchorPoints = CurrentPoints;
		AnchorGameTimeSeconds = OutGameTimeSeconds;
	}
	else
	{
		AnchorGameTimeSeconds = OutGameTimeSeconds;
	}
	return CurrentPoints;
}

bool UUtilityOperationComponent::TryAcquireMutationGuard()
{
	if (bMutationInProgress)
	{
		return false;
	}
	bMutationInProgress = true;
	return true;
}

void UUtilityOperationComponent::ReleaseMutationGuard()
{
	bMutationInProgress = false;
}

void UUtilityOperationComponent::SetRemainingPointsSilently(
	const float Points,
	const double GameTimeSeconds)
{
	AnchorPoints = FMath::Clamp(Points, 0.0f, MaxOperationPoints);
	AnchorGameTimeSeconds = GameTimeSeconds;
	++Revision;
	UpdateTickEnabled();
}

void UUtilityOperationComponent::PublishCommittedChanges(
	const float PreviousPoints,
	const bool bPreviousOperating)
{
	const float CurrentPoints = GetRemainingPoints();
	const bool bCurrentOperating = bPlacedClockActive && CurrentPoints > 0.0f;
	const bool bPointsChanged = CurrentPoints != PreviousPoints;
	const bool bOperatingChanged = bCurrentOperating != bPreviousOperating;
	LastPublishedPoints = CurrentPoints;
	bLastPublishedOperating = bCurrentOperating;
	const TWeakObjectPtr<UUtilityOperationComponent> WeakThis(this);
	if (bPointsChanged)
	{
		OnOperationChanged.Broadcast();
	}
	if (WeakThis.IsValid() && bOperatingChanged)
	{
		OnOperatingChanged.Broadcast(bCurrentOperating);
	}
}

void UUtilityOperationComponent::UpdateTickEnabled()
{
	SetComponentTickEnabled(bPlacedClockActive && GetRemainingPoints() > 0.0f);
}

#if WITH_EDITOR
EDataValidationResult UUtilityOperationComponent::IsDataValid(FDataValidationContext& Context) const
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
