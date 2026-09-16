#include "Facility/BathhouseBathFacilityActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Facility/BathWaterControlComponent.h"
#include "Facility/BathWaterStateComponent.h"
#include "Facility/BathhouseFacilitySubsystem.h"
#include "NiagaraComponent.h"
#include "Placement/FacilityPlacementComponent.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BathhouseBathFacilityActor"

ABathhouseBathFacilityActor::ABathhouseBathFacilityActor()
{
	PrimaryActorTick.bCanEverTick = false;
	FacilityType = EBathhouseFacilityType::Bath;

	FillValveControl = CreateDefaultSubobject<UBathWaterControlComponent>(TEXT("FillValveControl"));
	FillValveControl->SetupAttachment(SceneRoot);
	FillValveControl->ConfigureControlType(EBathWaterControlType::FillValve);

	DrainLeverControl = CreateDefaultSubobject<UBathWaterControlComponent>(TEXT("DrainLeverControl"));
	DrainLeverControl->SetupAttachment(SceneRoot);
	DrainLeverControl->ConfigureControlType(EBathWaterControlType::DrainLever);

	FillFlowNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FillFlowNiagara"));
	FillFlowNiagara->SetupAttachment(SceneRoot);
	FillFlowNiagara->SetAutoActivate(false);

	WaterPresentationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WaterPresentationRoot"));
	WaterPresentationRoot->SetupAttachment(SceneRoot);
	WaterSurfaceMover = CreateDefaultSubobject<USceneComponent>(TEXT("WaterSurfaceMover"));
	WaterSurfaceMover->SetupAttachment(WaterPresentationRoot);
	WaterSurfaceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterSurfaceMesh"));
	WaterSurfaceMesh->SetupAttachment(WaterSurfaceMover);
	WaterSurfaceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterSurfaceMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	WaterSurfaceMesh->SetGenerateOverlapEvents(false);
	WaterSurfaceMesh->SetSimulatePhysics(false);
	WaterSurfaceMesh->SetCanEverAffectNavigation(false);
	WaterLevelEmptyPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WaterLevelEmptyPoint"));
	WaterLevelEmptyPoint->SetupAttachment(WaterPresentationRoot);
	WaterLevelFullPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WaterLevelFullPoint"));
	WaterLevelFullPoint->SetupAttachment(WaterPresentationRoot);
}

void ABathhouseBathFacilityActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	InitializeBathWaterRuntime();
}

void ABathhouseBathFacilityActor::BeginPlay()
{
	InitializeBathWaterRuntime();
	Super::BeginPlay();
	bCachedReservationAvailability = IsAvailableForReservation();
	UpdateWaterSurface();
	UpdateFillFlow();
}

void ABathhouseBathFacilityActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bRecoveryCommitPending = true;
	bRecoveryHoldActive = false;
	UnbindBathWaterDelegates();
	if (FillFlowNiagara)
	{
		FillFlowNiagara->DeactivateImmediate();
	}
	Super::EndPlay(EndPlayReason);
}

bool ABathhouseBathFacilityActor::IsAvailableForReservation() const
{
	return Super::IsAvailableForReservation() && BathWaterState
		&& BathWaterState->IsCustomerUsable() && !BathWaterState->IsRecoveryFrozen();
}

bool ABathhouseBathFacilityActor::TryBeginFacilityRecoveryHold(FText& OutFailureReason)
{
	if (bRecoveryHoldActive || bRecoveryCommitPending)
	{
		OutFailureReason = LOCTEXT("RecoveryHoldAlreadyActive", "설비 회수 동결이 이미 시작되었습니다.");
		return false;
	}
	const FFacilityPlacementTransactionResult Query = QueryFacilityRecovery();
	if (!Query.bSucceeded)
	{
		OutFailureReason = Query.FailureReason;
		return false;
	}
	bRecoveryFlowWasActive = FillFlowNiagara && FillFlowNiagara->IsActive();
	FillValveControl->BeginRecoveryFreeze();
	DrainLeverControl->BeginRecoveryFreeze();
	if (!BathWaterState || !BathWaterState->BeginRecoveryFreeze(OutFailureReason))
	{
		FillValveControl->CancelRecoveryFreeze();
		DrainLeverControl->CancelRecoveryFreeze();
		return false;
	}
	bRecoveryHoldActive = true;
	UpdateFillFlow();
	RefreshReservationAvailability(true);
	return true;
}

void ABathhouseBathFacilityActor::CancelFacilityRecoveryHold()
{
	if (!bRecoveryHoldActive || bRecoveryCommitPending)
	{
		return;
	}
	bRecoveryHoldActive = false;
	BathWaterState->CancelRecoveryFreeze();
	FillValveControl->CancelRecoveryFreeze();
	DrainLeverControl->CancelRecoveryFreeze();
	if (FillFlowNiagara)
	{
		if (bRecoveryFlowWasActive)
		{
			FillFlowNiagara->Activate(true);
		}
		else
		{
			FillFlowNiagara->DeactivateImmediate();
		}
	}
	RefreshReservationAvailability(true);
}

bool ABathhouseBathFacilityActor::StagePlacedDomainUnregistration(
	FFacilityPlacementPublication& OutPublication,
	FText& OutFailureReason)
{
	if (!bRecoveryHoldActive || bRecoveryCommitPending
		|| !Super::StagePlacedDomainUnregistration(OutPublication, OutFailureReason))
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT("RecoverySnapshotMissing", "욕탕 회수 동결 상태를 찾을 수 없습니다.");
		}
		return false;
	}
	bRecoveryCommitPending = true;
	BathWaterState->PrepareRecoveryCommit();
	FillValveControl->PrepareRecoveryCommit();
	DrainLeverControl->PrepareRecoveryCommit();
	UpdateFillFlow();
	RefreshReservationAvailability(false);
	return true;
}

bool ABathhouseBathFacilityActor::RollbackPlacedDomainUnregistration(FText& OutFailureReason)
{
	if (!Super::RollbackPlacedDomainUnregistration(OutFailureReason))
	{
		return false;
	}
	bRecoveryCommitPending = false;
	CancelFacilityRecoveryHold();
	return true;
}

#if WITH_EDITOR
EDataValidationResult ABathhouseBathFacilityActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto AddError = [&Context, &Result](const FText& Error)
	{
		Context.AddError(Error);
		Result = EDataValidationResult::Invalid;
	};
	if (!FillValveControl || !DrainLeverControl || !WaterPresentationRoot || !WaterSurfaceMover
		|| !WaterSurfaceMesh || !WaterLevelEmptyPoint || !WaterLevelFullPoint || !FillFlowNiagara)
	{
		AddError(LOCTEXT("MissingComponents", "Bath water native components are incomplete."));
	}
	else
	{
		const EDataValidationResult FillValidation = FillValveControl->IsDataValid(Context);
		const EDataValidationResult DrainValidation = DrainLeverControl->IsDataValid(Context);
		if (FillValidation == EDataValidationResult::Invalid
			|| DrainValidation == EDataValidationResult::Invalid)
		{
			Result = EDataValidationResult::Invalid;
		}
		if (FillValveControl->GetControlType() != EBathWaterControlType::FillValve
			|| DrainLeverControl->GetControlType() != EBathWaterControlType::DrainLever)
		{
			AddError(LOCTEXT("WrongControlTypes", "Bath water control component types do not match their stable names."));
		}
		if (WaterSurfaceMover->GetAttachParent() != WaterPresentationRoot
			|| WaterLevelEmptyPoint->GetAttachParent() != WaterPresentationRoot
			|| WaterLevelFullPoint->GetAttachParent() != WaterPresentationRoot)
		{
			AddError(LOCTEXT("WrongPresentationSpace", "Water mover and level markers must share WaterPresentationRoot local space."));
		}
		const FVector Empty = WaterLevelEmptyPoint->GetRelativeLocation();
		const FVector Full = WaterLevelFullPoint->GetRelativeLocation();
		if (Empty.ContainsNaN() || Full.ContainsNaN() || Empty.Equals(Full))
		{
			AddError(LOCTEXT("InvalidWaterMarkers", "Water level marker locations must be finite and distinct."));
		}
		if (WaterSurfaceMesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision
			|| WaterSurfaceMesh->CanEverAffectNavigation() || WaterSurfaceMesh->GetGenerateOverlapEvents())
		{
			AddError(LOCTEXT("InvalidWaterCollision", "WaterSurfaceMesh must have collision, overlap and navigation disabled."));
		}
		if (FillFlowNiagara->bAutoActivate)
		{
			AddError(LOCTEXT("NiagaraAutoActivate", "FillFlowNiagara must not auto activate."));
		}
		if (GetClass() != StaticClass()
			&& (!FillValveControl->GetStaticMesh() || !DrainLeverControl->GetStaticMesh()
				|| !WaterSurfaceMesh->GetStaticMesh() || !FillFlowNiagara->GetAsset()))
		{
			AddError(LOCTEXT("MissingConcreteAssets", "A concrete Bath Blueprint requires both control meshes, a plane water mesh and a fill Niagara System."));
		}
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

void ABathhouseBathFacilityActor::InitializeBathWaterRuntime()
{
	if (bBathRuntimeInitialized || !BathWaterState || !FillValveControl || !DrainLeverControl)
	{
		return;
	}
	bBathRuntimeInitialized = true;
	FillValveControl->InitializeClosedPose();
	DrainLeverControl->InitializeClosedPose();
	BathWaterState->ResetEmptyForPlacement();
	FillValveControl->ApplyLogicalState(false, EBathWaterControlChangeReason::Reset);
	DrainLeverControl->ApplyLogicalState(false, EBathWaterControlChangeReason::Reset);
	AmountChangedHandle = BathWaterState->OnWaterAmountChangedNative.AddUObject(
		this, &ABathhouseBathFacilityActor::HandleWaterAmountChanged);
	UsabilityChangedHandle = BathWaterState->OnCustomerUsabilityChangedNative.AddUObject(
		this, &ABathhouseBathFacilityActor::HandleCustomerUsabilityChanged);
	ControlChangedHandle = BathWaterState->OnControlChangedNative.AddUObject(
		this, &ABathhouseBathFacilityActor::HandleControlChanged);
	RecoveryFreezeChangedHandle = BathWaterState->OnRecoveryFreezeChangedNative.AddUObject(
		this, &ABathhouseBathFacilityActor::HandleRecoveryFreezeChanged);
	UpdateWaterSurface();
	UpdateFillFlow();
}

void ABathhouseBathFacilityActor::UnbindBathWaterDelegates()
{
	if (!BathWaterState)
	{
		return;
	}
	BathWaterState->OnWaterAmountChangedNative.Remove(AmountChangedHandle);
	BathWaterState->OnCustomerUsabilityChangedNative.Remove(UsabilityChangedHandle);
	BathWaterState->OnControlChangedNative.Remove(ControlChangedHandle);
	BathWaterState->OnRecoveryFreezeChangedNative.Remove(RecoveryFreezeChangedHandle);
	AmountChangedHandle.Reset();
	UsabilityChangedHandle.Reset();
	ControlChangedHandle.Reset();
	RecoveryFreezeChangedHandle.Reset();
}

void ABathhouseBathFacilityActor::HandleWaterAmountChanged(
	const float PreviousAmount,
	const float CurrentAmount)
{
	UpdateWaterSurface();
	OnBathWaterAmountChanged(PreviousAmount * 100.0f, CurrentAmount * 100.0f);
}

void ABathhouseBathFacilityActor::HandleCustomerUsabilityChanged(const bool bUsable)
{
	OnBathCustomerUsabilityChanged(bUsable);
	RefreshReservationAvailability(true);
}

void ABathhouseBathFacilityActor::HandleControlChanged(
	const EBathWaterControlType ControlType,
	const bool bOpen,
	const EBathWaterControlChangeReason Reason)
{
	UBathWaterControlComponent* Control = ControlType == EBathWaterControlType::FillValve
		? FillValveControl
		: DrainLeverControl;
	if (Control)
	{
		Control->ApplyLogicalState(bOpen, Reason);
	}
	if (ControlType == EBathWaterControlType::FillValve)
	{
		UpdateFillFlow();
	}
}

void ABathhouseBathFacilityActor::HandleRecoveryFreezeChanged(const bool bFrozen)
{
	(void)bFrozen;
	UpdateFillFlow();
	RefreshReservationAvailability(true);
}

void ABathhouseBathFacilityActor::UpdateWaterSurface()
{
	if (!BathWaterState || !WaterSurfaceMover || !WaterSurfaceMesh
		|| !WaterLevelEmptyPoint || !WaterLevelFullPoint)
	{
		return;
	}
	const float Alpha = FMath::Clamp(BathWaterState->GetNormalizedAmount(), 0.0f, 1.0f);
	WaterSurfaceMover->SetRelativeLocation(FMath::Lerp(
		WaterLevelEmptyPoint->GetRelativeLocation(),
		WaterLevelFullPoint->GetRelativeLocation(),
		Alpha));
	const bool bHasVisibleWater = Alpha > 0.0f;
	WaterSurfaceMesh->SetHiddenInGame(!bHasVisibleWater, true);
	WaterSurfaceMesh->SetVisibility(bHasVisibleWater, true);
}

void ABathhouseBathFacilityActor::UpdateFillFlow()
{
	if (!FillFlowNiagara || !BathWaterState)
	{
		return;
	}
	const bool bShouldBeActive = BathWaterState->IsControlOpen(EBathWaterControlType::FillValve)
		&& !BathWaterState->IsRecoveryFrozen()
		&& BathWaterState->GetNormalizedAmount() < 1.0f
		&& FacilityPlacement && FacilityPlacement->IsPlacedDomainActive();
	const bool bWasActive = FillFlowNiagara->IsActive();
	if (bShouldBeActive)
	{
		FillFlowNiagara->Activate(true);
	}
	else
	{
		FillFlowNiagara->DeactivateImmediate();
	}
	if (bWasActive != bShouldBeActive)
	{
		OnFillFlowPresentationChanged(bShouldBeActive);
	}
}

void ABathhouseBathFacilityActor::RefreshReservationAvailability(const bool bPublishChange)
{
	const bool bAvailable = IsAvailableForReservation();
	if (bCachedReservationAvailability == bAvailable)
	{
		return;
	}
	bCachedReservationAvailability = bAvailable;
	if (bPublishChange)
	{
		if (UBathhouseFacilitySubsystem* Subsystem = GetWorld()
			? GetWorld()->GetSubsystem<UBathhouseFacilitySubsystem>()
			: nullptr)
		{
			Subsystem->NotifyFacilityAvailabilityChanged(EBathhouseFacilityType::Bath);
		}
	}
}

#undef LOCTEXT_NAMESPACE
