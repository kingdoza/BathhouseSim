#include "UI/BathWaterMapGridLayout.h"

bool ComputeBathWaterMapLetterbox(
	const FVector2D& CanvasSize, const float ZoneHalfXCm, const float ZoneHalfYCm, FBathWaterMapLetterbox& Out)
{
	Out = FBathWaterMapLetterbox();
	if (!FMath::IsFinite(CanvasSize.X) || !FMath::IsFinite(CanvasSize.Y)
		|| !FMath::IsFinite(ZoneHalfXCm) || !FMath::IsFinite(ZoneHalfYCm)
		|| CanvasSize.X <= 0.0 || CanvasSize.Y <= 0.0
		|| ZoneHalfXCm <= 0.0f || ZoneHalfYCm <= 0.0f)
	{
		return false;
	}
	Out.PixelsPerCm = static_cast<float>(FMath::Min(
		CanvasSize.X / (2.0 * ZoneHalfYCm), CanvasSize.Y / (2.0 * ZoneHalfXCm)));
	Out.ContentSize = FVector2D(2.0 * ZoneHalfYCm * Out.PixelsPerCm, 2.0 * ZoneHalfXCm * Out.PixelsPerCm);
	Out.Origin = (CanvasSize - Out.ContentSize) * 0.5;
	return true;
}

FVector2D ResolveRenderPixelsPerLayoutUnit(const FGeometry& PaintSpaceGeometry)
{
	const FSlateRenderTransform& Transform = PaintSpaceGeometry.GetAccumulatedRenderTransform();
	const FVector2D Scale(
		FVector2D(Transform.TransformVector(FVector2D(1.0, 0.0))).Size(),
		FVector2D(Transform.TransformVector(FVector2D(0.0, 1.0))).Size());
	if (!FMath::IsFinite(Scale.X) || !FMath::IsFinite(Scale.Y) || Scale.X <= 0.0 || Scale.Y <= 0.0)
	{
		return FVector2D(1.0, 1.0);
	}
	return Scale;
}

float ToLayoutLineThickness(const float RenderThicknessPx, const float RenderPixelsPerLayoutUnit)
{
	const float Scale = (FMath::IsFinite(RenderPixelsPerLayoutUnit) && RenderPixelsPerLayoutUnit > 0.0f)
		? RenderPixelsPerLayoutUnit : 1.0f;
	const float Thickness = FMath::IsFinite(RenderThicknessPx) ? RenderThicknessPx : 1.0f;
	return FMath::Max(1.0f, Thickness) / Scale;
}

bool BuildBathWaterMapGridLines(const FBathWaterMapGridInput& Input, TArray<FBathWaterMapGridLine>& OutLines)
{
	OutLines.Reset();
	FBathWaterMapLetterbox Box;
	if (!ComputeBathWaterMapLetterbox(Input.CanvasSize, Input.ZoneHalfXCm, Input.ZoneHalfYCm, Box)
		|| !FMath::IsFinite(Input.MajorSpacingCm) || Input.MajorSpacingCm <= 0.0f)
	{
		return false;
	}
	const float HalfX = Input.ZoneHalfXCm;
	const float HalfY = Input.ZoneHalfYCm;
	const float Spacing = Input.MajorSpacingCm;
	const float ScaleX = static_cast<float>(Input.RenderPixelsPerLayoutUnit.X);
	const float ScaleY = static_cast<float>(Input.RenderPixelsPerLayoutUnit.Y);
	// 세로선은 X축, 가로선은 Y축 배율로 두께를 정한다.
	const float GridW = ToLayoutLineThickness(Input.GridThicknessPx, ScaleX);
	const float GridH = ToLayoutLineThickness(Input.GridThicknessPx, ScaleY);
	const float BoundaryW = ToLayoutLineThickness(Input.BoundaryThicknessPx, ScaleX);
	const float BoundaryH = ToLayoutLineThickness(Input.BoundaryThicknessPx, ScaleY);
	const float EdgeTolerance = KINDA_SMALL_NUMBER * Spacing;

	// 비정상 geometry에서 widget 폭증을 막는 안전 상한(축별 선 수).
	if (HalfX / Spacing < 200.0f && HalfY / Spacing < 200.0f)
	{
		for (int32 Cell = FMath::CeilToInt(-HalfY / Spacing); Cell <= FMath::FloorToInt(HalfY / Spacing); ++Cell)
		{
			const float Coord = Cell * Spacing;
			if (FMath::Abs(Coord) >= HalfY - EdgeTolerance)
			{
				continue;
			}
			const double CenterX = Box.Origin.X + (Coord + HalfY) * Box.PixelsPerCm;
			OutLines.Add({ FVector2D(CenterX - GridW * 0.5, Box.Origin.Y), FVector2D(GridW, Box.ContentSize.Y), false });
		}
		for (int32 Cell = FMath::CeilToInt(-HalfX / Spacing); Cell <= FMath::FloorToInt(HalfX / Spacing); ++Cell)
		{
			const float Coord = Cell * Spacing;
			if (FMath::Abs(Coord) >= HalfX - EdgeTolerance)
			{
				continue;
			}
			const double CenterY = Box.Origin.Y + (HalfX - Coord) * Box.PixelsPerCm;
			OutLines.Add({ FVector2D(Box.Origin.X, CenterY - GridH * 0.5), FVector2D(Box.ContentSize.X, GridH), false });
		}
	}
	OutLines.Add({ Box.Origin, FVector2D(Box.ContentSize.X, BoundaryH), true });
	OutLines.Add({ Box.Origin + FVector2D(0.0, Box.ContentSize.Y - BoundaryH), FVector2D(Box.ContentSize.X, BoundaryH), true });
	OutLines.Add({ Box.Origin, FVector2D(BoundaryW, Box.ContentSize.Y), true });
	OutLines.Add({ Box.Origin + FVector2D(Box.ContentSize.X - BoundaryW, 0.0), FVector2D(BoundaryW, Box.ContentSize.Y), true });
	return true;
}
