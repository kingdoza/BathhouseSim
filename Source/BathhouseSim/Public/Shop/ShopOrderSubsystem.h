#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "Shop/ShopTypes.h"
#include "ShopOrderSubsystem.generated.h"

class ABathhousePlayerState;
class AShopDeliveryPointActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopOrdersChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopOrderDelivered, int64, OrderId);

USTRUCT()
struct BATHHOUSESIM_API FShopOrderRecord
{
	GENERATED_BODY()

	UPROPERTY()
	int64 OrderId = 0;

	UPROPERTY()
	TArray<FShopOrderLine> Lines;

	UPROPERTY()
	double ReadyGameTime = 0.0;

	UPROPERTY()
	bool bWaitingForSpace = false;
};

UCLASS()
class BATHHOUSESIM_API UShopOrderSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return !Orders.IsEmpty(); }

	FShopPlaceOrderEvaluation EvaluatePlaceOrder(const ABathhousePlayerState* PlayerState) const;
	bool TryPlaceOrder(ABathhousePlayerState* PlayerState, EShopFailureCode& OutFailure);
	TArray<FShopOrderSnapshot> GetOrderSnapshots() const;
	void RegisterDeliveryPoint(AShopDeliveryPointActor* DeliveryPoint);
	void UnregisterDeliveryPoint(AShopDeliveryPointActor* DeliveryPoint);

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnShopOrdersChanged OnOrdersChanged;

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnShopOrderDelivered OnOrderDelivered;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	bool TryDeliverFrontOrder(double Now);
	void ProcessReadyOrders(double Now);
	TArray<AShopDeliveryPointActor*> GetLiveDeliveryPoints() const;

	UPROPERTY(Transient)
	TArray<FShopOrderRecord> Orders;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AShopDeliveryPointActor>> DeliveryPoints;

	int64 NextOrderId = 1;
	double NextDeliveryAttemptTime = 0.0;
	bool bPlaceOrderInProgress = false;
	bool bWarnedMissingDeliveryPoint = false;
	bool bWarnedMultipleDeliveryPoints = false;
};
