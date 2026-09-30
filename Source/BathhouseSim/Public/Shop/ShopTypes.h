#pragma once

#include "CoreMinimal.h"
#include "ShopTypes.generated.h"

class UFacilityPlacementDefinition;
class UServiceItemDefinition;
class UTexture2D;

UENUM(BlueprintType)
enum class EShopFailureCode : uint8
{
	None,
	NotForSale,
	ProductLimit,
	CartLimit,
	EmptyCart,
	InvalidProduct,
	InsufficientMoney,
	Busy,
	MissingCatalog,
	InvalidContents,
	DeliveryUnavailable
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FShopProductEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FName ProductId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	bool bForSale = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "1"))
	int32 Price = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UFacilityPlacementDefinition> PlacementDefinition = nullptr;

	/** Exactly one of PlacementDefinition and ItemBoxDefinition is set. A box product delivers one full item box per unit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UServiceItemDefinition> ItemBoxDefinition = nullptr;
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FShopCartLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	FName ProductId;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	int32 Quantity = 0;
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FShopOrderLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	FName ProductId;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UFacilityPlacementDefinition> PlacementDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UServiceItemDefinition> ItemBoxDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	int32 Quantity = 0;
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FShopPlaceOrderEvaluation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	bool bCanOrder = false;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	EShopFailureCode Failure = EShopFailureCode::None;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	int32 TotalPrice = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	int32 ShortfallAmount = 0;
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FShopOrderSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	int64 OrderId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	TArray<FShopOrderLine> Lines;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	float SecondsRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	bool bWaitingForSpace = false;
};
