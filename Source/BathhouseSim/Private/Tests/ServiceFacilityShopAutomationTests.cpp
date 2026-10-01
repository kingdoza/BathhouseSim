#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ServiceFacilityAutomationTestSupport.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Shop/ShopCatalog.h"
#include "Shop/ShopSettings.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Misc/DataValidation.h"
using namespace ServiceFacilityTest;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseServiceUnitTwoShopTest,
								 "BathhouseSim.Service.Shop.UnitTwoCatalogUnboxingAndOffset",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceUnitTwoShopTest::RunTest(const FString& Parameters)
{
	FScopedUtilityLaborWorld Scope(TEXT("UnitTwoShopWorld"));
	ServiceFacilityTest::FFixture Fixture(Scope.Get());
	if (!Fixture.Install(*this))
	{
		return false;
	}
	auto* Catalog = NewObject<UShopCatalog>();
	auto& Vanity = Catalog->Products.AddDefaulted_GetRef();
	Vanity.ProductId = TEXT("Vanity");
	Vanity.DisplayName = FText::FromString(TEXT("화장대"));
	Vanity.PlacementDefinition = Fixture.Definition;
	Vanity.Price = 20000;
	Vanity.bForSale = true;
	auto Kinds = Fixture.Kinds;
	for (const auto* Id : {TEXT("Shampoo"), TEXT("BodyWash")})
	{
		auto* Kind = MakeKind(Id, Id, 6, 0, false);
		Kind->ConsumableUses = 30;
		Kinds.Add(Kind);
	}
	const int32 Prices[] = {10000, 6000, 3000, 3000, 6000, 6000};
	for (int32 Index = 0; Index < 6; ++Index)
	{
		auto& Product = Catalog->Products.AddDefaulted_GetRef();
		Product.ProductId = Kinds[Index]->ItemId;
		Product.DisplayName = Kinds[Index]->DisplayName;
		Product.ItemBoxDefinition = Kinds[Index];
		Product.Price = Prices[Index];
		Product.bForSale = true;
	}
	FDataValidationContext Validation;
	TestEqual(TEXT("Seven unit-two products validate"), Catalog->IsDataValid(Validation), EDataValidationResult::Valid);
	TestEqual(TEXT("Exactly seven product entries"), Catalog->Products.Num(), 7);
	auto* Lotion = Kinds[1].Get();
	FText Failure;
	TestTrue(TEXT("Identity box offset default"), Lotion->BoxItemOffset.Equals(FTransform::Identity));
	Lotion->BoxItemOffset = FTransform(FRotator(10, 25, 5), FVector(2, 3, 4), FVector(0.75f));
	TestTrue(TEXT("Positive finite offset validates"), Lotion->ValidateRuntime(Failure));
	auto* OffsetBox = SpawnBox(Fixture.World, Lotion, 2);
	FTransform Actual;
	OffsetBox->GetContentsVisual()->GetInstanceTransform(1, Actual, false);
	TestTrue(TEXT("Box runtime composes offset before slot"),
			 Actual.Equals(Lotion->BoxItemOffset * Lotion->BoxSlotTransforms[1]));
	Lotion->BoxItemOffset.SetScale3D(FVector(-1, 1, 1));
	TestFalse(TEXT("Negative offset scale rejected"), Lotion->ValidateRuntime(Failure));
	Lotion->BoxItemOffset = FTransform::Identity;
	Lotion->ConsumableUses = -1;
	TestFalse(TEXT("Negative uses rejected"), Lotion->ValidateRuntime(Failure));
	Lotion->ConsumableUses = 10;
	auto* Capsule = NewObject<UCapsuleComponent>(Fixture.Player.Pawn);
	Capsule->SetupAttachment(Fixture.Player.Camera);
	Capsule->InitCapsuleSize(34, 88);
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Fixture.Player.Pawn->AddInstanceComponent(Capsule);
	Capsule->RegisterComponent();
	auto* Settings = GetMutableDefault<UShopSettings>();
	auto SavedClass = Settings->ItemBoxClass;
	Settings->ItemBoxClass = AItemBoxActor::StaticClass();
	// A snapshot order with one vanity and one lotion box exercises both approved product types.
	FShopOrderLine FacilityLine;
	FacilityLine.ProductId = TEXT("Vanity");
	FacilityLine.PlacementDefinition = Fixture.Definition;
	FacilityLine.DisplayName = FText::FromString(TEXT("화장대"));
	FacilityLine.Quantity = 1;
	FShopOrderLine BoxLine;
	BoxLine.ProductId = Lotion->ItemId;
	BoxLine.ItemBoxDefinition = Lotion;
	BoxLine.DisplayName = Lotion->DisplayName;
	BoxLine.Quantity = 1;
	FTransform Transform(FVector(2000, 0, 300));
	auto* Delivery = Fixture.World->SpawnActorDeferred<AShopDeliveryBoxActor>(
		AShopDeliveryBoxActor::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	TestTrue(TEXT("Unit-two order snapshot accepted"), Delivery->InitializeContents(22, {FacilityLine, BoxLine}));
	Delivery->FinishSpawning(Transform);
	Delivery->ActivateFreeWorld(Transform, Failure);
	TestTrue(TEXT("Delivery held"), Fixture.Player.Carry->TryTakePhysicalObject(Delivery, Failure));
	auto Result = Fixture.Player.EquipmentUse->BeginEquipmentUse();
	Fixture.Player.EquipmentUse->EndEquipmentUse();
	Settings->ItemBoxClass = SavedClass;
	TestTrue(TEXT("SHOP-S03/S04 mixed unboxing succeeds"), Result.bSucceeded);
	TestTrue(TEXT("Delivery consumed and hand cleared"), !IsValid(Delivery) && Fixture.Player.Carry->IsHandEmpty());
	int32 FacilityItems = 0, FullLotionBoxes = 0;
	for (TActorIterator<APlaceableFacilityItemActor> It(Fixture.World); It; ++It)
	{
		if (IsValid(*It) && It->GetDefinition() == Fixture.Definition)
		{
			++FacilityItems;
		}
	}
	for (TActorIterator<AItemBoxActor> It(Fixture.World); It; ++It)
	{
		if (It->GetContents().Kind == Lotion && It->GetContents().Count == 6)
		{
			++FullLotionBoxes;
		}
	}
	TestEqual(TEXT("One vanity item delivered"), FacilityItems, 1);
	TestEqual(TEXT("One full six-item lotion box delivered"), FullLotionBoxes, 1);
	return true;
}
#endif
