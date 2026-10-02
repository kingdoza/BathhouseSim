#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include <limits>

#include "Building/BathhouseBuildingSettings.h"
#include "Building/BathhouseExpansionPurchaseSubsystem.h"
#include "Building/BathhouseSpaceActor.h"
#include "Building/BathhouseSpaceLayout.h"
#include "Building/BathhouseSpaceShellComponent.h"
#include "Building/BathhouseSpaceValidation.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Building/BathhouseSpacePreviewLabelWidget.h"
#include "Economy/PlayerWalletComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Facility/BathhouseExpansionAuthority.h"
#include "Facility/BathhouseExpansionDefinition.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Facility/BathhouseFacilitySubsystem.h"
#include "Facility/LockerCapacitySubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/BathhouseKeyRackActor.h"
#include "Misc/DataValidation.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Shop/ShopProductRules.h"
#include "Tests/BathhouseExpansionAutomationTestSupport.h"

namespace
{
	using FSnapshots = TArray<FBathhouseSpaceSnapshot>;
	using FProblems = TArray<FBathhouseLayoutProblem>;
	using Access = FBathhouseExpansionAutomationAccess;
	constexpr double ExpTolerance = 0.01;

	// 아래 크기·두께·양은 테스트 fixture 입력이다. 기대값은 항상 같은 입력에서 계산한다.
	FBathhouseLayoutValues FixtureValues()
	{
		FBathhouseLayoutValues Values;
		Values.WallThicknessCm = 20.0;
		Values.SlabThicknessCm = 20.0;
		Values.ChunkMaxSizeCm = FVector2D(400.0, 400.0);
		return Values;
	}

	FBathhouseValidationInputs FixtureInputs()
	{
		FBathhouseValidationInputs Inputs;
		Inputs.Layout = FixtureValues();
		Inputs.bBoxMeshValid = true;
		Inputs.bLitterClassSet = true;
		Inputs.bStainClassSet = true;
		Inputs.WalkableFloorAngleDegrees = GetDefault<UCharacterMovementComponent>()->GetWalkableFloorAngle();
		return Inputs;
	}

	// 줄 가격은 fixture 입력이다(검증이 가격 > 0을 요구한다).
	constexpr int32 FixtureStepPrice = 1000;

	/** 벽 하나짜리 줄. */
	FBathhouseExpansionStepSnapshot MakeStep(const EBathhouseSpaceSide Side, const double Amount, const int32 Price = FixtureStepPrice)
	{
		FBathhouseExpansionStepSnapshot Step;
		Step.Sides.Add({ Side, Amount });
		Step.Price = Price;
		return Step;
	}

	/** 같은 양의 여러 벽 줄. */
	FBathhouseExpansionStepSnapshot MakeMultiStep(const int32 Price, const double Amount, std::initializer_list<EBathhouseSpaceSide> Sides)
	{
		FBathhouseExpansionStepSnapshot Step;
		Step.Price = Price;
		for (const EBathhouseSpaceSide Side : Sides)
		{
			Step.Sides.Add({ Side, Amount });
		}
		return Step;
	}

	FBathhouseSpaceSnapshot MakeSnap(
		const EBathhouseSpaceKind Kind, const TCHAR* Name, const FVector2D& Center, const FVector2D& Size,
		const double FloorZ, const double Ceiling, const EBathhouseCleaningChunkKind Chunk)
	{
		FBathhouseSpaceSnapshot Space;
		Space.Kind = Kind;
		Space.DisplayName = Name;
		Space.ActorXY = Center;
		Space.FloorZ = FloorZ;
		Space.Interior = FBox2D(Center - Size * 0.5, Center + Size * 0.5);
		Space.BaseInterior = Space.Interior;
		Space.CeilingHeightCm = Ceiling;
		Space.LightSpacingCm = 600.0;
		Space.LightCeilingOffsetCm = 20.0;
		Space.ChunkKind = Chunk;
		return Space;
	}

	/** 홀(0), 목욕공간(1, 홀 동쪽, 간격 BathGap), 작업공간(2, 홀 아래). 월드 fixture와 같은 배치와 넓힘 줄. */
	FSnapshots MakeSnapshots(const FBathhouseLayoutValues& Values, const double BathGap = 0.0)
	{
		FBathhouseSpaceSnapshot Hall = MakeSnap(EBathhouseSpaceKind::Hall, TEXT("Hall"), FVector2D(0, 0),
			FVector2D(2000, 1400), 50.0, 350.0, EBathhouseCleaningChunkKind::Litter);
		const double BathWidth = 1000.0;
		const double BathCenterX = Hall.Interior.Max.X + 2.0 * Values.WallThicknessCm + BathWidth * 0.5 + BathGap;
		FBathhouseSpaceSnapshot Bath = MakeSnap(EBathhouseSpaceKind::Bath, TEXT("Bath"), FVector2D(BathCenterX, 0),
			FVector2D(BathWidth, 1300), 50.0, 350.0, EBathhouseCleaningChunkKind::Stain);
		FBathhouseSpaceSnapshot Work = MakeSnap(EBathhouseSpaceKind::Work, TEXT("Work"), FVector2D(-200, 0),
			FVector2D(1400, 900), 50.0 - 400.0, 300.0, EBathhouseCleaningChunkKind::None);
		FBathhouseOpeningSnapshot Entrance;
		Entrance.Side = EBathhouseSpaceSide::West;
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
		const double Amount = FBathhouseExpansionTestWorld::FixtureAmountCm;
		const EBathhouseSpaceSide South = EBathhouseSpaceSide::South;
		const EBathhouseSpaceSide North = EBathhouseSpaceSide::North;
		const EBathhouseSpaceSide East = EBathhouseSpaceSide::East;
		Hall.Steps = { MakeMultiStep(FixtureStepPrice, Amount, { South, North }), MakeMultiStep(FixtureStepPrice, Amount, { South, North }) };
		Bath.Steps = { MakeMultiStep(FixtureStepPrice, Amount, { South, North, East }), MakeMultiStep(FixtureStepPrice, Amount, { South, North, East }) };
		Work.Steps = { MakeMultiStep(FixtureStepPrice, Amount, { East, North }), MakeMultiStep(FixtureStepPrice, Amount, { East, North }) };
		return { Hall, Bath, Work };
	}

	/** 목록 끝 모습까지 덮고 지하 바닥은 넣지 않는 Nav 범위(출입구 밖 마당 포함). */
	FBox MakeCover(const FSnapshots& Snapshots, const FBathhouseLayoutValues& Values, const double Margin)
	{
		const FBathhouseSpaceSnapshot Hall = FBathhouseSpaceLayout::WithExpansionCount(Snapshots[0], Snapshots[0].Steps.Num());
		const FBathhouseSpaceSnapshot Bath = FBathhouseSpaceLayout::WithExpansionCount(Snapshots[1], Snapshots[1].Steps.Num());
		const double Entrance = Snapshots[0].Openings[0].WidthCm;
		return FBox(
			FVector(Hall.Interior.Min.X - Values.WallThicknessCm * 2.0 - Entrance - Margin,
				FMath::Min(Hall.Interior.Min.Y, Bath.Interior.Min.Y) - Margin, Snapshots[2].FloorZ + Margin),
			FVector(Bath.Interior.Max.X + Margin, FMath::Max(Hall.Interior.Max.Y, Bath.Interior.Max.Y) + Margin,
				Hall.FloorZ + Hall.CeilingHeightCm));
	}

	bool HasCode(const FProblems& Problems, const EBathhouseProblemCode Code, const EBathhouseProblemSeverity Severity)
	{
		return Problems.ContainsByPredicate([Code, Severity](const FBathhouseLayoutProblem& P)
		{
			return P.Code == Code && P.Severity == Severity;
		});
	}

	bool HasCodeAny(const FProblems& Problems, const EBathhouseProblemCode Code)
	{
		return Problems.ContainsByPredicate([Code](const FBathhouseLayoutProblem& P) { return P.Code == Code; });
	}

	FProblems ExpansionOnly(const FProblems& Problems)
	{
		FProblems Result;
		for (const FBathhouseLayoutProblem& Problem : Problems)
		{
			if (Problem.Code >= EBathhouseProblemCode::ExpansionAmountInvalid)
			{
				Result.Add(Problem);
			}
		}
		return Result;
	}

	void ExpectNear(FAutomationTestBase& Test, const FString& What, const double Actual, const double Expected, const double Tolerance)
	{
		Test.TestTrue(FString::Printf(TEXT("%s (actual %f, expected %f)"), *What, Actual, Expected),
			FMath::Abs(Actual - Expected) <= Tolerance);
	}

	double RectArea(const FBox2D& Rect) { return (Rect.Max.X - Rect.Min.X) * (Rect.Max.Y - Rect.Min.Y); }

	double IntersectionArea(const FBox2D& A, const FBox2D& B)
	{
		const double X = FMath::Min(A.Max.X, B.Max.X) - FMath::Max(A.Min.X, B.Min.X);
		const double Y = FMath::Min(A.Max.Y, B.Max.Y) - FMath::Max(A.Min.Y, B.Min.Y);
		return X > 0.0 && Y > 0.0 ? X * Y : 0.0;
	}

	bool RectNear(const FBox2D& A, const FBox2D& B)
	{
		return A.Min.Equals(B.Min, ExpTolerance) && A.Max.Equals(B.Max, ExpTolerance);
	}

	/** 한 ISM part의 world XY 바깥 경계. */
	FBox2D PartBounds(const UInstancedStaticMeshComponent* Component, const UStaticMesh* Mesh)
	{
		FBox2D Bounds(ForceInit);
		if (!Component || !Mesh)
		{
			return Bounds;
		}
		const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
		for (int32 Index = 0; Index < Component->GetInstanceCount(); ++Index)
		{
			FTransform Transform;
			Component->GetInstanceTransform(Index, Transform, true);
			const FVector Center = Transform.TransformPosition(MeshBounds.Origin);
			const FVector Half = MeshBounds.BoxExtent * Transform.GetScale3D().GetAbs();
			Bounds += FVector2D(Center.X - Half.X, Center.Y - Half.Y);
			Bounds += FVector2D(Center.X + Half.X, Center.Y + Half.Y);
		}
		return Bounds;
	}

	bool PlansEqual(const FBathhouseSpacePlan& A, const FBathhouseSpacePlan& B)
	{
		for (int32 Part = 0; Part < static_cast<int32>(EBathhouseShellPart::Count); ++Part)
		{
			if (A.Parts[Part].Num() != B.Parts[Part].Num())
			{
				return false;
			}
			for (int32 Index = 0; Index < A.Parts[Part].Num(); ++Index)
			{
				if (!A.Parts[Part][Index].Center.Equals(B.Parts[Part][Index].Center, ExpTolerance)
					|| !A.Parts[Part][Index].HalfExtent.Equals(B.Parts[Part][Index].HalfExtent, ExpTolerance))
				{
					return false;
				}
			}
		}
		if (A.LightLocations.Num() != B.LightLocations.Num() || A.ChunkRects.Num() != B.ChunkRects.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.LightLocations.Num(); ++Index)
		{
			if (!A.LightLocations[Index].Equals(B.LightLocations[Index], ExpTolerance))
			{
				return false;
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseExpansionLayoutTest,
	"BathhouseSim.Expansion.Layout.InteriorBandAndNeighbors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseExpansionLayoutTest::RunTest(const FString& Parameters)
{
	// EXP-023/025 형상: 한 변만 물러나고 새 형상은 옛 바깥 직사각형 밖에만 생긴다.
	const FBox2D Base(FVector2D(-100.0, -50.0), FVector2D(100.0, 50.0));
	const TArray<FBathhouseExpansionStepSnapshot> Steps = {
		MakeStep(EBathhouseSpaceSide::East, 30.0), MakeStep(EBathhouseSpaceSide::West, 10.0),
		MakeStep(EBathhouseSpaceSide::North, 20.0), MakeStep(EBathhouseSpaceSide::South, 5.0) };
	TestTrue(TEXT("Zero steps leave the base"), RectNear(FBathhouseSpaceLayout::ExpandInterior(Base, Steps, 0), Base));
	const FBox2D One = FBathhouseSpaceLayout::ExpandInterior(Base, Steps, 1);
	TestTrue(TEXT("East step moves only the east edge"),
		RectNear(One, FBox2D(Base.Min, FVector2D(Base.Max.X + Steps[0].Sides[0].AmountCm, Base.Max.Y))));
	const FBox2D Two = FBathhouseSpaceLayout::ExpandInterior(Base, Steps, 2);
	TestTrue(TEXT("West step then moves only the west edge"),
		RectNear(Two, FBox2D(FVector2D(Base.Min.X - Steps[1].Sides[0].AmountCm, Base.Min.Y), One.Max)));
	const FBox2D Three = FBathhouseSpaceLayout::ExpandInterior(Base, Steps, 3);
	TestEqual(TEXT("North step moves the north edge"), Three.Max.Y, Base.Max.Y + Steps[2].Sides[0].AmountCm);
	const FBox2D Four = FBathhouseSpaceLayout::ExpandInterior(Base, Steps, 4);
	TestEqual(TEXT("South step moves the south edge"), Four.Min.Y, Base.Min.Y - Steps[3].Sides[0].AmountCm);
	TestTrue(TEXT("Count beyond the list clamps to the end"), RectNear(FBathhouseSpaceLayout::ExpandInterior(Base, Steps, 99), Four));
	TestTrue(TEXT("Negative count clamps to the base"), RectNear(FBathhouseSpaceLayout::ExpandInterior(Base, Steps, -3), Base));

	TArray<FBathhouseExpansionStepSnapshot> WithBad = Steps;
	WithBad.Insert(MakeStep(EBathhouseSpaceSide::East, 0.0), 1);
	WithBad.Insert(MakeStep(EBathhouseSpaceSide::West, -4.0), 2);
	WithBad.Insert(MakeStep(EBathhouseSpaceSide::North, std::numeric_limits<double>::infinity()), 3);
	TestTrue(TEXT("Invalid amounts are skipped in the shape"),
		RectNear(FBathhouseSpaceLayout::ExpandInterior(Base, WithBad, WithBad.Num()), Four));
	{
		TArray<FBox2D> SkippedBand;
		FBathhouseSpaceLayout::ExpansionBandRects(Base, WithBad, 1, SkippedBand);
		TestEqual(TEXT("A skipped step has an empty band"), SkippedBand.Num(), 0);
	}

	for (int32 Index = 0; Index < Steps.Num(); ++Index)
	{
		const FBox2D Before = FBathhouseSpaceLayout::ExpandInterior(Base, Steps, Index);
		const FBox2D After = FBathhouseSpaceLayout::ExpandInterior(Base, Steps, Index + 1);
		TArray<FBox2D> Bands;
		FBathhouseSpaceLayout::ExpansionBandRects(Base, Steps, Index, Bands);
		double BandArea = 0.0;
		TArray<FBox2D> Chunks;
		for (const FBox2D& Band : Bands)
		{
			BandArea += RectArea(Band);
			TestEqual(TEXT("Band does not overlap the previous interior"), IntersectionArea(Band, Before), 0.0);
			TestTrue(TEXT("Band stays inside the next interior"), Band.Min.X >= After.Min.X - ExpTolerance && Band.Max.X <= After.Max.X + ExpTolerance
				&& Band.Min.Y >= After.Min.Y - ExpTolerance && Band.Max.Y <= After.Max.Y + ExpTolerance);
			TArray<FBox2D> BandChunks;
			FBathhouseSpaceLayout::SplitChunks(Band, FixtureValues().ChunkMaxSizeCm, BandChunks);
			Chunks.Append(BandChunks);
		}
		ExpectNear(*this, FString::Printf(TEXT("Band %d area = after - before"), Index), BandArea, RectArea(After) - RectArea(Before), ExpTolerance);
		double Sum = 0.0;
		for (int32 A = 0; A < Chunks.Num(); ++A)
		{
			Sum += RectArea(Chunks[A]);
			for (int32 B = A + 1; B < Chunks.Num(); ++B)
			{
				TestEqual(TEXT("Band chunks do not overlap"), IntersectionArea(Chunks[A], Chunks[B]), 0.0);
			}
		}
		ExpectNear(*this, TEXT("Band chunks cover the bands"), Sum, BandArea, ExpTolerance);
	}

	// 넓힌 뒤에도 이웃 공간 계획은 변하지 않는다(이웃은 넓힌 공간의 안쪽 직사각형을 읽지 않는다).
	const FBathhouseLayoutValues Values = FixtureValues();
	const FSnapshots Snapshots = MakeSnapshots(Values);
	FSnapshots HallExpanded = Snapshots;
	HallExpanded[0] = FBathhouseSpaceLayout::WithExpansionCount(Snapshots[0], 1);
	TestFalse(TEXT("The hall shape changes when expanded"), RectNear(HallExpanded[0].Interior, Snapshots[0].Interior));
	TestTrue(TEXT("Expanded hall keeps the base interior"), RectNear(HallExpanded[0].BaseInterior, Snapshots[0].BaseInterior));
	for (const int32 Neighbor : { 1, 2 })
	{
		TestTrue(FString::Printf(TEXT("Neighbor %d plan is unchanged by the hall expansion"), Neighbor),
			PlansEqual(FBathhouseSpaceLayout::BuildPlan(Snapshots, Neighbor, Values),
				FBathhouseSpaceLayout::BuildPlan(HallExpanded, Neighbor, Values)));
	}
	// 새 형상은 옛 바깥 직사각형 밖 띠에만 더해진다(바닥 판 면적 증가 = 띠 면적 + 두께 띠).
	const FBox2D OldOuter = FBathhouseSpaceLayout::OuterRect(Snapshots[0], Values.WallThicknessCm);
	const FBox2D NewOuter = FBathhouseSpaceLayout::OuterRect(HallExpanded[0], Values.WallThicknessCm);
	TestTrue(TEXT("New outer rectangle contains the old one"), NewOuter.Min.X <= OldOuter.Min.X + ExpTolerance
		&& NewOuter.Min.Y <= OldOuter.Min.Y + ExpTolerance && NewOuter.Max.X >= OldOuter.Max.X - ExpTolerance
		&& NewOuter.Max.Y >= OldOuter.Max.Y - ExpTolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseExpansionValidationTest,
	"BathhouseSim.Expansion.Validation.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseExpansionValidationTest::RunTest(const FString& Parameters)
{
	const FBathhouseLayoutValues Values = FixtureValues();
	const FBathhouseValidationInputs Inputs = FixtureInputs();
	const FSnapshots Valid = MakeSnapshots(Values);
	const FBox Cover = MakeCover(Valid, Values, 50.0);
	const int32 HallRows = Valid[0].Steps.Num() + 1;

	FProblems Problems;
	FBathhouseSpaceValidation::ValidateExpansion(Valid, { Cover }, Inputs, HallRows, Problems);
	TestEqual(TEXT("A valid expansion list has no problems"), Problems.Num(), 0);

	// 미리보기·적용 횟수와 무관하다.
	FSnapshots AtEnd;
	for (const FBathhouseSpaceSnapshot& Snapshot : Valid)
	{
		AtEnd.Add(FBathhouseSpaceLayout::WithExpansionCount(Snapshot, Snapshot.Steps.Num()));
	}
	FProblems EndProblems;
	FBathhouseSpaceValidation::ValidateExpansion(AtEnd, { Cover }, Inputs, HallRows, EndProblems);
	TestEqual(TEXT("Counts on the snapshots do not change the expansion result"), EndProblems.Num(), Problems.Num());

	// EXP-045 홀 효과 표 길이: 홀 줄 수 + 1줄은 통과, 짧으면 오류(홀), 표 정보 없음은 생략.
	Problems.Reset();
	FBathhouseSpaceValidation::ValidateExpansion(Valid, { Cover }, Inputs, HallRows - 1, Problems);
	TestTrue(TEXT("A hall effect table shorter than hall steps + 1 is an error"),
		HasCode(Problems, EBathhouseProblemCode::ExpansionHallEffectShort, EBathhouseProblemSeverity::Error));
	const FBathhouseLayoutProblem* ShortProblem = Problems.FindByPredicate([](const FBathhouseLayoutProblem& P)
	{
		return P.Code == EBathhouseProblemCode::ExpansionHallEffectShort;
	});
	TestTrue(TEXT("The short effect table error belongs to the hall"), ShortProblem && ShortProblem->OwnerIndex == 0);
	Problems.Reset();
	FBathhouseSpaceValidation::ValidateExpansion(Valid, { Cover }, Inputs, INDEX_NONE, Problems);
	TestFalse(TEXT("No effect table information skips the length check"), HasCodeAny(Problems, EBathhouseProblemCode::ExpansionHallEffectShort));
	TestEqual(TEXT("A valid list still has no problems without the table"), Problems.Num(), 0);

	// EXP-040~048 줄 형식: 빈 벽, 같은 벽 중복, 가격, 항목별 양.
	{
		FSnapshots Bad = Valid;
		Bad[0].Steps[0].Sides.Reset();
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Bad, { Cover }, Inputs, INDEX_NONE, Problems);
		TestTrue(TEXT("A step without walls is an error"), HasCode(Problems, EBathhouseProblemCode::ExpansionSidesEmpty, EBathhouseProblemSeverity::Error));
		Bad = Valid;
		const FBathhouseExpansionSideSnapshot FirstWall = Bad[0].Steps[0].Sides[0];
		Bad[0].Steps[0].Sides.Add(FirstWall);
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Bad, { Cover }, Inputs, INDEX_NONE, Problems);
		int32 DuplicateCount = 0;
		for (const FBathhouseLayoutProblem& Problem : Problems)
		{
			DuplicateCount += Problem.Code == EBathhouseProblemCode::ExpansionSideDuplicate ? 1 : 0;
		}
		TestEqual(TEXT("The same wall twice in one step is one error"), DuplicateCount, 1);
		TestFalse(TEXT("The duplicate item adds no amount error"), HasCodeAny(Problems, EBathhouseProblemCode::ExpansionAmountInvalid));
		Bad = Valid;
		Bad[0].Steps[1].Price = 0;
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Bad, { Cover }, Inputs, INDEX_NONE, Problems);
		TestTrue(TEXT("A step price of zero is an error"), HasCode(Problems, EBathhouseProblemCode::ExpansionPriceInvalid, EBathhouseProblemSeverity::Error));
		const FBathhouseLayoutProblem* PriceProblem = Problems.FindByPredicate([](const FBathhouseLayoutProblem& P)
		{
			return P.Code == EBathhouseProblemCode::ExpansionPriceInvalid;
		});
		TestTrue(TEXT("The price error names the step index"), PriceProblem && PriceProblem->OwnerIndex == 0 && PriceProblem->ItemIndex == 1);
		Bad = Valid;
		Bad[1].Steps[0].Sides[1].AmountCm = -5.0;
		Bad[1].Steps[0].Sides[2].AmountCm = 0.0;
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Bad, { Cover }, Inputs, INDEX_NONE, Problems);
		int32 AmountCount = 0;
		for (const FBathhouseLayoutProblem& Problem : Problems)
		{
			AmountCount += Problem.Code == EBathhouseProblemCode::ExpansionAmountInvalid ? 1 : 0;
		}
		TestEqual(TEXT("Each wall item with a bad amount is its own error"), AmountCount, 2);
		// 같은 줄의 항목은 항목마다 맞닿음을 본다: 홀 줄에 남(통과)과 동(통로 변)을 함께 두면 동만 오류.
		Bad = Valid;
		Bad[0].Steps[0] = MakeMultiStep(FixtureStepPrice, 100.0, { EBathhouseSpaceSide::South, EBathhouseSpaceSide::East });
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Bad, { Cover }, Inputs, INDEX_NONE, Problems);
		int32 TouchCount = 0;
		for (const FBathhouseLayoutProblem& Problem : Problems)
		{
			TouchCount += Problem.Code == EBathhouseProblemCode::ExpansionTouchingSide ? 1 : 0;
		}
		TestEqual(TEXT("Only the touching wall item is a touching error"), TouchCount, 1);
		// 출입구 변 경고도 항목마다.
		Bad = Valid;
		Bad[0].Steps[0] = MakeMultiStep(FixtureStepPrice, 100.0, { EBathhouseSpaceSide::South, EBathhouseSpaceSide::West });
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Bad, { MakeCover(Bad, Values, 50.0) }, Inputs, INDEX_NONE, Problems);
		TestTrue(TEXT("The entrance wall item warns"), HasCode(Problems, EBathhouseProblemCode::ExpansionEntranceSide, EBathhouseProblemSeverity::Warning));
		TestFalse(TEXT("The other wall item does not add a touching error"), HasCodeAny(Problems, EBathhouseProblemCode::ExpansionTouchingSide));
	}

	// 잘못된 양.
	{
		FSnapshots Bad = Valid;
		Bad[0].Steps[0].Sides[0].AmountCm = 0.0;
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Bad, { Cover }, Inputs, INDEX_NONE, Problems);
		TestTrue(TEXT("A zero amount is an error"), HasCode(Problems, EBathhouseProblemCode::ExpansionAmountInvalid, EBathhouseProblemSeverity::Error));
		const FBathhouseLayoutProblem* AmountProblem = Problems.FindByPredicate([](const FBathhouseLayoutProblem& P)
		{
			return P.Code == EBathhouseProblemCode::ExpansionAmountInvalid;
		});
		TestTrue(TEXT("The amount error belongs to the hall"), AmountProblem && AmountProblem->OwnerIndex == 0);
	}
	// 맞닿은 벽(통로 벽 포함)은 오류, 반대 변은 허용.
	{
		FSnapshots Touch = Valid;
		Touch[0].Steps = { MakeStep(EBathhouseSpaceSide::East, 100.0) };
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Touch, { Cover }, Inputs, INDEX_NONE, Problems);
		TestTrue(TEXT("Expanding the wall shared with the bath (passage wall) is an error"),
			HasCode(Problems, EBathhouseProblemCode::ExpansionTouchingSide, EBathhouseProblemSeverity::Error));
		FSnapshots BathWest = Valid;
		BathWest[1].Steps = { MakeStep(EBathhouseSpaceSide::West, 100.0) };
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(BathWest, { Cover }, Inputs, INDEX_NONE, Problems);
		TestTrue(TEXT("The bath touching wall is an error from the other side too"),
			HasCode(Problems, EBathhouseProblemCode::ExpansionTouchingSide, EBathhouseProblemSeverity::Error));
		FSnapshots BathEast = Valid;
		BathEast[1].Steps = { MakeStep(EBathhouseSpaceSide::East, 100.0) };
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(BathEast, { MakeCover(BathEast, Values, 50.0) }, Inputs, INDEX_NONE, Problems);
		TestFalse(TEXT("The opposite side is allowed"), HasCodeAny(Problems, EBathhouseProblemCode::ExpansionTouchingSide));
	}
	// 출입구 변은 경고.
	{
		FSnapshots Entrance = Valid;
		Entrance[0].Steps = { MakeStep(EBathhouseSpaceSide::West, 100.0) };
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Entrance, { MakeCover(Entrance, Values, 50.0) }, Inputs, INDEX_NONE, Problems);
		TestTrue(TEXT("Expanding the entrance wall warns"), HasCode(Problems, EBathhouseProblemCode::ExpansionEntranceSide, EBathhouseProblemSeverity::Warning));
		TestFalse(TEXT("The entrance wall is not a touching error"), HasCodeAny(Problems, EBathhouseProblemCode::ExpansionTouchingSide));
	}
	// 떨어진 두 공간이 끝 모습에서 겹침.
	{
		FSnapshots Gap = MakeSnapshots(Values, 300.0);
		Gap[0].Steps = { MakeStep(EBathhouseSpaceSide::East, 500.0) };
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Gap, { MakeCover(Gap, Values, 50.0) }, Inputs, INDEX_NONE, Problems);
		TestFalse(TEXT("A gap means the wall is not touching"), HasCodeAny(Problems, EBathhouseProblemCode::ExpansionTouchingSide));
		TestTrue(TEXT("End shapes that overlap are an error"), HasCode(Problems, EBathhouseProblemCode::ExpansionOverlap, EBathhouseProblemSeverity::Error));
	}
	// 끝 모습이 바깥 출입구 앞을 막음.
	{
		FSnapshots Blocked = Valid;
		const double EdgeX = Valid[0].BaseInterior.Min.X - Values.WallThicknessCm;
		FBathhouseSpaceSnapshot Neighbor = MakeSnap(EBathhouseSpaceKind::Bath, TEXT("Neighbor"),
			FVector2D(EdgeX - 300.0 - Values.WallThicknessCm - 250.0, 0), FVector2D(500, 600), 50.0, 350.0, EBathhouseCleaningChunkKind::None);
		Neighbor.Steps = { MakeStep(EBathhouseSpaceSide::East, 400.0) };
		Blocked[1] = Neighbor;
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Blocked, { MakeCover(Blocked, Values, 50.0) }, Inputs, INDEX_NONE, Problems);
		TestTrue(TEXT("End shape in front of an outside entrance is an error"),
			HasCode(Problems, EBathhouseProblemCode::ExpansionOutsideOpeningBlocked, EBathhouseProblemSeverity::Error));
	}
	// 끝 모습의 Nav 범위.
	{
		const FBathhouseSpaceSnapshot HallBase = Valid[0];
		const FBox2D HallEnd = FBathhouseSpaceLayout::ExpandInterior(HallBase.BaseInterior, HallBase.Steps, HallBase.Steps.Num());
		TestTrue(TEXT("Fixture hall really grows"), RectArea(HallEnd) > RectArea(HallBase.BaseInterior));
		FBox BaseOnly = Cover;
		BaseOnly.Min.Y = HallBase.BaseInterior.Min.Y;
		BaseOnly.Max.Y = HallBase.BaseInterior.Max.Y;
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Valid, { BaseOnly }, Inputs, INDEX_NONE, Problems);
		TestTrue(TEXT("Nav range that misses the expanded hall is an error"),
			HasCode(Problems, EBathhouseProblemCode::ExpansionNavOutside, EBathhouseProblemSeverity::Error));
		FBox WithWork = Cover;
		WithWork.Min.Z = Valid[2].FloorZ - 10.0;
		Problems.Reset();
		FBathhouseSpaceValidation::ValidateExpansion(Valid, { WithWork }, Inputs, INDEX_NONE, Problems);
		TestTrue(TEXT("Nav range covering the expanded work floor is an error"),
			HasCode(Problems, EBathhouseProblemCode::ExpansionNavWorkCovered, EBathhouseProblemSeverity::Error));
	}

	// world 검증: 횟수와 무관한 결과, 효과 표 길이.
	FBathhouseExpansionTestWorld Fx;
	if (Fx.Create(*this, TEXT("ExpansionValidationWorld")))
	{
		FSnapshots Snapshots;
		FProblems AtZero;
		FBathhouseSpaceValidation::ValidateWorld(*Fx.World, Snapshots, AtZero);
		Access::SetApplied(*Fx.Hall, 2);
		Access::SetApplied(*Fx.Bath, 1);
		FProblems AtApplied;
		FBathhouseSpaceValidation::ValidateWorld(*Fx.World, Snapshots, AtApplied);
		Access::SetApplied(*Fx.Hall, 0);
		Access::SetApplied(*Fx.Bath, 0);
		TestEqual(TEXT("ValidateWorld result count does not depend on the applied count"), AtApplied.Num(), AtZero.Num());
		for (int32 Index = 0; Index < FMath::Min(AtApplied.Num(), AtZero.Num()); ++Index)
		{
			TestEqual(TEXT("ValidateWorld problem code does not depend on the applied count"), AtApplied[Index].Code, AtZero[Index].Code);
		}
		TestFalse(TEXT("Fixture effect table covers the hall steps"), HasCodeAny(AtZero, EBathhouseProblemCode::ExpansionHallEffectShort));
		Fx.Definition->Tiers.Pop();
		FProblems Short;
		FBathhouseSpaceValidation::ValidateWorld(*Fx.World, Snapshots, Short);
		TestTrue(TEXT("World validation reports a hall effect table shorter than hall steps + 1"),
			HasCode(Short, EBathhouseProblemCode::ExpansionHallEffectShort, EBathhouseProblemSeverity::Error));
	}
	Fx.Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseExpansionDataTest,
	"BathhouseSim.Expansion.Data.DefinitionAuthorityKeyRack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseExpansionDataTest::RunTest(const FString& Parameters)
{
	UBathhouseExpansionDefinition* Definition = FBathhouseExpansionTestWorld::MakeDefinition();
	FText Reason;
	TestTrue(TEXT("Fixture definition is valid"), Definition->ValidatePurchaseData(Reason));
	TestTrue(TEXT("Hall effect below zero clamps to the first row"), Definition->GetHallEffect(-5) == &Definition->Tiers[0]);
	TestTrue(TEXT("Hall effect past the table clamps to the last row"),
		Definition->GetHallEffect(Definition->Tiers.Num() + 10) == &Definition->Tiers.Last());
	TestEqual(TEXT("Hall effect index follows the count"), Definition->GetHallEffectIndex(1), 1);

	auto Mutated = [&](TFunctionRef<void(UBathhouseExpansionDefinition&)> Mutate)
	{
		UBathhouseExpansionDefinition* Copy = FBathhouseExpansionTestWorld::MakeDefinition();
		Mutate(*Copy);
		return Copy;
	};
	TestFalse(TEXT("Empty effect table is invalid"),
		Mutated([](UBathhouseExpansionDefinition& D) { D.Tiers.Reset(); })->ValidatePurchaseData(Reason));
	TestFalse(TEXT("Shrinking keys between rows is invalid"),
		Mutated([](UBathhouseExpansionDefinition& D) { D.Tiers[2].KeyPoolSize = D.Tiers[1].KeyPoolSize - 1; })->ValidatePurchaseData(Reason));
	TestFalse(TEXT("Keys below locker slots is invalid"),
		Mutated([](UBathhouseExpansionDefinition& D) { D.Tiers[0].KeyPoolSize = D.Tiers[0].MaxInstalledLockerSlots - 1; })->ValidatePurchaseData(Reason));
	TestTrue(TEXT("A longer effect table than the hall steps + 1 is allowed"),
		Mutated([](UBathhouseExpansionDefinition& D)
		{
			const FBathhouseExpansionTier LastTier = D.Tiers.Last();
			D.Tiers.Add(LastTier);
		})->ValidatePurchaseData(Reason));
	TestTrue(TEXT("A one-row effect table passes the definition rules (the space validation owns the length)"),
		Mutated([](UBathhouseExpansionDefinition& D) { D.Tiers.SetNum(1); })->ValidatePurchaseData(Reason));

#if WITH_EDITOR
	{
		FDataValidationContext Good;
		TestEqual(TEXT("Valid definition passes Data Validation"), Definition->IsDataValid(Good), EDataValidationResult::Valid);
		FDataValidationContext Bad;
		UBathhouseExpansionDefinition* Broken = Mutated([](UBathhouseExpansionDefinition& D)
		{
			D.Tiers[2].KeyPoolSize = D.Tiers[1].KeyPoolSize - 1;
			D.Tiers[0].KeyPoolSize = D.Tiers[0].MaxInstalledLockerSlots - 1;
		});
		TestEqual(TEXT("Broken definition fails Data Validation"), Broken->IsDataValid(Bad), EDataValidationResult::Invalid);
		TestTrue(TEXT("Each problem has its own error"), Bad.GetNumErrors() >= 2);
		UBathhouseExpansionDefinition* Valid = FBathhouseExpansionTestWorld::MakeDefinition();
		ABathhouseExpansionAuthority* Authority = NewObject<ABathhouseExpansionAuthority>();
		FDataValidationContext NoDefinition;
		TestEqual(TEXT("Authority without a definition is invalid"), Authority->IsDataValid(NoDefinition), EDataValidationResult::Invalid);
		Access::ConfigureAuthority(*Authority, Valid, 1);
		FDataValidationContext NonZero;
		TestEqual(TEXT("Authority initial tier other than 0 is invalid"), Authority->IsDataValid(NonZero), EDataValidationResult::Invalid);
		Access::ConfigureAuthority(*Authority, Valid, 0);
		FDataValidationContext Ok;
		TestEqual(TEXT("Authority with a definition and tier 0 is valid"), Authority->IsDataValid(Ok), EDataValidationResult::Valid);
	}

	FBathhouseExpansionTestWorld Fx;
	if (Fx.Create(*this, TEXT("ExpansionKeyRackWorld")))
	{
		int32 NeededKeys = 0;
		for (const FBathhouseExpansionTier& Tier : Fx.Definition->Tiers)
		{
			NeededKeys = FMath::Max(NeededKeys, Tier.KeyPoolSize);
		}
		FDataValidationContext RackOk;
		TestEqual(TEXT("A rack with enough pairs is valid"), Fx.Rack->IsDataValid(RackOk), EDataValidationResult::Valid);
		ABathhouseKeyRackActor* Small = Fx.World->SpawnActorDeferred<ABathhouseKeyRackActor>(
			ABathhouseKeyRackActor::StaticClass(), FTransform(FVector(0.0f, 7000.0f, 0.0f)));
		Access::AddRackPairs(*Small, NeededKeys - 1);
		Small->FinishSpawning(FTransform(FVector(0.0f, 7000.0f, 0.0f)));
		if (!Small->HasActorBegunPlay())
		{
			Small->DispatchBeginPlay();
		}
		FDataValidationContext SmallBad;
		TestEqual(TEXT("A rack with fewer pairs than the reachable key count is invalid"),
			Small->IsDataValid(SmallBad), EDataValidationResult::Invalid);
		// 열쇠걸이는 홀 넓힘 줄 수와 무관하게 효과 표 모든 줄의 열쇠 수를 본다(표 끝 쪽 줄이 더 커도 포함).
		ABathhouseKeyRackActor* Exact = Fx.World->SpawnActorDeferred<ABathhouseKeyRackActor>(
			ABathhouseKeyRackActor::StaticClass(), FTransform(FVector(0.0f, 8000.0f, 0.0f)));
		Access::AddRackPairs(*Exact, NeededKeys);
		Exact->FinishSpawning(FTransform(FVector(0.0f, 8000.0f, 0.0f)));
		if (!Exact->HasActorBegunPlay())
		{
			Exact->DispatchBeginPlay();
		}
		FDataValidationContext ExactOk;
		TestEqual(TEXT("A rack with exactly the largest key count of all rows is valid"), Exact->IsDataValid(ExactOk), EDataValidationResult::Valid);
		Fx.Definition->Tiers.Add({ NeededKeys + 2, NeededKeys + 2 });
		FDataValidationContext ExactBad;
		TestEqual(TEXT("A larger key count in the last row (beyond the hall steps) makes the rack invalid"),
			Exact->IsDataValid(ExactBad), EDataValidationResult::Invalid);
	}
	Fx.Destroy();
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseExpansionSpaceRuntimeTest,
	"BathhouseSim.Expansion.World.SpaceRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseExpansionSpaceRuntimeTest::RunTest(const FString& Parameters)
{
	FBathhouseExpansionTestWorld Fx;
	if (!Fx.Create(*this, TEXT("ExpansionRuntimeWorld")))
	{
		Fx.Destroy();
		return false;
	}
	const UStaticMesh* Mesh = GetDefault<UBathhouseBuildingSettings>()->LoadShellBoxMesh();
	const FBathhouseLayoutValues Values = FixtureValues();
	ABathhouseSpaceActor* Hall = Fx.Hall;
	ABathhouseSpaceActor* Work = Fx.Work;

	TestEqual(TEXT("Game world starts at zero expansions"), Hall->GetAppliedExpansionCount(), 0);
	TestEqual(TEXT("Game world effective count is the applied count"), Hall->GetEffectiveExpansionCount(), 0);
	TestEqual(TEXT("Step count comes from the authored list"), Hall->GetExpansionStepCount(), 2);
	TestEqual(TEXT("Hall next price is the first step price"), Hall->GetNextExpansionPrice(), Access::Steps(*Hall)[0].Price);
	TestFalse(TEXT("A fresh hall is not at its limit"), Hall->IsAtExpansionLimit());
	TestTrue(TEXT("Spaces are registered with the purchase subsystem"), Fx.Purchase->BuildView(Fx.Player).Options[0].bPresent);
	const FBox2D Base = Hall->GetBaseInteriorRect();
	TestTrue(TEXT("Interior equals the base at zero"), RectNear(Hall->GetInteriorRect(), Base));

	const TArray<TWeakObjectPtr<AActor>> ChunksBefore = Hall->GetCleaningChunks();
	const int32 ChunkCountBefore = ChunksBefore.Num();
	TestTrue(TEXT("Hall has generated chunks"), ChunkCountBefore > 0);
	const UInstancedStaticMeshComponent* BathWallBefore = Fx.Bath->GetShell()->FindPartComponent(EBathhouseShellPart::Wall);
	const UInstancedStaticMeshComponent* WorkFloorBefore = Work->GetShell()->FindPartComponent(EBathhouseShellPart::Floor);
	const int32 FloorCountBefore = Hall->GetShell()->GetPartInstanceCount(EBathhouseShellPart::Floor);
	const int32 WallCountBefore = Hall->GetShell()->GetPartInstanceCount(EBathhouseShellPart::Wall);
	const FVector ZoneExtentBefore = Hall->GetZoneBounds()->GetUnscaledBoxExtent();
	const FVector ZoneLocationBefore = Hall->GetZoneBounds()->GetRelativeLocation();

	FBathhouseSpaceExpansionUndo Undo;
	FText Reason;
	TestTrue(TEXT("A hall expansion can be applied"), Hall->CanApplyNextExpansion(Reason));
	TestTrue(TEXT("Applying the next expansion succeeds"), Hall->ApplyNextExpansion(Undo, Reason));
	TestTrue(TEXT("The undo record is set"), Undo.IsSet());
	TestEqual(TEXT("The applied count rises"), Hall->GetAppliedExpansionCount(), 1);
	TestEqual(TEXT("The effective count follows the applied count"), Hall->GetEffectiveExpansionCount(), 1);
	const FBox2D Expanded = Hall->GetInteriorRect();
	TestTrue(TEXT("The interior matches the one-step rectangle"), RectNear(Expanded, Hall->GetInteriorRectForCount(1)));
	const FBathhouseSpaceExpansionStep& FirstStep = Access::Steps(*Hall)[0];
	TestEqual(TEXT("The fixture hall step has two walls (D3)"), FirstStep.Sides.Num(), 2);
	double SouthAmount = 0.0;
	double NorthAmount = 0.0;
	for (const FBathhouseSpaceExpansionSide& Wall : FirstStep.Sides)
	{
		(Wall.Side == EBathhouseSpaceSide::South ? SouthAmount : NorthAmount) = Wall.AmountCm;
	}
	ExpectNear(*this, TEXT("The south edge moves by its amount in one application"), Expanded.Min.Y, Base.Min.Y - SouthAmount, ExpTolerance);
	ExpectNear(*this, TEXT("The north edge moves by its amount in the same application"), Expanded.Max.Y, Base.Max.Y + NorthAmount, ExpTolerance);
	TestTrue(TEXT("The east and west edges stay"), FMath::IsNearlyEqual(Expanded.Min.X, Base.Min.X, ExpTolerance) && FMath::IsNearlyEqual(Expanded.Max.X, Base.Max.X, ExpTolerance));
	TestEqual(TEXT("The hall next price is now the second step price"), Hall->GetNextExpansionPrice(), Access::Steps(*Hall)[1].Price);
	const UBoxComponent* Zone = Hall->GetZoneBounds();
	ExpectNear(*this, TEXT("Zone Y extent follows the expanded interior"), Zone->GetUnscaledBoxExtent().Y, (Expanded.Max.Y - Expanded.Min.Y) * 0.5, ExpTolerance);
	ExpectNear(*this, TEXT("Zone X extent is unchanged"), Zone->GetUnscaledBoxExtent().X, ZoneExtentBefore.X, ExpTolerance);
	const FVector2D ZoneCenter(Zone->GetComponentLocation().X, Zone->GetComponentLocation().Y);
	TestTrue(TEXT("Zone center follows the expanded interior"), ZoneCenter.Equals(Expanded.GetCenter(), ExpTolerance));
	// 양쪽 벽이 다른 양으로 물러나면 중심이 옮겨 상대 위치가 바뀐다. 같은 양이면 중심이 그대로다(기대는 fixture에서 계산).
	const bool bCenterMoved = !Expanded.GetCenter().Equals(Base.GetCenter(), ExpTolerance);
	TestEqual(TEXT("Zone relative location changes exactly when the interior center moves"),
		!Zone->GetRelativeLocation().Equals(ZoneLocationBefore, ExpTolerance), bCenterMoved);

	// shell이 넓힌 바깥 직사각형을 덮는다.
	FSnapshots Snapshots;
	FBathhouseSpaceValidation::GatherSnapshots(*Fx.World, Snapshots);
	TArray<FBathhouseSpacePlan> Plans;
	FBathhouseSpaceLayout::Build(Snapshots, Values, Plans);
	const FBox2D Outer = FBathhouseSpaceLayout::OuterRect(Snapshots[0], Values.WallThicknessCm);
	const FBox2D FloorBounds = PartBounds(Hall->GetShell()->FindPartComponent(EBathhouseShellPart::Floor), Mesh);
	TestTrue(TEXT("Floor parts cover the expanded outer rectangle"), RectNear(FloorBounds, Outer));
	const FBox2D CeilingBounds = PartBounds(Hall->GetShell()->FindPartComponent(EBathhouseShellPart::Ceiling), Mesh);
	TestTrue(TEXT("Ceiling parts cover the expanded outer rectangle"), RectNear(CeilingBounds, Outer));
	for (const EBathhouseShellPart Part : { EBathhouseShellPart::Floor, EBathhouseShellPart::Wall, EBathhouseShellPart::Ceiling })
	{
		TestEqual(TEXT("Expanded shell instance count matches the plan"),
			Hall->GetShell()->GetPartInstanceCount(Part), Plans[0].Get(Part).Num());
	}
	TestEqual(TEXT("Expanded hall light count matches the plan"), Hall->GetShell()->GetLightCount(), Plans[0].LightLocations.Num());
	TestTrue(TEXT("Floor part count did not shrink"), Hall->GetShell()->GetPartInstanceCount(EBathhouseShellPart::Floor) >= FloorCountBefore);

	// 이웃 shell은 다시 만들지 않고 기존 조각은 그대로다. 띠 조각이 더해진다.
	TestTrue(TEXT("Neighbor wall component identity is kept"), Fx.Bath->GetShell()->FindPartComponent(EBathhouseShellPart::Wall) == BathWallBefore);
	TestTrue(TEXT("Work floor component identity is kept"), Work->GetShell()->FindPartComponent(EBathhouseShellPart::Floor) == WorkFloorBefore);
	TArray<FBathhouseExpansionStepSnapshot> StepSnapshots;
	for (const FBathhouseSpaceExpansionStep& Step : Access::Steps(*Hall))
	{
		FBathhouseExpansionStepSnapshot& Item = StepSnapshots.AddDefaulted_GetRef();
		Item.Price = Step.Price;
		for (const FBathhouseSpaceExpansionSide& Wall : Step.Sides)
		{
			Item.Sides.Add({ Wall.Side, Wall.AmountCm });
		}
	}
	TArray<FBox2D> BandRects;
	FBathhouseSpaceLayout::ExpansionBandRects(Base, StepSnapshots, 0, BandRects);
	TArray<FBox2D> BandChunks;
	for (const FBox2D& BandRect : BandRects)
	{
		TArray<FBox2D> Part;
		FBathhouseSpaceLayout::SplitChunks(BandRect, Values.ChunkMaxSizeCm, Part);
		BandChunks.Append(Part);
	}
	TestEqual(TEXT("The band adds its split chunks"), Hall->GetCleaningChunks().Num(), ChunkCountBefore + BandChunks.Num());
	for (int32 Index = 0; Index < ChunksBefore.Num(); ++Index)
	{
		TestTrue(TEXT("Existing chunk identity is kept"), Hall->GetCleaningChunks()[Index] == ChunksBefore[Index] && ChunksBefore[Index].IsValid());
	}
	// 되돌림.
	Hall->UndoExpansion(Undo);
	TestEqual(TEXT("Undo restores the applied count"), Hall->GetAppliedExpansionCount(), 0);
	TestTrue(TEXT("Undo restores the interior"), RectNear(Hall->GetInteriorRect(), Base));
	ExpectNear(*this, TEXT("Undo restores the zone extent"), Hall->GetZoneBounds()->GetUnscaledBoxExtent().Y, ZoneExtentBefore.Y, ExpTolerance);
	TestEqual(TEXT("Undo restores the chunk count"), Hall->GetCleaningChunks().Num(), ChunkCountBefore);
	TestEqual(TEXT("Undo restores the floor part count"), Hall->GetShell()->GetPartInstanceCount(EBathhouseShellPart::Floor), FloorCountBefore);
	TestEqual(TEXT("Undo restores the wall part count"), Hall->GetShell()->GetPartInstanceCount(EBathhouseShellPart::Wall), WallCountBefore);
	Hall->UndoExpansion(FBathhouseSpaceExpansionUndo());
	TestEqual(TEXT("An empty undo is a no-op"), Hall->GetAppliedExpansionCount(), 0);

	// 띠 조각이 없는 공간(조각 종류 없음).
	FBathhouseSpaceExpansionUndo WorkUndo;
	TestEqual(TEXT("Work has no chunks"), Work->GetCleaningChunks().Num(), 0);
	TestTrue(TEXT("Work expansion applies (east and north together)"), Work->ApplyNextExpansion(WorkUndo, Reason));
	TestEqual(TEXT("Work still has no chunks"), Work->GetCleaningChunks().Num(), 0);
	Work->UndoExpansion(WorkUndo);

	// 상한.
	FBathhouseSpaceExpansionUndo A, B, C;
	TestTrue(TEXT("First hall step"), Hall->ApplyNextExpansion(A, Reason));
	TestTrue(TEXT("Second hall step"), Hall->ApplyNextExpansion(B, Reason));
	TestFalse(TEXT("The per-space cap blocks the third step"), Hall->CanApplyNextExpansion(Reason));
	TestFalse(TEXT("The reason text is set"), Reason.IsEmpty());
	TestTrue(TEXT("At the limit"), Hall->IsAtExpansionLimit());
	TestEqual(TEXT("No next price at the limit"), Hall->GetNextExpansionPrice(), 0);
	TestFalse(TEXT("Applying past the cap fails"), Hall->ApplyNextExpansion(C, Reason));
	TestFalse(TEXT("A failed apply leaves no undo record"), C.IsSet());
	TestEqual(TEXT("A failed apply leaves the count"), Hall->GetAppliedExpansionCount(), 2);
	Fx.Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseExpansionPurchaseTest,
	"BathhouseSim.Expansion.Purchase.Transaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseExpansionPurchaseTest::RunTest(const FString& Parameters)
{
	FBathhouseExpansionTestWorld Fx;
	if (!Fx.Create(*this, TEXT("ExpansionPurchaseWorld")))
	{
		Fx.Destroy();
		return false;
	}
	UPlayerWalletComponent* Wallet = Fx.Wallet();
	UBathhouseExpansionPurchaseSubsystem* Purchase = Fx.Purchase;
	UBathhouseFacilitySubsystem* Facilities = Fx.World->GetSubsystem<UBathhouseFacilitySubsystem>();
	UBathhouseExpansionMoneyProbe* Probe = NewObject<UBathhouseExpansionMoneyProbe>();
	Wallet->OnMoneyChanged.AddDynamic(Probe, &UBathhouseExpansionMoneyProbe::HandleMoneyChanged);
	int32 ChangedCount = 0;
	const FDelegateHandle Handle = Purchase->OnExpansionChanged.AddLambda([&ChangedCount]() { ++ChangedCount; });
	const UBathhouseExpansionDefinition* Definition = Fx.Definition;

	// 가격 합이 크므로 잔액을 넉넉히 둔다. 기대값은 공간 줄 가격에서 읽는다.
	Wallet->TryAddMoney(100000);
	auto StepPrice = [&Fx](const EBathhouseSpaceKind Kind, const int32 Index)
	{
		return Access::Steps(*Fx.Space(Kind))[Index].Price;
	};

	// EXP-021 view.
	FBathhouseExpansionView View = Purchase->BuildView(Fx.Player);
	TestTrue(TEXT("View is available"), View.bAvailable);
	TestFalse(TEXT("View is not busy"), View.bBusy);
	TestFalse(TEXT("Not every space is at its limit"), View.bAllAtLimit);
	TestEqual(TEXT("Balance is the wallet money"), View.Balance, Wallet->GetCurrentMoney());
	TestEqual(TEXT("Locker limit is the tier-0 limit"), View.LockerSlotLimit, Definition->Tiers[0].MaxInstalledLockerSlots);
	TestEqual(TEXT("No installed locker slots in the fixture"), View.InstalledLockerSlots, 0);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const EBathhouseSpaceKind Kind = static_cast<EBathhouseSpaceKind>(Index);
		const FBathhouseExpansionOptionView& Option = View.Options[Index];
		TestTrue(TEXT("Each option has a space and can expand"), Option.bPresent && Option.bCanExpand);
		TestEqual(TEXT("Each option starts at zero applied"), Option.AppliedCount, 0);
		TestEqual(TEXT("The step count is the number of rows of that space"), Option.StepCount, Fx.Space(Kind)->GetExpansionStepCount());
		TestFalse(TEXT("No option is at its limit"), Option.bAtLimit);
		TestEqual(TEXT("Each option shows the first step price of its own space"), Option.NextPrice, StepPrice(Kind, 0));
		TestTrue(TEXT("The next size is larger in one axis"), Option.NextSizeCm.X * Option.NextSizeCm.Y > Option.CurrentSizeCm.X * Option.CurrentSizeCm.Y);
	}
	TestTrue(TEXT("Prices differ between spaces in the fixture"),
		StepPrice(EBathhouseSpaceKind::Hall, 0) != StepPrice(EBathhouseSpaceKind::Bath, 0));
	// 여러 벽 결과: 다음 크기는 두 방향이 함께 반영된 크기.
	{
		const FBox2D HallNext = Fx.Hall->GetInteriorRectForCount(1);
		TestTrue(TEXT("The hall next size is the multi-wall result"),
			View.Options[0].NextSizeCm.Equals(FVector2D(HallNext.Max.X - HallNext.Min.X, HallNext.Max.Y - HallNext.Min.Y), ExpTolerance));
	}
	TestTrue(TEXT("Only the hall has an effect"), View.Options[0].bHasHallEffect && !View.Options[1].bHasHallEffect && !View.Options[2].bHasHallEffect);
	TestEqual(TEXT("Hall keys now"), View.Options[0].KeysNow, Definition->Tiers[0].KeyPoolSize);
	TestEqual(TEXT("Hall keys next"), View.Options[0].KeysNext, Definition->Tiers[1].KeyPoolSize);
	TestEqual(TEXT("Hall locker limit next"), View.Options[0].LockerLimitNext, Definition->Tiers[1].MaxInstalledLockerSlots);
	TestEqual(TEXT("Rack starts with the tier-0 keys"), Fx.Rack->GetMaterializedPairCount(), Definition->Tiers[0].KeyPoolSize);

	// EXP-023 hall purchase: 홀 1번째 가격 1회, 여러 벽 한 번에, 홀 다음 가격 표시.
	ChangedCount = 0;
	const int32 HallFirst = StepPrice(EBathhouseSpaceKind::Hall, 0);
	const int32 MoneyBefore = Wallet->GetCurrentMoney();
	Probe->ChangeCount = 0;
	int32 EvaluatedPrice = 0;
	TestEqual(TEXT("Evaluate agrees before the purchase"), Purchase->EvaluatePurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0, &EvaluatedPrice), EBathhouseExpansionFailure::None);
	TestEqual(TEXT("Evaluate returns the hall first price"), EvaluatedPrice, HallFirst);
	TestEqual(TEXT("Hall purchase succeeds"), Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0), EBathhouseExpansionFailure::None);
	TestEqual(TEXT("Money drops by the hall first price exactly once"), Wallet->GetCurrentMoney(), MoneyBefore - HallFirst);
	TestEqual(TEXT("The HUD money change fires once"), Probe->ChangeCount, 1);
	TestEqual(TEXT("Only the hall expanded"), Fx.Hall->GetAppliedExpansionCount() * 100 + Fx.Bath->GetAppliedExpansionCount() * 10 + Fx.Work->GetAppliedExpansionCount(), 100);
	TestEqual(TEXT("Authority advanced one effect row for the multi-wall step"), Fx.Authority->GetCurrentTierIndex(), 1);
	TestEqual(TEXT("Rack materialized the new key count"), Fx.Rack->GetMaterializedPairCount(), Definition->Tiers[1].KeyPoolSize);
	TestEqual(TEXT("Locker limit is the new limit"), Facilities->GetMaxInstalledLockerSlots(), Definition->Tiers[1].MaxInstalledLockerSlots);
	TestEqual(TEXT("The expansion change broadcasts once"), ChangedCount, 1);
	View = Purchase->BuildView(Fx.Player);
	TestEqual(TEXT("The hall option shows 1 applied"), View.Options[0].AppliedCount, 1);
	TestEqual(TEXT("The hall option now shows the second step price"), View.Options[0].NextPrice, StepPrice(EBathhouseSpaceKind::Hall, 1));
	TestEqual(TEXT("The bath option still shows its first step price"), View.Options[1].NextPrice, StepPrice(EBathhouseSpaceKind::Bath, 0));
	TestEqual(TEXT("The work option still shows its first step price"), View.Options[2].NextPrice, StepPrice(EBathhouseSpaceKind::Work, 0));

	// EXP-027 double click: the second request with the same expected count is stale.
	TestEqual(TEXT("A second request from the same confirmation is stale"),
		Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0), EBathhouseExpansionFailure::StaleState);
	TestEqual(TEXT("The stale request charges nothing"), Wallet->GetCurrentMoney(), MoneyBefore - HallFirst);
	TestEqual(TEXT("The stale request broadcasts nothing"), ChangedCount, 1);

	// EXP-042: 다른 공간을 사고 나서도 홀 2번째는 홀 2번째 가격, 열쇠는 다음 효과 줄.
	const int32 KeysAfterHall = Fx.Rack->GetMaterializedPairCount();
	TestEqual(TEXT("Bath purchase succeeds with the bath applied count"), Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Bath, 0), EBathhouseExpansionFailure::None);
	TestEqual(TEXT("Bath purchase charges the bath first price (not a global order)"),
		Wallet->GetCurrentMoney(), MoneyBefore - HallFirst - StepPrice(EBathhouseSpaceKind::Bath, 0));
	TestEqual(TEXT("Bath purchase leaves the tier"), Fx.Authority->GetCurrentTierIndex(), 1);
	TestEqual(TEXT("Bath purchase leaves the keys"), Fx.Rack->GetMaterializedPairCount(), KeysAfterHall);
	TestEqual(TEXT("Bath expanded once"), Fx.Bath->GetAppliedExpansionCount(), 1);
	View = Purchase->BuildView(Fx.Player);
	TestEqual(TEXT("The hall price is unchanged by the bath purchase"), View.Options[0].NextPrice, StepPrice(EBathhouseSpaceKind::Hall, 1));
	TestEqual(TEXT("A hall expected count of 0 is stale after the hall expanded (counts are per space)"),
		Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0), EBathhouseExpansionFailure::StaleState);
	TestEqual(TEXT("Hall second step uses the hall second price and the next effect row"),
		Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 1), EBathhouseExpansionFailure::None);
	TestEqual(TEXT("Hall second step charged the hall second price"), Wallet->GetCurrentMoney(),
		MoneyBefore - HallFirst - StepPrice(EBathhouseSpaceKind::Bath, 0) - StepPrice(EBathhouseSpaceKind::Hall, 1));
	TestEqual(TEXT("Hall keys follow the third row"), Fx.Rack->GetMaterializedPairCount(), Definition->Tiers[2].KeyPoolSize);

	// EXP-043·044: 홀만 상한이면 홀만 막히고 다른 공간은 구입 가능, 모두 상한일 때만 최대.
	TestEqual(TEXT("The hall at its limit is rejected as space max"),
		Purchase->EvaluatePurchase(Fx.Player, EBathhouseSpaceKind::Hall, 2), EBathhouseExpansionFailure::SpaceMaxReached);
	View = Purchase->BuildView(Fx.Player);
	TestTrue(TEXT("The hall option is at its limit"), View.Options[0].bAtLimit && !View.Options[0].bCanExpand);
	TestEqual(TEXT("The capped option shows no price"), View.Options[0].NextPrice, 0);
	TestTrue(TEXT("The work option can still expand"), View.Options[2].bCanExpand);
	TestFalse(TEXT("Not all spaces are at the limit"), View.bAllAtLimit);
	TestEqual(TEXT("Work can still be bought at its first price"), Purchase->EvaluatePurchase(Fx.Player, EBathhouseSpaceKind::Work, 0), EBathhouseExpansionFailure::None);
	TestEqual(TEXT("Work first"), Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Work, 0), EBathhouseExpansionFailure::None);
	TestEqual(TEXT("Work second"), Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Work, 1), EBathhouseExpansionFailure::None);
	TestEqual(TEXT("Bath second"), Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Bath, 1), EBathhouseExpansionFailure::None);
	View = Purchase->BuildView(Fx.Player);
	TestTrue(TEXT("All spaces at their limits report the maximum"), View.bAllAtLimit);
	TestEqual(TEXT("A purchase at the limit is space max"), Purchase->EvaluatePurchase(Fx.Player, EBathhouseSpaceKind::Bath, 2), EBathhouseExpansionFailure::SpaceMaxReached);

	Purchase->OnExpansionChanged.Remove(Handle);
	Fx.Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseExpansionPurchaseFailureTest,
	"BathhouseSim.Expansion.Purchase.FailuresAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseExpansionPurchaseFailureTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		int32 Money = 0;
		int32 Hall = 0;
		int32 Bath = 0;
		int32 Tier = 0;
		int32 Keys = 0;
		int32 Chunks = 0;
		int32 Floor = 0;
		double HallHeight = 0.0;
		int32 Limit = 0;
	};
	auto Capture = [](const FBathhouseExpansionTestWorld& Fx)
	{
		FState State;
		State.Money = Fx.Wallet()->GetCurrentMoney();
		State.Hall = Fx.Hall->GetAppliedExpansionCount();
		State.Bath = Fx.Bath->GetAppliedExpansionCount();
		State.Tier = Fx.Authority->GetCurrentTierIndex();
		State.Keys = Fx.Rack->GetMaterializedPairCount();
		State.Chunks = Fx.Hall->GetCleaningChunks().Num();
		State.Floor = Fx.Hall->GetShell()->GetPartInstanceCount(EBathhouseShellPart::Floor);
		State.HallHeight = Fx.Hall->GetInteriorRect().Max.Y - Fx.Hall->GetInteriorRect().Min.Y;
		State.Limit = Fx.World->GetSubsystem<UBathhouseFacilitySubsystem>()->GetMaxInstalledLockerSlots();
		return State;
	};
	auto ExpectSame = [this](const TCHAR* Label, const FState& A, const FState& B)
	{
		TestEqual(FString::Printf(TEXT("%s: money"), Label), A.Money, B.Money);
		TestEqual(FString::Printf(TEXT("%s: hall count"), Label), A.Hall, B.Hall);
		TestEqual(FString::Printf(TEXT("%s: bath count"), Label), A.Bath, B.Bath);
		TestEqual(FString::Printf(TEXT("%s: tier"), Label), A.Tier, B.Tier);
		TestEqual(FString::Printf(TEXT("%s: keys"), Label), A.Keys, B.Keys);
		TestEqual(FString::Printf(TEXT("%s: chunks"), Label), A.Chunks, B.Chunks);
		TestEqual(FString::Printf(TEXT("%s: floor parts"), Label), A.Floor, B.Floor);
		TestEqual(FString::Printf(TEXT("%s: hall size"), Label), A.HallHeight, B.HallHeight);
		TestEqual(FString::Printf(TEXT("%s: locker limit"), Label), A.Limit, B.Limit);
	};

	// EXP-022 잔액 부족, StaleState, 공간별 상한.
	{
		FBathhouseExpansionTestWorld Fx;
		if (!Fx.Create(*this, TEXT("ExpansionFailureWorld")))
		{
			Fx.Destroy();
			return false;
		}
		const int32 Price = FBathhouseExpansionAutomationAccess::Steps(*Fx.Hall)[0].Price;
		UPlayerWalletComponent* Wallet = Fx.Wallet();
		Wallet->TrySpendMoney(Wallet->GetCurrentMoney() - (Price - 1));
		const FState Before = Capture(Fx);
		TestEqual(TEXT("Insufficient money is reported"), Fx.Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0), EBathhouseExpansionFailure::InsufficientMoney);
		ExpectSame(TEXT("Insufficient money"), Before, Capture(Fx));
		FBathhouseExpansionView View = Fx.Purchase->BuildView(Fx.Player);
		TestEqual(TEXT("The view balance is one below the hall first price"), View.Options[0].NextPrice - View.Balance, 1);
		Wallet->TryAddMoney(1);
		View = Fx.Purchase->BuildView(Fx.Player);
		TestEqual(TEXT("Money arriving later covers the hall first price"), View.Balance, View.Options[0].NextPrice);
		TestEqual(TEXT("Evaluate agrees once the money arrives"), Fx.Purchase->EvaluatePurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0), EBathhouseExpansionFailure::None);
		TestEqual(TEXT("A stale expected count is rejected"), Fx.Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 5), EBathhouseExpansionFailure::StaleState);
		// EXP-022: 잔액 부족은 고른 공간의 가격 기준이다(홀 가격에는 부족하지만 더 싼 공간에는 충분할 수 있다).
		{
			const int32 HallPrice = FBathhouseExpansionAutomationAccess::Steps(*Fx.Hall)[0].Price;
			const int32 WorkPrice = FBathhouseExpansionAutomationAccess::Steps(*Fx.Work)[0].Price;
			const int32 Cheaper = FMath::Min(HallPrice, WorkPrice);
			const EBathhouseSpaceKind CheaperKind = Cheaper == HallPrice ? EBathhouseSpaceKind::Hall : EBathhouseSpaceKind::Work;
			const EBathhouseSpaceKind OtherKind = CheaperKind == EBathhouseSpaceKind::Hall ? EBathhouseSpaceKind::Work : EBathhouseSpaceKind::Hall;
			if (HallPrice != WorkPrice)
			{
				Wallet->TrySpendMoney(Wallet->GetCurrentMoney() - Cheaper);
				TestEqual(TEXT("The pricier space is rejected for the balance"), Fx.Purchase->EvaluatePurchase(Fx.Player, OtherKind, 0), EBathhouseExpansionFailure::InsufficientMoney);
				TestEqual(TEXT("The cheaper space is affordable with the same balance"), Fx.Purchase->EvaluatePurchase(Fx.Player, CheaperKind, 0), EBathhouseExpansionFailure::None);
			}
		}
		Wallet->TryAddMoney(100000);
		TestEqual(TEXT("First hall step"), Fx.Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0), EBathhouseExpansionFailure::None);
		TestEqual(TEXT("Second hall step"), Fx.Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 1), EBathhouseExpansionFailure::None);
		const FState AtSpaceCap = Capture(Fx);
		TestEqual(TEXT("The per-space cap rejects a third hall step"), Fx.Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 2), EBathhouseExpansionFailure::SpaceMaxReached);
		ExpectSame(TEXT("Space cap"), AtSpaceCap, Capture(Fx));
		View = Fx.Purchase->BuildView(Fx.Player);
		TestFalse(TEXT("The view marks the hall option as capped"), View.Options[0].bCanExpand);
		Fx.Destroy();
	}

	// EXP-027 재진입은 Busy, 결제 callback 중 view는 busy.
	{
		FBathhouseExpansionTestWorld Fx;
		if (!Fx.Create(*this, TEXT("ExpansionReentryWorld")))
		{
			Fx.Destroy();
			return false;
		}
		UBathhouseExpansionMoneyProbe* Probe = NewObject<UBathhouseExpansionMoneyProbe>();
		Probe->bReenter = true;
		Probe->Subsystem = Fx.Purchase;
		Probe->Buyer = Fx.Player;
		Fx.Wallet()->OnMoneyChanged.AddDynamic(Probe, &UBathhouseExpansionMoneyProbe::HandleMoneyChanged);
		TestEqual(TEXT("Purchase with a re-entering callback succeeds"), Fx.Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0), EBathhouseExpansionFailure::None);
		TestTrue(TEXT("The callback ran"), Probe->bReentered);
		TestTrue(TEXT("The view is busy inside the callback"), Probe->ViewBusyInCallback);
		TestEqual(TEXT("A re-entering purchase is busy"), Probe->ReentryResult, EBathhouseExpansionFailure::Busy);
		TestEqual(TEXT("Only one money change"), Probe->ChangeCount, 1);
		TestEqual(TEXT("The bath did not expand"), Fx.Bath->GetAppliedExpansionCount(), 0);
		Fx.Destroy();
	}

	// 강제 실패 주입은 모두 원상이다.
	for (const int32 Step : { 4, 5, 6 })
	{
		FBathhouseExpansionTestWorld Fx;
		if (!Fx.Create(*this, TEXT("ExpansionInjectionWorld")))
		{
			Fx.Destroy();
			return false;
		}
		AddExpectedError(TEXT("확장 효과 줄 상승 실패"), EAutomationExpectedErrorFlags::Contains, 0);
		const FState Before = Capture(Fx);
		FBathhouseExpansionAutomationAccess::InjectFailure(*Fx.Purchase, Step);
		const EBathhouseExpansionFailure Result = Fx.Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0);
		TestTrue(FString::Printf(TEXT("Injected failure at step %d is reported"), Step), Result != EBathhouseExpansionFailure::None);
		ExpectSame(*FString::Printf(TEXT("Injected failure at step %d"), Step), Before, Capture(Fx));
		FBathhouseExpansionAutomationAccess::InjectFailure(*Fx.Purchase, 0);
		TestEqual(TEXT("The purchase works again after the injected failure"),
			Fx.Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0), EBathhouseExpansionFailure::None);
		Fx.Destroy();
	}

	// EXP-031 사용 불가.
	{
		FBathhouseExpansionTestWorld Fx;
		if (!Fx.Create(*this, TEXT("ExpansionUnavailableWorld")))
		{
			Fx.Destroy();
			return false;
		}
		AddExpectedError(TEXT("확장을 사용할 수 없습니다"), EAutomationExpectedErrorFlags::Contains, 0);
		const int32 Money = Fx.Wallet()->GetCurrentMoney();
		// Definition이 잘못되면 사용 불가.
		TArray<FBathhouseExpansionTier> SavedTiers = Fx.Definition->Tiers;
		Fx.Definition->Tiers.Reset();
		FBathhouseExpansionView View = Fx.Purchase->BuildView(Fx.Player);
		TestFalse(TEXT("An invalid definition makes the view unavailable"), View.bAvailable);
		TestEqual(TEXT("Balance stays visible when unavailable"), View.Balance, Money);
		TestEqual(TEXT("Unavailable data rejects the purchase"), Fx.Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0), EBathhouseExpansionFailure::Unavailable);
		TestEqual(TEXT("Money is unchanged"), Fx.Wallet()->GetCurrentMoney(), Money);
		Fx.Definition->Tiers = SavedTiers;
		TestTrue(TEXT("Restored data is available again"), Fx.Purchase->BuildView(Fx.Player).bAvailable);
		// tier 불일치는 사용 불가.
		FText TierReason;
		Fx.Authority->TryAdvanceToTier(1, TierReason);
		TestFalse(TEXT("A tier that disagrees with the hall count is unavailable"), Fx.Purchase->BuildView(Fx.Player).bAvailable);
		// 관리자가 없으면 사용 불가.
		Fx.Authority->Destroy();
		TestFalse(TEXT("A missing authority is unavailable"), Fx.Purchase->BuildView(Fx.Player).bAvailable);
		TestEqual(TEXT("Missing authority rejects the purchase"), Fx.Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0), EBathhouseExpansionFailure::Unavailable);
		TestEqual(TEXT("Money is still unchanged"), Fx.Wallet()->GetCurrentMoney(), Money);
		Fx.Destroy();
	}
	// 구매자 없는 평가(BeginPlay 순서, 사용자 없음)는 로그 없이 사용 불가, 구매자가 있으면 원인별 한 번 Error.
	{
		FBathhouseExpansionTestWorld Fx;
		if (!Fx.Create(*this, TEXT("ExpansionNoBuyerWorld"), false))
		{
			Fx.Destroy();
			return false;
		}
		// 이 구간에 expected error가 없으므로 로그가 남으면 실패한다.
		TestFalse(TEXT("No buyer is unavailable"), Fx.Purchase->BuildView(nullptr).bAvailable);
		TestEqual(TEXT("No buyer rejects the purchase"), Fx.Purchase->EvaluatePurchase(nullptr, EBathhouseSpaceKind::Hall, 0), EBathhouseExpansionFailure::Unavailable);
		AddExpectedError(TEXT("확장을 사용할 수 없습니다"), EAutomationExpectedErrorFlags::Contains, 1);
		TestFalse(TEXT("A buyer without an authority is unavailable"), Fx.Purchase->BuildView(Fx.Player).bAvailable);
		TestFalse(TEXT("The cause is logged only once"), Fx.Purchase->BuildView(Fx.Player).bAvailable);
		Fx.Destroy();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseExpansionLockerAndShopRulesTest,
	"BathhouseSim.Expansion.Locker.LimitAndShopRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseExpansionLockerAndShopRulesTest::RunTest(const FString& Parameters)
{
	// EXP-029: 홀 구입 전에는 한도를 넘는 설치가 거부되고 구입 뒤에는 같은 설치가 가능하다.
	{
		FBathhouseExpansionTestWorld Fx;
		if (!Fx.Create(*this, TEXT("ExpansionLockerWorld")))
		{
			Fx.Destroy();
			return false;
		}
		ULockerCapacitySubsystem* Lockers = Fx.World->GetSubsystem<ULockerCapacitySubsystem>();
		const int32 Request = Fx.Definition->Tiers[0].MaxInstalledLockerSlots + 1;
		TestTrue(TEXT("The request fits the next row"), Request <= Fx.Definition->Tiers[1].MaxInstalledLockerSlots);
		FText Reason;
		TestFalse(TEXT("Installing past the zero-expansion limit is rejected"), Lockers->CanInstallLockerSlots(Request, Reason));
		TestEqual(TEXT("The rejection uses the existing limit text"), Reason.ToString(),
			FString(TEXT("현재 확장 단계의 설치 가능한 락커 칸 수를 초과합니다.")));
		TestEqual(TEXT("Hall purchase succeeds"), Fx.Purchase->TryPurchase(Fx.Player, EBathhouseSpaceKind::Hall, 0), EBathhouseExpansionFailure::None);
		TestTrue(TEXT("After the hall purchase the same install is allowed"), Lockers->CanInstallLockerSlots(Request, Reason));
		Fx.Destroy();
	}

	// 상점 규칙: 락커 상품은 Discardable이 없어야 하고 비락커 상품은 있어야 한다.
	auto MakeDefinition = [](const int32 Slots, const bool bDiscardable)
	{
		UFacilityPlacementDefinition* Definition = NewObject<UFacilityPlacementDefinition>();
		Definition->StableId = MakeUniqueObjectName(GetTransientPackage(), UFacilityPlacementDefinition::StaticClass(), TEXT("ExpansionShopRule"));
		Definition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
		if (bDiscardable)
		{
			Definition->FacilityTags.AddTag(TAG_Facility_Discardable);
		}
		Definition->PlacedFacilityClass = ABathhouseFacilityActor::StaticClass();
		Definition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
		Definition->RecoveryItemMesh = nullptr;
		Definition->LockerSlotCount = Slots;
		return Definition;
	};
	// 설비 실제 사용 가능 판정(footprint 등)은 grid 설정에 달려 있어 기존 Shop 자동화가 본다. 여기서는 태그 규칙의 문구로 판정한다.
	const FString DiscardRequired = TEXT("설비 상품은 Facility.Discardable 설비여야 합니다.");
	const FString DiscardForbidden = TEXT("락커 상품에는 Facility.Discardable 태그를 둘 수 없습니다.");
	FText Reason;
	TestFalse(TEXT("A locker product with the discard tag is invalid"),
		FShopProductRules::ValidateDefinitions(MakeDefinition(1, true), nullptr, Reason));
	TestEqual(TEXT("It reports the locker tag rule"), Reason.ToString(), DiscardForbidden);
	TestFalse(TEXT("A non-locker product without the discard tag is invalid"),
		FShopProductRules::ValidateDefinitions(MakeDefinition(0, false), nullptr, Reason));
	TestEqual(TEXT("It reports the non-locker tag rule"), Reason.ToString(), DiscardRequired);
	FShopProductRules::ValidateDefinitions(MakeDefinition(1, false), nullptr, Reason);
	TestTrue(TEXT("A locker product without the discard tag passes the tag rule"),
		Reason.ToString() != DiscardRequired && Reason.ToString() != DiscardForbidden);
	FShopProductRules::ValidateDefinitions(MakeDefinition(0, true), nullptr, Reason);
	TestTrue(TEXT("A non-locker product with the discard tag passes the tag rule"),
		Reason.ToString() != DiscardRequired && Reason.ToString() != DiscardForbidden);
	FShopProductEntry Product;
	Product.ProductId = TEXT("ClothesLocker1");
	Product.DisplayName = NSLOCTEXT("ExpansionAutomation", "Locker1", "1칸 락커");
	Product.Price = 1;
	Product.bForSale = true;
	Product.PlacementDefinition = MakeDefinition(1, false);
	FShopProductRules::ValidateProduct(Product, Reason);
	TestTrue(TEXT("A one-slot locker product passes the product tag rule"),
		Reason.ToString() != DiscardRequired && Reason.ToString() != DiscardForbidden);
	Product.PlacementDefinition = MakeDefinition(1, true);
	TestFalse(TEXT("A discardable locker product is rejected by the product rules"), FShopProductRules::ValidateProduct(Product, Reason));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseExpansionPreviewLabelTest,
	"BathhouseSim.Expansion.Preview.LabelPlacementAndComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseExpansionPreviewLabelTest::RunTest(const FString& Parameters)
{
	const FBathhouseLayoutValues Values = FixtureValues();
	const double Height = 275.0;
	// 공간 하나: 자기 천장 판 윗면 + 여유.
	{
		const FSnapshots Single = { MakeSnapshots(Values)[0] };
		const FBathhousePreviewLabelPlacement One = FBathhouseSpaceLayout::PreviewLabelPlacement(
			Single, 0, Values.WallThicknessCm, Values.SlabThicknessCm, Height);
		ExpectNear(*this, TEXT("Single space label Z"), One.Location.Z,
			FBathhouseSpaceLayout::CeilingZ(Single[0]) + Values.SlabThicknessCm + Height, ExpTolerance);
		TestFalse(TEXT("A single space has no lower-floor flag"), One.bSouthOfCenter);
	}
	// 홀·목욕(같은 바닥)·지하: 세 글자 Z가 같고 지하만 남쪽, XY는 효과 횟수 안쪽 중심.
	FSnapshots All = MakeSnapshots(Values);
	All[0] = FBathhouseSpaceLayout::WithExpansionCount(All[0], 1);
	double Top = -TNumericLimits<double>::Max();
	for (const FBathhouseSpaceSnapshot& Space : All)
	{
		Top = FMath::Max(Top, FBathhouseSpaceLayout::CeilingZ(Space) + Values.SlabThicknessCm);
	}
	for (int32 Index = 0; Index < All.Num(); ++Index)
	{
		const FBathhousePreviewLabelPlacement Placement = FBathhouseSpaceLayout::PreviewLabelPlacement(
			All, Index, Values.WallThicknessCm, Values.SlabThicknessCm, Height);
		ExpectNear(*this, TEXT("All labels float at the highest ceiling top plus the margin"), Placement.Location.Z, Top + Height, ExpTolerance);
		TestTrue(TEXT("Label XY is the effective interior center"),
			FVector2D(Placement.Location.X, Placement.Location.Y).Equals(All[Index].Interior.GetCenter(), ExpTolerance));
		TestEqual(TEXT("Only the underground space sits south of its center"),
			Placement.bSouthOfCenter, All[Index].Kind == EBathhouseSpaceKind::Work);
	}

	// 편집 미리보기 shell 입력으로 만든 글자 component.
	FBathhouseExpansionTestWorld Fx;
	if (!Fx.Create(*this, TEXT("ExpansionPreviewLabelWorld")))
	{
		Fx.Destroy();
		return false;
	}
	const UBathhouseBuildingSettings* Settings = GetDefault<UBathhouseBuildingSettings>();
	FSnapshots Snapshots;
	FBathhouseSpaceValidation::GatherSnapshots(*Fx.World, Snapshots);
	TArray<FBathhouseSpacePlan> Plans;
	FBathhouseSpaceLayout::Build(Snapshots, Values, Plans);
	UBathhouseSpaceShellComponent* Shell = Fx.Hall->GetShell();
	FBathhouseShellVisualInputs Inputs;
	Inputs.BoxMesh = Settings->LoadShellBoxMesh();
	Inputs.bPreviewChunks = true;
	Inputs.PreviewLabel = TEXT("넓힘 미리보기 1회");
	const FBathhousePreviewLabelPlacement Placement = FBathhouseSpaceLayout::PreviewLabelPlacement(
		Snapshots, 0, Values.WallThicknessCm, Values.SlabThicknessCm, Settings->GetEditorPreviewLabelHeightCm());
	Inputs.PreviewLabelLocation = Placement.Location;
	Inputs.bPreviewLabelSouthOfCenter = Placement.bSouthOfCenter;
	Inputs.PreviewLabelWorldSizeCm = Settings->GetEditorPreviewLabelWorldSizeCm();
	Inputs.PreviewLabelFontSize = Settings->GetEditorPreviewLabelFontSize();
	TestTrue(TEXT("Settings values are positive"), Inputs.PreviewLabelFontSize > 0 && Inputs.PreviewLabelWorldSizeCm > 0.0f);
	Shell->Rebuild(Plans[0], Inputs);

	UWidgetComponent* Label = nullptr;
	int32 LabelCount = 0;
	for (USceneComponent* Child : Shell->GetAttachChildren())
	{
		if (UWidgetComponent* Widget = Cast<UWidgetComponent>(Child))
		{
			Label = Widget;
			++LabelCount;
		}
	}
	TestEqual(TEXT("Exactly one preview label component"), LabelCount, 1);
	if (Label)
	{
		TestTrue(TEXT("Label is editor-only"), Label->bIsEditorOnly);
		TestTrue(TEXT("Label is transient"), Label->HasAnyFlags(RF_Transient));
		TestTrue(TEXT("Label has no collision"), Label->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Label does not affect navigation"), Label->CanEverAffectNavigation());
		// 그리기 조건(엔진 ShouldDrawWidget은 protected라 공개 조건 묶음으로 같은 판정을 본다).
		TestFalse(TEXT("Label is not hidden in game (hidden blocks drawing)"), Label->bHiddenInGame);
		TestTrue(TEXT("Label IsVisible"), Label->IsVisible());
		TestTrue(TEXT("Label ticks when offscreen"), Label->GetTickWhenOffscreen());
		TestEqual(TEXT("Label redraws every tick"), Label->GetRedrawTime(), 0.0f);
		TestTrue(TEXT("Label component tick is enabled"), Label->IsComponentTickEnabled());
		TestTrue(TEXT("Label ticks in editor"), Label->bTickInEditor);
		TestTrue(TEXT("Label tick function is registered"), Label->PrimaryComponentTick.IsTickFunctionRegistered());
		TestTrue(TEXT("Label draw size is positive"), Label->GetDrawSize().X > 0.0 && Label->GetDrawSize().Y > 0.0);
		TestFalse(TEXT("Label takes no hardware input"), Label->GetReceiveHardwareInput());
		TestTrue(TEXT("Label is a world-space widget"), Label->GetWidgetSpace() == EWidgetSpace::World);
		TestTrue(TEXT("Label uses the preview label widget class"), Label->GetWidgetClass() == UBathhouseSpacePreviewLabelWidget::StaticClass());
		TestTrue(TEXT("Label is placed by the helper"), Label->GetComponentLocation().Equals(Placement.Location, ExpTolerance));
		TestTrue(TEXT("Label front faces up"), Label->GetForwardVector().Equals(FVector::UpVector, 0.001));
		TestTrue(TEXT("Label top points north"), Label->GetUpVector().Equals(FVector::RightVector, 0.001));
		const double ExpectedScale = static_cast<double>(Inputs.PreviewLabelWorldSizeCm) / Inputs.PreviewLabelFontSize;
		TestTrue(TEXT("Label scale is world size / font size"), Label->GetComponentScale().Equals(FVector(ExpectedScale), 0.001));
		TestEqual(TEXT("Pivot follows the north attach"), Label->GetPivot().Y, Placement.bSouthOfCenter ? 0.0 : 1.0);
		if (const UBathhouseSpacePreviewLabelWidget* Widget = Cast<UBathhouseSpacePreviewLabelWidget>(Label->GetUserWidgetObject()))
		{
			AddInfo(TEXT("Slate is available: the label widget was created."));
			(void)Widget;
		}
	}
	TestEqual(TEXT("The shell reports the label text"), Shell->GetPreviewLabelText(), FString(TEXT("넓힘 미리보기 1회")));

	// game world 경로(bPreviewChunks 거짓)와 재생성은 글자를 남기지 않는다.
	Inputs.bPreviewChunks = false;
	Shell->Rebuild(Plans[0], Inputs);
	TestEqual(TEXT("No label without the editor preview flag"), Shell->GetPreviewLabelText(), FString());
	TestTrue(TEXT("The game world hall has no label component"), !Fx.Hall->FindComponentByClass<UWidgetComponent>());
	Fx.Destroy();
	return true;
}

#endif
