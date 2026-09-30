#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DrinkSalesSubsystem.generated.h"

class UPlayerWalletComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDrinkSalesPendingChanged, int32, PendingAmount);

/** World-wide pool of uncollected drink sales. Fridges add; the collection box moves it into a wallet. */
UCLASS()
class BATHHOUSESIM_API UDrinkSalesSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	int32 GetPendingAmount() const { return PendingAmount; }

	/** Non-positive values are ignored. The pool saturates at MAX_int32 with a warning. */
	void AddSale(int32 Value);

	/** Moves the whole pool into the wallet, or leaves it untouched on any failure. */
	bool TryCollect(UPlayerWalletComponent& Wallet, int32& OutAmount, FText& OutFailureReason);

	void RegisterCollectionBox(const AActor& Box);
	void UnregisterCollectionBox(const AActor& Box);

	UPROPERTY(BlueprintAssignable, Category = "Drink Sales")
	FOnDrinkSalesPendingChanged OnPendingAmountChanged;

private:
	int32 PendingAmount = 0;
	bool bCollecting = false;
	bool bWarnedNoCollectionBox = false;
	TArray<TWeakObjectPtr<const AActor>> CollectionBoxes;
};
