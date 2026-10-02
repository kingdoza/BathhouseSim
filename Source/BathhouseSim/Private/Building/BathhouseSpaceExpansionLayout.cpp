#include "Building/BathhouseSpaceLayout.h"

// 공간 넓힘 순수 계산. BathhouseSpaceLayout.cpp는 크기 한도 때문에 건드리지 않는다.

namespace
{
	/** 줄 하나의 쓸 수 있는 벽(양 유효, 같은 줄에서 처음 나온 방향)을 Rect에 적용한다. */
	FBox2D ApplyStep(const FBox2D& Rect, const FBathhouseExpansionStepSnapshot& Step)
	{
		FBox2D Result = Rect;
		TArray<EBathhouseSpaceSide, TInlineAllocator<4>> Seen;
		for (const FBathhouseExpansionSideSnapshot& Wall : Step.Sides)
		{
			if (Seen.Contains(Wall.Side))
			{
				continue;
			}
			Seen.Add(Wall.Side);
			if (!FBathhouseSpaceLayout::IsUsableSide(Wall))
			{
				continue;
			}
			switch (Wall.Side)
			{
			case EBathhouseSpaceSide::East: Result.Max.X += Wall.AmountCm; break;
			case EBathhouseSpaceSide::West: Result.Min.X -= Wall.AmountCm; break;
			case EBathhouseSpaceSide::North: Result.Max.Y += Wall.AmountCm; break;
			default: Result.Min.Y -= Wall.AmountCm; break;
			}
		}
		return Result;
	}
}

bool FBathhouseSpaceLayout::IsUsableSide(const FBathhouseExpansionSideSnapshot& Side)
{
	return Side.AmountCm > 0.0 && FMath::IsFinite(Side.AmountCm);
}

bool FBathhouseSpaceLayout::IsStepApplicable(const FBathhouseExpansionStepSnapshot& Step)
{
	if (Step.Sides.IsEmpty())
	{
		return false;
	}
	for (int32 Index = 0; Index < Step.Sides.Num(); ++Index)
	{
		if (!IsUsableSide(Step.Sides[Index]))
		{
			return false;
		}
		for (int32 Other = 0; Other < Index; ++Other)
		{
			if (Step.Sides[Other].Side == Step.Sides[Index].Side)
			{
				return false;
			}
		}
	}
	return true;
}

FBox2D FBathhouseSpaceLayout::ExpandInterior(
	const FBox2D& Base, const TArray<FBathhouseExpansionStepSnapshot>& Steps, const int32 Count)
{
	FBox2D Result = Base;
	const int32 Applied = FMath::Clamp(Count, 0, Steps.Num());
	for (int32 Index = 0; Index < Applied; ++Index)
	{
		Result = ApplyStep(Result, Steps[Index]);
	}
	return Result;
}

void FBathhouseSpaceLayout::ExpansionBandRects(
	const FBox2D& Base, const TArray<FBathhouseExpansionStepSnapshot>& Steps, const int32 StepIndex, TArray<FBox2D>& OutRects)
{
	OutRects.Reset();
	if (!Steps.IsValidIndex(StepIndex))
	{
		return;
	}
	const FBox2D Before = ExpandInterior(Base, Steps, StepIndex);
	const FBox2D After = ExpandInterior(Base, Steps, StepIndex + 1);
	SubtractRects(After, TArray<FBox2D>{Before}, OutRects);
}

FBathhouseSpaceSnapshot FBathhouseSpaceLayout::WithExpansionCount(const FBathhouseSpaceSnapshot& Snapshot, const int32 Count)
{
	FBathhouseSpaceSnapshot Copy = Snapshot;
	Copy.ExpansionCount = FMath::Clamp(Count, 0, Snapshot.Steps.Num());
	Copy.Interior = ExpandInterior(Snapshot.BaseInterior, Snapshot.Steps, Copy.ExpansionCount);
	return Copy;
}

FBathhousePreviewLabelPlacement FBathhouseSpaceLayout::PreviewLabelPlacement(
	const TArray<FBathhouseSpaceSnapshot>& Snapshots, const int32 Index, const double WallThicknessCm,
	const double SlabThicknessCm, const double HeightCm)
{
	FBathhousePreviewLabelPlacement Result;
	if (!Snapshots.IsValidIndex(Index))
	{
		return Result;
	}
	double Top = CeilingZ(Snapshots[Index]) + SlabThicknessCm;
	for (const FBathhouseSpaceSnapshot& Space : Snapshots)
	{
		Top = FMath::Max(Top, CeilingZ(Space) + SlabThicknessCm);
	}
	const FVector2D Center = Snapshots[Index].Interior.GetCenter();
	Result.Location = FVector(Center.X, Center.Y, Top + HeightCm);
	const FBox2D Self = OuterRect(Snapshots[Index], WallThicknessCm);
	for (int32 Other = 0; Other < Snapshots.Num(); ++Other)
	{
		if (Other == Index || !(Snapshots[Other].FloorZ > Snapshots[Index].FloorZ + UE_KINDA_SMALL_NUMBER))
		{
			continue;
		}
		const FBox2D Rect = OuterRect(Snapshots[Other], WallThicknessCm);
		const double OverlapX = FMath::Min(Self.Max.X, Rect.Max.X) - FMath::Max(Self.Min.X, Rect.Min.X);
		const double OverlapY = FMath::Min(Self.Max.Y, Rect.Max.Y) - FMath::Max(Self.Min.Y, Rect.Min.Y);
		if (OverlapX > UE_KINDA_SMALL_NUMBER && OverlapY > UE_KINDA_SMALL_NUMBER)
		{
			Result.bSouthOfCenter = true;
			break;
		}
	}
	return Result;
}
