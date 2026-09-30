#include "Service/DrinkCollectionBoxActor.h"

#include "Components/StaticMeshComponent.h"
#include "Economy/BathhousePlayerState.h"
#include "Economy/PlayerWalletComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Service/DrinkSalesSubsystem.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "DrinkCollectionBoxActor"

ADrinkCollectionBoxActor::ADrinkCollectionBoxActor()
{
	PrimaryActorTick.bCanEverTick = false;
	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	SetRootComponent(BoxMesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		BoxMesh->SetStaticMesh(Cube.Object);
	}
	BoxMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void ADrinkCollectionBoxActor::BeginPlay()
{
	Super::BeginPlay();
	if (UDrinkSalesSubsystem* Sales = GetWorld() ? GetWorld()->GetSubsystem<UDrinkSalesSubsystem>() : nullptr)
	{
		Sales->RegisterCollectionBox(*this);
	}
}

void ADrinkCollectionBoxActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UDrinkSalesSubsystem* Sales = GetWorld() ? GetWorld()->GetSubsystem<UDrinkSalesSubsystem>() : nullptr)
	{
		Sales->UnregisterCollectionBox(*this);
	}
	Super::EndPlay(EndPlayReason);
}

FPlayerInteractionQuery ADrinkCollectionBoxActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	const UDrinkSalesSubsystem* Sales = GetWorld() ? GetWorld()->GetSubsystem<UDrinkSalesSubsystem>() : nullptr;
	if (!Sales)
	{
		return Query;
	}
	const int32 Pending = Sales->GetPendingAmount();
	Query.bVisible = true;
	Query.TargetName = FText::Format(LOCTEXT("BoxName", "음료 수거함 {0}원"), FText::AsNumber(Pending));
	Query.ActionName = LOCTEXT("CollectAction", "수금");
	const UPlayerWalletComponent* Wallet = ResolveWallet(Context);
	if (Pending <= 0)
	{
		Query.FailureReason = LOCTEXT("NothingToCollect", "모인 돈 없음");
	}
	else if (!Wallet)
	{
		Query.FailureReason = LOCTEXT("MissingWallet", "플레이어 지갑을 찾을 수 없습니다.");
	}
	else if (!Wallet->CanAddMoney(Pending))
	{
		Query.FailureReason = LOCTEXT("WalletOverflow", "지갑에 금액을 추가할 수 없습니다.");
	}
	else
	{
		Query.bCanInteract = true;
	}
	return Query;
}

FPlayerInteractionResult ADrinkCollectionBoxActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	if (bCollecting)
	{
		return FPlayerInteractionResult::Failed(LOCTEXT("CollectBusy", "수금 중입니다."));
	}
	const FPlayerInteractionQuery Query = QueryInteraction(Context);
	UPlayerWalletComponent* Wallet = ResolveWallet(Context);
	UDrinkSalesSubsystem* Sales = GetWorld() ? GetWorld()->GetSubsystem<UDrinkSalesSubsystem>() : nullptr;
	if (!Query.bCanInteract || !Wallet || !Sales)
	{
		return FPlayerInteractionResult::Failed(Query.FailureReason);
	}
	bCollecting = true;
	int32 Amount = 0;
	FText Failure;
	const bool bCollected = Sales->TryCollect(*Wallet, Amount, Failure);
	bCollecting = false;
	return bCollected
		? FPlayerInteractionResult::Succeeded()
		: FPlayerInteractionResult::Failed(Failure);
}

UPlayerWalletComponent* ADrinkCollectionBoxActor::ResolveWallet(const FPlayerInteractionContext& Context) const
{
	const APawn* Pawn = Cast<APawn>(Context.Interactor);
	if (!Pawn)
	{
		if (const AController* Controller = Cast<AController>(Context.Interactor))
		{
			Pawn = Controller->GetPawn();
		}
	}
	const ABathhousePlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<ABathhousePlayerState>() : nullptr;
	return PlayerState ? PlayerState->GetWallet() : nullptr;
}

#undef LOCTEXT_NAMESPACE
