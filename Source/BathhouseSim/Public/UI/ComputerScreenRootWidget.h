#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Computer/ComputerScreenContext.h"
#include "ComputerScreenRootWidget.generated.h"

class UBathWaterManagementScreenWidget;
class UButton;
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

private:
	UFUNCTION()
	void HandleManagementTabClicked();

	UFUNCTION()
	void HandleShopTabClicked();

	void ApplySelectedTab();

	FComputerScreenContext ScreenContext;
	TWeakObjectPtr<APlayerState> CurrentUser;
	bool bShopSelected = false;
	bool bButtonsBound = false;
};
