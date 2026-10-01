#include "Shop/ShopUnboxingCluster.h"

namespace
{
constexpr float AxisEpsilon = 1.0e-4f;

FVector GetLocalXAxis(const float YawDegrees)
{
	const float Radians = FMath::DegreesToRadians(YawDegrees);
	return FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.0f);
}

FVector GetLocalYAxis(const float YawDegrees)
{
	const float Radians = FMath::DegreesToRadians(YawDegrees);
	return FVector(-FMath::Sin(Radians), FMath::Cos(Radians), 0.0f);
}

float GetProjectedHalfExtent(
	const FVector& HalfExtent,
	const float YawDegrees,
	const FVector& Axis)
{
	return FMath::Abs(FVector::DotProduct(GetLocalXAxis(YawDegrees), Axis)) * HalfExtent.X
		+ FMath::Abs(FVector::DotProduct(GetLocalYAxis(YawDegrees), Axis)) * HalfExtent.Y
		+ FMath::Abs(Axis.Z) * HalfExtent.Z;
}

void AddBoxAxes(const float YawDegrees, TArray<FVector, TInlineAllocator<5>>& Axes)
{
	Axes.Add(GetLocalXAxis(YawDegrees));
	Axes.Add(GetLocalYAxis(YawDegrees));
}

float GetPairTargetDepth(
	const FVector& HalfExtentA,
	const FVector& HalfExtentB,
	const float RequestedDepthCm,
	const float ExtentRatio)
{
	return FMath::Min(RequestedDepthCm, ExtentRatio * FMath::Min(HalfExtentA.GetMin(), HalfExtentB.GetMin()));
}

bool IsValidHalfExtent(const FVector& HalfExtent)
{
	return !HalfExtent.ContainsNaN() && HalfExtent.GetMin() > KINDA_SMALL_NUMBER;
}

FShopUnboxClusterItem MakeCandidate(
	const FShopUnboxClusterItem& Anchor,
	const FVector& Direction,
	const float YawDegrees,
	const FVector& AnchorExtent,
	const FVector& CandidateExtent,
	const float TargetDepthCm)
{
	TArray<FVector, TInlineAllocator<5>> Axes;
	Axes.Reserve(5);
	AddBoxAxes(Anchor.YawDegrees, Axes);
	AddBoxAxes(YawDegrees, Axes);
	Axes.Add(FVector::UpVector);

	float TravelDistance = TNumericLimits<float>::Max();
	for (const FVector& Axis : Axes)
	{
		const float DirectionProjection = FMath::Abs(FVector::DotProduct(Direction, Axis));
		if (DirectionProjection <= AxisEpsilon)
		{
			continue;
		}
		const float Radius = GetProjectedHalfExtent(AnchorExtent, Anchor.YawDegrees, Axis)
			+ GetProjectedHalfExtent(CandidateExtent, YawDegrees, Axis);
		TravelDistance = FMath::Min(TravelDistance, (Radius - TargetDepthCm) / DirectionProjection);
	}

	FShopUnboxClusterItem Result;
	Result.Center = Anchor.Center + Direction * TravelDistance;
	Result.YawDegrees = YawDegrees;
	return Result;
}
}

float FShopUnboxingCluster::ComputePenetrationDepth(
	const FVector& CenterA,
	const float YawA,
	const FVector& HalfExtentA,
	const FVector& CenterB,
	const float YawB,
	const FVector& HalfExtentB)
{
	TArray<FVector, TInlineAllocator<5>> Axes;
	Axes.Reserve(5);
	AddBoxAxes(YawA, Axes);
	AddBoxAxes(YawB, Axes);
	Axes.Add(FVector::UpVector);

	const FVector CenterDelta = CenterB - CenterA;
	float MinimumOverlap = TNumericLimits<float>::Max();
	for (const FVector& Axis : Axes)
	{
		const float Radius = GetProjectedHalfExtent(HalfExtentA, YawA, Axis)
			+ GetProjectedHalfExtent(HalfExtentB, YawB, Axis);
		const float Overlap = Radius - FMath::Abs(FVector::DotProduct(CenterDelta, Axis));
		MinimumOverlap = FMath::Min(MinimumOverlap, Overlap);
	}
	return MinimumOverlap;
}

bool FShopUnboxingCluster::BuildLayout(
	const TArray<FVector>& HalfExtents,
	const float DepthCm,
	const FShopUnboxClusterTuning& Tuning,
	FRandomStream& RandomStream,
	TArray<FShopUnboxClusterItem>& OutItems)
{
	OutItems.Reset();
	if (HalfExtents.IsEmpty() || !FMath::IsFinite(DepthCm) || DepthCm < 0.0f)
	{
		return false;
	}
	for (const FVector& HalfExtent : HalfExtents)
	{
		if (!IsValidHalfExtent(HalfExtent))
		{
			return false;
		}
	}

	const float RequestedDepthCm = DepthCm;
	TArray<int32> ShuffledIndices;
	ShuffledIndices.Reserve(HalfExtents.Num());
	for (int32 Index = 0; Index < HalfExtents.Num(); ++Index)
	{
		ShuffledIndices.Add(Index);
	}
	for (int32 Index = ShuffledIndices.Num() - 1; Index > 0; --Index)
	{
		ShuffledIndices.Swap(Index, RandomStream.RandHelper(Index + 1));
	}

	TArray<FShopUnboxClusterItem> PlacedItems;
	PlacedItems.Reserve(HalfExtents.Num());
	TArray<int32> PlacedIndices;
	PlacedIndices.Reserve(HalfExtents.Num());

	const int32 FirstIndex = ShuffledIndices[0];
	FShopUnboxClusterItem FirstItem;
	FirstItem.YawDegrees = RandomStream.FRand() * 360.0f;
	PlacedItems.Add(FirstItem);
	PlacedIndices.Add(FirstIndex);

	for (int32 OrderIndex = 1; OrderIndex < ShuffledIndices.Num(); ++OrderIndex)
	{
		const int32 CandidateIndex = ShuffledIndices[OrderIndex];
		const FVector& CandidateExtent = HalfExtents[CandidateIndex];
		bool bPlaced = false;

		for (int32 Attempt = 0; Attempt < Tuning.PlacementAttempts; ++Attempt)
		{
			const int32 AnchorIndex = RandomStream.RandHelper(PlacedItems.Num());
			const float AzimuthRadians = FMath::DegreesToRadians(RandomStream.FRand() * 360.0f);
			const float ElevationDegrees = RandomStream.FRandRange(
				Tuning.MinElevationDegrees,
				Tuning.MaxElevationDegrees);
			const float ElevationSign = RandomStream.FRand() < 0.5f ? -1.0f : 1.0f;
			const float ElevationRadians = FMath::DegreesToRadians(ElevationDegrees * ElevationSign);
			const float HorizontalMagnitude = FMath::Cos(ElevationRadians);
			const FVector Direction(
				HorizontalMagnitude * FMath::Cos(AzimuthRadians),
				HorizontalMagnitude * FMath::Sin(AzimuthRadians),
				FMath::Sin(ElevationRadians));
			const float YawDegrees = RandomStream.FRand() * 360.0f;
			const FShopUnboxClusterItem Candidate = MakeCandidate(
				PlacedItems[AnchorIndex],
				Direction,
				YawDegrees,
				HalfExtents[PlacedIndices[AnchorIndex]],
				CandidateExtent,
				GetPairTargetDepth(
					HalfExtents[PlacedIndices[AnchorIndex]],
					CandidateExtent,
					RequestedDepthCm,
					Tuning.PairDepthExtentRatio));
			if (Candidate.Center.ContainsNaN())
			{
				continue;
			}

			bool bWithinPairDepthLimits = true;
			for (int32 ExistingIndex = 0; ExistingIndex < PlacedItems.Num(); ++ExistingIndex)
			{
				const float PairLimit = GetPairTargetDepth(
					HalfExtents[CandidateIndex],
					HalfExtents[PlacedIndices[ExistingIndex]],
					RequestedDepthCm,
					Tuning.PairDepthExtentRatio) + Tuning.DepthToleranceCm;
				const float PenetrationDepth = ComputePenetrationDepth(
					Candidate.Center,
					Candidate.YawDegrees,
					CandidateExtent,
					PlacedItems[ExistingIndex].Center,
					PlacedItems[ExistingIndex].YawDegrees,
					HalfExtents[PlacedIndices[ExistingIndex]]);
				if (PenetrationDepth > PairLimit + 0.01f)
				{
					bWithinPairDepthLimits = false;
					break;
				}
			}
			if (!bWithinPairDepthLimits)
			{
				continue;
			}

			PlacedItems.Add(Candidate);
			PlacedIndices.Add(CandidateIndex);
			bPlaced = true;
			break;
		}

		if (!bPlaced)
		{
			return false;
		}
	}

	OutItems.SetNum(HalfExtents.Num());
	for (int32 Index = 0; Index < PlacedItems.Num(); ++Index)
	{
		OutItems[PlacedIndices[Index]] = PlacedItems[Index];
	}
	return true;
}
