#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Shop/ShopCartComponent.h"
#include "BathhousePlayerState.generated.h"

class UPlayerWalletComponent;

UCLASS(Blueprintable)
class BATHHOUSESIM_API ABathhousePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ABathhousePlayerState();

	UFUNCTION(BlueprintPure, Category = "Economy")
	UPlayerWalletComponent* GetWallet() const { return Wallet; }

	UShopCartComponent* GetShopCart() const { return ShopCart; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Economy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPlayerWalletComponent> Wallet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShopCartComponent> ShopCart;
};
