#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ServiceAmenityAutomationTestSupport.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Shop/ShopCatalog.h"
#include "Interaction/PhysicalCarryDiscardable.h"
#include "HAL/IConsoleManager.h"
#include "Tests/CleaningLitterAutomationTestSupport.h"
using namespace ServiceAmenityTest;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServiceAmenityRestTest,
								 "BathhouseSim.Service.Amenity.Rest.TelevisionReinstallAndBenchSlots",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServiceAmenityRestTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("RestWorld"));
	FFixture F(Scope.Get());
	if (!F.Install(*this, ATelevisionActor::StaticClass()))
	{
		return false;
	}
	auto* TV = CastChecked<ATelevisionActor>(F.Facility);
	FText Failure;
	TestEqual(TEXT("TV no slots"), TV->GetFacilitySlots().Num(), 0);
	TestFalse(TEXT("REST-003 default off"), TV->IsPoweredOn());
	TestTrue(TEXT("REST-001 E turns on"), TV->ExecuteInteraction(F.Context()).bSucceeded && TV->IsPoweredOn());
	TV->TryBeginFacilityRecoveryHold(Failure);
	TV->CancelFacilityRecoveryHold();
	TestTrue(TEXT("REST-006 Q cancel preserves on"), TV->IsPoweredOn());
	auto* Towel = HoldTowel(*F.World, *F.Player.Carry);
	TestNotNull(TEXT("Towel held"), Towel);
	TestEqual(TEXT("REST-002 exact held reason"), TV->QueryInteraction(F.Context()).FailureReason.ToString(),
			  FString(TEXT("빈손으로 켜고 끌 수 있음")));
	TestFalse(TEXT("Held execute rejected"), TV->ExecuteInteraction(F.Context()).bSucceeded);
	F.Player.Carry->RecoverHeldPhysicalObject(Towel);
	Towel->SetActorLocation(FVector(0, 4000, 300));
	auto* Item = FFacilityActorConversionTransaction::RecoverFacilityToItem(*TV, Failure);
	if (!TestNotNull(TEXT("No-slot TV recovery"), Item))
	{
		return false;
	}
	F.Player.Carry->TryTakePhysicalObject(Item, Failure);
	TV = Cast<ATelevisionActor>(FFacilityActorConversionTransaction::PlaceItemAsFacility(*Item, F.Transform, *F.Zone,
																						 *F.Player.Carry, Failure));
	if (!TestNotNull(TEXT("TV reinstalls"), TV))
	{
		return false;
	}
	BeginActorPlayIfNeeded(TV);
	TestFalse(TEXT("REST-003 reinstall off"), TV->IsPoweredOn());
	TV->Destroy();
	F.Definition->PlacedFacilityClass = AServiceAmenityBenchProbe::StaticClass();
	Item = APlaceableFacilityItemActor::SpawnFreshItem(*F.World, *F.Definition, FTransform(FVector(11000, 0, 300)),
													   Failure);
	Item->ActivateFreeWorld(Item->GetActorTransform(), Failure);
	F.Player.Carry->TryTakePhysicalObject(Item, Failure);
	F.Facility = Cast<ABathhouseFacilityActor>(FFacilityActorConversionTransaction::PlaceItemAsFacility(
		*Item, F.Transform, *F.Zone, *F.Player.Carry, Failure));
	if (!TestNotNull(TEXT("Bench installs"), F.Facility))
	{
		return false;
	}
	BeginActorPlayIfNeeded(F.Facility);
	if (!TestEqual(TEXT("REST-004 three seats"), F.Facility->GetFacilitySlots().Num(), 3))
	{
		return false;
	}
	TArray<AServiceTestUserActor*> Users;
	for (int32 I = 0; I < 3; ++I)
	{
		auto* User = F.Occupy(I);
		if (!TestNotNull(TEXT("Bench user reserved"), User))
		{
			return false;
		}
		Users.Add(User);
	}
	TestFalse(TEXT("REST-005 occupied bench recovery denied"), F.Facility->QueryFacilityRecovery().bSucceeded);
	auto* Fourth = F.World->SpawnActor<AServiceTestUserActor>();
	bool FourthReserved = false;
	for (UBathhouseFacilitySlotComponent* Seat : F.Facility->GetFacilitySlots())
	{
		FourthReserved |= Seat->TryReserve(Fourth);
	}
	TestFalse(TEXT("REST-005 fourth user cannot reserve any seat"), FourthReserved);
	Fourth->Destroy();
	auto* Slot = F.Facility->GetFacilitySlots()[0].Get();
	Slot->EndUse(Users[0]);
	TestEqual(TEXT("Bench knockdown reserves"), Slot->GetSlotState(), EBathhouseFacilitySlotState::Reserved);
	TestTrue(TEXT("Bench stand restarts"), Slot->BeginUse(Users[0]));
	for (int32 I = 0; I < 3; ++I)
	{
		F.Facility->GetFacilitySlots()[I]->ForceRelease();
		Users[I]->Destroy();
	}
	TestTrue(TEXT("All available allows recovery"), F.Facility->QueryFacilityRecovery().bSucceeded);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServiceAmenityShopTest,
								 "BathhouseSim.Service.Amenity.Shop.FourDefinitionsUnboxWorldDiscardAndDebug",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServiceAmenityShopTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("AmenityShopWorld"));
	FScopedServiceGrid Grid;
	UWorld* World = Scope.Get();
	FPlayer Player;
	if (!BuildPlayer(*this, World, Player, true))
	{
		return false;
	}
	auto* Capsule = NewObject<UCapsuleComponent>(Player.Pawn);
	Capsule->SetupAttachment(Player.Camera);
	Capsule->InitCapsuleSize(34, 88);
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Player.Pawn->AddInstanceComponent(Capsule);
	Capsule->RegisterComponent();
	TArray<FShopOrderLine> Lines;
	TArray<UFacilityPlacementDefinition*> Definitions;
	UClass* Classes[] = {AServiceAmenityChairProbe::StaticClass(), AServiceAmenityBenchProbe::StaticClass(),
						 ATelevisionActor::StaticClass(), AServiceAmenityTableProbe::StaticClass()};
	for (int32 I = 0; I < 4; ++I)
	{
		auto* D = MakeFridgeDefinition();
		D->PlacedFacilityClass = Classes[I];
		Definitions.Add(D);
		auto& Line = Lines.AddDefaulted_GetRef();
		Line.ProductId = *FString::Printf(TEXT("Amenity%d"), I);
		Line.PlacementDefinition = D;
		Line.Quantity = 1;
		Line.DisplayName = FText::FromString(TEXT("Service"));
	}
	auto* Floor = CleaningLitterTest::Box(World, FVector(2000, 0, -20), FVector(5000, 5000, 20));
	Floor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	const FTransform Spawn(FVector(2000, 0, 300));
	auto* Delivery = World->SpawnActorDeferred<AShopDeliveryBoxActor>(
		AShopDeliveryBoxActor::StaticClass(), Spawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	TestTrue(TEXT("Four line order accepted"), Delivery->InitializeContents(44, Lines));
	Delivery->FinishSpawning(Spawn);
	FText Failure;
	Delivery->ActivateFreeWorld(Spawn, Failure);
	Player.Carry->TryTakePhysicalObject(Delivery, Failure);
	TestTrue(TEXT("SVC4-002 actual unbox"), Player.EquipmentUse->BeginEquipmentUse().bSucceeded);
	Player.EquipmentUse->EndEquipmentUse();
	int32 Items = 0;
	for (TActorIterator<APlaceableFacilityItemActor> It(World); It; ++It)
	{
		if (Definitions.Contains(It->GetDefinition()))
		{
			++Items;
			auto* Discard = Cast<IPhysicalCarryDiscardable>(*It);
			TestTrue(TEXT("World item discard contract"), Discard && Discard->CanDiscardFromWorld(Failure));
		}
	}
	TestEqual(TEXT("Exactly four delivered items"), Items, 4);
	const FTransform CollectionTransform(FVector(2000, 0, 0));
	auto* Collection = World->SpawnActorDeferred<ATrashCollectionZoneActor>(ATrashCollectionZoneActor::StaticClass(),
																			CollectionTransform);
	CleaningLitterTest::Set<FFloatProperty>(Collection, TEXT("CollectionIntervalSeconds"), 60.f);
	Collection->FindComponentByClass<UBoxComponent>()->SetBoxExtent(FVector(10000));
	Collection->FinishSpawning(CollectionTransform);
	BeginActorPlayIfNeeded(Collection);
	TickWorldForDuration(World, 61);
	int32 Remaining = 0;
	for (TActorIterator<APlaceableFacilityItemActor> It(World); It; ++It)
	{
		if (Definitions.Contains(It->GetDefinition()))
		{
			++Remaining;
		}
	}
	TestEqual(TEXT("SVC4-002 next collection removes all four items"), Remaining, 0);
#if !UE_BUILD_SHIPPING
	for (const auto* Name :
		 {TEXT("bathhouse.Debug.Service.SpawnTestUser"), TEXT("bathhouse.Debug.Service.KnockdownTestUser"),
		  TEXT("bathhouse.Debug.Service.StandUpTestUser"), TEXT("bathhouse.Debug.Service.RemoveTestUser")})
	{
		TestNotNull(TEXT("SVC4-003 debug command registered"), IConsoleManager::Get().FindConsoleObject(Name));
	}
#endif
	return true;
}
#endif
