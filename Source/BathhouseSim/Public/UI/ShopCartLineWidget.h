#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shop/ShopTypes.h"
#include "ShopCartLineWidget.generated.h"

class UButton;
class UShopCartComponent;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnShopCartLineFailure, EShopFailureCode);

UCLASS(Abstract, Blueprintable)
class BATHHOUSESIM_API UShopCartLineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeLine(FName ProductId, const FText& DisplayName, int32 Quantity, int32 LineTotal, UShopCartComponent* InCart);
	FOnShopCartLineFailure& OnLineFailure() { return LineFailure; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> QuantityText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LineTotalText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlusButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MinusButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RemoveButton;

private:
	UFUNCTION()
	void HandlePlusClicked();

	UFUNCTION()
	void HandleMinusClicked();

	UFUNCTION()
	void HandleRemoveClicked();

	void BindButtons();
	void UnbindButtons();

	FName ProductId;
	TWeakObjectPtr<UShopCartComponent> Cart;
	FOnShopCartLineFailure LineFailure;
	bool bButtonsBound = false;
};
