#include "Shop/ShopOrderSubsystem.h"
#include "Shop/ShopCatalog.h"

#include "Templates/UnrealTemplate.h"

#include "Engine/World.h"
#include "Economy/BathhousePlayerState.h"
#include "Economy/PlayerWalletComponent.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Shop/ShopCartComponent.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Shop/ShopDeliveryPointActor.h"
#include "Shop/ShopProductRules.h"
#include "Shop/ShopSettings.h"

void UShopOrderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	NextOrderId = 1;
	NextDeliveryAttemptTime = 0.0;
}

void UShopOrderSubsystem::Deinitialize()
{
	OnOrdersChanged.Clear();
	OnOrderDelivered.Clear();
	Orders.Reset();
	DeliveryPoints.Reset();
	Super::Deinitialize();
}

bool UShopOrderSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UShopOrderSubsystem::Tick(const float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World || Orders.IsEmpty())
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	if (Now < NextDeliveryAttemptTime)
	{
		return;
	}
	NextDeliveryAttemptTime = Now + GetDefault<UShopSettings>()->GetDeliveryAttemptIntervalSeconds();
	ProcessReadyOrders(Now);
}

TStatId UShopOrderSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UShopOrderSubsystem, STATGROUP_Tickables);
}

FShopPlaceOrderEvaluation UShopOrderSubsystem::EvaluatePlaceOrder(
	const ABathhousePlayerState* PlayerState) const
{
	FShopPlaceOrderEvaluation Evaluation;
	if (bPlaceOrderInProgress)
	{
		Evaluation.Failure = EShopFailureCode::Busy;
		return Evaluation;
	}
	const UShopSettings* Settings = GetDefault<UShopSettings>();
	UShopCatalog* Catalog = Settings ? Settings->LoadCatalog() : nullptr;
	const UShopCartComponent* Cart = PlayerState ? PlayerState->GetShopCart() : nullptr;
	const UPlayerWalletComponent* Wallet = PlayerState ? PlayerState->GetWallet() : nullptr;
	if (!Catalog || !Cart || !Wallet)
	{
		Evaluation.Failure = EShopFailureCode::MissingCatalog;
		return Evaluation;
	}
	if (Cart->GetLines().IsEmpty())
	{
		Evaluation.Failure = EShopFailureCode::EmptyCart;
		return Evaluation;
	}
	int64 Total = 0;
	for (const FShopCartLine& CartLine : Cart->GetLines())
	{
		const FShopProductEntry* Product = Catalog->FindProduct(CartLine.ProductId);
		FText DefinitionFailure;
		if (!Product || !Product->bForSale || CartLine.Quantity <= 0
			|| !FShopProductRules::ValidateProduct(*Product, DefinitionFailure))
		{
			Evaluation.Failure = EShopFailureCode::InvalidProduct;
			return Evaluation;
		}
		Total += static_cast<int64>(Product->Price) * static_cast<int64>(CartLine.Quantity);
		if (Total > MAX_int32)
		{
			Evaluation.Failure = EShopFailureCode::InvalidProduct;
			return Evaluation;
		}
	}
	Evaluation.TotalPrice = static_cast<int32>(Total);
	const int32 Balance = Wallet->GetCurrentMoney();
	Evaluation.ShortfallAmount = FMath::Max(0, Evaluation.TotalPrice - Balance);
	if (Evaluation.ShortfallAmount > 0)
	{
		Evaluation.Failure = EShopFailureCode::InsufficientMoney;
		return Evaluation;
	}
	Evaluation.bCanOrder = true;
	Evaluation.Failure = EShopFailureCode::None;
	return Evaluation;
}

bool UShopOrderSubsystem::TryPlaceOrder(
	ABathhousePlayerState* PlayerState,
	EShopFailureCode& OutFailure)
{
	OutFailure = EShopFailureCode::None;
	if (bPlaceOrderInProgress)
	{
		OutFailure = EShopFailureCode::Busy;
		return false;
	}
	const FShopPlaceOrderEvaluation Evaluation = EvaluatePlaceOrder(PlayerState);
	if (!Evaluation.bCanOrder)
	{
		OutFailure = Evaluation.Failure;
		return false;
	}
	TGuardValue<bool> PlaceOrderGuard(bPlaceOrderInProgress, true);
	const UShopSettings* Settings = GetDefault<UShopSettings>();
	UShopCatalog* Catalog = Settings ? Settings->LoadCatalog() : nullptr;
	UPlayerWalletComponent* Wallet = PlayerState ? PlayerState->GetWallet() : nullptr;
	UShopCartComponent* Cart = PlayerState ? PlayerState->GetShopCart() : nullptr;
	UWorld* World = GetWorld();
	if (!Settings || !Catalog || !Wallet || !Cart || !World)
	{
		OutFailure = EShopFailureCode::MissingCatalog;
		return false;
	}

	FShopOrderRecord NewOrder;
	NewOrder.OrderId = NextOrderId++;
	NewOrder.ReadyGameTime = World->GetTimeSeconds() + Settings->GetDeliveryDelaySeconds();
	for (const FShopCartLine& CartLine : Cart->GetLines())
	{
		const FShopProductEntry* Product = Catalog->FindProduct(CartLine.ProductId);
		if (!Product || (Product->PlacementDefinition == nullptr) == (Product->ItemBoxDefinition == nullptr))
		{
			OutFailure = EShopFailureCode::InvalidProduct;
			return false;
		}
		FShopOrderLine& OrderLine = NewOrder.Lines.AddDefaulted_GetRef();
		OrderLine.ProductId = Product->ProductId;
		OrderLine.PlacementDefinition = Product->PlacementDefinition;
		OrderLine.ItemBoxDefinition = Product->ItemBoxDefinition;
		OrderLine.DisplayName = Product->DisplayName;
		OrderLine.Quantity = CartLine.Quantity;
	}
	if (!Wallet->TrySpendMoney(Evaluation.TotalPrice))
	{
		OutFailure = EShopFailureCode::InsufficientMoney;
		return false;
	}

	Orders.Add(MoveTemp(NewOrder));
	Cart->Clear();
	OnOrdersChanged.Broadcast();
	if (Settings->GetDeliveryDelaySeconds() <= 0.0f)
	{
		ProcessReadyOrders(World->GetTimeSeconds());
	}
	return true;
}

TArray<FShopOrderSnapshot> UShopOrderSubsystem::GetOrderSnapshots() const
{
	TArray<FShopOrderSnapshot> Snapshots;
	Snapshots.Reserve(Orders.Num());
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	for (const FShopOrderRecord& Order : Orders)
	{
		FShopOrderSnapshot& Snapshot = Snapshots.AddDefaulted_GetRef();
		Snapshot.OrderId = Order.OrderId;
		Snapshot.Lines = Order.Lines;
		Snapshot.SecondsRemaining = static_cast<float>(FMath::Max(0.0, Order.ReadyGameTime - Now));
		Snapshot.bWaitingForSpace = Order.bWaitingForSpace;
	}
	return Snapshots;
}

void UShopOrderSubsystem::RegisterDeliveryPoint(AShopDeliveryPointActor* DeliveryPoint)
{
	if (IsValid(DeliveryPoint))
	{
		DeliveryPoints.AddUnique(DeliveryPoint);
		bWarnedMissingDeliveryPoint = false;
	}
}

void UShopOrderSubsystem::UnregisterDeliveryPoint(AShopDeliveryPointActor* DeliveryPoint)
{
	DeliveryPoints.RemoveAll([DeliveryPoint](const TWeakObjectPtr<AShopDeliveryPointActor>& Existing)
	{
		return !Existing.IsValid() || Existing.Get() == DeliveryPoint;
	});
}

TArray<AShopDeliveryPointActor*> UShopOrderSubsystem::GetLiveDeliveryPoints() const
{
	TArray<AShopDeliveryPointActor*> Result;
	for (const TWeakObjectPtr<AShopDeliveryPointActor>& Point : DeliveryPoints)
	{
		if (Point.IsValid())
		{
			Result.Add(Point.Get());
		}
	}
	return Result;
}

void UShopOrderSubsystem::ProcessReadyOrders(const double Now)
{
	while (!Orders.IsEmpty() && Orders[0].ReadyGameTime <= Now)
	{
		const bool bWasWaiting = Orders[0].bWaitingForSpace;
		if (!TryDeliverFrontOrder(Now))
		{
			if (!bWasWaiting)
			{
				Orders[0].bWaitingForSpace = true;
				OnOrdersChanged.Broadcast();
			}
			return;
		}
	}
}

bool UShopOrderSubsystem::TryDeliverFrontOrder(const double Now)
{
	(void)Now;
	if (Orders.IsEmpty())
	{
		return false;
	}
	const TArray<AShopDeliveryPointActor*> Points = GetLiveDeliveryPoints();
	if (Points.IsEmpty())
	{
		if (!bWarnedMissingDeliveryPoint)
		{
			UE_LOG(LogTemp, Warning, TEXT("Shop orders are waiting because no delivery point is registered."));
			bWarnedMissingDeliveryPoint = true;
		}
		return false;
	}
	if (Points.Num() > 1 && !bWarnedMultipleDeliveryPoints)
	{
		UE_LOG(LogTemp, Warning, TEXT("Multiple shop delivery points are registered; using the first one."));
		bWarnedMultipleDeliveryPoints = true;
	}
	const UShopSettings* Settings = GetDefault<UShopSettings>();
	TSubclassOf<AShopDeliveryBoxActor> BoxClass = Settings ? Settings->LoadDeliveryBoxClass() : nullptr;
	if (!Settings || !BoxClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Shop order is waiting because DeliveryBoxClass is not configured."));
		return false;
	}
	const AShopDeliveryBoxActor* BoxCDO = BoxClass->GetDefaultObject<AShopDeliveryBoxActor>();
	const UPrimitiveComponent* CollisionTemplate = BoxCDO ? BoxCDO->GetBoxMesh() : nullptr;
	if (!BoxCDO || !CollisionTemplate)
	{
		UE_LOG(LogTemp, Error, TEXT("Shop order is waiting because the delivery box has no collision root."));
		return false;
	}
	FTransform DropTransform;
	if (!Points[0]->FindDropTransform(BoxCDO->GetBoxHalfExtent(), *CollisionTemplate, DropTransform))
	{
		return false;
	}

	UWorld* World = GetWorld();
	AShopDeliveryBoxActor* Box = World
		? World->SpawnActorDeferred<AShopDeliveryBoxActor>(
			BoxClass,
			DropTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn)
		: nullptr;
	if (!Box)
	{
		UE_LOG(LogTemp, Error, TEXT("Shop delivery box spawn failed; the order remains queued."));
		return false;
	}
	if (!Box->InitializeContents(Orders[0].OrderId, Orders[0].Lines))
	{
		Box->Destroy();
		UE_LOG(LogTemp, Error, TEXT("Shop delivery box rejected its order contents; the order remains queued."));
		return false;
	}
	Box->FinishSpawning(DropTransform);
	FText ActivationFailure;
	if (!IsValid(Box) || !Box->ActivateFreeWorld(DropTransform, ActivationFailure))
	{
		if (IsValid(Box))
		{
			Box->Destroy();
		}
		UE_LOG(LogTemp, Error, TEXT("Shop delivery box activation failed: %s"), *ActivationFailure.ToString());
		return false;
	}
	const int64 DeliveredOrderId = Orders[0].OrderId;
	Orders.RemoveAt(0, 1, EAllowShrinking::No);
	OnOrderDelivered.Broadcast(DeliveredOrderId);
	OnOrdersChanged.Broadcast();
	return true;
}

#undef LOCTEXT_NAMESPACE
