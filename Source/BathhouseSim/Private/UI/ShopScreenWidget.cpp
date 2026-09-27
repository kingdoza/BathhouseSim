#include "UI/ShopScreenWidget.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Economy/BathhousePlayerState.h"
#include "Economy/PlayerWalletComponent.h"
#include "Engine/World.h"
#include "Shop/ShopCartComponent.h"
#include "Shop/ShopCatalog.h"
#include "Shop/ShopOrderSubsystem.h"
#include "Shop/ShopSettings.h"
#include "UI/ShopCartLineWidget.h"
#include "UI/ShopOrderLineWidget.h"
#include "UI/ShopProductCardWidget.h"

#define LOCTEXT_NAMESPACE "ShopScreenWidget"

void UShopScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (OrderButton && !bOrderButtonBound)
	{
		OrderButton->OnClicked.AddDynamic(this, &UShopScreenWidget::HandleOrderClicked);
		bOrderButtonBound = true;
	}
	RebindDomain();
}

void UShopScreenWidget::NativeDestruct()
{
	UnbindDomain();
	if (OrderButton && bOrderButtonBound)
	{
		OrderButton->OnClicked.RemoveDynamic(this, &UShopScreenWidget::HandleOrderClicked);
		bOrderButtonBound = false;
	}
	ProductCards.Reset();
	CartRows.Reset();
	OrderRows.Reset();
	Super::NativeDestruct();
}

void UShopScreenWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	CountdownRefreshElapsed += InDeltaTime;
	if (CountdownRefreshElapsed >= 1.0f)
	{
		CountdownRefreshElapsed = FMath::Fmod(CountdownRefreshElapsed, 1.0f);
		RefreshOrderCountdowns();
	}
}

void UShopScreenWidget::InitializeComputerScreen(const FComputerScreenContext& Context)
{
	ScreenContext = Context;
}

void UShopScreenWidget::NotifyComputerUserChanged(APlayerState* PlayerState)
{
	CurrentPlayerState = Cast<ABathhousePlayerState>(PlayerState);
	RebindDomain();
}

void UShopScreenWidget::RebindDomain()
{
	UnbindDomain();
	ABathhousePlayerState* PlayerState = CurrentPlayerState.Get();
	Wallet = PlayerState ? PlayerState->GetWallet() : nullptr;
	Cart = PlayerState ? PlayerState->GetShopCart() : nullptr;
	UWorld* World = GetWorld();
	Orders = World ? World->GetSubsystem<UShopOrderSubsystem>() : nullptr;
	const UShopSettings* Settings = GetDefault<UShopSettings>();
	Catalog = Settings ? Settings->LoadCatalog() : nullptr;

	if (Wallet.IsValid())
	{
		Wallet->OnMoneyChanged.AddDynamic(this, &UShopScreenWidget::HandleMoneyChanged);
	}
	if (Cart.IsValid())
	{
		Cart->OnCartChanged.AddDynamic(this, &UShopScreenWidget::HandleCartChanged);
	}
	if (Orders.IsValid())
	{
		Orders->OnOrdersChanged.AddDynamic(this, &UShopScreenWidget::HandleOrdersChanged);
	}
	RefreshBalance();
	RefreshProducts(true);
	RefreshCart(true);
	RefreshOrders(true);
	UpdateOrderButton();
}

void UShopScreenWidget::UnbindDomain()
{
	if (Wallet.IsValid())
	{
		Wallet->OnMoneyChanged.RemoveDynamic(this, &UShopScreenWidget::HandleMoneyChanged);
	}
	if (Cart.IsValid())
	{
		Cart->OnCartChanged.RemoveDynamic(this, &UShopScreenWidget::HandleCartChanged);
	}
	if (Orders.IsValid())
	{
		Orders->OnOrdersChanged.RemoveDynamic(this, &UShopScreenWidget::HandleOrdersChanged);
	}
	Wallet.Reset();
	Cart.Reset();
	Orders.Reset();
}

void UShopScreenWidget::HandleMoneyChanged(const int32 PreviousMoney, const int32 CurrentMoney)
{
	(void)PreviousMoney;
	(void)CurrentMoney;
	RefreshBalance();
	UpdateOrderButton();
}

void UShopScreenWidget::HandleCartChanged()
{
	RefreshCart(true);
	RefreshProducts(false);
	UpdateOrderButton();
}

void UShopScreenWidget::HandleOrdersChanged()
{
	RefreshOrders(true);
	UpdateOrderButton();
}


void UShopScreenWidget::HandleOrderClicked()
{
	UShopOrderSubsystem* LiveOrders = Orders.Get();
	ABathhousePlayerState* PlayerState = CurrentPlayerState.Get();
	if (!LiveOrders || !PlayerState)
	{
		ShowFailure(EShopFailureCode::MissingCatalog);
		return;
	}
	EShopFailureCode Failure = EShopFailureCode::None;
	if (LiveOrders->TryPlaceOrder(PlayerState, Failure))
	{
		LastDisplayedFailure = EShopFailureCode::None;
		if (OrderFeedbackText)
		{
			OrderFeedbackText->SetText(LOCTEXT("OrderComplete", "주문 완료"));

		}
		UpdateOrderButton();
		return;
	}
	ShowFailure(Failure);
	UpdateOrderButton();
}

void UShopScreenWidget::HandleProductAddAttempt(const FName ProductId, const EShopFailureCode Failure)
{
	(void)ProductId;
	if (Failure == EShopFailureCode::None)
	{
		if (OrderFeedbackText) OrderFeedbackText->SetText(FText::GetEmpty());
		LastDisplayedFailure = EShopFailureCode::None;
		UpdateOrderButton();
		return;
	}
	ShowFailure(Failure);
}

void UShopScreenWidget::HandleCartLineFailure(const EShopFailureCode Failure)
{
	ShowFailure(Failure);
}

void UShopScreenWidget::RefreshBalance()
{
	if (!BalanceText) return;
	const UPlayerWalletComponent* LiveWallet = Wallet.Get();
	const int32 Balance = LiveWallet ? LiveWallet->GetCurrentMoney() : 0;
	BalanceText->SetText(FText::Format(
		LOCTEXT("BalanceFormat", "잔액 {0}원"),
		FText::AsNumber(Balance)));
}

void UShopScreenWidget::RefreshCart(const bool bRecreateRows)
{
	UShopCartComponent* LiveCart = Cart.Get();
	UShopCatalog* LiveCatalog = Catalog.Get();
	if (CartQuantityText)
	{
		const UShopSettings* Settings = GetDefault<UShopSettings>();
		CartQuantityText->SetText(FText::Format(
			LOCTEXT("CartQuantityFormat", "{0}/{1}"),
			FText::AsNumber(LiveCart ? LiveCart->GetTotalQuantity() : 0),
			FText::AsNumber(Settings ? Settings->GetCartTotalQuantityLimit() : 10)));
	}
	int32 Total = 0;
	EShopFailureCode PriceFailure = EShopFailureCode::None;
	const bool bHasTotal = LiveCart && LiveCatalog
		&& LiveCart->CalculateTotalPrice(*LiveCatalog, Total, PriceFailure);
	if (CartTotalText)
	{
		CartTotalText->SetText(bHasTotal
			? FText::Format(LOCTEXT("CartTotalFormat", "합계 {0}원"), FText::AsNumber(Total))
			: LOCTEXT("CartTotalUnavailable", "합계 —"));
	}
	if (!bRecreateRows || !CartList) return;

	CartList->ClearChildren();
	CartRows.Reset();
	if (!LiveCart || !LiveCatalog || !CartLineWidgetClass) return;
	for (const FShopCartLine& Line : LiveCart->GetLines())
	{
		const FShopProductEntry* Product = LiveCatalog->FindProduct(Line.ProductId);
		const FText Name = Product ? Product->DisplayName : FText::FromName(Line.ProductId);
		const int32 UnitPrice = Product ? FMath::Max(0, Product->Price) : 0;
		const int64 LinePrice64 = static_cast<int64>(UnitPrice) * static_cast<int64>(Line.Quantity);
		const int32 LinePrice = static_cast<int32>(FMath::Clamp<int64>(LinePrice64, 0, MAX_int32));
		UShopCartLineWidget* Row = CreateWidget<UShopCartLineWidget>(GetOwningPlayer(), CartLineWidgetClass);
		if (!Row) continue;
		Row->InitializeLine(Line.ProductId, Name, Line.Quantity, LinePrice, LiveCart);
		Row->OnLineFailure().AddUObject(this, &UShopScreenWidget::HandleCartLineFailure);
		CartList->AddChildToVerticalBox(Row);
		CartRows.Add(Row);
	}
}

void UShopScreenWidget::RefreshProducts(const bool bRecreateRows)
{
	UShopCartComponent* LiveCart = Cart.Get();
	UShopCatalog* LiveCatalog = Catalog.Get();
	if (bRecreateRows && ProductGrid)
	{
		ProductGrid->ClearChildren();
		ProductCards.Reset();
		if (LiveCatalog && ProductCardWidgetClass)
		{
			for (const FShopProductEntry& Product : LiveCatalog->Products)
			{
				if (!Product.bForSale) continue;
				UShopProductCardWidget* Card = CreateWidget<UShopProductCardWidget>(GetOwningPlayer(), ProductCardWidgetClass);
				if (!Card) continue;
				Card->InitializeProduct(Product, LiveCart);
				Card->OnAddAttempt().AddUObject(this, &UShopScreenWidget::HandleProductAddAttempt);
				ProductGrid->AddChildToWrapBox(Card);
				ProductCards.Add(Card);
			}
		}
	}
	for (UShopProductCardWidget* Card : ProductCards)
	{
		if (!Card) continue;
		const EShopFailureCode AddFailure = LiveCart
			? LiveCart->EvaluateAdd(Card->GetProductId())
			: EShopFailureCode::MissingCatalog;
		Card->SetAddFailure(AddFailure);
	}
}

void UShopScreenWidget::RefreshOrders(const bool bRecreateRows)
{
	UShopOrderSubsystem* LiveOrders = Orders.Get();
	if (!bRecreateRows || !OrderList) return;
	OrderList->ClearChildren();
	OrderRows.Reset();
	if (!LiveOrders || !OrderLineWidgetClass) return;
	for (const FShopOrderSnapshot& Snapshot : LiveOrders->GetOrderSnapshots())
	{
		UShopOrderLineWidget* Row = CreateWidget<UShopOrderLineWidget>(GetOwningPlayer(), OrderLineWidgetClass);
		if (!Row) continue;
		Row->InitializeOrder(Snapshot);
		OrderList->AddChildToVerticalBox(Row);
		OrderRows.Add(Row);
	}
	CountdownRefreshElapsed = 0.0f;
}

void UShopScreenWidget::RefreshOrderCountdowns()
{
	UShopOrderSubsystem* LiveOrders = Orders.Get();
	if (!LiveOrders) return;
	const TArray<FShopOrderSnapshot> Snapshots = LiveOrders->GetOrderSnapshots();
	const int32 Count = FMath::Min(Snapshots.Num(), OrderRows.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (OrderRows[Index])
		{
			OrderRows[Index]->UpdateStatus(Snapshots[Index].SecondsRemaining, Snapshots[Index].bWaitingForSpace);
		}
	}
}

void UShopScreenWidget::ShowFailure(const EShopFailureCode Failure)
{
	LastDisplayedFailure = Failure;
	if (!OrderFeedbackText) return;
	switch (Failure)
	{
	case EShopFailureCode::NotForSale:
		OrderFeedbackText->SetText(LOCTEXT("NotForSale", "현재 판매하지 않는 상품입니다."));
		break;
	case EShopFailureCode::ProductLimit:
		OrderFeedbackText->SetText(LOCTEXT("ProductLimit", "상품별 최대 수량에 도달했습니다."));
		break;
	case EShopFailureCode::CartLimit:
		OrderFeedbackText->SetText(LOCTEXT("CartLimit", "장바구니 총 수량 한도에 도달했습니다."));
		break;
	case EShopFailureCode::EmptyCart:
		OrderFeedbackText->SetText(LOCTEXT("EmptyCart", "장바구니가 비어 있습니다."));
		break;
	case EShopFailureCode::InvalidProduct:
		OrderFeedbackText->SetText(LOCTEXT("InvalidProduct", "상품 정보를 확인할 수 없습니다."));
		break;
	case EShopFailureCode::InsufficientMoney:
	{
		const FShopPlaceOrderEvaluation Evaluation = Orders.IsValid() && CurrentPlayerState.IsValid()
			? Orders->EvaluatePlaceOrder(CurrentPlayerState.Get())
			: FShopPlaceOrderEvaluation();
		OrderFeedbackText->SetText(Evaluation.ShortfallAmount > 0
			? FText::Format(LOCTEXT("InsufficientMoney", "잔액이 부족합니다. {0}원 더 필요합니다."), FText::AsNumber(Evaluation.ShortfallAmount))
			: LOCTEXT("InsufficientMoneyGeneric", "잔액이 부족합니다."));
		break;
	}
	case EShopFailureCode::Busy:
		OrderFeedbackText->SetText(LOCTEXT("Busy", "주문을 처리하고 있습니다."));
		break;
	case EShopFailureCode::MissingCatalog:
		OrderFeedbackText->SetText(LOCTEXT("MissingCatalog", "상점 설정을 확인할 수 없습니다."));
		break;
	case EShopFailureCode::DeliveryUnavailable:
		OrderFeedbackText->SetText(LOCTEXT("DeliveryUnavailable", "배송을 준비할 수 없습니다."));
		break;
	case EShopFailureCode::InvalidContents:
		OrderFeedbackText->SetText(LOCTEXT("InvalidContents", "주문 상품 정보가 올바르지 않습니다."));
		break;
	default:
		OrderFeedbackText->SetText(FText::GetEmpty());
		break;
	}
}

void UShopScreenWidget::UpdateOrderButton()
{
	if (!OrderButton) return;
	const UShopOrderSubsystem* LiveOrders = Orders.Get();
	const ABathhousePlayerState* PlayerState = CurrentPlayerState.Get();
	const FShopPlaceOrderEvaluation Evaluation = LiveOrders && PlayerState
		? LiveOrders->EvaluatePlaceOrder(PlayerState)
		: FShopPlaceOrderEvaluation();
	OrderButton->SetIsEnabled(Evaluation.bCanOrder);
	if (Evaluation.Failure == EShopFailureCode::InsufficientMoney)
	{
		ShowFailure(EShopFailureCode::InsufficientMoney);
	}
	else if (LastDisplayedFailure == EShopFailureCode::InsufficientMoney)
	{
		ShowFailure(EShopFailureCode::None);
	}
}

#undef LOCTEXT_NAMESPACE
