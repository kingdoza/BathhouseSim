#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Shop/ShopTypes.h"
#include "ShopCartComponent.generated.h"

class UShopCatalog;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopCartChanged);

UCLASS(ClassGroup = (Shop), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UShopCartComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShopCartComponent();

	EShopFailureCode EvaluateAdd(FName ProductId) const;
	bool TryAdd(FName ProductId, EShopFailureCode& OutFailure);
	bool TryIncrement(FName ProductId, EShopFailureCode& OutFailure);
	bool TryDecrement(FName ProductId);
	bool TryRemoveLine(FName ProductId);
	void Clear();
	int32 GetTotalQuantity() const;
	bool CalculateTotalPrice(const UShopCatalog& Catalog, int32& OutTotal, EShopFailureCode& OutFailure) const;

	const TArray<FShopCartLine>& GetLines() const { return Lines; }
	const FShopCartLine* FindLine(FName ProductId) const;

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnShopCartChanged OnCartChanged;

private:
	bool TryIncrementInternal(FName ProductId, EShopFailureCode& OutFailure);
	void BroadcastChanged();

	UPROPERTY(Transient)
	TArray<FShopCartLine> Lines;
};
