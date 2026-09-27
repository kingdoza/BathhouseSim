#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "ShopNoticeWidget.generated.h"

class UShopOrderSubsystem;
class UTextBlock;

UCLASS(Abstract, Blueprintable)
class BATHHOUSESIM_API UShopNoticeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetOrderSubsystem(UShopOrderSubsystem* InOrders);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NoticeText;

private:
	UFUNCTION()
	void HandleOrderDelivered(int64 OrderId);

	void BindOrders();
	void UnbindOrders();
	void ClearNotice();

	TWeakObjectPtr<UShopOrderSubsystem> Orders;
	FTimerHandle NoticeTimerHandle;
	bool bIsConstructed = false;
};
