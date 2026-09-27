#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shop/ShopTypes.h"
#include "ShopProductCardWidget.generated.h"

class UButton;
class UImage;
class UShopCartComponent;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnShopProductAddAttempt, FName, EShopFailureCode);

UCLASS(Abstract, Blueprintable)
class BATHHOUSESIM_API UShopProductCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeProduct(const FShopProductEntry& Product, UShopCartComponent* InCart);
	void SetAddFailure(EShopFailureCode Failure);
	FName GetProductId() const { return ProductData.ProductId; }
	FOnShopProductAddAttempt& OnAddAttempt() { return AddAttempt; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PriceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AddButton;

private:
	UFUNCTION()
	void HandleAddClicked();

	UPROPERTY(Transient)
	FShopProductEntry ProductData;

	TWeakObjectPtr<UShopCartComponent> Cart;
	FOnShopProductAddAttempt AddAttempt;
	bool bButtonBound = false;
};
