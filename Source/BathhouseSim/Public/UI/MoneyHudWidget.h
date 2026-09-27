#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "MoneyHudWidget.generated.h"

class UPlayerWalletComponent;
class UTextBlock;

UCLASS(Abstract, Blueprintable)
class BATHHOUSESIM_API UMoneyHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetWallet(UPlayerWalletComponent* InWallet);
	bool HasWallet() const { return Wallet.IsValid(); }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MoneyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DeltaText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy|HUD", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float DeltaDisplaySeconds = 2.0f;

private:
	UFUNCTION()
	void HandleMoneyChanged(int32 PreviousMoney, int32 CurrentMoney);

	void BindWallet();
	void UnbindWallet();
	void RefreshMoney();
	void ClearDelta();

	TWeakObjectPtr<UPlayerWalletComponent> Wallet;
	FTimerHandle DeltaTimerHandle;
	bool bIsConstructed = false;
};
