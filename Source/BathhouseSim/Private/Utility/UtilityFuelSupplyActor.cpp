#include "Utility/UtilityFuelSupplyActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Utility/UtilityShovelActor.h"
#include "UtilityFuelTransaction.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "UtilityFuelSupplyActor"

AUtilityFuelSupplyActor::AUtilityFuelSupplyActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	SupplyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SupplyMesh"));
	SupplyMesh->SetupAttachment(SceneRoot);
	SupplyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SupplyMesh->SetCollisionObjectType(ECC_WorldDynamic);
	SupplyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	SupplyMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SupplyMesh->SetCanEverAffectNavigation(false);
}

FPlayerInteractionQuery AUtilityFuelSupplyActor::QueryInteraction(
	const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	Query.bVisible = true;
	const FText FuelName = GetUtilityFuelKindDisplayName(FuelKind);
	Query.TargetName = FText::Format(LOCTEXT("SupplyName", "{0} 공급함"), FuelName);
	Query.ActionName = FText::GetEmpty();
	Query.bCanInteract = false;
	Query.FailureReason = FText::GetEmpty();
	Query.bHeldApplyVisible = true;
	Query.HeldApplyActionName = FText::Format(LOCTEXT("SupplyPrimaryName", "{0} 퍼담기"), FuelName);
	Query.HeldApplyActivationMode = EPlayerInteractionActivationMode::Instant;

	AUtilityShovelActor* Shovel = Context.CarryComponent
		? Cast<AUtilityShovelActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	if (!IsValid(Shovel))
	{
		Query.HeldApplyFailureReason = LOCTEXT("UseShovel", "삽을 들고 있어야 합니다.");
		return Query;
	}

	const FUtilityFuelLoad Load = Shovel->GetFuelLoad();
	const FUtilityFuelResult Evaluation = Load.IsEmpty()
		? FUtilityFuelTransaction::EvaluateScoop(Context, *this)
		: FUtilityFuelTransaction::EvaluateReturn(Context, *this);
	if (!Load.IsEmpty() && Load.Kind == FuelKind)
	{
		Query.HeldApplyActionName = FText::Format(LOCTEXT("ReturnFuel", "{0} 반환"), FuelName);
	}
	Query.bCanHeldApply = Evaluation.bSucceeded;
	Query.HeldApplyFailureReason = Evaluation.FailureReason;
	return Query;
}

FPlayerInteractionResult AUtilityFuelSupplyActor::ExecuteInteraction(
	const FPlayerInteractionContext& Context)
{
	return FPlayerInteractionResult::Failed(FText::GetEmpty(), EPlayerInteractionIntent::Primary);
}

FPlayerInteractionResult AUtilityFuelSupplyActor::ExecuteHeldTargetUse(
	const FPlayerInteractionContext& Context,
	const EPlayerHeldTargetUseDirection Direction)
{
	const EPlayerInteractionIntent Intent = Direction == EPlayerHeldTargetUseDirection::Apply
		? EPlayerInteractionIntent::HeldApply
		: EPlayerInteractionIntent::HeldTake;
	if (Direction != EPlayerHeldTargetUseDirection::Apply)
	{
		return FPlayerInteractionResult::Failed(FText::GetEmpty(), Intent);
	}
	AUtilityShovelActor* Shovel = Context.CarryComponent
		? Cast<AUtilityShovelActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	if (!IsValid(Shovel))
	{
		return FPlayerInteractionResult::Failed(
			LOCTEXT("ReturnNeedsShovel", "삽을 들고 있어야 합니다."),
			Intent);
	}
	const FUtilityFuelLoad Load = Shovel->GetFuelLoad();
	if (Load.IsEmpty())
	{
		const FUtilityFuelResult Result = FUtilityFuelTransaction::Scoop(*Shovel, *this, Context);
		return Result.bSucceeded
			? FPlayerInteractionResult::Succeeded(Intent)
			: FPlayerInteractionResult::Failed(Result.FailureReason, Intent);
	}
	if (Load.Kind == FuelKind)
	{
		const FUtilityFuelResult Result = FUtilityFuelTransaction::Return(*Shovel, *this, Context);
		return Result.bSucceeded
			? FPlayerInteractionResult::Succeeded(Intent)
			: FPlayerInteractionResult::Failed(Result.FailureReason, Intent);
	}
	const FUtilityFuelResult Evaluation = FUtilityFuelTransaction::EvaluateReturn(Context, *this);
	return FPlayerInteractionResult::Failed(Evaluation.FailureReason, Intent);
}

bool AUtilityFuelSupplyActor::HasValidAuthoring(FText& OutFailureReason) const
{
	if (!IsSupportedUtilityFuelKind(FuelKind) || !FMath::IsFinite(ScoopPoints) || ScoopPoints <= 0.0f
		|| !SupplyMesh || !SupplyMesh->GetStaticMesh()
		|| SupplyMesh->GetCollisionEnabled() != ECollisionEnabled::QueryOnly
		|| SupplyMesh->GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block
		|| SupplyMesh->CanEverAffectNavigation())
	{
		OutFailureReason = LOCTEXT(
			"InvalidSupplyAuthoring",
			"연료 공급함에는 지원 연료 종류, 양수 설정과 메시 기반 QueryOnly Visibility Block target이 필요합니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool AUtilityFuelSupplyActor::CanScoop(
	const AActor* ShovelActor,
	FUtilityFuelLoad& OutLoad,
	FText& OutFailureReason) const
{
	OutLoad = FUtilityFuelLoad();
	if (bFuelMutationInProgress)
	{
		OutFailureReason = LOCTEXT("SupplyBusy", "연료 이동이 이미 처리 중입니다.");
		return false;
	}
	if (!HasValidAuthoring(OutFailureReason))
	{
		return false;
	}
	const AUtilityShovelActor* Shovel = Cast<AUtilityShovelActor>(ShovelActor);
	if (!IsValid(Shovel))
	{
		OutFailureReason = LOCTEXT("NoShovel", "삽을 들고 있어야 합니다.");
		return false;
	}
	OutLoad.Kind = FuelKind;
	OutLoad.Points = ScoopPoints;
	return Shovel->CanAcceptLoad(OutLoad, OutFailureReason);
}

bool AUtilityFuelSupplyActor::CanReturn(
	const FUtilityFuelLoad& Load,
	FText& OutFailureReason) const
{
	if (bFuelMutationInProgress)
	{
		OutFailureReason = LOCTEXT("SupplyBusy", "연료 이동이 이미 처리 중입니다.");
		return false;
	}
	if (!HasValidAuthoring(OutFailureReason))
	{
		return false;
	}
	if (!Load.IsValid() || Load.IsEmpty() || Load.Kind != FuelKind)
	{
		OutFailureReason = LOCTEXT("WrongFuelReturn", "같은 종류의 연료가 든 삽만 반환할 수 있습니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool AUtilityFuelSupplyActor::TryAcquireMutationGuard()
{
	if (bFuelMutationInProgress)
	{
		return false;
	}
	bFuelMutationInProgress = true;
	return true;
}

void AUtilityFuelSupplyActor::ReleaseMutationGuard()
{
	bFuelMutationInProgress = false;
}

#if WITH_EDITOR
EDataValidationResult AUtilityFuelSupplyActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FText FailureReason;
	if (!HasValidAuthoring(FailureReason))
	{
		Context.AddError(FailureReason);
		Result = EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

#undef LOCTEXT_NAMESPACE
