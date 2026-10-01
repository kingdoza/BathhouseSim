#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Computer/ComputerScreenContext.h"
#include "Shop/ShopTypes.h"
#include "ShopScreenWidget.generated.h"

class ABathhousePlayerState;
class UButton;
class UPlayerWalletComponent;
class UShopCartComponent;
class UShopCatalog;
class UShopCartLineWidget;
class UShopOrderLineWidget;
class UShopProductCardWidget;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWrapBox;

UCLASS(Abstract, Blueprintable)
class BATHHOUSESIM_API UShopScreenWidget
	: public UUserWidget
	, public IComputerScreenContextReceiver
{
	GENERATED_BODY()

public:
	virtual void InitializeComputerScreen(const FComputerScreenContext& Context) override;
	virtual void NotifyComputerUserChanged(APlayerState* PlayerState) override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ProductScroll;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> ProductGrid;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BalanceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> CartList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CartQuantityText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CartTotalText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> OrderButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> OrderFeedbackText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> OrderList;

	UPROPERTY(EditDefaultsOnly, Category = "Shop|Widgets")
	TSubclassOf<UShopProductCardWidget> ProductCardWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Shop|Widgets")
	TSubclassOf<UShopCartLineWidget> CartLineWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Shop|Widgets")
	TSubclassOf<UShopOrderLineWidget> OrderLineWidgetClass;

	/** 주문 목록 남은 시간 표시 갱신 간격 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|Display", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float CountdownRefreshIntervalSeconds = 1.0f;

private:
	UFUNCTION()
	void HandleMoneyChanged(int32 PreviousMoney, int32 CurrentMoney);

	UFUNCTION()
	void HandleCartChanged();

	UFUNCTION()
	void HandleOrdersChanged();


	UFUNCTION()
	void HandleOrderClicked();

	void RebindDomain();
	void UnbindDomain();
	void RefreshBalance();
	void RefreshCart(bool bRecreateRows);
	void RefreshProducts(bool bRecreateRows);
	void RefreshOrders(bool bRecreateRows);
	void RefreshOrderCountdowns();
	void HandleProductAddAttempt(FName ProductId, EShopFailureCode Failure);
	void HandleCartLineFailure(EShopFailureCode Failure);
	void ShowFailure(EShopFailureCode Failure);
	void UpdateOrderButton();

	FComputerScreenContext ScreenContext;
	TWeakObjectPtr<ABathhousePlayerState> CurrentPlayerState;
	TWeakObjectPtr<UPlayerWalletComponent> Wallet;
	TWeakObjectPtr<UShopCartComponent> Cart;
	UPROPERTY(Transient)
	TObjectPtr<UShopCatalog> Catalog = nullptr;

	TWeakObjectPtr<class UShopOrderSubsystem> Orders;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UShopProductCardWidget>> ProductCards;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UShopCartLineWidget>> CartRows;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UShopOrderLineWidget>> OrderRows;
	float CountdownRefreshElapsed = 0.0f;
	EShopFailureCode LastDisplayedFailure = EShopFailureCode::None;
	bool bOrderButtonBound = false;
};
