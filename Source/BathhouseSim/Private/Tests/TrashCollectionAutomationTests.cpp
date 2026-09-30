#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/CleaningLitterAutomationTestSupport.h"
#include "Interaction/BathhouseKeyActor.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Towel/WorldUsedTowelActor.h"
#include "Economy/PlayerWalletComponent.h"
using namespace CleaningLitterTest;

namespace
{
	ATrashBagActor* CollectionBag(UWorld* World, const FVector& Location)
	{
		auto* Bag = ATrashBagActor::SpawnTiedBag(*World, ATrashBagActor::StaticClass(), 4, FTransform(Location));
		BeginActorPlayIfNeeded(Bag);
		if (Bag)
		{
			Bag->GetPhysicalCarryPrimitive()->SetSimulatePhysics(false);
		}
		return Bag;
	}

	int32 FacilityItems(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<APlaceableFacilityItemActor> I(World); I; ++I)
		{
			++Count;
		}
		return Count;
	}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrashCollectionKindsTest, "BathhouseSim.Cleaning.Litter.CollectionKindsAndPayload",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrashCollectionKindsTest::RunTest(const FString&)
{
	FScopedServiceGrid Grid;
	FScopedUtilityLaborWorld Scope(TEXT("TrashCollectionKinds"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	FPlayer Player;
	BuildPlayer(*this, World, Player, true);
	FText Failure;
	auto* Zone = World->SpawnActor<ATrashCollectionZoneActor>(ATrashCollectionZoneActor::StaticClass(),
															  FTransform(FVector(1000, 0, 100)));
	auto* Bounds = Zone->FindComponentByClass<UBoxComponent>();
	Bounds->SetBoxExtent(FVector(600, 600, 300));
	auto* Kind = MakeKind(TEXT("CollectionGoods"), TEXT("Collection Goods"), 4, 500);
	auto* BoxItem = SpawnBox(World, Kind, 3, FVector(800, -300, 100));
	auto* EmptyBox = SpawnBox(World, Kind, 0, FVector(1000, -300, 100));
	auto* Definition = MakeFridgeDefinition();
	auto* Delivery = World->SpawnActorDeferred<AShopDeliveryBoxActor>(
		AShopDeliveryBoxActor::StaticClass(), FTransform(FVector(1200, -300, 100)), nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	FShopOrderLine Line;
	Line.ProductId = TEXT("CollectionShower");
	Line.PlacementDefinition = Definition;
	Line.DisplayName = FText::FromString(TEXT("Facility"));
	Line.Quantity = 1;
	TestTrue(TEXT("Delivery initialized with contents"), Delivery->InitializeContents(1, {Line}));
	Delivery->FinishSpawning(FTransform(FVector(1200, -300, 100)));
	BeginActorPlayIfNeeded(Delivery);
	TestTrue(TEXT("Delivery free world"), Delivery->ActivateFreeWorld(Delivery->GetActorTransform(), Failure));
	auto* FacilityItem =
		APlaceableFacilityItemActor::SpawnFreshItem(*World, *Definition, FTransform(FVector(800, 100, 100)), Failure);
	TestTrue(TEXT("Discardable facility item activates"),
			 FacilityItem && FacilityItem->ActivateFreeWorld(FacilityItem->GetActorTransform(), Failure));
	BeginActorPlayIfNeeded(FacilityItem);
	auto* Bag = CollectionBag(World, FVector(1000, 100, 100));
	auto* Held = CollectionBag(World, FVector(1200, 100, 100));
	TestTrue(TEXT("Take protected bag"), Player.Carry->TryTakePhysicalObject(Held, Failure));
	TestFalse(TEXT("COLL-003: held bag cannot world discard"), Held->CanDiscardFromWorld(Failure));
	Held->HandleDiscardFromWorldCommitted();
	TestTrue(TEXT("Rejected held commit leaves bag"), IsValid(Held) && Player.Carry->GetHeldObject() == Held);

	auto* LockerDefinition = LoadObject<UFacilityPlacementDefinition>(
		nullptr, TEXT("/Game/Bathhouse/Data/Placement/"
					  "DA_FacilityPlacement_ClothesLocker_1.DA_FacilityPlacement_ClothesLocker_1"));
	if (!TestNotNull(TEXT("Locker definition loads"), LockerDefinition))
	{
		return false;
	}
	auto* Locker = APlaceableFacilityItemActor::SpawnFreshItem(*World, *LockerDefinition,
															   FTransform(FVector(1000, 350, 100)), Failure);
	if (!TestNotNull(TEXT("Locker item fixture exists"), Locker))
	{
		return false;
	}
	Locker->ActivateFreeWorld(Locker->GetActorTransform(), Failure);
	BeginActorPlayIfNeeded(Locker);
	TestFalse(TEXT("COLL-002: locker cannot discard"), Locker->CanDiscardFromWorld(Failure));
	auto* Key =
		World->SpawnActor<ABathhouseKeyActor>(ABathhouseKeyActor::StaticClass(), FTransform(FVector(900, 350, 100)));
	BeginActorPlayIfNeeded(Key);
	auto* T = Tongs(World);
	T->SetActorLocation(FVector(950, 350, 100));
	auto* Mop = World->SpawnActor<AWetMopActor>();
	Mesh(Mop);
	Mop->SetActorLocation(FVector(1100, 350, 100));
	BeginActorPlayIfNeeded(Mop);
	auto* L = Litter(World, FVector(1100, -100, 0));
	auto* Towel = World->SpawnActor<AWorldUsedTowelActor>(AWorldUsedTowelActor::StaticClass(),
														  FTransform(FVector(1200, 350, 100)));
	BeginActorPlayIfNeeded(Towel);
	auto* Placed = World->SpawnActor<AServiceAutomationDisplayFacility>();
	Placed->SetActorLocation(FVector(1300, 100, 100));
	const int32 ItemsBefore = FacilityItems(World);
	const int32 MoneyBefore = Player.PlayerState->GetWallet()->GetCurrentMoney();
	TestTrue(TEXT("Filled box world discard allowed"), BoxItem->CanDiscardFromWorld(Failure));
	TestTrue(TEXT("Delivery world discard allowed"), Delivery->CanDiscardFromWorld(Failure));
	TestTrue(TEXT("Bag world discard allowed"), Bag->CanDiscardFromWorld(Failure));
	Zone->CollectNow();
	TestFalse(TEXT("COLL-001: filled box collected"), IsValid(BoxItem));
	TestFalse(TEXT("Empty initialized box collected"), IsValid(EmptyBox));
	TestFalse(TEXT("Delivery collected with contents"), IsValid(Delivery));
	TestFalse(TEXT("Discardable facility item collected"), IsValid(FacilityItem));
	TestFalse(TEXT("Tied bag collected"), IsValid(Bag));
	TestEqual(TEXT("COLL-004: wallet unchanged"), Player.PlayerState->GetWallet()->GetCurrentMoney(), MoneyBefore);
	TestEqual(TEXT("Consumed facility EndPlay creates no recovery item"), FacilityItems(World), ItemsBefore - 1);
	TestTrue(TEXT("COLL-002/008: held bag, locker, key, tools, litter, towel, placed facility remain"),
			 IsValid(Held) && IsValid(Locker) && IsValid(Key) && IsValid(T) && IsValid(Mop) && IsValid(L) &&
				 IsValid(Towel) && IsValid(Placed));
	TestEqual(TEXT("Collected delivery payload cleared"), Delivery->GetContents().Num(), 0);
	TestEqual(TEXT("Collected bag payload cleared"), Bag->GetLitterCount(), 0);
	Zone->CollectNow();
	TestEqual(TEXT("Repeated collection leaves wallet unchanged"), Player.PlayerState->GetWallet()->GetCurrentMoney(),
			  MoneyBefore);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrashCollectionCenterTest,
								 "BathhouseSim.Cleaning.Litter.CollectionCenterAndWorldGuard",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrashCollectionCenterTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("TrashCollectionCenter"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	FTransform Transform(FRotator(0, 45, 0), FVector(1000, 0, 100), FVector(2, 1, 1));
	auto* Zone = World->SpawnActor<ATrashCollectionZoneActor>(ATrashCollectionZoneActor::StaticClass(), Transform);
	auto* Bounds = Zone->FindComponentByClass<UBoxComponent>();
	Bounds->SetBoxExtent(FVector(100));
	const auto BoxTransform = Bounds->GetComponentTransform();
	auto* Inside = CollectionBag(World, BoxTransform.TransformPosition(FVector(90, 0, 0)));
	auto* Edge = CollectionBag(World, BoxTransform.TransformPosition(FVector(120, 0, 0)));
	auto* ZOutside = CollectionBag(World, BoxTransform.TransformPosition(FVector(0, 0, 120)));

	auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	auto* PositiveMesh = DuplicateObject<UStaticMesh>(Cube, GetTransientPackage());
	PositiveMesh->SetPositiveBoundsExtension(FVector(120, 0, 0));
	PositiveMesh->CalculateExtendedBounds();
	auto* NegativeMesh = DuplicateObject<UStaticMesh>(Cube, GetTransientPackage());
	NegativeMesh->SetNegativeBoundsExtension(FVector(120, 0, 0));
	NegativeMesh->CalculateExtendedBounds();
	auto* OriginInside = CollectionBag(World, BoxTransform.TransformPosition(FVector(90, 0, 0)));
	auto* OriginOutside = CollectionBag(World, BoxTransform.TransformPosition(FVector(120, 0, 0)));
	OriginInside->SetActorRotation(BoxTransform.GetRotation());
	OriginOutside->SetActorRotation(BoxTransform.GetRotation());
	auto* InsideMesh = CastChecked<UStaticMeshComponent>(OriginInside->GetPhysicalCarryPrimitive());
	InsideMesh->SetStaticMesh(PositiveMesh);
	InsideMesh->UpdateBounds();
	auto* OutsideMesh = CastChecked<UStaticMeshComponent>(OriginOutside->GetPhysicalCarryPrimitive());
	OutsideMesh->SetStaticMesh(NegativeMesh);
	OutsideMesh->UpdateBounds();
	TestTrue(TEXT("Mesh bounds center is outside despite inside Actor origin"),
			 BoxTransform.InverseTransformPosition(InsideMesh->Bounds.Origin).X > 100);
	TestTrue(TEXT("Mesh bounds center is inside despite outside Actor origin"),
			 BoxTransform.InverseTransformPosition(OutsideMesh->Bounds.Origin).X < 100);
	Zone->CollectNow();
	TestTrue(TEXT("Collection preserves inside-origin object with outside bounds center"), IsValid(OriginInside));
	TestFalse(TEXT("Collection removes outside-origin object with inside bounds center"), IsValid(OriginOutside));
	TestFalse(TEXT("COLL-007: inside primitive center collected"), IsValid(Inside));

	TestTrue(TEXT("Overlap-only edge outside center remains"), IsValid(Edge));
	TestTrue(TEXT("Z outside box remains"), IsValid(ZOutside));
	FPlayer Player;
	BuildPlayer(*this, World, Player);
	FText Failure;
	TestTrue(TEXT("Candidate initially world discardable"), Edge->CanDiscardFromWorld(Failure));
	Player.Carry->TryTakePhysicalObject(Edge, Failure);
	Edge->HandleDiscardFromWorldCommitted();
	TestTrue(TEXT("State recheck prevents a prepared target newly held from consumption"),
			 IsValid(Edge) && Player.Carry->GetHeldObject() == Edge);
	auto* Staged = World->SpawnActor<ATrashBagActor>();
	TestFalse(TEXT("Uninitialized staged bag is not discardable"), Staged->CanDiscardFromWorld(Failure));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrashCollectionTimerTest, "BathhouseSim.Cleaning.Litter.CollectionGameTime",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrashCollectionTimerTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("TrashCollectionTimer"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	World->GetWorldSettings()->WorldGravityZ = 0;
	World->GetWorldSettings()->bGlobalGravitySet = true;
	FPlayer Player;
	BuildPlayer(*this, World, Player, true);
	auto* Zone = World->SpawnActor<ATrashCollectionZoneActor>(ATrashCollectionZoneActor::StaticClass(),
															  FTransform(FVector(1000, 0, 100)));
	BeginActorPlayIfNeeded(Zone);
	auto* Bag = CollectionBag(World, FVector(1000, 0, 100));
	TickWorldForDuration(World, 299, .25f);
	TestTrue(TEXT("COLL-005: default 300 seconds does not collect early"), IsValid(Bag));
	World->GetWorldSettings()->SetPauserPlayerState(Player.PlayerState);
	TickWorldForDuration(World, 3, .25f);
	TestTrue(TEXT("Paused game time freezes collection"), IsValid(Bag));
	World->GetWorldSettings()->SetPauserPlayerState(nullptr);
	TickWorldForDuration(World, 1.5, .25f);
	TestFalse(TEXT("Collection occurs after a full game-time period"), IsValid(Bag));
	Zone->Destroy();
	auto* Fast = World->SpawnActor<ATrashCollectionZoneActor>(ATrashCollectionZoneActor::StaticClass(),
															  FTransform(FVector(1000, 0, 100)));
	Set<FFloatProperty>(Fast, TEXT("CollectionIntervalSeconds"), 60.f);
	BeginActorPlayIfNeeded(Fast);
	auto* Next = CollectionBag(World, FVector(1000, 0, 100));
	TickWorldForDuration(World, 59, .25f);
	TestTrue(TEXT("COLL-009: 60-second zone does not collect early"), IsValid(Next));
	TickWorldForDuration(World, 1.5, .25f);
	TestFalse(TEXT("Overridden period collects"), IsValid(Next));
	auto* Last = CollectionBag(World, FVector(1000, 0, 100));
	Fast->Destroy();
	TickWorldForDuration(World, 61, .25f);
	TestTrue(TEXT("EndPlay unregisters timer"), IsValid(Last));
	return true;
}
#endif
