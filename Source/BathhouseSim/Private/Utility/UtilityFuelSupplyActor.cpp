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
	Query.TargetName = LOCTEXT("SupplyName", "석탄 공급함");
	Query.ActionName = LOCTEXT("SupplyPrimaryName", "삽으로 석탄 퍼담기");
	Query.FailureReason = LOCTEXT("UseShovel", "삽을 들고 석탄 공급함을 향해 사용하세요.");
	Query.bSecondaryVisible = true;
	Query.SecondaryActionName = LOCTEXT("ReturnFuel", "삽의 연료 반환");

	AUtilityShovelActor* Shovel = Context.CarryComponent
		? Cast<AUtilityShovelActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	if (!IsValid(Shovel))
	{
		Query.SecondaryFailureReason = LOCTEXT("ReturnNeedsShovel", "연료가 든 삽을 들고 있어야 합니다.");
		return Query;
	}
	FText FailureReason;
	Query.bCanSecondaryInteract = CanReturn(Shovel->GetFuelLoad(), FailureReason);
	Query.SecondaryFailureReason = FailureReason;
	return Query;
}

FPlayerInteractionResult AUtilityFuelSupplyActor::ExecuteInteraction(
	const FPlayerInteractionContext& Context)
{
	(void)Context;
	return FPlayerInteractionResult::Failed(
		LOCTEXT("SupplyUseHeldEquipment", "삽을 들고 석탄 공급함을 향해 사용하세요."));
}

FPlayerInteractionResult AUtilityFuelSupplyActor::ExecuteSecondaryInteraction(
	const FPlayerInteractionContext& Context)
{
	AUtilityShovelActor* Shovel = Context.CarryComponent
		? Cast<AUtilityShovelActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	if (!IsValid(Shovel))
	{
		return FPlayerInteractionResult::Failed(
			LOCTEXT("ReturnNeedsShovel", "연료가 든 삽을 들고 있어야 합니다."),
			EPlayerInteractionIntent::Secondary);
	}
	const FUtilityFuelResult Result = FUtilityFuelTransaction::Return(*Shovel, *this, Context);
	return Result.bSucceeded
		? FPlayerInteractionResult::Succeeded(EPlayerInteractionIntent::Secondary)
		: FPlayerInteractionResult::Failed(Result.FailureReason, EPlayerInteractionIntent::Secondary);
}

bool AUtilityFuelSupplyActor::HasValidAuthoring(FText& OutFailureReason) const
{
	if (FuelKind != EUtilityFuelKind::Coal || !FMath::IsFinite(ScoopPoints) || ScoopPoints <= 0.0f
		|| !SupplyMesh || !SupplyMesh->GetStaticMesh()
		|| SupplyMesh->GetCollisionEnabled() != ECollisionEnabled::QueryOnly
		|| SupplyMesh->GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block
		|| SupplyMesh->CanEverAffectNavigation())
	{
		OutFailureReason = LOCTEXT(
			"InvalidSupplyAuthoring",
			"석탄 공급함에는 Coal/양수 설정과 메시 기반 QueryOnly Visibility Block target이 필요합니다.");
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
