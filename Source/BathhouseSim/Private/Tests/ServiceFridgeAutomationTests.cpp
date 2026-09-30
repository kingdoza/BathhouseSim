#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/ServiceAutomationTestSupport.h"

#include "Components/CapsuleComponent.h"
#include "Facility/BathhouseFacilityPlacementInstanceData.h"
#include "Interaction/HeldEquipmentUsable.h"
#include "Interaction/PhysicalCarryDiscardable.h"
#include "Misc/DataValidation.h"
#include "Placement/FacilityActorConversionTransaction.h"
#include "Placement/PlaceableFacility.h"
#include "Service/DrinkCollectionBoxActor.h"
#include "Service/DrinkSalesSubsystem.h"
#include "Shop/ShopCartComponent.h"
#include "Shop/ShopCatalog.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Shop/ShopDeliveryPointActor.h"
#include "Shop/ShopOrderSubsystem.h"
#include "Shop/ShopSettings.h"

namespace
{
using namespace ServiceTest;

struct FScopedServiceShopSettings
{
	UShopSettings* Settings = GetMutableDefault<UShopSettings>();
	TSoftObjectPtr<UShopCatalog> Catalog = Settings->Catalog;
	TSoftClassPtr<AShopDeliveryBoxActor> DeliveryBoxClass = Settings->DeliveryBoxClass;
	TSoftClassPtr<AItemBoxActor> ItemBoxClass = Settings->ItemBoxClass;
	int32 CartLimit = Settings->CartTotalQuantityLimit;
	float DeliveryDelay = Settings->DeliveryDelaySeconds;

	~FScopedServiceShopSettings()
	{
		Settings->Catalog = Catalog;
		Settings->DeliveryBoxClass = DeliveryBoxClass;
		Settings->ItemBoxClass = ItemBoxClass;
		Settings->CartTotalQuantityLimit = CartLimit;
		Settings->DeliveryDelaySeconds = DeliveryDelay;
	}
};

UShopCatalog* MakeCatalog(
	UFacilityPlacementDefinition* Facility,
	UServiceItemDefinition* BoxKind)
{
	UShopCatalog* Catalog = NewObject<UShopCatalog>(
		GetTransientPackage(),
		MakeUniqueObjectName(GetTransientPackage(), UShopCatalog::StaticClass(), TEXT("ServiceCatalog")));
	FShopProductEntry& Fridge = Catalog->Products.AddDefaulted_GetRef();
	Fridge.ProductId = TEXT("Fridge");
	Fridge.bForSale = true;
	Fridge.DisplayName = FText::FromString(TEXT("냉장고"));
	Fridge.Price = 30000;
	Fridge.PlacementDefinition = Facility;
	FShopProductEntry& Milk = Catalog->Products.AddDefaulted_GetRef();
	Milk.ProductId = TEXT("MilkBox");
	Milk.bForSale = true;
	Milk.DisplayName = FText::FromString(TEXT("바나나우유 박스"));
	Milk.Price = 12000;
	Milk.ItemBoxDefinition = BoxKind;
	return Catalog;
}

EDataValidationResult ValidateCatalog(const UShopCatalog& Catalog)
{
	FDataValidationContext Context;
	return Catalog.IsDataValid(Context);
}

FDisplaySpaceSnapshot MakeSnapshot(const int32 Index, UServiceItemDefinition* Kind, const int32 Count)
{
	FDisplaySpaceSnapshot Snapshot;
	Snapshot.SpaceIndex = Index;
	Snapshot.Kind = Kind;
	Snapshot.Count = Count;
	return Snapshot;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseServiceFridgePayloadTest,
	"BathhouseSim.Service.Fridge.PayloadRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceFridgePayloadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedServiceGrid GridGuard;
	FScopedUtilityLaborWorld Scope(TEXT("ServiceFridgePayloadWorld"));
	UWorld* World = Scope.Get();
	UFacilityPlacementDefinition* Definition = MakeFridgeDefinition();
	if (!TestNotNull(TEXT("Automation world exists"), World) || !TestNotNull(TEXT("Definition exists"), Definition))
	{
		return false;
	}
	UServiceItemDefinition* Milk = MakeKind(TEXT("PayloadMilk"), TEXT("바나나우유"), 12, 2000);
	UServiceItemDefinition* Juice = MakeKind(TEXT("PayloadJuice"), TEXT("주스"), 6, 1500);
	UServiceItemDefinition* Snack = MakeKind(TEXT("PayloadSnack"), TEXT("과자"), 4, 500, false);

	AServiceAutomationFridge* Fridge = SpawnInstalledFridge(*this, World, *Definition, FVector(3000.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Installed fridge exists"), Fridge))
	{
		return false;
	}
	TestEqual(TEXT("Fridge type is DrinkFridge"), Fridge->GetFacilityType(), EBathhouseFacilityType::DrinkFridge);
	TestEqual(TEXT("A freshly installed fridge has no bottles in space A"), Fridge->GetSpaceA()->GetStock().Count, 0);
	TestEqual(TEXT("A freshly installed fridge has no bottles in space B"), Fridge->GetSpaceB()->GetStock().Count, 0);
	TestFalse(TEXT("An empty fridge cannot be reserved"), Fridge->IsAvailableForReservation());

	FText Failure;
	TestTrue(TEXT("Space A imports milk"), Fridge->GetSpaceA()->ImportStock(Milk, 3, Failure));
	TestTrue(TEXT("Space B imports juice"), Fridge->GetSpaceB()->ImportStock(Juice, 2, Failure));
	TestEqual(TEXT("Total stock is five"), Fridge->GetTotalStock(), 5);
	TestFalse(TEXT("A space rejects an over-capacity import"), Fridge->GetSpaceA()->ImportStock(Milk, 4, Failure));
	TestFalse(TEXT("A space rejects a category-refused kind"), Fridge->GetSpaceA()->ImportStock(Snack, 1, Failure));
	TestEqual(TEXT("Rejected imports leave space A alone"), Fridge->GetSpaceA()->GetStock().Count, 3);

	// Real recovery keeps the stock in the payload.
	TestTrue(TEXT("Recovery hold begins on an idle fridge"), Fridge->TryBeginFacilityRecoveryHold(Failure));
	TestFalse(TEXT("Recovery hold blocks new reservations"), Fridge->IsAvailableForReservation());
	Fridge->CancelFacilityRecoveryHold();
	TestTrue(TEXT("Cancelling the hold restores availability"), Fridge->IsAvailableForReservation());
	TestEqual(TEXT("Cancelling the hold keeps the stock"), Fridge->GetTotalStock(), 5);

	TestTrue(TEXT("Second recovery hold begins"), Fridge->TryBeginFacilityRecoveryHold(Failure));
	APlaceableFacilityItemActor* Item = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Fridge, Failure);
	if (!TestNotNull(TEXT("Recovery produces an item"), Item))
	{
		AddError(Failure.ToString());
		return false;
	}
	const UServiceDisplayPlacementData* Data =
		GetDisplayData(Item->GetPlacementPayload());
	if (!TestNotNull(TEXT("Payload carries fridge data"), Data))
	{
		return false;
	}
	TestEqual(TEXT("Payload has one snapshot per space"), Data->Spaces.Num(), 2);
	TestTrue(TEXT("Snapshot A keeps milk 3"), Data->Spaces[0].Kind == Milk && Data->Spaces[0].Count == 3 && Data->Spaces[0].SpaceIndex == 0);
	TestTrue(TEXT("Snapshot B keeps juice 2"), Data->Spaces[1].Kind == Juice && Data->Spaces[1].Count == 2 && Data->Spaces[1].SpaceIndex == 1);
	TestEqual(TEXT("Payload summary lists kinds and totals"),
		Data->GetContentsSummary().ToString(), FString(TEXT("바나나우유 3병, 주스 2병")));
	TestEqual(TEXT("Held summary matches the payload"), Item->GetHeldSummaryText().ToString(),
		FString(TEXT("바나나우유 3병, 주스 2병")));
	TestTrue(TEXT("The item name carries the summary"),
		Item->GetPhysicalCarryDisplayName().ToString().Contains(TEXT("바나나우유 3병, 주스 2병")));

	// Re-install from the recovered payload.
	AServiceAutomationFridge* Reinstalled = SpawnInstalledFridge(
		*this, World, *Definition, FVector(4000.0f, 0.0f, 0.0f), &Item->GetPlacementPayload(), Item);
	if (!TestNotNull(TEXT("Recovered payload reinstalls"), Reinstalled))
	{
		return false;
	}
	TestTrue(TEXT("Reinstalled space A keeps milk 3"),
		Reinstalled->GetSpaceA()->GetStock().Kind == Milk && Reinstalled->GetSpaceA()->GetStock().Count == 3);
	TestTrue(TEXT("Reinstalled space B keeps juice 2"),
		Reinstalled->GetSpaceB()->GetStock().Kind == Juice && Reinstalled->GetSpaceB()->GetStock().Count == 2);

	// Invalid payloads are rejected all-or-nothing on a staged fridge.
	auto TryBadPayload = [&](const TFunction<void(UServiceDisplayPlacementData&)>& Configure, const TCHAR* Label)
	{
		AServiceAutomationFridge* Staged = World->SpawnActorDeferred<AServiceAutomationFridge>(
			AServiceAutomationFridge::StaticClass(),
			FTransform(FRotator::ZeroRotator, FVector(5000.0f, 0.0f, 0.0f)),
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		FText StagedFailure;
		const bool bPrepared = Staged
			&& Staged->GetPlacementForTest()->PrepareForStagedPlacement(*Definition, StagedFailure);
		auto* Base=NewObject<UBathhouseFacilityPlacementInstanceData>(Item);
		Base->FacilityType=EBathhouseFacilityType::DrinkFridge;
		UServiceDisplayPlacementData* Bad = NewObject<UServiceDisplayPlacementData>(Base);
		Bad->Key=TEXT("ServiceDisplay");
		Base->Extensions.Add(Bad);
		Configure(*Bad);
		FFacilityPlacementPayload BadPayload;
		BadPayload.Definition = Definition;
		BadPayload.InstanceData = Base;
		TestTrue(FString::Printf(TEXT("%s: staged fixture prepares"), Label), bPrepared);
		if (bPrepared)
		{
			TestTrue(FString::Printf(TEXT("%s: base import only stores the extension"), Label),
				Staged->ImportPlacementPayload(*Item, BadPayload, StagedFailure));
			Staged->FinishSpawning(FTransform(FRotator::ZeroRotator, FVector(5000.0f, 0.0f, 0.0f)));
			TestFalse(FString::Printf(TEXT("%s: finalize after construction is rejected"), Label),
				Staged->FinalizePlacementPayloadAfterConstruction(StagedFailure));
			TestEqual(TEXT("Rejected payload leaves space A empty"), Staged->GetSpaceA()->GetStock().Count, 0);
			TestEqual(TEXT("Rejected payload leaves space B empty"), Staged->GetSpaceB()->GetStock().Count, 0);
		}
		if (Staged)
		{
			Staged->Destroy();
		}
	};
	TryBadPayload([&](UServiceDisplayPlacementData& Data)
	{
		Data.Spaces = { MakeSnapshot(0, Milk, 2) };
	}, TEXT("Wrong space count"));
	TryBadPayload([&](UServiceDisplayPlacementData& Data)
	{
		Data.Spaces = { MakeSnapshot(0, Milk, 2), MakeSnapshot(5, Juice, 1) };
	}, TEXT("Wrong space index"));
	TryBadPayload([&](UServiceDisplayPlacementData& Data)
	{
		Data.Spaces = { MakeSnapshot(0, Milk, 2), MakeSnapshot(1, Juice, 4) };
	}, TEXT("Over capacity in the second space"));
	TryBadPayload([&](UServiceDisplayPlacementData& Data)
	{
		Data.Spaces = { MakeSnapshot(0, Milk, 2), MakeSnapshot(1, Snack, 1) };
	}, TEXT("Category refused in the second space"));
	TryBadPayload([&](UServiceDisplayPlacementData& Data)
	{
		Data.Spaces = { MakeSnapshot(0, nullptr, 2), MakeSnapshot(1, Juice, 1) };
	}, TEXT("Count without a kind"));

	// A plain facility payload (no fridge data) cannot be imported by a fridge.
	{
		AServiceAutomationFridge* Staged = World->SpawnActorDeferred<AServiceAutomationFridge>(
			AServiceAutomationFridge::StaticClass(),
			FTransform(FRotator::ZeroRotator, FVector(5500.0f, 0.0f, 0.0f)),
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		FText StagedFailure;
		if (TestNotNull(TEXT("Plain-payload fixture spawns"), Staged)
			&& TestTrue(TEXT("Plain-payload fixture prepares"),
				Staged->GetPlacementForTest()->PrepareForStagedPlacement(*Definition, StagedFailure)))
		{
			UBathhouseFacilityPlacementInstanceData* Plain = NewObject<UBathhouseFacilityPlacementInstanceData>(Item);
			Plain->FacilityType = EBathhouseFacilityType::DrinkFridge;
			FFacilityPlacementPayload PlainPayload;
			PlainPayload.Definition = Definition;
			PlainPayload.InstanceData = Plain;
			TestTrue(TEXT("Base import accepts base-only facility data"),
				Staged->ImportPlacementPayload(*Item, PlainPayload, StagedFailure));
			Staged->FinishSpawning(FTransform(FRotator::ZeroRotator, FVector(5500.0f, 0.0f, 0.0f)));
			TestFalse(TEXT("A fridge rejects base-only facility data after construction"),
				Staged->FinalizePlacementPayloadAfterConstruction(StagedFailure));
		}
		if (Staged)
		{
			Staged->Destroy();
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseServiceFridgeCustomerContractTest,
	"BathhouseSim.Service.Fridge.CustomerContractAndRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceFridgeCustomerContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedServiceGrid GridGuard;
	FScopedUtilityLaborWorld Scope(TEXT("ServiceFridgeCustomerWorld"));
	UWorld* World = Scope.Get();
	UFacilityPlacementDefinition* Definition = MakeFridgeDefinition();
	if (!TestNotNull(TEXT("Automation world exists"), World) || !TestNotNull(TEXT("Definition exists"), Definition))
	{
		return false;
	}
	UServiceItemDefinition* Milk = MakeKind(TEXT("CustomerMilk"), TEXT("바나나우유"), 12, 2000);
	UServiceItemDefinition* Juice = MakeKind(TEXT("CustomerJuice"), TEXT("주스"), 6, 1500);
	AServiceAutomationFridge* Fridge = SpawnInstalledFridge(*this, World, *Definition, FVector(3000.0f, 0.0f, 0.0f));
	UDrinkSalesSubsystem* Sales = World->GetSubsystem<UDrinkSalesSubsystem>();
	if (!TestNotNull(TEXT("Installed fridge exists"), Fridge) || !TestNotNull(TEXT("Sales subsystem exists"), Sales))
	{
		return false;
	}
	FText Failure;
	UBathhouseFacilitySlotComponent* Slot = Fridge->GetCustomerSlotForTest();
	AActor* Customer = World->SpawnActor<AActor>();
	AActor* Stranger = World->SpawnActor<AActor>();
	TestFalse(TEXT("Empty fridge is not reservable"), Fridge->IsAvailableForReservation());
	Fridge->GetSpaceA()->ImportStock(Milk, 2, Failure);
	Fridge->GetSpaceB()->ImportStock(Juice, 1, Failure);
	TestTrue(TEXT("Stocked fridge is reservable"), Fridge->IsAvailableForReservation());

	UServiceItemDefinition* Kind = nullptr;
	TestFalse(TEXT("Taking without a reservation fails"), Fridge->TryTakeDrinkForCustomer(*Customer, Kind, Failure));
	TestEqual(TEXT("Nothing was sold yet"), Sales->GetPendingAmount(), 0);

	TestTrue(TEXT("Customer reserves the slot"), Slot->TryReserve(Customer));
	TestFalse(TEXT("A stranger cannot reserve the occupied slot"), Slot->TryReserve(Stranger));
	const FFacilityPlacementTransactionResult ReservedRecovery = Fridge->QueryFacilityRecovery();
	TestFalse(TEXT("A reserved fridge cannot be recovered"), ReservedRecovery.bSucceeded);
	TestEqual(TEXT("Reserved recovery reason"), ReservedRecovery.FailureReason.ToString(),
		FString(TEXT("사용 또는 예약 중인 설비는 회수할 수 없습니다.")));
	TestFalse(TEXT("A stranger cannot take drinks"), Fridge->TryTakeDrinkForCustomer(*Stranger, Kind, Failure));
	TestTrue(TEXT("Customer begins use"), Slot->BeginUse(Customer));

	int32 SpaceChanges = 0;
	FDelegateHandle Handle;
	(void)Handle;
	TestTrue(TEXT("First take succeeds"), Fridge->TryTakeDrinkForCustomer(*Customer, Kind, Failure));
	TestTrue(TEXT("First bottle comes from the first non-empty space"), Kind == Milk);
	TestEqual(TEXT("First sale is recorded"), Sales->GetPendingAmount(), 2000);
	TestEqual(TEXT("Space A lost one bottle"), Fridge->GetSpaceA()->GetStock().Count, 1);
	TestTrue(TEXT("Second take succeeds"), Fridge->TryTakeDrinkForCustomer(*Customer, Kind, Failure) && Kind == Milk);
	TestTrue(TEXT("Emptied space unlocked its kind"), Fridge->GetSpaceA()->GetStock().Kind == nullptr);
	TestTrue(TEXT("Third take comes from the next space"), Fridge->TryTakeDrinkForCustomer(*Customer, Kind, Failure) && Kind == Juice);
	TestEqual(TEXT("Sales add up"), Sales->GetPendingAmount(), 5500);
	TestFalse(TEXT("A fourth take fails on an empty fridge"), Fridge->TryTakeDrinkForCustomer(*Customer, Kind, Failure));
	TestEqual(TEXT("A failed take records no sale"), Sales->GetPendingAmount(), 5500);
	TestEqual(TEXT("Fridge is empty"), Fridge->GetTotalStock(), 0);
	TestTrue(TEXT("Releasing the slot works"), Slot->Release(Customer));
	TestFalse(TEXT("An emptied fridge is not reservable"), Fridge->IsAvailableForReservation());
	(void)SpaceChanges;

	// Hold blocks reservation and taking; cancel restores.
	Fridge->GetSpaceA()->ImportStock(Milk, 1, Failure);
	TestTrue(TEXT("Refilled fridge is reservable"), Fridge->IsAvailableForReservation());
	TestTrue(TEXT("Recovery hold begins"), Fridge->TryBeginFacilityRecoveryHold(Failure));
	TestFalse(TEXT("A held fridge refuses reservations"), Fridge->IsAvailableForReservation());
	Slot->TryReserve(Customer);
	TestFalse(TEXT("A held fridge refuses takes"), Fridge->TryTakeDrinkForCustomer(*Customer, Kind, Failure));
	Slot->ForceRelease();
	Fridge->CancelFacilityRecoveryHold();
	TestTrue(TEXT("Cancelling the hold restores reservations"), Fridge->IsAvailableForReservation());
	TestEqual(TEXT("Cancelling the hold keeps the stock"), Fridge->GetTotalStock(), 1);

	// A destroyed user releases the slot; taken bottles and sales stay.
	AActor* Leaver = World->SpawnActor<AActor>();
	BeginActorPlayIfNeeded(Leaver);
	TestTrue(TEXT("Leaver reserves the slot"), Slot->TryReserve(Leaver));
	TestTrue(TEXT("Leaver begins use"), Slot->BeginUse(Leaver));
	TestTrue(TEXT("Leaver takes the last bottle"), Fridge->TryTakeDrinkForCustomer(*Leaver, Kind, Failure));
	Leaver->Destroy();
	TestEqual(TEXT("Destroying the user frees the slot"), Slot->GetSlotState(), EBathhouseFacilitySlotState::Available);
	TestEqual(TEXT("The sale stays after the user is gone"), Sales->GetPendingAmount(), 7500);

	// A player insert and a customer take on the same fridge each move exactly one bottle.
	FPlayer Player;
	if (!TestTrue(TEXT("Player fixture builds"), BuildPlayer(*this, World, Player)))
	{
		return false;
	}
	AItemBoxActor* Box = SpawnBox(World, Milk, 4);
	TestTrue(TEXT("Player takes a box"), Player.Carry->TryTakePhysicalObject(Box, Failure));
	FPlayerInteractionContext Context;
	Context.Interactor = Player.Pawn;
	Context.CarryComponent = Player.Carry;
	Context.InteractionComponent = Player.Interaction;
	AActor* Shopper = World->SpawnActor<AActor>();
	BeginActorPlayIfNeeded(Shopper);
	TestTrue(TEXT("Shopper uses the fridge"), Slot->TryReserve(Shopper) && Slot->BeginUse(Shopper));
	TestTrue(TEXT("Player inserts while the fridge is in use"),
		Fridge->GetSpaceA()->ExecuteHeldTargetUse(Context, EPlayerHeldTargetUseDirection::Apply).bSucceeded);
	TestTrue(TEXT("Shopper takes one bottle"), Fridge->TryTakeDrinkForCustomer(*Shopper, Kind, Failure));
	TestEqual(TEXT("Exactly one bottle in and one out"), Fridge->GetSpaceA()->GetStock().Count, 0);
	TestEqual(TEXT("Box lost exactly one bottle"), Box->GetContents().Count, 3);
	TestEqual(TEXT("Sales include the shopper's bottle"), Sales->GetPendingAmount(), 9500);
	Slot->Release(Shopper);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseServiceCollectionBoxTest,
	"BathhouseSim.Service.Collection.SalesAndBox",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceCollectionBoxTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedServiceGrid GridGuard;
	FScopedUtilityLaborWorld Scope(TEXT("ServiceCollectionWorld"));
	UWorld* World = Scope.Get();
	UFacilityPlacementDefinition* Definition = MakeFridgeDefinition();
	if (!TestNotNull(TEXT("Automation world exists"), World) || !TestNotNull(TEXT("Definition exists"), Definition))
	{
		return false;
	}
	FPlayer Player;
	if (!TestTrue(TEXT("Player fixture builds"), BuildPlayer(*this, World, Player, true)))
	{
		return false;
	}
	UServiceItemDefinition* Milk = MakeKind(TEXT("CollectMilk"), TEXT("바나나우유"), 12, 2000);
	AServiceAutomationFridge* FridgeA = SpawnInstalledFridge(*this, World, *Definition, FVector(3000.0f, 0.0f, 0.0f));
	AServiceAutomationFridge* FridgeB = SpawnInstalledFridge(*this, World, *Definition, FVector(3500.0f, 0.0f, 0.0f));
	UDrinkSalesSubsystem* Sales = World->GetSubsystem<UDrinkSalesSubsystem>();
	if (!TestNotNull(TEXT("Fridge A exists"), FridgeA) || !TestNotNull(TEXT("Fridge B exists"), FridgeB)
		|| !TestNotNull(TEXT("Sales subsystem exists"), Sales))
	{
		return false;
	}
	FText Failure;
	FridgeA->GetSpaceA()->ImportStock(Milk, 2, Failure);
	FridgeB->GetSpaceA()->ImportStock(Milk, 3, Failure);
	AActor* CustomerA = World->SpawnActor<AActor>();
	AActor* CustomerB = World->SpawnActor<AActor>();
	UServiceItemDefinition* Kind = nullptr;
	TestTrue(TEXT("Customer A uses fridge A"),
		FridgeA->GetCustomerSlotForTest()->TryReserve(CustomerA) && FridgeA->GetCustomerSlotForTest()->BeginUse(CustomerA));
	TestTrue(TEXT("Customer B uses fridge B"),
		FridgeB->GetCustomerSlotForTest()->TryReserve(CustomerB) && FridgeB->GetCustomerSlotForTest()->BeginUse(CustomerB));
	TestTrue(TEXT("A takes one"), FridgeA->TryTakeDrinkForCustomer(*CustomerA, Kind, Failure));
	TestTrue(TEXT("B takes two"),
		FridgeB->TryTakeDrinkForCustomer(*CustomerB, Kind, Failure)
		&& FridgeB->TryTakeDrinkForCustomer(*CustomerB, Kind, Failure));
	TestEqual(TEXT("Two fridges pool into one amount"), Sales->GetPendingAmount(), 6000);

	ADrinkCollectionBoxActor* Collection = World->SpawnActor<ADrinkCollectionBoxActor>();
	if (!TestNotNull(TEXT("Collection box exists"), Collection))
	{
		return false;
	}
	BeginActorPlayIfNeeded(Collection);
	TestNull(TEXT("Collection box is not carryable"), Cast<IPhysicalCarryable>(Collection));
	TestNull(TEXT("Collection box is not placeable"), Cast<IPlaceableFacility>(Collection));
	TestNull(TEXT("Collection box is not discardable"), Cast<IPhysicalCarryDiscardable>(Collection));

	FPlayerInteractionContext Context;
	Context.Interactor = Player.Pawn;
	Context.CarryComponent = Player.Carry;
	Context.InteractionComponent = Player.Interaction;
	UPlayerWalletComponent* Wallet = Player.PlayerState->GetWallet();
	const int32 StartMoney = Wallet->GetCurrentMoney();
	FPlayerInteractionQuery Query = Collection->QueryInteraction(Context);
	TestTrue(TEXT("Collection query is visible and ready"), Query.bVisible && Query.bCanInteract);
	TestEqual(TEXT("Collection target shows the pool"), Query.TargetName.ToString(),
		FText::Format(NSLOCTEXT("Test", "Name", "음료 수거함 {0}원"), FText::AsNumber(6000)).ToString());
	TestEqual(TEXT("Collection action"), Query.ActionName.ToString(), FString(TEXT("수금")));

	TestTrue(TEXT("First E collects"), Collection->ExecuteInteraction(Context).bSucceeded);
	TestEqual(TEXT("The wallet receives the pool once"), Wallet->GetCurrentMoney(), StartMoney + 6000);
	TestEqual(TEXT("The pool is empty"), Sales->GetPendingAmount(), 0);
	const FPlayerInteractionResult Second = Collection->ExecuteInteraction(Context);
	TestFalse(TEXT("An immediate second E pays nothing"), Second.bSucceeded);
	TestEqual(TEXT("Second E states why"), Second.FailureReason.ToString(), FString(TEXT("모인 돈 없음")));
	TestEqual(TEXT("The wallet is unchanged by the second E"), Wallet->GetCurrentMoney(), StartMoney + 6000);
	Query = Collection->QueryInteraction(Context);
	TestFalse(TEXT("An empty pool disables collection"), Query.bCanInteract);
	TestEqual(TEXT("Empty pool reason"), Query.FailureReason.ToString(), FString(TEXT("모인 돈 없음")));

	// Held objects do not matter (J1).
	AItemBoxActor* Box = SpawnBox(World, Milk, 1);
	TestTrue(TEXT("Player takes a box"), Player.Carry->TryTakePhysicalObject(Box, Failure));
	Sales->AddSale(1000);
	TestTrue(TEXT("Collecting works with a box in hand"), Collection->ExecuteInteraction(Context).bSucceeded);
	TestEqual(TEXT("Pool moved with a box in hand"), Wallet->GetCurrentMoney(), StartMoney + 7000);

	// Wallet overflow keeps the pool.
	Sales->AddSale(500);
	const int32 Headroom = Wallet->GetCurrentMoney();
	TestTrue(TEXT("Wallet is filled near its limit"), Wallet->TryAddMoney(MAX_int32 - Headroom - 100));
	const int32 FullMoney = Wallet->GetCurrentMoney();
	Query = Collection->QueryInteraction(Context);
	TestFalse(TEXT("A wallet that cannot hold the pool blocks collection"), Query.bCanInteract);
	TestFalse(TEXT("Execute fails on wallet overflow"), Collection->ExecuteInteraction(Context).bSucceeded);
	TestEqual(TEXT("A failed collection keeps the pool"), Sales->GetPendingAmount(), 500);
	TestEqual(TEXT("A failed collection keeps the wallet"), Wallet->GetCurrentMoney(), FullMoney);

	// Pool saturates instead of overflowing.
	Sales->AddSale(MAX_int32);
	Sales->AddSale(1000);
	TestEqual(TEXT("The pool saturates at MAX_int32"), Sales->GetPendingAmount(), MAX_int32);
	Sales->AddSale(0);
	Sales->AddSale(-5);
	TestEqual(TEXT("Non-positive sales are ignored"), Sales->GetPendingAmount(), MAX_int32);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseServiceShopIntegrationTest,
	"BathhouseSim.Service.Shop.CatalogOrderAndUnboxing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceShopIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedServiceShopSettings SettingsGuard;
	FScopedServiceGrid GridGuard;
	UFacilityPlacementDefinition* Facility = MakeFridgeDefinition();
	if (!TestNotNull(TEXT("Facility definition exists"), Facility))
	{
		return false;
	}
	UServiceItemDefinition* Milk = MakeKind(TEXT("ShopMilk"), TEXT("바나나우유"), 12, 2000);

	// Catalog validation: exactly one definition per product.
	UShopCatalog* Catalog = MakeCatalog(Facility, Milk);
	FText DefinitionFailure;
	TestTrue(TEXT("The fixture facility definition validates"), Facility->ValidateRuntime(DefinitionFailure));
	if (!DefinitionFailure.IsEmpty())
	{
		AddInfo(FString::Printf(TEXT("Facility definition failure: %s"), *DefinitionFailure.ToString()));
	}
	TestTrue(TEXT("The fixture item kind validates"), Milk->ValidateRuntime(DefinitionFailure));
	FDataValidationContext CatalogContext;
	TestEqual(TEXT("A facility product plus a box product validates"),
		Catalog->IsDataValid(CatalogContext), EDataValidationResult::Valid);
	for (const FDataValidationContext::FIssue& Issue : CatalogContext.GetIssues())
	{
		AddInfo(Issue.Message.ToString());
	}
	UShopCatalog* Both = MakeCatalog(Facility, Milk);
	Both->Products[0].ItemBoxDefinition = Milk;
	TestEqual(TEXT("A product with both definitions is invalid"), ValidateCatalog(*Both), EDataValidationResult::Invalid);
	UShopCatalog* Neither = MakeCatalog(Facility, Milk);
	Neither->Products[1].ItemBoxDefinition = nullptr;
	TestEqual(TEXT("A product with no definition is invalid"), ValidateCatalog(*Neither), EDataValidationResult::Invalid);
	UServiceItemDefinition* ZeroCapacity = MakeKind(TEXT("ShopZero"), TEXT("빈"), 0, 1);
	UShopCatalog* Zero = MakeCatalog(Facility, ZeroCapacity);
	TestEqual(TEXT("A zero-capacity box product is invalid"), ValidateCatalog(*Zero), EDataValidationResult::Invalid);
	UFacilityPlacementDefinition* NonDiscardable = MakeFridgeDefinition();
	NonDiscardable->FacilityTags.RemoveTag(TAG_Facility_Discardable);
	UShopCatalog* NoDiscard = MakeCatalog(NonDiscardable, Milk);
	TestEqual(TEXT("A facility product must stay discardable"), ValidateCatalog(*NoDiscard), EDataValidationResult::Invalid);

	SettingsGuard.Settings->Catalog = TSoftObjectPtr<UShopCatalog>(Catalog);
	SettingsGuard.Settings->DeliveryBoxClass = TSoftClassPtr<AShopDeliveryBoxActor>(AShopDeliveryBoxActor::StaticClass());
	SettingsGuard.Settings->ItemBoxClass = TSoftClassPtr<AItemBoxActor>(AItemBoxActor::StaticClass());
	SettingsGuard.Settings->DeliveryDelaySeconds = 0.0f;
	SettingsGuard.Settings->CartTotalQuantityLimit = 10;

	FScopedUtilityLaborWorld Scope(TEXT("ServiceShopWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	FPlayer Player;
	if (!TestTrue(TEXT("Player fixture builds"), BuildPlayer(*this, World, Player, true)))
	{
		return false;
	}
	UCapsuleComponent* Capsule = NewObject<UCapsuleComponent>(Player.Pawn, TEXT("ServiceCapsule"));
	Player.Pawn->AddInstanceComponent(Capsule);
	Capsule->SetupAttachment(Player.Camera);
	Capsule->InitCapsuleSize(34.0f, 88.0f);
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Capsule->RegisterComponent();

	UShopOrderSubsystem* Orders = World->GetSubsystem<UShopOrderSubsystem>();
	AShopDeliveryPointActor* Point = World->SpawnActor<AShopDeliveryPointActor>(
		AShopDeliveryPointActor::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(2000.0f, 0.0f, 0.0f)));
	if (!TestNotNull(TEXT("Order subsystem exists"), Orders) || !TestNotNull(TEXT("Delivery point exists"), Point))
	{
		return false;
	}
	Orders->RegisterDeliveryPoint(Point);
	UShopCartComponent* Cart = Player.PlayerState->GetShopCart();
	UPlayerWalletComponent* Wallet = Player.PlayerState->GetWallet();
	const int32 StartMoney = Wallet->GetCurrentMoney();

	EShopFailureCode CartFailure = EShopFailureCode::None;
	TestTrue(TEXT("A box product is added to the cart"), Cart->TryAdd(TEXT("MilkBox"), CartFailure));
	TestEqual(TEXT("One box product is quantity one"), Cart->GetTotalQuantity(), 1);
	TestTrue(TEXT("The box order is placed"), Orders->TryPlaceOrder(Player.PlayerState, CartFailure));
	TestEqual(TEXT("The box price is charged"), Wallet->GetCurrentMoney(), StartMoney - 12000);

	auto FindDeliveryBox = [World]() -> AShopDeliveryBoxActor*
	{
		for (TActorIterator<AShopDeliveryBoxActor> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	};
	AShopDeliveryBoxActor* DeliveryBox = FindDeliveryBox();
	if (!TestNotNull(TEXT("The order is delivered as a delivery box"), DeliveryBox))
	{
		return false;
	}
	if (!TestEqual(TEXT("The delivery box holds one line"), DeliveryBox->GetContents().Num(), 1))
	{
		return false;
	}
	TestTrue(TEXT("The delivery line is a box product"),
		DeliveryBox->GetContents()[0].ItemBoxDefinition == Milk && DeliveryBox->GetContents()[0].PlacementDefinition == nullptr
		&& DeliveryBox->GetContents()[0].Quantity == 1);

	FText Failure;
	TestTrue(TEXT("Player takes the delivery box"), Player.Carry->TryTakePhysicalObject(DeliveryBox, Failure));
	const FPlayerInteractionResult Unbox = Player.EquipmentUse->BeginEquipmentUse();
	TestTrue(TEXT("Opening the delivery box succeeds"), Unbox.bSucceeded);
	Player.EquipmentUse->EndEquipmentUse();

	TArray<AItemBoxActor*> ItemBoxes;
	for (TActorIterator<AItemBoxActor> It(World); It; ++It)
	{
		ItemBoxes.Add(*It);
	}
	TestEqual(TEXT("Opening yields exactly one item box"), ItemBoxes.Num(), 1);
	if (ItemBoxes.Num() == 1)
	{
		TestTrue(TEXT("The item box is full"), ItemBoxes[0]->GetContents().Kind == Milk && ItemBoxes[0]->GetContents().Count == 12);
		TestTrue(TEXT("The item box is a free-world carryable"), ItemBoxes[0]->IsFreeWorld());
	}
	TestTrue(TEXT("The delivery box is consumed"), !IsValid(DeliveryBox));

	// Mixed order: one facility and two boxes open together, three units in total.
	TestTrue(TEXT("Facility product is added"), Cart->TryAdd(TEXT("Fridge"), CartFailure));
	TestTrue(TEXT("First mixed box is added"), Cart->TryAdd(TEXT("MilkBox"), CartFailure));
	TestTrue(TEXT("Second mixed box is added"), Cart->TryAdd(TEXT("MilkBox"), CartFailure));
	TestEqual(TEXT("Each box counts as one unit"), Cart->GetTotalQuantity(), 3);
	const int32 BeforeMixedMoney = Wallet->GetCurrentMoney();
	TestTrue(TEXT("The mixed order is placed"), Orders->TryPlaceOrder(Player.PlayerState, CartFailure));
	TestEqual(TEXT("The mixed price is charged"), Wallet->GetCurrentMoney(), BeforeMixedMoney - (30000 + 2 * 12000));
	DeliveryBox = FindDeliveryBox();
	if (!TestNotNull(TEXT("The mixed order is delivered"), DeliveryBox))
	{
		return false;
	}
	TestTrue(TEXT("Player takes the mixed delivery box"), Player.Carry->TryTakePhysicalObject(DeliveryBox, Failure));
	TestTrue(TEXT("Opening the mixed box succeeds"), Player.EquipmentUse->BeginEquipmentUse().bSucceeded);
	Player.EquipmentUse->EndEquipmentUse();

	int32 FullBoxes = 0;
	int32 Items = 0;
	for (TActorIterator<AItemBoxActor> It(World); It; ++It)
	{
		FullBoxes += It->GetContents().Count == 12 ? 1 : 0;
	}
	for (TActorIterator<AFacilityPlacementItemAutomationActor> It(World); It; ++It)
	{
		++Items;
	}
	TestEqual(TEXT("Three full item boxes exist after the mixed opening"), FullBoxes, 3);
	TestEqual(TEXT("One facility item came out of the mixed box"), Items, 1);

	// The cart limit applies per unit, boxes included.
	SettingsGuard.Settings->CartTotalQuantityLimit = 2;
	TestTrue(TEXT("First unit fits the limit"), Cart->TryAdd(TEXT("MilkBox"), CartFailure));
	TestTrue(TEXT("Second unit fits the limit"), Cart->TryAdd(TEXT("MilkBox"), CartFailure));
	TestFalse(TEXT("A third unit exceeds the limit"), Cart->TryAdd(TEXT("MilkBox"), CartFailure));
	TestEqual(TEXT("The cart limit is reported"), CartFailure, EShopFailureCode::CartLimit);
	return true;
}

#endif
