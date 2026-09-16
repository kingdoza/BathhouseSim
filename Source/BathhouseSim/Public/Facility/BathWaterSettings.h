#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BathWaterSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Bath Water"))
class BATHHOUSESIM_API UBathWaterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }
	virtual FName GetSectionName() const override { return TEXT("Bath Water"); }

	UFUNCTION(BlueprintPure, Category = "Bath Water")
	float GetCustomerUsableThresholdPercent() const;

	UFUNCTION(BlueprintPure, Category = "Bath Water")
	float GetCustomerUsableThresholdNormalized() const;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Customer Use",
		meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float CustomerUsableThresholdPercent = 80.0f;
};
