#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shop/ShopTypes.h"
#include "ShopOrderLineWidget.generated.h"

class UTextBlock;

UCLASS(Abstract, Blueprintable)
class BATHHOUSESIM_API UShopOrderLineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeOrder(const FShopOrderSnapshot& Snapshot);
	void UpdateStatus(float SecondsRemaining, bool bWaitingForSpace);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SummaryText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

private:
	int64 OrderId = 0;
};
