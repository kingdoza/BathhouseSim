#include "UI/ExpansionScreenWidget.h"

#include "Building/BathhouseExpansionPurchaseSubsystem.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Economy/BathhousePlayerState.h"
#include "Economy/PlayerWalletComponent.h"
#include "Engine/World.h"
#include "Facility/LockerCapacitySubsystem.h"
#include "UI/ExpansionScreenModel.h"
#include "UI/ExpansionSpaceOptionWidget.h"

namespace
{
	void SetTextAndVisibility(UTextBlock* Block, const FText& Text, const bool bVisible)
	{
		if (Block)
		{
			Block->SetText(Text);
			Block->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	void SetPanelVisible(UPanelWidget* Panel, const bool bVisible)
	{
		if (Panel)
		{
			Panel->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void UExpansionScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!bButtonsBound)
	{
		if (PurchaseButton)
		{
			PurchaseButton->OnClicked.AddDynamic(this, &UExpansionScreenWidget::HandlePurchaseClicked);
		}
		if (ConfirmButton)
		{
			ConfirmButton->OnClicked.AddDynamic(this, &UExpansionScreenWidget::HandleConfirmClicked);
		}
		if (CancelButton)
		{
			CancelButton->OnClicked.AddDynamic(this, &UExpansionScreenWidget::HandleCancelClicked);
		}
		UExpansionSpaceOptionWidget* const Options[3] = { HallOption, BathOption, WorkOption };
		const EBathhouseSpaceKind Kinds[3] = { EBathhouseSpaceKind::Hall, EBathhouseSpaceKind::Bath, EBathhouseSpaceKind::Work };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (Options[Index])
			{
				Options[Index]->SetSpaceKind(Kinds[Index]);
				OptionHandles[Index] = Options[Index]->OnClicked().AddUObject(
					this, &UExpansionScreenWidget::HandleOptionClicked);
			}
		}
		bButtonsBound = true;
	}
	RebindDomain();
}

void UExpansionScreenWidget::NativeDestruct()
{
	UnbindDomain();
	if (bButtonsBound)
	{
		if (PurchaseButton)
		{
			PurchaseButton->OnClicked.RemoveDynamic(this, &UExpansionScreenWidget::HandlePurchaseClicked);
		}
		if (ConfirmButton)
		{
			ConfirmButton->OnClicked.RemoveDynamic(this, &UExpansionScreenWidget::HandleConfirmClicked);
		}
		if (CancelButton)
		{
			CancelButton->OnClicked.RemoveDynamic(this, &UExpansionScreenWidget::HandleCancelClicked);
		}
		UExpansionSpaceOptionWidget* const Options[3] = { HallOption, BathOption, WorkOption };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (Options[Index] && OptionHandles[Index].IsValid())
			{
				Options[Index]->OnClicked().Remove(OptionHandles[Index]);
			}
			OptionHandles[Index].Reset();
		}
		bButtonsBound = false;
	}
	Super::NativeDestruct();
}

void UExpansionScreenWidget::InitializeComputerScreen(const FComputerScreenContext& Context)
{
	ScreenContext = Context;
}

void UExpansionScreenWidget::NotifyComputerUserChanged(APlayerState* PlayerState)
{
	CurrentPlayerState = Cast<ABathhousePlayerState>(PlayerState);
	bConfirmPending = false;
	RebindDomain();
}

void UExpansionScreenWidget::NotifyComputerUseEnded()
{
	CancelPendingConfirm();
}

void UExpansionScreenWidget::RebindDomain()
{
	UnbindDomain();
	const ABathhousePlayerState* PlayerState = CurrentPlayerState.Get();
	Wallet = PlayerState ? PlayerState->GetWallet() : nullptr;
	UWorld* World = GetWorld();
	Lockers = World ? World->GetSubsystem<ULockerCapacitySubsystem>() : nullptr;
	Purchase = World ? World->GetSubsystem<UBathhouseExpansionPurchaseSubsystem>() : nullptr;
	if (Wallet.IsValid())
	{
		Wallet->OnMoneyChanged.AddDynamic(this, &UExpansionScreenWidget::HandleMoneyChanged);
	}
	if (Lockers.IsValid())
	{
		Lockers->OnLockerCapacityChanged.AddDynamic(this, &UExpansionScreenWidget::HandleLockerCapacityChanged);
	}
	if (Purchase.IsValid())
	{
		ExpansionChangedHandle = Purchase->OnExpansionChanged.AddUObject(this, &UExpansionScreenWidget::HandleExpansionChanged);
	}
	RefreshFromDomain();
}

void UExpansionScreenWidget::UnbindDomain()
{
	if (Wallet.IsValid())
	{
		Wallet->OnMoneyChanged.RemoveDynamic(this, &UExpansionScreenWidget::HandleMoneyChanged);
	}
	if (Lockers.IsValid())
	{
		Lockers->OnLockerCapacityChanged.RemoveDynamic(this, &UExpansionScreenWidget::HandleLockerCapacityChanged);
	}
	if (Purchase.IsValid() && ExpansionChangedHandle.IsValid())
	{
		Purchase->OnExpansionChanged.Remove(ExpansionChangedHandle);
	}
	ExpansionChangedHandle.Reset();
	Wallet.Reset();
	Lockers.Reset();
	Purchase.Reset();
}

void UExpansionScreenWidget::HandleMoneyChanged(const int32 PreviousMoney, const int32 CurrentMoney)
{
	(void)PreviousMoney;
	(void)CurrentMoney;
	RefreshFromDomain();
}

void UExpansionScreenWidget::HandleLockerCapacityChanged(const int32 InstalledCapacity, const int32 ActiveLeaseCount)
{
	(void)InstalledCapacity;
	(void)ActiveLeaseCount;
	RefreshFromDomain();
}

void UExpansionScreenWidget::HandleExpansionChanged()
{
	RefreshFromDomain();
}

void UExpansionScreenWidget::RefreshFromDomain()
{
	FBathhouseExpansionView View;
	if (const UBathhouseExpansionPurchaseSubsystem* Subsystem = Purchase.Get())
	{
		View = Subsystem->BuildView(CurrentPlayerState.Get());
	}
	if (View.bBusy)
	{
		return;
	}
	FExpansionScreenState State;
	State.Selected = SelectedKind;
	State.bConfirmPending = bConfirmPending;
	State.bShowCompleted = bShowCompleted;
	FExpansionScreenDisplay Display = FExpansionScreenModel::Build(View, State);
	if (Display.bClearSelection)
	{
		SelectedKind.Reset();
		bConfirmPending = false;
		State.Selected.Reset();
		State.bConfirmPending = false;
		Display = FExpansionScreenModel::Build(View, State);
	}
	ApplyDisplay(Display);
}

void UExpansionScreenWidget::ApplyDisplay(const FExpansionScreenDisplay& Display)
{
	SetTextAndVisibility(StageText, Display.Stage, Display.bStageVisible);
	SetTextAndVisibility(BalanceText, Display.Balance, Display.bBalanceVisible);
	SetTextAndVisibility(LockerText, Display.Locker, Display.bLockerVisible);
	SetPanelVisible(OptionsPanel, Display.bOptionsVisible);
	UExpansionSpaceOptionWidget* const Options[3] = { HallOption, BathOption, WorkOption };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (Options[Index])
		{
			Options[Index]->ApplyModel(Display.Options[Index]);
		}
	}
	SetTextAndVisibility(PriceText, Display.Price, Display.bPriceVisible);
	SetPanelVisible(PurchasePanel, Display.bPurchasePanelVisible);
	if (PurchaseButtonText)
	{
		PurchaseButtonText->SetText(Display.PurchaseButtonText);
	}
	if (PurchaseButton)
	{
		PurchaseButton->SetIsEnabled(Display.bPurchaseEnabled);
	}
	SetPanelVisible(ConfirmPanel, Display.bConfirmPanelVisible);
	if (ConfirmButton)
	{
		ConfirmButton->SetIsEnabled(Display.bConfirmEnabled);
	}
	SetTextAndVisibility(ShortfallText, Display.Shortfall, Display.bShortfallVisible);
	SetTextAndVisibility(ResultText, Display.Result, Display.bResultVisible);
	SetTextAndVisibility(MessageText, Display.Message, Display.bMessageVisible);
}

void UExpansionScreenWidget::HandleOptionClicked(const EBathhouseSpaceKind Kind)
{
	const UBathhouseExpansionPurchaseSubsystem* Subsystem = Purchase.Get();
	if (!Subsystem)
	{
		return;
	}
	const FBathhouseExpansionView View = Subsystem->BuildView(CurrentPlayerState.Get());
	const int32 Index = static_cast<int32>(Kind);
	if (View.bBusy || !View.bAvailable || View.bMaxReached || !View.Options[Index].bPresent || !View.Options[Index].bCanExpand)
	{
		return;
	}
	SelectedKind = Kind;
	bShowCompleted = false;
	bConfirmPending = false;
	RefreshFromDomain();
}

void UExpansionScreenWidget::HandlePurchaseClicked()
{
	const UBathhouseExpansionPurchaseSubsystem* Subsystem = Purchase.Get();
	if (!Subsystem || !SelectedKind.IsSet() || bConfirmPending)
	{
		return;
	}
	const FBathhouseExpansionView View = Subsystem->BuildView(CurrentPlayerState.Get());
	FExpansionScreenState State;
	State.Selected = SelectedKind;
	const FExpansionScreenDisplay Display = FExpansionScreenModel::Build(View, State);
	if (View.bBusy || !Display.bPurchaseEnabled)
	{
		return;
	}
	bConfirmPending = true;
	ConfirmPurchaseCount = View.PurchaseCount;
	bShowCompleted = false;
	RefreshFromDomain();
}

void UExpansionScreenWidget::HandleConfirmClicked()
{
	if (!bConfirmPending)
	{
		return;
	}
	// 대기를 먼저 내려 같은 확인에서 오는 두 번째 이벤트가 구입을 다시 요청하지 못하게 한다.
	bConfirmPending = false;
	UBathhouseExpansionPurchaseSubsystem* Subsystem = Purchase.Get();
	APlayerState* User = CurrentPlayerState.Get();
	if (Subsystem && User && SelectedKind.IsSet())
	{
		if (Subsystem->TryPurchase(User, SelectedKind.GetValue(), ConfirmPurchaseCount) == EBathhouseExpansionFailure::None)
		{
			SelectedKind.Reset();
			bShowCompleted = true;
		}
	}
	RefreshFromDomain();
}

void UExpansionScreenWidget::HandleCancelClicked()
{
	CancelPendingConfirm();
}

void UExpansionScreenWidget::CancelPendingConfirm()
{
	if (bConfirmPending)
	{
		bConfirmPending = false;
		RefreshFromDomain();
	}
}
