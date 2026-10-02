#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include <limits>

#include "Building/BathhouseBuildingSettings.h"
#include "Building/BathhouseSpaceActor.h"
#include "Building/BathhouseSpaceLayout.h"
#include "Building/BathhouseSpaceShellComponent.h"
#include "Components/BoxComponent.h"
#include "Tests/BathhouseExpansionAutomationTestSupport.h"

namespace
{
	using Access = FBathhouseExpansionAutomationAccess;
	constexpr double MultiTolerance = 0.01;

	// 아래 값은 fixture 입력이다. 기대값은 같은 입력에서 계산한다.
	constexpr int32 StepPrice = 700;

	FBathhouseExpansionStepSnapshot Step(std::initializer_list<FBathhouseExpansionSideSnapshot> Sides)
	{
		FBathhouseExpansionStepSnapshot Result;
		Result.Price = StepPrice;
		for (const FBathhouseExpansionSideSnapshot& Side : Sides)
		{
			Result.Sides.Add(Side);
		}
		return Result;
	}

	double Area(const FBox2D& Rect) { return (Rect.Max.X - Rect.Min.X) * (Rect.Max.Y - Rect.Min.Y); }

	double Overlap(const FBox2D& A, const FBox2D& B)
	{
		const double X = FMath::Min(A.Max.X, B.Max.X) - FMath::Max(A.Min.X, B.Min.X);
		const double Y = FMath::Min(A.Max.Y, B.Max.Y) - FMath::Max(A.Min.Y, B.Min.Y);
		return X > 0.0 && Y > 0.0 ? X * Y : 0.0;
	}

	bool Near(const FBox2D& A, const FBox2D& B)
	{
		return A.Min.Equals(B.Min, MultiTolerance) && A.Max.Equals(B.Max, MultiTolerance);
	}

	bool Contains(const FBox2D& Rect, const FVector2D& Point)
	{
		return Point.X > Rect.Min.X && Point.X < Rect.Max.X && Point.Y > Rect.Min.Y && Point.Y < Rect.Max.Y;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseExpansionMultiSideLayoutTest,
	"BathhouseSim.Expansion.MultiSide.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseExpansionMultiSideLayoutTest::RunTest(const FString& Parameters)
{
	const EBathhouseSpaceSide South = EBathhouseSpaceSide::South;
	const EBathhouseSpaceSide North = EBathhouseSpaceSide::North;
	const EBathhouseSpaceSide East = EBathhouseSpaceSide::East;
	const EBathhouseSpaceSide West = EBathhouseSpaceSide::West;
	const FBox2D Base(FVector2D(-100.0, -50.0), FVector2D(100.0, 50.0));
	const double A = 30.0;
	const double B = 70.0;
	const double C = 20.0;

	// EXP-023·040·041·047: 한 줄의 여러 벽이 같은 직사각형에 함께 적용된다.
	const TArray<FBathhouseExpansionStepSnapshot> SouthNorth = { Step({ { South, A }, { North, B } }) };
	TestTrue(TEXT("South and north move together by their own amounts"),
		Near(FBathhouseSpaceLayout::ExpandInterior(Base, SouthNorth, 1),
			FBox2D(FVector2D(Base.Min.X, Base.Min.Y - A), FVector2D(Base.Max.X, Base.Max.Y + B))));
	const TArray<FBathhouseExpansionStepSnapshot> ThreeWalls = { Step({ { South, A }, { North, B }, { East, C } }) };
	TestTrue(TEXT("South, north and east move together"),
		Near(FBathhouseSpaceLayout::ExpandInterior(Base, ThreeWalls, 1),
			FBox2D(FVector2D(Base.Min.X, Base.Min.Y - A), FVector2D(Base.Max.X + C, Base.Max.Y + B))));
	const TArray<FBathhouseExpansionStepSnapshot> EastNorth = { Step({ { East, A }, { North, B } }) };
	TestTrue(TEXT("East and north move together"),
		Near(FBathhouseSpaceLayout::ExpandInterior(Base, EastNorth, 1),
			FBox2D(Base.Min, FVector2D(Base.Max.X + A, Base.Max.Y + B))));
	// EXP-048: 벽마다 다른 양, 항목 순서와 무관.
	const TArray<FBathhouseExpansionStepSnapshot> Reordered = { Step({ { North, B }, { East, C }, { South, A } }) };
	TestTrue(TEXT("Wall order does not change the shape"),
		Near(FBathhouseSpaceLayout::ExpandInterior(Base, ThreeWalls, 1), FBathhouseSpaceLayout::ExpandInterior(Base, Reordered, 1)));
	// 같은 줄 뒤쪽 중복은 무시하고 처음 나온 항목만 쓴다.
	const TArray<FBathhouseExpansionStepSnapshot> Duplicate = { Step({ { South, A }, { South, 999.0 }, { North, B } }) };
	TestTrue(TEXT("A later duplicate wall is ignored"),
		Near(FBathhouseSpaceLayout::ExpandInterior(Base, Duplicate, 1), FBathhouseSpaceLayout::ExpandInterior(Base, SouthNorth, 1)));
	// 양이 잘못된 항목은 건너뛰고 나머지는 적용한다.
	const TArray<FBathhouseExpansionStepSnapshot> BadItem = {
		Step({ { South, 0.0 }, { North, B }, { East, std::numeric_limits<double>::infinity() }, { West, -3.0 } }) };
	TestTrue(TEXT("Unusable items are skipped and the others apply"),
		Near(FBathhouseSpaceLayout::ExpandInterior(Base, BadItem, 1), FBox2D(Base.Min, FVector2D(Base.Max.X, Base.Max.Y + B))));
	// 빈 줄은 변화 없음, 줄이 쌓이면 누적된다.
	const TArray<FBathhouseExpansionStepSnapshot> EmptyStep = { Step({}) };
	TestTrue(TEXT("An empty step leaves the interior"), Near(FBathhouseSpaceLayout::ExpandInterior(Base, EmptyStep, 1), Base));
	const TArray<FBathhouseExpansionStepSnapshot> Two = { Step({ { South, A }, { North, B } }), Step({ { East, C }, { South, A } }) };
	TestTrue(TEXT("Two steps accumulate"),
		Near(FBathhouseSpaceLayout::ExpandInterior(Base, Two, 2),
			FBox2D(FVector2D(Base.Min.X, Base.Min.Y - 2.0 * A), FVector2D(Base.Max.X + C, Base.Max.Y + B))));

	// IsStepApplicable 표.
	TestTrue(TEXT("A normal step is applicable"), FBathhouseSpaceLayout::IsStepApplicable(ThreeWalls[0]));
	TestFalse(TEXT("An empty step is not applicable"), FBathhouseSpaceLayout::IsStepApplicable(EmptyStep[0]));
	TestFalse(TEXT("A duplicate wall is not applicable"), FBathhouseSpaceLayout::IsStepApplicable(Duplicate[0]));
	TestFalse(TEXT("A bad amount is not applicable"), FBathhouseSpaceLayout::IsStepApplicable(BadItem[0]));
	TestFalse(TEXT("A zero amount is not applicable"), FBathhouseSpaceLayout::IsStepApplicable(Step({ { South, 0.0 } })));
	TestTrue(TEXT("A price of zero does not change the shape rule"), FBathhouseSpaceLayout::IsStepApplicable([&]()
	{
		FBathhouseExpansionStepSnapshot Free = ThreeWalls[0];
		Free.Price = 0;
		return Free;
	}()));

	// ExpansionBandRects: 겹치지 않고, 넓이 합 = 넓힌 뒤 - 넓히기 전, 모서리가 정확히 한 직사각형에 든다.
	for (const TArray<FBathhouseExpansionStepSnapshot>* Steps : { &SouthNorth, &ThreeWalls, &EastNorth, &Two })
	{
		for (int32 Index = 0; Index < Steps->Num(); ++Index)
		{
			const FBox2D Before = FBathhouseSpaceLayout::ExpandInterior(Base, *Steps, Index);
			const FBox2D After = FBathhouseSpaceLayout::ExpandInterior(Base, *Steps, Index + 1);
			TArray<FBox2D> Rects;
			FBathhouseSpaceLayout::ExpansionBandRects(Base, *Steps, Index, Rects);
			double Sum = 0.0;
			for (int32 First = 0; First < Rects.Num(); ++First)
			{
				Sum += Area(Rects[First]);
				TestEqual(TEXT("A band rectangle stays out of the previous interior"), Overlap(Rects[First], Before), 0.0);
				for (int32 Second = First + 1; Second < Rects.Num(); ++Second)
				{
					TestEqual(TEXT("Band rectangles do not overlap"), Overlap(Rects[First], Rects[Second]), 0.0);
				}
			}
			TestTrue(TEXT("The band area is after minus before"), FMath::Abs(Sum - (Area(After) - Area(Before))) <= MultiTolerance);
		}
	}
	{
		TArray<FBox2D> Rects;
		FBathhouseSpaceLayout::ExpansionBandRects(Base, EastNorth, 0, Rects);
		const FVector2D Corner(Base.Max.X + A * 0.5, Base.Max.Y + B * 0.5);
		int32 Count = 0;
		for (const FBox2D& Rect : Rects)
		{
			Count += Contains(Rect, Corner) ? 1 : 0;
		}
		TestEqual(TEXT("The corner where two moved walls meet is in exactly one rectangle"), Count, 1);
		const FVector2D SouthEast(Base.Max.X + C * 0.5, Base.Min.Y - A * 0.5);
		Rects.Reset();
		FBathhouseSpaceLayout::ExpansionBandRects(Base, ThreeWalls, 0, Rects);
		Count = 0;
		for (const FBox2D& Rect : Rects)
		{
			Count += Contains(Rect, SouthEast) ? 1 : 0;
		}
		TestEqual(TEXT("The south-east corner of a three wall step is in exactly one rectangle"), Count, 1);
	}
	{
		TArray<FBox2D> Rects;
		FBathhouseSpaceLayout::ExpansionBandRects(Base, ThreeWalls, 5, Rects);
		TestEqual(TEXT("An index past the list gives no rectangles"), Rects.Num(), 0);
		FBathhouseSpaceLayout::ExpansionBandRects(Base, ThreeWalls, -1, Rects);
		TestEqual(TEXT("A negative index gives no rectangles"), Rects.Num(), 0);
		FBathhouseSpaceLayout::ExpansionBandRects(Base, EmptyStep, 0, Rects);
		TestEqual(TEXT("An empty step gives no rectangles"), Rects.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseExpansionMultiSideRuntimeTest,
	"BathhouseSim.Expansion.MultiSide.Runtime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseExpansionMultiSideRuntimeTest::RunTest(const FString& Parameters)
{
	FBathhouseExpansionTestWorld Fx;
	if (!Fx.Create(*this, TEXT("ExpansionMultiSideWorld")))
	{
		Fx.Destroy();
		return false;
	}
	ABathhouseSpaceActor* Bath = Fx.Bath;
	ABathhouseSpaceActor* Hall = Fx.Hall;
	const FVector2D ChunkMax = GetDefault<UBathhouseBuildingSettings>()->GetCleaningChunkMaxSizeCm();
	const FBox2D Base = Bath->GetBaseInteriorRect();
	const TArray<TWeakObjectPtr<AActor>> ChunksBefore = Bath->GetCleaningChunks();
	const UInstancedStaticMeshComponent* HallWallBefore = Hall->GetShell()->FindPartComponent(EBathhouseShellPart::Wall);

	// EXP-048: 벽마다 다른 양(남은 fixture 양의 1.5배, 북은 0.5배, 동은 그대로).
	Access::SetStepAmount(*Bath, 0, 0, FBathhouseExpansionTestWorld::FixtureAmountCm * 1.5f);
	Access::SetStepAmount(*Bath, 0, 1, FBathhouseExpansionTestWorld::FixtureAmountCm * 0.5f);

	// EXP-047: 목욕공간 남·북·동을 한 번에, 모서리 포함 늘어난 바닥에 조각.
	FBathhouseSpaceExpansionUndo Undo;
	FText Reason;
	TestTrue(TEXT("The bath step with three walls can be applied"), Bath->CanApplyNextExpansion(Reason));
	TestTrue(TEXT("The bath step applies"), Bath->ApplyNextExpansion(Undo, Reason));
	TestEqual(TEXT("One step is counted for three walls"), Bath->GetAppliedExpansionCount(), 1);
	const FBox2D After = Bath->GetInteriorRect();
	const FBathhouseSpaceExpansionStep Step = Access::Steps(*Bath)[0];
	double SouthAmount = 0.0;
	double NorthAmount = 0.0;
	double EastAmount = 0.0;
	for (const FBathhouseSpaceExpansionSide& Wall : Step.Sides)
	{
		if (Wall.Side == EBathhouseSpaceSide::South) { SouthAmount = Wall.AmountCm; }
		if (Wall.Side == EBathhouseSpaceSide::North) { NorthAmount = Wall.AmountCm; }
		if (Wall.Side == EBathhouseSpaceSide::East) { EastAmount = Wall.AmountCm; }
	}
	TestTrue(TEXT("All three walls moved in the same application"),
		Near(After, FBox2D(FVector2D(Base.Min.X, Base.Min.Y - SouthAmount), FVector2D(Base.Max.X + EastAmount, Base.Max.Y + NorthAmount))));
	const UBoxComponent* Zone = Bath->GetZoneBounds();
	TestTrue(TEXT("The zone follows the multi-wall rectangle"),
		FMath::Abs(Zone->GetUnscaledBoxExtent().X - (After.Max.X - After.Min.X) * 0.5) <= MultiTolerance
		&& FMath::Abs(Zone->GetUnscaledBoxExtent().Y - (After.Max.Y - After.Min.Y) * 0.5) <= MultiTolerance);
	TestTrue(TEXT("The zone center follows the multi-wall rectangle"),
		FVector2D(Zone->GetComponentLocation().X, Zone->GetComponentLocation().Y).Equals(After.GetCenter(), MultiTolerance));
	TestTrue(TEXT("The neighbor hall shell is not rebuilt"), Hall->GetShell()->FindPartComponent(EBathhouseShellPart::Wall) == HallWallBefore);

	// 새 조각의 넓이 합은 늘어난 면적, 기존 조각 identity 유지, 모서리 중심이 조각 하나에 든다.
	TArray<FBox2D> BandRects;
	TArray<FBathhouseExpansionStepSnapshot> Steps;
	FBathhouseExpansionStepSnapshot Item;
	Item.Price = Step.Price;
	for (const FBathhouseSpaceExpansionSide& Wall : Step.Sides)
	{
		Item.Sides.Add({ Wall.Side, Wall.AmountCm });
	}
	Steps.Add(Item);
	FBathhouseSpaceLayout::ExpansionBandRects(Base, Steps, 0, BandRects);
	TArray<FBox2D> ExpectedChunks;
	for (const FBox2D& Rect : BandRects)
	{
		TArray<FBox2D> Split;
		FBathhouseSpaceLayout::SplitChunks(Rect, ChunkMax, Split);
		ExpectedChunks.Append(Split);
	}
	TestEqual(TEXT("The new stain chunk count matches the band split"),
		Bath->GetCleaningChunks().Num() - ChunksBefore.Num(), ExpectedChunks.Num());
	double ExpectedArea = 0.0;
	for (const FBox2D& Chunk : ExpectedChunks)
	{
		ExpectedArea += Area(Chunk);
	}
	const double Grown = Area(After) - Area(Base);
	TestTrue(TEXT("The new chunk area covers the whole grown floor, corners included"), FMath::Abs(ExpectedArea - Grown) <= MultiTolerance);
	for (int32 Index = 0; Index < ChunksBefore.Num(); ++Index)
	{
		TestTrue(TEXT("Existing chunk identity is kept"), Bath->GetCleaningChunks()[Index] == ChunksBefore[Index] && ChunksBefore[Index].IsValid());
	}
	const FVector2D NorthEastCorner(Base.Max.X + EastAmount * 0.5, Base.Max.Y + NorthAmount * 0.5);
	int32 CornerHits = 0;
	for (const FBox2D& Chunk : ExpectedChunks)
	{
		CornerHits += Contains(Chunk, NorthEastCorner) ? 1 : 0;
	}
	TestEqual(TEXT("The north-east corner has exactly one new chunk"), CornerHits, 1);

	// 되돌림은 한 번에 원상.
	Bath->UndoExpansion(Undo);
	TestEqual(TEXT("Undo restores the count"), Bath->GetAppliedExpansionCount(), 0);
	TestTrue(TEXT("Undo restores the interior"), Near(Bath->GetInteriorRect(), Base));
	TestEqual(TEXT("Undo removes the new chunks"), Bath->GetCleaningChunks().Num(), ChunksBefore.Num());

	// 적용 조건: 빈 벽·중복·양 잘못·가격 0은 적용할 수 없다.
	Access::Steps(*Bath)[0].Sides.Reset();
	TestFalse(TEXT("A step without walls cannot be applied"), Bath->CanApplyNextExpansion(Reason));
	TestFalse(TEXT("The reason is set"), Reason.IsEmpty());
	Access::Steps(*Bath)[0].Sides = Step.Sides;
	const FBathhouseSpaceExpansionSide FirstWall = Access::Steps(*Bath)[0].Sides[0];
	Access::Steps(*Bath)[0].Sides.Add(FirstWall);
	TestFalse(TEXT("A duplicate wall cannot be applied"), Bath->CanApplyNextExpansion(Reason));
	Access::Steps(*Bath)[0].Sides.Pop();
	Access::SetStepAmount(*Bath, 0, 1, 0.0f);
	TestFalse(TEXT("A zero amount item cannot be applied"), Bath->CanApplyNextExpansion(Reason));
	Access::SetStepAmount(*Bath, 0, 1, static_cast<float>(NorthAmount));
	Access::SetStepPrice(*Bath, 0, 0);
	TestFalse(TEXT("A price of zero cannot be applied"), Bath->CanApplyNextExpansion(Reason));
	Access::SetStepPrice(*Bath, 0, Step.Price > 0 ? Step.Price : 1);
	TestTrue(TEXT("The restored step can be applied again"), Bath->CanApplyNextExpansion(Reason));
	Fx.Destroy();
	return true;
}

#endif
