#include "Economy/BathhousePlayerState.h"

#include "Economy/PlayerWalletComponent.h"
#include "Shop/ShopCartComponent.h"

ABathhousePlayerState::ABathhousePlayerState()
{
	Wallet = CreateDefaultSubobject<UPlayerWalletComponent>(TEXT("Wallet"));
	ShopCart = CreateDefaultSubobject<UShopCartComponent>(TEXT("ShopCart"));
}
