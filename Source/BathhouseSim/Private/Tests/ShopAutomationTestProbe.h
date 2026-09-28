#pragma once

#include "CoreMinimal.h"

class AActor;
#include "Economy/PlayerWalletComponent.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Shop/ShopCartComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Shop/ShopOrderSubsystem.h"
#include "ShopAutomationTestProbe.generated.h"

UCLASS(Transient, NotBlueprintable)
class UShopAutomationTestProbe final : public UObject
{
	GENERATED_BODY()

public:
	void Bind(UPlayerWalletComponent* Wallet, UShopCartComponent* Cart, UShopOrderSubsystem* Orders);
	void BindCarry(UPlayerCarryComponent* Carry);
	void ResetHeldChanges();
	void Unbind();

	int32 MoneyChangedCount = 0;
	int32 CartChangedCount = 0;
	int32 OrderDeliveredCount = 0;
	int32 LastPreviousMoney = 0;
	int32 LastCurrentMoney = 0;
	int32 HeldObjectChangedCount = 0;
	AActor* LastHeldObject = nullptr;
	TArray<int64> DeliveredOrderIds;

private:
	UFUNCTION()
	void HandleMoneyChanged(int32 PreviousMoney, int32 CurrentMoney);

	UFUNCTION()
	void HandleCartChanged();

	UFUNCTION()
	void HandleOrderDelivered(int64 OrderId);

	UFUNCTION()
	void HandleHeldObjectChanged(AActor* HeldObject);

	UPROPERTY(Transient)
	TObjectPtr<UPlayerWalletComponent> Wallet = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UShopCartComponent> Cart = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UShopOrderSubsystem> Orders = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerCarryComponent> Carry = nullptr;
};

UCLASS(Transient, NotBlueprintable)
class AShopDeliveryBoxScaleAutomationActor final : public AShopDeliveryBoxActor
{
	GENERATED_BODY()

public:
	AShopDeliveryBoxScaleAutomationActor()
	{
		GetBoxMesh()->SetRelativeScale3D(FVector(0.8f));
	}

	void SetHeldTransformForTest(const FTransform& InTransform)
	{
		HeldTransform = InTransform;
	}
};
