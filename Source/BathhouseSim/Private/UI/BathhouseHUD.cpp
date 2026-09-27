#include "UI/BathhouseHUD.h"

#include "Blueprint/UserWidget.h"
#include "Economy/BathhousePlayerState.h"
#include "Economy/PlayerWalletComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Shop/ShopOrderSubsystem.h"
#include "Shop/ShopSettings.h"
#include "TimerManager.h"
#include "UI/InteractionPromptWidget.h"
#include "UI/MoneyHudWidget.h"
#include "UI/ShopNoticeWidget.h"

void ABathhouseHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	PlayerController->OnPossessedPawnChanged.AddDynamic(this, &ABathhouseHUD::HandlePossessedPawnChanged);
	if (InteractionPromptWidgetClass)
	{
		InteractionPromptWidget = CreateWidget<UInteractionPromptWidget>(PlayerController, InteractionPromptWidgetClass);
		if (InteractionPromptWidget)
		{
			InteractionPromptWidget->AddToViewport();
		}
	}
	if (MoneyHudWidgetClass)
	{
		MoneyHudWidget = CreateWidget<UMoneyHudWidget>(PlayerController, MoneyHudWidgetClass);
		if (MoneyHudWidget)
		{
			MoneyHudWidget->AddToViewport();
		}
	}
	if (ShopNoticeWidgetClass)
	{
		ShopNoticeWidget = CreateWidget<UShopNoticeWidget>(PlayerController, ShopNoticeWidgetClass);
		if (ShopNoticeWidget)
		{
			UShopOrderSubsystem* Orders = GetWorld()
				? GetWorld()->GetSubsystem<UShopOrderSubsystem>()
				: nullptr;
			ShopNoticeWidget->SetOrderSubsystem(Orders);
			ShopNoticeWidget->AddToViewport();
		}
	}

	RebindPawn(PlayerController->GetPawn());
	TryBindWallet();
}

void ABathhouseHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &ABathhouseHUD::HandlePossessedPawnChanged);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WalletBindRetryTimerHandle);
	}
	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->SetInteractionComponent(nullptr);
		InteractionPromptWidget->RemoveFromParent();
		InteractionPromptWidget = nullptr;
	}
	if (MoneyHudWidget)
	{
		MoneyHudWidget->SetWallet(nullptr);
		MoneyHudWidget->RemoveFromParent();
		MoneyHudWidget = nullptr;
	}
	if (ShopNoticeWidget)
	{
		ShopNoticeWidget->SetOrderSubsystem(nullptr);
		ShopNoticeWidget->RemoveFromParent();
		ShopNoticeWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void ABathhouseHUD::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	(void)OldPawn;
	RebindPawn(NewPawn);
	TryBindWallet();
}

void ABathhouseHUD::RebindPawn(APawn* Pawn)
{
	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->SetInteractionComponent(Pawn ? Pawn->FindComponentByClass<UPlayerInteractionComponent>() : nullptr);
	}
}

void ABathhouseHUD::TryBindWallet()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	ABathhousePlayerState* PlayerState = PlayerController
		? Cast<ABathhousePlayerState>(PlayerController->PlayerState)
		: nullptr;
	UPlayerWalletComponent* Wallet = PlayerState ? PlayerState->GetWallet() : nullptr;
	if (MoneyHudWidget && IsValid(Wallet))
	{
		MoneyHudWidget->SetWallet(Wallet);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(WalletBindRetryTimerHandle);
		}
		return;
	}

	if (MoneyHudWidget && GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(WalletBindRetryTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			WalletBindRetryTimerHandle,
			this,
			&ABathhouseHUD::TryBindWallet,
			0.25f,
			true);
	}
}
