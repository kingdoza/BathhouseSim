#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/CleaningLitterAutomationTestSupport.h"
#include "Character/FirstPersonCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Customer/BathhouseCustomerCharacter.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMesh.h"
#include "Interaction/BathhouseKeyActor.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Placement/FacilityPlacementZoneActor.h"
#include "Towel/WorldUsedTowelActor.h"

using namespace CleaningLitterTest;

namespace
{
	// Fixture footprints for the zone queries. Height and floor offset come from the director's data.
	constexpr float StainRadiusFixtureCm = 30.0f;
	constexpr float LitterRadiusFixtureCm = 15.0f;

	struct FSpawnZones
	{
		AStainSpawnZoneActor* Stain;
		ALitterSpawnZoneActor* Litter;

		explicit FSpawnZones(UWorld* World)
		{
			const FTransform Pose(FVector(0, 0, 100));
			Stain = World->SpawnActor<AStainSpawnZoneActor>(AStainSpawnZoneActor::StaticClass(), Pose);
			Litter = World->SpawnActor<ALitterSpawnZoneActor>(ALitterSpawnZoneActor::StaticClass(), Pose);
			Stain->GetSpawnBounds()->SetBoxExtent(FVector(.01, .01, 100));
			Litter->GetSpawnBounds()->SetBoxExtent(FVector(.01, .01, 100));
			BeginActorPlayIfNeeded(Stain);
			BeginActorPlayIfNeeded(Litter);
		}

		void Expect(FAutomationTestBase& Test, const TCHAR* Label, bool Expected)
		{
			FRandomStream StainRandom(77), LitterRandom(77);
			FTransform Pose;
			const ACleaningDirectorActor* Director = GetDefault<ACleaningDirectorActor>();
			const float Height = Director->GetSpawnClearanceHeightCm();
			const float Offset = Director->GetSpawnClearanceFloorOffsetCm();
			Test.TestEqual(FString(Label) + TEXT(" (stain zone)"),
						   Stain->FindSpawnTransform(StainRandom, 0, Pose, StainRadiusFixtureCm, Height, Offset), Expected);
			Test.TestEqual(FString(Label) + TEXT(" (litter zone)"),
						   Litter->FindSpawnTransform(LitterRandom, 0, Pose, LitterRadiusFixtureCm, Height, Offset),
						   Expected);
		}
	};

	UStaticMeshComponent* StaticFloor(UWorld* World, const FTransform& Pose)
	{
		auto* Actor = World->SpawnActor<AActor>();
		auto* Component = NewObject<UStaticMeshComponent>(Actor);
		Actor->SetRootComponent(Component);
		Actor->AddInstanceComponent(Component);
		Component->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Component->SetWorldTransform(Pose);
		Component->SetMobility(EComponentMobility::Static);
		Component->SetCollisionProfileName(TEXT("BlockAll"));
		Component->RegisterComponent();
		return Component;
	}

	ABathhouseCustomerCharacter* Customer(UWorld* World, const FVector& Location)
	{
		auto* Actor = World->SpawnActorDeferred<ABathhouseCustomerCharacter>(
			ABathhouseCustomerCharacter::StaticClass(), FTransform(Location), nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		Actor->AutoPossessAI = EAutoPossessAI::Disabled;
		Actor->FinishSpawning(FTransform(Location));
		return Actor;
	}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningSpawnRegionsTest, "BathhouseSim.Cleaning.Clearance.RegionsAndDirector",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningSpawnRegionsTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("CleaningSpawnRegions"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	Box(World, FVector(0, 0, -20), FVector(2000, 2000, 20))->SetMobility(EComponentMobility::Static);
	auto* Placement = World->SpawnActor<AFacilityPlacementZoneActor>(AFacilityPlacementZoneActor::StaticClass(),
																	 FTransform(FVector(600, -100, 0)));
	Placement->GetZoneBounds()->SetBoxExtent(FVector(1400, 900, 10));
	TestEqual(TEXT("DefaultMap placement bounds ignore PhysicsBody"),
			  Placement->GetZoneBounds()->GetCollisionResponseToChannel(ECC_PhysicsBody), ECR_Ignore);
	FSpawnZones Zones(World);
	Zones.Expect(*this, TEXT("F1: both overlapping zones inside DefaultMap placement bounds"), true);
	// This space overlaps clearance at ground level, without covering the vertical floor ray.
	auto* Space = Box(World, FVector(20, 0, 0), FVector(5, 20, 10));
	Space->SetMobility(EComponentMobility::Movable);
	Space->SetCollisionObjectType(ECC_WorldDynamic);
	Space->SetCollisionResponseToAllChannels(ECR_Ignore);
	Space->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Zones.Expect(*this, TEXT("Visibility-only display space does not obstruct clearance"), true);
	Space->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	Zones.Expect(*this, TEXT("Nonblocking overlap does not obstruct clearance"), true);
	auto* Director = World->SpawnActor<ACleaningDirectorActor>();
	ConfigureDirector(Director);
	Director->AdvanceSpawnScheduleForTesting(100000, {FVector(0, 0, 88)});
	BeginCleaning(World);
	auto* Registry = World->GetSubsystem<UCleaningWorldSubsystem>();
	TestEqual(TEXT("TRSH-001/018/019 representative: director actually spawns stain inside placement zone"),
			  Registry->GetActiveStainCountForZone(Zones.Stain), 1);
	TestEqual(TEXT("TRSH-001/018/019 representative: director actually spawns litter inside overlapping zone"),
			  Registry->GetActiveLitterCountForZone(Zones.Litter), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningSpawnTerrainTest, "BathhouseSim.Cleaning.Clearance.SlopeAndStep",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningSpawnTerrainTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("CleaningSpawnTerrain"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	FSpawnZones Zones(World);
	const float Pitch = 20;
	auto* Floor = StaticFloor(World, FTransform(FRotator(Pitch, 0, 0),
												FVector(0, 0, -20 / FMath::Cos(FMath::DegreesToRadians(Pitch))),
												FVector(10, 10, .4)));
	Zones.Expect(*this, TEXT("F2: 20 degree static mesh floor within five centimetre plane tolerance"), true);
	Set<FFloatProperty>(Zones.Stain, TEXT("MaximumFloorSlopeDegrees"), 15.f);
	Set<FFloatProperty>(Zones.Litter, TEXT("MaximumFloorSlopeDegrees"), 15.f);
	Zones.Expect(*this, TEXT("Authored slope limit remains enforced"), false);
	Set<FFloatProperty>(Zones.Stain, TEXT("MaximumFloorSlopeDegrees"), 25.f);
	Set<FFloatProperty>(Zones.Litter, TEXT("MaximumFloorSlopeDegrees"), 25.f);
	Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Two adjacent slabs share one mesh and one collision component. The ray hits the lower slab;
	// the raised slab starts 10 cm away and its top reaches the clearance floor offset.
	const float FloorOffset = GetDefault<ACleaningDirectorActor>()->GetSpawnClearanceFloorOffsetCm();
	auto* SteppedMesh = DuplicateObject<UStaticMesh>(Floor->GetStaticMesh(), GetTransientPackage());
	auto* Setup = NewObject<UBodySetup>(SteppedMesh);
	Setup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
	FKBoxElem Lower;
	Lower.Center = FVector(-245, 0, -10);
	Lower.X = 510;
	Lower.Y = 1000;
	Lower.Z = 20;
	FKBoxElem Raised;
	Raised.Center = FVector(255, 0, FloorOffset - 10);
	Raised.X = 490;
	Raised.Y = 1000;
	Raised.Z = 20;
	Setup->AggGeom.BoxElems = {Lower, Raised};
	SteppedMesh->SetBodySetup(Setup);
	auto* Step = StaticFloor(World, FTransform::Identity);
	Step->SetStaticMesh(SteppedMesh);
	Step->RecreatePhysicsState();
	Zones.Expect(*this, TEXT("F2: floor-offset-high step on same mesh component is ignored"), true);
	Step->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box(World, FVector(0, 0, -10), FVector(500, 500, 10))->SetMobility(EComponentMobility::Static);
	// The lip is 20 cm tall (0.2 scale of the 100 cm cube): its top is half a centimetre above the offset.
	auto* OtherStep = StaticFloor(World, FTransform(FVector(12, 0, FloorOffset + 0.5f - 10)));
	OtherStep->SetWorldScale3D(FVector(.1, .4, .2));
	Zones.Expect(*this, TEXT("Separate mesh lip above the floor offset remains blocking"), false);
	OtherStep->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningSpawnPhysicalTest, "BathhouseSim.Cleaning.Clearance.PhysicalObstaclesAndWall",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningSpawnPhysicalTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("CleaningSpawnPhysical"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	auto* Floor = Box(World, FVector(0, 0, -20), FVector(500, 500, 20));
	Floor->SetMobility(EComponentMobility::Static);
	FSpawnZones Zones(World);
	auto* Towel = World->SpawnActor<AWorldUsedTowelActor>();
	Mesh(Towel, FVector(.2, .2, .03));
	Towel->SetActorLocation(FVector(0, 0, 1.5));
	auto* TowelMesh = Towel->FindComponentByClass<UStaticMeshComponent>();
	TowelMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TestEqual(TEXT("Actual towel profile retained"), TowelMesh->GetCollisionProfileName(),
			  FName(TEXT("BlockAllDynamic")));
	Zones.Expect(*this, TEXT("TRSH-004: three-centimetre used towel top is not static terrain"), false);
	Towel->SetActorLocation(FVector(20, 0, 1.5));
	Zones.Expect(*this, TEXT("Used towel beside candidate blocks clearance"), false);
	Towel->Destroy();
	auto* Plate = Box(World, FVector(0, 0, 1.5), FVector(10, 10, 1.5));
	Plate->SetMobility(EComponentMobility::Movable);
	Plate->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Zones.Expect(*this, TEXT("Three-centimetre movable plate top rejected before floor exclusion"), false);
	Plate->SetCollisionObjectType(ECC_WorldStatic);
	Zones.Expect(*this, TEXT("WorldStatic object with Movable mobility still rejected"), false);
	Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	auto* Key = World->SpawnActor<ABathhouseKeyActor>();
	Mesh(Key, FVector(.2, .2, .03));
	Key->SetActorLocation(FVector(0, 0, 1.5));
	Key->FindComponentByClass<UStaticMeshComponent>()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Zones.Expect(*this, TEXT("PhysicsActor key top rejected"), false);
	Key->Destroy();
	auto* Item = World->SpawnActor<APlaceableFacilityItemActor>();
	Mesh(Item, FVector(.2, .2, .03));
	Item->SetActorLocation(FVector(20, 0, 1.5));
	Item->FindComponentByClass<UStaticMeshComponent>()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Zones.Expect(*this, TEXT("PhysicsActor facility item beside candidate blocks clearance"), false);
	Item->Destroy();
	auto* Kind = MakeKind(TEXT("ClearanceBox"), TEXT("Box"), 1, 0);
	auto* ItemBox = SpawnBox(World, Kind, 1, FVector(0, 0, 50));
	Zones.Expect(*this, TEXT("Physical item box top rejected"), false);
	ItemBox->Destroy();
	auto* Facility = World->SpawnActor<AServiceAutomationDisplayFacility>();
	auto* Body = Box(World, FVector(12, 0, 15), FVector(5, 20, 15), Facility);
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Zones.Expect(*this, TEXT("BlockAllDynamic facility body beside candidate blocks clearance"), false);
	Body->SetWorldLocation(FVector(0, 0, -10));
	Zones.Expect(*this, TEXT("Facility top within plane tolerance rejected"), false);
	Facility->Destroy();
	auto* Wall = Box(World, FVector(12, 0, 15), FVector(2, 100, 15));
	Zones.Expect(*this, TEXT("TRSH-030: wall within radius remains blocking"), false);
	Wall->SetMobility(EComponentMobility::Movable);
	Wall->SetWorldLocation(FVector(40, 0, 15));
	Wall->SetMobility(EComponentMobility::Static);
	Zones.Expect(*this, TEXT("TRSH-030: wall beyond radius allows spawn"), true);
	Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Floor->SetMobility(EComponentMobility::Movable);
	Floor->SetWorldLocation(FVector(0, 0, -40));
	Floor->SetMobility(EComponentMobility::Static);
	Zones.Expect(*this, TEXT("TRSH-004: lower bath floor remains outside tolerance"), false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningSpawnPawnTest, "BathhouseSim.Cleaning.Clearance.PawnsAndRagdoll",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningSpawnPawnTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("CleaningSpawnPawns"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	Box(World, FVector(0, 0, -20), FVector(500, 500, 20))->SetMobility(EComponentMobility::Static);
	FSpawnZones Zones(World);
	auto* Player =
		World->SpawnActor<AFirstPersonCharacter>(AFirstPersonCharacter::StaticClass(), FTransform(FVector(0, 0, 96)));
	Zones.Expect(*this, TEXT("TRSH-029: standing player feet allow spawn"), true);
	Player->SetActorLocation(FVector(45, 0, 96));
	Zones.Expect(*this, TEXT("TRSH-029: immediately beside standing player allows spawn"), true);
	Player->Destroy();
	auto* Guest = Customer(World, FVector(0, 0, 88));
	Zones.Expect(*this, TEXT("TRSH-029: standing customer feet allow spawn"), true);
	Guest->SetActorLocation(FVector(45, 0, 88));
	Zones.Expect(*this, TEXT("TRSH-029: immediately beside standing customer allows spawn"), true);
	Guest->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	auto* Ragdoll = Guest->GetMesh();
	auto* SkeletalMesh = LoadObject<USkeletalMesh>(
		nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	auto* PhysicsAsset =
		LoadObject<UPhysicsAsset>(nullptr, TEXT("/Game/Characters/Mannequins/Rigs/PA_Mannequin.PA_Mannequin"));
	if (!TestNotNull(TEXT("Ragdoll skeletal mesh loads"), SkeletalMesh) ||
		!TestNotNull(TEXT("Ragdoll physics asset loads"), PhysicsAsset))
	{
		return false;
	}
	Ragdoll->SetSkeletalMesh(SkeletalMesh);
	Ragdoll->SetPhysicsAsset(PhysicsAsset);
	Ragdoll->SetCollisionProfileName(TEXT("Ragdoll"));
	Ragdoll->SetWorldTransform(FTransform(FRotator(0, 0, 90), FVector(0, 0, 20), FVector(.3)));
	Ragdoll->SetSimulatePhysics(true);
	TestTrue(TEXT("Ragdoll skeletal mesh has physics bodies"), Ragdoll->IsSimulatingPhysics());
	FCollisionResponseParams Responses(ECR_Block);
	TArray<FOverlapResult> Hits;
	World->OverlapMultiByChannel(Hits, FVector(0, 0, 16), FQuat::Identity, ECC_PhysicsBody,
								 FCollisionShape::MakeBox(FVector(15, 15, 15)),
								 FCollisionQueryParams::DefaultQueryParam, Responses);
	bool bRagdollBlocksQuery = false;
	for (const auto& Hit : Hits)
	{
		bRagdollBlocksQuery |= Hit.GetComponent() == Ragdoll && Hit.bBlockingHit;
	}
	TestTrue(TEXT("Regression fixture: ragdoll actually overlaps and blocks PhysicsBody clearance"),
			 bRagdollBlocksQuery);
	Zones.Expect(*this, TEXT("TRSH-029: overlapping ragdoll PhysicsBody is ignored by owner Pawn"), true);
	return true;
}
#endif
