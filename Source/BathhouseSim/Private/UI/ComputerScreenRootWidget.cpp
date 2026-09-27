#include "UI/ComputerScreenRootWidget.h"

#include "GameFramework/PlayerState.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "UI/BathWaterManagementScreenWidget.h"
#include "UI/ShopScreenWidget.h"

void UComputerScreenRootWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!bButtonsBound)
	{
		if (ManagementTabButton)
		{
			ManagementTabButton->OnClicked.AddDynamic(this, &UComputerScreenRootWidget::HandleManagementTabClicked);
		}
		if (ShopTabButton)
		{
			ShopTabButton->OnClicked.AddDynamic(this, &UComputerScreenRootWidget::HandleShopTabClicked);
		}
		bButtonsBound = true;
	}
	ApplySelectedTab();
	if (ManagementScreen)
	{
		ManagementScreen->InitializeComputerScreen(ScreenContext);
	}
	if (ShopScreen)
	{
		ShopScreen->NotifyComputerUserChanged(CurrentUser.Get());
	}
}

void UComputerScreenRootWidget::NativeDestruct()
{
	if (bButtonsBound)
	{
		if (ManagementTabButton)
		{
			ManagementTabButton->OnClicked.RemoveDynamic(this, &UComputerScreenRootWidget::HandleManagementTabClicked);
		}
		if (ShopTabButton)
		{
			ShopTabButton->OnClicked.RemoveDynamic(this, &UComputerScreenRootWidget::HandleShopTabClicked);
		}
		bButtonsBound = false;
	}
	Super::NativeDestruct();
}

void UComputerScreenRootWidget::InitializeComputerScreen(const FComputerScreenContext& Context)
{
	ScreenContext = Context;
	if (ManagementScreen)
	{
		ManagementScreen->InitializeComputerScreen(Context);
	}
	if (ShopScreen)
	{
		ShopScreen->InitializeComputerScreen(Context);
		ShopScreen->NotifyComputerUserChanged(CurrentUser.Get());
	}
}

void UComputerScreenRootWidget::NotifyComputerUserChanged(APlayerState* PlayerState)
{
	CurrentUser = PlayerState;
	if (ManagementScreen)
	{
		ManagementScreen->NotifyComputerUserChanged(PlayerState);
	}
	if (ShopScreen)
	{
		ShopScreen->NotifyComputerUserChanged(PlayerState);
	}
}

void UComputerScreenRootWidget::HandleManagementTabClicked()
{
	bShopSelected = false;
	ApplySelectedTab();
}

void UComputerScreenRootWidget::HandleShopTabClicked()
{
	bShopSelected = true;
	ApplySelectedTab();
}

void UComputerScreenRootWidget::ApplySelectedTab()
{
	if (ScreenSwitcher)
	{
		ScreenSwitcher->SetActiveWidgetIndex(bShopSelected ? 1 : 0);
	}
	if (ManagementTabButton)
	{
		ManagementTabButton->SetIsEnabled(bShopSelected);
	}
	if (ShopTabButton)
	{
		ShopTabButton->SetIsEnabled(!bShopSelected);
	}
}
