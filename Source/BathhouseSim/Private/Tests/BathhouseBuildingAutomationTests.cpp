#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Building/BathhouseBuildingSettings.h"
#include "Building/BathhouseSpaceActor.h"
#include "Building/BathhouseSpaceEditorSync.h"
#include "Building/BathhouseSpaceLayout.h"
#include "Building/BathhouseSpaceShellComponent.h"
#include "Building/BathhouseSpaceValidation.h"
#include "Cleaning/LitterSpawnZoneActor.h"
#include "Cleaning/StainSpawnZoneActor.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "Camera/CameraComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "Components/StaticMeshComponent.h"
#include "Placement/FacilityActorConversionTransaction.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementSettings.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Placement/PlaceableFacility.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Placement/PlayerFacilityPlacementComponent.h"
#include "Tests/FacilityPlacementAutomationTestProbe.h"

/** 테스트가 공간 Actor의 authored property를 직접 설정하게 하는 접근 통로. */
class FBathhouseBuildingAutomationAccess
{
public:
	static ABathhouseSpaceActor* SpawnConfigured(UWorld& World, const FBathhouseSpaceSnapshot& Snapshot, FName Name)
	{
		FActorSpawnParameters Params;
		Params.Name = Name;
		const FTransform Transform(FVector(Snapshot.ActorXY.X, Snapshot.ActorXY.Y, Snapshot.FloorZ));
		ABathhouseSpaceActor* Actor = World.SpawnActorDeferred<ABathhouseSpaceActor>(
			ABathhouseSpaceActor::StaticClass(), Transform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Actor)
		{
			return nullptr;
		}
		Actor->SpaceKind = Snapshot.Kind;
		Actor->FloorSizeCm = FVector2D(
			Snapshot.Interior.Max.X - Snapshot.Interior.Min.X, Snapshot.Interior.Max.Y - Snapshot.Interior.Min.Y);
		Actor->CeilingHeightCm = Snapshot.CeilingHeightCm;
		Actor->CleaningChunkKind = Snapshot.ChunkKind;
		Actor->Lighting.SpacingCm = Snapshot.LightSpacingCm;
		Actor->Lighting.CeilingOffsetCm = Snapshot.LightCeilingOffsetCm;
		Actor->Lighting.IntensityCandela = Snapshot.LightSpacingCm;
		Actor->Lighting.AttenuationRadiusCm = Snapshot.LightSpacingCm;
		return Actor;
	}

	static void Link(const TArray<ABathhouseSpaceActor*>& Actors, const FBathhouseSpaceSnapshot& Snapshot, ABathhouseSpaceActor& Target)
	{
		for (const FBathhouseOpeningSnapshot& Opening : Snapshot.Openings)
		{
			FBathhouseSpaceOpening& Item = Target.Openings.AddDefaulted_GetRef();
			Item.Side = Opening.Side;
			Item.CenterOffsetCm = static_cast<float>(Opening.CenterOffsetCm);
			Item.WidthCm = static_cast<float>(Opening.WidthCm);
			Item.HeightCm = static_cast<float>(Opening.HeightCm);
			Item.ConnectedSpace = Actors.IsValidIndex(Opening.ConnectedIndex) ? Actors[Opening.ConnectedIndex] : nullptr;
		}
		for (const FBathhouseStairSnapshot& Stair : Snapshot.Stairs)
		{
			FBathhouseStairSpec& Item = Target.Stairs.AddDefaulted_GetRef();
			Item.LowerSpace = Actors.IsValidIndex(Stair.LowerIndex) ? Actors[Stair.LowerIndex] : nullptr;
			Item.TopEdgeCenterOffsetCm = Stair.TopEdgeOffsetCm;
			Item.DownSide = Stair.DownSide;
			Item.WidthCm = static_cast<float>(Stair.WidthCm);
			Item.RunCm = static_cast<float>(Stair.RunCm);
			Item.StepCount = Stair.StepCount;
			Item.GuardHeightCm = static_cast<float>(Stair.GuardHeightCm);
		}
	}

	static void AddAllowedTag(ABathhouseSpaceActor& Actor, const FGameplayTag& Tag) { Actor.AllowedFacilityTags.AddTag(Tag); }
	static void SetOpeningWidth(ABathhouseSpaceActor& Actor, int32 Index, float Width) { Actor.Openings[Index].WidthCm = Width; }
	static float GetOpeningWidth(const ABathhouseSpaceActor& Actor, int32 Index) { return Actor.Openings[Index].WidthCm; }
};

namespace
{
	using FSnapshots = TArray<FBathhouseSpaceSnapshot>;
	using FProblems = TArray<FBathhouseLayoutProblem>;
	constexpr double TestTolerance = 0.01;

	// 아래 크기·두께는 테스트 fixture 입력이다. 기대값은 항상 같은 입력에서 계산한다.
	FBathhouseLayoutValues FixtureValues()
	{
		FBathhouseLayoutValues Values;
		Values.WallThicknessCm = 20.0;
		Values.SlabThicknessCm = 20.0;
		Values.ChunkMaxSizeCm = FVector2D(400.0, 400.0);
		return Values;
	}

	FBathhouseSpaceSnapshot MakeSpace(
		const EBathhouseSpaceKind Kind, const TCHAR* Name, const FVector2D& Center, const FVector2D& Size,
		const double FloorZ, const double Ceiling, const EBathhouseCleaningChunkKind Chunk)
	{
		FBathhouseSpaceSnapshot Space;
		Space.Kind = Kind;
		Space.DisplayName = Name;
		Space.ActorXY = Center;
		Space.FloorZ = FloorZ;
		Space.Interior = FBox2D(Center - Size * 0.5, Center + Size * 0.5);
		Space.CeilingHeightCm = Ceiling;
		Space.LightSpacingCm = 600.0;
		Space.LightCeilingOffsetCm = 20.0;
		Space.ChunkKind = Chunk;
		return Space;
	}

	/** 홀(0), 목욕공간(1, 홀 동쪽), 작업공간(2, 홀 아래). 통로는 홀 목록, 계단도 홀 목록. */
	FSnapshots MakeValidSnapshots(const FBathhouseLayoutValues& Values)
	{
		FSnapshots Snapshots;
		FBathhouseSpaceSnapshot Hall = MakeSpace(EBathhouseSpaceKind::Hall, TEXT("Hall"), FVector2D(0, 0),
			FVector2D(2000, 1400), 50.0, 350.0, EBathhouseCleaningChunkKind::Litter);
		const double BathWidth = 1000.0;
		const double BathCenterX = Hall.Interior.Max.X + 2.0 * Values.WallThicknessCm + BathWidth * 0.5;
		FBathhouseSpaceSnapshot Bath = MakeSpace(EBathhouseSpaceKind::Bath, TEXT("Bath"), FVector2D(BathCenterX, 0),
			FVector2D(BathWidth, 1300), 50.0, 350.0, EBathhouseCleaningChunkKind::Stain);
		FBathhouseSpaceSnapshot Work = MakeSpace(EBathhouseSpaceKind::Work, TEXT("Work"), FVector2D(-200, 0),
			FVector2D(1400, 900), 50.0 - 400.0, 300.0, EBathhouseCleaningChunkKind::None);
		FBathhouseOpeningSnapshot Entrance;
		Entrance.Side = EBathhouseSpaceSide::West;
		Entrance.CenterOffsetCm = 0.0;
		Entrance.WidthCm = 200.0;
		Entrance.HeightCm = 260.0;
		Hall.Openings.Add(Entrance);
		FBathhouseOpeningSnapshot Passage = Entrance;
		Passage.Side = EBathhouseSpaceSide::East;
		Passage.ConnectedIndex = 1;
		Hall.Openings.Add(Passage);
		FBathhouseStairSnapshot Stair;
		Stair.LowerIndex = 2;
		Stair.TopEdgeOffsetCm = FVector2D(-300, 0);
		Stair.DownSide = EBathhouseSpaceSide::East;
		Stair.WidthCm = 120.0;
		Stair.RunCm = 600.0;
		Stair.StepCount = 10;
		Stair.GuardHeightCm = 100.0;
		Stair.bHasStepMaterial = true;
		Stair.bHasStairWallMaterial = true;
		Hall.Stairs.Add(Stair);
		Snapshots.Add(Hall);
		Snapshots.Add(Bath);
		Snapshots.Add(Work);
		return Snapshots;
	}

	FBathhouseValidationInputs MakeInputs(const FBathhouseLayoutValues& Values)
	{
		FBathhouseValidationInputs Inputs;
		Inputs.Layout = Values;
		Inputs.bBoxMeshValid = true;
		Inputs.bLitterClassSet = true;
		Inputs.bStainClassSet = true;
		Inputs.WalkableFloorAngleDegrees = GetDefault<UCharacterMovementComponent>()->GetWalkableFloorAngle();
		return Inputs;
	}

	bool HasCode(const FProblems& Problems, const EBathhouseProblemCode Code)
	{
		return Problems.ContainsByPredicate([Code](const FBathhouseLayoutProblem& P) { return P.Code == Code; });
	}

	int32 CountSeverity(const FProblems& Problems, const EBathhouseProblemSeverity Severity)
	{
		int32 Count = 0;
		for (const FBathhouseLayoutProblem& Problem : Problems)
		{
			Count += Problem.Severity == Severity ? 1 : 0;
		}
		return Count;
	}

	double RectArea(const FBox2D& Rect) { return (Rect.Max.X - Rect.Min.X) * (Rect.Max.Y - Rect.Min.Y); }

	FBox2D BoxToRect(const FBathhouseBoxPart& Box)
	{
		return FBox2D(
			FVector2D(Box.Center.X - Box.HalfExtent.X, Box.Center.Y - Box.HalfExtent.Y),
			FVector2D(Box.Center.X + Box.HalfExtent.X, Box.Center.Y + Box.HalfExtent.Y));
	}

	double IntersectionArea(const FBox2D& A, const FBox2D& B)
	{
		const double X = FMath::Min(A.Max.X, B.Max.X) - FMath::Max(A.Min.X, B.Min.X);
		const double Y = FMath::Min(A.Max.Y, B.Max.Y) - FMath::Max(A.Min.Y, B.Min.Y);
		return X > 0.0 && Y > 0.0 ? X * Y : 0.0;
	}

	void ExpectNear(FAutomationTestBase& Test, const FString& What, const double Actual, const double Expected, const double Tolerance)
	{
		Test.TestTrue(FString::Printf(TEXT("%s (actual %f, expected %f)"), *What, Actual, Expected),
			FMath::Abs(Actual - Expected) <= Tolerance);
	}

	bool BoxContains(const FBathhouseBoxPart& Box, const FVector& Point)
	{
		return FMath::Abs(Point.X - Box.Center.X) < Box.HalfExtent.X - TestTolerance
			&& FMath::Abs(Point.Y - Box.Center.Y) < Box.HalfExtent.Y - TestTolerance
			&& FMath::Abs(Point.Z - Box.Center.Z) < Box.HalfExtent.Z - TestTolerance;
	}

	bool PlanContains(const FBathhouseSpacePlan& Plan, const EBathhouseShellPart Part, const FVector& Point)
	{
		for (const FBathhouseBoxPart& Box : Plan.Get(Part))
		{
			if (BoxContains(Box, Point))
			{
				return true;
			}
		}
		return false;
	}

	FBathhouseBoxPart MakeBoxPart(const FBox2D& Rect, const double Z0, const double Z1)
	{
		FBathhouseBoxPart Part;
		Part.Center = FVector((Rect.Min.X + Rect.Max.X) * 0.5, (Rect.Min.Y + Rect.Max.Y) * 0.5, (Z0 + Z1) * 0.5);
		Part.HalfExtent = FVector((Rect.Max.X - Rect.Min.X) * 0.5, (Rect.Max.Y - Rect.Min.Y) * 0.5, (Z1 - Z0) * 0.5);
		return Part;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseBuildingLayoutSingleTest,
	"BathhouseSim.Building.Layout.SingleSpace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseBuildingLayoutSingleTest::RunTest(const FString& Parameters)
{
	const FBathhouseLayoutValues Values = FixtureValues();
	FSnapshots Snapshots;
	Snapshots.Add(MakeSpace(EBathhouseSpaceKind::Hall, TEXT("Solo"), FVector2D(300, -200), FVector2D(2000, 1400),
		50.0, 350.0, EBathhouseCleaningChunkKind::None));
	const FBathhouseSpaceSnapshot& Space = Snapshots[0];
	const FBathhouseSpacePlan Plan = FBathhouseSpaceLayout::BuildPlan(Snapshots, 0, Values);
	const FBox2D Outer = FBathhouseSpaceLayout::OuterRect(Space, Values.WallThicknessCm);
	const double Zf = Space.FloorZ;
	const double Zc = FBathhouseSpaceLayout::CeilingZ(Space);

	// EXP-001: 바닥·천장 Z 범위와 바깥 직사각형.
	TestEqual(TEXT("EXP-001 one floor box without holes"), Plan.Get(EBathhouseShellPart::Floor).Num(), 1);
	TestEqual(TEXT("EXP-001 one ceiling box without holes"), Plan.Get(EBathhouseShellPart::Ceiling).Num(), 1);
	if (Plan.Get(EBathhouseShellPart::Floor).Num() == 1 && Plan.Get(EBathhouseShellPart::Ceiling).Num() == 1)
	{
		const FBathhouseBoxPart& Floor = Plan.Get(EBathhouseShellPart::Floor)[0];
		const FBathhouseBoxPart& Ceiling = Plan.Get(EBathhouseShellPart::Ceiling)[0];
		ExpectNear(*this, TEXT("EXP-001 floor bottom = floor top - slab"), Floor.Center.Z - Floor.HalfExtent.Z, Zf - Values.SlabThicknessCm, TestTolerance);
		ExpectNear(*this, TEXT("EXP-001 floor top = floor Z"), Floor.Center.Z + Floor.HalfExtent.Z, Zf, TestTolerance);
		ExpectNear(*this, TEXT("EXP-001 ceiling bottom = ceiling Z"), Ceiling.Center.Z - Ceiling.HalfExtent.Z, Zc, TestTolerance);
		ExpectNear(*this, TEXT("EXP-001 ceiling top = ceiling + slab"), Ceiling.Center.Z + Ceiling.HalfExtent.Z, Zc + Values.SlabThicknessCm, TestTolerance);
		ExpectNear(*this, TEXT("EXP-001 floor covers the outer rectangle"), RectArea(BoxToRect(Floor)), RectArea(Outer), TestTolerance);
		TestTrue(TEXT("EXP-001 floor XY equals the outer rectangle"), BoxToRect(Floor).Min.Equals(Outer.Min, TestTolerance)
			&& BoxToRect(Floor).Max.Equals(Outer.Max, TestTolerance));
	}

	// 벽 4개: 모서리 비겹침, 합집합이 바깥 띠를 덮음, Z 범위.
	const TArray<FBathhouseBoxPart>& Walls = Plan.Get(EBathhouseShellPart::Wall);
	TestEqual(TEXT("EXP-001 four walls without openings"), Walls.Num(), 4);
	double WallArea = 0.0;
	for (int32 A = 0; A < Walls.Num(); ++A)
	{
		WallArea += RectArea(BoxToRect(Walls[A]));
		ExpectNear(*this, TEXT("EXP-001 wall bottom = floor Z"), Walls[A].Center.Z - Walls[A].HalfExtent.Z, Zf, TestTolerance);
		ExpectNear(*this, TEXT("EXP-001 wall top = ceiling Z"), Walls[A].Center.Z + Walls[A].HalfExtent.Z, Zc, TestTolerance);
		for (int32 B = A + 1; B < Walls.Num(); ++B)
		{
			ExpectNear(*this, TEXT("EXP-001 wall corners do not overlap"),
				IntersectionArea(BoxToRect(Walls[A]), BoxToRect(Walls[B])), 0.0, TestTolerance);
		}
	}
	ExpectNear(*this, TEXT("EXP-001 walls cover exactly the outer band"),
		WallArea, RectArea(Outer) - RectArea(Space.Interior), TestTolerance);

	// 조명: 축마다 간격 이하의 같은 칸, 최소 1개.
	const int32 ExpectedLightsX = FMath::Max(1, FMath::CeilToInt((Space.Interior.Max.X - Space.Interior.Min.X) / Space.LightSpacingCm));
	const int32 ExpectedLightsY = FMath::Max(1, FMath::CeilToInt((Space.Interior.Max.Y - Space.Interior.Min.Y) / Space.LightSpacingCm));
	TestEqual(TEXT("EXP-001 light count per axis"), Plan.LightLocations.Num(), ExpectedLightsX * ExpectedLightsY);
	for (const FVector& Light : Plan.LightLocations)
	{
		ExpectNear(*this, TEXT("EXP-001 light Z = ceiling - offset"), Light.Z, Zc - Space.LightCeilingOffsetCm, TestTolerance);
		TestTrue(TEXT("EXP-001 light inside the interior"), Space.Interior.IsInside(FVector2D(Light.X, Light.Y)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseBuildingLayoutOpeningsTest,
	"BathhouseSim.Building.Layout.AdjacentOpenings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseBuildingLayoutOpeningsTest::RunTest(const FString& Parameters)
{
	const FBathhouseLayoutValues Values = FixtureValues();
	const FSnapshots Snapshots = MakeValidSnapshots(Values);
	TArray<FBathhouseSpacePlan> Plans;
	FBathhouseSpaceLayout::Build(Snapshots, Values, Plans);
	const FBathhouseSpaceSnapshot& Hall = Snapshots[0];
	const FBathhouseSpaceSnapshot& Bath = Snapshots[1];
	const FBox2D HallOuter = FBathhouseSpaceLayout::OuterRect(Hall, Values.WallThicknessCm);
	const FBox2D BathOuter = FBathhouseSpaceLayout::OuterRect(Bath, Values.WallThicknessCm);

	// EXP-001: 맞닿은 두 공간의 바닥·천장은 변만 닿고 겹치지 않는다.
	ExpectNear(*this, TEXT("EXP-001 outer edges touch exactly"), HallOuter.Max.X, BathOuter.Min.X, TestTolerance);
	for (const EBathhouseShellPart Part : { EBathhouseShellPart::Floor, EBathhouseShellPart::Ceiling })
	{
		for (const FBathhouseBoxPart& HallBox : Plans[0].Get(Part))
		{
			for (const FBathhouseBoxPart& BathBox : Plans[1].Get(Part))
			{
				ExpectNear(*this, TEXT("EXP-001 adjacent floors and ceilings do not overlap"),
					IntersectionArea(BoxToRect(HallBox), BoxToRect(BathBox)), 0.0, TestTolerance);
			}
		}
	}

	// EXP-002: 바깥 출입구(홀 서쪽). 구간 안은 비고 그 옆은 벽이며 위는 인방.
	const FBathhouseOpeningSnapshot& Entrance = Hall.Openings[0];
	const FVector2D EntranceInterval = FBathhouseSpaceLayout::OpeningWorldInterval(Hall, Entrance);
	const double WestWallX = Hall.Interior.Min.X - Values.WallThicknessCm * 0.5;
	const double Zf = Hall.FloorZ;
	const FBathhouseSpacePlan& HallPlan = Plans[0];
	TestFalse(TEXT("EXP-002 entrance is open at mid height"),
		PlanContains(HallPlan, EBathhouseShellPart::Wall, FVector(WestWallX, (EntranceInterval.X + EntranceInterval.Y) * 0.5, Zf + Entrance.HeightCm * 0.5)));
	TestTrue(TEXT("EXP-002 lintel closes the wall above the entrance"),
		PlanContains(HallPlan, EBathhouseShellPart::Wall, FVector(WestWallX, (EntranceInterval.X + EntranceInterval.Y) * 0.5, Zf + Entrance.HeightCm + 5.0)));
	TestTrue(TEXT("EXP-002 wall remains beside the entrance (low)"),
		PlanContains(HallPlan, EBathhouseShellPart::Wall, FVector(WestWallX, EntranceInterval.Y + 5.0, Zf + 5.0)));
	TestTrue(TEXT("EXP-002 wall remains beside the entrance (other side)"),
		PlanContains(HallPlan, EBathhouseShellPart::Wall, FVector(WestWallX, EntranceInterval.X - 5.0, Zf + 5.0)));
	TestEqual(TEXT("EXP-002 west wall = two side pieces + one lintel"), [&]()
	{
		int32 Count = 0;
		for (const FBathhouseBoxPart& Box : HallPlan.Get(EBathhouseShellPart::Wall))
		{
			Count += Box.Center.X < Hall.Interior.Min.X ? 1 : 0;
		}
		return Count;
	}(), 3);

	// EXP-003: 통로는 두 공간의 벽에서 같은 world 구간으로 뚫린다.
	const FBathhouseOpeningSnapshot& Passage = Hall.Openings[1];
	const FVector2D PassageInterval = FBathhouseSpaceLayout::OpeningWorldInterval(Hall, Passage);
	const double PassageMidY = (PassageInterval.X + PassageInterval.Y) * 0.5;
	const double HallEastWallX = Hall.Interior.Max.X + Values.WallThicknessCm * 0.5;
	const double BathWestWallX = Bath.Interior.Min.X - Values.WallThicknessCm * 0.5;
	TestFalse(TEXT("EXP-003 hall east wall is open at the passage"),
		PlanContains(Plans[0], EBathhouseShellPart::Wall, FVector(HallEastWallX, PassageMidY, Zf + Passage.HeightCm * 0.5)));
	TestFalse(TEXT("EXP-003 bath west wall is open at the same passage"),
		PlanContains(Plans[1], EBathhouseShellPart::Wall, FVector(BathWestWallX, PassageMidY, Zf + Passage.HeightCm * 0.5)));
	for (const double Y : { PassageInterval.X - 5.0, PassageInterval.Y + 5.0 })
	{
		TestTrue(TEXT("EXP-003 hall east wall stays beside the passage"),
			PlanContains(Plans[0], EBathhouseShellPart::Wall, FVector(HallEastWallX, Y, Zf + 5.0)));
		TestTrue(TEXT("EXP-003 bath west wall stays beside the passage"),
			PlanContains(Plans[1], EBathhouseShellPart::Wall, FVector(BathWestWallX, Y, Zf + 5.0)));
	}
	TestTrue(TEXT("EXP-003 bath wall lintel above the passage"),
		PlanContains(Plans[1], EBathhouseShellPart::Wall, FVector(BathWestWallX, PassageMidY, Zf + Passage.HeightCm + 5.0)));

	// 직사각형 빼기: 겹치지 않고 면적 보존.
	const FBox2D Base(FVector2D(0, 0), FVector2D(1000, 600));
	const TArray<FBox2D> Holes = { FBox2D(FVector2D(100, 100), FVector2D(300, 400)), FBox2D(FVector2D(250, 300), FVector2D(700, 700)), FBox2D(FVector2D(-50, 500), FVector2D(40, 650)) };
	TArray<FBox2D> Pieces;
	FBathhouseSpaceLayout::SubtractRects(Base, Holes, Pieces);
	double PieceArea = 0.0;
	for (int32 A = 0; A < Pieces.Num(); ++A)
	{
		PieceArea += RectArea(Pieces[A]);
		for (int32 B = A + 1; B < Pieces.Num(); ++B)
		{
			ExpectNear(*this, TEXT("SubtractRects pieces do not overlap"), IntersectionArea(Pieces[A], Pieces[B]), 0.0, TestTolerance);
		}
		for (const FBox2D& Hole : Holes)
		{
			ExpectNear(*this, TEXT("SubtractRects pieces avoid holes"), IntersectionArea(Pieces[A], Hole), 0.0, TestTolerance);
		}
	}
	double HoleUnionArea = 0.0;
	{
		// 구멍 합집합 면적 = 구멍 면적 합 - 구멍끼리 겹침(이 fixture는 둘만 겹침) - Base 밖.
		HoleUnionArea = IntersectionArea(Base, Holes[0]) + IntersectionArea(Base, Holes[1]) + IntersectionArea(Base, Holes[2])
			- IntersectionArea(Holes[0], Holes[1]);
	}
	ExpectNear(*this, TEXT("SubtractRects keeps the remaining area"), PieceArea, RectArea(Base) - HoleUnionArea, TestTolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseBuildingLayoutStairsTest,
	"BathhouseSim.Building.Layout.Stairs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseBuildingLayoutStairsTest::RunTest(const FString& Parameters)
{
	const FBathhouseLayoutValues Values = FixtureValues();
	const FSnapshots Snapshots = MakeValidSnapshots(Values);
	TArray<FBathhouseSpacePlan> Plans;
	FBathhouseSpaceLayout::Build(Snapshots, Values, Plans);
	const FBathhouseSpaceSnapshot& Hall = Snapshots[0];
	const FBathhouseSpaceSnapshot& Work = Snapshots[2];
	const FBathhouseStairSnapshot& Stair = Hall.Stairs[0];
	const FBox2D Hole = FBathhouseSpaceLayout::StairHole(Hall, Stair);
	const FBathhouseStairFrame Frame = FBathhouseSpaceLayout::MakeStairFrame(Hall, Stair);
	const double ZUpper = Hall.FloorZ;
	const double ZLower = Work.FloorZ;
	const double T = Values.WallThicknessCm;
	const double S = Values.SlabThicknessCm;

	// EXP-004: 위층 바닥과 아래층 천장의 구멍이 같은 R이다.
	const FBox2D SlabHole = FBathhouseSpaceLayout::StairSlabHole(Hall, Stair, T);
	const FVector2D HoleCenter = Hole.GetCenter();
	TestFalse(TEXT("EXP-004 upper floor has the hole"),
		PlanContains(Plans[0], EBathhouseShellPart::Floor, FVector(HoleCenter.X, HoleCenter.Y, ZUpper - S * 0.5)));
	TestFalse(TEXT("EXP-004 lower ceiling has the same hole"),
		PlanContains(Plans[2], EBathhouseShellPart::Ceiling, FVector(HoleCenter.X, HoleCenter.Y, FBathhouseSpaceLayout::CeilingZ(Work) + S * 0.5)));
	for (const FVector2D& Outside : { FVector2D(SlabHole.Min.X - 5.0, HoleCenter.Y), FVector2D(SlabHole.Max.X + 5.0, HoleCenter.Y),
		FVector2D(HoleCenter.X, SlabHole.Min.Y - 5.0), FVector2D(HoleCenter.X, SlabHole.Max.Y + 5.0) })
	{
		TestTrue(TEXT("EXP-004 upper floor remains around the hole"),
			PlanContains(Plans[0], EBathhouseShellPart::Floor, FVector(Outside.X, Outside.Y, ZUpper - S * 0.5)));
		TestTrue(TEXT("EXP-004 lower ceiling remains around the hole"),
			PlanContains(Plans[2], EBathhouseShellPart::Ceiling, FVector(Outside.X, Outside.Y, FBathhouseSpaceLayout::CeilingZ(Work) + S * 0.5)));
	}
	ExpectNear(*this, TEXT("EXP-004 hole length = run"), Hole.Max.X - Hole.Min.X, Stair.RunCm, TestTolerance);
	ExpectNear(*this, TEXT("EXP-004 hole width = stair width"), Hole.Max.Y - Hole.Min.Y, Stair.WidthCm, TestTolerance);
	// 복귀 A2: 판 구멍은 옆 벽 발자국까지 넓다.
	ExpectNear(*this, TEXT("A2 slab hole length = run"), SlabHole.Max.X - SlabHole.Min.X, Stair.RunCm, TestTolerance);
	ExpectNear(*this, TEXT("A2 slab hole width = stair width + 2 wall"), SlabHole.Max.Y - SlabHole.Min.Y, Stair.WidthCm + 2.0 * T, TestTolerance);

	// 경사로 윗면 양 끝점.
	const TArray<FBathhouseBoxPart>& Ramps = Plans[0].Get(EBathhouseShellPart::StairRamp);
	TestEqual(TEXT("EXP-004 one ramp"), Ramps.Num(), 1);
	if (Ramps.Num() == 1)
	{
		const FBathhouseBoxPart& Ramp = Ramps[0];
		const FTransform RampTransform(Ramp.Rotation, Ramp.Center);
		const FVector TopFront = RampTransform.TransformPosition(FVector(-Ramp.HalfExtent.X, 0.0, Ramp.HalfExtent.Z));
		const FVector TopBack = RampTransform.TransformPosition(FVector(Ramp.HalfExtent.X, 0.0, Ramp.HalfExtent.Z));
		const FVector2D Start = Frame.ToWorld(0.0, 0.0);
		const FVector2D End = Frame.ToWorld(Stair.RunCm, 0.0);
		TestTrue(TEXT("EXP-004 ramp top starts at the upper floor edge"), TopFront.Equals(FVector(Start.X, Start.Y, ZUpper), TestTolerance));
		TestTrue(TEXT("EXP-004 ramp top ends at the lower floor"), TopBack.Equals(FVector(End.X, End.Y, ZLower), TestTolerance));
		ExpectNear(*this, TEXT("EXP-004 ramp thickness = slab"), Ramp.HalfExtent.Z * 2.0, S, TestTolerance);
	}
	TestEqual(TEXT("EXP-004 step count"), Plans[0].Get(EBathhouseShellPart::StairStep).Num(), Stair.StepCount);

	// 계단 벽: 위 입구는 바닥 위로 열리고 아래 출구는 아래층 천장 아래로 열린다.
	const FVector2D UpperEnd = Frame.ToWorld(-T * 0.5, 0.0);
	const FVector2D LowerEnd = Frame.ToWorld(Stair.RunCm + T * 0.5, 0.0);
	const double ZLowerCeiling = FBathhouseSpaceLayout::CeilingZ(Work);
	TestFalse(TEXT("EXP-004 upper entrance is open above the floor"),
		PlanContains(Plans[0], EBathhouseShellPart::StairWall, FVector(UpperEnd.X, UpperEnd.Y, ZUpper + 20.0)));
	TestTrue(TEXT("EXP-004 upper end is closed below the floor"),
		PlanContains(Plans[0], EBathhouseShellPart::StairWall, FVector(UpperEnd.X, UpperEnd.Y, ZUpper - S - 20.0)));
	TestFalse(TEXT("EXP-004 lower exit is open below the lower ceiling"),
		PlanContains(Plans[0], EBathhouseShellPart::StairWall, FVector(LowerEnd.X, LowerEnd.Y, ZLowerCeiling - 20.0)));
	TestTrue(TEXT("EXP-004 lower end is closed above the lower ceiling"),
		PlanContains(Plans[0], EBathhouseShellPart::StairWall, FVector(LowerEnd.X, LowerEnd.Y, ZLowerCeiling + S + 20.0)));
	for (const double Side : { -1.0, 1.0 })
	{
		const FVector2D SideWall = Frame.ToWorld(Stair.RunCm * 0.5, Side * (Stair.WidthCm * 0.5 + T * 0.5));
		TestTrue(TEXT("EXP-004 side wall reaches the guard height"),
			PlanContains(Plans[0], EBathhouseShellPart::StairWall, FVector(SideWall.X, SideWall.Y, ZUpper + Stair.GuardHeightCm - 5.0)));
		TestFalse(TEXT("EXP-004 side wall stops at the guard height"),
			PlanContains(Plans[0], EBathhouseShellPart::StairWall, FVector(SideWall.X, SideWall.Y, ZUpper + Stair.GuardHeightCm + 5.0)));
	}

	// 복귀 A2: 보이는 part 상자끼리 양의 부피로 겹치지 않는다(같은 방향 동일 평면 면 겹침 없음).
	{
		TArray<FBathhouseBoxPart> Visible;
		for (const int32 PlanIndex : { 0, 2 })
		{
			for (const EBathhouseShellPart Part : { EBathhouseShellPart::Floor, EBathhouseShellPart::Wall, EBathhouseShellPart::Ceiling,
				EBathhouseShellPart::StairWall, EBathhouseShellPart::StairStep })
			{
				Visible.Append(Plans[PlanIndex].Get(Part));
			}
		}
		int32 Overlaps = 0;
		for (int32 A = 0; A < Visible.Num(); ++A)
		{
			for (int32 B = A + 1; B < Visible.Num(); ++B)
			{
				const FVector Gap = (Visible[A].Center - Visible[B].Center).GetAbs() - (Visible[A].HalfExtent + Visible[B].HalfExtent);
				Overlaps += (Gap.X < -TestTolerance && Gap.Y < -TestTolerance && Gap.Z < -TestTolerance) ? 1 : 0;
			}
		}
		TestEqual(TEXT("A2 visible stair, slab and wall boxes never overlap in volume"), Overlaps, 0);
		TestFalse(TEXT("A2 upper end wall skips the floor slab band"),
			PlanContains(Plans[0], EBathhouseShellPart::StairWall, FVector(UpperEnd.X, UpperEnd.Y, ZUpper - S * 0.5)));
		TestFalse(TEXT("A2 lower end wall skips the ceiling slab band"),
			PlanContains(Plans[0], EBathhouseShellPart::StairWall, FVector(LowerEnd.X, LowerEnd.Y, ZLowerCeiling + S * 0.5)));
	}

	// 구멍 막이: R 위 [바닥, 천장].
	const TArray<FBathhouseBoxPart>& KeepClear = Plans[0].Get(EBathhouseShellPart::StairKeepClear);
	TestEqual(TEXT("EXP-004 one keep-clear box"), KeepClear.Num(), 1);
	if (KeepClear.Num() == 1)
	{
		TestTrue(TEXT("EXP-004 keep-clear XY = hole"), BoxToRect(KeepClear[0]).Min.Equals(Hole.Min, TestTolerance)
			&& BoxToRect(KeepClear[0]).Max.Equals(Hole.Max, TestTolerance));
		ExpectNear(*this, TEXT("EXP-004 keep-clear bottom = upper floor"), KeepClear[0].Center.Z - KeepClear[0].HalfExtent.Z, ZUpper, TestTolerance);
		ExpectNear(*this, TEXT("EXP-004 keep-clear top = upper ceiling"), KeepClear[0].Center.Z + KeepClear[0].HalfExtent.Z,
			FBathhouseSpaceLayout::CeilingZ(Hall), TestTolerance);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseBuildingLayoutChunksTest,
	"BathhouseSim.Building.Layout.CleaningChunks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseBuildingLayoutChunksTest::RunTest(const FString& Parameters)
{
	const FBathhouseLayoutValues Values = FixtureValues();
	const FBox2D Rect(FVector2D(-1000, -700), FVector2D(1000, 700));
	for (const FVector2D& Max : { Values.ChunkMaxSizeCm, FVector2D(300, 500), FVector2D(5000, 5000) })
	{
		TArray<FBox2D> Chunks;
		FBathhouseSpaceLayout::SplitChunks(Rect, Max, Chunks);
		const double SizeX = Rect.Max.X - Rect.Min.X;
		const double SizeY = Rect.Max.Y - Rect.Min.Y;
		const int32 CountX = FMath::Max(1, FMath::CeilToInt(SizeX / Max.X));
		const int32 CountY = FMath::Max(1, FMath::CeilToInt(SizeY / Max.Y));
		TestEqual(TEXT("EXP-010 chunk count per axis"), Chunks.Num(), CountX * CountY);
		double Area = 0.0;
		for (int32 A = 0; A < Chunks.Num(); ++A)
		{
			const double CellX = Chunks[A].Max.X - Chunks[A].Min.X;
			const double CellY = Chunks[A].Max.Y - Chunks[A].Min.Y;
			ExpectNear(*this, TEXT("EXP-010 chunk X size is equal"), CellX, SizeX / CountX, TestTolerance);
			ExpectNear(*this, TEXT("EXP-010 chunk Y size is equal"), CellY, SizeY / CountY, TestTolerance);
			TestTrue(TEXT("EXP-010 chunk is not larger than the maximum"), CellX <= Max.X + TestTolerance && CellY <= Max.Y + TestTolerance);
			Area += RectArea(Chunks[A]);
			for (int32 B = A + 1; B < Chunks.Num(); ++B)
			{
				ExpectNear(*this, TEXT("EXP-010 chunks do not overlap"), IntersectionArea(Chunks[A], Chunks[B]), 0.0, TestTolerance);
			}
		}
		ExpectNear(*this, TEXT("EXP-010 chunks cover the whole interior"), Area, RectArea(Rect), TestTolerance);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseBuildingValidationRulesTest,
	"BathhouseSim.Building.Validation.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseBuildingValidationRulesTest::RunTest(const FString& Parameters)
{
	const FBathhouseLayoutValues Values = FixtureValues();
	const FBathhouseValidationInputs Inputs = MakeInputs(Values);
	const FSnapshots Valid = MakeValidSnapshots(Values);
	{
		FProblems Problems;
		FBathhouseSpaceValidation::ValidateLayout(Valid, Inputs, Problems);
		TestEqual(TEXT("Valid fixture has no errors"), CountSeverity(Problems, EBathhouseProblemSeverity::Error), 0);
		TestEqual(TEXT("Valid fixture has no warnings"), CountSeverity(Problems, EBathhouseProblemSeverity::Warning), 0);
	}

	struct FCase
	{
		const TCHAR* Name;
		TFunction<void(FSnapshots&, FBathhouseValidationInputs&)> Mutate;
		EBathhouseProblemCode Code;
		EBathhouseProblemSeverity Severity;
	};
	const EBathhouseProblemSeverity Error = EBathhouseProblemSeverity::Error;
	const TArray<FCase> Cases = {
		{ TEXT("missing kind"), [](FSnapshots& S, FBathhouseValidationInputs&) { S.RemoveAt(2); }, EBathhouseProblemCode::KindMissing, Error },
		{ TEXT("duplicate kind"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[1].Kind = EBathhouseSpaceKind::Hall; }, EBathhouseProblemCode::KindDuplicate, Error },
		{ TEXT("rotated or scaled"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].bTransformValid = false; }, EBathhouseProblemCode::TransformInvalid, Error },
		{ TEXT("zero floor size"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[1].Interior = FBox2D(S[1].Interior.Min, S[1].Interior.Min); }, EBathhouseProblemCode::ValueInvalid, Error },
		{ TEXT("zero light spacing"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].LightSpacingCm = 0.0; }, EBathhouseProblemCode::ValueInvalid, Error },
		{ TEXT("empty allowed tags"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[1].bAllowedTagsEmpty = true; }, EBathhouseProblemCode::TagsEmpty, Error },
		{ TEXT("volume overlap"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[2].FloorZ = S[0].FloorZ; }, EBathhouseProblemCode::Overlap, Error },
		{ TEXT("work too shallow"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[2].FloorZ = S[0].FloorZ - 100.0; }, EBathhouseProblemCode::Overlap, Error },
		{ TEXT("opening width zero"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].Openings[0].WidthCm = 0.0; }, EBathhouseProblemCode::OpeningInvalid, Error },
		{ TEXT("opening taller than ceiling"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].Openings[0].HeightCm = S[0].CeilingHeightCm + 10.0; }, EBathhouseProblemCode::OpeningInvalid, Error },
		{ TEXT("opening outside the wall"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].Openings[0].CenterOffsetCm = S[0].Interior.Max.Y; }, EBathhouseProblemCode::OpeningOutsideWall, Error },
		{ TEXT("openings overlap"), [](FSnapshots& S, FBathhouseValidationInputs&)
			{ FBathhouseOpeningSnapshot Copy = S[0].Openings[0]; Copy.CenterOffsetCm += Copy.WidthCm * 0.5; S[0].Openings.Add(Copy); }, EBathhouseProblemCode::OpeningOverlap, Error },
		{ TEXT("passage does not touch"), [](FSnapshots& S, FBathhouseValidationInputs&) { FBathhouseSpaceValidation::ApplyMove(S[1], FVector2D(300, 0), 0.0); }, EBathhouseProblemCode::PassageNotTouching, Error },
		{ TEXT("passage range outside target wall"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].Openings[1].CenterOffsetCm = 560.0; }, EBathhouseProblemCode::PassageRangeOutside, Error },
		{ TEXT("passage floor Z differs"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[1].FloorZ += 10.0; }, EBathhouseProblemCode::PassageFloorZ, Error },
		{ TEXT("passage taller than target ceiling"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[1].CeilingHeightCm = S[0].Openings[1].HeightCm - 10.0; }, EBathhouseProblemCode::PassageHeight, Error },
		{ TEXT("outside entrance blocked by another space"), [](FSnapshots& S, FBathhouseValidationInputs&)
			{ S[0].Openings[1].ConnectedIndex = INDEX_NONE; }, EBathhouseProblemCode::OutsideOpeningBlocked, Error },
		{ TEXT("hall has no outside entrance"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].Openings.RemoveAt(0); }, EBathhouseProblemCode::HallNoEntrance, Error },
		{ TEXT("no stairs to work"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].Stairs.Reset(); }, EBathhouseProblemCode::WorkNoStairs, Error },
		{ TEXT("stair lower space missing"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].Stairs[0].LowerIndex = INDEX_NONE; }, EBathhouseProblemCode::StairInvalid, Error },
		{ TEXT("stair lower is not lower"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].Stairs[0].LowerIndex = 1; }, EBathhouseProblemCode::StairInvalid, Error },
		{ TEXT("stair outside the lower interior"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].Stairs[0].TopEdgeOffsetCm.X = 400.0; }, EBathhouseProblemCode::StairOutside, Error },
		{ TEXT("stair without a landing"), [](FSnapshots& S, FBathhouseValidationInputs&)
			{ S[0].Stairs[0].TopEdgeOffsetCm.X = S[0].Interior.Min.X + S[0].Stairs[0].WidthCm * 0.5; }, EBathhouseProblemCode::StairLanding, Error },
		{ TEXT("stair step material missing"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].Stairs[0].bHasStepMaterial = false; }, EBathhouseProblemCode::MaterialMissing, EBathhouseProblemSeverity::Warning },
		{ TEXT("stair wall material missing"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].Stairs[0].bHasStairWallMaterial = false; }, EBathhouseProblemCode::MaterialMissing, EBathhouseProblemSeverity::Warning },
		{ TEXT("second stair material ignored"), [](FSnapshots& S, FBathhouseValidationInputs&) { const FBathhouseStairSnapshot Copy = S[0].Stairs[0]; S[0].Stairs.Add(Copy); }, EBathhouseProblemCode::MaterialMissing, EBathhouseProblemSeverity::Warning },
		{ TEXT("settings wall thickness zero"), [](FSnapshots&, FBathhouseValidationInputs& I) { I.Layout.WallThicknessCm = 0.0; }, EBathhouseProblemCode::ValueInvalid, Error },
		{ TEXT("settings chunk size zero"), [](FSnapshots&, FBathhouseValidationInputs& I) { I.Layout.ChunkMaxSizeCm.X = 0.0; }, EBathhouseProblemCode::ValueInvalid, Error },
		{ TEXT("stair too steep"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[0].Stairs[0].RunCm = 300.0; }, EBathhouseProblemCode::StairSteep, Error },
		{ TEXT("box mesh invalid"), [](FSnapshots&, FBathhouseValidationInputs& I) { I.bBoxMeshValid = false; }, EBathhouseProblemCode::BoxMeshMissing, Error },
		{ TEXT("litter chunk class missing"), [](FSnapshots&, FBathhouseValidationInputs& I) { I.bLitterClassSet = false; }, EBathhouseProblemCode::ChunkClassMissing, Error },
		{ TEXT("stain chunk class missing"), [](FSnapshots&, FBathhouseValidationInputs& I) { I.bStainClassSet = false; }, EBathhouseProblemCode::ChunkClassMissing, Error },
		{ TEXT("work chunk kind warning"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[2].ChunkKind = EBathhouseCleaningChunkKind::Litter; }, EBathhouseProblemCode::WorkChunkKind, EBathhouseProblemSeverity::Warning },
		{ TEXT("missing material warning"), [](FSnapshots& S, FBathhouseValidationInputs&) { S[1].bHasWallMaterial = false; }, EBathhouseProblemCode::MaterialMissing, EBathhouseProblemSeverity::Warning },
	};
	for (const FCase& Case : Cases)
	{
		FSnapshots Mutated = Valid;
		FBathhouseValidationInputs MutatedInputs = Inputs;
		Case.Mutate(Mutated, MutatedInputs);
		FProblems Problems;
		FBathhouseSpaceValidation::ValidateLayout(Mutated, MutatedInputs, Problems);
		const bool bFound = Problems.ContainsByPredicate([&Case](const FBathhouseLayoutProblem& P)
		{
			return P.Code == Case.Code && P.Severity == Case.Severity;
		});
		TestTrue(FString::Printf(TEXT("Rule '%s' reports its problem"), Case.Name), bFound);
	}

	// 목욕공간만 바깥 출입구가 없어도 오류가 아니다(홀에만 요구).
	{
		FProblems Problems;
		FBathhouseSpaceValidation::ValidateLayout(Valid, Inputs, Problems);
		TestFalse(TEXT("Bath without an outside entrance is not an error"), HasCode(Problems, EBathhouseProblemCode::HallNoEntrance));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseBuildingValidationSuggestionTest,
	"BathhouseSim.Building.Validation.PositionSuggestions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseBuildingValidationSuggestionTest::RunTest(const FString& Parameters)
{
	const FBathhouseLayoutValues Values = FixtureValues();
	const FBathhouseValidationInputs Inputs = MakeInputs(Values);
	const FSnapshots Valid = MakeValidSnapshots(Values);
	const double T = Values.WallThicknessCm;
	const double ExpectedBathX = Valid[0].Interior.Max.X + 2.0 * T + (Valid[1].Interior.Max.X - Valid[1].Interior.Min.X) * 0.5;
	const FString ExpectedBathText = FString::SanitizeFloat(ExpectedBathX, 0);

	for (const double Shift : { 300.0, -30.0 })
	{
		FSnapshots Moved = Valid;
		FBathhouseSpaceValidation::ApplyMove(Moved[1], FVector2D(Shift, 0.0), 0.0);
		FProblems Problems;
		FBathhouseSpaceValidation::ValidateLayout(Moved, Inputs, Problems);
		// 통로를 적은 홀을 옮기는 값: 목욕공간 안쪽 서쪽 변 - 2 벽 - 홀 안쪽 X/2.
		const double ExpectedHallX = Moved[1].Interior.Min.X - 2.0 * T - (Valid[0].Interior.Max.X - Valid[0].Interior.Min.X) * 0.5;
		const FString ExpectedHallText = FString::SanitizeFloat(ExpectedHallX, 0);
		const EBathhouseProblemCode Expected = Shift > 0.0 ? EBathhouseProblemCode::PassageNotTouching : EBathhouseProblemCode::Overlap;
		const FBathhouseLayoutProblem* Problem = Problems.FindByPredicate([Expected](const FBathhouseLayoutProblem& P) { return P.Code == Expected; });
		TestNotNull(*FString::Printf(TEXT("Shift %.0f reports the touching problem"), Shift), Problem);
		if (Problem)
		{
			const FString Message = Problem->Message.ToString();
			TestTrue(FString::Printf(TEXT("Shift %.0f suggests the connected space location, got: %s"), Shift, *Message),
				Message.Contains(ExpectedBathText));
			TestTrue(TEXT("The passage owner location is suggested as the alternative"), Message.Contains(ExpectedHallText));
		}
		// 제안 값을 적용하면 같은 오류가 사라지고 새 겹침이 없다.
		FBathhouseLocationSuggestion Suggestion;
		TestTrue(TEXT("Suggestion for the connected space exists"),
			FBathhouseSpaceValidation::SuggestTouchingLocation(Moved, 0, 1, EBathhouseSpaceSide::East, T, true, Suggestion));
		ExpectNear(*this, TEXT("Suggested X = hall max + 2 wall + half bath"), Suggestion.NewLocation.X, ExpectedBathX, TestTolerance);
		FSnapshots Fixed = Moved;
		FBathhouseSpaceValidation::ApplyMove(Fixed[Suggestion.MoveIndex], Suggestion.DeltaXY, Suggestion.DeltaZ);
		FProblems After;
		FBathhouseSpaceValidation::ValidateLayout(Fixed, Inputs, After);
		TestEqual(TEXT("Applying the suggestion leaves no errors"), CountSeverity(After, EBathhouseProblemSeverity::Error), 0);
	}

	// 바닥 Z 차이: 연결 대상 Z를 통로 쪽 Z로 맞추는 값.
	{
		FSnapshots Moved = Valid;
		Moved[1].FloorZ += 10.0;
		FProblems Problems;
		FBathhouseSpaceValidation::ValidateLayout(Moved, Inputs, Problems);
		const FBathhouseLayoutProblem* Problem = Problems.FindByPredicate([](const FBathhouseLayoutProblem& P) { return P.Code == EBathhouseProblemCode::PassageFloorZ; });
		TestNotNull(TEXT("Floor Z difference is reported"), Problem);
		if (Problem)
		{
			TestTrue(TEXT("Floor Z suggestion names the passage-side Z"), Problem->Message.ToString().Contains(FString::SanitizeFloat(Valid[0].FloorZ, 0)));
		}
	}

	// 통로 없는 겹침은 겹침이 작은 축으로 각 공간을 뗀다.
	{
		FSnapshots Moved = Valid;
		Moved[0].Openings.RemoveAt(1);
		FBathhouseSpaceValidation::ApplyMove(Moved[1], FVector2D(-30.0, 0.0), 0.0);
		FProblems Problems;
		FBathhouseSpaceValidation::ValidateLayout(Moved, Inputs, Problems);
		const FBathhouseLayoutProblem* Problem = Problems.FindByPredicate([](const FBathhouseLayoutProblem& P) { return P.Code == EBathhouseProblemCode::Overlap; });
		TestNotNull(TEXT("Overlap without a passage is reported"), Problem);
		if (Problem)
		{
			const FString Message = Problem->Message.ToString();
			const double OverlapAmount = 30.0;
			TestTrue(TEXT("Overlap suggestion moves hall west by the overlap"),
				Message.Contains(FString::SanitizeFloat(Valid[0].ActorXY.X - OverlapAmount, 0)));
			TestTrue(TEXT("Overlap suggestion moves bath east by the overlap"),
				Message.Contains(FString::SanitizeFloat(Valid[1].ActorXY.X, 0)));
		}
	}

	// 제안이 오류를 못 없애면 제안하지 않는다(목욕공간이 통로 벽 방향으로 멀리 떨어져 있고 벽이 통로보다 좁음).
	{
		FSnapshots Moved = Valid;
		FBathhouseSpaceValidation::ApplyMove(Moved[1], FVector2D(300.0, 3000.0), 0.0);
		Moved[1].Interior = FBox2D(
			FVector2D(Moved[1].Interior.Min.X, Moved[1].ActorXY.Y - Moved[0].Openings[1].WidthCm * 0.25),
			FVector2D(Moved[1].Interior.Max.X, Moved[1].ActorXY.Y + Moved[0].Openings[1].WidthCm * 0.25));
		FProblems Problems;
		FBathhouseSpaceValidation::ValidateLayout(Moved, Inputs, Problems);
		const FBathhouseLayoutProblem* Problem = Problems.FindByPredicate([](const FBathhouseLayoutProblem& P) { return P.Code == EBathhouseProblemCode::PassageNotTouching; });
		TestNotNull(TEXT("Unfixable passage is reported"), Problem);
		if (Problem)
		{
			TestFalse(TEXT("No suggestion is attached when it cannot remove the error"), Problem->Message.ToString().Contains(TEXT("제안")));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseBuildingValidationNavigationTest,
	"BathhouseSim.Building.Validation.Navigation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseBuildingValidationNavigationTest::RunTest(const FString& Parameters)
{
	const FBathhouseLayoutValues Values = FixtureValues();
	const FBathhouseValidationInputs Inputs = MakeInputs(Values);
	const FSnapshots Snapshots = MakeValidSnapshots(Values);
	const FBathhouseSpaceSnapshot& Hall = Snapshots[0];
	const FBathhouseSpaceSnapshot& Bath = Snapshots[1];
	const FBathhouseSpaceSnapshot& Work = Snapshots[2];
	const double EntranceWidth = Hall.Openings[0].WidthCm;
	const double Margin = 10.0;
	// 홀·목욕공간과 출입구 밖 마당을 덮고 지하 바닥(Z)은 넣지 않는 범위.
	FBox Cover(
		FVector(Hall.Interior.Min.X - Values.WallThicknessCm * 2.0 - EntranceWidth - Margin, Hall.Interior.Min.Y - Margin, Work.FloorZ + Margin),
		FVector(Bath.Interior.Max.X + Margin, Hall.Interior.Max.Y + Margin, Hall.FloorZ + Hall.CeilingHeightCm));
	FProblems Problems;
	FBathhouseSpaceValidation::ValidateNavigation(Snapshots, { Cover }, Inputs, Problems);
	TestEqual(TEXT("Covering Nav range has no problems"), Problems.Num(), 0);

	Problems.Reset();
	FBox WithWork = Cover;
	WithWork.Min.Z = Work.FloorZ - Margin;
	FBathhouseSpaceValidation::ValidateNavigation(Snapshots, { WithWork }, Inputs, Problems);
	TestTrue(TEXT("Nav range covering the work floor is an error"), HasCode(Problems, EBathhouseProblemCode::NavWorkCovered));

	Problems.Reset();
	FBox TooSmall = Cover;
	TooSmall.Max.X = Hall.Interior.Max.X;
	FBathhouseSpaceValidation::ValidateNavigation(Snapshots, { TooSmall }, Inputs, Problems);
	TestTrue(TEXT("Nav range missing the bath floor is an error"), HasCode(Problems, EBathhouseProblemCode::NavOutside));

	Problems.Reset();
	FBox NoCourtyard = Cover;
	NoCourtyard.Min.X = Hall.Interior.Min.X - Values.WallThicknessCm;
	FBathhouseSpaceValidation::ValidateNavigation(Snapshots, { NoCourtyard }, Inputs, Problems);
	TestTrue(TEXT("Nav range missing the point outside the entrance is an error"), HasCode(Problems, EBathhouseProblemCode::NavOutside));

	Problems.Reset();
	FBathhouseSpaceValidation::ValidateNavigation(Snapshots, {}, Inputs, Problems);
	TestTrue(TEXT("No Nav volume at all is an error"), HasCode(Problems, EBathhouseProblemCode::NavOutside));
	return true;
}

namespace
{
	struct FBuildingWorld
	{
		UWorld* World = nullptr;
		FWorldContext* Context = nullptr;
		TArray<ABathhouseSpaceActor*> Spaces;
		FSnapshots Snapshots;
		FBathhouseLayoutValues Values;

		bool Create(FAutomationTestBase& Test, const TCHAR* BaseName)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is required."));
				return false;
			}
			// Nav 범위 volume이 없는 test world이므로 손님 길 범위 오류만 예상한다.
			Test.AddExpectedError(TEXT("손님 길 범위"), EAutomationExpectedErrorFlags::Contains, 0);
			const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), BaseName);
			Context = &GEngine->CreateNewWorldContext(EWorldType::Game);
			World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
			if (!World)
			{
				GEngine->DestroyWorldContext(World);
				Test.AddError(TEXT("Failed to create the building world."));
				return false;
			}
			World->AddToRoot();
			Context->SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();

			Values = FixtureValues();
			Snapshots = MakeValidSnapshots(Values);
			const TCHAR* Names[] = { TEXT("Space_Hall"), TEXT("Space_Bath"), TEXT("Space_Work") };
			for (int32 Index = 0; Index < Snapshots.Num(); ++Index)
			{
				Spaces.Add(FBathhouseBuildingAutomationAccess::SpawnConfigured(*World, Snapshots[Index], Names[Index]));
			}
			if (Spaces.Contains(nullptr))
			{
				Test.AddError(TEXT("Failed to spawn a space actor."));
				return false;
			}
			const TArray<TArray<const TCHAR*>> Tags = {
				{ TEXT("Facility.Type.ClothesLocker"), TEXT("Facility.Type.DrinkFridge"), TEXT("Facility.Type.MassageChair"),
				  TEXT("Facility.Type.RestBench"), TEXT("Facility.Type.Television"), TEXT("Facility.Type.Vanity") },
				{ TEXT("Facility.Type.Bath"), TEXT("Facility.Type.Shower"), TEXT("Facility.Type.ScrubTable") },
				{ TEXT("Facility.Type.Boiler"), TEXT("Facility.Type.Cooler"), TEXT("Facility.Type.Circulator"),
				  TEXT("Facility.Type.Washer"), TEXT("Facility.Type.Dryer") } };
			for (int32 Index = 0; Index < Spaces.Num(); ++Index)
			{
				FBathhouseBuildingAutomationAccess::Link(Spaces, Snapshots[Index], *Spaces[Index]);
				for (const TCHAR* Tag : Tags[Index])
				{
					FBathhouseBuildingAutomationAccess::AddAllowedTag(*Spaces[Index], FGameplayTag::RequestGameplayTag(FName(Tag)));
				}
			}
			for (ABathhouseSpaceActor* Space : Spaces)
			{
				Space->FinishSpawning(Space->GetActorTransform());
			}
			for (ABathhouseSpaceActor* Space : Spaces)
			{
				if (!Space->HasActorBegunPlay())
				{
					Space->DispatchBeginPlay();
				}
			}
			return true;
		}

		void Destroy()
		{
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
				World->RemoveFromRoot();
				World = nullptr;
			}
		}
	};

	struct FWorldBox
	{
		FVector Center = FVector::ZeroVector;
		FVector Half = FVector::ZeroVector;
	};

	TArray<FWorldBox> ReadPartBoxes(const UInstancedStaticMeshComponent* Component, const UStaticMesh* Mesh)
	{
		TArray<FWorldBox> Boxes;
		if (!Component || !Mesh)
		{
			return Boxes;
		}
		const FBoxSphereBounds Bounds = Mesh->GetBounds();
		for (int32 Index = 0; Index < Component->GetInstanceCount(); ++Index)
		{
			FTransform Transform;
			Component->GetInstanceTransform(Index, Transform, true);
			FWorldBox Box;
			Box.Center = Transform.TransformPosition(Bounds.Origin);
			Box.Half = Bounds.BoxExtent * Transform.GetScale3D().GetAbs();
			Boxes.Add(Box);
		}
		return Boxes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseBuildingWorldShellTest,
	"BathhouseSim.Building.World.ShellChunksLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseBuildingWorldShellTest::RunTest(const FString& Parameters)
{
	FBuildingWorld Fixture;
	if (!Fixture.Create(*this, TEXT("BuildingShellAutomationWorld")))
	{
		Fixture.Destroy();
		return false;
	}
	UWorld* World = Fixture.World;
	ABathhouseSpaceActor* Hall = Fixture.Spaces[0];
	ABathhouseSpaceActor* Bath = Fixture.Spaces[1];
	ABathhouseSpaceActor* Work = Fixture.Spaces[2];
	const UBathhouseBuildingSettings* Settings = GetDefault<UBathhouseBuildingSettings>();
	const UStaticMesh* Mesh = Settings->LoadShellBoxMesh();
	TestNotNull(TEXT("Project Settings provide the shell box mesh"), Mesh);

	// part별 component와 충돌 설정(EXP-005).
	UBathhouseSpaceShellComponent* HallShell = Hall->GetShell();
	TestNotNull(TEXT("Hall shell component exists"), HallShell);
	if (!HallShell)
	{
		Fixture.Destroy();
		return false;
	}
	TArray<FBathhouseSpacePlan> Plans;
	FBathhouseSpaceLayout::Build(Fixture.Snapshots, Fixture.Values, Plans);
	for (const EBathhouseShellPart Part : { EBathhouseShellPart::Floor, EBathhouseShellPart::Wall, EBathhouseShellPart::Ceiling,
		EBathhouseShellPart::StairStep, EBathhouseShellPart::StairRamp, EBathhouseShellPart::StairWall, EBathhouseShellPart::StairKeepClear })
	{
		TestEqual(FString::Printf(TEXT("Hall part %d instance count matches the plan"), static_cast<int32>(Part)),
			HallShell->GetPartInstanceCount(Part), Plans[0].Get(Part).Num());
	}
	for (const EBathhouseShellPart Part : { EBathhouseShellPart::Wall, EBathhouseShellPart::Ceiling })
	{
		const UInstancedStaticMeshComponent* Component = HallShell->FindPartComponent(Part);
		if (Component)
		{
			TestEqual(TEXT("EXP-005 wall/ceiling object type is WorldStatic"), Component->GetCollisionObjectType(), ECC_WorldStatic);
			TestTrue(TEXT("EXP-005 wall/ceiling collide for queries and physics"), Component->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);
			for (const ECollisionChannel Channel : { ECC_Pawn, ECC_PhysicsBody, ECC_Visibility, BathhousePlacementCollision::ZoneTraceChannel })
			{
				TestEqual(TEXT("EXP-005 wall/ceiling block the channel"), Component->GetCollisionResponseToChannel(Channel), ECR_Block);
			}
			TestEqual(TEXT("EXP-005 shell parts are static"), Component->Mobility.GetValue(), EComponentMobility::Static);
		}
		else
		{
			AddError(TEXT("EXP-005 wall/ceiling component missing"));
		}
	}
	if (const UInstancedStaticMeshComponent* Floor = HallShell->FindPartComponent(EBathhouseShellPart::Floor))
	{
		TestEqual(TEXT("Floor is WorldStatic so the cleaning Floor Rule accepts it"), Floor->GetCollisionObjectType(), ECC_WorldStatic);
		TestEqual(TEXT("Floor is Static mobility"), Floor->Mobility.GetValue(), EComponentMobility::Static);
		TestTrue(TEXT("Floor affects navigation"), Floor->CanEverAffectNavigation());
	}
	if (const UInstancedStaticMeshComponent* Ceiling = HallShell->FindPartComponent(EBathhouseShellPart::Ceiling))
	{
		TestFalse(TEXT("Ceiling does not affect navigation"), Ceiling->CanEverAffectNavigation());
	}
	if (const UInstancedStaticMeshComponent* Ramp = HallShell->FindPartComponent(EBathhouseShellPart::StairRamp))
	{
		TestTrue(TEXT("EXP-004 ramp blocks everything"), Ramp->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
		TestFalse(TEXT("EXP-004 ramp is not navigation relevant"), Ramp->CanEverAffectNavigation());
		TestFalse(TEXT("EXP-004 ramp is invisible"), Ramp->IsVisible());
	}
	if (const UInstancedStaticMeshComponent* Step = HallShell->FindPartComponent(EBathhouseShellPart::StairStep))
	{
		TestTrue(TEXT("EXP-004 step plates have no collision"), Step->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	}
	if (const UInstancedStaticMeshComponent* StairWall = HallShell->FindPartComponent(EBathhouseShellPart::StairWall))
	{
		TestTrue(TEXT("EXP-004 stair walls are navigation relevant"), StairWall->CanEverAffectNavigation());
	}
	if (const UInstancedStaticMeshComponent* KeepClear = HallShell->FindPartComponent(EBathhouseShellPart::StairKeepClear))
	{
		TestTrue(TEXT("Keep-clear is query only"), KeepClear->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
		TestEqual(TEXT("Keep-clear ignores the placement trace channel"), KeepClear->GetCollisionResponseToChannel(BathhousePlacementCollision::ZoneTraceChannel), ECR_Ignore);
		TestEqual(TEXT("Keep-clear blocks PhysicsBody"), KeepClear->GetCollisionResponseToChannel(ECC_PhysicsBody), ECR_Block);
		TestEqual(TEXT("Keep-clear ignores Pawn"), KeepClear->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
	}
	// 실제 instance transform이 계획과 같다(상자 mesh bounds에서 scale·offset 파생).
	if (const UInstancedStaticMeshComponent* Floor = HallShell->FindPartComponent(EBathhouseShellPart::Floor))
	{
		const TArray<FWorldBox> Boxes = ReadPartBoxes(Floor, Mesh);
		const TArray<FBathhouseBoxPart>& Planned = Plans[0].Get(EBathhouseShellPart::Floor);
		TestEqual(TEXT("Floor ISM box count equals the plan"), Boxes.Num(), Planned.Num());
		for (int32 Index = 0; Index < FMath::Min(Boxes.Num(), Planned.Num()); ++Index)
		{
			TestTrue(TEXT("Floor ISM instance center equals the plan"), Boxes[Index].Center.Equals(Planned[Index].Center, 0.1));
			TestTrue(TEXT("Floor ISM instance half extent equals the plan"), Boxes[Index].Half.Equals(Planned[Index].HalfExtent, 0.1));
		}
	}
	TestEqual(TEXT("Hall light count matches the plan"), HallShell->GetLightCount(), Plans[0].LightLocations.Num());

	// 라이프사이클: 두 번 연속 재생성해도 component 수가 같다.
	const int32 CountBefore = HallShell->GetGeneratedComponentCount();
	Hall->RebuildShell();
	Hall->RebuildShell();
	TestEqual(TEXT("Rebuilding twice keeps the generated component count"), HallShell->GetGeneratedComponentCount(), CountBefore);
	TestTrue(TEXT("Generated component count is positive"), CountBefore > 0);

	// EXP-013: 구역 bounds = 안쪽 직사각형.
	for (int32 Index = 0; Index < Fixture.Spaces.Num(); ++Index)
	{
		const FBox2D Interior = Fixture.Spaces[Index]->GetInteriorRect();
		const UBoxComponent* Bounds = Fixture.Spaces[Index]->GetZoneBounds();
		ExpectNear(*this, TEXT("EXP-013 zone X extent = half interior"), Bounds->GetUnscaledBoxExtent().X, (Interior.Max.X - Interior.Min.X) * 0.5, TestTolerance);
		ExpectNear(*this, TEXT("EXP-013 zone Y extent = half interior"), Bounds->GetUnscaledBoxExtent().Y, (Interior.Max.Y - Interior.Min.Y) * 0.5, TestTolerance);
		TestTrue(TEXT("EXP-013 zone center = interior center"), FVector2D(Bounds->GetComponentLocation().X, Bounds->GetComponentLocation().Y).Equals(Interior.GetCenter(), TestTolerance));
		TestEqual(TEXT("EXP-013 space kind getter"), Fixture.Spaces[Index]->GetSpaceKind(), Fixture.Snapshots[Index].Kind);
	}

	// EXP-010: 종류별 조각만 생기고 수·바닥 Z·크기가 맞는다.
	auto CheckChunks = [&](const TCHAR* Label, ABathhouseSpaceActor* Space, const bool bExpectLitter, const bool bExpectStain)
	{
		TArray<FBox2D> Rects;
		FBathhouseSpaceLayout::SplitChunks(Space->GetInteriorRect(), Settings->GetCleaningChunkMaxSizeCm(), Rects);
		int32 LitterCount = 0;
		int32 StainCount = 0;
		for (TActorIterator<ALitterSpawnZoneActor> It(World); It; ++It)
		{
			if (It->GetOwner() == Space)
			{
				++LitterCount;
				ExpectNear(*this, *FString::Printf(TEXT("EXP-010 %s litter chunk floor Z = space floor"), Label), It->GetSpawnFloor()->GetComponentLocation().Z, Space->GetFloorZ(), TestTolerance);
				const FVector Extent = It->GetSpawnBounds()->GetUnscaledBoxExtent();
				const FBox2D& Rect = Rects[0];
				ExpectNear(*this, *FString::Printf(TEXT("EXP-010 %s litter chunk X half size"), Label), Extent.X, (Rect.Max.X - Rect.Min.X) * 0.5, TestTolerance);
				ExpectNear(*this, *FString::Printf(TEXT("EXP-010 %s litter chunk Y half size"), Label), Extent.Y, (Rect.Max.Y - Rect.Min.Y) * 0.5, TestTolerance);
			}
		}
		for (TActorIterator<AStainSpawnZoneActor> It(World); It; ++It)
		{
			if (It->GetOwner() == Space)
			{
				++StainCount;
				ExpectNear(*this, *FString::Printf(TEXT("EXP-010 %s stain chunk floor Z = space floor"), Label), It->GetSpawnFloor()->GetComponentLocation().Z, Space->GetFloorZ(), TestTolerance);
			}
		}
		TestEqual(*FString::Printf(TEXT("EXP-010 %s litter chunk count"), Label), LitterCount, bExpectLitter ? Rects.Num() : 0);
		TestEqual(*FString::Printf(TEXT("EXP-010 %s stain chunk count"), Label), StainCount, bExpectStain ? Rects.Num() : 0);
		TestEqual(*FString::Printf(TEXT("EXP-010 %s tracked chunk count"), Label), Space->GetCleaningChunks().Num(), (bExpectLitter || bExpectStain) ? Rects.Num() : 0);
	};
	CheckChunks(TEXT("hall"), Hall, true, false);
	CheckChunks(TEXT("bath"), Bath, false, true);
	CheckChunks(TEXT("work"), Work, false, false);

	// 라이프사이클: 이웃 공간 값이 바뀌면 동기화가 통로 구멍을 맞춘다.
	const UInstancedStaticMeshComponent* BathWall = Bath->GetShell()->FindPartComponent(EBathhouseShellPart::Wall);
	const auto GapWidthOnBathWestWall = [&]() -> double
	{
		// 서쪽 벽의 전체 높이 조각 두 개 사이 간격 = 통로 구멍 폭.
		const double WestX = Bath->GetInteriorRect().Min.X - Fixture.Values.WallThicknessCm * 0.5;
		const double Zf = Bath->GetFloorZ();
		TArray<FWorldBox> Sides;
		for (const FWorldBox& Box : ReadPartBoxes(Bath->GetShell()->FindPartComponent(EBathhouseShellPart::Wall), Mesh))
		{
			if (FMath::Abs(Box.Center.X - WestX) < TestTolerance && FMath::Abs((Box.Center.Z - Box.Half.Z) - Zf) < TestTolerance
				&& FMath::Abs(Box.Half.Z * 2.0 - (Bath->GetCeilingZ() - Zf)) < TestTolerance)
			{
				Sides.Add(Box);
			}
		}
		if (Sides.Num() != 2)
		{
			return -1.0;
		}
		Sides.Sort([](const FWorldBox& A, const FWorldBox& B) { return A.Center.Y < B.Center.Y; });
		return (Sides[1].Center.Y - Sides[1].Half.Y) - (Sides[0].Center.Y + Sides[0].Half.Y);
	};
	TestNotNull(TEXT("Bath wall ISM exists"), BathWall);
	const double GapBefore = GapWidthOnBathWestWall();
	const float WidthBefore = FBathhouseBuildingAutomationAccess::GetOpeningWidth(*Hall, 1);
	const float WidthAfter = WidthBefore * 1.5f;
	FBathhouseBuildingAutomationAccess::SetOpeningWidth(*Hall, 1, WidthAfter);
	FBathhouseSpaceEditorSync::RebuildNow(World);
	const double GapAfter = GapWidthOnBathWestWall();
	TestTrue(FString::Printf(TEXT("Neighbor passage hole follows the hall opening (gap %.1f -> %.1f)"), GapBefore, GapAfter),
		GapAfter > GapBefore + TestTolerance);
	const int32 CountAfterSync = Hall->GetShell()->GetGeneratedComponentCount();
	FBathhouseSpaceEditorSync::RebuildNow(World);
	TestEqual(TEXT("Repeated sync keeps the hall component count"), Hall->GetShell()->GetGeneratedComponentCount(), CountAfterSync);

	// EndPlay: 생성 조각 파괴.
	TArray<TWeakObjectPtr<AActor>> Chunks = Hall->GetCleaningChunks();
	TestTrue(TEXT("Hall owns generated chunks before EndPlay"), Chunks.Num() > 0);
	Hall->Destroy();
	for (const TWeakObjectPtr<AActor>& Chunk : Chunks)
	{
		TestFalse(TEXT("EXP-010 EndPlay destroys the generated chunk"), Chunk.IsValid());
	}
	Fixture.Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseSpacePlacementAutomationTest,
	"BathhouseSim.Building.World.SpacePlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseSpacePlacementAutomationTest::RunTest(const FString& Parameters)
{
	UFacilityPlacementSettings* PlacementSettings = GetMutableDefault<UFacilityPlacementSettings>();
	const float SavedGrid = PlacementSettings->GridSizeCm;
	PlacementSettings->GridSizeCm = 10.0f;
	struct FRestore
	{
		UFacilityPlacementSettings* Settings;
		float Grid;
		~FRestore() { Settings->GridSizeCm = Grid; }
	} Restore{ PlacementSettings, SavedGrid };

	FBuildingWorld Fixture;
	if (!Fixture.Create(*this, TEXT("BuildingPlacementAutomationWorld")))
	{
		Fixture.Destroy();
		return false;
	}
	UWorld* World = Fixture.World;
	ABathhouseSpaceActor* Hall = Fixture.Spaces[0];
	ABathhouseSpaceActor* Bath = Fixture.Spaces[1];
	ABathhouseSpaceActor* Work = Fixture.Spaces[2];

	// EXP-008, 009: 공간별 허용 종류 표(계약 4.2). 종류 태그 14개 각각이 정확히 한 공간에만 허용된다.
	const TArray<TPair<const TCHAR*, int32>> KindToSpace = {
		{ TEXT("Facility.Type.ClothesLocker"), 0 }, { TEXT("Facility.Type.DrinkFridge"), 0 }, { TEXT("Facility.Type.MassageChair"), 0 },
		{ TEXT("Facility.Type.RestBench"), 0 }, { TEXT("Facility.Type.Television"), 0 }, { TEXT("Facility.Type.Vanity"), 0 },
		{ TEXT("Facility.Type.Bath"), 1 }, { TEXT("Facility.Type.Shower"), 1 }, { TEXT("Facility.Type.ScrubTable"), 1 },
		{ TEXT("Facility.Type.Boiler"), 2 }, { TEXT("Facility.Type.Cooler"), 2 }, { TEXT("Facility.Type.Circulator"), 2 },
		{ TEXT("Facility.Type.Washer"), 2 }, { TEXT("Facility.Type.Dryer"), 2 } };
	for (const TPair<const TCHAR*, int32>& Entry : KindToSpace)
	{
		UFacilityPlacementDefinition* Definition = NewObject<UFacilityPlacementDefinition>();
		Definition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName(Entry.Key)));
		for (int32 Index = 0; Index < Fixture.Spaces.Num(); ++Index)
		{
			TestEqual(FString::Printf(TEXT("EXP-008 %s allowed in space %d"), Entry.Key, Index),
				Fixture.Spaces[Index]->IsDefinitionAllowed(*Definition), Index == Entry.Value);
		}
	}
	TestEqual(TEXT("EXP-008 not-allowed reason text"),
		AFacilityPlacementZoneActor::GetDefinitionNotAllowedReason().ToString(), FString(TEXT("이 공간에는 놓을 수 없는 설비입니다")));

	// 실제 배치 검증: 공간 Actor를 구역으로 쓴다.
	UFacilityPlacementDefinition* Definition = NewObject<UFacilityPlacementDefinition>();
	Definition->StableId = TEXT("BuildingAutomationShower");
	Definition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Type.Shower")));
	Definition->PlacedFacilityClass = AFacilityPlacementAutomationActor::StaticClass();
	Definition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
	const FVector SourceLocation(Hall->GetActorLocation().X + 600.0, Hall->GetActorLocation().Y + 400.0, Hall->GetFloorZ());
	AFacilityPlacementAutomationActor* Source = World->SpawnActorDeferred<AFacilityPlacementAutomationActor>(
		AFacilityPlacementAutomationActor::StaticClass(), FTransform(SourceLocation));
	Source->ConfigureForTest(*Definition);
	Source->FinishSpawning(FTransform(SourceLocation));
	if (!Source->HasActorBegunPlay())
	{
		Source->DispatchBeginPlay();
	}
	FText Failure;
	APlaceableFacilityItemActor* Item = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Source, Failure);
	TestNotNull(TEXT("Placement fixture item"), Item);
	AActor* PlayerActor = World->SpawnActor<AActor>();
	UPlayerFacilityPlacementComponent* Placement = NewObject<UPlayerFacilityPlacementComponent>(PlayerActor);
	PlayerActor->AddInstanceComponent(Placement);
	Placement->RegisterComponent();
	const AActor* PlacedCDO = Definition->PlacedFacilityClass->GetDefaultObject<AActor>();
	const IPlaceableFacility* Placeable = Cast<IPlaceableFacility>(PlacedCDO);
	UFacilityPlacementComponent* FacilityPlacement = Placeable ? Placeable->GetFacilityPlacementComponent() : nullptr;
	if (!Item || !FacilityPlacement)
	{
		AddError(TEXT("Placement fixture is incomplete."));
		Fixture.Destroy();
		return false;
	}
	auto Validate = [&](ABathhouseSpaceActor& Zone, const FVector& Point)
	{
		FTransform Candidate;
		FText CandidateFailure;
		if (!FacilityPlacement->BuildPlacedActorTransform(Zone.MakeCandidateTransform(Point, 0.0f, false), Candidate, CandidateFailure))
		{
			AddError(FString::Printf(TEXT("Candidate transform failed: %s"), *CandidateFailure.ToString()));
		}
		return Placement->ValidateWorldPlacement(*Item, Candidate, Zone);
	};
	const FBathhouseStairSnapshot& Stair = Fixture.Snapshots[0].Stairs[0];
	const FBox2D Hole = FBathhouseSpaceLayout::StairHole(Fixture.Snapshots[0], Stair);

	const FFacilityPlacementTransactionResult Open = Validate(*Hall, FVector(SourceLocation.X, SourceLocation.Y, Hall->GetFloorZ()));
	TestTrue(FString::Printf(TEXT("EXP-008 open floor of the hall places (floor support on the space floor), got %s"), *Open.FailureReason.ToString()), Open.bSucceeded);
	const FVector2D HoleCenter = Hole.GetCenter();
	const FFacilityPlacementTransactionResult OverHole = Validate(*Hall, FVector(HoleCenter.X, HoleCenter.Y, Hall->GetFloorZ()));
	TestFalse(TEXT("EXP-004 candidate over the stair hole is rejected"), OverHole.bSucceeded);
	TestTrue(TEXT("EXP-004 stair hole rejection is a blocked overlap"), OverHole.FailureCode == EFacilityPlacementFailureCode::Blocked);
	const FBathhouseBoxPart* GuardWall = nullptr;
	TArray<FBathhouseSpacePlan> Plans;
	FBathhouseSpaceLayout::Build(Fixture.Snapshots, Fixture.Values, Plans);
	for (const FBathhouseBoxPart& Box : Plans[0].Get(EBathhouseShellPart::StairWall))
	{
		if (Box.HalfExtent.X > Box.HalfExtent.Y && Box.Center.Y > HoleCenter.Y)
		{
			GuardWall = &Box;
			break;
		}
	}
	TestNotNull(TEXT("A stair side wall exists"), GuardWall);
	if (GuardWall)
	{
		const FFacilityPlacementTransactionResult OnWall = Validate(*Hall, FVector(GuardWall->Center.X, GuardWall->Center.Y, Hall->GetFloorZ()));
		TestFalse(TEXT("EXP-008 candidate overlapping a stair wall is rejected"), OnWall.bSucceeded);
		TestTrue(TEXT("EXP-008 stair wall rejection is a blocked overlap"), OnWall.FailureCode == EFacilityPlacementFailureCode::Blocked);
	}
	// 구역 밖(목욕공간 구역에 홀 지점)은 구역 안에 없다.
	const FFacilityPlacementTransactionResult Outside = Validate(*Bath, FVector(SourceLocation.X, SourceLocation.Y, Hall->GetFloorZ()));
	TestTrue(TEXT("A footprint outside the bath zone is rejected as outside"), !Outside.bSucceeded && Outside.FailureCode == EFacilityPlacementFailureCode::OutsideZone);
	const FFacilityPlacementTransactionResult InBath = Validate(*Bath, FVector(Bath->GetActorLocation().X, Bath->GetActorLocation().Y + 300.0, Bath->GetFloorZ()));
	TestTrue(FString::Printf(TEXT("EXP-008 the bath floor supports a candidate, got %s"), *InBath.FailureReason.ToString()), InBath.bSucceeded);
	const FFacilityPlacementTransactionResult InWork = Validate(*Work, FVector(Work->GetActorLocation().X - 300.0, Work->GetActorLocation().Y + 300.0, Work->GetFloorZ()));
	TestTrue(FString::Printf(TEXT("EXP-009 the work floor supports a candidate, got %s"), *InWork.FailureReason.ToString()), InWork.bSucceeded);

	// 복귀 A1: 실제 TracePlacementZone 경로. 형상(벽·천장·계단 벽) hit는 구역이 아니고 ZoneBounds 윗면 hit만 구역이다.
	UCameraComponent* Camera = NewObject<UCameraComponent>(PlayerActor);
	PlayerActor->AddInstanceComponent(Camera);
	PlayerActor->SetRootComponent(Camera);
	Camera->RegisterComponent();
	Placement->Camera = Camera;
	AFacilityPlacementZoneActor* AimedZone = nullptr;
	auto Aim = [&](const FVector& Location, const FRotator& Rotation)
	{
		Camera->SetWorldLocationAndRotation(Location, Rotation);
		FVector Point;
		AimedZone = nullptr;
		return Placement->TracePlacementZone(AimedZone, Point);
	};
	auto RawHit = [&](const FVector& Location, const FRotator& Rotation)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(BuildingAimProbe), true, PlayerActor);
		World->LineTraceSingleByChannel(Hit, Location,
			Location + Rotation.Vector() * GetDefault<UFacilityPlacementSettings>()->GetPlacementTraceDistance(),
			BathhousePlacementCollision::ZoneTraceChannel, Params);
		return Hit;
	};
	const FBathhouseSpaceSnapshot& HallSnap = Fixture.Snapshots[0];
	const double HallFloor = Hall->GetFloorZ();
	TestTrue(TEXT("A1 looking down at the hall floor picks the hall zone"),
		Aim(FVector(300.0, -400.0, HallFloor + 150.0), FRotator(-90.0, 0.0, 0.0)) && AimedZone == Hall);
	TestTrue(TEXT("A1 looking down at the bath floor picks the bath zone"),
		Aim(FVector(Bath->GetActorLocation().X, 300.0, Bath->GetFloorZ() + 150.0), FRotator(-90.0, 0.0, 0.0)) && AimedZone == Bath);
	{
		const FVector Eye(300.0, -400.0, HallFloor + 100.0);
		const FHitResult CeilingHit = RawHit(Eye, FRotator(90.0, 0.0, 0.0));
		TestTrue(TEXT("A1 the ceiling blocks the placement trace"), CeilingHit.GetActor() == Hall && CeilingHit.GetComponent() != Hall->GetZoneBounds());
		TestFalse(TEXT("A1 looking at the ceiling is not a zone"), Aim(Eye, FRotator(90.0, 0.0, 0.0)));
	}
	{
		const FVector Eye(HallSnap.Interior.Min.X + 400.0, -400.0, HallFloor + 100.0);
		const FHitResult WallHit = RawHit(Eye, FRotator(0.0, 180.0, 0.0));
		TestTrue(TEXT("A1 the wall blocks the placement trace"), WallHit.GetActor() == Hall && WallHit.GetComponent() != Hall->GetZoneBounds());
		TestFalse(TEXT("A1 looking at a wall is not a zone"), Aim(Eye, FRotator(0.0, 180.0, 0.0)));
	}
	{
		const FVector Eye(HoleCenter.X, Hole.Max.Y + 300.0, HallFloor + 60.0);
		const FHitResult StairWallHit = RawHit(Eye, FRotator(0.0, -90.0, 0.0));
		TestTrue(TEXT("A1 the stair wall blocks the placement trace"), StairWallHit.GetActor() == Hall && StairWallHit.GetComponent() != Hall->GetZoneBounds());
		TestFalse(TEXT("A1 looking at a stair wall is not a zone"), Aim(Eye, FRotator(0.0, -90.0, 0.0)));
	}
	{
		const FBathhouseStairFrame Frame = FBathhouseSpaceLayout::MakeStairFrame(Fixture.Snapshots[0], Stair);
		const FVector2D EyeXY = Frame.ToWorld(Stair.RunCm * 0.3, 0.0);
		const double Rise = HallFloor - Work->GetFloorZ();
		const FVector Eye(EyeXY.X, EyeXY.Y, HallFloor - Rise * 0.3 + 20.0);
		const FHitResult UpHit = RawHit(Eye, FRotator(90.0, 0.0, 0.0));
		TestTrue(TEXT("A1 the zone underside is hit from the shaft"), UpHit.GetComponent() == Hall->GetZoneBounds() && UpHit.ImpactNormal.Z < 0.0);
		TestFalse(TEXT("A1 the zone underside is not a zone"), Aim(Eye, FRotator(90.0, 0.0, 0.0)));
	}

	// 복귀 A3: 아래층 천장 판 두께 구간의 외부 blocking 물체도 계단 통로 오류다.
	{
		auto CountStairBlocked = [&]()
		{
			FSnapshots Snaps;
			FProblems Problems;
			FBathhouseSpaceValidation::ValidateWorld(*World, Snaps, Problems);
			int32 Count = 0;
			for (const FBathhouseLayoutProblem& Problem : Problems)
			{
				Count += Problem.Code == EBathhouseProblemCode::StairBlocked ? 1 : 0;
			}
			return Count;
		};
		TestEqual(TEXT("A3 no stair blocker before the obstacle"), CountStairBlocked(), 0);
		AActor* Obstacle = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Obstacle);
		Obstacle->AddInstanceComponent(Box);
		Obstacle->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(10.0));
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->RegisterComponent();
		const double CeilingSlabMid = FBathhouseSpaceLayout::CeilingZ(Fixture.Snapshots[2]) + Fixture.Values.SlabThicknessCm * 0.5;
		Obstacle->SetActorLocation(FVector(HoleCenter.X, HoleCenter.Y, CeilingSlabMid));
		TestTrue(TEXT("A3 an obstacle inside the lower ceiling slab band is a stair error"), CountStairBlocked() > 0);
		Obstacle->Destroy();

		// 회귀: complex 전용 충돌(HLOD 지형 mesh와 같은 성질)은 게임 충돌이 아니므로 계단 통로를 막지 않는다.
		UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		UStaticMesh* ComplexOnlyMesh = CubeMesh ? DuplicateObject<UStaticMesh>(CubeMesh, GetTransientPackage()) : nullptr;
		TestNotNull(TEXT("Complex-only fixture mesh"), ComplexOnlyMesh);
		if (ComplexOnlyMesh && ComplexOnlyMesh->GetBodySetup())
		{
			ComplexOnlyMesh->GetBodySetup()->AggGeom.EmptyElements();
			// 단순 충돌은 상자 모서리의 작은 상자 하나뿐이라 중앙을 지나는 trace는 complex(삼각형)만 맞힌다.
			FKBoxElem CornerBox(2.0f, 2.0f, 2.0f);
			CornerBox.Center = FVector(45.0f, 45.0f, 45.0f);
			ComplexOnlyMesh->GetBodySetup()->AggGeom.BoxElems.Add(CornerBox);
			ComplexOnlyMesh->GetBodySetup()->CollisionTraceFlag = CTF_UseDefault;
			ComplexOnlyMesh->GetBodySetup()->InvalidatePhysicsData();
			ComplexOnlyMesh->GetBodySetup()->CreatePhysicsMeshes();
			AActor* ComplexActor = World->SpawnActor<AActor>();
			UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(ComplexActor);
			ComplexActor->AddInstanceComponent(Mesh);
			ComplexActor->SetRootComponent(Mesh);
			Mesh->SetStaticMesh(ComplexOnlyMesh);
			Mesh->SetCollisionObjectType(ECC_WorldStatic);
			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Mesh->SetCollisionResponseToAllChannels(ECR_Block);
			Mesh->RegisterComponent();
			Mesh->SetWorldScale3D(FVector(0.5));
			ComplexActor->SetActorLocation(FVector(HoleCenter.X, HoleCenter.Y, CeilingSlabMid));
			FCollisionObjectQueryParams Objects;
			Objects.AddObjectTypesToQuery(ECC_WorldStatic);
			const FVector TraceStart(HoleCenter.X, HoleCenter.Y, HallFloor);
			const FVector TraceEnd(HoleCenter.X, HoleCenter.Y, FBathhouseSpaceLayout::CeilingZ(Fixture.Snapshots[2]));
			auto ProbeParams = [&](const bool bComplex)
			{
				FCollisionQueryParams Params(SCENE_QUERY_STAT(BuildingComplexProbe), bComplex);
				Params.AddIgnoredActors(TArray<AActor*>{ Hall, Bath, Work });
				return Params;
			};
			FHitResult ComplexHit, SimpleHit;
			const bool bComplexHits = World->LineTraceSingleByObjectType(ComplexHit, TraceStart, TraceEnd, Objects, ProbeParams(true));
			const bool bSimpleHits = World->LineTraceSingleByObjectType(SimpleHit, TraceStart, TraceEnd, Objects, ProbeParams(false));
			TestTrue(*FString::Printf(TEXT("Fixture has complex-only collision at the center: complex trace hits and simple trace does not (complex %d, simple %d)"), bComplexHits, bSimpleHits), bComplexHits && !bSimpleHits);
			TestEqual(TEXT("Complex-only collision does not block the stair shaft"), CountStairBlocked(), 0);
			ComplexActor->Destroy();
		}
	}

	Fixture.Destroy();
	return true;
}

#endif
