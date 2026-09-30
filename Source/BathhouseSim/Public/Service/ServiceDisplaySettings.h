#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ServiceDisplaySettings.generated.h"

class UMaterialInterface;

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Bathhouse Service Display"))
class BATHHOUSESIM_API UServiceDisplaySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	bool ShouldShowTakeHighlight() const { return bShowTakeHighlight; }
	int32 GetTakeHighlightStencilValue() const { return FMath::Clamp(TakeHighlightStencilValue, 1, 255); }
	UMaterialInterface* LoadInsertPreviewMaterial() const;

	UPROPERTY(Config, EditAnywhere, Category = "Display")
	bool bShowTakeHighlight = true;

	UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "1", ClampMax = "255"))
	int32 TakeHighlightStencilValue = 1;

	UPROPERTY(Config, EditAnywhere, Category = "Display")
	TSoftObjectPtr<UMaterialInterface> InsertPreviewMaterial;
};
