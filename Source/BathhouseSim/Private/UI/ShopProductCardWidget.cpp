#include "UI/ShopProductCardWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Shop/ShopCartComponent.h"

void UShopProductCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (AddButton && !bButtonBound)
	{
		AddButton->OnClicked.AddDynamic(this, &UShopProductCardWidget::HandleAddClicked);
		bButtonBound = true;
	}
	InitializeProduct(ProductData, Cart.Get());
}

void UShopProductCardWidget::NativeDestruct()
{
	if (AddButton && bButtonBound)
	{
		AddButton->OnClicked.RemoveDynamic(this, &UShopProductCardWidget::HandleAddClicked);
		bButtonBound = false;
	}
	Cart.Reset();
	Super::NativeDestruct();
}

void UShopProductCardWidget::InitializeProduct(
	const FShopProductEntry& Product,
	UShopCartComponent* InCart)
{
	ProductData = Product;
	Cart = InCart;
	if (NameText)
	{
		NameText->SetText(ProductData.DisplayName);
	}
	if (PriceText)
	{
		PriceText->SetText(FText::Format(
			NSLOCTEXT("ShopProductCard", "ProductPrice", "{0}원"),
			FText::AsNumber(ProductData.Price)));
	}
	if (IconImage)
	{
		if (UTexture2D* Icon = ProductData.Icon.LoadSynchronous())
		{
			IconImage->SetBrushFromTexture(Icon);
			IconImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UShopProductCardWidget::SetAddFailure(const EShopFailureCode Failure)
{
	if (!AddButton) return;
	const bool bInvalidProduct = Failure == EShopFailureCode::NotForSale
		|| Failure == EShopFailureCode::MissingCatalog
		|| Failure == EShopFailureCode::InvalidProduct;
	AddButton->SetIsEnabled(!bInvalidProduct);
	switch (Failure)
	{
	case EShopFailureCode::ProductLimit:
		AddButton->SetToolTipText(NSLOCTEXT("ShopProductCard", "ProductLimit", "상품별 최대 수량에 도달했습니다."));
		break;
	case EShopFailureCode::CartLimit:
		AddButton->SetToolTipText(NSLOCTEXT("ShopProductCard", "CartLimit", "장바구니 총 수량 한도에 도달했습니다."));
		break;
	default:
		AddButton->SetToolTipText(FText::GetEmpty());
		break;
	}
}

void UShopProductCardWidget::HandleAddClicked()
{
	UShopCartComponent* LiveCart = Cart.Get();
	if (!LiveCart)
	{
		AddAttempt.Broadcast(ProductData.ProductId, EShopFailureCode::MissingCatalog);
		return;
	}
	EShopFailureCode Failure = EShopFailureCode::None;
	if (!LiveCart->TryAdd(ProductData.ProductId, Failure))
	{
		AddAttempt.Broadcast(ProductData.ProductId, Failure);
		return;
	}
	AddAttempt.Broadcast(ProductData.ProductId, EShopFailureCode::None);
}
