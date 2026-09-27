#include "Tests/ShopAutomationTestProbe.h"

void UShopAutomationTestProbe::Bind(
	UPlayerWalletComponent* InWallet,
	UShopCartComponent* InCart,
	UShopOrderSubsystem* InOrders)
{
	Unbind();
	Wallet = InWallet;
	Cart = InCart;
	Orders = InOrders;
	if (Wallet) Wallet->OnMoneyChanged.AddDynamic(this, &UShopAutomationTestProbe::HandleMoneyChanged);
	if (Cart) Cart->OnCartChanged.AddDynamic(this, &UShopAutomationTestProbe::HandleCartChanged);
	if (Orders) Orders->OnOrderDelivered.AddDynamic(this, &UShopAutomationTestProbe::HandleOrderDelivered);
}

void UShopAutomationTestProbe::BindCarry(UPlayerCarryComponent* InCarry)
{
	if (Carry) Carry->OnHeldObjectChanged.RemoveDynamic(this, &UShopAutomationTestProbe::HandleHeldObjectChanged);
	Carry = InCarry;
	if (Carry) Carry->OnHeldObjectChanged.AddDynamic(this, &UShopAutomationTestProbe::HandleHeldObjectChanged);
}

void UShopAutomationTestProbe::ResetHeldChanges()
{
	HeldObjectChangedCount = 0;
	LastHeldObject = nullptr;
}

void UShopAutomationTestProbe::Unbind()
{
	if (Wallet) Wallet->OnMoneyChanged.RemoveDynamic(this, &UShopAutomationTestProbe::HandleMoneyChanged);
	if (Cart) Cart->OnCartChanged.RemoveDynamic(this, &UShopAutomationTestProbe::HandleCartChanged);
	if (Orders) Orders->OnOrderDelivered.RemoveDynamic(this, &UShopAutomationTestProbe::HandleOrderDelivered);
	if (Carry) Carry->OnHeldObjectChanged.RemoveDynamic(this, &UShopAutomationTestProbe::HandleHeldObjectChanged);
	Wallet = nullptr;
	Cart = nullptr;
	Orders = nullptr;
	Carry = nullptr;
}

void UShopAutomationTestProbe::HandleMoneyChanged(const int32 PreviousMoney, const int32 CurrentMoney)
{
	++MoneyChangedCount;
	LastPreviousMoney = PreviousMoney;
	LastCurrentMoney = CurrentMoney;
}

void UShopAutomationTestProbe::HandleCartChanged()
{
	++CartChangedCount;
}

void UShopAutomationTestProbe::HandleOrderDelivered(const int64 OrderId)
{
	++OrderDeliveredCount;
	DeliveredOrderIds.Add(OrderId);
}

void UShopAutomationTestProbe::HandleHeldObjectChanged(AActor* HeldObject)
{
	++HeldObjectChangedCount;
	LastHeldObject = HeldObject;
}
