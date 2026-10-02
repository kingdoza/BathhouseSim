#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Computer/ComputerScreenContext.h"
#include "ComputerScreenRootWidget.generated.h"

class UBathWaterManagementScreenWidget;
class UButton;
class UExpansionScreenWidget;
class UShopScreenWidget;
class UWidgetSwitcher;

UCLASS()
class BATHHOUSESIM_API UComputerScreenRootWidget
	: public UUserWidget
	, public IComputerScreenContextReceiver
{
	GENERATED_BODY()

public:
	virtual void InitializeComputerScreen(const FComputerScreenContext& Context) override;
	virtual void NotifyComputerUserChanged(APlayerState* PlayerState) override;
	virtual void NotifyComputerUseEnded() override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ManagementTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ShopTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> ScreenSwitcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBathWaterManagementScreenWidget> ManagementScreen;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UShopScreenWidget> ShopScreen;

	/** 기존 WBP가 Editor 작업 전에도 compile되도록 optional이다. 존재는 content 자동화가 확인한다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ExpansionTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UExpansionScreenWidget> ExpansionScreen;

private:
	friend class FExpansionScreenAutomationAccess;

	enum class EComputerScreenTab : uint8
	{
		Management,
		Shop,
		Expansion
	};

	UFUNCTION()
	void HandleManagementTabClicked();

	UFUNCTION()
	void HandleShopTabClicked();

	UFUNCTION()
	void HandleExpansionTabClicked();

	void SelectTab(EComputerScreenTab Tab);
	void ApplySelectedTab();

	FComputerScreenContext ScreenContext;
	TWeakObjectPtr<APlayerState> CurrentUser;
	EComputerScreenTab SelectedTab = EComputerScreenTab::Management;
	bool bButtonsBound = false;
};
