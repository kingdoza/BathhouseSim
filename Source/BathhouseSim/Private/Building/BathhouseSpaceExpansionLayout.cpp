#include "Building/BathhouseSpaceLayout.h"

// 공간 넓힘 순수 계산. BathhouseSpaceLayout.cpp는 크기 한도 때문에 건드리지 않는다.

namespace
{
	bool IsUsableStep(const FBathhouseExpansionStepSnapshot& Step)
	{
		return Step.AmountCm > 0.0 && FMath::IsFinite(Step.AmountCm);
	}

	FBox2D ApplyStep(const FBox2D& Rect, const FBathhouseExpansionStepSnapshot& Step)
	{
		FBox2D Result = Rect;
		switch (Step.Side)
		{
		case EBathhouseSpaceSide::East: Result.Max.X += Step.AmountCm; break;
		case EBathhouseSpaceSide::West: Result.Min.X -= Step.AmountCm; break;
		case EBathhouseSpaceSide::North: Result.Max.Y += Step.AmountCm; break;
		default: Result.Min.Y -= Step.AmountCm; break;
		}
		return Result;
	}
}

FBox2D FBathhouseSpaceLayout::ExpandInterior(
	const FBox2D& Base, const TArray<FBathhouseExpansionStepSnapshot>& Steps, const int32 Count)
{
	FBox2D Result = Base;
	const int32 Applied = FMath::Clamp(Count, 0, Steps.Num());
	for (int32 Index = 0; Index < Applied; ++Index)
	{
		if (IsUsableStep(Steps[Index]))
		{
			Result = ApplyStep(Result, Steps[Index]);
		}
	}
	return Result;
}

FBox2D FBathhouseSpaceLayout::ExpansionBand(
	const FBox2D& Base, const TArray<FBathhouseExpansionStepSnapshot>& Steps, const int32 StepIndex)
{
	const FBox2D Empty(FVector2D::ZeroVector, FVector2D::ZeroVector);
	if (!Steps.IsValidIndex(StepIndex) || !IsUsableStep(Steps[StepIndex]))
	{
		return Empty;
	}
	const FBox2D Before = ExpandInterior(Base, Steps, StepIndex);
	const FBox2D After = ApplyStep(Before, Steps[StepIndex]);
	switch (Steps[StepIndex].Side)
	{
	case EBathhouseSpaceSide::East: return FBox2D(FVector2D(Before.Max.X, Before.Min.Y), After.Max);
	case EBathhouseSpaceSide::West: return FBox2D(After.Min, FVector2D(Before.Min.X, Before.Max.Y));
	case EBathhouseSpaceSide::North: return FBox2D(FVector2D(Before.Min.X, Before.Max.Y), After.Max);
	default: return FBox2D(After.Min, FVector2D(Before.Max.X, Before.Min.Y));
	}
}

FBathhouseSpaceSnapshot FBathhouseSpaceLayout::WithExpansionCount(const FBathhouseSpaceSnapshot& Snapshot, const int32 Count)
{
	FBathhouseSpaceSnapshot Copy = Snapshot;
	Copy.ExpansionCount = FMath::Clamp(Count, 0, Snapshot.Steps.Num());
	Copy.Interior = ExpandInterior(Snapshot.BaseInterior, Snapshot.Steps, Copy.ExpansionCount);
	return Copy;
}
