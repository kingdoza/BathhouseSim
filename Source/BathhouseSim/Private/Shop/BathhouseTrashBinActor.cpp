#include "Shop/BathhouseTrashBinActor.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/PhysicalCarryDiscardable.h"
#include "Interaction/PlayerCarryComponent.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "BathhouseTrashBinActor"

ABathhouseTrashBinActor::ABathhouseTrashBinActor()
{
	PrimaryActorTick.bCanEverTick = false;
	BinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BinMesh"));
	SetRootComponent(BinMesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		BinMesh->SetStaticMesh(Cube.Object);
	}
	BinMesh->SetCollisionProfileName(TEXT("BlockAll"));
	BinMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	BinMesh->SetSimulatePhysics(false);
}

FPlayerInteractionQuery ABathhouseTrashBinActor::QueryInteraction(
	const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	Query.bVisible = true;
	Query.TargetName = LOCTEXT("TrashBinName", "쓰레기통");
	if (!Context.CarryComponent || Context.CarryComponent->IsHandEmpty())
	{
		Query.ActionName = LOCTEXT("TrashEmptyAction", "버릴 물건이 없습니다");
		Query.FailureReason = Query.ActionName;
		return Query;
	}

	AActor* HeldObject = Context.CarryComponent->GetHeldObject();
	const IPhysicalCarryDiscardable* Discardable = Cast<IPhysicalCarryDiscardable>(HeldObject);
	FText Failure;
	if (!Discardable || !Discardable->CanDiscardCarriedObject(Failure))
	{
		Query.ActionName = LOCTEXT("TrashCannotDiscard", "버릴 수 없는 물건");
		Query.FailureReason = Query.ActionName;
		return Query;
	}
	Query.bCanInteract = true;
	Query.ActionName = LOCTEXT("TrashDiscardAction", "버리기 (되돌릴 수 없음)");
	return Query;
}

FPlayerInteractionResult ABathhouseTrashBinActor::ExecuteInteraction(
	const FPlayerInteractionContext& Context)
{
	const FPlayerInteractionQuery Query = QueryInteraction(Context);
	if (!Query.bCanInteract || !Context.CarryComponent)
	{
		return FPlayerInteractionResult::Failed(
			Query.FailureReason.IsEmpty() ? LOCTEXT("TrashUnavailable", "이 물건은 버릴 수 없습니다.") : Query.FailureReason);
	}
	AActor* HeldObject = Context.CarryComponent->GetHeldObject();
	IPhysicalCarryDiscardable* Discardable = Cast<IPhysicalCarryDiscardable>(HeldObject);
	if (!Discardable
		|| !Context.CarryComponent->CommitConsumeHeldObject(HeldObject, [Discardable]()
		{
			Discardable->HandleDiscardCommitted();
			return true;
		}))
	{
		return FPlayerInteractionResult::Failed(LOCTEXT("TrashCommitFailed", "물건을 버릴 수 없습니다."));
	}
	return FPlayerInteractionResult::Succeeded();
}

#undef LOCTEXT_NAMESPACE
