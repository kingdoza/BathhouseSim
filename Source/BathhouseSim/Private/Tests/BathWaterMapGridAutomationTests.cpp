#include "Misc/AutomationTest.h"

#include "Components/Button.h"
#include "Layout/Geometry.h"
#include "Rendering/SlateRenderTransform.h"
#include "UI/BathWaterBathTileWidget.h"
#include "UI/BathWaterMapGridLayout.h"
#include "UI/BathWaterMapWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

// MAPG-001, MAPG-003: 선 두께는 render px 기준이고 어떤 배율·시작 소수부에서도 선이 사라지지 않는다.
// 두께 입력은 WBP Class Defaults의 원본 프로퍼티 기본값에서 읽고 기대값은 입력에서 계산한다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathWaterMapGridLineLayoutTest,
	"BathhouseSim.BathWater.Operations.MapGridLineLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathWaterMapGridLineLayoutTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const UBathWaterMapWidget* Defaults = GetDefault<UBathWaterMapWidget>();
	const float GridPx = Defaults->GridLineThicknessPx;
	const float BoundaryPx = Defaults->BoundaryLineThicknessPx;

	struct FCase { const TCHAR* Name; FVector2D Scale; float GridPx; float BoundaryPx; };
	const FCase Cases[] = {
		{ TEXT("scale below one (non-integer, non-uniform)"), FVector2D(0.61, 0.73), GridPx, BoundaryPx },
		{ TEXT("scale one"), FVector2D(1.0, 1.0), GridPx, BoundaryPx },
		{ TEXT("scale above one"), FVector2D(1.5, 1.3), GridPx, BoundaryPx },
		{ TEXT("input thickness below one"), FVector2D(0.61, 0.73), 0.5f, 0.5f },
	};
	const FVector2D Canvas(640.0, 420.0);
	const float HalfX = 200.0f;
	const float HalfY = 150.0f;
	const float Spacing = 45.0f;

	for (const FCase& Case : Cases)
	{
		FBathWaterMapGridInput Input;
		Input.CanvasSize = Canvas;
		Input.ZoneHalfXCm = HalfX;
		Input.ZoneHalfYCm = HalfY;
		Input.MajorSpacingCm = Spacing;
		Input.RenderPixelsPerLayoutUnit = Case.Scale;
		Input.GridThicknessPx = Case.GridPx;
		Input.BoundaryThicknessPx = Case.BoundaryPx;
		TArray<FBathWaterMapGridLine> Lines;
		if (!TestTrue(FString::Printf(TEXT("[MAPG-001] %s builds lines"), Case.Name),
			BuildBathWaterMapGridLines(Input, Lines)))
		{
			continue;
		}
		FBathWaterMapLetterbox Box;
		TestTrue(TEXT("Letterbox is valid"), ComputeBathWaterMapLetterbox(Canvas, HalfX, HalfY, Box));

		int32 ExpectedVertical = 0;
		int32 ExpectedHorizontal = 0;
		const float Tolerance = KINDA_SMALL_NUMBER * Spacing;
		for (int32 K = -200; K <= 200; ++K)
		{
			ExpectedVertical += FMath::Abs(K * Spacing) < HalfY - Tolerance ? 1 : 0;
			ExpectedHorizontal += FMath::Abs(K * Spacing) < HalfX - Tolerance ? 1 : 0;
		}
		int32 Boundary = 0;
		int32 Vertical = 0;
		int32 Horizontal = 0;
		for (const FBathWaterMapGridLine& Line : Lines)
		{
			const bool bVertical = Line.Size.Y > Line.Size.X;
			const float Wanted = FMath::Max(1.0f, Line.bBoundary ? Case.BoundaryPx : Case.GridPx);
			const double RenderThickness = bVertical ? Line.Size.X * Case.Scale.X : Line.Size.Y * Case.Scale.Y;
			TestTrue(FString::Printf(TEXT("[MAPG-001] %s: render thickness equals max(1, input)"), Case.Name),
				FMath::IsNearlyEqual(static_cast<float>(RenderThickness), Wanted, 1e-4f));
			const double Start = bVertical ? Line.Position.X * Case.Scale.X : Line.Position.Y * Case.Scale.Y;
			bool bAlwaysVisible = true;
			bool bOldRuleVanishes = false;
			const double OldThickness = bVertical ? Case.Scale.X : Case.Scale.Y;
			for (int32 Step = 0; Step < 100; ++Step)
			{
				const double Offset = Start + Step * 0.01;
				bAlwaysVisible &= FMath::RoundToInt(Offset + RenderThickness) - FMath::RoundToInt(Offset) >= 1;
				bOldRuleVanishes |= FMath::RoundToInt(Offset + OldThickness) - FMath::RoundToInt(Offset) < 1;
			}
			TestTrue(FString::Printf(TEXT("[MAPG-001] %s: line covers a pixel for every start fraction"), Case.Name),
				bAlwaysVisible);
			if (Case.Scale.X < 1.0 && !Line.bBoundary)
			{
				TestTrue(FString::Printf(TEXT("[MAPG-001] %s: legacy 1-layout-px rule can vanish"), Case.Name),
					bOldRuleVanishes);
			}
			if (Line.bBoundary)
			{
				++Boundary;
				TestTrue(TEXT("Boundary line stays inside content rect"),
					Line.Position.X >= Box.Origin.X - 1e-3 && Line.Position.Y >= Box.Origin.Y - 1e-3
					&& Line.Position.X + Line.Size.X <= Box.Origin.X + Box.ContentSize.X + 1e-3
					&& Line.Position.Y + Line.Size.Y <= Box.Origin.Y + Box.ContentSize.Y + 1e-3);
				continue;
			}
			// 칸 선 중심이 칸 경계 좌표와 일치하고(Y는 위 방향 반전) content rect 안이다.
			const double Center = bVertical
				? Line.Position.X + Line.Size.X * 0.5 : Line.Position.Y + Line.Size.Y * 0.5;
			bool bOnCell = false;
			for (int32 K = -200; K <= 200 && !bOnCell; ++K)
			{
				const double Expected = bVertical
					? Box.Origin.X + (K * Spacing + HalfY) * Box.PixelsPerCm
					: Box.Origin.Y + (HalfX - K * Spacing) * Box.PixelsPerCm;
				bOnCell = FMath::IsNearlyEqual(static_cast<float>(Center), static_cast<float>(Expected), 1e-3f);
			}
			TestTrue(FString::Printf(TEXT("[MAPG-001] %s: grid line centered on a cell boundary"), Case.Name), bOnCell);
			TestTrue(TEXT("Grid line center is inside content rect"),
				bVertical
				? Center > Box.Origin.X && Center < Box.Origin.X + Box.ContentSize.X
				: Center > Box.Origin.Y && Center < Box.Origin.Y + Box.ContentSize.Y);
			++(bVertical ? Vertical : Horizontal);
		}
		TestEqual(FString::Printf(TEXT("[MAPG-003] %s: boundary line count"), Case.Name), Boundary, 4);
		TestEqual(FString::Printf(TEXT("[MAPG-001] %s: vertical cell line count"), Case.Name), Vertical, ExpectedVertical);
		TestEqual(FString::Printf(TEXT("[MAPG-001] %s: horizontal cell line count"), Case.Name), Horizontal, ExpectedHorizontal);
	}

	TArray<FBathWaterMapGridLine> Lines;
	FBathWaterMapGridInput Invalid;
	TestFalse(TEXT("Invalid input builds nothing"), BuildBathWaterMapGridLines(Invalid, Lines));

	// ResolveRenderPixelsPerLayoutUnit: 누적 배율 읽기와 무효 대체.
	const FVector2D Size(100.0, 50.0);
	const FVector2D Read = ResolveRenderPixelsPerLayoutUnit(
		FGeometry::MakeRoot(Size, FSlateLayoutTransform(0.37f)));
	TestTrue(TEXT("Accumulated scale is read from the geometry"), Read.Equals(FVector2D(0.37, 0.37), 1e-4));
	TestTrue(TEXT("Default geometry falls back to identity"),
		ResolveRenderPixelsPerLayoutUnit(FGeometry()).Equals(FVector2D(1.0, 1.0)));
	TestTrue(TEXT("Zero scale falls back to identity"), ResolveRenderPixelsPerLayoutUnit(
		FGeometry::MakeRoot(Size, FSlateLayoutTransform(0.0f))).Equals(FVector2D(1.0, 1.0)));
	const float NaNScale = std::numeric_limits<float>::quiet_NaN();
	TestTrue(TEXT("NaN scale falls back to identity"), ResolveRenderPixelsPerLayoutUnit(
		FGeometry::MakeRoot(Size, FSlateLayoutTransform(NaNScale))).Equals(FVector2D(1.0, 1.0)));

	// MAPG-002 보조: 타일 상태색·opacity는 같은 타일 객체의 프로퍼티 값에서 온다.
	UBathWaterBathTileWidget* Tile = NewObject<UBathWaterBathTileWidget>();
	Tile->SelectButton = NewObject<UButton>(Tile);
	FBathWaterBathSnapshot Snapshot;
	Tile->ApplyBathSnapshot(Snapshot, false);
	TestTrue(TEXT("Normal tile uses NormalTileColor"),
		Tile->SelectButton->GetBackgroundColor().Equals(Tile->NormalTileColor));
	TestTrue(TEXT("Unselected tile uses UnselectedRenderOpacity"),
		FMath::IsNearlyEqual(Tile->GetRenderOpacity(), Tile->UnselectedRenderOpacity));
	Tile->ApplyBathSnapshot(Snapshot, true);
	TestTrue(TEXT("Selected tile uses SelectedTileColor"),
		Tile->SelectButton->GetBackgroundColor().Equals(Tile->SelectedTileColor));
	TestTrue(TEXT("Selected tile uses SelectedRenderOpacity"),
		FMath::IsNearlyEqual(Tile->GetRenderOpacity(), Tile->SelectedRenderOpacity));
	Snapshot.bHeatingCapacityDeficit = true;
	Tile->ApplyBathSnapshot(Snapshot, true);
	TestTrue(TEXT("Deficit has priority over selection"),
		Tile->SelectButton->GetBackgroundColor().Equals(Tile->DeficitTileColor));
	return true;
}

#endif
