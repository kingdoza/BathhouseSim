#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "Shop/ShopTypes.h"
#include "ShopCatalog.generated.h"

UCLASS(BlueprintType)
class BATHHOUSESIM_API UShopCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	const FShopProductEntry* FindProduct(FName ProductId) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	TArray<FShopProductEntry> Products;
};
