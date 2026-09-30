#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "ServiceItemDefinition.generated.h"

class UStaticMesh;

UCLASS(BlueprintType)
class BATHHOUSESIM_API UServiceItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Mesh used for display spaces: DisplayMesh when authored, otherwise ItemMesh. */
	UStaticMesh* ResolveDisplayMesh() const;

	bool ValidateRuntime(FText& OutFailureReason) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service Item")
	FName ItemId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service Item")
	TObjectPtr<UStaticMesh> ItemMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service Item|Box", meta = (ClampMin = "1"))
	int32 BoxCapacity = 1;

	/** Box-root-relative cm transforms. Count must equal BoxCapacity; array order is the fill order. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service Item|Box")
	TArray<FTransform> BoxSlotTransforms;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service Item|Display")
	TObjectPtr<UStaticMesh> DisplayMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service Item|Display")
	FTransform DisplayOffset = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service Item|Display")
	FGameplayTagContainer DisplayCategories;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Service Item|Sale", meta = (ClampMin = "0"))
	int32 SaleValue = 0;
};
