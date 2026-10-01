#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/CleaningLitterAutomationTestSupport.h"
#include "Customer/BathhouseCustomerCharacter.h"
#include "Placement/PlayerFacilityPlacementComponent.h"
using namespace CleaningLitterTest;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningLitterClockTest, "BathhouseSim.Cleaning.Litter.SpawnClock",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningLitterClockTest::RunTest(const FString&)
{
	auto MeanCount = [](int32 N)
	{
		int32 Total = 0;
		for (int32 Seed = 1; Seed <= 256; ++Seed)
		{
			FRandomStream Random(Seed);
			float Remaining = FCleaningSpawnClock::SampleNext(Random);
			for (int32 Step = 0; Step < 2400; ++Step)
			{
				if (FCleaningSpawnClock::Advance(Remaining, N, .25f, 120))
				{
					++Total;
					Remaining = FCleaningSpawnClock::SampleNext(Random);
				}
			}
		}
		return Total / 256.f;
	};
	const float One = MeanCount(1), Two = MeanCount(2);
	TestEqual(TEXT("TRSH-001/018: empty zone never fires"), MeanCount(0), 0.f);
	TestTrue(TEXT("TRSH-002/019: two customers produce 10 +/-20% in 600 seconds"), Two >= 8 && Two <= 12);
	TestTrue(TEXT("One customer frequency halves"), One >= 4 && One <= 6 && Two / One >= 1.8 && Two / One <= 2.2);
	float Remaining = .01f;
	TestFalse(TEXT("No customers preserve clock"), FCleaningSpawnClock::Advance(Remaining, 0, .25f, 120));
	TestEqual(TEXT("Clock is unchanged"), Remaining, .01f);
	TestTrue(TEXT("Customer arrival affects next step"), FCleaningSpawnClock::Advance(Remaining, 8, .25f, 120));
	FRandomStream Random(23);
	Remaining = FCleaningSpawnClock::SampleNext(Random);
	TestTrue(TEXT("Large step returns one event"), FCleaningSpawnClock::Advance(Remaining, 2, 100000, 120));
	Remaining = FCleaningSpawnClock::SampleNext(Random);
	const float Fresh = Remaining;
	TestFalse(TEXT("No debt is carried into next zero-customer step"),
			  FCleaningSpawnClock::Advance(Remaining, 0, .25f, 120));
	TestEqual(TEXT("Fresh exponential sample preserved"), Remaining, Fresh);
	FTransform Transform(FRotator(0, 90, 0), FVector(200, 100, 50), FVector(2, 1, 1));
	TArray<FVector> Positions = {Transform.TransformPosition(FVector(10, 20, 30)),
								 Transform.TransformPosition(FVector(60, 0, 0)),
								 Transform.TransformPosition(FVector(0, 0, 60))};
	TestEqual(TEXT("Customer capsule centers use scaled oriented XYZ box"),
			  CountCustomersInBox(Positions, Transform, FVector(50)), 1);
	AddInfo(FString::Printf(TEXT("256 seeds: n=1 %.3f, n=2 %.3f events/600 seconds"), One, Two));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningLitterDirectorTest, "BathhouseSim.Cleaning.Litter.DirectorCapsAndCustomers",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningLitterDirectorTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("LitterDirector"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	Box(World, FVector(0, 0, -20), FVector(1000, 1000, 20))->SetMobility(EComponentMobility::Static);
	auto* Zone =
		World->SpawnActor<ALitterSpawnZoneActor>(ALitterSpawnZoneActor::StaticClass(), FTransform(FVector(0, 0, 100)));
	Zone->GetSpawnBounds()->SetBoxExtent(FVector(600, 600, 100));
	auto* StainZone =
		World->SpawnActor<AStainSpawnZoneActor>(AStainSpawnZoneActor::StaticClass(), FTransform(FVector(0, 0, 100)));
	StainZone->GetSpawnBounds()->SetBoxExtent(FVector(600, 600, 100));
	BeginActorPlayIfNeeded(Zone);
	BeginActorPlayIfNeeded(StainZone);
	auto* Director = World->SpawnActor<ACleaningDirectorActor>();
	ConfigureDirector(Director);
	auto* Registry = World->GetSubsystem<UCleaningWorldSubsystem>();
	Director->AdvanceSpawnScheduleForTesting(100000, {FVector(3000, 0, 100)});
	BeginCleaning(World);
	TestEqual(TEXT("Outside customers do not spawn litter"), Registry->GetActiveLitterCount(), 0);
	TestEqual(TEXT("Outside customers do not spawn stains"), Registry->GetActiveStainCount(), 0);
	for (int32 I = 0; I < 10; ++I)
	{
		Director->AdvanceSpawnScheduleForTesting(100000, {FVector(0, 0, 100), FVector(100, 100, 100)});
		BeginCleaning(World);
	}
	TestEqual(TEXT("TRSH-003: per-zone litter cap"), Registry->GetActiveLitterCount(), 5);
	TestEqual(TEXT("Stain zone cap retained"), Registry->GetActiveStainCount(), StainZone->GetMaxActiveStains());
	Set<FIntProperty>(Director, TEXT("MaxActiveLitter"), 5);
	auto* Other = World->SpawnActor<ALitterSpawnZoneActor>(ALitterSpawnZoneActor::StaticClass(),
														   FTransform(FVector(700, 0, 100)));
	BeginActorPlayIfNeeded(Other);
	Director->AdvanceSpawnScheduleForTesting(100000, {FVector(700, 0, 100)});
	BeginCleaning(World);
	TestEqual(TEXT("Global cap rejects another zone"), Registry->GetActiveLitterCount(), 5);
	ALitterActor* Removed = nullptr;
	for (TActorIterator<ALitterActor> I(World); I; ++I)
	{
		Removed = *I;
		break;
	}
	TestTrue(TEXT("Collection terminal commits once"), Removed && Removed->CommitCollected());
	TestFalse(TEXT("Repeated collection no-op"), Removed->CommitCollected());
	TestEqual(TEXT("Removal immediately frees capacity"), Registry->GetActiveLitterCount(), 4);
	Director->AdvanceSpawnScheduleForTesting(100000, {FVector(0, 0, 100)});
	BeginCleaning(World);
	TestEqual(TEXT("TRSH-025: spawn resumes after removal"), Registry->GetActiveLitterCount(), 5);
	for (TActorIterator<ALitterActor> I(World); I; ++I)
	{
		I->ClearForFacilityPlacement();
	}
	for (TActorIterator<AWaterStainActor> I(World); I; ++I)
	{
		I->ClearForFacilityPlacement();
	}
	Set<FFloatProperty>(Director, TEXT("LitterMeanIntervalPerCustomerSeconds"), .001f);
	Set<FFloatProperty>(Director, TEXT("SpawnIntervalSeconds"), .001f);

	auto SpawnCustomer = [World](const FVector& Location)
	{
		auto* Customer = World->SpawnActorDeferred<ABathhouseCustomerCharacter>(
			ABathhouseCustomerCharacter::StaticClass(), FTransform(Location), nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Customer)
		{
			Customer->AutoPossessAI = EAutoPossessAI::Disabled;
			Customer->FinishSpawning(FTransform(Location));
		}
		return Customer;
	};
	auto* First = SpawnCustomer(FVector(100, 100, 88));
	auto* Second = SpawnCustomer(FVector(-100, -100, 88));
	if (!TestNotNull(TEXT("Real first customer Actor"), First) ||
		!TestNotNull(TEXT("Real second customer Actor"), Second))
	{
		return false;
	}
	First->SetActorTickEnabled(false);
	Second->SetActorTickEnabled(false);
	BeginActorPlayIfNeeded(Director);
	Director->SetSpawnRandomSeedForTesting(7);
	TickWorldForDuration(World, .5, .1f);
	BeginCleaning(World);
	TestEqual(TEXT("Real customer collection runs from director timer: litter"), Registry->GetActiveLitterCount(), 1);
	TestEqual(TEXT("Real customer collection runs from director timer: stain"), Registry->GetActiveStainCount(), 1);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningLitterFloorTest, "BathhouseSim.Cleaning.Litter.FloorAndVariation",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningLitterFloorTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("LitterFloor"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	auto* Floor = Box(World, FVector(0, 0, -20), FVector(500, 500, 20));
	Floor->SetMobility(EComponentMobility::Static);
	Floor->ComponentTags.Add(TEXT("CleaningFloor"));
	FCleaningFloorSpawnSettings S;
	S.BoxTransform = FTransform(FVector(0, 0, 100));
	S.Extent = FVector(.1, .1, 100);
	S.FloorZ = 0;
	S.RequiredFloorTag = TEXT("CleaningFloor");
	// Fixture values for the query; clearance height and floor offset come from the director's data.
	S.TraceDistance = 300;
	S.MaximumSlopeDegrees = 25;
	S.FloorTolerance = 5;
	S.Radius = 15;
	S.Spacing = 40;
	S.ClearanceHeight = GetDefault<ACleaningDirectorActor>()->GetSpawnClearanceHeightCm();
	S.ClearanceFloorOffset = GetDefault<ACleaningDirectorActor>()->GetSpawnClearanceFloorOffsetCm();
	FTransform Result;
	auto Find = [&](bool Spacing = true)
	{
		FRandomStream Random(1);
		return FCleaningFloorSpawnQuery::Find(
			*World, nullptr, S, Random,
			[Spacing](const FVector&, float)
			{
				return Spacing;
			},
			Result);
	};
	TestTrue(TEXT("TRSH-004: authoritative floor allowed"), Find());
	S.FloorZ = 40;
	TestFalse(TEXT("Lower bath floor outside tolerance rejected"), Find());
	S.FloorZ = 0;
	S.RequiredFloorTag = TEXT("WrongFloor");
	TestFalse(TEXT("Required floor tag enforced"), Find());
	S.RequiredFloorTag = TEXT("CleaningFloor");
	TestFalse(TEXT("Same-kind spacing callback enforced"), Find(false));
	auto* Raised = Box(World, FVector(0, 0, 30), FVector(100, 100, 10));
	Raised->ComponentTags.Add(TEXT("CleaningFloor"));
	TestFalse(TEXT("Counter top height rejected"), Find());
	Raised->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	auto* Facility = World->SpawnActor<AServiceAutomationDisplayFacility>();
	auto* Top = Box(World, FVector(0, 0, -20), FVector(100, 100, 20), Facility);
	Top->ComponentTags.Add(TEXT("CleaningFloor"));
	Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Facility top at even matching height rejected"), Find());
	Facility->Destroy();
	Floor->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	auto* Kind = MakeKind(TEXT("FloorBox"), TEXT("Box"), 2, 0);
	auto* Item = SpawnBox(World, Kind, 1, FVector(0, 0, 50));
	TestFalse(TEXT("Physical box blocks spawn"), Find());
	Item->Destroy();
	auto* Wall = Box(World, FVector(12, 0, 15), FVector(2, 100, 15));
	TestFalse(TEXT("Wall edge intersects clearance box"), Find());
	Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	auto* Pawn = World->SpawnActor<APawn>();
	auto* Sphere = NewObject<USphereComponent>(Pawn);
	Pawn->SetRootComponent(Sphere);
	Pawn->AddInstanceComponent(Sphere);
	Sphere->InitSphereRadius(20);
	Sphere->SetCollisionObjectType(ECC_Pawn);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->RegisterComponent();
	Pawn->SetActorLocation(FVector(0, 0, 20));
	TestTrue(TEXT("TRSH-029: Pawn position does not block spawn"), Find());
	Pawn->Destroy();
	auto* L = Litter(World, FVector::ZeroVector, nullptr, 91);
	TestTrue(TEXT("Stain query may trace and overlap litter"), Find());
	auto* Stain = World->SpawnActor<AWaterStainActor>();
	BeginActorPlayIfNeeded(Stain);
	TestTrue(TEXT("Litter query may trace and overlap stain"), Find());
	auto* B = Litter(World, FVector(300, 0, 0), nullptr, 91);
	auto* LM = L->FindComponentByClass<UStaticMeshComponent>();
	auto* BM = B->FindComponentByClass<UStaticMeshComponent>();
	TestEqual(TEXT("TRSH-005: fixed seed picks same mesh"), LM->GetStaticMesh(), BM->GetStaticMesh());
	TestTrue(TEXT("Fixed seed picks same yaw"), LM->GetRelativeRotation().Equals(BM->GetRelativeRotation()));
	const auto Rotation = LM->GetRelativeRotation();
	L->ConfigureVisualVariationSeed(7);
	TestTrue(TEXT("Seed immutable after initialization"), LM->GetRelativeRotation().Equals(Rotation));
	TestFalse(TEXT("Litter has no physics"), L->FindComponentByClass<USphereComponent>()->IsSimulatingPhysics());
	TestEqual(TEXT("Litter ignores Pawn"),
			  L->FindComponentByClass<USphereComponent>()->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
	Stain->Destroy();
	L->Destroy();
	B->Destroy();
	Floor->SetMobility(EComponentMobility::Movable);
	Floor->SetWorldRotation(FRotator(35, 0, 0));
	Floor->SetMobility(EComponentMobility::Static);
	S.RequiredFloorTag = NAME_None;
	S.FloorTolerance = 100;
	S.MaximumSlopeDegrees = 25;
	TestFalse(TEXT("Steep floor is rejected"), Find());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningLitterPlacementTest, "BathhouseSim.Cleaning.Litter.PlacementClear",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningLitterPlacementTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("LitterPlacement"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	FTransform Footprint(FRotator(0, 45, 0), FVector(0, 0, 50), FVector(2, 1, 1));
	const float HeightToleranceFixtureCm = 5.0f;
	const float FootprintHalfHeight = 50.0f;
	TestTrue(TEXT("Footprint includes overlapping circle with center outside"),
			 FCleaningFootprintOverlap::Intersects(Footprint.TransformPosition(FVector(55, 0, 0)), 12, Footprint,
												   FVector(50, 20, 50), HeightToleranceFixtureCm));
	TestFalse(TEXT("Far circle remains"),
			  FCleaningFootprintOverlap::Intersects(Footprint.TransformPosition(FVector(80, 0, 0)), 12, Footprint,
													FVector(50, 20, 50), HeightToleranceFixtureCm));
	const float FootprintTopZ = Footprint.GetLocation().Z + FootprintHalfHeight;
	TestTrue(TEXT("Z inside tolerance is cleared"),
			 FCleaningFootprintOverlap::Intersects(FVector(0, 0, FootprintTopZ + HeightToleranceFixtureCm - 1), 12,
												   Footprint, FVector(50, 20, 50), HeightToleranceFixtureCm));
	TestFalse(TEXT("Z outside tolerance remains"),
			  FCleaningFootprintOverlap::Intersects(FVector(0, 0, FootprintTopZ + HeightToleranceFixtureCm + 1), 12,
													Footprint, FVector(50, 20, 50), HeightToleranceFixtureCm));
	auto* Events = World->GetSubsystem<UFacilityPlacementEventSubsystem>();
	int32 Publications = 0;
	const auto Handle = Events->OnFacilityPlaced.AddLambda(
		[&](const FFacilityPlacedEvent&)
		{
			++Publications;
		});
	auto* CDO = GetDefault<AServiceAutomationDisplayFacility>();
	auto* Placement = CDO->GetPlacementForTest();
	auto* Bounds = Placement->GetPlacementFootprint();
	const FVector Center(12000, 0, 100), Extent = Bounds->GetUnscaledBoxExtent();
	const FTransform BoundTransform = Bounds->GetRelativeTransform() * FTransform(Center);
	auto* Edge = Litter(World, BoundTransform.TransformPosition(FVector(Extent.X + 10, 0, -Extent.Z)));
	auto* Inside = Litter(World, BoundTransform.TransformPosition(FVector(0, 0, -Extent.Z)));
	auto* Outside = Litter(World, FVector(13000, 0, 0));
	auto* Stain =
		World->SpawnActor<AWaterStainActor>(AWaterStainActor::StaticClass(), FTransform(Inside->GetActorLocation()));
	BeginActorPlayIfNeeded(Stain);
	FPlayer MopPlayer;
	BuildPlayer(*this, World, MopPlayer);
	auto* Mop = World->SpawnActor<AWetMopActor>();
	Mesh(Mop);
	BeginActorPlayIfNeeded(Mop);
	FText Failure;
	TestTrue(TEXT("Mop held before placement"), MopPlayer.Carry->TryTakePhysicalObject(Mop, Failure));
	auto MopContext = Context(MopPlayer, Mop, Stain);
	TestTrue(TEXT("Begin mopping active stain"), Mop->BeginEquipmentUse(MopContext).bSucceeded);
	Mop->UpdateEquipmentUse(MopContext, .2f);
	ServiceFacilityTest::FFixture Fixture(World);

	FPlayer PreviewPlayer;
	if (!BuildPlayer(*this, World, PreviewPlayer))
	{
		return false;
	}
	auto* PreviewInput = NewObject<UPlayerFacilityPlacementComponent>(PreviewPlayer.Pawn);
	PreviewPlayer.Pawn->AddInstanceComponent(PreviewInput);
	PreviewInput->RegisterComponent();
	PreviewInput->Configure(PreviewPlayer.Camera, PreviewPlayer.Carry, PreviewPlayer.Interaction);
	auto* PreviewDefinition = MakeFridgeDefinition();
	auto* PreviewItem = APlaceableFacilityItemActor::SpawnFreshItem(*World, *PreviewDefinition,
																	FTransform(FVector(-1000, 0, 300)), Failure);
	if (!TestNotNull(TEXT("Preview item fixture created"), PreviewItem))
	{
		return false;
	}
	TestTrue(TEXT("Preview item becomes free world"),
			 PreviewItem->ActivateFreeWorld(PreviewItem->GetActorTransform(), Failure));
	TestTrue(TEXT("Carry change starts actual preview"),
			 PreviewPlayer.Carry->TryTakePhysicalObject(PreviewItem, Failure));
	TestTrue(TEXT("Preview session is active"), PreviewInput->IsPlacementActive());
	TestEqual(TEXT("Actual preview emits no event"), Publications, 0);
	TestFalse(TEXT("Preview without compatible zone cannot confirm"), PreviewInput->ConfirmPlacement().bSucceeded);
	TestEqual(TEXT("Failed preview confirm emits no event"), Publications, 0);
	PreviewInput->CancelAllSessions();
	TestFalse(TEXT("Preview session canceled"), PreviewInput->IsPlacementActive());
	TestEqual(TEXT("Cancel emits no event"), Publications, 0);
	TestTrue(TEXT("Preview failure/cancel leaves litter and stain"),
			 IsValid(Edge) && IsValid(Inside) && IsValid(Stain));
	PreviewPlayer.Carry->RecoverHeldPhysicalObject(PreviewItem);

	TestTrue(TEXT("Actual PlaceItemAsFacility succeeds through construction fixture"), Fixture.Install(*this));
	TestEqual(TEXT("Exactly one success publication"), Publications, 1);
	TestFalse(TEXT("TRSH-016/017/027: edge circle cleared"), IsValid(Edge));
	TestFalse(TEXT("Interior litter cleared"), IsValid(Inside));
	TestFalse(TEXT("Cleaning stain cleared"), IsValid(Stain));
	TestTrue(TEXT("Distant litter remains"), IsValid(Outside));
	TestTrue(TEXT("Mop keeps mopping after target removal"), Mop->IsMopping());
	MopContext.FocusHit = FHitResult();
	Mop->UpdateEquipmentUse(MopContext, .2f);
	TestTrue(TEXT("Mop stays active without progress target"), Mop->IsMopping());
	auto* BadItem = World->SpawnActor<APlaceableFacilityItemActor>();
	TestNull(TEXT("Failed placement returns no facility"),
			 FFacilityActorConversionTransaction::PlaceItemAsFacility(*BadItem, FTransform::Identity, *Fixture.Zone,
																	  *Fixture.Player.Carry, Failure));
	TestEqual(TEXT("Failure emits no event"), Publications, 1);
	Events->OnFacilityPlaced.Remove(Handle);
	return true;
}
#endif
