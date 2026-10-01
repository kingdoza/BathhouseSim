#include "Interaction/PlayerEquipmentUseComponent.h"

#include "Camera/CameraComponent.h"
#include "Interaction/HeldEquipmentMotionComponent.h"
#include "Interaction/HeldEquipmentSecondaryUsable.h"
#include "Interaction/PhysicalCarryable.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"

#define LOCTEXT_NAMESPACE "PlayerEquipmentUseComponent"

UPlayerEquipmentUseComponent::UPlayerEquipmentUseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerEquipmentUseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelEquipmentUse();
	Camera = nullptr;
	CarryComponent = nullptr;
	InteractionComponent = nullptr;
	MotionComponent = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UPlayerEquipmentUseComponent::Configure(
	UCameraComponent* InCamera,
	UPlayerCarryComponent* InCarry,
	UPlayerInteractionComponent* InInteraction,
	UHeldEquipmentMotionComponent* InMotion)
{
	Camera = InCamera;
	CarryComponent = InCarry;
	InteractionComponent = InInteraction;
	MotionComponent = InMotion;
}

FPlayerInteractionQuery UPlayerEquipmentUseComponent::MergeEquipmentQuery(
	const FPlayerInteractionQuery& BaseQuery) const
{
	FPlayerInteractionQuery Result = BaseQuery;
	if (CarryComponent)
	{
		const IPhysicalCarryable* HeldCarryable = Cast<IPhysicalCarryable>(CarryComponent->GetHeldObject());
		if (HeldCarryable)
		{
			Result.HeldObjectSummary = HeldCarryable->GetHeldSummaryText();
		}
	}
	AActor* Equipment = nullptr;
	const IHeldEquipmentUsable* Usable = GetHeldUsable(Equipment);
	if (!Usable)
	{
		return Result;
	}

	Result.bHeldTakeVisible = false;
	Result.bCanHeldTake = false;
	Result.HeldTakeActionName = FText::GetEmpty();
	Result.HeldTakeFailureReason = FText::GetEmpty();
	Result.HeldTakeActivationMode = EPlayerInteractionActivationMode::Instant;

	FHeldEquipmentUseContext Context;
	if (!BuildContext(Equipment, Context))
	{
		Result.bHeldApplyVisible = false;
		Result.bCanHeldApply = false;
		Result.HeldApplyActionName = FText::GetEmpty();
		Result.HeldApplyFailureReason = FText::GetEmpty();
		Result.bEquipmentUseVisible = true;
		Result.bCanEquipmentUse = false;
		Result.EquipmentFailureReason = LOCTEXT("InvalidUseContext", "장비 사용 상태를 확인할 수 없습니다.");
		return Result;
	}

	if (const auto* Secondary = Cast<IHeldEquipmentSecondaryUsable>(Equipment))
	{
		const auto Query = Secondary->QuerySecondaryEquipmentUse(Context);
		Result.bEquipmentSecondaryVisible = Query.bVisible;
		Result.bCanEquipmentSecondary = Query.bCanUse;
		Result.EquipmentSecondaryActionName = Query.ActionName;
		Result.EquipmentSecondaryFailureReason = Query.FailureReason;
	}
	const FHeldEquipmentUseQuery EquipmentQuery = Usable->QueryEquipmentUse(Context);
	if (EquipmentQuery.bVisible)
	{
		Result.bHeldApplyVisible = false;
		Result.bCanHeldApply = false;
		Result.HeldApplyActionName = FText::GetEmpty();
		Result.HeldApplyFailureReason = FText::GetEmpty();
		Result.HeldApplyActivationMode = EPlayerInteractionActivationMode::Instant;
	}
	else
	{
		Result.bCanHeldApply = false;
		Result.HeldApplyActionName = FText::GetEmpty();
	}
	Result.bEquipmentUseVisible = EquipmentQuery.bVisible;
	Result.bCanEquipmentUse = EquipmentQuery.bCanUse;
	Result.EquipmentActionName = EquipmentQuery.ActionName;
	Result.EquipmentFailureReason = EquipmentQuery.FailureReason;
	Result.EquipmentActivationMode = EquipmentQuery.ActivationMode;
	Result.EquipmentUseProgress = FMath::Clamp(EquipmentQuery.Progress, 0.0f, 1.0f);
	if (Result.TargetName.IsEmpty())
	{
		Result.TargetName = EquipmentQuery.DisplayName;
	}
	return Result;
}

FPlayerInteractionResult UPlayerEquipmentUseComponent::BeginEquipmentUse()
{
	if (bInputActive)
	{
		return FPlayerInteractionResult::Failed(
			LOCTEXT("UseAlreadyActive", "이미 장비를 사용 중입니다."),
			EPlayerInteractionIntent::EquipmentUse);
	}

	AActor* Equipment = nullptr;
	IHeldEquipmentUsable* Usable = GetHeldUsable(Equipment);
	FHeldEquipmentUseContext Context;
	if (!Usable || !BuildContext(Equipment, Context))
	{
		const FPlayerInteractionQuery FocusQuery = InteractionComponent
			? InteractionComponent->GetCurrentInteractionQuery()
			: FPlayerInteractionQuery();
		const FPlayerInteractionResult Result = FPlayerInteractionResult::Failed(
			FocusQuery.bEquipmentUseVisible && !FocusQuery.EquipmentFailureReason.IsEmpty()
				? FocusQuery.EquipmentFailureReason
				: LOCTEXT("NoUsableEquipment", "사용할 장비를 들고 있지 않습니다."),
			EPlayerInteractionIntent::EquipmentUse);
		if (InteractionComponent)
		{
			InteractionComponent->ReportExternalInteractionAttempt(Result);
		}
		return Result;
	}

	const FHeldEquipmentUseQuery Query = Usable->QueryEquipmentUse(Context);
	if (!Query.bVisible && Query.FailureReason.IsEmpty())
	{
		const auto FocusQuery =
			InteractionComponent ? InteractionComponent->GetCurrentInteractionQuery() : FPlayerInteractionQuery();
		const auto Result =
			FPlayerInteractionResult::Failed(FocusQuery.HeldApplyFailureReason, EPlayerInteractionIntent::EquipmentUse);
		if (InteractionComponent && !FocusQuery.HeldApplyFailureReason.IsEmpty())
		{
			InteractionComponent->ReportExternalInteractionAttempt(Result);
		}
		return Result;
	}
	if (!Query.bVisible || !Query.bCanUse)
	{
		const FPlayerInteractionResult Result = FPlayerInteractionResult::Failed(
			Query.FailureReason.IsEmpty() ? LOCTEXT("EquipmentUnavailable", "이 장비를 지금 사용할 수 없습니다.") : Query.FailureReason,
			EPlayerInteractionIntent::EquipmentUse);
		if (InteractionComponent)
		{
			InteractionComponent->ReportExternalInteractionAttempt(Result);
		}
		return Result;
	}

	const FHeldEquipmentUseResult BeginResult = Usable->BeginEquipmentUse(Context);
	const FPlayerInteractionResult Result = BeginResult.bSucceeded
		? FPlayerInteractionResult::Succeeded(EPlayerInteractionIntent::EquipmentUse)
		: FPlayerInteractionResult::Failed(BeginResult.FailureReason, EPlayerInteractionIntent::EquipmentUse);
	if (BeginResult.bSucceeded)
	{
		ActiveEquipment = Equipment;
		ActiveContext = Context;
		ActiveMode = Query.ActivationMode;
		bInputActive = true;
	}
	if (InteractionComponent)
	{
		InteractionComponent->RefreshInteractionQuery();
		InteractionComponent->ReportExternalInteractionAttempt(Result);
	}
	return Result;
}

void UPlayerEquipmentUseComponent::UpdateEquipmentUse(const float DeltaTime)
{
	if (!bInputActive || ActiveMode != EPlayerInteractionActivationMode::Hold)
	{
		return;
	}
	AActor* Equipment = ActiveEquipment.Get();
	IHeldEquipmentUsable* Usable = Cast<IHeldEquipmentUsable>(Equipment);
	FHeldEquipmentUseContext Context;
	if (!Usable || !BuildContext(Equipment, Context) || !CarryComponent || CarryComponent->GetHeldObject() != Equipment)
	{
		CancelEquipmentUse();
		return;
	}

	ActiveContext = Context;
	const FHeldEquipmentUseUpdate Update = Usable->UpdateEquipmentUse(Context, FMath::Max(0.0f, DeltaTime));
	if (InteractionComponent)
	{
		InteractionComponent->RefreshInteractionQuery();
	}
	if (Update.State == EPlayerHoldInteractionState::Running)
	{
		return;
	}

	const bool bSucceeded = Update.State == EPlayerHoldInteractionState::Succeeded;
	ClearActiveUse();
	if (InteractionComponent)
	{
		InteractionComponent->ReportExternalInteractionAttempt(
			bSucceeded
				? FPlayerInteractionResult::Succeeded(EPlayerInteractionIntent::EquipmentUse)
				: FPlayerInteractionResult::Failed(Update.FailureReason, EPlayerInteractionIntent::EquipmentUse));
	}
}

void UPlayerEquipmentUseComponent::EndEquipmentUse()
{
	if (!bInputActive)
	{
		return;
	}
	AActor* Equipment = ActiveEquipment.Get();
	IHeldEquipmentUsable* Usable = Cast<IHeldEquipmentUsable>(Equipment);
	FHeldEquipmentUseContext Context = ActiveContext;
	BuildContext(Equipment, Context);
	ClearActiveUse();
	if (Usable)
	{
		Usable->EndEquipmentUse(Context);
	}
	if (InteractionComponent)
	{
		InteractionComponent->RefreshInteractionQuery();
	}
}

void UPlayerEquipmentUseComponent::CancelEquipmentUse()
{
	if (!bInputActive)
	{
		AActor* HeldEquipment = nullptr;
		if (IHeldEquipmentUsable* HeldUsable = GetHeldUsable(HeldEquipment))
		{
			FHeldEquipmentUseContext Context;
			if (BuildContext(HeldEquipment, Context))
			{
				HeldUsable->CancelEquipmentUse(Context);
			}
		}
		if (MotionComponent)
		{
			MotionComponent->StopMotion();
		}
		return;
	}
	AActor* Equipment = ActiveEquipment.Get();
	IHeldEquipmentUsable* Usable = Cast<IHeldEquipmentUsable>(Equipment);
	const FHeldEquipmentUseContext Context = ActiveContext;
	ClearActiveUse();
	if (Usable)
	{
		Usable->CancelEquipmentUse(Context);
	}
	if (MotionComponent)
	{
		MotionComponent->StopMotion();
	}
	if (InteractionComponent)
	{
		InteractionComponent->RefreshInteractionQuery();
	}
}

bool UPlayerEquipmentUseComponent::BuildContext(
	AActor* Equipment,
	FHeldEquipmentUseContext& OutContext) const
{
	if (!IsValid(Equipment) || !Camera || !CarryComponent || !GetOwner())
	{
		return false;
	}
	OutContext = FHeldEquipmentUseContext();
	OutContext.User = GetOwner();
	OutContext.Equipment = Equipment;
	OutContext.CarryComponent = CarryComponent;
	OutContext.InteractionComponent = InteractionComponent;
	OutContext.Camera = Camera;
	OutContext.MotionComponent = MotionComponent;
	OutContext.CameraOrigin = Camera->GetComponentLocation();
	OutContext.CameraDirection = Camera->GetForwardVector().GetSafeNormal();
	if (InteractionComponent)
	{
		InteractionComponent->GetCurrentFocusHit(OutContext.FocusHit);
	}
	return !OutContext.CameraDirection.IsNearlyZero();
}

bool UPlayerEquipmentUseComponent::HasUsableHeldEquipment() const
{
	AActor* Equipment = nullptr;
	return GetHeldUsable(Equipment) != nullptr;
}

IHeldEquipmentUsable* UPlayerEquipmentUseComponent::GetHeldUsable(AActor*& OutEquipment) const
{
	OutEquipment = CarryComponent ? CarryComponent->GetHeldObject() : nullptr;
	return IsValid(OutEquipment) ? Cast<IHeldEquipmentUsable>(OutEquipment) : nullptr;
}

bool UPlayerEquipmentUseComponent::HasSecondaryEquipmentUse() const
{
	AActor* Equipment = nullptr;
	return GetHeldUsable(Equipment) && Cast<IHeldEquipmentSecondaryUsable>(Equipment);
}

FPlayerInteractionResult UPlayerEquipmentUseComponent::ExecuteSecondaryEquipmentUse()
{
	if (bInputActive)
	{
		return FPlayerInteractionResult::Failed(FText::GetEmpty(), EPlayerInteractionIntent::EquipmentSecondaryUse);
	}
	AActor* Equipment = nullptr;
	GetHeldUsable(Equipment);
	auto* Secondary = Cast<IHeldEquipmentSecondaryUsable>(Equipment);
	FHeldEquipmentUseContext Context;
	if (!Secondary || !BuildContext(Equipment, Context))
	{
		return FPlayerInteractionResult::Failed(FText::GetEmpty(), EPlayerInteractionIntent::EquipmentSecondaryUse);
	}
	const auto Query = Secondary->QuerySecondaryEquipmentUse(Context);
	const auto UseResult = Query.bVisible && Query.bCanUse ? Secondary->ExecuteSecondaryEquipmentUse(Context)
														   : FHeldEquipmentUseResult::Failed(Query.FailureReason);
	const auto Result = UseResult.bSucceeded
		? FPlayerInteractionResult::Succeeded(EPlayerInteractionIntent::EquipmentSecondaryUse)
		: FPlayerInteractionResult::Failed(UseResult.FailureReason, EPlayerInteractionIntent::EquipmentSecondaryUse);
	if (InteractionComponent)
	{
		InteractionComponent->ReportExternalInteractionAttempt(Result);
		InteractionComponent->RefreshInteractionQuery();
	}
	return Result;
}

void UPlayerEquipmentUseComponent::ClearActiveUse()
{
	bInputActive = false;
	ActiveEquipment = nullptr;
	ActiveContext = FHeldEquipmentUseContext();
	ActiveMode = EPlayerInteractionActivationMode::Instant;
}

#undef LOCTEXT_NAMESPACE
