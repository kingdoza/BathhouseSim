#include "Service/PlayerScrubFocusComponent.h"
#include "Service/ScrubTableActor.h"
#include "Character/FirstPersonMovementComponent.h"
#include "Computer/PlayerComputerUseComponent.h"
#include "Computer/ComputerFocusExitPlacement.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"

UPlayerScrubFocusComponent::UPlayerScrubFocusComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPlayerScrubFocusComponent::Configure(UFirstPersonMovementComponent* InMovement,
										   UPlayerInteractionComponent* InInteraction, UPlayerCarryComponent* InCarry)
{
	Movement = InMovement;
	Interaction = InInteraction;
	Carry = InCarry;
}

APlayerController* UPlayerScrubFocusComponent::ResolveController() const
{
	const auto* Pawn = Cast<APawn>(GetOwner());
	return Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
}

bool UPlayerScrubFocusComponent::HasValidContext() const
{
	const auto* Pawn = Cast<APawn>(GetOwner());
	auto* Controller = ResolveController();
	return IsValid(Pawn) && Pawn->IsLocallyControlled() && Controller && Controller->IsLocalController() &&
		   IsValid(Movement) && IsValid(Interaction) && IsValid(Carry) &&
		   Carry->GetHeldKind() == EPhysicalCarryKind::ScrubTowel;
}

bool UPlayerScrubFocusComponent::BeginScrubFocus(AScrubTableActor* Table)
{
	auto* Computer = GetOwner()->FindComponentByClass<UPlayerComputerUseComponent>();
	if (IsCapturingInput() || !HasValidContext() || !IsValid(Table) || !GetWorld() ||
		(Computer && Computer->IsCapturingInput()) || Interaction->IsInteractionSuppressed() ||
		!Table->TryBeginScrubSession(*this))
	{
		return false;
	}
	auto* Controller = ResolveController();
	PreviousViewTarget = Controller->GetViewTarget();
	SessionController = Controller;
	SavedMovementMode = Movement->MovementMode;
	SavedCustomMovementMode = Movement->CustomMovementMode;
	bSavedCursorVisible = Controller->bShowMouseCursor;
	bSnapshotValid = true;
	ActiveTable = Table;
	Phase = EPlayerScrubFocusPhase::FocusingIn;
	bRubbing = false;
	CursorLocal = FVector2D::ZeroVector;
	SessionEndedHandle = Table->OnScrubSessionEnded.AddUObject(this, &UPlayerScrubFocusComponent::HandleSessionEnded);
	SetComponentTickEnabled(true);
	if (auto* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->StopJumping();
	}
	Movement->StopSprinting();
	Movement->StopMovementImmediately();
	Movement->DisableMovement();
	CastChecked<APawn>(GetOwner())->ConsumeMovementInputVector();
	Interaction->SetInteractionSuppressed(true);
	// Axes remain game input; the native mesh is the scrub cursor.
	Controller->bShowMouseCursor = false;
	Controller->SetInputMode(FInputModeGameOnly());
	Controller->SetViewTargetWithBlend(Table, Table->GetFocusBlendInSeconds());
	UpdateCursor();
	HideHeldTowel();
	Table->GetScrubCursor()->SetHiddenInGame(false);
	if (Table->GetFocusBlendInSeconds() <= 0)
	{
		CompleteFocusIn();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(BlendTimer, this, &UPlayerScrubFocusComponent::CompleteFocusIn,
											   Table->GetFocusBlendInSeconds(), false);
	}
	return true;
}

void UPlayerScrubFocusComponent::HideHeldTowel()
{
	auto* Towel = Carry ? Carry->GetHeldObject() : nullptr;
	if (!IsValid(Towel) || !HiddenHeldTowel.IsExplicitlyNull())
	{
		return;
	}
	HiddenHeldTowel = Towel;
	bHiddenHeldTowelWasHidden = Towel->IsHidden();
	Towel->SetActorHiddenInGame(true);
}

void UPlayerScrubFocusComponent::RestoreHeldTowelVisibility()
{
	if (auto* Towel = HiddenHeldTowel.Get())
	{
		Towel->SetActorHiddenInGame(bHiddenHeldTowelWasHidden);
	}
	HiddenHeldTowel.Reset();
}

void UPlayerScrubFocusComponent::CompleteFocusIn()
{
	GetWorld()->GetTimerManager().ClearTimer(BlendTimer);
	if (Phase != EPlayerScrubFocusPhase::FocusingIn)
	{
		return;
	}
	if (!HasValidContext() || ResolveController() != SessionController.Get() || !ActiveTable.IsValid() ||
		!ActiveTable->HasOccupiedUser())
	{
		ForceCleanup(true);
		return;
	}
	Phase = EPlayerScrubFocusPhase::Active;
}

void UPlayerScrubFocusComponent::SetRubbing(bool Rubbing)
{
	bRubbing = Rubbing && Phase == EPlayerScrubFocusPhase::Active;
}

void UPlayerScrubFocusComponent::AddRubInput(FVector2D Axis)
{
	auto* Table = ActiveTable.Get();
	if (Phase != EPlayerScrubFocusPhase::Active || !Table || !HasValidContext() || Axis.ContainsNaN())
	{
		return;
	}
	const FVector Extent = Table->GetScrubArea()->GetUnscaledBoxExtent();
	const FVector2D Old = CursorLocal;
	CursorLocal += FVector2D(Axis.Y, Axis.X) * Table->GetRubCmPerInputUnit();
	CursorLocal.X = FMath::Clamp(CursorLocal.X, -Extent.X, Extent.X);
	CursorLocal.Y = FMath::Clamp(CursorLocal.Y, -Extent.Y, Extent.Y);
	UpdateCursor();
	if (bRubbing)
	{
		Table->AddRubDistance(*this, FVector2D::Distance(Old, CursorLocal));
	}
}

void UPlayerScrubFocusComponent::UpdateCursor()
{
	if (auto* Table = ActiveTable.Get())
	{
		auto* Area = Table->GetScrubArea();
		Table->GetScrubCursor()->SetWorldLocationAndRotation(
			Area->GetComponentTransform().TransformPosition(
				FVector(CursorLocal.X, CursorLocal.Y, Area->GetUnscaledBoxExtent().Z)),
			Area->GetComponentQuat());
	}
}

void UPlayerScrubFocusComponent::RequestEndScrubFocus()
{
	if (!IsCapturingInput() || Phase == EPlayerScrubFocusPhase::FocusingOut)
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(BlendTimer);
	bRubbing = false;
	auto* Table = ActiveTable.Get();
	auto* Character = Cast<ACharacter>(GetOwner());
	auto* Controller = ResolveController();
	if (Table)
	{
		Table->GetScrubCursor()->SetHiddenInGame(true);
	}
	RestoreHeldTowelVisibility();
	if (!IsValid(Table) || Table->IsActorBeingDestroyed() || !Character || !Controller ||
		Controller != SessionController.Get())
	{
		ForceCleanup(true);
		return;
	}
	const auto Foot = Table->GetExitFootTransform();
	FComputerFocusExitPlacementResult Exit;
	if (!FComputerFocusExitPlacement::Resolve(GetWorld(), Character->GetCapsuleComponent(), Character,
											  Foot.GetLocation(), Foot.Rotator().Yaw, Table->GetExitSearchRadiusCm(),
											  Exit))
	{
		ForceCleanup(true);
		return;
	}
	Character->SetActorLocationAndRotation(Exit.CapsuleCenter, FRotator(0, Foot.Rotator().Yaw, 0), false, nullptr,
										   ETeleportType::TeleportPhysics);
	Controller->SetControlRotation(FRotator(Foot.Rotator().Pitch, Foot.Rotator().Yaw, 0));
	Phase = EPlayerScrubFocusPhase::FocusingOut;
	Controller->SetInputMode(FInputModeGameOnly());
	Controller->SetViewTargetWithBlend(Character, Table->GetFocusBlendOutSeconds());
	if (Table->GetFocusBlendOutSeconds() <= 0)
	{
		CompleteFocusOut();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(BlendTimer, this, &UPlayerScrubFocusComponent::CompleteFocusOut,
											   Table->GetFocusBlendOutSeconds(), false);
	}
}

void UPlayerScrubFocusComponent::CompleteFocusOut()
{
	if (Phase != EPlayerScrubFocusPhase::FocusingOut)
	{
		return;
	}
	ForceCleanup(false);
}

void UPlayerScrubFocusComponent::HandleSessionEnded()
{
	auto* Table = ActiveTable.Get();
	if (!Table || Table->IsActorBeingDestroyed())
	{
		ForceCleanup(true);
	}
	else
	{
		RequestEndScrubFocus();
	}
}

void UPlayerScrubFocusComponent::ForceCleanup(bool RestoreView)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BlendTimer);
	}
	auto* Table = ActiveTable.Get();
	if (Table)
	{
		Table->OnScrubSessionEnded.Remove(SessionEndedHandle);
		Table->GetScrubCursor()->SetHiddenInGame(true);
	}
	RestoreHeldTowelVisibility();
	if (Table)
	{
		Table->EndScrubSession(*this);
	}
	const bool HadSnapshot = bSnapshotValid;
	if (HadSnapshot)
	{
		if (auto* Controller = SessionController.Get())
		{
			if (RestoreView)
			{
				Controller->SetViewTarget(PreviousViewTarget.IsValid() ? PreviousViewTarget.Get() : GetOwner());
			}
			Controller->bShowMouseCursor = bSavedCursorVisible;
			Controller->SetInputMode(FInputModeGameOnly());
		}
		if (Movement)
		{
			Movement->SetMovementMode(SavedMovementMode, SavedCustomMovementMode);
		}
	}
	ActiveTable.Reset();
	PreviousViewTarget.Reset();
	SessionController.Reset();
	SessionEndedHandle.Reset();
	bSnapshotValid = false;
	bRubbing = false;
	Phase = EPlayerScrubFocusPhase::Inactive;
	SetComponentTickEnabled(false);
	if (HadSnapshot && Interaction)
	{
		Interaction->SetInteractionSuppressed(false);
	}
}

void UPlayerScrubFocusComponent::TickComponent(float Delta, ELevelTick Tick, FActorComponentTickFunction* Function)
{
	Super::TickComponent(Delta, Tick, Function);
	if (IsCapturingInput() &&
		(!HasValidContext() || ResolveController() != SessionController.Get() || !ActiveTable.IsValid()))
	{
		ForceCleanup(true);
	}
}

void UPlayerScrubFocusComponent::EndPlay(EEndPlayReason::Type Reason)
{
	ForceCleanup(true);
	Movement = nullptr;
	Interaction = nullptr;
	Carry = nullptr;
	Super::EndPlay(Reason);
}
