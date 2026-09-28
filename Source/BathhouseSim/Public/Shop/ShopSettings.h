#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ShopSettings.generated.h"

class AShopDeliveryBoxActor;
class UShopCatalog;

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Bathhouse Shop"))
class BATHHOUSESIM_API UShopSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UShopCatalog* LoadCatalog() const;
	TSubclassOf<AShopDeliveryBoxActor> LoadDeliveryBoxClass() const;
	int32 GetCartTotalQuantityLimit() const { return FMath::Max(1, CartTotalQuantityLimit); }
	int32 GetPerProductQuantityLimit() const { return FMath::Max(1, PerProductQuantityLimit); }
	float GetDeliveryDelaySeconds() const;
	float GetUnboxForwardDistanceCm() const;
	float GetUnboxOverlapDepthCm() const;
	float GetDeliveryNoticeSeconds() const;

	UPROPERTY(Config, EditAnywhere, Category = "Shop")
	TSoftObjectPtr<UShopCatalog> Catalog;

	UPROPERTY(Config, EditAnywhere, Category = "Shop")
	TSoftClassPtr<AShopDeliveryBoxActor> DeliveryBoxClass;

	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "1"))
	int32 CartTotalQuantityLimit = 10;

	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "1"))
	int32 PerProductQuantityLimit = 99;

	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "0.0"))
	float DeliveryDelaySeconds = 10.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "0.0"))
	float UnboxForwardDistanceCm = 100.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float UnboxOverlapDepthCm = 8.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "0.01"))
	float DeliveryNoticeSeconds = 3.0f;
};
