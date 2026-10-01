#include "Computer/PlayerComputerUseComponent.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Character/FirstPersonMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Computer/BathhouseComputerActor.h"
#include "Computer/ComputerFocusExitPlacement.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "SceneUtils.h"
#include "TimerManager.h"

namespace
{
const FName FocusedComputerAntiAliasingTag(TEXT("Bathhouse.ComputerFocus"));

void LogNonFixedComputerExit(
	const FComputerFocusExitPlacementResult& Placement,
	const ABathhouseComputerActor* Computer)
{

	const TCHAR* PathName = TEXT("Forced");
	if (Placement.Path == FComputerFocusExitPlacementResult::EPath::Searched)
	{
		PathName = TEXT("Searched");
	}
	UE_LOG(LogBathhouseComputer, Verbose,
		TEXT("Computer focus exit used %s placement at %.1f cm from %s."),
		PathName,
		Placement.SearchDistanceCm,
		Computer ? *Computer->GetPathName() : TEXT("<invalid computer>"));
}
}

UPlayerComputerUseComponent::UPlayerComputerUseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPlayerComputerUseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ForceCleanup(true, true);
	Camera = nullptr;
	Movement = nullptr;
	Interaction = nullptr;
	Carry = nullptr;
	WidgetInteraction = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UPlayerComputerUseComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	APlayerController* PlayerController = ResolvePlayerController();
	if (IsCapturingInput()
		&& (!ActiveComputer.IsValid()
			|| !PlayerController
			|| PlayerController != SessionPlayerController.Get()
			|| !HasValidConfiguredContext()))
	{
		ForceCleanup(true, true);
	}
}

void UPlayerComputerUseComponent::Configure(
	UCameraComponent* InCamera,
	UFirstPersonMovementComponent* InMovement,
	UPlayerInteractionComponent* InInteraction,
	UPlayerCarryComponent* InCarry,
	UWidgetInteractionComponent* InWidgetInteraction)
{
	if (IsCapturingInput())
	{
		ForceCleanup(true, true);
	}

	Camera = InCamera;
	Movement = InMovement;
	Interaction = InInteraction;
	Carry = InCarry;
	WidgetInteraction = InWidgetInteraction;
}

bool UPlayerComputerUseComponent::BeginComputerUse(ABathhouseComputerActor* Computer)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = ResolvePlayerController();
	if (Phase != EPlayerComputerUsePhase::Inactive
		|| !OwnerPawn
		|| !OwnerPawn->IsLocallyControlled()
		|| !PlayerController
		|| !PlayerController->IsLocalController()
		|| !Computer
		|| !Computer->IsReservedBy(this)
		|| !Computer->IsScreenReady()
		|| !HasValidConfiguredContext()
		|| !Carry->IsHandEmpty()
		|| !GetWorld())
	{
		return false;
	}

	PreviousViewTarget = PlayerController->GetViewTarget();
	if (!PreviousViewTarget.IsValid() || PreviousViewTarget.Get() == Computer)
	{
		PreviousViewTarget = OwnerPawn;
	}
	SessionPlayerController = PlayerController;
	SavedMovementMode = Movement->MovementMode;
	SavedCustomMovementMode = Movement->CustomMovementMode;
	bSavedCursorVisible = PlayerController->bShowMouseCursor;
	bSnapshotValid = true;
	bPointerDown = false;
	ActiveComputer = Computer;
	Phase = EPlayerComputerUsePhase::FocusingIn;
	SetComponentTickEnabled(true);

	if (ACharacter* Character = Cast<ACharacter>(OwnerPawn))
	{
		Character->StopJumping();
	}
	Movement->StopSprinting();
	Movement->StopMovementImmediately();
	Movement->DisableMovement();
	OwnerPawn->ConsumeMovementInputVector();
	Interaction->SetInteractionSuppressed(true);
	WidgetInteraction->bEnableHitTesting = false;

	const float BlendSeconds = FMath::Max(0.0f, Computer->GetFocusBlendInSeconds());
	PlayerController->SetViewTargetWithBlend(Computer, BlendSeconds);
	if (BlendSeconds <= 0.0f)
	{
		CompleteFocusIn();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(
			BlendTimer,
			this,
			&UPlayerComputerUseComponent::CompleteFocusIn,
			BlendSeconds,
			false);
	}
	return true;
}

void UPlayerComputerUseComponent::RequestEndComputerUse()
{
	if (Phase == EPlayerComputerUsePhase::Inactive || Phase == EPlayerComputerUsePhase::FocusingOut)
	{
		return;
	}

	ClearBlendTimer();
	RestoreFocusedAntiAliasingOverride();
	ReleasePointerIfNeeded();
	if (WidgetInteraction)
	{
		WidgetInteraction->bEnableHitTesting = false;
	}

	APlayerController* PlayerController = ResolvePlayerController();
	if (!PlayerController || PlayerController != SessionPlayerController.Get())
	{
		ForceCleanup(true, false);
		return;
	}

	PlayerController->bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);

	ABathhouseComputerActor* Computer = ActiveComputer.Get();
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	UCapsuleComponent* PlayerCapsule = OwnerCharacter ? OwnerCharacter->GetCapsuleComponent() : nullptr;
	if (!IsValid(Computer) || !IsValid(OwnerCharacter) || !IsValid(PlayerCapsule) || !GetWorld())
	{
		ForceCleanup(true, true);
		return;
	}

	const FTransform ExitFootTransform = Computer->GetFocusExitFootTransform();
	const FRotator ExitRotation = ExitFootTransform.Rotator();
	FComputerFocusExitPlacementResult Placement;
	if (!FComputerFocusExitPlacement::Resolve(
		GetWorld(),
		PlayerCapsule,
		OwnerCharacter,
		ExitFootTransform.GetLocation(),
		ExitRotation.Yaw,
		Computer->GetFocusExitSearchRadiusCm(),
		Placement))
	{
		ForceCleanup(true, true);
		return;
	}

	OwnerCharacter->SetActorLocationAndRotation(
		Placement.CapsuleCenter,
		FRotator(0.0f, ExitRotation.Yaw, 0.0f),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	PlayerController->SetControlRotation(FRotator(ExitRotation.Pitch, ExitRotation.Yaw, 0.0f));
	if (Placement.Path != FComputerFocusExitPlacementResult::EPath::Fixed)
	{
		LogNonFixedComputerExit(Placement, Computer);
	}

	const float BlendSeconds = FMath::Max(0.0f, Computer->GetFocusBlendOutSeconds());
	PlayerController->SetViewTargetWithBlend(OwnerCharacter, BlendSeconds);
	Phase = EPlayerComputerUsePhase::FocusingOut;
	if (BlendSeconds <= 0.0f)
	{
		CompleteFocusOut();
	}
	else if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			BlendTimer,
			this,
			&UPlayerComputerUseComponent::CompleteFocusOut,
			BlendSeconds,
			false);
	}
	else
	{
		ForceCleanup(true, false);
	}
}

void UPlayerComputerUseComponent::HandleComputerUnavailable(ABathhouseComputerActor* Computer)
{
	if (Computer && ActiveComputer.Get() == Computer)
	{
		ActiveComputer.Reset();
		ForceCleanup(false, true);
	}
}

bool UPlayerComputerUseComponent::PressPointer()
{
	if (Phase != EPlayerComputerUsePhase::Active || bPointerDown || !WidgetInteraction)
	{
		return false;
	}

	WidgetInteraction->PressPointerKey(EKeys::LeftMouseButton);
	bPointerDown = true;
	return true;
}

void UPlayerComputerUseComponent::ReleasePointer()
{
	ReleasePointerIfNeeded();
}

bool UPlayerComputerUseComponent::ScrollPointerWheel(float WheelDelta)
{
	const UWidgetComponent* HoveredComponent = WidgetInteraction ? WidgetInteraction->GetHoveredWidgetComponent() : nullptr;
	if (!CanInjectPointerWheel(WheelDelta, HoveredComponent))
	{
		return false;
	}

	WidgetInteraction->ScrollWheel(WheelDelta);
	return true;
}

bool UPlayerComputerUseComponent::CanInjectPointerWheel(float WheelDelta, const UWidgetComponent* HoveredComponent) const
{
	if (Phase != EPlayerComputerUsePhase::Active || !IsValid(WidgetInteraction) || !WidgetInteraction->bEnableHitTesting)
	{
		return false;
	}
	if (!FMath::IsFinite(WheelDelta) || FMath::IsNearlyZero(WheelDelta))
	{
		return false;
	}
	const ABathhouseComputerActor* Computer = ActiveComputer.Get();
	if (!IsValid(Computer) || !IsValid(Computer->GetScreenWidget()))
	{
		return false;
	}
	return HoveredComponent != nullptr && HoveredComponent == Computer->GetScreenWidget();
}

APlayerController* UPlayerComputerUseComponent::ResolvePlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}

AActor* UPlayerComputerUseComponent::ResolveReturnViewTarget(APlayerController* PlayerController) const
{
	if (PreviousViewTarget.IsValid() && PreviousViewTarget.Get() != ActiveComputer.Get())
	{
		return PreviousViewTarget.Get();
	}
	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		return const_cast<APawn*>(OwnerPawn);
	}
	return PlayerController;
}

void UPlayerComputerUseComponent::CompleteFocusIn()
{
	ClearBlendTimer();
	APlayerController* PlayerController = ResolvePlayerController();
	ABathhouseComputerActor* Computer = ActiveComputer.Get();
	UUserWidget* ScreenWidget = Computer && Computer->GetScreenWidget()
		? Computer->GetScreenWidget()->GetUserWidgetObject()
		: nullptr;
	if (Phase != EPlayerComputerUsePhase::FocusingIn
		|| !PlayerController
		|| PlayerController != SessionPlayerController.Get()
		|| !ScreenWidget
		|| !WidgetInteraction)
	{
		ForceCleanup(true, true);
		return;
	}

	Phase = EPlayerComputerUsePhase::Active;
	ApplyFocusedAntiAliasingOverride();
	PlayerController->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);
	UWidgetBlueprintLibrary::SetFocusToGameViewport();
	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX > 0 && ViewportSizeY > 0)
	{
		PlayerController->SetMouseLocation(ViewportSizeX / 2, ViewportSizeY / 2);
	}
	WidgetInteraction->bEnableHitTesting = true;
}

void UPlayerComputerUseComponent::CompleteFocusOut()
{
	RestoreFocusedAntiAliasingOverride();
	if (Phase != EPlayerComputerUsePhase::FocusingOut)
	{
		return;
	}

	APlayerController* PlayerController = ResolvePlayerController();
	if (!PlayerController || PlayerController != SessionPlayerController.Get())
	{
		ForceCleanup(true, true);
		return;
	}

	ClearBlendTimer();
	RestoreMovement();
	RestorePlayerInput(PlayerController);
	if (Interaction)
	{
		Interaction->SetInteractionSuppressed(false);
	}

	ABathhouseComputerActor* Computer = ActiveComputer.Get();
	ActiveComputer.Reset();
	if (Computer)
	{
		Computer->ReleaseReservation(this);
	}

	PreviousViewTarget.Reset();
	SessionPlayerController.Reset();
	bSnapshotValid = false;
	Phase = EPlayerComputerUsePhase::Inactive;
	SetComponentTickEnabled(false);
}

void UPlayerComputerUseComponent::ForceCleanup(const bool bReleaseReservation, const bool bRestoreView)
{
	const bool bHadSession = Phase != EPlayerComputerUsePhase::Inactive
		|| bSnapshotValid
		|| ActiveComputer.IsValid()
		|| bPointerDown;
	ClearBlendTimer();
	RestoreFocusedAntiAliasingOverride();
	if (!bHadSession)
	{
		SetComponentTickEnabled(false);
		return;
	}
	ReleasePointerIfNeeded();
	if (WidgetInteraction)
	{
		WidgetInteraction->bEnableHitTesting = false;
	}

	APlayerController* PlayerController = SessionPlayerController.Get();
	if (!PlayerController)
	{
		PlayerController = ResolvePlayerController();
	}
	if (bRestoreView && PlayerController)
	{
		if (AActor* ReturnViewTarget = ResolveReturnViewTarget(PlayerController))
		{
			PlayerController->SetViewTarget(ReturnViewTarget);
		}
	}
	RestoreMovement();
	RestorePlayerInput(PlayerController);
	if (Interaction)
	{
		Interaction->SetInteractionSuppressed(false);
	}

	ABathhouseComputerActor* Computer = ActiveComputer.Get();
	ActiveComputer.Reset();
	if (bReleaseReservation && Computer)
	{
		Computer->ReleaseReservation(this);
	}

	PreviousViewTarget.Reset();
	SessionPlayerController.Reset();
	bSnapshotValid = false;
	bPointerDown = false;
	Phase = EPlayerComputerUsePhase::Inactive;
	SetComponentTickEnabled(false);
}

void UPlayerComputerUseComponent::ReleasePointerIfNeeded()
{
	if (!bPointerDown)
	{
		return;
	}

	if (WidgetInteraction)
	{
		WidgetInteraction->ReleasePointerKey(EKeys::LeftMouseButton);
	}
	bPointerDown = false;
}

void UPlayerComputerUseComponent::RestoreMovement()
{
	if (bSnapshotValid && Movement)
	{
		Movement->SetMovementMode(SavedMovementMode, SavedCustomMovementMode);
	}
}

void UPlayerComputerUseComponent::RestorePlayerInput(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);
	if (bSnapshotValid)
	{
		PlayerController->bShowMouseCursor = bSavedCursorVisible;
	}
}

void UPlayerComputerUseComponent::ApplyFocusedAntiAliasingOverride()
{
	IConsoleVariable* AntiAliasingMethod = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"));
	if (!AntiAliasingMethod)
	{
		return;
	}

	const IConsoleVariable::FSetContext SetContext(ECVF_SetByCode, FocusedComputerAntiAliasingTag);
	AntiAliasingMethod->Set(static_cast<int32>(AAM_FXAA), SetContext);
}

void UPlayerComputerUseComponent::RestoreFocusedAntiAliasingOverride()
{
	if (IConsoleVariable* AntiAliasingMethod = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod")))
	{
		AntiAliasingMethod->Unset(ECVF_SetByCode, FocusedComputerAntiAliasingTag);
	}
}

void UPlayerComputerUseComponent::ClearBlendTimer()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BlendTimer);
	}
}

bool UPlayerComputerUseComponent::HasValidConfiguredContext() const
{
	return IsValid(Camera)
		&& IsValid(Movement)
		&& IsValid(Interaction)
		&& IsValid(Carry)
		&& IsValid(WidgetInteraction);
}
