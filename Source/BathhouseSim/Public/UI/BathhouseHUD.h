#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TimerManager.h"
#include "BathhouseHUD.generated.h"

class APawn;
class UInteractionPromptWidget;
class UMoneyHudWidget;
class UShopNoticeWidget;
class UScrubFocusHudWidget;

UCLASS(Blueprintable)
class BATHHOUSESIM_API ABathhouseHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UInteractionPromptWidget> InteractionPromptWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Economy")
	TSubclassOf<UMoneyHudWidget> MoneyHudWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Shop")
	TSubclassOf<UShopNoticeWidget> ShopNoticeWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Service")
	TSubclassOf<UScrubFocusHudWidget> ScrubFocusHudWidgetClass;

private:

	UPROPERTY(Transient)
	TObjectPtr<UScrubFocusHudWidget> ScrubFocusHudWidget;
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void RebindPawn(APawn* Pawn);
	void TryBindWallet();

	UPROPERTY(Transient)
	TObjectPtr<UInteractionPromptWidget> InteractionPromptWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMoneyHudWidget> MoneyHudWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UShopNoticeWidget> ShopNoticeWidget = nullptr;

	FTimerHandle WalletBindRetryTimerHandle;
};
