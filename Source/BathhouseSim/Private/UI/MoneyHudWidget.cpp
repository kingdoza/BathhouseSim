#include "UI/MoneyHudWidget.h"

#include "Components/TextBlock.h"
#include "Economy/PlayerWalletComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UMoneyHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bIsConstructed = true;
	ClearDelta();
	BindWallet();
}

void UMoneyHudWidget::NativeDestruct()
{
	bIsConstructed = false;
	UnbindWallet();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeltaTimerHandle);
	}
	Super::NativeDestruct();
}

void UMoneyHudWidget::SetWallet(UPlayerWalletComponent* InWallet)
{
	if (Wallet.Get() == InWallet)
	{
		return;
	}
	UnbindWallet();
	Wallet = InWallet;
	if (bIsConstructed)
	{
		BindWallet();
	}
}

void UMoneyHudWidget::BindWallet()
{
	if (Wallet.IsValid())
	{
		Wallet->OnMoneyChanged.AddDynamic(this, &UMoneyHudWidget::HandleMoneyChanged);
		RefreshMoney();
	}
}

void UMoneyHudWidget::UnbindWallet()
{
	if (Wallet.IsValid())
	{
		Wallet->OnMoneyChanged.RemoveDynamic(this, &UMoneyHudWidget::HandleMoneyChanged);
	}
	Wallet.Reset();
}

void UMoneyHudWidget::RefreshMoney()
{
	if (MoneyText)
	{
		const UPlayerWalletComponent* LiveWallet = Wallet.Get();
		MoneyText->SetText(FText::Format(
			NSLOCTEXT("MoneyHud", "MoneyFormat", "{0}원"),
			FText::AsNumber(LiveWallet ? LiveWallet->GetCurrentMoney() : 0)));
	}
}

void UMoneyHudWidget::HandleMoneyChanged(const int32 PreviousMoney, const int32 CurrentMoney)
{
	RefreshMoney();
	if (!DeltaText)
	{
		return;
	}
	const int64 Delta = static_cast<int64>(CurrentMoney) - static_cast<int64>(PreviousMoney);
	const FString Sign = Delta >= 0 ? TEXT("+") : TEXT("−");
	const int64 Magnitude = Delta >= 0 ? Delta : -Delta;
	DeltaText->SetText(FText::Format(
		NSLOCTEXT("MoneyHud", "MoneyDeltaFormat", "{0}{1}원"),
		FText::FromString(Sign),
		FText::AsNumber(Magnitude)));
	DeltaText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeltaTimerHandle);
		World->GetTimerManager().SetTimer(
			DeltaTimerHandle,
			this,
			&UMoneyHudWidget::ClearDelta,
			FMath::Max(0.1f, DeltaDisplaySeconds),
			false);
	}
}

void UMoneyHudWidget::ClearDelta()
{
	if (DeltaText)
	{
		DeltaText->SetText(FText::GetEmpty());
		DeltaText->SetVisibility(ESlateVisibility::Collapsed);
	}
}
