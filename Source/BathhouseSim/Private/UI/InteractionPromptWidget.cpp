#include "UI/InteractionPromptWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "Interaction/PlayerInteractionComponent.h"

void UInteractionPromptWidget::SetInteractionComponent(UPlayerInteractionComponent* InInteractionComponent)
{
	if (InteractionComponent == InInteractionComponent)
	{
		return;
	}
	UnbindInteraction();
	ClearAllTransientFailures(false);
	InteractionComponent = InInteractionComponent;
	BindInteraction();
	if (IsConstructed() && !InteractionComponent)
	{
		PresentQuery(FPlayerInteractionQuery(), true);
	}
}

void UInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bHasPresentedQuery = false;
	BindInteraction();
	if (!InteractionComponent)
	{
		PresentQuery(FPlayerInteractionQuery());
	}
}

void UInteractionPromptWidget::NativeDestruct()
{
	UnbindInteraction();
	InteractionComponent = nullptr;
	ClearAllTransientFailures(false);
	PresentQuery(FPlayerInteractionQuery(), true);
	Super::NativeDestruct();
}

void UInteractionPromptWidget::HandleInteractionQueryChanged(const FPlayerInteractionQuery& Query)
{
	ClearAllTransientFailures(false);
	PresentQuery(Query, true);
}

void UInteractionPromptWidget::HandleInteractionAttemptFinished(const FPlayerInteractionResult& Result)
{
	if (Result.bSucceeded || Result.FailureReason.IsEmpty())
	{
		ClearTransientFailure(Result.Intent, true);
		return;
	}

	ClearTransientFailure(Result.Intent, false);
	FText* FailureReason = &PrimaryTransientFailureReason;
	FTimerHandle* TimerHandle = &PrimaryFailureTimerHandle;
	FTimerDelegate ExpiredDelegate;
	switch (Result.Intent)
	{
	case EPlayerInteractionIntent::Secondary:
		FailureReason = &SecondaryTransientFailureReason;
		TimerHandle = &SecondaryFailureTimerHandle;
		ExpiredDelegate.BindUObject(this, &UInteractionPromptWidget::HandleSecondaryTransientFailureExpired);
		break;
	case EPlayerInteractionIntent::HeldApply:
		FailureReason = &HeldApplyTransientFailureReason;
		TimerHandle = &HeldApplyFailureTimerHandle;
		ExpiredDelegate.BindUObject(this, &UInteractionPromptWidget::HandleHeldApplyTransientFailureExpired);
		break;
	case EPlayerInteractionIntent::HeldTake:
	case EPlayerInteractionIntent::EquipmentSecondaryUse:
		// Both intents surface in the single RMB row.
		FailureReason = &HeldTakeTransientFailureReason;
		TimerHandle = &HeldTakeFailureTimerHandle;
		ExpiredDelegate.BindUObject(this, &UInteractionPromptWidget::HandleHeldTakeTransientFailureExpired);
		break;
	case EPlayerInteractionIntent::EquipmentUse:
		FailureReason = &EquipmentTransientFailureReason;
		TimerHandle = &EquipmentFailureTimerHandle;
		ExpiredDelegate.BindUObject(this, &UInteractionPromptWidget::HandleEquipmentTransientFailureExpired);
		break;
	case EPlayerInteractionIntent::PlacementConfirm:
		FailureReason = &PlacementTransientFailureReason;
		TimerHandle = &PlacementFailureTimerHandle;
		ExpiredDelegate.BindUObject(this, &UInteractionPromptWidget::HandlePlacementTransientFailureExpired);
		break;
	case EPlayerInteractionIntent::FacilityRecovery:
		FailureReason = &RecoveryTransientFailureReason;
		TimerHandle = &RecoveryFailureTimerHandle;
		ExpiredDelegate.BindUObject(this, &UInteractionPromptWidget::HandleRecoveryTransientFailureExpired);
		break;
	default:
		ExpiredDelegate.BindUObject(this, &UInteractionPromptWidget::HandlePrimaryTransientFailureExpired);
		break;
	}

	*FailureReason = Result.FailureReason;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			*TimerHandle,
			ExpiredDelegate,
			FMath::Max(0.1f, FailureDisplayDurationSeconds),
			false);
	}
	ApplyCurrentPresentation();
}

void UInteractionPromptWidget::HandlePlacementTransientFailureExpired()
{
	ClearTransientFailure(EPlayerInteractionIntent::PlacementConfirm, true);
}

void UInteractionPromptWidget::HandleRecoveryTransientFailureExpired()
{
	ClearTransientFailure(EPlayerInteractionIntent::FacilityRecovery, true);
}

void UInteractionPromptWidget::HandlePrimaryTransientFailureExpired()
{
	ClearTransientFailure(EPlayerInteractionIntent::Primary, true);
}

void UInteractionPromptWidget::HandleSecondaryTransientFailureExpired()
{
	ClearTransientFailure(EPlayerInteractionIntent::Secondary, true);
}

void UInteractionPromptWidget::HandleHeldApplyTransientFailureExpired()
{
	ClearTransientFailure(EPlayerInteractionIntent::HeldApply, true);
}

void UInteractionPromptWidget::HandleHeldTakeTransientFailureExpired()
{
	ClearTransientFailure(EPlayerInteractionIntent::HeldTake, true);
}

void UInteractionPromptWidget::HandleEquipmentTransientFailureExpired()
{
	ClearTransientFailure(EPlayerInteractionIntent::EquipmentUse, true);
}

void UInteractionPromptWidget::BindInteraction()
{
	if (bIsQueryBound || InteractionResultHandle.IsValid() || !IsConstructed() || !InteractionComponent)
	{
		return;
	}
	InteractionComponent->OnInteractionQueryChanged.AddDynamic(this, &UInteractionPromptWidget::HandleInteractionQueryChanged);
	bIsQueryBound = true;
	InteractionResultHandle = InteractionComponent->OnInteractionAttemptFinishedNative.AddUObject(
		this,
		&UInteractionPromptWidget::HandleInteractionAttemptFinished);
	PresentQuery(InteractionComponent->GetCurrentInteractionQuery(), true);
}

void UInteractionPromptWidget::UnbindInteraction()
{
	if (InteractionComponent)
	{
		if (bIsQueryBound)
		{
			InteractionComponent->OnInteractionQueryChanged.RemoveDynamic(this, &UInteractionPromptWidget::HandleInteractionQueryChanged);
		}
		if (InteractionResultHandle.IsValid())
		{
			InteractionComponent->OnInteractionAttemptFinishedNative.Remove(InteractionResultHandle);
		}
	}
	bIsQueryBound = false;
	InteractionResultHandle.Reset();
}

void UInteractionPromptWidget::PresentQuery(const FPlayerInteractionQuery& Query, const bool bForceRefresh)
{
	if (!bForceRefresh && bHasPresentedQuery && CachedQuery.Equals(Query))
	{
		return;
	}
	CachedQuery = Query;
	bHasPresentedQuery = true;
	ApplyCurrentPresentation();
}

void UInteractionPromptWidget::ApplyCurrentPresentation()
{
	if (!ensureMsgf(
		PromptRoot && TargetNameText && ActionNameText && FailureReasonText
			&& SecondaryActionNameText && SecondaryFailureReasonText && InteractionProgressBar
			&& EquipmentActionNameText && EquipmentFailureReasonText && EquipmentProgressBar
			&& PlacementActionNameText && PlacementFailureReasonText
			&& RecoveryActionNameText && RecoveryFailureReasonText && RecoveryProgressBar,
		TEXT("InteractionPromptWidget is missing one or more required BindWidget fields.")))
	{
		return;
	}

	const FText EmptyText = FText::GetEmpty();
	const bool bHasVisibleQuery = CachedQuery.bVisible;
	const bool bPrimaryActionVisible = bHasVisibleQuery && !CachedQuery.ActionName.IsEmpty();
	const bool bHasPrimaryTransientFailure = !PrimaryTransientFailureReason.IsEmpty();
	const bool bHasSecondaryTransientFailure = !SecondaryTransientFailureReason.IsEmpty();
	const bool bHasHeldApplyTransientFailure = !HeldApplyTransientFailureReason.IsEmpty();
	const bool bHasHeldTakeTransientFailure = !HeldTakeTransientFailureReason.IsEmpty();
	const bool bHasEquipmentTransientFailure = !EquipmentTransientFailureReason.IsEmpty();
	const bool bHasPlacementTransientFailure = !PlacementTransientFailureReason.IsEmpty();
	const bool bHasRecoveryTransientFailure = !RecoveryTransientFailureReason.IsEmpty();
	const bool bEquipmentRowVisible = CachedQuery.bEquipmentUseVisible;
	const bool bHeldApplyRowVisible = !bEquipmentRowVisible && CachedQuery.bHeldApplyVisible;
	const bool bLmbRowVisible = bEquipmentRowVisible || bHeldApplyRowVisible
		|| bHasEquipmentTransientFailure || bHasHeldApplyTransientFailure;
	// RMB row source: the equipment's own secondary action wins over the target's held-use Take.
	const bool bRmbFromEquipment = CachedQuery.bEquipmentSecondaryVisible;
	const bool bRmbVisible = bRmbFromEquipment || CachedQuery.bHeldTakeVisible;
	const bool bRmbCanUse = bRmbFromEquipment ? CachedQuery.bCanEquipmentSecondary : CachedQuery.bCanHeldTake;
	const bool bHeldTakeRowVisible = bRmbVisible || bHasHeldTakeTransientFailure;
	const bool bHasPersistentTarget = CachedQuery.bVisible || CachedQuery.bEquipmentUseVisible
		|| CachedQuery.bHeldApplyVisible || bRmbVisible;
	const bool bHasTransientFailure = bHasPrimaryTransientFailure || bHasSecondaryTransientFailure
		|| bHasHeldApplyTransientFailure || bHasHeldTakeTransientFailure
		|| bHasEquipmentTransientFailure || bHasPlacementTransientFailure || bHasRecoveryTransientFailure;
	const bool bHasHeldSummary = !CachedQuery.HeldObjectSummary.IsEmpty();
	const bool bShowPrompt = bHasPersistentTarget || CachedQuery.bPlacementVisible
		|| CachedQuery.bRecoveryVisible || bHasTransientFailure || bHasHeldSummary;
	const bool bSecondaryVisible = bHasVisibleQuery && CachedQuery.bSecondaryVisible;
	const bool bPromptEnabled = IsPromptRootEnabled(CachedQuery);
	const bool bPrimaryEnabled = IsLegacyPrimaryEnabled(CachedQuery);
	const FText& TargetName = bHasPersistentTarget ? CachedQuery.TargetName : EmptyText;
	const FText& ActionName = bPrimaryActionVisible ? CachedQuery.ActionName : EmptyText;
	const FText& EffectiveFailureReason = bHasPrimaryTransientFailure
		? PrimaryTransientFailureReason
		: (bPrimaryActionVisible ? CachedQuery.FailureReason : EmptyText);
	const FText& SecondaryActionName = bSecondaryVisible ? CachedQuery.SecondaryActionName : EmptyText;
	const FText& EffectiveSecondaryFailureReason = bHasSecondaryTransientFailure
		? SecondaryTransientFailureReason
		: (bSecondaryVisible ? CachedQuery.SecondaryFailureReason : EmptyText);
	const bool bShowFailure = bPrimaryActionVisible && !EffectiveFailureReason.IsEmpty();
	const bool bShowSecondaryFailure = (bSecondaryVisible || bHasSecondaryTransientFailure)
		&& !EffectiveSecondaryFailureReason.IsEmpty();
	const bool bShowHold = bHasVisibleQuery
		&& (CachedQuery.PrimaryActivationMode == EPlayerInteractionActivationMode::Hold
			|| CachedQuery.bPrimaryProgressVisible);

	const FText& LmbActionName = bEquipmentRowVisible
		? CachedQuery.EquipmentActionName
		: (bHeldApplyRowVisible ? CachedQuery.HeldApplyActionName : EmptyText);
	const FText& EffectiveLmbFailureReason = bHasEquipmentTransientFailure
		? EquipmentTransientFailureReason
		: bHasHeldApplyTransientFailure
		? HeldApplyTransientFailureReason
		: bEquipmentRowVisible
		? CachedQuery.EquipmentFailureReason
		: bHeldApplyRowVisible ? CachedQuery.HeldApplyFailureReason : EmptyText;
	const bool bShowLmbFailure = bLmbRowVisible && !EffectiveLmbFailureReason.IsEmpty();
	const bool bShowLmbHold = bEquipmentRowVisible
		&& CachedQuery.EquipmentActivationMode == EPlayerInteractionActivationMode::Hold;
	const FText& HeldTakeActionName = bRmbFromEquipment
		? CachedQuery.EquipmentSecondaryActionName
		: (CachedQuery.bHeldTakeVisible ? CachedQuery.HeldTakeActionName : EmptyText);
	const FText& RmbRowFailureReason = bRmbFromEquipment
		? CachedQuery.EquipmentSecondaryFailureReason : CachedQuery.HeldTakeFailureReason;
	const FText& EffectiveHeldTakeFailureReason = bHasHeldTakeTransientFailure
		? HeldTakeTransientFailureReason
		: (bRmbVisible ? RmbRowFailureReason : EmptyText);
	const bool bShowHeldTakeFailure = bHeldTakeRowVisible && !EffectiveHeldTakeFailureReason.IsEmpty();

	const bool bPlacementVisible = CachedQuery.bPlacementVisible || bHasPlacementTransientFailure;
	const FText& EffectivePlacementFailure = bHasPlacementTransientFailure
		? PlacementTransientFailureReason
		: CachedQuery.PlacementFailureReason;
	const bool bRecoveryVisible = CachedQuery.bRecoveryVisible || bHasRecoveryTransientFailure;
	const FText& EffectiveRecoveryFailure = bHasRecoveryTransientFailure
		? RecoveryTransientFailureReason
		: CachedQuery.RecoveryFailureReason;

	PromptRoot->SetVisibility(bShowPrompt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	PromptRoot->SetIsEnabled(bPromptEnabled);
	TargetNameText->SetText(TargetName);
	ActionNameText->SetText(ActionName);
	ActionNameText->SetIsEnabled(bPrimaryActionVisible && CachedQuery.bCanInteract);
	ActionNameText->SetVisibility(bPrimaryActionVisible
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	FailureReasonText->SetText(bShowFailure ? EffectiveFailureReason : EmptyText);
	FailureReasonText->SetVisibility(bShowFailure
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (PrimaryKeyText)
	{
		PrimaryKeyText->SetText(PrimaryKeyLabel);
		PrimaryKeyText->SetVisibility(bPrimaryActionVisible
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	SecondaryActionNameText->SetText(SecondaryActionName);
	SecondaryActionNameText->SetIsEnabled(bSecondaryVisible && CachedQuery.bCanSecondaryInteract);
	SecondaryActionNameText->SetVisibility(bSecondaryVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	SecondaryFailureReasonText->SetText(
		bShowSecondaryFailure ? EffectiveSecondaryFailureReason : EmptyText);
	SecondaryFailureReasonText->SetVisibility(
		bShowSecondaryFailure ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	InteractionProgressBar->SetPercent(FMath::Clamp(CachedQuery.HoldProgress, 0.0f, 1.0f));
	InteractionProgressBar->SetVisibility(bShowHold ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	EquipmentActionNameText->SetText(LmbActionName);
	EquipmentActionNameText->SetIsEnabled(
		(bEquipmentRowVisible && CachedQuery.bCanEquipmentUse)
		|| (bHeldApplyRowVisible && CachedQuery.bCanHeldApply));
	EquipmentActionNameText->SetVisibility(bLmbRowVisible
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	EquipmentFailureReasonText->SetText(bShowLmbFailure ? EffectiveLmbFailureReason : EmptyText);
	EquipmentFailureReasonText->SetVisibility(bShowLmbFailure
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	EquipmentProgressBar->SetPercent(FMath::Clamp(CachedQuery.EquipmentUseProgress, 0.0f, 1.0f));
	EquipmentProgressBar->SetVisibility(bShowLmbHold
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (LmbKeyText)
	{
		LmbKeyText->SetText(LmbKeyLabel);
		LmbKeyText->SetVisibility(bLmbRowVisible
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	PlacementActionNameText->SetText(CachedQuery.bPlacementVisible ? CachedQuery.PlacementActionName : EmptyText);
	PlacementActionNameText->SetIsEnabled(CachedQuery.bPlacementVisible && CachedQuery.bCanPlace);
	PlacementActionNameText->SetVisibility(bPlacementVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	PlacementFailureReasonText->SetText(EffectivePlacementFailure);
	PlacementFailureReasonText->SetVisibility(bPlacementVisible && !EffectivePlacementFailure.IsEmpty()
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	RecoveryActionNameText->SetText(CachedQuery.bRecoveryVisible ? CachedQuery.RecoveryActionName : EmptyText);
	RecoveryActionNameText->SetIsEnabled(CachedQuery.bRecoveryVisible && CachedQuery.bCanRecover);
	RecoveryActionNameText->SetVisibility(bRecoveryVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	RecoveryFailureReasonText->SetText(EffectiveRecoveryFailure);
	RecoveryFailureReasonText->SetVisibility(bRecoveryVisible && !EffectiveRecoveryFailure.IsEmpty()
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	RecoveryProgressBar->SetPercent(FMath::Clamp(CachedQuery.RecoveryProgress, 0.0f, 1.0f));
	RecoveryProgressBar->SetVisibility(CachedQuery.bRecoveryVisible
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (HeldTakeActionNameText)
	{
		HeldTakeActionNameText->SetText(HeldTakeActionName);
		HeldTakeActionNameText->SetIsEnabled(bRmbVisible && bRmbCanUse);
		HeldTakeActionNameText->SetVisibility(bRmbVisible
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (HeldTakeFailureReasonText)
	{
		HeldTakeFailureReasonText->SetText(bShowHeldTakeFailure
			? EffectiveHeldTakeFailureReason : EmptyText);
		HeldTakeFailureReasonText->SetVisibility(bShowHeldTakeFailure
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (RmbKeyText)
	{
		RmbKeyText->SetText(RmbKeyLabel);
		RmbKeyText->SetVisibility(bHeldTakeRowVisible
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (HeldSummaryText)
	{
		HeldSummaryText->SetText(CachedQuery.HeldObjectSummary);
		HeldSummaryText->SetVisibility(bHasHeldSummary
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	OnInteractionPromptChanged(
		bShowPrompt,
		bPrimaryEnabled,
		TargetName,
		ActionName,
		EffectiveFailureReason);
	OnInteractionPromptDetailsChanged(
		bSecondaryVisible,
		bSecondaryVisible && CachedQuery.bCanSecondaryInteract,
		SecondaryActionName,
		EffectiveSecondaryFailureReason,
		bShowHold,
		FMath::Clamp(CachedQuery.HoldProgress, 0.0f, 1.0f));
	OnEquipmentUsePromptChanged(
		bLmbRowVisible,
		(bEquipmentRowVisible && CachedQuery.bCanEquipmentUse)
			|| (bHeldApplyRowVisible && CachedQuery.bCanHeldApply),
		LmbActionName,
		EffectiveLmbFailureReason,
		bShowLmbHold,
		FMath::Clamp(CachedQuery.EquipmentUseProgress, 0.0f, 1.0f));
	OnFacilityPlacementPromptChanged(
		bPlacementVisible,
		CachedQuery.bPlacementVisible && CachedQuery.bCanPlace,
		EffectivePlacementFailure,
		bRecoveryVisible,
		CachedQuery.bRecoveryVisible && CachedQuery.bCanRecover,
		FMath::Clamp(CachedQuery.RecoveryProgress, 0.0f, 1.0f));
}

bool UInteractionPromptWidget::IsPromptRootEnabled(const FPlayerInteractionQuery& Query)
{
	return (Query.bVisible && Query.bCanInteract && !Query.ActionName.IsEmpty())
		|| (Query.bSecondaryVisible && Query.bCanSecondaryInteract)
		|| (Query.bEquipmentUseVisible && Query.bCanEquipmentUse)
		|| (Query.bHeldApplyVisible && Query.bCanHeldApply)
		|| (Query.bHeldTakeVisible && Query.bCanHeldTake)
		|| (Query.bEquipmentSecondaryVisible && Query.bCanEquipmentSecondary)
		|| (Query.bPlacementVisible && Query.bCanPlace)
		|| (Query.bRecoveryVisible && Query.bCanRecover);
}

bool UInteractionPromptWidget::IsLegacyPrimaryEnabled(const FPlayerInteractionQuery& Query)
{
	return Query.bVisible && Query.bCanInteract && !Query.ActionName.IsEmpty();
}

bool UInteractionPromptWidget::ClearTransientFailure(
	const EPlayerInteractionIntent Intent,
	const bool bRefreshPresentation)
{
	FText* FailureReason = &PrimaryTransientFailureReason;
	FTimerHandle* TimerHandle = &PrimaryFailureTimerHandle;
	switch (Intent)
	{
	case EPlayerInteractionIntent::Secondary:
		FailureReason = &SecondaryTransientFailureReason;
		TimerHandle = &SecondaryFailureTimerHandle;
		break;
	case EPlayerInteractionIntent::HeldApply:
		FailureReason = &HeldApplyTransientFailureReason;
		TimerHandle = &HeldApplyFailureTimerHandle;
		break;
	case EPlayerInteractionIntent::HeldTake:
	case EPlayerInteractionIntent::EquipmentSecondaryUse:
		FailureReason = &HeldTakeTransientFailureReason;
		TimerHandle = &HeldTakeFailureTimerHandle;
		break;
	case EPlayerInteractionIntent::EquipmentUse:
		FailureReason = &EquipmentTransientFailureReason;
		TimerHandle = &EquipmentFailureTimerHandle;
		break;
	case EPlayerInteractionIntent::PlacementConfirm:
		FailureReason = &PlacementTransientFailureReason;
		TimerHandle = &PlacementFailureTimerHandle;
		break;
	case EPlayerInteractionIntent::FacilityRecovery:
		FailureReason = &RecoveryTransientFailureReason;
		TimerHandle = &RecoveryFailureTimerHandle;
		break;
	default:
		break;
	}
	const bool bHadTransientFailure = !FailureReason->IsEmpty() || TimerHandle->IsValid();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(*TimerHandle);
	}
	TimerHandle->Invalidate();
	*FailureReason = FText::GetEmpty();
	if (bRefreshPresentation && bHadTransientFailure && bHasPresentedQuery)
	{
		ApplyCurrentPresentation();
	}
	return bHadTransientFailure;
}

bool UInteractionPromptWidget::ClearAllTransientFailures(const bool bRefreshPresentation)
{
	const bool bClearedPrimary = ClearTransientFailure(EPlayerInteractionIntent::Primary, false);
	const bool bClearedSecondary = ClearTransientFailure(EPlayerInteractionIntent::Secondary, false);
	const bool bClearedHeldApply = ClearTransientFailure(EPlayerInteractionIntent::HeldApply, false);
	const bool bClearedHeldTake = ClearTransientFailure(EPlayerInteractionIntent::HeldTake, false);
	const bool bClearedEquipment = ClearTransientFailure(EPlayerInteractionIntent::EquipmentUse, false);
	const bool bClearedPlacement = ClearTransientFailure(EPlayerInteractionIntent::PlacementConfirm, false);
	const bool bClearedRecovery = ClearTransientFailure(EPlayerInteractionIntent::FacilityRecovery, false);
	if (bRefreshPresentation && (bClearedPrimary || bClearedSecondary || bClearedHeldApply
		|| bClearedHeldTake || bClearedEquipment || bClearedPlacement || bClearedRecovery)
		&& bHasPresentedQuery)
	{
		ApplyCurrentPresentation();
	}
	return bClearedPrimary || bClearedSecondary || bClearedHeldApply || bClearedHeldTake
		|| bClearedEquipment || bClearedPlacement || bClearedRecovery;
}
