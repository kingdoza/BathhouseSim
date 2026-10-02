#include "UI/ComputerScreenRootWidget.h"

#include "GameFramework/PlayerState.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "UI/BathWaterManagementScreenWidget.h"
#include "UI/ExpansionScreenWidget.h"
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
		if (ExpansionTabButton)
		{
			ExpansionTabButton->OnClicked.AddDynamic(this, &UComputerScreenRootWidget::HandleExpansionTabClicked);
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
	if (ExpansionScreen)
	{
		ExpansionScreen->NotifyComputerUserChanged(CurrentUser.Get());
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
		if (ExpansionTabButton)
		{
			ExpansionTabButton->OnClicked.RemoveDynamic(this, &UComputerScreenRootWidget::HandleExpansionTabClicked);
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
	if (ExpansionScreen)
	{
		ExpansionScreen->InitializeComputerScreen(Context);
		ExpansionScreen->NotifyComputerUserChanged(CurrentUser.Get());
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
	if (ExpansionScreen)
	{
		ExpansionScreen->NotifyComputerUserChanged(PlayerState);
	}
}

void UComputerScreenRootWidget::NotifyComputerUseEnded()
{
	// 확인 대기만 취소한다. 탭과 선택 등 나머지 화면 상태는 유지한다.
	if (ExpansionScreen)
	{
		ExpansionScreen->CancelPendingConfirm();
	}
}

void UComputerScreenRootWidget::HandleManagementTabClicked()
{
	SelectTab(EComputerScreenTab::Management);
}

void UComputerScreenRootWidget::HandleShopTabClicked()
{
	SelectTab(EComputerScreenTab::Shop);
}

void UComputerScreenRootWidget::HandleExpansionTabClicked()
{
	SelectTab(EComputerScreenTab::Expansion);
}

void UComputerScreenRootWidget::SelectTab(const EComputerScreenTab Tab)
{
	if (SelectedTab == EComputerScreenTab::Expansion && Tab != EComputerScreenTab::Expansion && ExpansionScreen)
	{
		ExpansionScreen->CancelPendingConfirm();
	}
	SelectedTab = Tab;
	ApplySelectedTab();
	if (Tab == EComputerScreenTab::Expansion && ExpansionScreen)
	{
		ExpansionScreen->RefreshFromDomain();
	}
}

void UComputerScreenRootWidget::ApplySelectedTab()
{
	if (ScreenSwitcher)
	{
		ScreenSwitcher->SetActiveWidgetIndex(static_cast<int32>(SelectedTab));
	}
	if (ManagementTabButton)
	{
		ManagementTabButton->SetIsEnabled(SelectedTab != EComputerScreenTab::Management);
	}
	if (ShopTabButton)
	{
		ShopTabButton->SetIsEnabled(SelectedTab != EComputerScreenTab::Shop);
	}
	if (ExpansionTabButton)
	{
		ExpansionTabButton->SetIsEnabled(SelectedTab != EComputerScreenTab::Expansion);
	}
}
