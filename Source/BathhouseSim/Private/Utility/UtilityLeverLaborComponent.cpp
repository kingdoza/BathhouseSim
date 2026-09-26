#include "Utility/UtilityLeverLaborComponent.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Facility/BathWaterUtilityFacilityActor.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Utility/UtilityLaborInputGuard.h"
#include "Utility/UtilityLeverOperatingVolumeComponent.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Utility/UtilityOperationComponent.h"
#include "Utility/UtilityPivotRotation.h"

#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#endif

#define LOCTEXT_NAMESPACE "UtilityLeverLaborComponent"
DEFINE_LOG_CATEGORY_STATIC(LogBathWaterUtilityLabor, Log, All);

UUtilityLeverLaborComponent::UUtilityLeverLaborComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	PivotRotation = new FUtilityPivotRotation();
}

UUtilityLeverLaborComponent::~UUtilityLeverLaborComponent()
{
	delete PivotRotation;
	PivotRotation = nullptr;
}

void UUtilityLeverLaborComponent::BeginPlay()
{
	Super::BeginPlay();
	State = EUtilityLeverLaborState::Idle;
	ElapsedSeconds = 0.0f;
	ReturnElapsedSeconds = 0.0f;
	PoseAlpha = 0.0f;
	bEditorPreviewPose = false;
	RestoreUpPoseInternal();
	SetComponentTickEnabled(false);
}

void UUtilityLeverLaborComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	State = EUtilityLeverLaborState::Idle;
	ElapsedSeconds = 0.0f;
	ReturnElapsedSeconds = 0.0f;
	ReturnStartAlpha = 0.0f;
	PoseAlpha = 0.0f;
	bEditorPreviewPose = false;
	SetComponentTickEnabled(false);
	if (PivotRotation && IsValid(Pivot))
	{
		PivotRotation->Reset(Pivot);
	}
	ClearActiveSource();
	Super::EndPlay(EndPlayReason);
}

void UUtilityLeverLaborComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (State == EUtilityLeverLaborState::Idle || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f)
	{
		return;
	}

	if (State == EUtilityLeverLaborState::Stroking)
	{
		FText FailureReason;
		if (!IsStrokeContextValid(FailureReason))
		{
			StartReturn();
		}
		else
		{
			ElapsedSeconds = FMath::Min(StrokeSeconds, ElapsedSeconds + DeltaTime);
			const float StrokeProgress = FMath::Clamp(ElapsedSeconds / StrokeSeconds, 0.0f, 1.0f);
			PoseAlpha = FMath::Clamp(
				StrokeProgress < 0.5f ? StrokeProgress * 2.0f : 2.0f - StrokeProgress * 2.0f,
				0.0f,
				1.0f);
			ApplyPose();
			if (ElapsedSeconds >= StrokeSeconds)
			{
				if (Operation->ApplyLaborReward(StrokeRewardPoints, FailureReason))
				{
					State = EUtilityLeverLaborState::Idle;
					ElapsedSeconds = 0.0f;
					PoseAlpha = 0.0f;
					ApplyPose();
					ClearActiveSource();
					SetComponentTickEnabled(false);
					return;
				}
				UE_LOG(LogBathWaterUtilityLabor, Warning,
					TEXT("Circulator lever reward failed for %s: %s"),
					*GetNameSafe(GetOwner()), *FailureReason.ToString());
				StartReturn();
			}
		}
	}

	if (State == EUtilityLeverLaborState::Returning)
	{
		const float ReturnDuration = FMath::IsFinite(CancelReturnSeconds)
			? FMath::Max(CancelReturnSeconds, 0.0f) : 0.0f;
		ReturnElapsedSeconds = FMath::Min(ReturnDuration, ReturnElapsedSeconds + DeltaTime);
		const float ReturnAlpha = ReturnDuration > 0.0f
			? FMath::Clamp(ReturnElapsedSeconds / ReturnDuration, 0.0f, 1.0f)
			: 1.0f;
		PoseAlpha = FMath::Lerp(ReturnStartAlpha, 0.0f, ReturnAlpha);
		ApplyPose();
		if (ReturnAlpha >= 1.0f)
		{
			State = EUtilityLeverLaborState::Idle;
			ElapsedSeconds = 0.0f;
			ReturnElapsedSeconds = 0.0f;
			ReturnStartAlpha = 0.0f;
			PoseAlpha = 0.0f;
			ApplyPose();
			ClearActiveSource();
			SetComponentTickEnabled(false);
		}
	}
}

void UUtilityLeverLaborComponent::Configure(
	UUtilityOperationComponent* InOperation,
	USceneComponent* InPivot)
{
	const bool bSamePivot = Pivot == InPivot;
	Operation = InOperation;
	if (!bSamePivot)
	{
		Pivot = InPivot;
		bEditorPreviewPose = false;
		if (PivotRotation)
		{
			PivotRotation->Forget();
		}
	}
	if (State == EUtilityLeverLaborState::Idle)
	{
		RestoreUpPoseInternal();
	}
}

void UUtilityLeverLaborComponent::SetOperatingVolume(UUtilityLeverOperatingVolumeComponent* InVolume)
{
	OperatingVolume = InVolume;
}

FPlayerInteractionQuery UUtilityLeverLaborComponent::QueryInteraction(
	const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	Query.bVisible = true;
	Query.TargetName = LOCTEXT("CirculatorLeverName", "순환기 레버");
	if (State == EUtilityLeverLaborState::Returning)
	{
		Query.ActionName = LOCTEXT("LeverStrokeAction", "레버 왕복");
		Query.FailureReason = LOCTEXT("LeverReturning", "레버가 돌아오는 중");
		return Query;
	}
	if (State == EUtilityLeverLaborState::Stroking)
	{
		if (IsSameActiveSource(Context.InteractionComponent))
		{
			Query.ActionName = LOCTEXT("LeverStrokeInProgress", "레버 왕복 중");
			Query.bPrimaryProgressVisible = true;
			Query.HoldProgress = GetProgress();
		}
		else
		{
			Query.ActionName = LOCTEXT("LeverStrokeAction", "레버 왕복");
			Query.FailureReason = LOCTEXT("LeverOwnedByOther", "다른 사용자가 조작 중");
		}
		return Query;
	}

	Query.ActionName = LOCTEXT("LeverStrokeAction", "레버 왕복");
	Query.bCanInteract = EvaluateStart(Context, Query.FailureReason);
	return Query;
}

bool UUtilityLeverLaborComponent::EvaluateStart(
	const FPlayerInteractionContext& Context,
	FText& OutFailureReason) const
{
	if (State != EUtilityLeverLaborState::Idle)
	{
		OutFailureReason = LOCTEXT("LeverNotIdle", "레버가 이미 움직이고 있습니다.");
		return false;
	}
	if (!HasValidAuthoring(OutFailureReason))
	{
		return false;
	}
	const ABathWaterUtilityFacilityActor* Facility = Cast<ABathWaterUtilityFacilityActor>(GetOwner());
	if (!Facility || !Facility->HasValidUtilityAuthoring(OutFailureReason))
	{
		return false;
	}
	AActor* User = Context.Interactor;
	UPlayerCarryComponent* Carry = Context.CarryComponent;
	UPlayerInteractionComponent* Interaction = Context.InteractionComponent;
	if (!IsValid(User) || !IsValid(Carry) || !IsValid(Interaction)
		|| Carry->GetOwner() != User || Interaction->GetOwner() != User
		|| Interaction->IsInteractionSuppressed())
	{
		OutFailureReason = LOCTEXT("InvalidLeverUser", "레버를 조작할 사용자 상태가 올바르지 않습니다.");
		return false;
	}
	if (Context.HitActor != GetOwner() || Context.HitComponent != OperatingVolume)
	{
		OutFailureReason = LOCTEXT("WrongLeverHit", "순환기 레버 조작 영역을 조준하세요.");
		return false;
	}
	if (!Carry->IsHandEmpty())
	{
		OutFailureReason = LOCTEXT("LeverNeedsFreeHands", "손이 비어 있을 때 레버를 조작할 수 있습니다.");
		return false;
	}
	if (!UtilityLaborInputGuard::ValidateOwnerInput(User, OutFailureReason))
	{
		return false;
	}
	const UFacilityPlacementComponent* Placement = Facility
		? Facility->GetFacilityPlacementComponent() : nullptr;
	if (!Placement || Placement->GetMode() != EPlaceableFacilityMode::Placed
		|| Placement->IsStagedPlacement() || !Placement->IsPlacedDomainActive()
		|| !Operation->IsPlacedClockActive())
	{
		OutFailureReason = LOCTEXT("LeverFacilityNotInstalled", "설치된 순환기 레버만 조작할 수 있습니다.");
		return false;
	}
	if (Operation->IsLaborBlocked())
	{
		OutFailureReason = LOCTEXT("LeverFacilityRecovery", "회수 중에는 레버를 조작할 수 없습니다.");
		return false;
	}
	if (Operation->GetRemainingPoints() >= Operation->GetMaximumPoints())
	{
		OutFailureReason = LOCTEXT("LeverOperationAtCapacity", "설비가 가득 차 있습니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UUtilityLeverLaborComponent::TryStartStroke(
	const FPlayerInteractionContext& Context,
	FText& OutFailureReason)
{
	if (!EvaluateStart(Context, OutFailureReason))
	{
		return false;
	}
	FHitResult Hit;
	if (!Context.InteractionComponent->GetCurrentFocusHit(Hit)
		|| Hit.GetActor() != GetOwner() || Hit.GetComponent() != OperatingVolume)
	{
		OutFailureReason = LOCTEXT("LeverFreshTraceMismatch", "레버 조작 영역을 다시 조준하세요.");
		return false;
	}
	ActiveSource = Context.InteractionComponent;
	ActiveCarry = Context.CarryComponent;
	ActiveUser = Context.Interactor;
	State = EUtilityLeverLaborState::Stroking;
	ElapsedSeconds = 0.0f;
	ReturnElapsedSeconds = 0.0f;
	ReturnStartAlpha = 0.0f;
	PoseAlpha = 0.0f;
	SetComponentTickEnabled(true);
	OutFailureReason = FText::GetEmpty();
	return true;
}

void UUtilityLeverLaborComponent::NotifySourceFocusEnded(const UPlayerInteractionComponent& Source)
{
	if (State == EUtilityLeverLaborState::Stroking && ActiveSource.Get() == &Source)
	{
		StartReturn();
	}
}

void UUtilityLeverLaborComponent::CancelForRecoveryStart()
{
	StartReturn();
}

bool UUtilityLeverLaborComponent::HasValidAuthoring(FText& OutFailureReason) const
{
	if (!IsValid(Operation) || !IsValid(Pivot) || !IsValid(OperatingVolume)
		|| !FMath::IsFinite(StrokeSeconds) || StrokeSeconds <= 0.0f
		|| !FMath::IsFinite(StrokeRewardPoints) || StrokeRewardPoints <= 0.0f
		|| !FMath::IsFinite(CancelReturnSeconds) || CancelReturnSeconds < 0.0f
		|| !FMath::IsFinite(LocalRotationAxis.X) || !FMath::IsFinite(LocalRotationAxis.Y)
		|| !FMath::IsFinite(LocalRotationAxis.Z) || LocalRotationAxis.IsNearlyZero()
		|| !FMath::IsFinite(DownAngleDegrees) || DownAngleDegrees == 0.0f)
	{
		OutFailureReason = LOCTEXT("InvalidLeverAuthoring", "순환기 레버의 Operation, pivot, 조작 영역 또는 동작 값이 올바르지 않습니다.");
		return false;
	}
	if (!Operation->HasValidAuthoring(OutFailureReason))
	{
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

void UUtilityLeverLaborComponent::PreviewDownPose()
{
	if (!IsEditorPreviewWorld())
	{
		return;
	}
	FText FailureReason;
	if (!HasValidAuthoring(FailureReason))
	{
		return;
	}
	if (!PivotRotation->HasBaseline())
	{
		PivotRotation->CaptureBaseline(Pivot);
	}
	bEditorPreviewPose = true;
	PoseAlpha = 1.0f;
	ApplyPose();
}

void UUtilityLeverLaborComponent::RestoreUpPose()
{
	if (!IsEditorPreviewWorld())
	{
		return;
	}
	bEditorPreviewPose = false;
	RestoreUpPoseInternal();
}

bool UUtilityLeverLaborComponent::IsEditorPreviewWorld() const
{
	const UWorld* World = GetWorld();
	return World && (World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::EditorPreview);
}

void UUtilityLeverLaborComponent::RestoreUpPoseInternal()
{
	if (!IsValid(Pivot) || !PivotRotation)
	{
		return;
	}
	PoseAlpha = 0.0f;
	if (!PivotRotation->HasBaseline())
	{
		PivotRotation->CaptureBaseline(Pivot);
	}
	PivotRotation->Reset(Pivot);
}

float UUtilityLeverLaborComponent::GetProgress() const
{
	return State == EUtilityLeverLaborState::Stroking && StrokeSeconds > 0.0f
		? FMath::Clamp(ElapsedSeconds / StrokeSeconds, 0.0f, 1.0f) : 0.0f;
}

bool UUtilityLeverLaborComponent::IsStrokeContextValid(FText& OutFailureReason) const
{
	UPlayerInteractionComponent* Interaction = ActiveSource.Get();
	UPlayerCarryComponent* Carry = ActiveCarry.Get();
	AActor* User = ActiveUser.Get();
	if (!IsValid(Interaction) || !IsValid(Carry) || !IsValid(User)
		|| Interaction->GetOwner() != User || Carry->GetOwner() != User
		|| Interaction->IsInteractionSuppressed() || !Carry->IsHandEmpty()
		|| !UtilityLaborInputGuard::ValidateOwnerInput(User, OutFailureReason))
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT("LeverContextExpired", "레버 조작 조건이 유지되지 않았습니다.");
		}
		return false;
	}
	const ABathWaterUtilityFacilityActor* Facility = Cast<ABathWaterUtilityFacilityActor>(GetOwner());
	const UFacilityPlacementComponent* Placement = Facility
		? Facility->GetFacilityPlacementComponent() : nullptr;
	if (!Placement || Placement->GetMode() != EPlaceableFacilityMode::Placed
		|| Placement->IsStagedPlacement() || !Placement->IsPlacedDomainActive()
		|| !Operation || !Operation->IsPlacedClockActive() || Operation->IsLaborBlocked())
	{
		OutFailureReason = LOCTEXT("LeverConditionsChanged", "레버 조작 조건이 변경되었습니다.");
		return false;
	}
	FHitResult Hit;
	if (!Interaction->GetCurrentFocusHit(Hit)
		|| Hit.GetActor() != GetOwner() || Hit.GetComponent() != OperatingVolume)
	{
		OutFailureReason = LOCTEXT("LeverFocusLost", "레버에서 시선을 떼었습니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UUtilityLeverLaborComponent::IsSameActiveSource(const UPlayerInteractionComponent* Source) const
{
	return Source && ActiveSource.Get() == Source;
}

void UUtilityLeverLaborComponent::StartReturn()
{
	if (State == EUtilityLeverLaborState::Idle || State == EUtilityLeverLaborState::Returning)
	{
		return;
	}
	ReturnStartAlpha = PoseAlpha;
	ReturnElapsedSeconds = 0.0f;
	if (!FMath::IsFinite(CancelReturnSeconds) || CancelReturnSeconds <= 0.0f)
	{
		State = EUtilityLeverLaborState::Idle;
		ElapsedSeconds = 0.0f;
		ReturnStartAlpha = 0.0f;
		PoseAlpha = 0.0f;
		ApplyPose();
		ClearActiveSource();
		SetComponentTickEnabled(false);
		return;
	}
	State = EUtilityLeverLaborState::Returning;
	SetComponentTickEnabled(true);
}

void UUtilityLeverLaborComponent::ApplyPose()
{
	if (!IsValid(Pivot) || !PivotRotation)
	{
		return;
	}
	if (!PivotRotation->HasBaseline())
	{
		PivotRotation->CaptureBaseline(Pivot);
	}
	PivotRotation->Apply(Pivot, LocalRotationAxis, DownAngleDegrees * PoseAlpha);
}

void UUtilityLeverLaborComponent::ClearActiveSource()
{
	ActiveSource.Reset();
	ActiveCarry.Reset();
	ActiveUser.Reset();
}

#if WITH_EDITOR
void UUtilityLeverLaborComponent::PreSave(FObjectPreSaveContext SaveContext)
{
	if (bEditorPreviewPose && IsEditorPreviewWorld())
	{
		State = EUtilityLeverLaborState::Idle;
		ElapsedSeconds = 0.0f;
		ReturnElapsedSeconds = 0.0f;
		ReturnStartAlpha = 0.0f;
		PoseAlpha = 0.0f;
		SetComponentTickEnabled(false);
		ClearActiveSource();
		bEditorPreviewPose = false;
		RestoreUpPoseInternal();
	}
	Super::PreSave(SaveContext);
}
#endif

#undef LOCTEXT_NAMESPACE
