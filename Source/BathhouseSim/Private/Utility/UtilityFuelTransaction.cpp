#include "UtilityFuelTransaction.h"

#include "Components/PrimitiveComponent.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Utility/BathWaterFuelUtilityFacilityActor.h"
#include "Utility/UtilityFuelIntakeVolumeComponent.h"
#include "Utility/UtilityLaborInputGuard.h"
#include "Utility/UtilityFuelSupplyActor.h"
#include "Utility/UtilityOperationComponent.h"
#include "Utility/UtilityShovelActor.h"

#define LOCTEXT_NAMESPACE "UtilityFuelTransaction"

namespace
{
FUtilityFuelResult Fail(const EUtilityFuelFailure Failure, const FText& Reason)
{
	return FUtilityFuelResult::Failed(Failure, Reason);
}

FUtilityFuelLoad EmptyLoad()
{
	return FUtilityFuelLoad();
}
}

FUtilityFuelResult FUtilityFuelTransaction::EvaluateScoop(
	const FPlayerInteractionContext& Context,
	const AUtilityFuelSupplyActor& Supply)
{
	AUtilityShovelActor* Shovel = GetHeldShovel(Context);
	if (!IsValid(Shovel))
	{
		return Fail(EUtilityFuelFailure::NoShovel,
			LOCTEXT("ScoopNeedsShovel", "삽을 들고 있어야 합니다."));
	}
	if (!IsValid(&Supply))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget,
			LOCTEXT("InvalidSupplyTarget", "연료 공급함 상태가 올바르지 않습니다."));
	}

	FText FailureReason;
	if (!ValidateHeldContext(*Shovel, Context, FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget, FailureReason);
	}
	if (!Shovel->HasValidAuthoring(FailureReason) || !Supply.HasValidAuthoring(FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidAuthoring, FailureReason);
	}
	if (!Shovel->FuelLoad.IsValid())
	{
		return Fail(EUtilityFuelFailure::InvalidLoad,
			LOCTEXT("InvalidExistingShovelLoad", "삽의 연료 상태가 올바르지 않습니다."));
	}
	if (!Shovel->FuelLoad.IsEmpty())
	{
		return Fail(EUtilityFuelFailure::ShovelAlreadyLoaded,
			LOCTEXT("ScoopOnlyWhenEmpty", "삽이 비어 있을 때만 연료를 퍼담을 수 있습니다."));
	}

	FUtilityFuelLoad CandidateLoad;
	CandidateLoad.Kind = Supply.GetFuelKind();
	CandidateLoad.Points = Supply.GetScoopPoints();
	if (!CandidateLoad.IsValid() || CandidateLoad.IsEmpty())
	{
		return Fail(EUtilityFuelFailure::InvalidLoad,
			LOCTEXT("InvalidScoopCandidate", "공급할 연료 종류와 양이 올바르지 않습니다."));
	}
	return FUtilityFuelResult::Succeeded(CandidateLoad.Points);
}

FUtilityFuelResult FUtilityFuelTransaction::EvaluateReturn(
	const FPlayerInteractionContext& Context,
	const AUtilityFuelSupplyActor& Supply)
{
	AUtilityShovelActor* Shovel = GetHeldShovel(Context);
	if (!IsValid(Shovel))
	{
		return Fail(EUtilityFuelFailure::NoShovel,
			LOCTEXT("ReturnNeedsShovel", "연료가 든 삽을 들고 있어야 합니다."));
	}
	if (!IsValid(&Supply))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget,
			LOCTEXT("InvalidReturnSupply", "연료 공급함 상태가 올바르지 않습니다."));
	}

	FText FailureReason;
	if (!ValidateHeldContext(*Shovel, Context, FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget, FailureReason);
	}
	if (!Shovel->HasValidAuthoring(FailureReason) || !Supply.HasValidAuthoring(FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidAuthoring, FailureReason);
	}
	if (!Shovel->FuelLoad.IsValid())
	{
		return Fail(EUtilityFuelFailure::InvalidLoad,
			LOCTEXT("InvalidReturnLoad", "삽의 연료 상태가 올바르지 않습니다."));
	}
	if (Shovel->FuelLoad.IsEmpty())
	{
		return Fail(EUtilityFuelFailure::ShovelEmpty,
			LOCTEXT("ReturnEmptyShovel", "삽에 반환할 연료가 없습니다."));
	}
	if (Shovel->FuelLoad.Kind != Supply.GetFuelKind())
	{
		return Fail(EUtilityFuelFailure::WrongFuel,
			LOCTEXT("ReturnWrongFuel", "같은 종류의 연료가 든 삽만 반환할 수 있습니다."));
	}
	return FUtilityFuelResult::Succeeded(Shovel->FuelLoad.Points);
}

FUtilityFuelResult FUtilityFuelTransaction::EvaluateInsert(
	const FPlayerInteractionContext& Context,
	const UUtilityFuelIntakeVolumeComponent& Intake)
{
	AUtilityShovelActor* Shovel = GetHeldShovel(Context);
	if (!IsValid(Shovel))
	{
		return Fail(EUtilityFuelFailure::NoShovel,
			LOCTEXT("InsertNeedsShovel", "삽을 들고 있어야 합니다."));
	}
	if (!IsValid(&Intake))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget,
			LOCTEXT("InvalidIntakeTarget", "보일러 투입구 상태가 올바르지 않습니다."));
	}

	ABathWaterFuelUtilityFacilityActor* FuelFacility = Cast<ABathWaterFuelUtilityFacilityActor>(Intake.GetOwner());
	UUtilityOperationComponent* Operation = FuelFacility ? FuelFacility->GetUtilityOperation() : nullptr;
	if (!IsValid(FuelFacility) || !IsValid(Operation) || FuelFacility->GetFuelIntakeVolume() != &Intake)
	{
		return Fail(EUtilityFuelFailure::InvalidTarget,
			LOCTEXT("WrongIntake", "연료 설비의 실제 투입구를 조준하세요."));
	}

	FText FailureReason;
	if (!ValidateHeldContext(*Shovel, Context, FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget, FailureReason);
	}
	if (!Shovel->HasValidAuthoring(FailureReason)
		|| !FuelFacility->HasValidUtilityAuthoring(FailureReason)
		|| !Operation->HasValidAuthoring(FailureReason)
		|| !Intake.HasValidAuthoring(FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidAuthoring, FailureReason);
	}
	if (!Shovel->FuelLoad.IsValid())
	{
		return Fail(EUtilityFuelFailure::InvalidLoad,
			LOCTEXT("InvalidInsertLoad", "삽의 연료 상태가 올바르지 않습니다."));
	}
	if (Shovel->FuelLoad.IsEmpty())
	{
		return Fail(EUtilityFuelFailure::ShovelEmpty,
			LOCTEXT("ShovelHasNoFuel", "삽에 연료가 없습니다."));
	}
	if (Shovel->FuelLoad.Kind != FuelFacility->GetAcceptedFuelKind())
	{
		return Fail(EUtilityFuelFailure::WrongFuel,
			FText::Format(LOCTEXT("WrongInsertFuel", "{0}에는 {1}만 넣을 수 있습니다."),
				FuelFacility->GetFuelFacilityDisplayName(),
				GetUtilityFuelKindDisplayName(FuelFacility->GetAcceptedFuelKind())));
	}

	const UFacilityPlacementComponent* Placement = FuelFacility->GetFacilityPlacementComponent();
	if (!Placement || Placement->IsStagedPlacement() || !Placement->IsPlacedDomainActive()
		|| !Operation->IsPlacedClockActive())
	{
		return Fail(EUtilityFuelFailure::NotInstalled,
			LOCTEXT("FuelFacilityNotInstalled", "설치된 연료 설비에만 연료를 넣을 수 있습니다."));
	}
	if (Operation->IsLaborBlocked())
	{
		return Fail(EUtilityFuelFailure::RecoveryInProgress,
			LOCTEXT("FuelFacilityRecoveryInProgress", "회수 중에는 재료를 넣을 수 없습니다."));
	}

	const float CurrentPoints = Operation->GetRemainingPoints();
	const float MaximumPoints = Operation->GetMaximumPoints();
	if (!FMath::IsFinite(CurrentPoints) || !FMath::IsFinite(MaximumPoints) || MaximumPoints <= 0.0f)
	{
		return Fail(EUtilityFuelFailure::InvalidAuthoring,
			LOCTEXT("InvalidOperationCapacity", "설비 가동 용량을 확인할 수 없습니다."));
	}
	if (CurrentPoints >= MaximumPoints)
	{
		return Fail(EUtilityFuelFailure::CapacityFull,
			LOCTEXT("FuelFacilityFull", "설비가 가득 차 있습니다."));
	}
	return FUtilityFuelResult::Succeeded(FMath::Min(MaximumPoints, CurrentPoints + Shovel->FuelLoad.Points) - CurrentPoints);
}

FUtilityFuelResult FUtilityFuelTransaction::Scoop(
	AUtilityShovelActor& Shovel,
	AUtilityFuelSupplyActor& Supply,
	const FPlayerInteractionContext& Context)
{
	FUtilityFuelResult Evaluation = EvaluateScoop(Context, Supply);
	if (!Evaluation.bSucceeded)
	{
		return Evaluation;
	}

	FText FailureReason;
	if (!ValidateFreshHit(Context.InteractionComponent, Context.Interactor, &Supply, Supply.GetSupplyMesh(), FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget, FailureReason);
	}
	if (Shovel.bFuelMutationInProgress || Supply.bFuelMutationInProgress)
	{
		return Fail(EUtilityFuelFailure::TransactionBusy,
			LOCTEXT("ScoopAlreadyInProgress", "연료 이동이 이미 처리 중입니다."));
	}

	TGuardValue<bool> ShovelGuard(Shovel.bFuelMutationInProgress, true);
	TGuardValue<bool> SupplyGuard(Supply.bFuelMutationInProgress, true);
	Evaluation = EvaluateScoop(Context, Supply);
	if (!Evaluation.bSucceeded || !ValidateHeldContext(Shovel, Context, FailureReason))
	{
		return Evaluation.bSucceeded
			? Fail(EUtilityFuelFailure::InvalidTarget, FailureReason)
			: Evaluation;
	}

	FUtilityFuelLoad CandidateLoad;
	CandidateLoad.Kind = Supply.FuelKind;
	CandidateLoad.Points = Supply.ScoopPoints;
	if (!CandidateLoad.IsValid() || CandidateLoad.IsEmpty())
	{
		return Fail(EUtilityFuelFailure::InvalidLoad,
			LOCTEXT("InvalidScoopLoad", "공급할 연료 양이 올바르지 않습니다."));
	}
	Shovel.SetFuelLoadSilently(CandidateLoad);
	const TWeakObjectPtr<AUtilityShovelActor> WeakShovel(&Shovel);
	if (WeakShovel.IsValid())
	{
		Shovel.PublishFuelLoadChanged();
	}
	return FUtilityFuelResult::Succeeded(CandidateLoad.Points);
}

FUtilityFuelResult FUtilityFuelTransaction::Insert(
	AUtilityShovelActor& Shovel,
	UUtilityFuelIntakeVolumeComponent& Intake,
	const FPlayerInteractionContext& Context)
{
	FUtilityFuelResult Evaluation = EvaluateInsert(Context, Intake);
	if (!Evaluation.bSucceeded)
	{
		return Evaluation;
	}

	ABathWaterFuelUtilityFacilityActor* FuelFacility = Cast<ABathWaterFuelUtilityFacilityActor>(Intake.GetOwner());
	UUtilityOperationComponent* Operation = FuelFacility ? FuelFacility->GetUtilityOperation() : nullptr;
	FText FailureReason;
	if (!IsValid(FuelFacility) || !IsValid(Operation)
		|| !ValidateFreshHit(Context.InteractionComponent, Context.Interactor, FuelFacility, &Intake, FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget,
			FailureReason.IsEmpty() ? LOCTEXT("InvalidInsertTarget", "연료 설비의 실제 투입구를 조준하세요.") : FailureReason);
	}
	if (Shovel.bFuelMutationInProgress || Operation->bMutationInProgress)
	{
		return Fail(EUtilityFuelFailure::TransactionBusy,
			LOCTEXT("InsertAlreadyInProgress", "연료 이동이 이미 처리 중입니다."));
	}

	TGuardValue<bool> ShovelGuard(Shovel.bFuelMutationInProgress, true);
	TGuardValue<bool> OperationGuard(Operation->bMutationInProgress, true);
	Evaluation = EvaluateInsert(Context, Intake);
	if (!Evaluation.bSucceeded || !ValidateHeldContext(Shovel, Context, FailureReason))
	{
		return Evaluation.bSucceeded
			? Fail(EUtilityFuelFailure::InvalidTarget, FailureReason)
			: Evaluation;
	}

	const double GameTimeSeconds = Operation->GetGameTimeSeconds();
	const float CurrentPoints = Operation->GetRemainingPointsAt(GameTimeSeconds);
	const bool bWasProvidingCapacity = Operation->bPlacedClockActive && CurrentPoints > 0.0f;
	if (CurrentPoints >= Operation->MaxOperationPoints)
	{
		return Fail(EUtilityFuelFailure::CapacityFull,
			LOCTEXT("FuelFacilityFull", "설비가 가득 차 있습니다."));
	}
	const float CandidatePoints = FMath::Min(
		Operation->MaxOperationPoints,
		CurrentPoints + Shovel.FuelLoad.Points);
	if (!FMath::IsFinite(CandidatePoints) || CandidatePoints <= CurrentPoints)
	{
		return Fail(EUtilityFuelFailure::InvalidLoad,
			LOCTEXT("InvalidFuelFacilityCandidate", "연료 투입 결과를 계산할 수 없습니다."));
	}

	Operation->SetRemainingPointsSilently(CandidatePoints, GameTimeSeconds);
	Shovel.SetFuelLoadSilently(EmptyLoad());
	const TWeakObjectPtr<AUtilityShovelActor> WeakShovel(&Shovel);
	Operation->PublishCommittedChanges(CurrentPoints, bWasProvidingCapacity);
	if (WeakShovel.IsValid())
	{
		Shovel.PublishFuelLoadChanged();
	}
	return FUtilityFuelResult::Succeeded(CandidatePoints - CurrentPoints);
}

FUtilityFuelResult FUtilityFuelTransaction::Return(
	AUtilityShovelActor& Shovel,
	AUtilityFuelSupplyActor& Supply,
	const FPlayerInteractionContext& Context)
{
	FUtilityFuelResult Evaluation = EvaluateReturn(Context, Supply);
	if (!Evaluation.bSucceeded)
	{
		return Evaluation;
	}

	FText FailureReason;
	if (!ValidateFreshHit(Context.InteractionComponent, Context.Interactor, &Supply, Supply.GetSupplyMesh(), FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget, FailureReason);
	}
	if (Shovel.bFuelMutationInProgress || Supply.bFuelMutationInProgress)
	{
		return Fail(EUtilityFuelFailure::TransactionBusy,
			LOCTEXT("ReturnAlreadyInProgress", "연료 이동이 이미 처리 중입니다."));
	}

	TGuardValue<bool> ShovelGuard(Shovel.bFuelMutationInProgress, true);
	TGuardValue<bool> SupplyGuard(Supply.bFuelMutationInProgress, true);
	Evaluation = EvaluateReturn(Context, Supply);
	if (!Evaluation.bSucceeded || !ValidateHeldContext(Shovel, Context, FailureReason))
	{
		return Evaluation.bSucceeded
			? Fail(EUtilityFuelFailure::InvalidTarget, FailureReason)
			: Evaluation;
	}

	const float ReturnedPoints = Shovel.FuelLoad.Points;
	Shovel.SetFuelLoadSilently(EmptyLoad());
	const TWeakObjectPtr<AUtilityShovelActor> WeakShovel(&Shovel);
	if (WeakShovel.IsValid())
	{
		Shovel.PublishFuelLoadChanged();
	}
	return FUtilityFuelResult::Succeeded(ReturnedPoints);
}

AUtilityShovelActor* FUtilityFuelTransaction::GetHeldShovel(const FPlayerInteractionContext& Context)
{
	return Context.CarryComponent
		? Cast<AUtilityShovelActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
}

bool FUtilityFuelTransaction::ValidateFreshHit(
	UPlayerInteractionComponent* Interaction,
	AActor* User,
	AActor* ExpectedActor,
	UPrimitiveComponent* ExpectedComponent,
	FText& OutFailureReason)
{
	FHitResult Hit;
	if (!IsValid(Interaction) || !IsValid(User) || !IsValid(ExpectedActor)
		|| !IsValid(ExpectedComponent) || Interaction->GetOwner() != User
		|| Interaction->IsInteractionSuppressed() || !Interaction->GetCurrentFocusHit(Hit)
		|| Hit.GetActor() != ExpectedActor || Hit.GetComponent() != ExpectedComponent)
	{
		OutFailureReason = LOCTEXT("FreshTraceMismatch", "조준 대상이 바뀌었거나 앞을 가리는 물체가 있습니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool FUtilityFuelTransaction::ValidateHeldContext(
	const AUtilityShovelActor& Shovel,
	const FPlayerInteractionContext& Context,
	FText& OutFailureReason)
{
	if (!IsValid(Context.Interactor) || !IsValid(Context.CarryComponent)
		|| !IsValid(Context.InteractionComponent)
		|| Context.CarryComponent->GetOwner() != Context.Interactor
		|| Context.CarryComponent->GetHeldObject() != &Shovel
		|| Context.InteractionComponent->GetOwner() != Context.Interactor
		|| Context.InteractionComponent->IsInteractionSuppressed())
	{
		OutFailureReason = LOCTEXT("ShovelContextInvalid", "현재 들고 있는 삽과 사용자가 일치하지 않습니다.");
		return false;
	}
	return UtilityLaborInputGuard::ValidateOwnerInput(Context.Interactor, OutFailureReason);
}


#undef LOCTEXT_NAMESPACE
