#include "UI/ShopCartLineWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Shop/ShopCartComponent.h"

void UShopCartLineWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindButtons();
}

void UShopCartLineWidget::NativeDestruct()
{
	UnbindButtons();
	Cart.Reset();
	Super::NativeDestruct();
}

void UShopCartLineWidget::InitializeLine(
	const FName InProductId,
	const FText& DisplayName,
	const int32 Quantity,
	const int32 LineTotal,
	UShopCartComponent* InCart)
{
	ProductId = InProductId;
	Cart = InCart;
	if (NameText) NameText->SetText(DisplayName);
	if (QuantityText) QuantityText->SetText(FText::AsNumber(Quantity));
	if (LineTotalText)
	{
		LineTotalText->SetText(FText::Format(
			NSLOCTEXT("ShopCartLine", "LineTotal", "{0}원"),
			FText::AsNumber(LineTotal)));
	}
	BindButtons();
}

void UShopCartLineWidget::BindButtons()
{
	if (bButtonsBound) return;
	if (PlusButton) PlusButton->OnClicked.AddDynamic(this, &UShopCartLineWidget::HandlePlusClicked);
	if (MinusButton) MinusButton->OnClicked.AddDynamic(this, &UShopCartLineWidget::HandleMinusClicked);
	if (RemoveButton) RemoveButton->OnClicked.AddDynamic(this, &UShopCartLineWidget::HandleRemoveClicked);
	bButtonsBound = true;
}

void UShopCartLineWidget::UnbindButtons()
{
	if (!bButtonsBound) return;
	if (PlusButton) PlusButton->OnClicked.RemoveDynamic(this, &UShopCartLineWidget::HandlePlusClicked);
	if (MinusButton) MinusButton->OnClicked.RemoveDynamic(this, &UShopCartLineWidget::HandleMinusClicked);
	if (RemoveButton) RemoveButton->OnClicked.RemoveDynamic(this, &UShopCartLineWidget::HandleRemoveClicked);
	bButtonsBound = false;
}

void UShopCartLineWidget::HandlePlusClicked()
{
	UShopCartComponent* LiveCart = Cart.Get();
	if (!LiveCart)
	{
		LineFailure.Broadcast(EShopFailureCode::MissingCatalog);
		return;
	}
	EShopFailureCode Failure = EShopFailureCode::None;
	if (!LiveCart->TryIncrement(ProductId, Failure))
	{
		LineFailure.Broadcast(Failure);
		return;
	}
	LineFailure.Broadcast(EShopFailureCode::None);
}

void UShopCartLineWidget::HandleMinusClicked()
{
	if (UShopCartComponent* LiveCart = Cart.Get())
	{
		LiveCart->TryDecrement(ProductId);
	}
}

void UShopCartLineWidget::HandleRemoveClicked()
{
	if (UShopCartComponent* LiveCart = Cart.Get())
	{
		LiveCart->TryRemoveLine(ProductId);
	}
}
