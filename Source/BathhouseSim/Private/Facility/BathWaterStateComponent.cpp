#include "Facility/BathWaterStateComponent.h"

#include "Facility/BathWaterSettings.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BathWaterStateComponent"

UBathWaterStateComponent::UBathWaterStateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;
}

void UBathWaterStateComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bRecoveryFrozen || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f)
	{
		RefreshTickState();
		return;
	}

	const float NetPercentPerSecond = GetNetPercentPerSecond();
	float NewAmount = FMath::Clamp(
		NormalizedAmount + NetPercentPerSecond * 0.01f * DeltaTime,
		0.0f,
		1.0f);
	if (NewAmount <= KINDA_SMALL_NUMBER)
	{
		NewAmount = 0.0f;
	}
	else if (NewAmount >= 1.0f - KINDA_SMALL_NUMBER)
	{
		NewAmount = 1.0f;
	}
	CommitAmount(NewAmount);

	if (NormalizedAmount == 1.0f && bFillValveOpen)
	{
		CommitControlState(
			EBathWaterControlType::FillValve,
			false,
			EBathWaterControlChangeReason::FullAutoClose);
	}
	CommitDerivedState();
	RefreshTickState();
}

void UBathWaterStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetComponentTickEnabled(false);
	RecoverySnapshot.Reset();
	bRecoveryFrozen = false;
	OnWaterAmountChangedNative.Clear();
	OnWaterStateChangedNative.Clear();
	OnCustomerUsabilityChangedNative.Clear();
	OnControlChangedNative.Clear();
	OnRecoveryFreezeChangedNative.Clear();
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
EDataValidationResult UBathWaterStateComponent::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!FMath::IsFinite(FillRatePercentPerSecond) || FillRatePercentPerSecond < 0.0f)
	{
		Context.AddError(LOCTEXT("InvalidFillRate", "FillRatePercentPerSecond must be finite and non-negative."));
		Result = EDataValidationResult::Invalid;
	}
	if (!FMath::IsFinite(DrainRatePercentPerSecond) || DrainRatePercentPerSecond < 0.0f)
	{
		Context.AddError(LOCTEXT("InvalidDrainRate", "DrainRatePercentPerSecond must be finite and non-negative."));
		Result = EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

bool UBathWaterStateComponent::SetWaterState(const EBathWaterState NewState)
{
	if (WaterState == NewState)
	{
		return false;
	}
	const EBathWaterState Previous = WaterState;
	WaterState = NewState;
	OnWaterStateChanged.Broadcast(Previous, WaterState);
	OnWaterStateChangedNative.Broadcast(Previous, WaterState);
	return true;
}

void UBathWaterStateComponent::SetNormalizedAmount(const float NewAmount)
{
	CommitAmount(FMath::Clamp(FMath::IsFinite(NewAmount) ? NewAmount : 0.0f, 0.0f, 1.0f));
	CommitDerivedState();
	RefreshTickState();
}

bool UBathWaterStateComponent::IsCustomerUsable() const
{
	return NormalizedAmount >= GetDefault<UBathWaterSettings>()->GetCustomerUsableThresholdNormalized();
}

bool UBathWaterStateComponent::IsControlOpen(const EBathWaterControlType ControlType) const
{
	return ControlType == EBathWaterControlType::FillValve ? bFillValveOpen : bDrainLeverOpen;
}

float UBathWaterStateComponent::GetFillRatePercentPerSecond() const
{
	return FMath::IsFinite(FillRatePercentPerSecond)
		? FMath::Max(0.0f, FillRatePercentPerSecond)
		: 0.0f;
}

float UBathWaterStateComponent::GetDrainRatePercentPerSecond() const
{
	return FMath::IsFinite(DrainRatePercentPerSecond)
		? FMath::Max(0.0f, DrainRatePercentPerSecond)
		: 0.0f;
}

bool UBathWaterStateComponent::CanSetControlOpen(
	const EBathWaterControlType ControlType,
	const bool bOpen,
	FText& OutFailureReason) const
{
	OutFailureReason = FText::GetEmpty();
	if (!StaticEnum<EBathWaterControlType>()->IsValidEnumValue(static_cast<int64>(ControlType)))
	{
		OutFailureReason = LOCTEXT("InvalidControl", "물 조작부 설정이 올바르지 않습니다.");
		return false;
	}
	if (bRecoveryFrozen)
	{
		OutFailureReason = LOCTEXT("RecoveryFrozen", "설비 회수 중입니다");
		return false;
	}
	if (ControlType == EBathWaterControlType::FillValve && bOpen
		&& !bFillValveOpen && NormalizedAmount == 1.0f)
	{
		OutFailureReason = LOCTEXT("BathAlreadyFull", "이미 물이 가득 차 있습니다");
		return false;
	}
	return true;
}

bool UBathWaterStateComponent::RequestSetControlOpen(
	const EBathWaterControlType ControlType,
	const bool bOpen,
	const EBathWaterControlChangeReason Reason,
	FText& OutFailureReason)
{
	if (!CanSetControlOpen(ControlType, bOpen, OutFailureReason))
	{
		return false;
	}
	if (IsControlOpen(ControlType) == bOpen)
	{
		return true;
	}
	CommitControlState(ControlType, bOpen, Reason);
	CommitDerivedState();
	RefreshTickState();
	return true;
}

void UBathWaterStateComponent::ResetEmptyForPlacement()
{
	RecoverySnapshot.Reset();
	bRecoveryFrozen = false;
	const bool bOldFill = bFillValveOpen;
	const bool bOldDrain = bDrainLeverOpen;
	bFillValveOpen = false;
	bDrainLeverOpen = false;
	CommitAmount(0.0f);
	if (bOldFill)
	{
		OnControlChanged.Broadcast(EBathWaterControlType::FillValve, false, EBathWaterControlChangeReason::Reset);
		OnControlChangedNative.Broadcast(EBathWaterControlType::FillValve, false, EBathWaterControlChangeReason::Reset);
	}
	if (bOldDrain)
	{
		OnControlChanged.Broadcast(EBathWaterControlType::DrainLever, false, EBathWaterControlChangeReason::Reset);
		OnControlChangedNative.Broadcast(EBathWaterControlType::DrainLever, false, EBathWaterControlChangeReason::Reset);
	}
	CommitDerivedState();
	bCachedCustomerUsable = IsCustomerUsable();
	SetComponentTickEnabled(false);
}

bool UBathWaterStateComponent::BeginRecoveryFreeze(FText& OutFailureReason)
{
	if (RecoverySnapshot.IsSet() || bRecoveryFrozen)
	{
		OutFailureReason = LOCTEXT("RecoveryAlreadyFrozen", "설비 회수 동결이 이미 시작되었습니다.");
		return false;
	}
	RecoverySnapshot.Emplace(FRecoverySnapshot{
		NormalizedAmount,
		WaterState,
		bFillValveOpen,
		bDrainLeverOpen,
		bCachedCustomerUsable,
		IsComponentTickEnabled() });
	SetComponentTickEnabled(false);
	SetRecoveryFrozen(true);
	return true;
}

void UBathWaterStateComponent::CancelRecoveryFreeze()
{
	if (!RecoverySnapshot.IsSet())
	{
		return;
	}
	const FRecoverySnapshot Snapshot = RecoverySnapshot.GetValue();
	RecoverySnapshot.Reset();
	const bool bOldFill = bFillValveOpen;
	const bool bOldDrain = bDrainLeverOpen;
	const float OldAmount = NormalizedAmount;
	const EBathWaterState OldState = WaterState;
	bFillValveOpen = Snapshot.bFillOpen;
	bDrainLeverOpen = Snapshot.bDrainOpen;
	NormalizedAmount = Snapshot.Amount;
	WaterState = Snapshot.State;
	bCachedCustomerUsable = Snapshot.bCustomerUsable;
	if (bOldFill != bFillValveOpen)
	{
		OnControlChanged.Broadcast(EBathWaterControlType::FillValve, bFillValveOpen, EBathWaterControlChangeReason::Reset);
		OnControlChangedNative.Broadcast(EBathWaterControlType::FillValve, bFillValveOpen, EBathWaterControlChangeReason::Reset);
	}
	if (bOldDrain != bDrainLeverOpen)
	{
		OnControlChanged.Broadcast(EBathWaterControlType::DrainLever, bDrainLeverOpen, EBathWaterControlChangeReason::Reset);
		OnControlChangedNative.Broadcast(EBathWaterControlType::DrainLever, bDrainLeverOpen, EBathWaterControlChangeReason::Reset);
	}
	if (OldAmount != NormalizedAmount)
	{
		OnWaterAmountChanged.Broadcast(OldAmount, NormalizedAmount);
		OnWaterAmountChangedNative.Broadcast(OldAmount, NormalizedAmount);
	}
	if (OldState != WaterState)
	{
		OnWaterStateChanged.Broadcast(OldState, WaterState);
		OnWaterStateChangedNative.Broadcast(OldState, WaterState);
	}
	SetRecoveryFrozen(false);
	SetComponentTickEnabled(Snapshot.bTickEnabled);
	RefreshTickState();
}

void UBathWaterStateComponent::PrepareRecoveryCommit()
{
	if (!RecoverySnapshot.IsSet() || !bRecoveryFrozen)
	{
		return;
	}
	if (bFillValveOpen)
	{
		CommitControlState(EBathWaterControlType::FillValve, false, EBathWaterControlChangeReason::RecoveryCommit);
	}
	if (bDrainLeverOpen)
	{
		CommitControlState(EBathWaterControlType::DrainLever, false, EBathWaterControlChangeReason::RecoveryCommit);
	}
	CommitDerivedState();
	SetComponentTickEnabled(false);
}

float UBathWaterStateComponent::GetNetPercentPerSecond() const
{
	return (bFillValveOpen ? GetFillRatePercentPerSecond() : 0.0f)
		- (bDrainLeverOpen ? GetDrainRatePercentPerSecond() : 0.0f);
}

EBathWaterState UBathWaterStateComponent::DeriveWaterState() const
{
	if (NormalizedAmount == 0.0f)
	{
		return EBathWaterState::Empty;
	}
	if (NormalizedAmount == 1.0f)
	{
		return EBathWaterState::Filled;
	}
	const float NetRate = GetNetPercentPerSecond();
	if (NetRate > 0.0f)
	{
		return EBathWaterState::Filling;
	}
	if (NetRate < 0.0f)
	{
		return EBathWaterState::Draining;
	}
	return EBathWaterState::Holding;
}

void UBathWaterStateComponent::CommitAmount(const float NewAmount)
{
	if (NormalizedAmount == NewAmount)
	{
		return;
	}
	const float Previous = NormalizedAmount;
	NormalizedAmount = NewAmount;
	OnWaterAmountChanged.Broadcast(Previous, NormalizedAmount);
	OnWaterAmountChangedNative.Broadcast(Previous, NormalizedAmount);
	BroadcastUsabilityIfChanged();
}

void UBathWaterStateComponent::CommitDerivedState()
{
	const EBathWaterState NewState = DeriveWaterState();
	if (WaterState == NewState)
	{
		return;
	}
	const EBathWaterState Previous = WaterState;
	WaterState = NewState;
	OnWaterStateChanged.Broadcast(Previous, WaterState);
	OnWaterStateChangedNative.Broadcast(Previous, WaterState);
}

void UBathWaterStateComponent::CommitControlState(
	const EBathWaterControlType ControlType,
	const bool bOpen,
	const EBathWaterControlChangeReason Reason)
{
	bool& State = ControlType == EBathWaterControlType::FillValve
		? bFillValveOpen
		: bDrainLeverOpen;
	if (State == bOpen)
	{
		return;
	}
	State = bOpen;
	OnControlChanged.Broadcast(ControlType, bOpen, Reason);
	OnControlChangedNative.Broadcast(ControlType, bOpen, Reason);
}

void UBathWaterStateComponent::RefreshTickState()
{
	if (bRecoveryFrozen)
	{
		SetComponentTickEnabled(false);
		return;
	}
	const float NetRate = GetNetPercentPerSecond();
	const bool bCanChange = (NetRate > 0.0f && NormalizedAmount < 1.0f)
		|| (NetRate < 0.0f && NormalizedAmount > 0.0f);
	SetComponentTickEnabled(bCanChange);
}

void UBathWaterStateComponent::BroadcastUsabilityIfChanged()
{
	const bool bUsable = IsCustomerUsable();
	if (bCachedCustomerUsable == bUsable)
	{
		return;
	}
	bCachedCustomerUsable = bUsable;
	OnCustomerUsabilityChanged.Broadcast(bUsable);
	OnCustomerUsabilityChangedNative.Broadcast(bUsable);
}

void UBathWaterStateComponent::SetRecoveryFrozen(const bool bFrozen)
{
	if (bRecoveryFrozen == bFrozen)
	{
		return;
	}
	bRecoveryFrozen = bFrozen;
	OnRecoveryFreezeChanged.Broadcast(bRecoveryFrozen);
	OnRecoveryFreezeChangedNative.Broadcast(bRecoveryFrozen);
}

#undef LOCTEXT_NAMESPACE
