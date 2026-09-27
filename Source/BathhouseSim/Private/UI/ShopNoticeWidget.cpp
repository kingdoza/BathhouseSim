#include "UI/ShopNoticeWidget.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Shop/ShopOrderSubsystem.h"
#include "Shop/ShopSettings.h"
#include "TimerManager.h"

void UShopNoticeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bIsConstructed = true;
	if (NoticeText)
	{
		NoticeText->SetVisibility(ESlateVisibility::Collapsed);
	}
	BindOrders();
}

void UShopNoticeWidget::NativeDestruct()
{
	bIsConstructed = false;
	UnbindOrders();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NoticeTimerHandle);
	}
	Super::NativeDestruct();
}

void UShopNoticeWidget::SetOrderSubsystem(UShopOrderSubsystem* InOrders)
{
	if (Orders.Get() == InOrders)
	{
		return;
	}
	UnbindOrders();
	Orders = InOrders;
	if (bIsConstructed)
	{
		BindOrders();
	}
}

void UShopNoticeWidget::BindOrders()
{
	if (Orders.IsValid())
	{
		Orders->OnOrderDelivered.AddDynamic(this, &UShopNoticeWidget::HandleOrderDelivered);
	}
}

void UShopNoticeWidget::UnbindOrders()
{
	if (Orders.IsValid())
	{
		Orders->OnOrderDelivered.RemoveDynamic(this, &UShopNoticeWidget::HandleOrderDelivered);
	}
	Orders.Reset();
}

void UShopNoticeWidget::HandleOrderDelivered(const int64 OrderId)
{
	(void)OrderId;
	if (NoticeText)
	{
		NoticeText->SetText(NSLOCTEXT("ShopNotice", "DeliveryArrived", "배송 도착"));
		NoticeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	const UShopSettings* Settings = GetDefault<UShopSettings>();
	const float Duration = Settings ? Settings->GetDeliveryNoticeSeconds() : 3.0f;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NoticeTimerHandle);
		World->GetTimerManager().SetTimer(
			NoticeTimerHandle,
			this,
			&UShopNoticeWidget::ClearNotice,
			FMath::Max(0.01f, Duration),
			false);
	}
}

void UShopNoticeWidget::ClearNotice()
{
	if (NoticeText)
	{
		NoticeText->SetText(FText::GetEmpty());
		NoticeText->SetVisibility(ESlateVisibility::Collapsed);
	}
}
