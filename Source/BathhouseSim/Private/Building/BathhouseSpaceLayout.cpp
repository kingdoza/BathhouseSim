#include "Building/BathhouseSpaceLayout.h"

namespace
{
	constexpr double LayoutTolerance = UE_DOUBLE_KINDA_SMALL_NUMBER;

	bool HasArea(const FBox2D& Rect)
	{
		return Rect.Max.X - Rect.Min.X > LayoutTolerance && Rect.Max.Y - Rect.Min.Y > LayoutTolerance;
	}

	void AddIfArea(TArray<FBox2D>& Out, const FBox2D& Rect)
	{
		if (HasArea(Rect))
		{
			Out.Add(Rect);
		}
	}

	FBathhouseBoxPart MakeBox(const FBox2D& Rect, const double Z0, const double Z1)
	{
		FBathhouseBoxPart Part;
		Part.Center = FVector(
			(Rect.Min.X + Rect.Max.X) * 0.5, (Rect.Min.Y + Rect.Max.Y) * 0.5, (Z0 + Z1) * 0.5);
		Part.HalfExtent = FVector(
			(Rect.Max.X - Rect.Min.X) * 0.5, (Rect.Max.Y - Rect.Min.Y) * 0.5, (Z1 - Z0) * 0.5);
		return Part;
	}

	bool IsBuildableStair(
		const TArray<FBathhouseSpaceSnapshot>& Snapshots,
		const int32 UpperIndex,
		const FBathhouseStairSnapshot& Stair)
	{
		if (!Snapshots.IsValidIndex(Stair.LowerIndex) || Stair.LowerIndex == UpperIndex)
		{
			return false;
		}
		const FBathhouseSpaceSnapshot& Upper = Snapshots[UpperIndex];
		const FBathhouseSpaceSnapshot& Lower = Snapshots[Stair.LowerIndex];
		return Stair.WidthCm > 0.0 && Stair.RunCm > 0.0 && Stair.StepCount > 0
			&& Lower.FloorZ < Upper.FloorZ - LayoutTolerance;
	}

	bool HasValidInterior(const FBathhouseSpaceSnapshot& Space)
	{
		return Space.Interior.Max.X - Space.Interior.Min.X > LayoutTolerance
			&& Space.Interior.Max.Y - Space.Interior.Min.Y > LayoutTolerance
			&& Space.CeilingHeightCm > LayoutTolerance;
	}

	struct FWallHole
	{
		FVector2D Interval = FVector2D::ZeroVector;
		double Height = 0.0;
	};
}

FBox2D FBathhouseStairFrame::ToWorldRect(const double X0, const double X1, const double Y0, const double Y1) const
{
	const FVector2D A = ToWorld(X0, Y0);
	const FVector2D B = ToWorld(X1, Y1);
	return FBox2D(
		FVector2D(FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y)),
		FVector2D(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y)));
}

FVector2D FBathhouseSpaceLayout::SideNormal(const EBathhouseSpaceSide Side)
{
	switch (Side)
	{
	case EBathhouseSpaceSide::East: return FVector2D(1.0, 0.0);
	case EBathhouseSpaceSide::West: return FVector2D(-1.0, 0.0);
	case EBathhouseSpaceSide::North: return FVector2D(0.0, 1.0);
	default: return FVector2D(0.0, -1.0);
	}
}

EBathhouseSpaceSide FBathhouseSpaceLayout::Opposite(const EBathhouseSpaceSide Side)
{
	switch (Side)
	{
	case EBathhouseSpaceSide::East: return EBathhouseSpaceSide::West;
	case EBathhouseSpaceSide::West: return EBathhouseSpaceSide::East;
	case EBathhouseSpaceSide::North: return EBathhouseSpaceSide::South;
	default: return EBathhouseSpaceSide::North;
	}
}

FBox2D FBathhouseSpaceLayout::OuterRect(const FBathhouseSpaceSnapshot& Space, const double WallThicknessCm)
{
	return FBox2D(
		Space.Interior.Min - FVector2D(WallThicknessCm, WallThicknessCm),
		Space.Interior.Max + FVector2D(WallThicknessCm, WallThicknessCm));
}

FVector2D FBathhouseSpaceLayout::OpeningWorldInterval(
	const FBathhouseSpaceSnapshot& Space, const FBathhouseOpeningSnapshot& Opening)
{
	const double Base = IsAlongY(Opening.Side) ? Space.ActorXY.Y : Space.ActorXY.X;
	const double Center = Base + Opening.CenterOffsetCm;
	return FVector2D(Center - Opening.WidthCm * 0.5, Center + Opening.WidthCm * 0.5);
}

FBathhouseStairFrame FBathhouseSpaceLayout::MakeStairFrame(
	const FBathhouseSpaceSnapshot& Upper, const FBathhouseStairSnapshot& Stair)
{
	FBathhouseStairFrame Frame;
	Frame.Origin = Upper.ActorXY + Stair.TopEdgeOffsetCm;
	Frame.Dir = SideNormal(Stair.DownSide);
	Frame.Perp = FVector2D(-Frame.Dir.Y, Frame.Dir.X);
	return Frame;
}

FBox2D FBathhouseSpaceLayout::StairHole(
	const FBathhouseSpaceSnapshot& Upper, const FBathhouseStairSnapshot& Stair)
{
	return MakeStairFrame(Upper, Stair).ToWorldRect(
		0.0, Stair.RunCm, -Stair.WidthCm * 0.5, Stair.WidthCm * 0.5);
}

void FBathhouseSpaceLayout::SubtractRects(
	const FBox2D& Base, const TArray<FBox2D>& Holes, TArray<FBox2D>& OutRects)
{
	OutRects.Reset();
	if (!HasArea(Base))
	{
		return;
	}
	OutRects.Add(Base);
	for (const FBox2D& Hole : Holes)
	{
		TArray<FBox2D> Next;
		for (const FBox2D& Rect : OutRects)
		{
			const FVector2D IMin(FMath::Max(Rect.Min.X, Hole.Min.X), FMath::Max(Rect.Min.Y, Hole.Min.Y));
			const FVector2D IMax(FMath::Min(Rect.Max.X, Hole.Max.X), FMath::Min(Rect.Max.Y, Hole.Max.Y));
			if (IMax.X - IMin.X <= LayoutTolerance || IMax.Y - IMin.Y <= LayoutTolerance)
			{
				Next.Add(Rect);
				continue;
			}
			AddIfArea(Next, FBox2D(Rect.Min, FVector2D(IMin.X, Rect.Max.Y)));
			AddIfArea(Next, FBox2D(FVector2D(IMax.X, Rect.Min.Y), Rect.Max));
			AddIfArea(Next, FBox2D(FVector2D(IMin.X, Rect.Min.Y), FVector2D(IMax.X, IMin.Y)));
			AddIfArea(Next, FBox2D(FVector2D(IMin.X, IMax.Y), FVector2D(IMax.X, Rect.Max.Y)));
		}
		OutRects = MoveTemp(Next);
	}
}

void FBathhouseSpaceLayout::SplitChunks(const FBox2D& Rect, const FVector2D& MaxSize, TArray<FBox2D>& OutRects)
{
	OutRects.Reset();
	if (!HasArea(Rect) || MaxSize.X <= 0.0 || MaxSize.Y <= 0.0)
	{
		return;
	}
	const double SizeX = Rect.Max.X - Rect.Min.X;
	const double SizeY = Rect.Max.Y - Rect.Min.Y;
	const int32 CountX = FMath::Max(1, FMath::CeilToInt(SizeX / MaxSize.X - LayoutTolerance));
	const int32 CountY = FMath::Max(1, FMath::CeilToInt(SizeY / MaxSize.Y - LayoutTolerance));
	const double CellX = SizeX / CountX;
	const double CellY = SizeY / CountY;
	for (int32 Y = 0; Y < CountY; ++Y)
	{
		for (int32 X = 0; X < CountX; ++X)
		{
			const FVector2D Min(Rect.Min.X + CellX * X, Rect.Min.Y + CellY * Y);
			OutRects.Add(FBox2D(Min, Min + FVector2D(CellX, CellY)));
		}
	}
}

void FBathhouseSpaceLayout::SplitLightCenters(const FBox2D& Rect, const double SpacingCm, TArray<FVector2D>& OutCenters)
{
	OutCenters.Reset();
	if (!HasArea(Rect) || SpacingCm <= 0.0)
	{
		return;
	}
	const double SizeX = Rect.Max.X - Rect.Min.X;
	const double SizeY = Rect.Max.Y - Rect.Min.Y;
	const int32 CountX = FMath::Max(1, FMath::CeilToInt(SizeX / SpacingCm - LayoutTolerance));
	const int32 CountY = FMath::Max(1, FMath::CeilToInt(SizeY / SpacingCm - LayoutTolerance));
	for (int32 Y = 0; Y < CountY; ++Y)
	{
		for (int32 X = 0; X < CountX; ++X)
		{
			OutCenters.Add(FVector2D(
				Rect.Min.X + SizeX / CountX * (X + 0.5),
				Rect.Min.Y + SizeY / CountY * (Y + 0.5)));
		}
	}
}

FBathhouseSpacePlan FBathhouseSpaceLayout::BuildPlan(
	const TArray<FBathhouseSpaceSnapshot>& Snapshots, const int32 Index, const FBathhouseLayoutValues& Values)
{
	FBathhouseSpacePlan Plan;
	if (!Snapshots.IsValidIndex(Index) || Values.WallThicknessCm <= 0.0 || Values.SlabThicknessCm <= 0.0)
	{
		return Plan;
	}
	const FBathhouseSpaceSnapshot& Space = Snapshots[Index];
	if (!HasValidInterior(Space))
	{
		return Plan;
	}
	const double T = Values.WallThicknessCm;
	const double S = Values.SlabThicknessCm;
	const double Zf = Space.FloorZ;
	const double Zc = CeilingZ(Space);
	const FBox2D Outer = OuterRect(Space, T);
	const FBox2D& Inner = Space.Interior;

	// 바닥: 이 공간이 위층인 계단 구멍을 뺀다.
	{
		TArray<FBox2D> Holes;
		for (const FBathhouseStairSnapshot& Stair : Space.Stairs)
		{
			if (IsBuildableStair(Snapshots, Index, Stair))
			{
				Holes.Add(StairHole(Space, Stair));
			}
		}
		TArray<FBox2D> Rects;
		SubtractRects(Outer, Holes, Rects);
		for (const FBox2D& Rect : Rects)
		{
			Plan.Get(EBathhouseShellPart::Floor).Add(MakeBox(Rect, Zf - S, Zf));
		}
	}

	// 천장: 이 공간이 아래층인 계단 구멍을 뺀다.
	{
		TArray<FBox2D> Holes;
		for (int32 OtherIndex = 0; OtherIndex < Snapshots.Num(); ++OtherIndex)
		{
			for (const FBathhouseStairSnapshot& Stair : Snapshots[OtherIndex].Stairs)
			{
				if (Stair.LowerIndex == Index && IsBuildableStair(Snapshots, OtherIndex, Stair))
				{
					Holes.Add(StairHole(Snapshots[OtherIndex], Stair));
				}
			}
		}
		TArray<FBox2D> Rects;
		SubtractRects(Outer, Holes, Rects);
		for (const FBox2D& Rect : Rects)
		{
			Plan.Get(EBathhouseShellPart::Ceiling).Add(MakeBox(Rect, Zc, Zc + S));
		}
	}

	// 벽: 동·서 벽은 Y로 O 전체, 남·북 벽은 X로 I 길이(모서리 겹침 없음).
	for (const EBathhouseSpaceSide Side : {
		EBathhouseSpaceSide::East, EBathhouseSpaceSide::West,
		EBathhouseSpaceSide::North, EBathhouseSpaceSide::South })
	{
		const bool bAlongY = IsAlongY(Side);
		const double AlongMin = bAlongY ? Outer.Min.Y : Inner.Min.X;
		const double AlongMax = bAlongY ? Outer.Max.Y : Inner.Max.X;
		double NormalMin = 0.0;
		double NormalMax = 0.0;
		switch (Side)
		{
		case EBathhouseSpaceSide::East: NormalMin = Inner.Max.X; NormalMax = Inner.Max.X + T; break;
		case EBathhouseSpaceSide::West: NormalMin = Inner.Min.X - T; NormalMax = Inner.Min.X; break;
		case EBathhouseSpaceSide::North: NormalMin = Inner.Max.Y; NormalMax = Inner.Max.Y + T; break;
		default: NormalMin = Inner.Min.Y - T; NormalMax = Inner.Min.Y; break;
		}

		TArray<FWallHole> WallHoles;
		for (const FBathhouseOpeningSnapshot& Opening : Space.Openings)
		{
			if (Opening.Side == Side)
			{
				WallHoles.Add({ OpeningWorldInterval(Space, Opening), Opening.HeightCm });
			}
		}
		for (int32 OtherIndex = 0; OtherIndex < Snapshots.Num(); ++OtherIndex)
		{
			if (OtherIndex == Index)
			{
				continue;
			}
			for (const FBathhouseOpeningSnapshot& Opening : Snapshots[OtherIndex].Openings)
			{
				if (Opening.ConnectedIndex == Index && Opposite(Opening.Side) == Side)
				{
					WallHoles.Add({ OpeningWorldInterval(Snapshots[OtherIndex], Opening), Opening.HeightCm });
				}
			}
		}

		// (벽 길이 방향, Z) 평면에서 개구부를 뺀다.
		TArray<FBox2D> Holes;
		for (const FWallHole& Hole : WallHoles)
		{
			if (Hole.Interval.Y - Hole.Interval.X > LayoutTolerance && Hole.Height > LayoutTolerance)
			{
				Holes.Add(FBox2D(FVector2D(Hole.Interval.X, Zf), FVector2D(Hole.Interval.Y, FMath::Min(Zf + Hole.Height, Zc))));
			}
		}
		TArray<FBox2D> Pieces;
		SubtractRects(FBox2D(FVector2D(AlongMin, Zf), FVector2D(AlongMax, Zc)), Holes, Pieces);
		for (const FBox2D& Piece : Pieces)
		{
			const FBox2D PlanRect = bAlongY
				? FBox2D(FVector2D(NormalMin, Piece.Min.X), FVector2D(NormalMax, Piece.Max.X))
				: FBox2D(FVector2D(Piece.Min.X, NormalMin), FVector2D(Piece.Max.X, NormalMax));
			Plan.Get(EBathhouseShellPart::Wall).Add(MakeBox(PlanRect, Piece.Min.Y, Piece.Max.Y));
		}
	}

	// 계단(이 공간이 위층).
	for (const FBathhouseStairSnapshot& Stair : Space.Stairs)
	{
		if (!IsBuildableStair(Snapshots, Index, Stair))
		{
			continue;
		}
		const FBathhouseSpaceSnapshot& Lower = Snapshots[Stair.LowerIndex];
		const FBathhouseStairFrame Frame = MakeStairFrame(Space, Stair);
		const double W = Stair.WidthCm;
		const double Run = Stair.RunCm;
		const double ZUpper = Zf;
		const double ZLower = Lower.FloorZ;
		const double ZLowerCeiling = CeilingZ(Lower);
		const double Rise = ZUpper - ZLower;

		// 경사로: 윗면이 (0, ZUpper)와 (Run, ZLower)를 잇는 두께 S의 회전 상자.
		{
			const double SlopeLength = FMath::Sqrt(Run * Run + Rise * Rise);
			const double Cos = Run / SlopeLength;
			const double Sin = Rise / SlopeLength;
			const FVector SlopeX(Frame.Dir.X * Cos, Frame.Dir.Y * Cos, -Sin);
			const FVector SlopeY(Frame.Perp.X, Frame.Perp.Y, 0.0);
			const FVector SlopeZ = FVector::CrossProduct(SlopeX, SlopeY);
			const FVector2D MidXY = Frame.ToWorld(Run * 0.5, 0.0);
			const FVector TopCenter(MidXY.X, MidXY.Y, (ZUpper + ZLower) * 0.5);
			FBathhouseBoxPart Ramp;
			Ramp.Center = TopCenter - SlopeZ * (S * 0.5);
			Ramp.HalfExtent = FVector(SlopeLength * 0.5, W * 0.5, S * 0.5);
			Ramp.Rotation = FQuat(FMatrix(SlopeX, SlopeY, SlopeZ, FVector::ZeroVector));
			Plan.Get(EBathhouseShellPart::StairRamp).Add(Ramp);
		}

		// 계단 판: 윗면은 경사로의 판 중앙 높이.
		for (int32 StepIndex = 0; StepIndex < Stair.StepCount; ++StepIndex)
		{
			const double X0 = Run * StepIndex / Stair.StepCount;
			const double X1 = Run * (StepIndex + 1) / Stair.StepCount;
			const double Top = ZUpper - Rise * (StepIndex + 0.5) / Stair.StepCount;
			Plan.Get(EBathhouseShellPart::StairStep).Add(
				MakeBox(Frame.ToWorldRect(X0, X1, -W * 0.5, W * 0.5), ZLower, Top));
		}

		// 계단 벽(두께 T, 구멍 R 바깥).
		const double GuardTop = ZUpper + Stair.GuardHeightCm;
		Plan.Get(EBathhouseShellPart::StairWall).Add(
			MakeBox(Frame.ToWorldRect(0.0, Run, W * 0.5, W * 0.5 + T), ZLower, GuardTop));
		Plan.Get(EBathhouseShellPart::StairWall).Add(
			MakeBox(Frame.ToWorldRect(0.0, Run, -W * 0.5 - T, -W * 0.5), ZLower, GuardTop));
		Plan.Get(EBathhouseShellPart::StairWall).Add(
			MakeBox(Frame.ToWorldRect(-T, 0.0, -W * 0.5 - T, W * 0.5 + T), ZLower, ZUpper));
		Plan.Get(EBathhouseShellPart::StairWall).Add(
			MakeBox(Frame.ToWorldRect(Run, Run + T, -W * 0.5 - T, W * 0.5 + T), ZLowerCeiling, GuardTop));

		// 위층 구멍 막이.
		Plan.Get(EBathhouseShellPart::StairKeepClear).Add(
			MakeBox(Frame.ToWorldRect(0.0, Run, -W * 0.5, W * 0.5), ZUpper, Zc));
	}

	// 조명.
	{
		TArray<FVector2D> Centers;
		SplitLightCenters(Inner, Space.LightSpacingCm, Centers);
		for (const FVector2D& Center : Centers)
		{
			Plan.LightLocations.Add(FVector(Center.X, Center.Y, Zc - Space.LightCeilingOffsetCm));
		}
	}

	SplitChunks(Inner, Values.ChunkMaxSizeCm, Plan.ChunkRects);
	return Plan;
}

void FBathhouseSpaceLayout::Build(
	const TArray<FBathhouseSpaceSnapshot>& Snapshots,
	const FBathhouseLayoutValues& Values,
	TArray<FBathhouseSpacePlan>& OutPlans)
{
	OutPlans.Reset();
	for (int32 Index = 0; Index < Snapshots.Num(); ++Index)
	{
		OutPlans.Add(BuildPlan(Snapshots, Index, Values));
	}
}
