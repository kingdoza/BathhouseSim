#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include <limits>

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Interaction/PlayerViewFrontPlacement.h"
#include "Placement/FacilityPlacementCollisionUtils.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Service/ItemBoxActor.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Shop/ShopSettings.h"
#include "Shop/ShopUnboxingCluster.h"
#include "Shop/ShopUnboxingPlacement.h"
#include "Shop/ShopUnboxingTuning.h"
#include "Tests/ShopUnboxShapeTestSupport.h"

// USV-001..USV-015, USV-021 and the value-source contract for the camera view-front unboxing placement.
// Every expectation is computed from FShopUnboxingTuning::FromSettings (the runtime's own single read point) or
// from the fixture geometry; fixture offsets are expressed relative to those settings values.
namespace
{
const TCHAR* const DefinitionPaths[] =
{
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Bath.DA_FacilityPlacement_Bath"),
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Shower.DA_FacilityPlacement_Shower"),
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Washer.DA_FacilityPlacement_Washer"),
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Dryer.DA_FacilityPlacement_Dryer"),
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler.DA_FacilityPlacement_Boiler"),
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Cooler.DA_FacilityPlacement_Cooler"),
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Circulator.DA_FacilityPlacement_Circulator")
};
constexpr int32 ShowerIndex = 1;
constexpr int32 BathIndex = 0;
constexpr float DistanceTolerance = 0.6f;
constexpr int32 SeedCount = 20;

struct FResultBox
{
	FVector Center = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
	FVector HalfExtent = FVector::ZeroVector;

	void GetCorners(TArray<FVector>& OutCorners) const
	{
		for (const float X : { -1.0f, 1.0f })
		{
			for (const float Y : { -1.0f, 1.0f })
			{
				for (const float Z : { -1.0f, 1.0f })
				{
					OutCorners.Add(Center + Rotation.RotateVector(
						FVector(X * HalfExtent.X, Y * HalfExtent.Y, Z * HalfExtent.Z)));
				}
			}
		}
	}

	bool Contains(const FVector& Point) const
	{
		const FVector Local = Rotation.UnrotateVector(Point - Center);
		return FMath::Abs(Local.X) <= HalfExtent.X && FMath::Abs(Local.Y) <= HalfExtent.Y
			&& FMath::Abs(Local.Z) <= HalfExtent.Z;
	}
};

struct FViewFrontFixture
{
	FWorldContext* Context = nullptr;
	UWorld* World = nullptr;
	ACharacter* Player = nullptr;
	AShopDeliveryBoxActor* Box = nullptr;
	TArray<UFacilityPlacementDefinition*> Definitions;
	TArray<FShopUnboxItemShape> Shapes; // facility shapes followed by optional item box shapes

	bool Initialize(FAutomationTestBase& Test, const TCHAR* Name, const bool bSimulatePhysics = false,
		const bool bAddFloor = true, const FVector& PlayerLocation = FVector(0.0f, 0.0f, 1000.0f))
	{
		if (!GEngine)
		{
			Test.AddError(TEXT("GEngine is required."));
			return false;
		}
		Context = &GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(
			EWorldType::Game, false, MakeUniqueObjectName(nullptr, UWorld::StaticClass(), Name), GetTransientPackage());
		if (!World)
		{
			GEngine->DestroyWorldContext(World);
			Context = nullptr;
			Test.AddError(TEXT("Failed to create the test world."));
			return false;
		}
		World->AddToRoot();
		Context->SetCurrentWorld(World);
		if (bSimulatePhysics)
		{
			World->bShouldSimulatePhysics = true;
		}
		World->InitializeActorsForPlay(FURL());
		if (bSimulatePhysics)
		{
			World->BeginPlay();
		}
		Player = World->SpawnActor<ACharacter>(ACharacter::StaticClass(), PlayerLocation, FRotator::ZeroRotator);
		Box = World->SpawnActor<AShopDeliveryBoxActor>(
			AShopDeliveryBoxActor::StaticClass(), FVector(90000.0f, 90000.0f, 1000.0f), FRotator::ZeroRotator);
		Test.TestNotNull(TEXT("Player exists"), Player);
		Test.TestNotNull(TEXT("Box exists"), Box);
		if (!Player || !Box || !Player->GetCapsuleComponent())
		{
			return false;
		}
		if (bAddFloor)
		{
			AddBlocker(TEXT("ViewFrontFloor"), GetFoot() - FVector::UpVector * 50.0f, FVector(20000.0f, 20000.0f, 50.0f));
		}
		return true;
	}

	~FViewFrontFixture()
	{
		if (World)
		{
			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
			World->RemoveFromRoot();
		}
	}

	FVector GetFoot() const
	{
		return Player->GetActorLocation() - FVector::UpVector * Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}

	FVector GetCamera() const { return Player->GetActorLocation() + FVector(0.0f, 0.0f, Player->BaseEyeHeight); }

	UBoxComponent* AddBlocker(const TCHAR* Name, const FVector& Center, const FVector& HalfExtent,
		const bool bBlockVisibility = true, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Rotation, Center));
		if (!Owner)
		{
			return nullptr;
		}
		UBoxComponent* Component = NewObject<UBoxComponent>(Owner, Name);
		Owner->SetRootComponent(Component);
		Owner->AddInstanceComponent(Component);
		Component->SetBoxExtent(HalfExtent);
		Component->SetCollisionObjectType(ECC_WorldStatic);
		Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Component->SetCollisionResponseToAllChannels(ECR_Block);
		if (!bBlockVisibility)
		{
			Component->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		}
		Component->RegisterComponent();
		Owner->SetActorTransform(FTransform(Rotation, Center), false, nullptr, ETeleportType::TeleportPhysics);
		Component->UpdateComponentToWorld();
		return Component;
	}

	/** Thin wall across the view axis at DistanceFromCameraCm along the horizontal forward direction. */
	UBoxComponent* AddWallAhead(const TCHAR* Name, const float DistanceFromCameraCm, const bool bBlockVisibility = true)
	{
		return AddBlocker(Name, GetCamera() + FVector(DistanceFromCameraCm + 0.5f, 0.0f, 0.0f),
			FVector(0.5f, 3000.0f, 3000.0f), bBlockVisibility);
	}

	/** Thin plate at eye level in front of the camera: blocks the view-front stage's line of sight only. */
	UBoxComponent* AddViewStageBlocker(const FShopUnboxingTuning& Tuning)
	{
		return AddBlocker(TEXT("ViewStageBlocker"),
			GetCamera() + FVector(Tuning.ViewMinDistanceCm * 0.5f, 0.0f, 0.0f), FVector(0.5f, 3000.0f, 20.0f));
	}

	bool Load(FAutomationTestBase& Test, const TArray<int32>& Indices)
	{
		Definitions.Reset();
		for (const int32 Index : Indices)
		{
			UFacilityPlacementDefinition* Definition = LoadObject<UFacilityPlacementDefinition>(
				nullptr, DefinitionPaths[Index % UE_ARRAY_COUNT(DefinitionPaths)]);
			if (!Test.TestNotNull(TEXT("Definition loads"), Definition))
			{
				return false;
			}
			Definitions.Add(Definition);
		}
		Shapes = ShopUnboxTest::MakeShapes(Definitions);
		return true;
	}

	/** Appends item box shapes (the configured item box class, falling back to the native class). */
	bool AddItemBoxShapes(FAutomationTestBase& Test, const int32 Count)
	{
		TSubclassOf<AItemBoxActor> BoxClass = GetDefault<UShopSettings>()->LoadItemBoxClass();
		if (!BoxClass)
		{
			BoxClass = AItemBoxActor::StaticClass();
		}
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FShopUnboxItemShape Shape;
			FText Failure;
			if (!Test.TestTrue(TEXT("Item box shape resolves"), FShopUnboxItemShape::FromItemBoxClass(BoxClass, Shape, Failure)))
			{
				return false;
			}
			Shapes.Add(Shape);
		}
		return true;
	}

	bool Find(const FShopUnboxingPlacementRequest& Request, const FShopUnboxingTuning& Tuning, const int32 Seed,
		TArray<FTransform>& OutTransforms, EShopUnboxPlacementStage& OutStage, FText& OutFailure)
	{
		FRandomStream Stream(Seed);
		return FShopUnboxingPlacement::FindSpawnTransforms(*World, *Player, *Player->GetCapsuleComponent(), *Box,
			Shapes, Request, Tuning, Stream, OutTransforms, OutFailure, &OutStage);
	}

	bool ResolveBoxes(const TArray<FTransform>& Transforms, TArray<FResultBox>& OutBoxes) const
	{
		OutBoxes.Reset();
		if (Transforms.Num() != Shapes.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Transforms.Num(); ++Index)
		{
			FResultBox& Result = OutBoxes.AddDefaulted_GetRef();
			FCollisionShape Shape;
			const UPrimitiveComponent* Template = nullptr;
			FText Failure;
			if (!Shapes[Index].BuildCollisionQuery
				|| !Shapes[Index].BuildCollisionQuery(Transforms[Index], Result.Center, Result.Rotation, Shape, Template, Failure))
			{
				return false;
			}
			Result.HalfExtent = Shape.GetExtent().GetAbs();
		}
		return true;
	}
};

FShopUnboxingPlacementRequest MakeRequestAt(const FViewFrontFixture& Fixture, const float PitchDegrees)
{
	return ShopUnboxTest::MakeRequest(*Fixture.Player, FRotator(PitchDegrees, 0.0f, 0.0f).Vector());
}

void GetAllCorners(const TArray<FResultBox>& Boxes, TArray<FVector>& OutCorners)
{
	for (const FResultBox& Box : Boxes)
	{
		Box.GetCorners(OutCorners);
	}
}

float GetMinProjection(const TArray<FResultBox>& Boxes, const FVector& Origin, const FVector& Axis)
{
	TArray<FVector> Corners;
	GetAllCorners(Boxes, Corners);
	float Min = TNumericLimits<float>::Max();
	for (const FVector& Corner : Corners)
	{
		Min = FMath::Min(Min, static_cast<float>(FVector::DotProduct(Corner - Origin, Axis)));
	}
	return Min;
}

float GetMaxZ(const TArray<FResultBox>& Boxes)
{
	TArray<FVector> Corners;
	GetAllCorners(Boxes, Corners);
	float Max = -TNumericLimits<float>::Max();
	for (const FVector& Corner : Corners)
	{
		Max = FMath::Max(Max, static_cast<float>(Corner.Z));
	}
	return Max;
}

float GetMinZ(const TArray<FResultBox>& Boxes)
{
	TArray<FVector> Corners;
	GetAllCorners(Boxes, Corners);
	float Min = TNumericLimits<float>::Max();
	for (const FVector& Corner : Corners)
	{
		Min = FMath::Min(Min, static_cast<float>(Corner.Z));
	}
	return Min;
}

bool IsCameraInsideAnyBox(const TArray<FResultBox>& Boxes, const FVector& Camera)
{
	for (const FResultBox& Box : Boxes)
	{
		if (Box.Contains(Camera))
		{
			return true;
		}
	}
	return false;
}

bool IsOnPullList(const float DistanceCm, const TArray<float>& Distances)
{
	for (const float Entry : Distances)
	{
		if (FMath::Abs(Entry - DistanceCm) <= DistanceTolerance)
		{
			return true;
		}
	}
	return false;
}

TArray<int32> RepeatIndices(const int32 Count)
{
	TArray<int32> Indices;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Indices.Add(Index);
	}
	return Indices;
}

const TCHAR* StageName(const EShopUnboxPlacementStage Stage)
{
	switch (Stage)
	{
	case EShopUnboxPlacementStage::ViewFront: return TEXT("ViewFront");
	case EShopUnboxPlacementStage::FloorFront: return TEXT("FloorFront");
	case EShopUnboxPlacementStage::Overhead: return TEXT("Overhead");
	case EShopUnboxPlacementStage::FinalStack: return TEXT("FinalStack");
	default: return TEXT("None");
	}
}
}

// USV-001, 002, 012, 015
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxViewFrontOpenAutomationTest,
	"BathhouseSim.Shop.UnboxViewFront.OpenAndRepresentative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxViewFrontOpenAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FViewFrontFixture Fixture;
	if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontOpenWorld")))
	{
		return false;
	}
	const FShopUnboxingTuning Tuning = ShopUnboxTest::MakeTuning();
	const FShopUnboxingPlacementRequest Request = MakeRequestAt(Fixture, 0.0f);
	const FVector Camera = Fixture.GetCamera();
	const FVector View = Request.CameraDirection.GetSafeNormal();
	TArray<float> Distances;
	PlayerViewFrontPlacement::BuildPullDistances(
		Tuning.ViewDistanceCm, Tuning.ViewMinDistanceCm, Tuning.ViewPullStepCm, Distances);

	const auto RunCase = [&](const TCHAR* Label, const TArray<int32>& Indices, const bool bRequireViewFront,
		const bool bExpectExactDistance, const int32 ItemBoxCount = 0)
	{
		if (!Fixture.Load(*this, Indices) || !Fixture.AddItemBoxShapes(*this, ItemBoxCount))
		{
			return;
		}
		int32 ViewFrontCount = 0;
		for (int32 Seed = 1; Seed <= SeedCount; ++Seed)
		{
			TArray<FTransform> Transforms;
			EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
			FText Failure;
			TestTrue(FString::Printf(TEXT("%s seed %d places"), Label, Seed),
				Fixture.Find(Request, Tuning, Seed, Transforms, Stage, Failure));
			TArray<FResultBox> Boxes;
			if (!TestTrue(TEXT("Result boxes resolve"), Fixture.ResolveBoxes(Transforms, Boxes)))
			{
				continue;
			}
			TestFalse(FString::Printf(TEXT("%s seed %d: camera is in no item"), Label, Seed),
				IsCameraInsideAnyBox(Boxes, Camera));
			if (Stage == EShopUnboxPlacementStage::ViewFront)
			{
				++ViewFrontCount;
				const float MinView = GetMinProjection(Boxes, Camera, View);
				TestTrue(FString::Printf(TEXT("%s seed %d: nearest part is on the pull distance list (%.2f)"),
					Label, Seed, MinView), IsOnPullList(MinView, Distances));
				if (bExpectExactDistance)
				{
					TestTrue(FString::Printf(TEXT("%s seed %d: nearest part equals the view distance setting"), Label, Seed),
						FMath::Abs(MinView - Tuning.ViewDistanceCm) <= DistanceTolerance);
				}
			}
			else if (bRequireViewFront)
			{
				AddError(FString::Printf(TEXT("%s seed %d expected ViewFront but got %s"), Label, Seed, StageName(Stage)));
			}
		}
		AddInfo(FString::Printf(TEXT("UnboxViewFront %s: ViewFront %d/%d"), Label, ViewFrontCount, SeedCount));
		if (!bRequireViewFront)
		{
			TestTrue(FString::Printf(TEXT("%s: most seeds resolve in front of the camera"), Label),
				ViewFrontCount * 2 >= SeedCount);
		}
	};

	RunCase(TEXT("one shower"), { ShowerIndex }, true, true);
	RunCase(TEXT("four representative items"), { 0, 1, 2, 3 }, true, true);

	// Center of a lone item lines up with the camera on the screen vertical.
	if (Fixture.Load(*this, { ShowerIndex }))
	{
		TArray<FTransform> Transforms;
		EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
		FText Failure;
		if (TestTrue(TEXT("Single shower places"), Fixture.Find(Request, Tuning, 7, Transforms, Stage, Failure)))
		{
			TArray<FResultBox> Boxes;
			if (Fixture.ResolveBoxes(Transforms, Boxes))
			{
				TestTrue(TEXT("Single shower center height equals the camera height"),
					FMath::Abs(Boxes[0].Center.Z - Camera.Z) <= DistanceTolerance);
				TestTrue(TEXT("Single shower center is on the view ray"),
					FMath::Abs(Boxes[0].Center.Y - Camera.Y) <= DistanceTolerance);
			}
		}
	}

	// USV-015: facilities mixed with item boxes (the only non-cuboid shape), every seed in front of the camera.
	RunCase(TEXT("mixed facilities and item boxes"), { 0, 1, 2 }, true, true, 3);

	// Maximum quantity of facilities: regardless of stage the camera is never wrapped.
	const int32 MaxQuantity = GetDefault<UShopSettings>()->GetCartTotalQuantityLimit();
	RunCase(TEXT("maximum quantity"), RepeatIndices(MaxQuantity), false, false);
	return true;
}

// USV-003..006
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxViewFrontPitchAutomationTest,
	"BathhouseSim.Shop.UnboxViewFront.PitchAndFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxViewFrontPitchAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FViewFrontFixture Fixture;
	if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontPitchWorld")) || !Fixture.Load(*this, { ShowerIndex }))
	{
		return false;
	}
	const FShopUnboxingTuning Tuning = ShopUnboxTest::MakeTuning();
	const FVector Camera = Fixture.GetCamera();
	const float FloorTop = Fixture.GetFoot().Z;
	TArray<float> Distances;
	PlayerViewFrontPlacement::BuildPullDistances(
		Tuning.ViewDistanceCm, Tuning.ViewMinDistanceCm, Tuning.ViewPullStepCm, Distances);
	const float CapsuleRadius = Fixture.Player->GetCapsuleComponent()->GetScaledCapsuleRadius();

	for (const float Pitch : { -45.0f, -80.0f, 45.0f })
	{
		const FShopUnboxingPlacementRequest Request = MakeRequestAt(Fixture, Pitch);
		const FVector View = Request.CameraDirection.GetSafeNormal();
		for (int32 Seed = 1; Seed <= SeedCount; ++Seed)
		{
			TArray<FTransform> Transforms;
			EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
			FText Failure;
			TestTrue(TEXT("Pitched view places"), Fixture.Find(Request, Tuning, Seed, Transforms, Stage, Failure));
			TArray<FResultBox> Boxes;
			if (!Fixture.ResolveBoxes(Transforms, Boxes))
			{
				continue;
			}
			const FString Label = FString::Printf(TEXT("pitch %.0f seed %d"), Pitch, Seed);
			TestFalse(*(Label + TEXT(": camera is in no item")), IsCameraInsideAnyBox(Boxes, Camera));
			if (Pitch != -80.0f)
			{
				TestTrue(*(Label + TEXT(": resolves in front of the camera")), Stage == EShopUnboxPlacementStage::ViewFront);
			}
			if (Stage != EShopUnboxPlacementStage::ViewFront)
			{
				continue;
			}
			const float MinView = GetMinProjection(Boxes, Camera, View);
			TestTrue(*(Label + FString::Printf(TEXT(": nearest part %.2f is on the pull list"), MinView)),
				IsOnPullList(MinView, Distances));
			TestTrue(*(Label + TEXT(": nothing below the floor surface")), GetMinZ(Boxes) > FloorTop - 0.5f);
			if (Pitch > 0.0f)
			{
				TestTrue(*(Label + TEXT(": center is above the camera")), Boxes[0].Center.Z > Camera.Z);
			}
			if (Pitch == -80.0f)
			{
				// Near the feet the player's body area is allowed; the item may overlap the player capsule footprint.
				const float Horizontal = FVector::Dist2D(Boxes[0].Center, Camera);
				TestTrue(*(Label + TEXT(": center is near the player footprint")),
					Horizontal < CapsuleRadius + Boxes[0].HalfExtent.GetMax() + Tuning.ViewDistanceCm);
			}
		}
	}

	// Floor-front lowest surface (representative four, shallow view blocked by the eye-level plate).
	FViewFrontFixture FloorFixture;
	if (FloorFixture.Initialize(*this, TEXT("UnboxViewFrontFloorHeightWorld")) && FloorFixture.Load(*this, { 0, 1, 2, 3 }))
	{
		FloorFixture.AddViewStageBlocker(Tuning);
		const FShopUnboxingPlacementRequest Request = MakeRequestAt(FloorFixture, 0.0f);
		for (int32 Seed = 1; Seed <= SeedCount; ++Seed)
		{
			TArray<FTransform> Transforms;
			EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
			FText Failure;
			TestTrue(TEXT("Blocked view places"), FloorFixture.Find(Request, Tuning, Seed, Transforms, Stage, Failure));
			TArray<FResultBox> Boxes;
			if (Stage == EShopUnboxPlacementStage::FloorFront && FloorFixture.ResolveBoxes(Transforms, Boxes))
			{
				TestTrue(TEXT("Floor-front lowest surface is at the configured height above the feet"),
					FMath::Abs(GetMinZ(Boxes) - (FloorFixture.GetFoot().Z + Tuning.ForwardFloorClearanceCm)) <= DistanceTolerance);
			}
		}
	}
	return true;
}

// USV-007..010
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxViewFrontBlockedAutomationTest,
	"BathhouseSim.Shop.UnboxViewFront.BlockedStages",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxViewFrontBlockedAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FShopUnboxingTuning Tuning = ShopUnboxTest::MakeTuning();

	// Low ceiling and an upward view with four items: nothing may reach the ceiling.
	{
		FViewFrontFixture Fixture;
		if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontCeilingWorld")) || !Fixture.Load(*this, { 0, 1, 2, 3 }))
		{
			return false;
		}
		const float CeilingBottom = Fixture.GetCamera().Z + Tuning.ViewMinDistanceCm;
		Fixture.AddBlocker(TEXT("LowCeiling"), FVector(0.0f, 0.0f, CeilingBottom + 1.0f), FVector(5000.0f, 5000.0f, 1.0f));
		const FShopUnboxingPlacementRequest Request = MakeRequestAt(Fixture, 60.0f);
		for (int32 Seed = 1; Seed <= SeedCount; ++Seed)
		{
			TArray<FTransform> Transforms;
			EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
			FText Failure;
			TestTrue(TEXT("Low ceiling placement is always possible"), Fixture.Find(Request, Tuning, Seed, Transforms, Stage, Failure));
			TArray<FResultBox> Boxes;
			if (!Fixture.ResolveBoxes(Transforms, Boxes))
			{
				continue;
			}
			if (Stage != EShopUnboxPlacementStage::FinalStack)
			{
				TestTrue(FString::Printf(TEXT("Low ceiling seed %d (%s): every top plus clearance is below the ceiling"),
					Seed, StageName(Stage)), GetMaxZ(Boxes) + Tuning.OverlapDepthCm * 0.5f < CeilingBottom + 0.5f);
			}
			TestTrue(TEXT("Low ceiling upward view does not stay in the view-front stage above the ceiling"),
				Stage != EShopUnboxPlacementStage::ViewFront || GetMaxZ(Boxes) < CeilingBottom);
		}
	}

	// A wall and a lone shower within the view distance: the cluster is pulled to this side of the wall.
	{
		FViewFrontFixture Fixture;
		if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontWallWorld")) || !Fixture.Load(*this, { ShowerIndex }))
		{
			return false;
		}
		FCollisionShape Probe;
		FVector ProbeCenter;
		FQuat ProbeRotation;
		const UPrimitiveComponent* Template = nullptr;
		FText Failure;
		FVector ItemScale = FVector::OneVector;
		if (!TestTrue(TEXT("Shower item scale resolves"),
				APlaceableFacilityItemActor::GetDefinitionItemScale(*Fixture.Definitions[0], ItemScale, Failure))
			|| !TestTrue(TEXT("Shower collision query resolves"), APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
			*Fixture.Definitions[0], FTransform(FRotator::ZeroRotator, FVector::ZeroVector, ItemScale), ProbeCenter,
			ProbeRotation, Probe, Template, Failure)))
		{
			return false;
		}
		const FVector ProbeExtent = Probe.GetExtent().GetAbs();
		// The nearest part at the view distance always reaches past the wall face, so the first distance is blocked.
		const float WallDistance = Tuning.ViewDistanceCm + 2.0f * FMath::Min(ProbeExtent.X, ProbeExtent.Y) - 1.0f;
		Fixture.AddWallAhead(TEXT("NearWall"), WallDistance);
		const FShopUnboxingPlacementRequest Request = MakeRequestAt(Fixture, 0.0f);
		const FVector Camera = Fixture.GetCamera();
		for (int32 Seed = 1; Seed <= SeedCount; ++Seed)
		{
			TArray<FTransform> Transforms;
			EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
			TestTrue(TEXT("Wall placement succeeds"), Fixture.Find(Request, Tuning, Seed, Transforms, Stage, Failure));
			TArray<FResultBox> Boxes;
			if (!Fixture.ResolveBoxes(Transforms, Boxes))
			{
				continue;
			}
			TestTrue(FString::Printf(TEXT("Wall seed %d resolves in front of the camera"), Seed),
				Stage == EShopUnboxPlacementStage::ViewFront);
			const float MinView = GetMinProjection(Boxes, Camera, Request.CameraDirection.GetSafeNormal());
			TestTrue(FString::Printf(TEXT("Wall seed %d: pulled nearer than the view distance (%.2f)"), Seed, MinView),
				MinView < Tuning.ViewDistanceCm - 0.1f && MinView >= Tuning.ViewMinDistanceCm - DistanceTolerance);
			TArray<FVector> Corners;
			GetAllCorners(Boxes, Corners);
			for (const FVector& Corner : Corners)
			{
				TestTrue(TEXT("Every corner is on this side of the wall including the clearance"),
					Corner.X - Camera.X + Tuning.OverlapDepthCm <= WallDistance + DistanceTolerance);
			}
		}
	}

	// A close wall inside the minimum distance and four items: nothing is placed beyond the wall.
	{
		FViewFrontFixture Fixture;
		if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontCloseWallWorld")) || !Fixture.Load(*this, { 0, 1, 2, 3 }))
		{
			return false;
		}
		const float WallDistance = Tuning.ViewMinDistanceCm * 0.5f;
		Fixture.AddWallAhead(TEXT("CloseWall"), WallDistance);
		const FShopUnboxingPlacementRequest Request = MakeRequestAt(Fixture, 0.0f);
		const FVector Camera = Fixture.GetCamera();
		for (int32 Seed = 1; Seed <= SeedCount; ++Seed)
		{
			TArray<FTransform> Transforms;
			EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
			FText Failure;
			TestTrue(TEXT("Close wall placement succeeds"), Fixture.Find(Request, Tuning, Seed, Transforms, Stage, Failure));
			TestTrue(TEXT("Close wall never uses the view-front stage"), Stage != EShopUnboxPlacementStage::ViewFront);
			TArray<FResultBox> Boxes;
			if (Fixture.ResolveBoxes(Transforms, Boxes) && Stage != EShopUnboxPlacementStage::FinalStack)
			{
				TArray<FVector> Corners;
				GetAllCorners(Boxes, Corners);
				for (const FVector& Corner : Corners)
				{
					TestTrue(FString::Printf(TEXT("Close wall seed %d (%s): nothing beyond the wall"), Seed, StageName(Stage)),
						Corner.X - Camera.X <= WallDistance + DistanceTolerance);
				}
			}
		}
	}

	// Four-sided tall cage around the player: only the overhead or final stack remain.
	{
		FViewFrontFixture Fixture;
		if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontCageWorld")) || !Fixture.Load(*this, { ShowerIndex }))
		{
			return false;
		}
		const float Reach = Tuning.ViewMinDistanceCm * 0.5f;
		const FVector Center = Fixture.Player->GetActorLocation();
		const float CageHalfHeight = 2000.0f;
		Fixture.AddBlocker(TEXT("CageFront"), Center + FVector(Reach, 0, 0), FVector(1.0f, 400.0f, CageHalfHeight));
		Fixture.AddBlocker(TEXT("CageBack"), Center - FVector(Reach, 0, 0), FVector(1.0f, 400.0f, CageHalfHeight));
		Fixture.AddBlocker(TEXT("CageLeft"), Center + FVector(0, Reach, 0), FVector(400.0f, 1.0f, CageHalfHeight));
		Fixture.AddBlocker(TEXT("CageRight"), Center - FVector(0, Reach, 0), FVector(400.0f, 1.0f, CageHalfHeight));
		const FShopUnboxingPlacementRequest Request = MakeRequestAt(Fixture, 0.0f);
		for (int32 Seed = 1; Seed <= SeedCount; ++Seed)
		{
			TArray<FTransform> Transforms;
			EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
			FText Failure;
			TestTrue(TEXT("Cage placement always succeeds"), Fixture.Find(Request, Tuning, Seed, Transforms, Stage, Failure));
			TestTrue(FString::Printf(TEXT("Cage seed %d resolves overhead or as the final stack (%s)"), Seed, StageName(Stage)),
				Stage == EShopUnboxPlacementStage::Overhead || Stage == EShopUnboxPlacementStage::FinalStack);
		}
	}
	return true;
}

// Stage-2 contract preserved: shallow-view block, XY and height of the floor-front row.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxViewFrontFloorFrontUnchangedAutomationTest,
	"BathhouseSim.Shop.UnboxViewFront.FloorFrontUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxViewFrontFloorFrontUnchangedAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FViewFrontFixture Fixture;
	if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontFloorUnchangedWorld")) || !Fixture.Load(*this, { ShowerIndex }))
	{
		return false;
	}
	FShopUnboxingTuning Tuning = ShopUnboxTest::MakeTuning();
	Fixture.AddViewStageBlocker(Tuning);
	const FShopUnboxingPlacementRequest Request = MakeRequestAt(Fixture, 0.0f);
	const FVector Foot = Fixture.GetFoot();

	// Overlap depth sweep from the property clamp metadata: every value stays on the floor-front stage.
	const FProperty* DepthProperty = UShopSettings::StaticClass()->FindPropertyByName(TEXT("UnboxOverlapDepthCm"));
	const float DepthMin = DepthProperty ? FCString::Atof(*DepthProperty->GetMetaData(TEXT("ClampMin"))) : 0.0f;
	const float DepthMax = DepthProperty ? FCString::Atof(*DepthProperty->GetMetaData(TEXT("ClampMax"))) : 0.0f;
	TestTrue(TEXT("Overlap depth property exposes a clamp range"), DepthMax > DepthMin);
	for (const float Alpha : { 0.0f, 0.15f, 0.4f, 0.7f, 1.0f })
	{
		Tuning.OverlapDepthCm = DepthMin + (DepthMax - DepthMin) * Alpha;
		for (const int32 Seed : { 11, 12, 13 })
		{
			TArray<FTransform> Transforms;
			EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
			FText Failure;
			TestTrue(TEXT("Blocked view still places"), Fixture.Find(Request, Tuning, Seed, Transforms, Stage, Failure));
			TestTrue(FString::Printf(TEXT("D=%.1f seed %d adopts the floor-front stage (%s)"),
				Tuning.OverlapDepthCm, Seed, StageName(Stage)), Stage == EShopUnboxPlacementStage::FloorFront);
			TArray<FResultBox> Boxes;
			if (Stage == EShopUnboxPlacementStage::FloorFront && Fixture.ResolveBoxes(Transforms, Boxes))
			{
				TestTrue(TEXT("Floor-front XY center is the forward distance from the feet"),
					FVector::Dist2D(Boxes[0].Center, Foot + FVector::ForwardVector * Tuning.ForwardDistanceCm) <= DistanceTolerance);
				TestTrue(TEXT("Floor-front lowest surface is the configured height above the feet"),
					FMath::Abs(GetMinZ(Boxes) - (Foot.Z + Tuning.ForwardFloorClearanceCm)) <= DistanceTolerance);
			}
		}
	}
	return true;
}

// USV-013, P10
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxViewFrontCameraClearanceAutomationTest,
	"BathhouseSim.Shop.UnboxViewFront.CameraClearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxViewFrontCameraClearanceAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FShopUnboxingTuning Tuning = ShopUnboxTest::MakeTuning();
	const int32 MaxQuantity = GetDefault<UShopSettings>()->GetCartTotalQuantityLimit();

	// (a) Steep downward view, no ceiling, maximum quantity of large items.
	{
		FViewFrontFixture Fixture;
		TArray<int32> Large;
		for (int32 Index = 0; Index < MaxQuantity; ++Index)
		{
			Large.Add(Index % 2 == 0 ? BathIndex : 4);
		}
		if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontClearanceWorld")) || !Fixture.Load(*this, Large))
		{
			return false;
		}
		const FShopUnboxingPlacementRequest Request = MakeRequestAt(Fixture, -80.0f);
		const FVector Camera = Fixture.GetCamera();
		const FVector Forward = FVector::ForwardVector;
		int32 PushedCount = 0;
		for (int32 Seed = 1; Seed <= SeedCount; ++Seed)
		{
			TArray<FTransform> Transforms;
			EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
			FText Failure;
			TestTrue(TEXT("Steep view places"), Fixture.Find(Request, Tuning, Seed, Transforms, Stage, Failure));
			TArray<FResultBox> Boxes;
			if (!Fixture.ResolveBoxes(Transforms, Boxes))
			{
				continue;
			}
			TestFalse(FString::Printf(TEXT("Steep seed %d (%s): camera is in no item"), Seed, StageName(Stage)),
				IsCameraInsideAnyBox(Boxes, Camera));
			if (Stage == EShopUnboxPlacementStage::FloorFront)
			{
				const float MinForward = GetMinProjection(Boxes, Camera, Forward);
				const bool bBelowCamera = GetMaxZ(Boxes) <= Camera.Z - Tuning.CameraClearanceCm + DistanceTolerance;
				const bool bSeparated = MinForward >= Tuning.CameraClearanceCm - DistanceTolerance;
				TestTrue(FString::Printf(TEXT("Steep seed %d: floor-front cluster is below or ahead of the camera"), Seed),
					bBelowCamera || bSeparated);
				TestTrue(TEXT("Floor-front lowest surface keeps the configured height"),
					FMath::Abs(GetMinZ(Boxes) - (Fixture.GetFoot().Z + Tuning.ForwardFloorClearanceCm))
						<= DistanceTolerance);
				if (!bBelowCamera && FMath::Abs(MinForward - Tuning.CameraClearanceCm) <= DistanceTolerance)
				{
					++PushedCount;
				}
			}
		}
		AddInfo(FString::Printf(TEXT("UnboxViewFront CameraClearance: push path used in %d/%d seeds"), PushedCount, SeedCount));
		TestTrue(TEXT("The camera-clearance push path runs at least once"), PushedCount >= 1);
	}

	// (b) Low ceiling at pitch 0 with the maximum quantity: the camera is never wrapped.
	{
		FViewFrontFixture Fixture;
		if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontLowCeilingClearanceWorld"))
			|| !Fixture.Load(*this, RepeatIndices(MaxQuantity)))
		{
			return false;
		}
		const FShopUnboxingPlacementRequest Request = MakeRequestAt(Fixture, 0.0f);
		const FVector Camera = Fixture.GetCamera();
		// A ceiling above the head but below what the maximum quantity needs in front of the camera.
		const float CeilingBottom = Camera.Z + Tuning.ViewDistanceCm;
		Fixture.AddBlocker(TEXT("LowCeiling"), FVector(0.0f, 0.0f, CeilingBottom + 1.0f), FVector(5000.0f, 5000.0f, 1.0f));
		for (int32 Seed = 1; Seed <= SeedCount; ++Seed)
		{
			TArray<FTransform> Transforms;
			EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
			FText Failure;
			TestTrue(TEXT("Low ceiling places"), Fixture.Find(Request, Tuning, Seed, Transforms, Stage, Failure));
			TArray<FResultBox> Boxes;
			if (!Fixture.ResolveBoxes(Transforms, Boxes))
			{
				continue;
			}
			// The final stack is the unchanged last resort (design: no camera rule); every other stage keeps the camera free.
			if (Stage != EShopUnboxPlacementStage::FinalStack)
			{
				TestFalse(FString::Printf(TEXT("Low ceiling seed %d (%s): camera is in no item"), Seed, StageName(Stage)),
					IsCameraInsideAnyBox(Boxes, Camera));
			}
			if (Stage == EShopUnboxPlacementStage::ViewFront)
			{
				TestTrue(TEXT("View-front nearest part respects the minimum"),
					GetMinProjection(Boxes, Camera, FVector::ForwardVector) >= Tuning.ViewMinDistanceCm - DistanceTolerance);
			}
			else if (Stage == EShopUnboxPlacementStage::FloorFront)
			{
				const bool bBelowCamera = GetMaxZ(Boxes) <= Camera.Z - Tuning.CameraClearanceCm + DistanceTolerance;
				TestTrue(TEXT("Floor-front cluster is separated from the camera"),
					bBelowCamera || GetMinProjection(Boxes, Camera, FVector::ForwardVector) >= Tuning.CameraClearanceCm - DistanceTolerance);
			}
			if (Stage != EShopUnboxPlacementStage::FinalStack)
			{
				TestTrue(TEXT("Every top plus clearance is below the ceiling"),
					GetMaxZ(Boxes) + Tuning.OverlapDepthCm * 0.5f < CeilingBottom + 0.5f);
			}
		}
	}
	return true;
}

// USV-011, USV-014
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxViewFrontGuestSeedAutomationTest,
	"BathhouseSim.Shop.UnboxViewFront.GuestAndSeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxViewFrontGuestSeedAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FViewFrontFixture Fixture;
	if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontGuestWorld")) || !Fixture.Load(*this, { ShowerIndex }))
	{
		return false;
	}
	const FShopUnboxingTuning Tuning = ShopUnboxTest::MakeTuning();
	const FShopUnboxingPlacementRequest Request = MakeRequestAt(Fixture, 0.0f);
	TArray<FTransform> Baseline;
	EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
	FText Failure;
	TestTrue(TEXT("Baseline places"), Fixture.Find(Request, Tuning, 31415, Baseline, Stage, Failure));
	TArray<FResultBox> BaselineBoxes;
	if (!Fixture.ResolveBoxes(Baseline, BaselineBoxes))
	{
		return false;
	}
	ACharacter* Guest = Fixture.World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(), BaselineBoxes[0].Center, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Guest is placed over the baseline candidate"), Guest))
	{
		return false;
	}
	TArray<FTransform> Safe;
	TestTrue(TEXT("Placement retries around the guest"), Fixture.Find(Request, Tuning, 31415, Safe, Stage, Failure));
	TArray<FResultBox> SafeBoxes;
	if (Fixture.ResolveBoxes(Safe, SafeBoxes))
	{
		FCollisionObjectQueryParams PawnObjects;
		PawnObjects.AddObjectTypesToQuery(ECC_Pawn);
		FCollisionQueryParams PawnParams(SCENE_QUERY_STAT(UnboxViewFrontGuestAssertion), false);
		PawnParams.AddIgnoredActor(Fixture.Player);
		const FVector Extent = SafeBoxes[0].HalfExtent;
		TestFalse(TEXT("Result clears the guest with the clearance shape"),
			Fixture.World->OverlapAnyTestByObjectType(
				SafeBoxes[0].Center + FVector(0.0f, 0.0f, Tuning.OverlapDepthCm * 0.5f), SafeBoxes[0].Rotation, PawnObjects,
				FCollisionShape::MakeBox(FVector(Extent.X + Tuning.OverlapDepthCm, Extent.Y + Tuning.OverlapDepthCm,
					Extent.Z + Tuning.OverlapDepthCm * 0.5f)),
				PawnParams));
	}
	Guest->Destroy();

	// Same seed reproduces, a different seed changes position or yaw.
	if (!Fixture.Load(*this, { ShowerIndex, ShowerIndex, ShowerIndex }))
	{
		return false;
	}
	TArray<FTransform> First;
	TArray<FTransform> Repeat;
	TArray<FTransform> Different;
	TestTrue(TEXT("First seed places"), Fixture.Find(Request, Tuning, 4242, First, Stage, Failure));
	TestTrue(TEXT("Repeat seed places"), Fixture.Find(Request, Tuning, 4242, Repeat, Stage, Failure));
	TestTrue(TEXT("Other seed places"), Fixture.Find(Request, Tuning, 4243, Different, Stage, Failure));
	bool bSame = First.Num() == Repeat.Num();
	bool bDifferent = false;
	for (int32 Index = 0; bSame && Index < First.Num(); ++Index)
	{
		bSame &= First[Index].Equals(Repeat[Index], 0.001f);
	}
	for (int32 Index = 0; Index < First.Num() && Index < Different.Num(); ++Index)
	{
		bDifferent |= !First[Index].Equals(Different[Index], 0.001f);
	}
	TestTrue(TEXT("Same seed gives the same result"), bSame);
	TestTrue(TEXT("Different seed gives a different layout"), bDifferent);
	return true;
}

// P2 environment clearance on the view-front stage
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxViewFrontEnvironmentAutomationTest,
	"BathhouseSim.Shop.UnboxViewFront.EnvironmentClearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxViewFrontEnvironmentAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FViewFrontFixture Fixture;
	if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontEnvironmentWorld")) || !Fixture.Load(*this, { ShowerIndex }))
	{
		return false;
	}
	const FShopUnboxingTuning Tuning = ShopUnboxTest::MakeTuning();
	const FShopUnboxingPlacementRequest Request = MakeRequestAt(Fixture, 0.0f);
	constexpr int32 Seed = 71129;
	TArray<FTransform> Baseline;
	EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
	FText Failure;
	TestTrue(TEXT("Baseline places"), Fixture.Find(Request, Tuning, Seed, Baseline, Stage, Failure));
	TArray<FResultBox> BaselineBoxes;
	if (!Fixture.ResolveBoxes(Baseline, BaselineBoxes))
	{
		return false;
	}
	const FResultBox& Base = BaselineBoxes[0];
	FCollisionShape Shape;
	FVector Center;
	FQuat Rotation;
	const UPrimitiveComponent* Template = nullptr;
	if (!APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
		*Fixture.Definitions[0], Baseline[0], Center, Rotation, Shape, Template, Failure))
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UnboxViewFrontEnvironmentAssertion), false);
	Params.AddIgnoredActor(Fixture.Player);
	Params.AddIgnoredActor(Fixture.Box);
	const auto ClearanceShape = [&](const FResultBox& Box)
	{
		return FCollisionShape::MakeBox(FVector(Box.HalfExtent.X + Tuning.OverlapDepthCm,
			Box.HalfExtent.Y + Tuning.OverlapDepthCm, Box.HalfExtent.Z + Tuning.OverlapDepthCm * 0.5f));
	};
	const auto ClearanceCenter = [&](const FResultBox& Box)
	{
		return Box.Center + FVector(0.0f, 0.0f, Tuning.OverlapDepthCm * 0.5f);
	};

	// Wall inside the horizontal clearance of the baseline candidate (outside the shape itself).
	UBoxComponent* SideWall = Fixture.AddBlocker(TEXT("SideWall"),
		Base.Center + Base.Rotation.RotateVector(FVector(0.0f, Base.HalfExtent.Y + Tuning.OverlapDepthCm * 0.5f + 1.0f, 0.0f)),
		FVector(1000.0f, 1.0f, 400.0f), false, Base.Rotation.Rotator());
	if (!SideWall)
	{
		return false;
	}
	TestFalse(TEXT("Side wall is outside the original shape"),
		FacilityPlacementCollision::HasBlockingOverlap(*Fixture.World, Base.Center, Base.Rotation, Shape, *Template, Params));
	TestTrue(TEXT("Side wall is inside the clearance shape"), FacilityPlacementCollision::HasBlockingOverlap(
		*Fixture.World, ClearanceCenter(Base), Base.Rotation, ClearanceShape(Base), *Template, Params));
	TArray<FTransform> Relocated;
	TestTrue(TEXT("Side wall layout relocates"), Fixture.Find(Request, Tuning, Seed, Relocated, Stage, Failure));
	TArray<FResultBox> RelocatedBoxes;
	if (Fixture.ResolveBoxes(Relocated, RelocatedBoxes))
	{
		TestFalse(TEXT("Relocated result clears the wall with the clearance"),
			FacilityPlacementCollision::HasBlockingOverlap(*Fixture.World, ClearanceCenter(RelocatedBoxes[0]),
				RelocatedBoxes[0].Rotation, ClearanceShape(RelocatedBoxes[0]), *Template, Params));
	}
	SideWall->GetOwner()->Destroy();

	// Ceiling inside the upward clearance of the baseline candidate.
	UBoxComponent* Ceiling = Fixture.AddBlocker(TEXT("Ceiling"),
		FVector(Base.Center.X, Base.Center.Y, Base.Center.Z + Base.HalfExtent.Z + Tuning.OverlapDepthCm * 0.5f + 1.0f),
		FVector(2000.0f, 2000.0f, 1.0f), false);
	if (!Ceiling)
	{
		return false;
	}
	TestTrue(TEXT("Ceiling is inside the upward clearance shape"), FacilityPlacementCollision::HasBlockingOverlap(
		*Fixture.World, ClearanceCenter(Base), Base.Rotation, ClearanceShape(Base), *Template, Params));
	Relocated.Reset();
	TestTrue(TEXT("Ceiling layout relocates"), Fixture.Find(Request, Tuning, Seed, Relocated, Stage, Failure));
	if (Fixture.ResolveBoxes(Relocated, RelocatedBoxes))
	{
		TestFalse(TEXT("Relocated result clears the ceiling with the clearance"),
			FacilityPlacementCollision::HasBlockingOverlap(*Fixture.World, ClearanceCenter(RelocatedBoxes[0]),
				RelocatedBoxes[0].Rotation, ClearanceShape(RelocatedBoxes[0]), *Template, Params));
	}
	return true;
}

// Value-source contract: nothing in the placement reads a value from code.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxViewFrontTuningSourceAutomationTest,
	"BathhouseSim.Shop.UnboxViewFront.TuningSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxViewFrontTuningSourceAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UShopSettings* Settings = GetMutableDefault<UShopSettings>();

	// Every property is read through FromSettings: set each to a distinct value and compare.
	{
		TGuardValue<float> ViewDistance(Settings->UnboxViewDistanceCm, 211.0f);
		TGuardValue<float> ViewMin(Settings->UnboxViewMinDistanceCm, 47.0f);
		TGuardValue<float> ViewStep(Settings->UnboxViewPullStepCm, 13.0f);
		TGuardValue<int32> ViewAttempts(Settings->UnboxViewLayoutAttempts, 7);
		TGuardValue<float> Forward(Settings->UnboxForwardDistanceCm, 133.0f);
		TGuardValue<float> ForwardMin(Settings->UnboxMinForwardDistanceCm, 9.0f);
		TGuardValue<float> ForwardStep(Settings->UnboxForwardPullStepCm, 17.0f);
		TGuardValue<int32> ForwardAttempts(Settings->UnboxForwardLayoutAttempts, 6);
		TGuardValue<float> FloorClearance(Settings->UnboxForwardFloorClearanceCm, 31.0f);
		TGuardValue<float> CameraClearance(Settings->UnboxCameraClearanceCm, 11.5f);
		TGuardValue<float> OverheadClearance(Settings->UnboxOverheadClearanceCm, 14.0f);
		TGuardValue<float> OverheadStep(Settings->UnboxOverheadStepCm, 33.0f);
		TGuardValue<int32> OverheadCount(Settings->UnboxOverheadStepCount, 5);
		TGuardValue<int32> OverheadAttempts(Settings->UnboxOverheadLayoutAttempts, 2);
		TGuardValue<int32> ClusterAttempts(Settings->UnboxClusterPlacementAttempts, 9);
		TGuardValue<float> MinElevation(Settings->UnboxClusterMinElevationDegrees, 12.0f);
		TGuardValue<float> MaxElevation(Settings->UnboxClusterMaxElevationDegrees, 52.0f);
		TGuardValue<float> Tolerance(Settings->UnboxClusterDepthToleranceCm, 0.7f);
		TGuardValue<float> Ratio(Settings->UnboxClusterPairDepthExtentRatio, 0.4f);
		const FShopUnboxingTuning Tuning = FShopUnboxingTuning::FromSettings(*Settings);
		TestEqual(TEXT("View distance"), Tuning.ViewDistanceCm, 211.0f);
		TestEqual(TEXT("View minimum"), Tuning.ViewMinDistanceCm, 47.0f);
		TestEqual(TEXT("View step"), Tuning.ViewPullStepCm, 13.0f);
		TestEqual(TEXT("View attempts"), Tuning.ViewLayoutAttempts, 7);
		TestEqual(TEXT("Forward distance"), Tuning.ForwardDistanceCm, 133.0f);
		TestEqual(TEXT("Forward minimum"), Tuning.MinForwardDistanceCm, 9.0f);
		TestEqual(TEXT("Forward step"), Tuning.ForwardPullStepCm, 17.0f);
		TestEqual(TEXT("Forward attempts"), Tuning.ForwardLayoutAttempts, 6);
		TestEqual(TEXT("Floor clearance"), Tuning.ForwardFloorClearanceCm, 31.0f);
		TestEqual(TEXT("Camera clearance"), Tuning.CameraClearanceCm, 11.5f);
		TestEqual(TEXT("Overhead clearance"), Tuning.OverheadClearanceCm, 14.0f);
		TestEqual(TEXT("Overhead step"), Tuning.OverheadStepCm, 33.0f);
		TestEqual(TEXT("Overhead count"), Tuning.OverheadStepCount, 5);
		TestEqual(TEXT("Overhead attempts"), Tuning.OverheadLayoutAttempts, 2);
		TestEqual(TEXT("Cluster attempts"), Tuning.Cluster.PlacementAttempts, 9);
		TestEqual(TEXT("Cluster min elevation"), Tuning.Cluster.MinElevationDegrees, 12.0f);
		TestEqual(TEXT("Cluster max elevation"), Tuning.Cluster.MaxElevationDegrees, 52.0f);
		TestEqual(TEXT("Cluster tolerance"), Tuning.Cluster.DepthToleranceCm, 0.7f);
		TestEqual(TEXT("Cluster ratio"), Tuning.Cluster.PairDepthExtentRatio, 0.4f);
	}

	// Defaults and fallbacks come from the header constants.
	{
		TGuardValue<float> NaNView(Settings->UnboxViewDistanceCm, std::numeric_limits<float>::quiet_NaN());
		TGuardValue<float> NaNOverhead(Settings->UnboxOverheadStepCm, std::numeric_limits<float>::quiet_NaN());
		TestEqual(TEXT("Non-finite view distance falls back to the header default"),
			Settings->GetUnboxViewDistanceCm(), FMath::Max(UShopSettings::DefaultUnboxViewDistanceCm, Settings->GetUnboxViewMinDistanceCm()));
		TestEqual(TEXT("Non-finite overhead step falls back to the header default"),
			Settings->GetUnboxOverheadStepCm(), UShopSettings::DefaultUnboxOverheadStepCm);
	}

	// Behavior follows the settings: view distance and floor height.
	FViewFrontFixture Fixture;
	if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontTuningWorld")) || !Fixture.Load(*this, { ShowerIndex }))
	{
		return false;
	}
	const FVector Camera = Fixture.GetCamera();
	{
		FShopUnboxingTuning Tuning = ShopUnboxTest::MakeTuning();
		Tuning.ViewDistanceCm += 40.0f;
		TArray<FTransform> Transforms;
		EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
		FText Failure;
		TestTrue(TEXT("Changed view distance places"),
			Fixture.Find(MakeRequestAt(Fixture, 0.0f), Tuning, 5, Transforms, Stage, Failure));
		TArray<FResultBox> Boxes;
		if (Fixture.ResolveBoxes(Transforms, Boxes))
		{
			TestTrue(TEXT("Nearest part follows the changed view distance"),
				Stage == EShopUnboxPlacementStage::ViewFront
				&& FMath::Abs(GetMinProjection(Boxes, Camera, FVector::ForwardVector) - Tuning.ViewDistanceCm) <= DistanceTolerance);
		}
	}
	{
		FShopUnboxingTuning Tuning = ShopUnboxTest::MakeTuning();
		Tuning.ForwardFloorClearanceCm += 13.0f;
		Tuning.ForwardDistanceCm += 25.0f;
		Fixture.AddViewStageBlocker(Tuning);
		TArray<FTransform> Transforms;
		EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
		FText Failure;
		TestTrue(TEXT("Changed floor values place"),
			Fixture.Find(MakeRequestAt(Fixture, 0.0f), Tuning, 5, Transforms, Stage, Failure));
		TArray<FResultBox> Boxes;
		if (Fixture.ResolveBoxes(Transforms, Boxes))
		{
			TestTrue(TEXT("Floor-front follows the changed distance and height"),
				Stage == EShopUnboxPlacementStage::FloorFront
				&& FMath::Abs(GetMinZ(Boxes) - (Fixture.GetFoot().Z + Tuning.ForwardFloorClearanceCm)) <= DistanceTolerance
				&& FVector::Dist2D(Boxes[0].Center, Fixture.GetFoot() + FVector::ForwardVector * Tuning.ForwardDistanceCm) <= DistanceTolerance);
		}
	}
	return true;
}

// USV-021: items placed from above stay inside a closed physical room.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxViewFrontRoomPhysicsAutomationTest,
	"BathhouseSim.Shop.UnboxViewFront.RoomPhysics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxViewFrontRoomPhysicsAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	// The room is sized from the view distance setting so the cluster lands near walls and ceiling.
	const FShopUnboxingTuning Tuning = ShopUnboxTest::MakeTuning();
	const float InnerHalfWidth = Tuning.ViewDistanceCm * 3.0f;
	const float InnerCeiling = Tuning.ViewDistanceCm * 4.0f;
	FViewFrontFixture Fixture;
	if (!Fixture.Initialize(*this, TEXT("UnboxViewFrontRoomWorld"), true, true, FVector(0.0f, 0.0f, 120.0f)))
	{
		return false;
	}
	const FVector Foot = Fixture.GetFoot();
	Fixture.AddBlocker(TEXT("RoomCeiling"), FVector(0.0f, 0.0f, Foot.Z + InnerCeiling + 50.0f), FVector(InnerHalfWidth, InnerHalfWidth, 50.0f));
	Fixture.AddBlocker(TEXT("RoomNorth"), FVector(InnerHalfWidth + 50.0f, 0.0f, Foot.Z + InnerCeiling * 0.5f), FVector(50.0f, InnerHalfWidth, InnerCeiling * 0.5f));
	Fixture.AddBlocker(TEXT("RoomSouth"), FVector(-InnerHalfWidth - 50.0f, 0.0f, Foot.Z + InnerCeiling * 0.5f), FVector(50.0f, InnerHalfWidth, InnerCeiling * 0.5f));
	Fixture.AddBlocker(TEXT("RoomEast"), FVector(0.0f, InnerHalfWidth + 50.0f, Foot.Z + InnerCeiling * 0.5f), FVector(InnerHalfWidth, 50.0f, InnerCeiling * 0.5f));
	Fixture.AddBlocker(TEXT("RoomWest"), FVector(0.0f, -InnerHalfWidth - 50.0f, Foot.Z + InnerCeiling * 0.5f), FVector(InnerHalfWidth, 50.0f, InnerCeiling * 0.5f));
	const int32 MaxQuantity = GetDefault<UShopSettings>()->GetCartTotalQuantityLimit();
	if (!Fixture.Load(*this, RepeatIndices(MaxQuantity)))
	{
		return false;
	}
	const FShopUnboxingPlacementRequest Request = MakeRequestAt(Fixture, 60.0f);
	TArray<FTransform> Transforms;
	EShopUnboxPlacementStage Stage = EShopUnboxPlacementStage::None;
	FText Failure;
	TestTrue(TEXT("Room placement succeeds"), Fixture.Find(Request, Tuning, 20260928, Transforms, Stage, Failure));
	TestEqual(TEXT("One transform per item"), Transforms.Num(), MaxQuantity);
	AddInfo(FString::Printf(TEXT("UnboxViewFront RoomPhysics adopted stage %s"), StageName(Stage)));
	TArray<APlaceableFacilityItemActor*> Items;
	for (int32 Index = 0; Index < FMath::Min(Transforms.Num(), Fixture.Definitions.Num()); ++Index)
	{
		APlaceableFacilityItemActor* Item = APlaceableFacilityItemActor::SpawnFreshItem(
			*Fixture.World, *Fixture.Definitions[Index], Transforms[Index], Failure);
		if (Item && Item->ActivateFreeWorld(Transforms[Index], Failure))
		{
			Items.Add(Item);
		}
		else
		{
			AddError(FString::Printf(TEXT("Item %d could not be activated: %s"), Index, *Failure.ToString()));
		}
	}
	for (int32 Step = 0; Step < 180; ++Step)
	{
		++GFrameCounter;
		Fixture.World->Tick(LEVELTICK_All, 1.0f / 60.0f);
	}
	float LowestBottom = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		FVector Center;
		FQuat Rotation;
		FCollisionShape Shape;
		const UPrimitiveComponent* Template = nullptr;
		if (!Fixture.Shapes[Index].BuildCollisionQuery(
			Items[Index]->GetActorTransform(), Center, Rotation, Shape, Template, Failure))
		{
			AddError(TEXT("Settled item collision query failed."));
			continue;
		}
		const FVector Extent = Shape.GetExtent().GetAbs();
		TestTrue(TEXT("Item stays inside the room walls"),
			FMath::Abs(Center.X) + Extent.X < InnerHalfWidth + 1.0f && FMath::Abs(Center.Y) + Extent.Y < InnerHalfWidth + 1.0f);
		TestTrue(TEXT("Item rests above the floor"), Center.Z - Extent.Z > Foot.Z - 1.0f);
		TestTrue(TEXT("Item stays below the ceiling"), Center.Z + Extent.Z < Foot.Z + InnerCeiling + 1.0f);
		LowestBottom = FMath::Min(LowestBottom, Center.Z - Extent.Z);
	}
	TestTrue(TEXT("The lowest item bottom has settled on the floor"), FMath::Abs(LowestBottom - Foot.Z) <= 2.0f);
	return true;
}

#endif
