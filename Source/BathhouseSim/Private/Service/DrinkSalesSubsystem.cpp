#include "Service/DrinkSalesSubsystem.h"

#include "Economy/PlayerWalletComponent.h"
#include "GameFramework/Actor.h"

#define LOCTEXT_NAMESPACE "DrinkSalesSubsystem"

void UDrinkSalesSubsystem::AddSale(const int32 Value)
{
	if (Value <= 0)
	{
		return;
	}
	CollectionBoxes.RemoveAll([](const TWeakObjectPtr<const AActor>& Box) { return !Box.IsValid(); });
	if (CollectionBoxes.IsEmpty() && !bWarnedNoCollectionBox)
	{
		bWarnedNoCollectionBox = true;
		UE_LOG(LogTemp, Warning, TEXT("Drink sales accrue but the level has no ADrinkCollectionBoxActor to collect them."));
	}
	const int64 Sum = static_cast<int64>(PendingAmount) + static_cast<int64>(Value);
	if (Sum > MAX_int32)
	{
		UE_LOG(LogTemp, Warning, TEXT("Drink sales pool saturated at MAX_int32."));
	}
	const int32 NewAmount = static_cast<int32>(FMath::Min<int64>(Sum, MAX_int32));
	if (NewAmount != PendingAmount)
	{
		PendingAmount = NewAmount;
		OnPendingAmountChanged.Broadcast(PendingAmount);
	}
}

bool UDrinkSalesSubsystem::TryCollect(
	UPlayerWalletComponent& Wallet,
	int32& OutAmount,
	FText& OutFailureReason)
{
	OutAmount = 0;
	if (bCollecting)
	{
		OutFailureReason = LOCTEXT("CollectBusy", "수금 중입니다.");
		return false;
	}
	if (PendingAmount <= 0)
	{
		OutFailureReason = LOCTEXT("NothingToCollect", "모인 돈 없음");
		return false;
	}
	const int32 Amount = PendingAmount;
	if (!Wallet.CanAddMoney(Amount))
	{
		OutFailureReason = LOCTEXT("WalletOverflow", "지갑에 금액을 추가할 수 없습니다.");
		return false;
	}
	// TryAddMoney broadcasts synchronously; the guard keeps listeners from re-entering collection.
	bCollecting = true;
	const bool bAdded = Wallet.TryAddMoney(Amount);
	bCollecting = false;
	if (!bAdded)
	{
		OutFailureReason = LOCTEXT("CollectFailed", "수금할 수 없습니다.");
		return false;
	}
	PendingAmount = FMath::Max(0, PendingAmount - Amount);
	OutAmount = Amount;
	OnPendingAmountChanged.Broadcast(PendingAmount);
	return true;
}

void UDrinkSalesSubsystem::RegisterCollectionBox(const AActor& Box)
{
	CollectionBoxes.AddUnique(&Box);
}

void UDrinkSalesSubsystem::UnregisterCollectionBox(const AActor& Box)
{
	CollectionBoxes.Remove(&Box);
}

#undef LOCTEXT_NAMESPACE
