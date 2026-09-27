#include "UI/ShopOrderLineWidget.h"

#include "Components/TextBlock.h"

void UShopOrderLineWidget::InitializeOrder(const FShopOrderSnapshot& Snapshot)
{
	OrderId = Snapshot.OrderId;
	TArray<FString> Parts;
	for (const FShopOrderLine& Line : Snapshot.Lines)
	{
		if (!Line.DisplayName.IsEmpty() && Line.Quantity > 0)
		{
			Parts.Add(FText::Format(
				NSLOCTEXT("ShopOrderLine", "OrderSummaryPart", "{0} {1}"),
				Line.DisplayName,
				FText::AsNumber(Line.Quantity)).ToString());
		}
	}
	if (SummaryText)
	{
		SummaryText->SetText(FText::Format(
			NSLOCTEXT("ShopOrderLine", "OrderSummary", "주문 #{0}: {1}"),
			FText::AsNumber(OrderId),
			FText::FromString(FString::Join(Parts, TEXT(", ")))));
	}
	UpdateStatus(Snapshot.SecondsRemaining, Snapshot.bWaitingForSpace);
}

void UShopOrderLineWidget::UpdateStatus(const float SecondsRemaining, const bool bWaitingForSpace)
{
	if (!StatusText) return;
	if (bWaitingForSpace)
	{
		StatusText->SetText(NSLOCTEXT("ShopOrderLine", "WaitingForSpace", "배송 공간 대기"));
		return;
	}
	StatusText->SetText(FText::Format(
		NSLOCTEXT("ShopOrderLine", "DeliveryCountdown", "배송까지 {0}초"),
		FText::AsNumber(FMath::CeilToInt(FMath::Max(0.0f, SecondsRemaining)))));
}
