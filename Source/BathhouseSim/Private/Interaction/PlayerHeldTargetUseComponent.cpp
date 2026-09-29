#include "Interaction/PlayerHeldTargetUseComponent.h"

#include "GameFramework/Pawn.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerEquipmentUseComponent.h"
#include "Interaction/PlayerInteractable.h"
#include "Interaction/PlayerInteractionComponent.h"

#define LOCTEXT_NAMESPACE "PlayerHeldTargetUseComponent"

namespace
{
struct FHeldTargetUseAvailability
{
	bool bVisible = false;
	bool bCanUse = false;
	const FText* ActionName = nullptr;
	const FText* FailureReason = nullptr;
	EPlayerInteractionActivationMode ActivationMode = EPlayerInteractionActivationMode::Instant;
};

FHeldTargetUseAvailability GetAvailability(
	const FPlayerInteractionQuery& Query,
	const EPlayerHeldTargetUseDirection Direction)
{
	if (Direction == EPlayerHeldTargetUseDirection::Apply)
	{
		return {
			Query.bHeldApplyVisible,
			Query.bCanHeldApply,
			&Query.HeldApplyActionName,
			&Query.HeldApplyFailureReason,
			Query.HeldApplyActivationMode
		};
	}
	return {
		Query.bHeldTakeVisible,
		Query.bCanHeldTake,
		&Query.HeldTakeActionName,
		&Query.HeldTakeFailureReason,
		Query.HeldTakeActivationMode
	};
}

EPlayerInteractionIntent GetIntent(const EPlayerHeldTargetUseDirection Direction)
{
	return Direction == EPlayerHeldTargetUseDirection::Apply
		? EPlayerInteractionIntent::HeldApply
		: EPlayerInteractionIntent::HeldTake;
}
}

UPlayerHeldTargetUseComponent::UPlayerHeldTargetUseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	SetComponentTickEnabled(false);
}

void UPlayerHeldTargetUseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelUse();
	InteractionComponent = nullptr;
	CarryComponent = nullptr;
	EquipmentUseComponent = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UPlayerHeldTargetUseComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickRepeat(DeltaTime);
}

void UPlayerHeldTargetUseComponent::Configure(
	UPlayerInteractionComponent* InInteraction,
	UPlayerCarryComponent* InCarry,
	UPlayerEquipmentUseComponent* InEquipmentUse)
{
	InteractionComponent = InInteraction;
	CarryComponent = InCarry;
	EquipmentUseComponent = InEquipmentUse;
}

void UPlayerHeldTargetUseComponent::BeginUse(const EPlayerHeldTargetUseDirection Direction)
{
	if (bUseActive)
	{
		return;
	}

	bUseActive = true;
	ActiveDirection = Direction;
	StopRepeating();
	if (!IsReadyForUse())
	{
		return;
	}

	FPlayerInteractionContext Context;
	IPlayerInteractable* Interactable = nullptr;
	UObject* TargetObject = nullptr;
	if (!InteractionComponent->ResolveFocusedInteraction(Context, Interactable, TargetObject)
		|| !Interactable || !IsValid(TargetObject))
	{
		return;
	}

	const FPlayerInteractionQuery Query = Interactable->QueryInteraction(Context);
	const FHeldTargetUseAvailability Availability = GetAvailability(Query, Direction);
	if (!Availability.bVisible && (!Availability.FailureReason || Availability.FailureReason->IsEmpty()))
	{
		return;
	}
	if (!Availability.bCanUse)
	{
		InteractionComponent->ReportExternalInteractionAttempt(
			FPlayerInteractionResult::Failed(
				Availability.FailureReason ? *Availability.FailureReason : FText::GetEmpty(),
				GetIntent(Direction)));
		return;
	}

	FPlayerInteractionResult Result = Interactable->ExecuteHeldTargetUse(Context, Direction);
	Result.Intent = GetIntent(Direction);
	InteractionComponent->RefreshInteractionQuery();
	InteractionComponent->ReportExternalInteractionAttempt(Result);
	if (!Result.bSucceeded || Availability.ActivationMode != EPlayerInteractionActivationMode::Repeat)
	{
		return;
	}

	AActor* HeldObject = CarryComponent->GetHeldObject();
	if (!IsValid(HeldObject))
	{
		return;
	}
	RepeatTarget = TargetObject;
	RepeatHeldObject = HeldObject;
	RepeatElapsedSeconds = 0.0f;
	bRepeatActive = true;
	SetComponentTickEnabled(true);
}

void UPlayerHeldTargetUseComponent::EndUse()
{
	ClearUseState();
}

void UPlayerHeldTargetUseComponent::CancelUse()
{
	ClearUseState();
}

bool UPlayerHeldTargetUseComponent::IsLocallyControlledOwner() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn && Pawn->IsLocallyControlled();
}

bool UPlayerHeldTargetUseComponent::IsReadyForUse() const
{
	return InteractionComponent
		&& CarryComponent
		&& EquipmentUseComponent
		&& !InteractionComponent->IsInteractionSuppressed()
		&& IsLocallyControlledOwner();
}

void UPlayerHeldTargetUseComponent::TickRepeat(const float DeltaTime)
{
	if (!bUseActive || !bRepeatActive)
	{
		return;
	}
	if (!IsReadyForUse() || CarryComponent->GetHeldObject() != RepeatHeldObject.Get())
	{
		StopRepeating();
		return;
	}

	FPlayerInteractionContext Context;
	IPlayerInteractable* Interactable = nullptr;
	UObject* TargetObject = nullptr;
	if (!InteractionComponent->ResolveFocusedInteraction(Context, Interactable, TargetObject)
		|| !Interactable || TargetObject != RepeatTarget.Get())
	{
		StopRepeating();
		return;
	}

	const FPlayerInteractionQuery Query = Interactable->QueryInteraction(Context);
	const FHeldTargetUseAvailability Availability = GetAvailability(Query, ActiveDirection);
	if (!Availability.bVisible && (!Availability.FailureReason || Availability.FailureReason->IsEmpty()))
	{
		StopRepeating();
		return;
	}
	if (!Availability.bCanUse)
	{
		StopRepeating();
		InteractionComponent->ReportExternalInteractionAttempt(
			FPlayerInteractionResult::Failed(
				Availability.FailureReason ? *Availability.FailureReason : FText::GetEmpty(),
				GetIntent(ActiveDirection)));
		return;
	}
	if (Availability.ActivationMode != EPlayerInteractionActivationMode::Repeat)
	{
		StopRepeating();
		return;
	}

	const float Interval = FMath::Max(0.05f, RepeatIntervalSeconds);
	RepeatElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	if (RepeatElapsedSeconds < Interval)
	{
		return;
	}
	RepeatElapsedSeconds -= Interval;

	FPlayerInteractionResult Result = Interactable->ExecuteHeldTargetUse(Context, ActiveDirection);
	Result.Intent = GetIntent(ActiveDirection);
	InteractionComponent->RefreshInteractionQuery();
	InteractionComponent->ReportExternalInteractionAttempt(Result);
	if (!Result.bSucceeded)
	{
		StopRepeating();
	}
}

void UPlayerHeldTargetUseComponent::StopRepeating()
{
	bRepeatActive = false;
	RepeatTarget.Reset();
	RepeatHeldObject.Reset();
	RepeatElapsedSeconds = 0.0f;
	SetComponentTickEnabled(false);
}

void UPlayerHeldTargetUseComponent::ClearUseState()
{
	bUseActive = false;
	ActiveDirection = EPlayerHeldTargetUseDirection::Apply;
	StopRepeating();
}

#undef LOCTEXT_NAMESPACE