#include "Facility/BathWaterControlComponent.h"

#include "Facility/BathhouseBathFacilityActor.h"
#include "Engine/StaticMesh.h"
#include "Placement/FacilityPlacementComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshResources.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BathWaterControlComponent"

UBathWaterControlComponent::UBathWaterControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetGenerateOverlapEvents(false);
	SetSimulatePhysics(false);
	SetCanEverAffectNavigation(false);
}

void UBathWaterControlComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeClosedPose();
}

void UBathWaterControlComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetComponentTickEnabled(false);
	RecoveryMotionSnapshot.Reset();
	bMoving = false;
	bRecoveryFrozen = false;
	Super::EndPlay(EndPlayReason);
}

void UBathWaterControlComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bMoving || bRecoveryFrozen)
	{
		SetComponentTickEnabled(false);
		return;
	}
	if (!HasFiniteMotionState() || !FMath::IsFinite(DeltaTime))
	{
		MotionElapsed = 0.0f;
		MotionDuration = 0.0f;
		bMoving = false;
		SetComponentTickEnabled(false);
		return;
	}
	MotionElapsed = FMath::Min(MotionElapsed + FMath::Max(0.0f, DeltaTime), MotionDuration);
	const float LinearAlpha = FMath::Clamp(MotionElapsed / MotionDuration, 0.0f, 1.0f);
	const float EasedAlpha = FMath::SmoothStep(0.0f, 1.0f, LinearAlpha);
	const FQuat NextRotation = FQuat::Slerp(MotionStartRotation, MotionTargetRotation, EasedAlpha).GetNormalized();
	if (NextRotation.ContainsNaN())
	{
		MotionElapsed = 0.0f;
		MotionDuration = 0.0f;
		bMoving = false;
		SetComponentTickEnabled(false);
		return;
	}
	SetRelativeRotation(NextRotation);
	if (LinearAlpha >= 1.0f)
	{
		StopAtRotation(MotionTargetRotation);
	}
}

#if WITH_EDITOR
EDataValidationResult UBathWaterControlComponent::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FText MotionFailureReason;
	if (!ValidateMotionAuthoring(MotionFailureReason))
	{
		Context.AddError(MotionFailureReason);
		Result = EDataValidationResult::Invalid;
	}
	const ABathhouseBathFacilityActor* BathOwner = GetBathOwner();
	if (BathOwner && BathOwner->GetClass() != ABathhouseBathFacilityActor::StaticClass()
		&& !HasValidInteractionGeometry())
	{
		Context.AddError(LOCTEXT("InvalidInteractionGeometry", "A concrete bath control requires a Static Mesh with query collision that blocks Visibility."));
		Result = EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

FPlayerInteractionQuery UBathWaterControlComponent::QueryInteraction(
	const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	const ABathhouseBathFacilityActor* Bath = GetBathOwner();
	const UBathWaterStateComponent* Water = Bath ? Bath->GetBathWaterState() : nullptr;
	const UFacilityPlacementComponent* Placement = Bath ? Bath->GetFacilityPlacementComponent() : nullptr;
	if (!Bath || !Water || !Placement || Placement->GetMode() != EPlaceableFacilityMode::Placed
		|| Placement->IsStagedPlacement() || !Placement->IsPlacedDomainActive()
		|| (Context.HitComponent && Context.HitComponent != this))
	{
		return Query;
	}

	Query.bVisible = true;
	Query.TargetName = LOCTEXT("BathTarget", "욕탕");
	const bool bOpen = Water->IsControlOpen(ControlType);
	if (ControlType == EBathWaterControlType::FillValve)
	{
		Query.ActionName = bOpen ? LOCTEXT("CloseFill", "급수밸브 닫기") : LOCTEXT("OpenFill", "급수밸브 열기");
	}
	else
	{
		Query.ActionName = bOpen ? LOCTEXT("CloseDrain", "배수레버 닫기") : LOCTEXT("OpenDrain", "배수레버 열기");
	}

	if (!HasValidInteractionGeometry())
	{
		Query.FailureReason = LOCTEXT("MissingInteractionGeometry", "물 조작부 충돌 설정이 올바르지 않습니다");
		return Query;
	}
	FText MotionFailureReason;
	if (!ValidateMotionAuthoring(MotionFailureReason))
	{
		Query.FailureReason = MotionFailureReason;
		return Query;
	}
	if (bMoving)
	{
		Query.FailureReason = LOCTEXT("ControlMoving", "작동 중입니다");
		return Query;
	}
	FText FailureReason;
	Query.bCanInteract = Water->CanSetControlOpen(ControlType, !bOpen, FailureReason);
	Query.FailureReason = FailureReason;
	return Query;
}

FPlayerInteractionResult UBathWaterControlComponent::ExecuteInteraction(
	const FPlayerInteractionContext& Context)
{
	const FPlayerInteractionQuery Query = QueryInteraction(Context);
	if (!Query.bVisible || !Query.bCanInteract)
	{
		return FPlayerInteractionResult::Failed(Query.FailureReason);
	}
	ABathhouseBathFacilityActor* Bath = GetBathOwner();
	UBathWaterStateComponent* Water = Bath ? Bath->GetBathWaterState() : nullptr;
	FText FailureReason;
	if (!Water || !Water->RequestSetControlOpen(
		ControlType,
		!Water->IsControlOpen(ControlType),
		EBathWaterControlChangeReason::PlayerInteraction,
		FailureReason))
	{
		return FPlayerInteractionResult::Failed(FailureReason.IsEmpty() ? Query.FailureReason : FailureReason);
	}
	return FPlayerInteractionResult::Succeeded();
}

void UBathWaterControlComponent::InitializeClosedPose()
{
	if (bClosedPoseInitialized)
	{
		return;
	}
	const FQuat AuthoredRotation = GetRelativeRotation().Quaternion();
	ClosedRotation = AuthoredRotation.ContainsNaN() ? FQuat::Identity : AuthoredRotation.GetNormalized();
	bClosedPoseInitialized = true;
	StopAtRotation(ClosedRotation);
}

void UBathWaterControlComponent::ApplyLogicalState(
	const bool bOpen,
	const EBathWaterControlChangeReason Reason)
{
	InitializeClosedPose();
	if (bRecoveryFrozen)
	{
		if (Reason == EBathWaterControlChangeReason::RecoveryCommit)
		{
			StopAtRotation(ClosedRotation);
		}
		return;
	}
	if (Reason == EBathWaterControlChangeReason::Reset)
	{
		StopAtRotation(bOpen ? CalculateOpenRotation() : ClosedRotation);
		return;
	}
	StartMotion(bOpen, Reason == EBathWaterControlChangeReason::FullAutoClose);
}

void UBathWaterControlComponent::BeginRecoveryFreeze()
{
	if (RecoveryMotionSnapshot.IsSet())
	{
		return;
	}
	RecoveryMotionSnapshot.Emplace(FRecoveryMotionSnapshot{
		GetRelativeRotation().Quaternion().GetNormalized(),
		MotionStartRotation,
		MotionTargetRotation,
		MotionElapsed,
		MotionDuration,
		bMoving,
		bOpening,
		IsComponentTickEnabled() });
	bRecoveryFrozen = true;
	SetComponentTickEnabled(false);
}

void UBathWaterControlComponent::CancelRecoveryFreeze()
{
	if (!RecoveryMotionSnapshot.IsSet())
	{
		return;
	}
	const FRecoveryMotionSnapshot Snapshot = RecoveryMotionSnapshot.GetValue();
	RecoveryMotionSnapshot.Reset();
	SetRelativeRotation(Snapshot.CurrentRotation);
	MotionStartRotation = Snapshot.StartRotation;
	MotionTargetRotation = Snapshot.TargetRotation;
	MotionElapsed = Snapshot.Elapsed;
	MotionDuration = Snapshot.Duration;
	bMoving = Snapshot.bMoving;
	bOpening = Snapshot.bOpening;
	bRecoveryFrozen = false;
	SetComponentTickEnabled(Snapshot.bTickEnabled && Snapshot.bMoving);
}

void UBathWaterControlComponent::PrepareRecoveryCommit()
{
	if (RecoveryMotionSnapshot.IsSet())
	{
		StopAtRotation(ClosedRotation);
	}
}

bool UBathWaterControlComponent::HasValidInteractionGeometry() const
{
	const UStaticMesh* Mesh = GetStaticMesh();
	if (!Mesh || !IsQueryCollisionEnabled()
		|| GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block)
	{
		return false;
	}

	const UBodySetup* BodySetup = Mesh->GetBodySetup();
	if (!BodySetup)
	{
		return false;
	}
	const bool bHasSimpleGeometry = BodySetup->AggGeom.GetElementCount() > 0;
	const FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
	const bool bHasComplexGeometry = BodySetup->CollisionTraceFlag != CTF_UseSimpleAsComplex
		&& RenderData && !RenderData->LODResources.IsEmpty()
		&& RenderData->LODResources[0].GetNumTriangles() > 0;
	return bHasSimpleGeometry || bHasComplexGeometry;
}

bool UBathWaterControlComponent::ValidateMotionAuthoring(FText& OutFailureReason) const
{
	OutFailureReason = FText::GetEmpty();
	if (LocalRotationAxis.ContainsNaN() || LocalRotationAxis.IsNearlyZero())
	{
		OutFailureReason = LOCTEXT("InvalidAxis", "LocalRotationAxis must be finite and non-zero.");
		return false;
	}
	if (!FMath::IsFinite(OpenAngleDegrees))
	{
		OutFailureReason = LOCTEXT("InvalidAngle", "OpenAngleDegrees must be finite.");
		return false;
	}
	if (!FMath::IsFinite(RotationDurationSeconds) || RotationDurationSeconds <= 0.0f)
	{
		OutFailureReason = LOCTEXT("InvalidDuration", "RotationDurationSeconds must be finite and greater than zero.");
		return false;
	}
	return true;
}

bool UBathWaterControlComponent::HasFiniteMotionState() const
{
	return FMath::IsFinite(MotionElapsed) && FMath::IsFinite(MotionDuration)
		&& MotionDuration > 0.0f
		&& !MotionStartRotation.ContainsNaN()
		&& !MotionTargetRotation.ContainsNaN();
}

FQuat UBathWaterControlComponent::CalculateOpenRotation() const
{
	const FQuat SafeClosedRotation = ClosedRotation.ContainsNaN()
		? FQuat::Identity
		: ClosedRotation.GetNormalized();
	FText MotionFailureReason;
	if (!ValidateMotionAuthoring(MotionFailureReason))
	{
		return SafeClosedRotation;
	}
	const FVector Axis = LocalRotationAxis.GetSafeNormal();
	const FQuat OpenRotation = (SafeClosedRotation * FQuat(
		Axis,
		FMath::DegreesToRadians(OpenAngleDegrees))).GetNormalized();
	return OpenRotation.ContainsNaN() ? SafeClosedRotation : OpenRotation;
}

void UBathWaterControlComponent::StartMotion(const bool bOpen, const bool bAllowMidMotionReversal)
{
	if (bMoving && !bAllowMidMotionReversal)
	{
		return;
	}
	FText MotionFailureReason;
	const FQuat CurrentRotation = GetRelativeRotation().Quaternion();
	if (!ValidateMotionAuthoring(MotionFailureReason) || CurrentRotation.ContainsNaN()
		|| ClosedRotation.ContainsNaN())
	{
		MotionElapsed = 0.0f;
		MotionDuration = 0.0f;
		bMoving = false;
		SetComponentTickEnabled(false);
		return;
	}
	MotionStartRotation = CurrentRotation.GetNormalized();
	MotionTargetRotation = bOpen ? CalculateOpenRotation() : ClosedRotation.GetNormalized();
	if (MotionStartRotation.ContainsNaN() || MotionTargetRotation.ContainsNaN())
	{
		MotionElapsed = 0.0f;
		MotionDuration = 0.0f;
		bMoving = false;
		SetComponentTickEnabled(false);
		return;
	}
	bOpening = bOpen;
	MotionElapsed = 0.0f;
	const float FullAngle = FQuat::ErrorAutoNormalize(ClosedRotation, CalculateOpenRotation());
	const float RemainingAngle = FQuat::ErrorAutoNormalize(MotionStartRotation, MotionTargetRotation);
	const float Ratio = FullAngle > SMALL_NUMBER ? FMath::Clamp(RemainingAngle / FullAngle, 0.0f, 1.0f) : 0.0f;
	MotionDuration = FMath::Max(0.0f, RotationDurationSeconds) * Ratio;
	if (MotionDuration <= SMALL_NUMBER)
	{
		StopAtRotation(MotionTargetRotation);
		return;
	}
	bMoving = true;
	SetComponentTickEnabled(true);
}

void UBathWaterControlComponent::StopAtRotation(const FQuat& Rotation)
{
	const FQuat SafeRotation = Rotation.ContainsNaN() ? FQuat::Identity : Rotation.GetNormalized();
	SetRelativeRotation(SafeRotation);
	MotionStartRotation = SafeRotation;
	MotionTargetRotation = SafeRotation;
	MotionElapsed = 0.0f;
	MotionDuration = 0.0f;
	bMoving = false;
	SetComponentTickEnabled(false);
}

ABathhouseBathFacilityActor* UBathWaterControlComponent::GetBathOwner() const
{
	return Cast<ABathhouseBathFacilityActor>(GetOwner());
}

#undef LOCTEXT_NAMESPACE
