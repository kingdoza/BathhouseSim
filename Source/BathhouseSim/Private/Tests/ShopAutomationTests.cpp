#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Cleaning/WetMopActor.h"
#include "Combat/MonkeyWrenchActor.h"
#include "Computer/BathhouseComputerActor.h"
#include "Economy/BathhousePlayerState.h"
#include "Economy/PlayerWalletComponent.h"
#include "Facility/BathWaterUtilityFacilityActor.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interaction/HeldEquipmentUsable.h"
#include "Interaction/InteractionTypes.h"
#include "Interaction/BathhouseKeyActor.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "GameFramework/Character.h"
#include "Misc/CommandLine.h"
#include "Misc/DataValidation.h"
#include "Misc/Parse.h"
#include "Misc/PackageName.h"
#include "Placement/FacilityActorConversionTransaction.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Placement/FacilityPlacementCollisionUtils.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementSettings.h"
#include "Placement/FacilityPlacementZoneActor.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Shop/BathhouseTrashBinActor.h"
#include "Shop/ShopCartComponent.h"
#include "Shop/ShopCatalog.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Shop/ShopDeliveryPointActor.h"
#include "Shop/ShopOrderSubsystem.h"
#include "Shop/ShopSettings.h"
#include "Shop/ShopUnboxingPlacement.h"
#include "Tests/ShopUnboxShapeTestSupport.h"
#include "Tests/FacilityPlacementAutomationTestProbe.h"
#include "Tests/ShopAutomationTestProbe.h"
#include "Towel/TowelBasketActor.h"
#include "Utility/UtilityOperationComponent.h"
#include "Utility/UtilityShovelActor.h"
#include "UI/BathhouseHUD.h"
#include "UI/InteractionPromptWidget.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"

namespace
{
struct FScopedShopSettingsOverride
{
	UShopSettings* Settings = GetMutableDefault<UShopSettings>();
	TSoftObjectPtr<UShopCatalog> Catalog = Settings->Catalog;
	TSoftClassPtr<AShopDeliveryBoxActor> DeliveryBoxClass = Settings->DeliveryBoxClass;
	int32 CartTotalQuantityLimit = Settings->CartTotalQuantityLimit;
	int32 PerProductQuantityLimit = Settings->PerProductQuantityLimit;
	float DeliveryDelaySeconds = Settings->DeliveryDelaySeconds;
	float UnboxOverlapDepthCm = Settings->UnboxOverlapDepthCm;

	~FScopedShopSettingsOverride()
	{
		Settings->Catalog = Catalog;
		Settings->DeliveryBoxClass = DeliveryBoxClass;
		Settings->CartTotalQuantityLimit = CartTotalQuantityLimit;
		Settings->PerProductQuantityLimit = PerProductQuantityLimit;
		Settings->DeliveryDelaySeconds = DeliveryDelaySeconds;
		Settings->UnboxOverlapDepthCm = UnboxOverlapDepthCm;
	}
};

struct FScopedShopFacilityPlacementGridOverride
{
	UFacilityPlacementSettings* Settings = GetMutableDefault<UFacilityPlacementSettings>();
	float SavedGridSizeCm = Settings ? Settings->GridSizeCm : 10.0f;

	FScopedShopFacilityPlacementGridOverride()
	{
		if (Settings)
		{
			Settings->GridSizeCm = 10.0f;
		}
	}

	~FScopedShopFacilityPlacementGridOverride()
	{
		if (Settings)
		{
			Settings->GridSizeCm = SavedGridSizeCm;
		}
	}
};

struct FShopAutomationWorld
{
	FWorldContext* Context = nullptr;
	UWorld* World = nullptr;

	bool Initialize(FAutomationTestBase& Test, const TCHAR* Name)
	{
		if (!GEngine)
		{
			Test.AddError(TEXT("GEngine is required for Shop world automation."));
			return false;
		}
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), Name);
		Context = &GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World)
		{
			GEngine->DestroyWorldContext(World);
			Context = nullptr;
			Test.AddError(TEXT("Failed to create a Shop automation world."));
			return false;
		}
		World->AddToRoot();
		Context->SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return true;
	}

	void Destroy()
	{
		if (World)
		{
			World->DestroyWorld(false);
			if (GEngine) GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
			World = nullptr;
		}
		Context = nullptr;
	}

	~FShopAutomationWorld()
	{
		Destroy();
	}
};

UFacilityPlacementDefinition* MakeShopTestDefinition()
{
	UFacilityPlacementDefinition* Source = LoadObject<UFacilityPlacementDefinition>(
		nullptr,
		TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Shower.DA_FacilityPlacement_Shower"));
	if (!Source)
	{
		return nullptr;
	}
	const FName ObjectName = MakeUniqueObjectName(
		GetTransientPackage(),
		UFacilityPlacementDefinition::StaticClass(),
		TEXT("ShopAutomationShowerDefinition"));
	UFacilityPlacementDefinition* Definition = DuplicateObject<UFacilityPlacementDefinition>(
		Source,
		GetTransientPackage(),
		ObjectName);
	if (!Definition)
	{
		return nullptr;
	}
	Definition->StableId = MakeUniqueObjectName(
		GetTransientPackage(),
		UFacilityPlacementDefinition::StaticClass(),
		TEXT("ShopAutomationShowerId"));
	Definition->PlacedFacilityClass = AFacilityPlacementAutomationActor::StaticClass();
	Definition->RecoveryItemClass = AFacilityPlacementItemAutomationActor::StaticClass();
	Definition->LockerSlotCount = 0;
	Definition->FacilityTags.AddTag(TAG_Facility_Discardable);
	return Definition;
}

UShopCatalog* MakeShopTestCatalog(UFacilityPlacementDefinition* Definition, const int32 Price)
{
	if (!Definition)
	{
		return nullptr;
	}
	const FName ObjectName = MakeUniqueObjectName(
		GetTransientPackage(),
		UShopCatalog::StaticClass(),
		TEXT("ShopAutomationCatalog"));
	UShopCatalog* Catalog = NewObject<UShopCatalog>(GetTransientPackage(), ObjectName);
	FShopProductEntry& Product = Catalog->Products.AddDefaulted_GetRef();
	Product.ProductId = TEXT("Shower");
	Product.bForSale = true;
	Product.DisplayName = NSLOCTEXT("ShopAutomation", "ShowerDisplayName", "샤워기");
	Product.Price = Price;
	Product.PlacementDefinition = Definition;
	return Catalog;
}

void AssignTestCatalog(UShopSettings& Settings, UShopCatalog* Catalog)
{
	Settings.Catalog = TSoftObjectPtr<UShopCatalog>(Catalog);
}

UBlueprint* FindBlueprintInPackage(UPackage* Package)
{
	if (!Package)
	{
		return nullptr;
	}
	UBlueprint* Result = nullptr;
	ForEachObjectWithOuter(Package, [&Result](UObject* Object)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			Result = Blueprint;
		}
	}, EGetObjectsFlags::None);
	return Result;
}

struct FShopBlueprintLoadSpec
{
	const TCHAR* PackagePath;
	const TCHAR* MigrationCheckPackagePath;
	UClass* NativeParent;
};

const FShopBlueprintLoadSpec ShopBlueprintSpecs[] =
{
	{
		TEXT("/Game/Bathhouse/Blueprints/Computer/BP_BathhouseComputer"),
		TEXT("/Game/Developers/MigrationCheck/BP_BathhouseComputer"),
		ABathhouseComputerActor::StaticClass()
	},
	{
		TEXT("/Game/Bathhouse/Blueprints/Game/BP_BathhousePlayerState"),
		TEXT("/Game/Developers/MigrationCheck/BP_BathhousePlayerState"),
		ABathhousePlayerState::StaticClass()
	},
	{
		TEXT("/Game/Bathhouse/Blueprints/Game/BP_BathhouseHUD"),
		TEXT("/Game/Developers/MigrationCheck/BP_BathhouseHUD"),
		ABathhouseHUD::StaticClass()
	},
	{
		TEXT("/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem"),
		TEXT("/Game/Developers/MigrationCheck/BP_PlaceableFacilityItem"),
		APlaceableFacilityItemActor::StaticClass()
	},
	{
		TEXT("/Game/Bathhouse/Blueprints/Shop/BP_ShopDeliveryBox"),
		TEXT("/Game/Developers/MigrationCheck/BP_ShopDeliveryBox"),
		AShopDeliveryBoxActor::StaticClass()
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopWalletAndCartAutomationTest,
	"BathhouseSim.Shop.WalletAndCart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopWalletAndCartAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedShopSettingsOverride SavedSettings;
	FScopedShopFacilityPlacementGridOverride PlacementGridOverride;
	UShopSettings& Settings = *SavedSettings.Settings;
	Settings.CartTotalQuantityLimit = 3;
	Settings.PerProductQuantityLimit = 99;

	FShopAutomationWorld WalletWorld;
	if (!WalletWorld.Initialize(*this, TEXT("ShopWalletAutomationWorld")))
	{
		return false;
	}
	ABathhousePlayerState* WalletOwner = WalletWorld.World->SpawnActor<ABathhousePlayerState>(
		ABathhousePlayerState::StaticClass(), FTransform::Identity);
	TestNotNull(TEXT("Wallet test PlayerState is spawned"), WalletOwner);
	if (!WalletOwner)
	{
		return false;
	}
	UPlayerWalletComponent* Wallet = WalletOwner->GetWallet();
	UShopAutomationTestProbe* Probe = NewObject<UShopAutomationTestProbe>();
	TestNotNull(TEXT("Wallet component is created"), Wallet);
	TestNotNull(TEXT("Wallet test observer is created"), Probe);
	if (!Wallet || !Probe)
	{
		return false;
	}

	const FIntProperty* StartingMoneyProperty = FindFProperty<FIntProperty>(
		UPlayerWalletComponent::StaticClass(),
		TEXT("StartingMoney"));
	TestNotNull(TEXT("Wallet exposes StartingMoney as a native reflected field"), StartingMoneyProperty);
	if (!StartingMoneyProperty)
	{
		return false;
	}
	TestEqual(TEXT("New wallets start with 100000"), StartingMoneyProperty->GetPropertyValue_InContainer(
		GetDefault<UPlayerWalletComponent>()), 100000);
	Probe->Bind(Wallet, nullptr, nullptr);
	TestEqual(TEXT("InitializeComponent applies StartingMoney"), Wallet->GetCurrentMoney(), 100000);
	TestEqual(TEXT("Wallet initialization emits no balance event"), Probe->MoneyChangedCount, 0);

	TestFalse(TEXT("Zero spending is rejected"), Wallet->TrySpendMoney(0));
	TestFalse(TEXT("Negative spending is rejected"), Wallet->TrySpendMoney(-1));
	TestTrue(TEXT("Valid spending succeeds"), Wallet->TrySpendMoney(25000));
	TestEqual(TEXT("Spending changes money once"), Wallet->GetCurrentMoney(), 75000);
	TestEqual(TEXT("Spending publishes one previous/current event"), Probe->MoneyChangedCount, 1);
	TestEqual(TEXT("Spend event previous balance"), Probe->LastPreviousMoney, 100000);
	TestEqual(TEXT("Spend event current balance"), Probe->LastCurrentMoney, 75000);
	TestFalse(TEXT("Spending above balance is rejected"), Wallet->TrySpendMoney(75001));
	TestEqual(TEXT("Rejected spends do not broadcast"), Probe->MoneyChangedCount, 1);
	TestTrue(TEXT("Existing cash addition remains available"), Wallet->TryAddMoney(10000));
	TestEqual(TEXT("Wallet additions and spending share one balance"), Wallet->GetCurrentMoney(), 85000);
	TestEqual(TEXT("Successful additions broadcast once"), Probe->MoneyChangedCount, 2);
	Probe->Unbind();

	UFacilityPlacementDefinition* Definition = MakeShopTestDefinition();
	UShopCatalog* Catalog = MakeShopTestCatalog(Definition, 1000);
	TestNotNull(TEXT("Transient catalog fixture is created"), Catalog);
	if (!Definition || !Catalog)
	{
		return false;
	}
	AssignTestCatalog(Settings, Catalog);
	TestEqual(TEXT("Soft catalog setting resolves its assigned loaded fixture"), Settings.LoadCatalog(), Catalog);

	ABathhousePlayerState* PlayerState = NewObject<ABathhousePlayerState>();
	TestNotNull(TEXT("PlayerState default subobjects are created"), PlayerState);
	if (!PlayerState)
	{
		return false;
	}
	UPlayerWalletComponent* ShopPlayerWallet = PlayerState->GetWallet();
	if (!ShopPlayerWallet) return false;
	TestEqual(TEXT("Shop PlayerState test wallet begins at zero for the unfunded-order case"),
		ShopPlayerWallet->GetCurrentMoney(), 0);
	UShopCartComponent* Cart = PlayerState->GetShopCart();
	UShopAutomationTestProbe* CartProbe = NewObject<UShopAutomationTestProbe>();
	CartProbe->Bind(nullptr, Cart, nullptr);

	EShopFailureCode Failure = EShopFailureCode::None;
	TestTrue(TEXT("The first shower can be added"), Cart->TryAdd(TEXT("Shower"), Failure));
	TestTrue(TEXT("The second shower can be incremented"), Cart->TryIncrement(TEXT("Shower"), Failure));
	TestTrue(TEXT("The third shower can be incremented to the total cart limit"), Cart->TryAdd(TEXT("Shower"), Failure));
	TestFalse(TEXT("The total cart quantity limit is enforced"), Cart->TryAdd(TEXT("Shower"), Failure));
	TestEqual(TEXT("Total cart failure is reported"), Failure, EShopFailureCode::CartLimit);
	TestEqual(TEXT("One line retains all three ordered items"), Cart->GetLines().Num(), 1);
	TestEqual(TEXT("Cart total quantity is three"), Cart->GetTotalQuantity(), 3);
	TestEqual(TEXT("Successful line mutations broadcast once each"), CartProbe->CartChangedCount, 3);

	Settings.CartTotalQuantityLimit = 10;
	Settings.PerProductQuantityLimit = 2;
	TestTrue(TEXT("Decrement reduces the retained line"), Cart->TryDecrement(TEXT("Shower")));
	TestFalse(TEXT("The product-specific quantity limit is enforced"), Cart->TryIncrement(TEXT("Shower"), Failure));
	TestEqual(TEXT("Product limit failure is reported"), Failure, EShopFailureCode::ProductLimit);
	TestEqual(TEXT("Failed increments leave the cart unchanged"), Cart->GetTotalQuantity(), 2);

	int32 Total = 0;
	const int32 ValidPrice = Catalog->Products[0].Price;
	Catalog->Products[0].Price = MAX_int32;
	TestFalse(TEXT("Cart total overflow is rejected"), Cart->CalculateTotalPrice(*Catalog, Total, Failure));
	TestEqual(TEXT("Cart total overflow reports an invalid product"), Failure, EShopFailureCode::InvalidProduct);
	Catalog->Products[0].Price = ValidPrice;
	TestTrue(TEXT("Cart total price is calculated from its catalog snapshot"), Cart->CalculateTotalPrice(*Catalog, Total, Failure));
	TestEqual(TEXT("Two shower line items total 2000"), Total, 2000);
	UShopOrderSubsystem* Orders = NewObject<UShopOrderSubsystem>();
	const FShopPlaceOrderEvaluation Insufficient = Orders->EvaluatePlaceOrder(PlayerState);
	TestFalse(TEXT("Order evaluation rejects an insufficient balance"), Insufficient.bCanOrder);
	TestEqual(TEXT("Insufficient balance has the correct failure code"), Insufficient.Failure, EShopFailureCode::InsufficientMoney);
	TestEqual(TEXT("Shortfall is calculated from the unchanged cart"), Insufficient.ShortfallAmount, 2000);
	EShopFailureCode UnfundedOrderFailure = EShopFailureCode::None;
	TestFalse(TEXT("Insufficient funds reject order before committing"), Orders->TryPlaceOrder(PlayerState, UnfundedOrderFailure));
	TestEqual(TEXT("Unfunded order reports insufficient money"), UnfundedOrderFailure, EShopFailureCode::InsufficientMoney);
	TestEqual(TEXT("Unfunded order leaves the PlayerState wallet unchanged"), PlayerState->GetWallet()->GetCurrentMoney(), 0);
	TestEqual(TEXT("Unfunded order preserves both cart items"), Cart->GetTotalQuantity(), 2);
	TestTrue(TEXT("Funds can be added independently of cart operations"), PlayerState->GetWallet()->TryAddMoney(2000));
	TestTrue(TEXT("Order evaluation passes after the balance is funded"), Orders->EvaluatePlaceOrder(PlayerState).bCanOrder);

	Catalog->Products[0].bForSale = false;
	TestEqual(TEXT("A stopped product cannot be added again"), Cart->EvaluateAdd(TEXT("Shower")), EShopFailureCode::NotForSale);
	TestEqual(TEXT("A stopped catalog product invalidates order evaluation"), Orders->EvaluatePlaceOrder(PlayerState).Failure, EShopFailureCode::InvalidProduct);
	TestEqual(TEXT("Stopping sale does not erase its existing cart line"), Cart->GetTotalQuantity(), 2);
	TestTrue(TEXT("An existing line can be removed explicitly"), Cart->TryRemoveLine(TEXT("Shower")));
	TestTrue(TEXT("Removing the last line empties the cart"), Cart->GetLines().IsEmpty());
	TestEqual(TEXT("Cart remove broadcasts its committed change"), CartProbe->CartChangedCount, 5);
	CartProbe->Unbind();

#if WITH_EDITOR
	UFacilityPlacementDefinition* LockerDefinition = DuplicateObject<UFacilityPlacementDefinition>(
		Definition,
		GetTransientPackage(),
		MakeUniqueObjectName(GetTransientPackage(), UFacilityPlacementDefinition::StaticClass(), TEXT("ShopLockerDefinition")));
	LockerDefinition->LockerSlotCount = 4;
	FDataValidationContext LockerValidation;
	TestEqual(TEXT("Discardable locker definitions fail Data Validation"),
		LockerDefinition->IsDataValid(LockerValidation), EDataValidationResult::Invalid);

	const FShopProductEntry DuplicateProduct = Catalog->Products[0];
	Catalog->Products.Add(DuplicateProduct);
	const int32 SavedCatalogPrice = Catalog->Products[0].Price;
	Catalog->Products[0].Price = 0;
	FDataValidationContext PriceValidation;
	TestEqual(TEXT("Catalog Data Validation rejects a nonpositive price"),
		Catalog->IsDataValid(PriceValidation), EDataValidationResult::Invalid);
	Catalog->Products[0].Price = SavedCatalogPrice;
	Definition->FacilityTags.RemoveTag(TAG_Facility_Discardable);
	FDataValidationContext MissingDiscardTagValidation;
	TestEqual(TEXT("Catalog Data Validation rejects a product without the discard tag"),
		Catalog->IsDataValid(MissingDiscardTagValidation), EDataValidationResult::Invalid);
	Definition->FacilityTags.AddTag(TAG_Facility_Discardable);
	FDataValidationContext DuplicateValidation;
	TestEqual(TEXT("Catalog Data Validation rejects duplicate product identifiers"),
		Catalog->IsDataValid(DuplicateValidation), EDataValidationResult::Invalid);
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopOrderDelayWaitingAndFifoAutomationTest,
	"BathhouseSim.Shop.OrderDelayWaitingAndFIFO",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopOrderDelayWaitingAndFifoAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedShopSettingsOverride SavedSettings;
	FScopedShopFacilityPlacementGridOverride PlacementGridOverride;
	UShopSettings& Settings = *SavedSettings.Settings;
	Settings.DeliveryBoxClass = TSoftClassPtr<AShopDeliveryBoxActor>(AShopDeliveryBoxActor::StaticClass());
	Settings.DeliveryDelaySeconds = 10.0f;
	Settings.CartTotalQuantityLimit = 10;
	Settings.PerProductQuantityLimit = 99;
	UFacilityPlacementDefinition* Definition = MakeShopTestDefinition();
	UShopCatalog* Catalog = MakeShopTestCatalog(Definition, 1000);
	if (!Definition || !Catalog)
	{
		AddError(TEXT("Failed to create valid product definitions for delivery automation."));
		return false;
	}
	AssignTestCatalog(Settings, Catalog);

	FShopAutomationWorld WorldFixture;
	if (!WorldFixture.Initialize(*this, TEXT("ShopOrderAutomationWorld")))
	{
		return false;
	}
	UWorld* World = WorldFixture.World;
	UShopOrderSubsystem* Orders = World->GetSubsystem<UShopOrderSubsystem>();
	AShopDeliveryPointActor* Point = World->SpawnActor<AShopDeliveryPointActor>(
		AShopDeliveryPointActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, FVector::ZeroVector));
	ABathhousePlayerState* PlayerState = World->SpawnActor<ABathhousePlayerState>(
		ABathhousePlayerState::StaticClass(),
		FTransform::Identity);
	TestNotNull(TEXT("World shop subsystem exists"), Orders);
	TestNotNull(TEXT("One delivery point is spawned"), Point);
	TestNotNull(TEXT("World PlayerState is spawned"), PlayerState);
	if (!Orders || !Point || !PlayerState)
	{
		return false;
	}
	Orders->RegisterDeliveryPoint(Point);

	UShopAutomationTestProbe* Probe = NewObject<UShopAutomationTestProbe>();
	Probe->Bind(nullptr, nullptr, Orders);
	UShopCartComponent* Cart = PlayerState->GetShopCart();
	EShopFailureCode Failure = EShopFailureCode::None;
	const bool bFirstOrderAdded = Cart->TryAdd(TEXT("Shower"), Failure);
	TestTrue(TEXT("The first order is added"), bFirstOrderAdded);
	if (!bFirstOrderAdded)
	{
		AddError(FString::Printf(TEXT("First order setup failed with failure code %d."), static_cast<int32>(Failure)));
		return false;
	}
	const bool bFirstOrderPlaced = Orders->TryPlaceOrder(PlayerState, Failure);
	TestTrue(TEXT("The first order is placed"), bFirstOrderPlaced);
	if (!bFirstOrderPlaced)
	{
		AddError(FString::Printf(TEXT("First order setup failed with failure code %d."), static_cast<int32>(Failure)));
		return false;
	}
	const TArray<FShopOrderSnapshot> FirstOrderSnapshots = Orders->GetOrderSnapshots();
	TestEqual(TEXT("Ten-second order remains queued"), FirstOrderSnapshots.Num(), 1);
	if (FirstOrderSnapshots.Num() != 1)
	{
		return false;
	}
	TestTrue(TEXT("Ten-second order snapshot starts at ten seconds"),
		FMath::IsNearlyEqual(FirstOrderSnapshots[0].SecondsRemaining, 10.0f, 0.05f));
	TestTrue(TEXT("Successful order clears the cart"), Cart->GetLines().IsEmpty());
	TestEqual(TEXT("First order charges once"), PlayerState->GetWallet()->GetCurrentMoney(), 99000);
	EShopFailureCode RepeatFailure = EShopFailureCode::None;
	TestFalse(TEXT("A second immediate order call is rejected"), Orders->TryPlaceOrder(PlayerState, RepeatFailure));
	TestEqual(TEXT("Empty cart reports the repeat-call failure"), RepeatFailure, EShopFailureCode::EmptyCart);
	TestEqual(TEXT("Rejected repeat does not charge again"), PlayerState->GetWallet()->GetCurrentMoney(), 99000);
	TestEqual(TEXT("Rejected repeat does not append an order"), Orders->GetOrderSnapshots().Num(), 1);
	TestEqual(TEXT("No box is delivered before the delay elapses"), Probe->OrderDeliveredCount, 0);

	Settings.DeliveryDelaySeconds = 0.0f;
	TestTrue(TEXT("The second order is added"), Cart->TryAdd(TEXT("Shower"), Failure));
	TestTrue(TEXT("The second order is queued behind the unready first"), Orders->TryPlaceOrder(PlayerState, Failure));
	TestEqual(TEXT("FIFO head readiness blocks the later zero-delay order"), Probe->OrderDeliveredCount, 0);

	AActor* LowCeilingActor = World->SpawnActor<AActor>(
		AActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 95.0f)));
	UBoxComponent* LowCeiling = NewObject<UBoxComponent>(LowCeilingActor, TEXT("ShopLowCeiling"));
	LowCeilingActor->SetRootComponent(LowCeiling);
	LowCeilingActor->AddInstanceComponent(LowCeiling);
	LowCeiling->SetBoxExtent(FVector(500.0f, 500.0f, 5.0f));
	LowCeiling->SetCollisionObjectType(ECC_WorldStatic);
	LowCeiling->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LowCeiling->SetCollisionResponseToAllChannels(ECR_Block);
	LowCeiling->RegisterComponent();
	LowCeiling->UpdateComponentToWorld();

	for (int32 TickIndex = 0; TickIndex < 41; ++TickIndex)
	{
		World->Tick(LEVELTICK_All, 0.25f);
	}
	const TArray<FShopOrderSnapshot> WaitingSnapshots = Orders->GetOrderSnapshots();
	TestEqual(TEXT("Both ready orders remain queued while the delivery point is blocked"), WaitingSnapshots.Num(), 2);
	if (WaitingSnapshots.Num() == 2)
	{
		TestTrue(TEXT("Only the blocked FIFO head reports waiting for space"), WaitingSnapshots[0].bWaitingForSpace);
		TestFalse(TEXT("Later orders are not processed past the blocked head"), WaitingSnapshots[1].bWaitingForSpace);
	}
	TestEqual(TEXT("No box is delivered under a ceiling too low for the crate"), Probe->OrderDeliveredCount, 0);

	LowCeilingActor->Destroy();
	World->Tick(LEVELTICK_All, 0.25f);
	TestEqual(TEXT("Removing the blocker releases both ready orders"), Orders->GetOrderSnapshots().Num(), 0);
	TestEqual(TEXT("Both orders deliver in the original FIFO order"), Probe->OrderDeliveredCount, 2);
	if (Probe->DeliveredOrderIds.Num() == 2)
	{
		TestEqual(TEXT("First delivered order id"), Probe->DeliveredOrderIds[0], static_cast<int64>(1));
		TestEqual(TEXT("Second delivered order id"), Probe->DeliveredOrderIds[1], static_cast<int64>(2));
	}
	AShopDeliveryBoxActor* FirstBox = nullptr;
	AShopDeliveryBoxActor* SecondBox = nullptr;
	int32 BoxCount = 0;
	for (TActorIterator<AShopDeliveryBoxActor> It(World); It; ++It)
	{
		++BoxCount;
		if (It->GetOrderId() == 1) FirstBox = *It;
		if (It->GetOrderId() == 2) SecondBox = *It;
	}
	TestEqual(TEXT("One delivery box exists per delivered order"), BoxCount, 2);
	TestNotNull(TEXT("The first order has a delivery box"), FirstBox);
	TestNotNull(TEXT("The second order has a delivery box"), SecondBox);
	if (FirstBox && SecondBox)
	{
		TestTrue(TEXT("Second same-tick delivery stacks above the first"), SecondBox->GetActorLocation().Z > FirstBox->GetActorLocation().Z);
		TestEqual(TEXT("First box retains the shower order summary"), FirstBox->GetContents()[0].DisplayName.ToString(), FString(TEXT("샤워기")));
	}
	TestEqual(TEXT("Two orders spend exactly two line totals"), PlayerState->GetWallet()->GetCurrentMoney(), 98000);
	TestTrue(TEXT("A third order can be added after delivery"), Cart->TryAdd(TEXT("Shower"), Failure));
	TestTrue(TEXT("Zero-delay order is delivered synchronously"), Orders->TryPlaceOrder(PlayerState, Failure));
	TestEqual(TEXT("Zero-delay order immediately publishes delivery"), Probe->OrderDeliveredCount, 3);
	TestEqual(TEXT("Zero-delay order leaves no queued snapshot"), Orders->GetOrderSnapshots().Num(), 0);
	TestEqual(TEXT("All three orders spend only their line totals"), PlayerState->GetWallet()->GetCurrentMoney(), 97000);
	Probe->Unbind();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopFreshInstallTrashAndUnboxingAutomationTest,
	"BathhouseSim.Shop.FreshInstallTrashAndUnboxing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopFreshInstallTrashAndUnboxingAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedShopFacilityPlacementGridOverride PlacementGridOverride;
	UFacilityPlacementDefinition* Definition = MakeShopTestDefinition();
	if (!Definition)
	{
		AddError(TEXT("Failed to load the Shower placement definition test fixture."));
		return false;
	}

	FShopAutomationWorld WorldFixture;
	if (!WorldFixture.Initialize(*this, TEXT("ShopTrashAutomationWorld")))
	{
		return false;
	}
	UWorld* World = WorldFixture.World;

	FText FailureReason;
	UFacilityPlacementDefinition* BoilerSource = LoadObject<UFacilityPlacementDefinition>(
		nullptr,
		TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler.DA_FacilityPlacement_Boiler"));
	TestNotNull(TEXT("Boiler placement definition loads for the fresh-install precheck"), BoilerSource);
	if (!BoilerSource)
	{
		return false;
	}
	UFacilityPlacementDefinition* BoilerDefinition = DuplicateObject<UFacilityPlacementDefinition>(
		BoilerSource,
		GetTransientPackage(),
		MakeUniqueObjectName(GetTransientPackage(), UFacilityPlacementDefinition::StaticClass(), TEXT("ShopFreshBoilerDefinition")));
	BoilerDefinition->StableId = MakeUniqueObjectName(
		GetTransientPackage(), UFacilityPlacementDefinition::StaticClass(), TEXT("ShopFreshBoilerId"));
	BoilerDefinition->FacilityTags.AddTag(TAG_Facility_Discardable);
	TestTrue(TEXT("Tagged boiler definition is runtime-valid"), BoilerDefinition->ValidateRuntime(FailureReason));
	APlaceableFacilityItemActor* BoilerFreshItem = APlaceableFacilityItemActor::SpawnFreshItem(
		*World,
		*BoilerDefinition,
		FTransform(FRotator::ZeroRotator, FVector(15000.0f, 0.0f, 300.0f)),
		FailureReason);
	TestNotNull(TEXT("Fresh boiler item is created"), BoilerFreshItem);
	if (!BoilerFreshItem)
	{
		AddError(FailureReason.ToString());
		return false;
	}
	const FTransform BoilerTransform(FRotator::ZeroRotator, FVector(16000.0f, 0.0f, 0.0f));
	AActor* BoilerActorObject = World->SpawnActorDeferred<AActor>(
		BoilerDefinition->PlacedFacilityClass.Get(), BoilerTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	ABathWaterUtilityFacilityActor* BoilerActor = Cast<ABathWaterUtilityFacilityActor>(BoilerActorObject);
	TestNotNull(TEXT("Boiler class creates a utility facility actor"), BoilerActor);
	if (!BoilerActor || !BoilerActor->GetFacilityPlacementComponent()
		|| !BoilerActor->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*BoilerDefinition, FailureReason))
	{
		if (IsValid(BoilerActorObject)) BoilerActorObject->Destroy();
		AddError(FailureReason.ToString());
		return false;
	}
	TestTrue(TEXT("Fresh boiler payload imports through the utility domain"),
		BoilerActor->ImportPlacementPayload(*BoilerFreshItem, BoilerFreshItem->GetPlacementPayload(), FailureReason));
	TestNotNull(TEXT("Fresh boiler owns its operation component"), BoilerActor->GetUtilityOperation());
	if (BoilerActor->GetUtilityOperation())
	{
		TestEqual(TEXT("Fresh boiler imports with zero remaining operation points"),
			BoilerActor->GetUtilityOperation()->GetRemainingPoints(), 0.0f);
	}
	BoilerActor->FinishSpawning(BoilerTransform);
	TestTrue(TEXT("Boiler remains at zero operation points after actor initialization"),
		BoilerActor->GetUtilityOperation()
		&& FMath::IsNearlyZero(BoilerActor->GetUtilityOperation()->GetRemainingPoints()));
	BoilerActor->Destroy();
	APlaceableFacilityItemActor* FreshItem = APlaceableFacilityItemActor::SpawnFreshItem(
		*World,
		*Definition,
		FTransform(FRotator::ZeroRotator, FVector(3000.0f, 0.0f, 200.0f)),
		FailureReason);
	TestNotNull(TEXT("Fresh-install factory creates a staged shower item"), FreshItem);
	if (!FreshItem)
	{
		AddError(FailureReason.ToString());
		return false;
	}
	TestTrue(TEXT("Fresh-install payload is distinguished by a null InstanceData"), FreshItem->GetPlacementPayload().IsFreshInstall());
	TestNull(TEXT("Fresh-install factory leaves domain instance data null"), FreshItem->GetPlacementPayload().InstanceData.Get());
	TestTrue(TEXT("Fresh-install item can enter the free world"),
		FreshItem->ActivateFreeWorld(FreshItem->GetActorTransform(), FailureReason));
	UPrimitiveComponent* FreshItemPrimitive = FreshItem->GetPhysicalCarryPrimitive();
	TestTrue(TEXT("Free-world facility items ignore Pawn collision"), FreshItemPrimitive
		&& FreshItemPrimitive->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);

	ACharacter* Player = World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(),
		FTransform(FRotator::ZeroRotator, FVector(5000.0f, 0.0f, 1000.0f)));
	TestNotNull(TEXT("Unboxing player character is spawned"), Player);
	if (!Player)
	{
		return false;
	}
	AShopDeliveryBoxActor* BoxForQuery = World->SpawnActor<AShopDeliveryBoxActor>(
		AShopDeliveryBoxActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, FVector(8000.0f, 0.0f, 1000.0f)));
	TestNotNull(TEXT("Unboxing query box is spawned"), BoxForQuery);
	if (!BoxForQuery)
	{
		return false;
	}

	UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector FootLocation = Player->GetActorLocation() - FVector::UpVector * HalfHeight;
	TArray<UFacilityPlacementDefinition*> Definitions = { Definition };
	TArray<FTransform> SpawnTransforms;
	FRandomStream RandomStream(1001);
	const FShopUnboxingTuning FloorStageTuning = ShopUnboxTest::MakeFloorStageTuning(
		ShopUnboxTest::MakeTuning().ForwardDistanceCm, ShopUnboxTest::MakeTuning().OverlapDepthCm);
	{
		// Open floor with the default view stage: the cluster is placed in front of the camera.
		TArray<FTransform> ViewTransforms;
		EShopUnboxPlacementStage ViewStage = EShopUnboxPlacementStage::None;
		FRandomStream ViewStream(1000);
		TestTrue(TEXT("Open floor with the settings tuning places in front of the camera"),
			FShopUnboxingPlacement::FindSpawnTransforms(
				*World, *Player, *Capsule, *BoxForQuery, ShopUnboxTest::MakeShapes(Definitions),
				ShopUnboxTest::MakeRequest(*Player), ShopUnboxTest::MakeTuning(), ViewStream, ViewTransforms,
				FailureReason, &ViewStage));
		TestTrue(TEXT("Open floor selects the view-front stage"), ViewStage == EShopUnboxPlacementStage::ViewFront);
	}
	TestTrue(TEXT("Open floor chooses the configured forward row"),
		FShopUnboxingPlacement::FindSpawnTransforms(
		*World,
		*Player,
		*Capsule,
		*BoxForQuery,
		ShopUnboxTest::MakeShapes(Definitions),
		ShopUnboxTest::MakeRequest(*Player),
		FloorStageTuning,
		RandomStream,
		SpawnTransforms,
		FailureReason));
	TestEqual(TEXT("Open floor returns one spawn transform"), SpawnTransforms.Num(), 1);
	if (SpawnTransforms.Num() == 1)
	{
		FVector ShapeCenter = FVector::ZeroVector;
		FQuat ShapeRotation = FQuat::Identity;
		FCollisionShape Shape;
		const UPrimitiveComponent* CollisionTemplate = nullptr;
		TestTrue(TEXT("Open-floor spawn uses the definition collision query"),
			APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
				*Definition, SpawnTransforms[0], ShapeCenter, ShapeRotation, Shape, CollisionTemplate, FailureReason));
		TestTrue(TEXT("Open floor aligns cluster bounds center at the configured forward position"),
			FMath::IsNearlyEqual(ShapeCenter.X, FootLocation.X + FloorStageTuning.ForwardDistanceCm, 0.1f));
		TestTrue(TEXT("Open floor keeps the lowest shape surface at the configured height above the feet"),
			FMath::IsNearlyEqual(ShapeCenter.Z - Shape.GetExtent().Z, FootLocation.Z + FloorStageTuning.ForwardFloorClearanceCm, 0.1f));

		APlaceableFacilityItemActor* SpawnedOpenFloorItem = APlaceableFacilityItemActor::SpawnFreshItem(
			*World, *Definition, SpawnTransforms[0], FailureReason);
		TestNotNull(TEXT("Open-floor candidate spawns as a real facility item"), SpawnedOpenFloorItem);
		if (SpawnedOpenFloorItem)
		{
			TestTrue(TEXT("Open-floor item activates in the world"),
				SpawnedOpenFloorItem->ActivateFreeWorld(SpawnedOpenFloorItem->GetActorTransform(), FailureReason));
			UStaticMeshComponent* ItemRoot = SpawnedOpenFloorItem->GetItemRoot();
			UStaticMesh* ItemMesh = ItemRoot ? ItemRoot->GetStaticMesh() : nullptr;
			TestNotNull(TEXT("Spawned item has its collision mesh"), ItemMesh);
			if (ItemRoot && ItemMesh)
			{
				const FBoxSphereBounds MeshBounds = ItemMesh->GetBounds();
				const FVector ActualBoundsCenter = ItemRoot->GetComponentTransform().TransformPosition(MeshBounds.Origin);
				const FVector ActualBoundsExtent = MeshBounds.BoxExtent * ItemRoot->GetComponentScale().GetAbs();
				const float ActualCollisionBottom = ActualBoundsCenter.Z - ActualBoundsExtent.Z;
				TestTrue(TEXT("Spawned actor preserves the definition item scale"),
					SpawnedOpenFloorItem->GetActorScale3D().Equals(SpawnTransforms[0].GetScale3D(), 1.0e-3f));
				TestTrue(TEXT("Spawned actor collision bottom stays 20 cm above the feet"),
					FMath::IsNearlyEqual(ActualCollisionBottom, FootLocation.Z + FloorStageTuning.ForwardFloorClearanceCm, 0.5f));
			}
			SpawnedOpenFloorItem->Destroy();
		}
	}

	auto SpawnBlockingBox = [World](const TCHAR* Name, const FVector& Center, const FVector& HalfExtent) -> UBoxComponent*
	{
		AActor* Obstacle = World->SpawnActor<AActor>(
			AActor::StaticClass(), FTransform::Identity);
		if (!Obstacle) return nullptr;
		UBoxComponent* Collision = NewObject<UBoxComponent>(Obstacle, Name);
		Obstacle->SetRootComponent(Collision);
		Obstacle->AddInstanceComponent(Collision);
		Collision->SetBoxExtent(HalfExtent);
		Collision->SetCollisionObjectType(ECC_WorldStatic);
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Collision->SetCollisionResponseToAllChannels(ECR_Block);
		Collision->RegisterComponent();
		Obstacle->SetActorLocation(Center, false, nullptr, ETeleportType::TeleportPhysics);
		Collision->UpdateComponentToWorld();
		return Collision;
	};

	const FVector PlayerCenter = Capsule->GetComponentLocation();
	FVector ItemCollisionLocation = FVector::ZeroVector;
	FQuat ItemCollisionRotation = FQuat::Identity;
	FCollisionShape ItemCollisionShape;
	const UPrimitiveComponent* ItemCollisionTemplate = nullptr;
	FVector ItemDefaultScale = FVector::OneVector;
	TestTrue(TEXT("Definition item scale comes from its default root"),
		APlaceableFacilityItemActor::GetDefinitionItemScale(
			*Definition, ItemDefaultScale, FailureReason));
	const FTransform ItemQueryTransform(
		FRotator::ZeroRotator,
		FVector::ZeroVector,
		ItemDefaultScale);
	TestTrue(TEXT("Definition collision dimensions are queryable at the recovery item default scale"),
		APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
			*Definition,
			ItemQueryTransform,
			ItemCollisionLocation,
			ItemCollisionRotation,
			ItemCollisionShape,
			ItemCollisionTemplate,
			FailureReason));
	const FVector ItemHalfExtent = ItemCollisionShape.GetExtent().GetAbs();
	const FVector WallCenter(PlayerCenter.X + 0.5f * FloorStageTuning.ForwardDistanceCm, PlayerCenter.Y, PlayerCenter.Z);
	UBoxComponent* Wall = SpawnBlockingBox(TEXT("ShopUnboxWall"), WallCenter, FVector(10.0f, 200.0f, 150.0f));
	TestNotNull(TEXT("Fifty-centimetre wall blocker is created"), Wall);
	RandomStream.Initialize(1002);
	TestTrue(TEXT("A wall in front keeps the item on the near side"),
		FShopUnboxingPlacement::FindSpawnTransforms(
		*World,
		*Player,
		*Capsule,
		*BoxForQuery,
		ShopUnboxTest::MakeShapes(Definitions),
		ShopUnboxTest::MakeRequest(*Player),
		FloorStageTuning,
		RandomStream,
		SpawnTransforms,
		FailureReason));
	if (SpawnTransforms.Num() == 1 && Wall)
	{
		TestTrue(TEXT("Wall candidate uses the definition collision query"),
			APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
				*Definition, SpawnTransforms[0], ItemCollisionLocation, ItemCollisionRotation,
				ItemCollisionShape, ItemCollisionTemplate, FailureReason));
		FCollisionQueryParams WallClearanceParams(SCENE_QUERY_STAT(ShopUnboxWallClearanceAssertion), false);
		WallClearanceParams.AddIgnoredActor(Player);
		WallClearanceParams.AddIgnoredActor(BoxForQuery);
		const FVector WallCandidateHalfExtent = ItemCollisionShape.GetExtent().GetAbs();
		const FVector WallClearanceCenter = ItemCollisionLocation + FVector(0.0f, 0.0f, FloorStageTuning.OverlapDepthCm * 0.5f);
		const FVector WallClearanceExtent(
			WallCandidateHalfExtent.X + FloorStageTuning.OverlapDepthCm,
			WallCandidateHalfExtent.Y + FloorStageTuning.OverlapDepthCm,
			WallCandidateHalfExtent.Z + FloorStageTuning.OverlapDepthCm * 0.5f);
		TestFalse(TEXT("Wall candidate clears blocking geometry with horizontal and upper 8 cm clearance"),
			FacilityPlacementCollision::HasBlockingOverlap(
				*World,
				WallClearanceCenter,
				ItemCollisionRotation,
				FCollisionShape::MakeBox(WallClearanceExtent),
				*ItemCollisionTemplate,
				WallClearanceParams));
	}

	if (Wall && Wall->GetOwner()) Wall->GetOwner()->Destroy();
	const FVector BlockedRowCenter(
		FootLocation.X,
		FootLocation.Y,
		FootLocation.Z + ItemHalfExtent.Z + FloorStageTuning.ForwardFloorClearanceCm);
	UBoxComponent* FloorBlocker = SpawnBlockingBox(
		TEXT("ShopUnboxFloorBlocker"),
		BlockedRowCenter,
		FVector(150.0f, 80.0f, ItemHalfExtent.Z + 5.0f));
	TestNotNull(TEXT("Ground-level forward candidates are blocked"), FloorBlocker);
	RandomStream.Initialize(1003);
	TestTrue(TEXT("Blocked forward row safely stacks above the capsule"),
		FShopUnboxingPlacement::FindSpawnTransforms(
		*World,
		*Player,
		*Capsule,
		*BoxForQuery,
		ShopUnboxTest::MakeShapes(Definitions),
		ShopUnboxTest::MakeRequest(*Player),
		FloorStageTuning,
		RandomStream,
		SpawnTransforms,
		FailureReason));
	if (SpawnTransforms.Num() == 1)
	{
		const float CapsuleTopZ = PlayerCenter.Z + Capsule->GetScaledCapsuleHalfHeight();
		TestTrue(TEXT("Vertical fallback shape uses the definition collision query"),
			APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
				*Definition, SpawnTransforms[0], ItemCollisionLocation, ItemCollisionRotation,
				ItemCollisionShape, ItemCollisionTemplate, FailureReason));
		TestTrue(TEXT("Safe vertical fallback is above the player capsule"),
			ItemCollisionLocation.Z - ItemCollisionShape.GetExtent().Z > CapsuleTopZ);
	}

	if (FloorBlocker && FloorBlocker->GetOwner()) FloorBlocker->GetOwner()->Destroy();
	const float CageHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float FrontCageOffset = ItemHalfExtent.X + FloorStageTuning.OverlapDepthCm * 0.5f;
	const float SideCageOffset = ItemHalfExtent.Y + FloorStageTuning.OverlapDepthCm * 0.5f;
	const FVector FrontCageHalfExtent(5.0f, ItemHalfExtent.Y + FloorStageTuning.OverlapDepthCm, CageHalfHeight);
	const FVector SideCageHalfExtent(FloorStageTuning.ForwardDistanceCm + ItemHalfExtent.X + FloorStageTuning.OverlapDepthCm, 5.0f, CageHalfHeight);
	UBoxComponent* FrontCage = SpawnBlockingBox(TEXT("ShopUnboxFrontCage"),
		PlayerCenter + FVector(FrontCageOffset, 0.0f, 0.0f), FrontCageHalfExtent);
	UBoxComponent* BackCage = SpawnBlockingBox(TEXT("ShopUnboxBackCage"),
		PlayerCenter - FVector(FrontCageOffset, 0.0f, 0.0f), FrontCageHalfExtent);
	UBoxComponent* LeftCage = SpawnBlockingBox(TEXT("ShopUnboxLeftCage"),
		PlayerCenter + FVector(0.0f, SideCageOffset, 0.0f), SideCageHalfExtent);
	UBoxComponent* RightCage = SpawnBlockingBox(TEXT("ShopUnboxRightCage"),
		PlayerCenter - FVector(0.0f, SideCageOffset, 0.0f), SideCageHalfExtent);
	TestNotNull(TEXT("Front cage blocker exists"), FrontCage);
	TestNotNull(TEXT("Back cage blocker exists"), BackCage);
	TestNotNull(TEXT("Left cage blocker exists"), LeftCage);
	TestNotNull(TEXT("Right cage blocker exists"), RightCage);
	RandomStream.Initialize(1004);
	TestTrue(TEXT("A four-sided blocked corner stacks items above the capsule safely"),
		FShopUnboxingPlacement::FindSpawnTransforms(
		*World,
		*Player,
		*Capsule,
		*BoxForQuery,
		ShopUnboxTest::MakeShapes(Definitions),
		ShopUnboxTest::MakeRequest(*Player),
		FloorStageTuning,
		RandomStream,
		SpawnTransforms,
		FailureReason));
	if (SpawnTransforms.Num() == 1)
	{
		const float CapsuleTopZ = PlayerCenter.Z + CageHalfHeight;
		TestTrue(TEXT("Caged fallback shape uses the definition collision query"),
			APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
				*Definition, SpawnTransforms[0], ItemCollisionLocation, ItemCollisionRotation,
				ItemCollisionShape, ItemCollisionTemplate, FailureReason));
		const float ItemBottomZ = ItemCollisionLocation.Z - ItemCollisionShape.GetExtent().Z;
		TestTrue(TEXT("Four-sided corner fallback is entirely above the capsule"), ItemBottomZ > CapsuleTopZ);
	}
	for (UBoxComponent* Cage : { FrontCage, BackCage, LeftCage, RightCage })
	{
		if (Cage && Cage->GetOwner()) Cage->GetOwner()->Destroy();
	}

	USceneComponent* HeldAnchor = NewObject<USceneComponent>(Player, TEXT("ShopAutomationHeldAnchor"));
	HeldAnchor->SetupAttachment(Player->GetRootComponent());
	Player->AddInstanceComponent(HeldAnchor);
	HeldAnchor->RegisterComponent();
	UPlayerCarryComponent* Carry = NewObject<UPlayerCarryComponent>(Player, TEXT("ShopAutomationCarry"));
	Player->AddInstanceComponent(Carry);
	Carry->RegisterComponent();
	Carry->ConfigureHeldAnchor(HeldAnchor);

	AFacilityPlacementZoneAutomationActor* PlacementZone =
		World->SpawnActor<AFacilityPlacementZoneAutomationActor>(
			AFacilityPlacementZoneAutomationActor::StaticClass(),
			FTransform(FRotator::ZeroRotator, FVector(12000.0f, 0.0f, 0.0f)));
	TestNotNull(TEXT("Fresh installation placement zone is created"), PlacementZone);
	if (!PlacementZone)
	{
		return false;
	}
	for (const FGameplayTag& Tag : Definition->FacilityTags)
	{
		PlacementZone->AddAllowedTag(Tag);
	}
	PlacementZone->GetZoneBounds()->SetBoxExtent(FVector(500.0f, 500.0f, 500.0f));

	APlaceableFacilityItemActor* PlacementItem = APlaceableFacilityItemActor::SpawnFreshItem(
		*World,
		*Definition,
		FTransform(FRotator::ZeroRotator, FVector(11000.0f, 0.0f, 300.0f)),
		FailureReason);
	TestNotNull(TEXT("Fresh installation item is staged for placement"), PlacementItem);
	if (PlacementItem)
	{
		TestTrue(TEXT("Fresh placement item activates"), PlacementItem->ActivateFreeWorld(PlacementItem->GetActorTransform(), FailureReason));
		TestTrue(TEXT("Fresh placement item is held"), Carry->TryTakePhysicalObject(PlacementItem, FailureReason));
		const FTransform PlacementTransform(FRotator::ZeroRotator, FVector(12000.0f, 0.0f, 100.0f));
		AActor* FirstPlacedActor = FFacilityActorConversionTransaction::PlaceItemAsFacility(
			*PlacementItem, PlacementTransform, *PlacementZone, *Carry, FailureReason);
		AFacilityPlacementAutomationActor* FirstShower = Cast<AFacilityPlacementAutomationActor>(FirstPlacedActor);
		TestNotNull(TEXT("Fresh install uses the existing placement transaction"), FirstShower);
		if (FirstShower)
		{
			TestEqual(TEXT("Fresh shower keeps the default unnumbered value"), FirstShower->GetFacilityNumber(), INDEX_NONE);
			TestTrue(TEXT("Fresh shower keeps default selection weight one"),
				FMath::IsNearlyEqual(FirstShower->GetSelectionWeight(), 1.0f));
			APlaceableFacilityItemActor* RecoveredItem =
				FFacilityActorConversionTransaction::RecoverFacilityToItem(*FirstShower, FailureReason);
			TestNotNull(TEXT("Freshly placed shower can be recovered"), RecoveredItem);
			if (RecoveredItem)
			{
				TestFalse(TEXT("Recovery exports domain instance data after the first placement"),
					RecoveredItem->GetPlacementPayload().IsFreshInstall());
				TestTrue(TEXT("Recovered fresh shower can be held again"),
					Carry->TryTakePhysicalObject(RecoveredItem, FailureReason));
				AActor* ReplacedActor = FFacilityActorConversionTransaction::PlaceItemAsFacility(
					*RecoveredItem, PlacementTransform, *PlacementZone, *Carry, FailureReason);
				AFacilityPlacementAutomationActor* ReplacedShower = Cast<AFacilityPlacementAutomationActor>(ReplacedActor);
				TestNotNull(TEXT("Recovered shower can be placed again"), ReplacedShower);
				if (ReplacedShower)
				{
					TestEqual(TEXT("Replaced shower retains its default unnumbered value"), ReplacedShower->GetFacilityNumber(), INDEX_NONE);
					TestTrue(TEXT("Replaced shower retains selection weight one"),
						FMath::IsNearlyEqual(ReplacedShower->GetSelectionWeight(), 1.0f));
					ReplacedShower->Destroy();
				}
			}
		}
	}

	TestNull(TEXT("Keys do not implement the discard contract"),
		Cast<IPhysicalCarryDiscardable>(GetDefault<ABathhouseKeyActor>()));
	TestNull(TEXT("Wet mops do not implement the discard contract"),
		Cast<IPhysicalCarryDiscardable>(GetDefault<AWetMopActor>()));
	TestNull(TEXT("Monkey wrenches do not implement the discard contract"),
		Cast<IPhysicalCarryDiscardable>(GetDefault<AMonkeyWrenchActor>()));
	TestNull(TEXT("Towel baskets do not implement the discard contract"),
		Cast<IPhysicalCarryDiscardable>(GetDefault<ATowelBasketActor>()));
	TestNull(TEXT("Shovels do not implement the discard contract"),
		Cast<IPhysicalCarryDiscardable>(GetDefault<AUtilityShovelActor>()));
	ABathhouseTrashBinActor* TrashBin = World->SpawnActor<ABathhouseTrashBinActor>();
	TestNotNull(TEXT("Trash bin is spawned"), TrashBin);
	if (!TrashBin)
	{
		return false;
	}
	TestTrue(TEXT("Boiler item can enter free world for discard coverage"),
		BoilerFreshItem->ActivateFreeWorld(BoilerFreshItem->GetActorTransform(), FailureReason));
	TestTrue(TEXT("Tagged boiler item can be held for trash coverage"),
		Carry->TryTakePhysicalObject(BoilerFreshItem, FailureReason));
	FPlayerInteractionContext BoilerTrashContext;
	BoilerTrashContext.Interactor = Player;
	BoilerTrashContext.CarryComponent = Carry;
	BoilerTrashContext.HitActor = TrashBin;
	TestTrue(TEXT("Tagged boiler item is discardable independent of sale state"),
		TrashBin->QueryInteraction(BoilerTrashContext).bCanInteract);
	TestTrue(TEXT("Tagged boiler item can be discarded"),
		TrashBin->ExecuteInteraction(BoilerTrashContext).bSucceeded);
	TestFalse(TEXT("Discarded boiler item is destroyed"), IsValid(BoilerFreshItem));
	FPlayerInteractionContext Context;
	Context.Interactor = Player;
	Context.CarryComponent = Carry;
	Context.HitActor = TrashBin;
	FPlayerInteractionQuery Query = TrashBin->QueryInteraction(Context);
	TestFalse(TEXT("Empty hand cannot use the trash bin"), Query.bCanInteract);
	TestEqual(TEXT("Empty hand receives the no-item prompt"), Query.ActionName.ToString(), FString(TEXT("버릴 물건이 없습니다")));

	UShopAutomationTestProbe* CarryProbe = NewObject<UShopAutomationTestProbe>();
	CarryProbe->BindCarry(Carry);
	TestTrue(TEXT("Fresh shower item can be taken"), Carry->TryTakePhysicalObject(FreshItem, FailureReason));
	TestTrue(TEXT("Tagged non-locker shower is discardable"),
		TrashBin->QueryInteraction(Context).bCanInteract);
	CarryProbe->ResetHeldChanges();
	TestFalse(TEXT("Failed domain commit is rejected"),
		Carry->CommitConsumeHeldObject(FreshItem, []() { return false; }));
	TestTrue(TEXT("Failed domain commit restores the original held item"), Carry->GetHeldObject() == FreshItem);
	TestEqual(TEXT("Failed domain commit does not publish a held-object change"), CarryProbe->HeldObjectChangedCount, 0);
	const FPlayerInteractionResult DiscardResult = TrashBin->ExecuteInteraction(Context);
	TestTrue(TEXT("Trash-bin discard succeeds after a fresh query"), DiscardResult.bSucceeded);
	TestTrue(TEXT("Successful consumption leaves the hand empty"), Carry->IsHandEmpty());
	TestEqual(TEXT("Successful consumption publishes exactly one held-object change"), CarryProbe->HeldObjectChangedCount, 1);
	TestFalse(TEXT("Discarded fresh item is destroyed"), IsValid(FreshItem));

	for (const int32 LockerSlots : { 1, 4, 8 })
	{
		UFacilityPlacementDefinition* LockerDefinition = DuplicateObject<UFacilityPlacementDefinition>(
			Definition,
			GetTransientPackage(),
			MakeUniqueObjectName(GetTransientPackage(), UFacilityPlacementDefinition::StaticClass(), TEXT("ShopLockerTrashDefinition")));
		LockerDefinition->StableId = MakeUniqueObjectName(
			GetTransientPackage(), UFacilityPlacementDefinition::StaticClass(), TEXT("ShopLockerTrashId"));
		LockerDefinition->LockerSlotCount = LockerSlots;
		FDataValidationContext LockerValidation;
		TestEqual(FString::Printf(TEXT("LockerSlotCount %d with discard tag fails Definition Data Validation"), LockerSlots),
			LockerDefinition->IsDataValid(LockerValidation), EDataValidationResult::Invalid);

		APlaceableFacilityItemActor* LockerItem = APlaceableFacilityItemActor::SpawnFreshItem(
			*World,
			*LockerDefinition,
			FTransform(FRotator::ZeroRotator, FVector(3500.0f + LockerSlots * 300.0f, 0.0f, 200.0f)),
			FailureReason);
		TestNotNull(FString::Printf(TEXT("Locker %d recovery item is created"), LockerSlots), LockerItem);
		if (!LockerItem) continue;
		TestTrue(TEXT("Locker item enters free world"), LockerItem->ActivateFreeWorld(LockerItem->GetActorTransform(), FailureReason));
		TestTrue(TEXT("Locker recovery item can be held for trash-query coverage"), Carry->TryTakePhysicalObject(LockerItem, FailureReason));
		Query = TrashBin->QueryInteraction(Context);
		TestFalse(FString::Printf(TEXT("LockerSlotCount %d is rejected by the trash bin"), LockerSlots), Query.bCanInteract);
		TestEqual(TEXT("Rejected facility reports the trash-bin rejection prompt"), Query.ActionName.ToString(), FString(TEXT("버릴 수 없는 물건")));
		if (Carry->GetHeldObject() == LockerItem)
		{
			Carry->RecoverHeldPhysicalObject(LockerItem);
		}
		LockerItem->Destroy();
	}

	UFacilityPlacementDefinition* UntaggedDefinition = DuplicateObject<UFacilityPlacementDefinition>(
		Definition,
		GetTransientPackage(),
		MakeUniqueObjectName(GetTransientPackage(), UFacilityPlacementDefinition::StaticClass(), TEXT("ShopUntaggedTrashDefinition")));
	UntaggedDefinition->StableId = MakeUniqueObjectName(
		GetTransientPackage(), UFacilityPlacementDefinition::StaticClass(), TEXT("ShopUntaggedTrashId"));
	UntaggedDefinition->FacilityTags.RemoveTag(TAG_Facility_Discardable);
	APlaceableFacilityItemActor* UntaggedItem = APlaceableFacilityItemActor::SpawnFreshItem(
		*World,
		*UntaggedDefinition,
		FTransform(FRotator::ZeroRotator, FVector(6000.0f, 0.0f, 200.0f)),
		FailureReason);
	TestNotNull(TEXT("Non-discardable recovery item is created"), UntaggedItem);
	if (UntaggedItem)
	{
		TestTrue(TEXT("Non-discardable item enters free world"), UntaggedItem->ActivateFreeWorld(UntaggedItem->GetActorTransform(), FailureReason));
		TestTrue(TEXT("Non-discardable item can be held for trash-query coverage"), Carry->TryTakePhysicalObject(UntaggedItem, FailureReason));
		TestFalse(TEXT("Facility tag is the discard authority"), TrashBin->QueryInteraction(Context).bCanInteract);
		if (Carry->GetHeldObject() == UntaggedItem)
		{
			Carry->RecoverHeldPhysicalObject(UntaggedItem);
		}
		UntaggedItem->Destroy();
	}

	FShopOrderLine OpenLine;
	OpenLine.ProductId = TEXT("Shower");
	OpenLine.PlacementDefinition = Definition;
	OpenLine.DisplayName = NSLOCTEXT("ShopAutomation", "ShowerDisplayName", "샤워기");
	OpenLine.Quantity = 1;
	TArray<FShopOrderLine> OpenContents = { OpenLine };
	const FTransform OpenBoxTransform(FRotator::ZeroRotator, FVector(8500.0f, 0.0f, 300.0f));
	AShopDeliveryBoxActor* OpenBox = World->SpawnActorDeferred<AShopDeliveryBoxActor>(
		AShopDeliveryBoxActor::StaticClass(), OpenBoxTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	TestNotNull(TEXT("Unboxing transaction box is staged"), OpenBox);
	if (!OpenBox)
	{
		return false;
	}
	TestTrue(TEXT("Unboxing box accepts one shower line"), OpenBox->InitializeContents(4, OpenContents));
	OpenBox->FinishSpawning(OpenBoxTransform);
	TestTrue(TEXT("Unboxing box activates in free world"), OpenBox->ActivateFreeWorld(OpenBoxTransform, FailureReason));
	TestTrue(TEXT("Carry hand is empty before picking up the unboxing box"), Carry->IsHandEmpty());
	TestTrue(TEXT("Unboxing box is held before opening"), Carry->TryTakePhysicalObject(OpenBox, FailureReason));
	if (Carry->GetHeldObject() != OpenBox) AddError(FailureReason.ToString());
	UPlayerInteractionComponent* EquipmentInteraction = NewObject<UPlayerInteractionComponent>(Player, TEXT("ShopEquipmentInteraction"));
	FHeldEquipmentUseContext OpenContext;
	OpenContext.User = Player;
	OpenContext.Equipment = OpenBox;
	OpenContext.CarryComponent = Carry;
	OpenContext.InteractionComponent = EquipmentInteraction;
	OpenContext.CameraOrigin = ShopUnboxTest::MakeRequest(*Player).CameraOrigin;
	OpenContext.CameraDirection = FVector::ForwardVector;
	EquipmentInteraction->SetInteractionSuppressed(true);
	CarryProbe->ResetHeldChanges();
	TestFalse(TEXT("Suppressed interaction cannot open the delivery box"), OpenBox->BeginEquipmentUse(OpenContext).bSucceeded);
	TestTrue(TEXT("Suppression refusal leaves the original box held"), Carry->GetHeldObject() == OpenBox);
	TestEqual(TEXT("Suppression refusal does not publish a consume event"), CarryProbe->HeldObjectChangedCount, 0);
	EquipmentInteraction->SetInteractionSuppressed(false);
	int32 ItemCountBeforeUnboxing = 0;
	for (TActorIterator<APlaceableFacilityItemActor> It(World); It; ++It) ++ItemCountBeforeUnboxing;
	TestTrue(TEXT("Unboxing transaction succeeds on open floor"), OpenBox->BeginEquipmentUse(OpenContext).bSucceeded);
	TestTrue(TEXT("Successful unboxing leaves the hand empty"), Carry->IsHandEmpty());
	TestFalse(TEXT("Successful unboxing destroys its source box"), IsValid(OpenBox));
	TestEqual(TEXT("Successful unboxing publishes one consume event"), CarryProbe->HeldObjectChangedCount, 1);
	int32 ItemCountAfterUnboxing = 0;
	APlaceableFacilityItemActor* UnboxedItem = nullptr;
	for (TActorIterator<APlaceableFacilityItemActor> It(World); It; ++It)
	{
		++ItemCountAfterUnboxing;
		if (It->GetDefinition() == Definition)
		{
			UnboxedItem = *It;
		}
	}
	TestEqual(TEXT("Successful unboxing creates one fresh facility item"), ItemCountAfterUnboxing, ItemCountBeforeUnboxing + 1);
	TestNotNull(TEXT("Unboxing publishes its fresh shower item"), UnboxedItem);
	UPrimitiveComponent* UnboxedPrimitive = UnboxedItem ? UnboxedItem->GetPhysicalCarryPrimitive() : nullptr;
	TestTrue(TEXT("Unboxed facility item ignores Pawn collision"), UnboxedPrimitive
		&& UnboxedPrimitive->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);
	FShopOrderLine BoxLine;
	BoxLine.ProductId = TEXT("Shower");
	BoxLine.PlacementDefinition = Definition;
	BoxLine.DisplayName = NSLOCTEXT("ShopAutomation", "ShowerDisplayName", "샤워기");
	BoxLine.Quantity = 1;
	TArray<FShopOrderLine> BoxContents = { BoxLine };
	const FTransform BoxTransform(FRotator::ZeroRotator, FVector(9000.0f, 0.0f, 300.0f));
	AShopDeliveryBoxActor* ContentsBox = World->SpawnActorDeferred<AShopDeliveryBoxActor>(
		AShopDeliveryBoxActor::StaticClass(), BoxTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	TestNotNull(TEXT("Initialized-content delivery box is staged"), ContentsBox);
	if (ContentsBox)
	{
		TestTrue(TEXT("Delivery box initializes its immutable order contents"), ContentsBox->InitializeContents(3, BoxContents));
		ContentsBox->FinishSpawning(BoxTransform);
		TestTrue(TEXT("Delivery box enters the free world"), ContentsBox->ActivateFreeWorld(BoxTransform, FailureReason));
		TestEqual(TEXT("Delivery boxes use the appended physical-carry kind"),
			ContentsBox->GetPhysicalCarryKind(), EPhysicalCarryKind::DeliveryBox);
		TestTrue(TEXT("Delivery box summary contains its shower line"),
			ContentsBox->GetContentsSummary().ToString().Contains(TEXT("샤워기 1")));
		TestTrue(TEXT("Delivery box can be picked up"), Carry->TryTakePhysicalObject(ContentsBox, FailureReason));
		TestTrue(TEXT("Delivery box can be freely dropped"), Carry->TryFreeDropHeldObject(FVector::ForwardVector).bSucceeded);
		TestTrue(TEXT("Dropped box keeps its identity and order contents"),
			IsValid(ContentsBox) && ContentsBox->GetOrderId() == 3 && ContentsBox->GetContents().Num() == 1);
		TestTrue(TEXT("Dropped box still advertises E pickup"), ContentsBox->QueryInteraction(Context).bCanInteract);
		TestTrue(TEXT("E pickup reuses the same delivery box actor"), ContentsBox->ExecuteInteraction(Context).bSucceeded
			&& Carry->GetHeldObject() == ContentsBox);
		TestTrue(TEXT("Delivery box contents can be discarded with the box"),
			TrashBin->QueryInteraction(Context).bCanInteract);
		TestTrue(TEXT("Discarding a delivery box consumes the box"),
			TrashBin->ExecuteInteraction(Context).bSucceeded);
		TestTrue(TEXT("Discarded delivery box leaves an empty hand"), Carry->IsHandEmpty());
	}
	CarryProbe->Unbind();
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopBlueprintLoadAutomationTest,
	"BathhouseSim.Shop.BlueprintLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopBlueprintLoadAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FString RequestedPath;
	FParse::Value(FCommandLine::Get(), TEXT("BathhouseShopLoadPath="), RequestedPath);
	TArray<const FShopBlueprintLoadSpec*> RequestedSpecs;
	if (!RequestedPath.IsEmpty())
	{
		if (!FPackageName::IsValidLongPackageName(RequestedPath))
		{
			AddError(FString::Printf(TEXT("Invalid Shop Blueprint package path: %s"), *RequestedPath));
			return false;
		}
		for (const FShopBlueprintLoadSpec& Spec : ShopBlueprintSpecs)
		{
			if (RequestedPath.Equals(Spec.PackagePath, ESearchCase::IgnoreCase)
				|| RequestedPath.Equals(Spec.MigrationCheckPackagePath, ESearchCase::IgnoreCase))
			{
				RequestedSpecs.Add(&Spec);
				break;
			}
		}
		if (RequestedSpecs.IsEmpty())
		{
			AddError(FString::Printf(TEXT("Unsupported Shop Blueprint package path: %s"), *RequestedPath));
			return false;
		}
	}
	else
	{
		for (const FShopBlueprintLoadSpec& Spec : ShopBlueprintSpecs)
		{
			RequestedSpecs.Add(&Spec);
		}
	}

	bool bSucceeded = true;
	for (const FShopBlueprintLoadSpec* Spec : RequestedSpecs)
	{
		const FString PackagePathToLoad = RequestedPath.IsEmpty() || RequestedPath.Equals(Spec->PackagePath, ESearchCase::IgnoreCase)
		? Spec->PackagePath
		: RequestedPath;
		UPackage* Package = LoadPackage(nullptr, *PackagePathToLoad, LOAD_None);
		UBlueprint* Blueprint = FindBlueprintInPackage(Package);
		if (!TestNotNull(FString::Printf(TEXT("%s package loads"), *PackagePathToLoad), Package)
			|| !TestNotNull(FString::Printf(TEXT("%s Blueprint object loads"), *PackagePathToLoad), Blueprint)
			|| !TestNotNull(FString::Printf(TEXT("%s generated class loads"), *PackagePathToLoad), Blueprint ? Blueprint->GeneratedClass.Get() : nullptr))
		{
			bSucceeded = false;
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s keeps its expected native parent"), *PackagePathToLoad),
			Blueprint->GeneratedClass->GetSuperClass() == Spec->NativeParent);

		UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
		if (Spec->NativeParent == ABathhousePlayerState::StaticClass())
		{
			ABathhousePlayerState* PlayerStateCDO = Cast<ABathhousePlayerState>(CDO);
			TestTrue(TEXT("PlayerState CDO contains the ShopCart default subobject"),
				PlayerStateCDO && IsValid(PlayerStateCDO->GetShopCart()));
			UPlayerWalletComponent* Wallet = PlayerStateCDO ? PlayerStateCDO->GetWallet() : nullptr;
			const FIntProperty* StartingMoneyProperty = FindFProperty<FIntProperty>(
				UPlayerWalletComponent::StaticClass(), TEXT("StartingMoney"));
			TestNotNull(TEXT("Wallet StartingMoney property is present"), StartingMoneyProperty);
			TestEqual(TEXT("PlayerState wallet keeps the 100000 default"),
				Wallet && StartingMoneyProperty
					? StartingMoneyProperty->GetPropertyValue_InContainer(Wallet)
					: INDEX_NONE,
				100000);
		}
		else if (Spec->NativeParent == ABathhouseComputerActor::StaticClass())
		{
			ABathhouseComputerActor* ComputerCDO = Cast<ABathhouseComputerActor>(CDO);
			TestTrue(TEXT("Computer ScreenWidget keeps a configured WidgetClass"),
				ComputerCDO && ComputerCDO->GetScreenWidget()
				&& IsValid(ComputerCDO->GetScreenWidget()->GetWidgetClass()));
		}
		else if (Spec->NativeParent == ABathhouseHUD::StaticClass())
		{
			ABathhouseHUD* HUDCDO = Cast<ABathhouseHUD>(CDO);
			const FClassProperty* PromptClassProperty = FindFProperty<FClassProperty>(
				ABathhouseHUD::StaticClass(), TEXT("InteractionPromptWidgetClass"));
			TestNotNull(TEXT("HUD keeps its InteractionPromptWidgetClass property"), PromptClassProperty);
			UClass* PromptWidgetClass = HUDCDO && PromptClassProperty
				? Cast<UClass>(PromptClassProperty->GetObjectPropertyValue_InContainer(HUDCDO))
				: nullptr;
			TestTrue(TEXT("HUD Blueprint retains an interaction prompt widget class"), IsValid(PromptWidgetClass));
		}
		else if (Spec->NativeParent == AShopDeliveryBoxActor::StaticClass())
		{
			const AShopDeliveryBoxActor* BoxCDO = Cast<AShopDeliveryBoxActor>(CDO);
			TestTrue(TEXT("Delivery box Blueprint CDO has its BoxMesh root"),
				BoxCDO && BoxCDO->GetBoxMesh() && BoxCDO->GetRootComponent() == BoxCDO->GetBoxMesh());
			const FVector RootScale = BoxCDO && BoxCDO->GetBoxMesh()
				? BoxCDO->GetBoxMesh()->GetRelativeScale3D()
				: FVector::ZeroVector;
			TestTrue(TEXT("Delivery box CDO root scale is finite and positive"),
				FMath::IsFinite(RootScale.X) && FMath::IsFinite(RootScale.Y) && FMath::IsFinite(RootScale.Z)
				&& RootScale.X > KINDA_SMALL_NUMBER && RootScale.Y > KINDA_SMALL_NUMBER
				&& RootScale.Z > KINDA_SMALL_NUMBER);
			const FStructProperty* HeldTransformProperty = FindFProperty<FStructProperty>(
				AShopDeliveryBoxActor::StaticClass(), TEXT("HeldTransform"));
			TestNotNull(TEXT("Delivery box class exposes HeldTransform"), HeldTransformProperty);
			const AShopDeliveryBoxActor* NativeBoxCDO = GetDefault<AShopDeliveryBoxActor>();
			const FTransform* NativeHeldTransformValue = NativeBoxCDO && HeldTransformProperty
				? HeldTransformProperty->ContainerPtrToValuePtr<FTransform>(NativeBoxCDO)
				: nullptr;
			TestTrue(TEXT("Native delivery box HeldTransform defaults to identity"),
				NativeHeldTransformValue && NativeHeldTransformValue->Equals(FTransform::Identity));
		}
	}
	return bSucceeded;
}

#endif
