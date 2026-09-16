#include "Customer/CustomerSessionComponent.h"

#include "Customer/CustomerBathLoopLog.h"
#include "Customer/CustomerRoutineDefinition.h"
#include "Engine/World.h"
#include "Facility/BathWaterSettings.h"
#include "Facility/BathWaterStateComponent.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "TimerManager.h"

bool UCustomerSessionComponent::StartBathStay()
{
	if (bBathStayStarted || !RoutineDefinition || !GetWorld())
	{
		return bBathStayStarted;
	}
	bBathStayStarted = true;
	bBathStayExpired = false;
	bBathSearchExpired = false;
	BathSearchResolvedDurationSeconds = 0.0f;
	ActualBathAccumulatedSeconds = 0.0f;
	const float Duration = FMath::Max(RoutineDefinition->BathStayDurationSeconds, 0.1f);
	BathStayEndTime = GetWorld()->GetTimeSeconds() + Duration;
	GetWorld()->GetTimerManager().SetTimer(
		BathStayTimerHandle,
		this,
		&UCustomerSessionComponent::HandleBathStayExpired,
		Duration,
		false);
	UE_LOG(LogBathhouseCustomerBath, Log,
		TEXT("Customer=%s Iteration=%llu Phase=BathStayBegin BathActor=None Slot=None WaterPercent=0 ThresholdPercent=%.2f BathStayRemaining=%.2f ActualBathSeconds=0 SearchElapsed=0 Result=Started Reason=None"),
		*GetNameSafe(GetOwner()), BathLoopIteration,
		GetDefault<UBathWaterSettings>()->GetCustomerUsableThresholdPercent(), Duration);
	return true;
}

float UCustomerSessionComponent::GetRemainingBathStaySeconds() const
{
	if (bRoutineTimersPaused && bPausedBathStayTimer)
	{
		return PausedBathStayRemainingSeconds;
	}
	return bBathStayStarted && !bBathStayExpired && GetWorld()
		? FMath::Max(0.0, BathStayEndTime - GetWorld()->GetTimeSeconds())
		: 0.0f;
}

bool UCustomerSessionComponent::BeginBathSearchWindow()
{
	if (bBathSearchActive)
	{
		return true;
	}
	if (!RoutineDefinition || !GetWorld() || !bBathStayStarted || bBathStayExpired)
	{
		return false;
	}
	const float Duration = FMath::Min(
		FMath::Max(0.1f, RoutineDefinition->BathSearchTimeoutSeconds),
		GetRemainingBathStaySeconds());
	if (Duration <= 0.0f)
	{
		HandleBathStayExpired();
		return false;
	}
	bBathSearchActive = true;
	bBathSearchExpired = false;
	BathSearchResolvedDurationSeconds = Duration;
	++BathSearchSerial;
	++BathLoopIteration;
	BathSearchEndTime = GetWorld()->GetTimeSeconds() + Duration;
	GetWorld()->GetTimerManager().SetTimer(
		BathSearchTimerHandle,
		this,
		&UCustomerSessionComponent::HandleBathSearchExpired,
		Duration,
		false);
	UE_LOG(LogBathhouseCustomerBath, Log,
		TEXT("Customer=%s Iteration=%llu Phase=SearchBegin BathActor=None Slot=None WaterPercent=0 ThresholdPercent=%.2f BathStayRemaining=%.2f ActualBathSeconds=%.2f SearchElapsed=0 Result=Started Reason=None"),
		*GetNameSafe(GetOwner()), BathLoopIteration,
		GetDefault<UBathWaterSettings>()->GetCustomerUsableThresholdPercent(),
		GetRemainingBathStaySeconds(), GetActualBathSeconds());
	return true;
}

void UCustomerSessionComponent::CompleteBathSearchWindow()
{
	if (!bBathSearchActive)
	{
		return;
	}
	const float SearchElapsed = GetBathSearchElapsedSeconds();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BathSearchTimerHandle);
	}
	bBathSearchActive = false;
	bBathSearchExpired = false;
	bPausedBathSearchTimer = false;
	PausedBathSearchRemainingSeconds = 0.0f;
	UE_LOG(LogBathhouseCustomerBath, Log,
		TEXT("Customer=%s Iteration=%llu Phase=SearchEnd BathActor=%s Slot=%s WaterPercent=%.2f ThresholdPercent=%.2f BathStayRemaining=%.2f ActualBathSeconds=%.2f SearchElapsed=%.2f Result=Reserved Reason=None"),
		*GetNameSafe(GetOwner()), BathLoopIteration, *GetNameSafe(CurrentFacilityActor), *GetNameSafe(CurrentFacilitySlot),
		BoundBathWaterState ? BoundBathWaterState->GetWaterPercent() : 0.0f,
		GetDefault<UBathWaterSettings>()->GetCustomerUsableThresholdPercent(),
		GetRemainingBathStaySeconds(), GetActualBathSeconds(), SearchElapsed);
	BathSearchResolvedDurationSeconds = 0.0f;
}

void UCustomerSessionComponent::CancelBathSearchWindow()
{
	if (!bBathSearchActive && !bPausedBathSearchTimer)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BathSearchTimerHandle);
	}
	bBathSearchActive = false;
	bPausedBathSearchTimer = false;
	PausedBathSearchRemainingSeconds = 0.0f;
	BathSearchResolvedDurationSeconds = 0.0f;
}

float UCustomerSessionComponent::GetRemainingBathSearchSeconds() const
{
	if (bRoutineTimersPaused && bPausedBathSearchTimer)
	{
		return PausedBathSearchRemainingSeconds;
	}
	return bBathSearchActive && GetWorld()
		? FMath::Max(0.0, BathSearchEndTime - GetWorld()->GetTimeSeconds())
		: 0.0f;
}

float UCustomerSessionComponent::GetBathSearchElapsedSeconds() const
{
	if (!FMath::IsFinite(BathSearchResolvedDurationSeconds)
		|| BathSearchResolvedDurationSeconds <= 0.0f
		|| (!bBathSearchActive && !bPausedBathSearchTimer && !bBathSearchExpired))
	{
		return 0.0f;
	}
	return FMath::Clamp(
		BathSearchResolvedDurationSeconds - GetRemainingBathSearchSeconds(),
		0.0f,
		BathSearchResolvedDurationSeconds);
}

float UCustomerSessionComponent::GetActualBathSeconds() const
{
	const double ActiveDelta = bActualBathSegmentActive && GetWorld()
		? FMath::Max(0.0, GetWorld()->GetTimeSeconds() - ActualBathSegmentStartTime)
		: 0.0;
	return ActualBathAccumulatedSeconds + static_cast<float>(ActiveDelta);
}

bool UCustomerSessionComponent::IsCurrentBathUsable() const
{
	return CurrentFacilityActor && CurrentFacilityActor->GetFacilityType() == EBathhouseFacilityType::Bath
		&& BoundBathWaterState && BoundBathWaterState->IsCustomerUsable()
		&& !BoundBathWaterState->IsRecoveryFrozen();
}

bool UCustomerSessionComponent::HasCurrentBathFacility() const
{
	return CurrentFacilityActor
		&& CurrentFacilityActor->GetFacilityType() == EBathhouseFacilityType::Bath;
}

FString UCustomerSessionComponent::BuildBathLoopDiagnosticLine(
	const TCHAR* Phase,
	const TCHAR* Result,
	const TOptional<ECustomerBathLoopReason> Reason,
	const float SearchElapsed) const
{
	return FString::Printf(
		TEXT("Customer=%s Iteration=%llu Phase=%s BathActor=%s Slot=%s WaterPercent=%.2f ThresholdPercent=%.2f BathStayRemaining=%.2f ActualBathSeconds=%.2f SearchElapsed=%.2f Result=%s Reason=%s"),
		*GetNameSafe(GetOwner()),
		BathLoopIteration,
		Phase,
		*GetNameSafe(CurrentFacilityActor),
		*GetNameSafe(CurrentFacilitySlot),
		BoundBathWaterState ? BoundBathWaterState->GetWaterPercent() : 0.0f,
		GetDefault<UBathWaterSettings>()->GetCustomerUsableThresholdPercent(),
		GetRemainingBathStaySeconds(),
		GetActualBathSeconds(),
		FMath::Max(0.0f, SearchElapsed),
		Result,
		Reason.IsSet() ? LexToString(Reason.GetValue()) : TEXT("None"));
}

void UCustomerSessionComponent::LogBathLoopStateTreeEvent(
	const ECustomerBathLoopLogLevel Level,
	const TCHAR* Phase,
	const TCHAR* Result,
	const TOptional<ECustomerBathLoopReason> Reason) const
{
	const FString Message = BuildBathLoopDiagnosticLine(
		Phase,
		Result,
		Reason,
		GetBathSearchElapsedSeconds());
	switch (Level)
	{
	case ECustomerBathLoopLogLevel::Warning:
		UE_LOG(LogBathhouseCustomerBath, Warning, TEXT("%s"), *Message);
		break;
	case ECustomerBathLoopLogLevel::Log:
		UE_LOG(LogBathhouseCustomerBath, Log, TEXT("%s"), *Message);
		break;
	default:
		UE_LOG(LogBathhouseCustomerBath, Verbose, TEXT("%s"), *Message);
		break;
	}
}

void UCustomerSessionComponent::LogBathForcedExitCleanup(const bool bApproachReturned) const
{
	LogBathLoopStateTreeEvent(
		bApproachReturned ? ECustomerBathLoopLogLevel::Log : ECustomerBathLoopLogLevel::Warning,
		TEXT("ForcedExitCleanup"),
		bApproachReturned ? TEXT("Succeeded") : TEXT("Failed"),
		CurrentBathExitReason.IsSet()
			? CurrentBathExitReason
			: TOptional<ECustomerBathLoopReason>(ECustomerBathLoopReason::StateTreeExited));
}

bool UCustomerSessionComponent::BeginActualBathSegment()
{
	if (bActualBathSegmentActive)
	{
		return true;
	}
	if (!GetWorld() || bCurrentBathExitPending || !IsCurrentBathUsable()
		|| !bSnappedToFacilityActionPoint
		|| !CurrentFacilitySlot
		|| CurrentFacilitySlot->GetSlotState() != EBathhouseFacilitySlotState::Occupied)
	{
		return false;
	}
	ActualBathSegmentStartTime = GetWorld()->GetTimeSeconds();
	bActualBathSegmentActive = true;
	UE_LOG(LogBathhouseCustomerBath, Log,
		TEXT("Customer=%s Iteration=%llu Phase=DwellBegin BathActor=%s Slot=%s WaterPercent=%.2f ThresholdPercent=%.2f BathStayRemaining=%.2f ActualBathSeconds=%.2f SearchElapsed=0 Result=Started Reason=None"),
		*GetNameSafe(GetOwner()), BathLoopIteration, *GetNameSafe(CurrentFacilityActor), *GetNameSafe(CurrentFacilitySlot),
		BoundBathWaterState->GetWaterPercent(), GetDefault<UBathWaterSettings>()->GetCustomerUsableThresholdPercent(),
		GetRemainingBathStaySeconds(), ActualBathAccumulatedSeconds);
	return true;
}

void UCustomerSessionComponent::EndActualBathSegment(const ECustomerBathLoopReason Reason)
{
	if (!bActualBathSegmentActive)
	{
		return;
	}
	if (GetWorld())
	{
		ActualBathAccumulatedSeconds += static_cast<float>(FMath::Max(
			0.0,
			GetWorld()->GetTimeSeconds() - ActualBathSegmentStartTime));
	}
	bActualBathSegmentActive = false;
	ActualBathSegmentStartTime = 0.0;
	UE_LOG(LogBathhouseCustomerBath, Log,
		TEXT("Customer=%s Iteration=%llu Phase=DwellEnd BathActor=%s Slot=%s WaterPercent=%.2f ThresholdPercent=%.2f BathStayRemaining=%.2f ActualBathSeconds=%.2f SearchElapsed=0 Result=Stopped Reason=%s"),
		*GetNameSafe(GetOwner()), BathLoopIteration, *GetNameSafe(CurrentFacilityActor), *GetNameSafe(CurrentFacilitySlot),
		BoundBathWaterState ? BoundBathWaterState->GetWaterPercent() : 0.0f,
		GetDefault<UBathWaterSettings>()->GetCustomerUsableThresholdPercent(),
		GetRemainingBathStaySeconds(), ActualBathAccumulatedSeconds, LexToString(Reason));
}

void UCustomerSessionComponent::BindCurrentBathWater()
{
	UnbindCurrentBathWater();
	if (!CurrentFacilityActor || CurrentFacilityActor->GetFacilityType() != EBathhouseFacilityType::Bath)
	{
		return;
	}
	BoundBathWaterState = CurrentFacilityActor->GetBathWaterState();
	if (BoundBathWaterState)
	{
		CurrentBathUsabilityHandle = BoundBathWaterState->OnCustomerUsabilityChangedNative.AddUObject(
			this, &UCustomerSessionComponent::HandleCurrentBathUsabilityChanged);
	}
}

void UCustomerSessionComponent::UnbindCurrentBathWater()
{
	if (BoundBathWaterState && CurrentBathUsabilityHandle.IsValid())
	{
		BoundBathWaterState->OnCustomerUsabilityChangedNative.Remove(CurrentBathUsabilityHandle);
	}
	CurrentBathUsabilityHandle.Reset();
	BoundBathWaterState = nullptr;
}

void UCustomerSessionComponent::CommitCurrentBathInvalidation(const ECustomerBathLoopReason Reason)
{
	if (bCurrentBathExitPending || !CurrentFacilityActor
		|| CurrentFacilityActor->GetFacilityType() != EBathhouseFacilityType::Bath)
	{
		return;
	}
	bCurrentBathExitPending = true;
	CurrentBathExitReason = Reason;
	EndActualBathSegment(Reason);
	UE_LOG(LogBathhouseCustomerBath, Log,
		TEXT("Customer=%s Iteration=%llu Phase=ForcedExit BathActor=%s Slot=%s WaterPercent=%.2f ThresholdPercent=%.2f BathStayRemaining=%.2f ActualBathSeconds=%.2f SearchElapsed=0 Result=Pending Reason=%s"),
		*GetNameSafe(GetOwner()), BathLoopIteration, *GetNameSafe(CurrentFacilityActor), *GetNameSafe(CurrentFacilitySlot),
		BoundBathWaterState ? BoundBathWaterState->GetWaterPercent() : 0.0f,
		GetDefault<UBathWaterSettings>()->GetCustomerUsableThresholdPercent(),
		GetRemainingBathStaySeconds(), GetActualBathSeconds(), LexToString(Reason));
	SendCustomerEvent(TAG_Customer_Event_BathBecameUnusable);
}

void UCustomerSessionComponent::HandleCurrentBathUsabilityChanged(const bool bUsable)
{
	if (!bUsable)
	{
		CommitCurrentBathInvalidation(ECustomerBathLoopReason::WaterBelowThreshold);
	}
}

void UCustomerSessionComponent::HandleBathStayExpired()
{
	if (bBathStayExpired)
	{
		return;
	}
	const float SearchElapsed = GetBathSearchElapsedSeconds();
	bBathStayExpired = true;
	CancelBathSearchWindow();
	EndActualBathSegment(ECustomerBathLoopReason::BathStayExpired);
	UE_LOG(LogBathhouseCustomerBath, Log,
		TEXT("Customer=%s Iteration=%llu Phase=BathStayEnd BathActor=%s Slot=%s WaterPercent=%.2f ThresholdPercent=%.2f BathStayRemaining=0 ActualBathSeconds=%.2f SearchElapsed=%.2f Result=Expired Reason=%s"),
		*GetNameSafe(GetOwner()), BathLoopIteration, *GetNameSafe(CurrentFacilityActor), *GetNameSafe(CurrentFacilitySlot),
		BoundBathWaterState ? BoundBathWaterState->GetWaterPercent() : 0.0f,
		GetDefault<UBathWaterSettings>()->GetCustomerUsableThresholdPercent(),
		GetActualBathSeconds(), SearchElapsed, LexToString(ECustomerBathLoopReason::BathStayExpired));
	SendCustomerEvent(TAG_Customer_Event_BathStayExpired);
}

void UCustomerSessionComponent::HandleBathSearchExpired()
{
	if (!bBathSearchActive || bBathSearchExpired)
	{
		return;
	}
	const float SearchElapsed = GetBathSearchElapsedSeconds();
	bBathSearchActive = false;
	bBathSearchExpired = true;
	bPausedBathSearchTimer = false;
	PausedBathSearchRemainingSeconds = 0.0f;
	UE_LOG(LogBathhouseCustomerBath, Log,
		TEXT("Customer=%s Iteration=%llu Phase=SearchEnd BathActor=None Slot=None WaterPercent=0 ThresholdPercent=%.2f BathStayRemaining=%.2f ActualBathSeconds=%.2f SearchElapsed=%.2f Result=Expired Reason=%s"),
		*GetNameSafe(GetOwner()), BathLoopIteration,
		GetDefault<UBathWaterSettings>()->GetCustomerUsableThresholdPercent(),
		GetRemainingBathStaySeconds(), GetActualBathSeconds(), SearchElapsed,
		LexToString(ECustomerBathLoopReason::SearchExpired));
	SendCustomerEvent(TAG_Customer_Event_BathSearchExpired);
}
