#include "UtilityFuelTransaction.h"

#include "Placement/FacilityPlacementComponent.h"
#include "Placement/PlayerFacilityPlacementComponent.h"
#include "Character/FirstPersonCharacter.h"
#include "Computer/PlayerComputerUseComponent.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Utility/BathWaterBoilerFacilityActor.h"
#include "Utility/UtilityFuelIntakeComponent.h"
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

FUtilityFuelResult FUtilityFuelTransaction::Scoop(
	AUtilityShovelActor& Shovel,
	AUtilityFuelSupplyActor& Supply,
	const FHeldEquipmentUseContext& Context)
{
	FText FailureReason;
	if (!IsValid(&Shovel) || !IsValid(&Supply)
		|| !ValidateHeldContext(Shovel, Context, FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget,
			FailureReason.IsEmpty() ? LOCTEXT("InvalidScoopContext", "삽 또는 연료 공급함 상태가 올바르지 않습니다.") : FailureReason);
	}
	if (Shovel.bFuelMutationInProgress || Supply.bFuelMutationInProgress)
	{
		return Fail(EUtilityFuelFailure::TransactionBusy,
			LOCTEXT("ScoopAlreadyInProgress", "연료 이동이 이미 처리 중입니다."));
	}
	if (!Shovel.HasValidAuthoring(FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidAuthoring, FailureReason);
	}
	if (!ValidateFreshHit(Context.InteractionComponent, Context.User, &Supply, Supply.GetSupplyMesh(), FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget, FailureReason);
	}
	FUtilityFuelLoad CandidateLoad;
	if (!Supply.CanScoop(&Shovel, CandidateLoad, FailureReason))
	{
		return Fail(Shovel.IsLoadEmpty()
			? EUtilityFuelFailure::InvalidAuthoring
			: EUtilityFuelFailure::ShovelAlreadyLoaded, FailureReason);
	}

	TGuardValue<bool> ShovelGuard(Shovel.bFuelMutationInProgress, true);
	TGuardValue<bool> SupplyGuard(Supply.bFuelMutationInProgress, true);
	if (!ValidateHeldContext(Shovel, Context, FailureReason)
		|| !ValidateFreshHit(Context.InteractionComponent, Context.User, &Supply, Supply.GetSupplyMesh(), FailureReason)
		|| !Shovel.HasValidAuthoring(FailureReason)
		|| !Supply.HasValidAuthoring(FailureReason)
		|| !Shovel.FuelLoad.IsEmpty()
		|| Supply.FuelKind != EUtilityFuelKind::Coal
		|| !FMath::IsFinite(Supply.ScoopPoints) || Supply.ScoopPoints <= 0.0f)
	{
		return Fail(EUtilityFuelFailure::InvalidTarget,
			FailureReason.IsEmpty() ? LOCTEXT("ScoopRevalidationFailed", "퍼담기 조건이 변경되었습니다.") : FailureReason);
	}

	CandidateLoad.Kind = Supply.FuelKind;
	CandidateLoad.Points = Supply.ScoopPoints;
	if (!CandidateLoad.IsValid() || CandidateLoad.IsEmpty())
	{
		return Fail(EUtilityFuelFailure::InvalidLoad, LOCTEXT("InvalidScoopLoad", "공급할 연료 양이 올바르지 않습니다."));
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
	UUtilityFuelIntakeComponent& Intake,
	const FHeldEquipmentUseContext& Context)
{
	FText FailureReason;
	ABathWaterBoilerFacilityActor* Boiler = Cast<ABathWaterBoilerFacilityActor>(Intake.GetOwner());
	UUtilityOperationComponent* Operation = Boiler ? Boiler->GetUtilityOperation() : nullptr;
	if (!IsValid(&Shovel) || !IsValid(&Intake) || !IsValid(Boiler) || !IsValid(Operation)
		|| !ValidateHeldContext(Shovel, Context, FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget,
			FailureReason.IsEmpty() ? LOCTEXT("InvalidInsertContext", "삽 또는 보일러 투입구 상태가 올바르지 않습니다.") : FailureReason);
	}
	if (Shovel.bFuelMutationInProgress || Operation->bMutationInProgress)
	{
		return Fail(EUtilityFuelFailure::TransactionBusy,
			LOCTEXT("InsertAlreadyInProgress", "연료 이동이 이미 처리 중입니다."));
	}
	if (Boiler->GetFuelIntake() != &Intake
		|| !ValidateFreshHit(Context.InteractionComponent, Context.User, Boiler, &Intake, FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget,
			FailureReason.IsEmpty() ? LOCTEXT("WrongIntake", "보일러의 실제 연료 투입구를 조준하세요.") : FailureReason);
	}
	const UFacilityPlacementComponent* Placement = Boiler->GetFacilityPlacementComponent();
	if (!Placement || Placement->IsStagedPlacement() || !Placement->IsPlacedDomainActive()
		|| !Operation->IsPlacedClockActive())
	{
		return Fail(EUtilityFuelFailure::NotInstalled, LOCTEXT("BoilerNotInstalled", "설치된 보일러에만 연료를 넣을 수 있습니다."));
	}
	if (!Boiler->HasValidUtilityAuthoring(FailureReason)
		|| !Shovel.HasValidAuthoring(FailureReason)
		|| Shovel.FuelLoad.IsEmpty() || !Shovel.FuelLoad.IsValid()
		|| Shovel.FuelLoad.Kind != EUtilityFuelKind::Coal)
	{
		return Fail(EUtilityFuelFailure::InvalidLoad,
			FailureReason.IsEmpty() ? LOCTEXT("ShovelHasNoCoal", "삽에 유효한 석탄이 없습니다.") : FailureReason);
	}
	if (!Operation->CanAcceptFuel(Shovel.FuelLoad, FailureReason))
	{
		const EUtilityFuelFailure Code = Operation->IsLaborBlocked()
			? EUtilityFuelFailure::RecoveryInProgress
			: (Operation->GetRemainingPoints() >= Operation->GetMaximumPoints()
				? EUtilityFuelFailure::CapacityFull : EUtilityFuelFailure::NotInstalled);
		return Fail(Code, FailureReason);
	}
	TGuardValue<bool> ShovelGuard(Shovel.bFuelMutationInProgress, true);
	TGuardValue<bool> OperationGuard(Operation->bMutationInProgress, true);
	if (!ValidateHeldContext(Shovel, Context, FailureReason)
		|| !ValidateFreshHit(Context.InteractionComponent, Context.User, Boiler, &Intake, FailureReason)
		|| !IsValid(Boiler) || Boiler->GetFuelIntake() != &Intake
		|| !Placement->IsPlacedDomainActive() || Placement->IsStagedPlacement()
		|| !Operation->bPlacedClockActive || Operation->bLaborBlocked
		|| !Shovel.HasValidAuthoring(FailureReason)
		|| !Shovel.FuelLoad.IsValid() || Shovel.FuelLoad.IsEmpty()
		|| Shovel.FuelLoad.Kind != EUtilityFuelKind::Coal
		|| !Intake.HasValidAuthoring(FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget,
			FailureReason.IsEmpty() ? LOCTEXT("InsertRevalidationFailed", "연료 투입 조건이 변경되었습니다.") : FailureReason);
	}

	const double GameTimeSeconds = Operation->GetGameTimeSeconds();
	const float CurrentPoints = Operation->GetRemainingPointsAt(GameTimeSeconds);
	const bool bWasProvidingCapacity = Operation->bPlacedClockActive && CurrentPoints > 0.0f;
	if (CurrentPoints >= Operation->MaxOperationPoints)
	{
		return Fail(EUtilityFuelFailure::CapacityFull, LOCTEXT("BoilerFull", "보일러가 가득 차 있습니다."));
	}
	const float CandidatePoints = FMath::Min(
		Operation->MaxOperationPoints,
		CurrentPoints + Shovel.FuelLoad.Points);
	if (!FMath::IsFinite(CandidatePoints) || CandidatePoints <= CurrentPoints)
	{
		return Fail(EUtilityFuelFailure::InvalidLoad, LOCTEXT("InvalidBoilerCandidate", "연료 투입 결과를 계산할 수 없습니다."));
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
	FText FailureReason;
	if (!IsValid(&Shovel) || !IsValid(&Supply)
		|| !ValidateHeldContext(Shovel, Context, FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget,
			FailureReason.IsEmpty() ? LOCTEXT("InvalidReturnContext", "삽 또는 연료 공급함 상태가 올바르지 않습니다.") : FailureReason);
	}
	if (Shovel.bFuelMutationInProgress || Supply.bFuelMutationInProgress)
	{
		return Fail(EUtilityFuelFailure::TransactionBusy,
			LOCTEXT("ReturnAlreadyInProgress", "연료 이동이 이미 처리 중입니다."));
	}
	if (!Shovel.HasValidAuthoring(FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidAuthoring, FailureReason);
	}
	if (!ValidateFreshHit(Context.InteractionComponent, Context.Interactor, &Supply, Supply.GetSupplyMesh(), FailureReason))
	{
		return Fail(EUtilityFuelFailure::InvalidTarget, FailureReason);
	}
	if (!Supply.CanReturn(Shovel.FuelLoad, FailureReason))
	{
		return Fail(Shovel.FuelLoad.IsEmpty()
			? EUtilityFuelFailure::ShovelEmpty : EUtilityFuelFailure::WrongFuel, FailureReason);
	}
	TGuardValue<bool> ShovelGuard(Shovel.bFuelMutationInProgress, true);
	TGuardValue<bool> SupplyGuard(Supply.bFuelMutationInProgress, true);
	if (!ValidateHeldContext(Shovel, Context, FailureReason)
		|| !ValidateFreshHit(Context.InteractionComponent, Context.Interactor, &Supply, Supply.GetSupplyMesh(), FailureReason)
		|| !Shovel.HasValidAuthoring(FailureReason)
		|| !Supply.HasValidAuthoring(FailureReason)
		|| !Shovel.FuelLoad.IsValid() || Shovel.FuelLoad.IsEmpty()
		|| Shovel.FuelLoad.Kind != Supply.FuelKind)
	{
		return Fail(EUtilityFuelFailure::WrongFuel,
			FailureReason.IsEmpty() ? LOCTEXT("ReturnRevalidationFailed", "연료 반환 조건이 변경되었습니다.") : FailureReason);
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
	const FHeldEquipmentUseContext& Context,
	FText& OutFailureReason)
{
	if (!IsValid(Context.User) || !IsValid(Context.CarryComponent)
		|| !IsValid(Context.InteractionComponent) || Context.Equipment != &Shovel
		|| Context.CarryComponent->GetOwner() != Context.User
		|| Context.CarryComponent->GetHeldObject() != &Shovel
		|| Context.InteractionComponent->GetOwner() != Context.User
		|| Context.InteractionComponent->IsInteractionSuppressed())
	{
		OutFailureReason = LOCTEXT("ShovelContextInvalid", "현재 들고 있는 삽과 사용자가 일치하지 않습니다.");
		return false;
	}
	return ValidateOwnerInput(Context.User, OutFailureReason);
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
		OutFailureReason = LOCTEXT("ReturnContextInvalid", "현재 들고 있는 삽과 사용자가 일치하지 않습니다.");
		return false;
	}
	return ValidateOwnerInput(Context.Interactor, OutFailureReason);
}

bool FUtilityFuelTransaction::ValidateOwnerInput(AActor* User, FText& OutFailureReason)
{
	const AFirstPersonCharacter* Character = Cast<AFirstPersonCharacter>(User);
	if (Character)
	{
		if (const UPlayerComputerUseComponent* Computer = Character->GetPlayerComputerUse();
			Computer && Computer->IsCapturingInput())
		{
			OutFailureReason = LOCTEXT("ComputerOwnsInput", "컴퓨터 조작 중에는 삽을 사용할 수 없습니다.");
			return false;
		}
		if (const UPlayerFacilityPlacementComponent* Placement = Character->GetPlayerFacilityPlacement();
			Placement && Placement->IsPlacementActive())
		{
			OutFailureReason = LOCTEXT("PlacementOwnsInput", "설비 배치 중에는 삽을 사용할 수 없습니다.");
			return false;
		}
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
