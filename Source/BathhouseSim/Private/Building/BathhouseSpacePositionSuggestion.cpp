#include "Building/BathhouseSpaceValidation.h"

#include "Building/BathhouseSpaceValidationInternal.h"

namespace BathhouseSpaceValidationDetail
{
	struct FMove
	{
		int32 Index = INDEX_NONE;
		FVector2D DeltaXY = FVector2D::ZeroVector;
		double DeltaZ = 0.0;
	};

	int32 CountCode(const TArray<FBathhouseLayoutProblem>& Problems, const EBathhouseProblemCode Code)
	{
		int32 Count = 0;
		for (const FBathhouseLayoutProblem& Problem : Problems)
		{
			Count += Problem.Code == Code ? 1 : 0;
		}
		return Count;
	}

	bool HasSameProblem(const TArray<FBathhouseLayoutProblem>& Problems, const FBathhouseLayoutProblem& Original)
	{
		for (const FBathhouseLayoutProblem& Problem : Problems)
		{
			if (Problem.Code == Original.Code && Problem.OwnerIndex == Original.OwnerIndex
				&& Problem.OtherIndex == Original.OtherIndex && Problem.ItemIndex == Original.ItemIndex)
			{
				return true;
			}
		}
		return false;
	}

	FString DescribeMove(const TArray<FBathhouseSpaceSnapshot>& S, const FMove& Move)
	{
		const FBathhouseSpaceSnapshot& Space = S[Move.Index];
		TArray<FString> Parts;
		if (!FMath::IsNearlyZero(Move.DeltaXY.X, Tol))
		{
			Parts.Add(FString::Printf(TEXT("X를 %s로"), *FormatCm(Space.ActorXY.X + Move.DeltaXY.X)));
		}
		if (!FMath::IsNearlyZero(Move.DeltaXY.Y, Tol))
		{
			Parts.Add(FString::Printf(TEXT("Y를 %s로"), *FormatCm(Space.ActorXY.Y + Move.DeltaXY.Y)));
		}
		if (!FMath::IsNearlyZero(Move.DeltaZ, Tol))
		{
			Parts.Add(FString::Printf(TEXT("Z를 %s로"), *FormatCm(Space.FloorZ + Move.DeltaZ)));
		}
		return FString::Printf(TEXT("%s Location %s"), *Space.DisplayName, *FString::Join(Parts, TEXT(", ")));
	}

	/** 후보 이동을 적용해 같은 검증을 다시 돌리고 그 오류가 사라지며 새 겹침이 없을 때만 true. */
	bool MoveFixesProblem(
		const TArray<FBathhouseSpaceSnapshot>& S,
		const FBathhouseValidationInputs& In,
		const FBathhouseLayoutProblem& Problem,
		const int32 OverlapsBefore,
		const FMove& Move)
	{
		if (!S.IsValidIndex(Move.Index)
			|| (Move.DeltaXY.IsNearlyZero(Tol) && FMath::IsNearlyZero(Move.DeltaZ, Tol)))
		{
			return false;
		}
		TArray<FBathhouseSpaceSnapshot> Moved = S;
		FBathhouseSpaceValidation::ApplyMove(Moved[Move.Index], Move.DeltaXY, Move.DeltaZ);
		TArray<FBathhouseLayoutProblem> After;
		CollectProblems(Moved, In, After);
		const int32 Allowed = OverlapsBefore - (Problem.Code == EBathhouseProblemCode::Overlap ? 1 : 0);
		return !HasSameProblem(After, Problem) && CountCode(After, EBathhouseProblemCode::Overlap) <= Allowed;
	}

	void CollectCandidates(
		const TArray<FBathhouseSpaceSnapshot>& S,
		const FBathhouseValidationInputs& In,
		const FBathhouseLayoutProblem& Problem,
		TArray<FMove>& OutCandidates)
	{
		const double T = In.Layout.WallThicknessCm;
		const int32 Owner = Problem.OwnerIndex;
		const int32 Other = Problem.OtherIndex;
		if (!S.IsValidIndex(Owner) || !S.IsValidIndex(Other))
		{
			return;
		}

		auto PassageCandidates = [&](const int32 A, const int32 B, const FBathhouseOpeningSnapshot& Opening)
		{
			FBathhouseLocationSuggestion MoveB, MoveA;
			const bool bAlongY = FBathhouseSpaceLayout::IsAlongY(Opening.Side);
			// B를 옮기는 값: 맞닿음 축 + 구간이 B 벽 안쪽에 들어오는 벽 길이 방향 값 + 바닥 Z.
			FMove B_Move;
			B_Move.Index = B;
			if (FBathhouseSpaceValidation::SuggestTouchingLocation(S, A, B, Opening.Side, T, true, MoveB))
			{
				B_Move.DeltaXY = MoveB.DeltaXY;
			}
			const FVector2D Interval = FBathhouseSpaceLayout::OpeningWorldInterval(S[A], Opening);
			const double TargetMin = AlongMin(S[B], FBathhouseSpaceLayout::Opposite(Opening.Side));
			const double TargetMax = AlongMax(S[B], FBathhouseSpaceLayout::Opposite(Opening.Side));
			const double Lo = Interval.Y - TargetMax;
			const double Hi = Interval.X - TargetMin;
			if (Lo <= Hi + Tol && (Lo > Tol || Hi < -Tol))
			{
				const double AlongDelta = FMath::Clamp(0.0, Lo, Hi);
				(bAlongY ? B_Move.DeltaXY.Y : B_Move.DeltaXY.X) += AlongDelta;
			}
			B_Move.DeltaZ = S[A].FloorZ - S[B].FloorZ;
			OutCandidates.Add(B_Move);
			if (FBathhouseSpaceValidation::SuggestTouchingLocation(S, A, B, Opening.Side, T, false, MoveA))
			{
				FMove A_Move;
				A_Move.Index = A;
				A_Move.DeltaXY = MoveA.DeltaXY;
				OutCandidates.Add(A_Move);
			}
		};

		if (Problem.Code == EBathhouseProblemCode::Overlap)
		{
			// 통로가 있는 쌍은 맞닿음 제안을 쓴다.
			for (const int32 A : { Owner, Other })
			{
				const int32 B = A == Owner ? Other : Owner;
				for (const FBathhouseOpeningSnapshot& Opening : S[A].Openings)
				{
					if (Opening.ConnectedIndex == B)
					{
						PassageCandidates(A, B, Opening);
						return;
					}
				}
			}
			const FBox2D OA = FBathhouseSpaceLayout::OuterRect(S[Owner], T);
			const FBox2D OB = FBathhouseSpaceLayout::OuterRect(S[Other], T);
			double ZMinA, ZMaxA, ZMinB, ZMaxB;
			VolumeZ(S[Owner], In.Layout.SlabThicknessCm, ZMinA, ZMaxA);
			VolumeZ(S[Other], In.Layout.SlabThicknessCm, ZMinB, ZMaxB);
			const double OverlapX = RectOverlap(OA.Min.X, OA.Max.X, OB.Min.X, OB.Max.X);
			const double OverlapY = RectOverlap(OA.Min.Y, OA.Max.Y, OB.Min.Y, OB.Max.Y);
			const double OverlapZ = RectOverlap(ZMinA, ZMaxA, ZMinB, ZMaxB);
			// 겹침이 가장 작은 축으로 각 공간을 뗀다.
			FVector2D Axis(0.0, 0.0);
			double Amount = OverlapX;
			bool bZ = false;
			double SignOwner = (OA.Min.X + OA.Max.X) <= (OB.Min.X + OB.Max.X) ? -1.0 : 1.0;
			Axis = FVector2D(1.0, 0.0);
			if (OverlapY < Amount)
			{
				Amount = OverlapY;
				Axis = FVector2D(0.0, 1.0);
				SignOwner = (OA.Min.Y + OA.Max.Y) <= (OB.Min.Y + OB.Max.Y) ? -1.0 : 1.0;
			}
			if (OverlapZ < Amount)
			{
				Amount = OverlapZ;
				bZ = true;
				SignOwner = (ZMinA + ZMaxA) <= (ZMinB + ZMaxB) ? -1.0 : 1.0;
			}
			FMove OwnerMove, OtherMove;
			OwnerMove.Index = Owner;
			OtherMove.Index = Other;
			if (bZ)
			{
				OwnerMove.DeltaZ = SignOwner * Amount;
				OtherMove.DeltaZ = -SignOwner * Amount;
			}
			else
			{
				OwnerMove.DeltaXY = Axis * (SignOwner * Amount);
				OtherMove.DeltaXY = Axis * (-SignOwner * Amount);
			}
			OutCandidates.Add(OwnerMove);
			OutCandidates.Add(OtherMove);
			return;
		}

		if (S[Owner].Openings.IsValidIndex(Problem.ItemIndex))
		{
			PassageCandidates(Owner, Other, S[Owner].Openings[Problem.ItemIndex]);
		}
	}

	void AppendSuggestions(
		const TArray<FBathhouseSpaceSnapshot>& S, const FBathhouseValidationInputs& In, TArray<FBathhouseLayoutProblem>& Problems)
	{
		const int32 OverlapsBefore = CountCode(Problems, EBathhouseProblemCode::Overlap);
		for (FBathhouseLayoutProblem& Problem : Problems)
		{
			if (Problem.Code != EBathhouseProblemCode::Overlap
				&& Problem.Code != EBathhouseProblemCode::PassageNotTouching
				&& Problem.Code != EBathhouseProblemCode::PassageFloorZ
				&& Problem.Code != EBathhouseProblemCode::PassageRangeOutside)
			{
				continue;
			}
			TArray<FMove> Candidates;
			CollectCandidates(S, In, Problem, Candidates);
			TArray<FString> Descriptions;
			for (const FMove& Move : Candidates)
			{
				if (MoveFixesProblem(S, In, Problem, OverlapsBefore, Move))
				{
					Descriptions.Add(DescribeMove(S, Move));
				}
			}
			if (!Descriptions.IsEmpty())
			{
				Problem.Message = FText::FromString(FString::Printf(TEXT("%s 제안: %s로 옮기면 해결됩니다."),
					*Problem.Message.ToString(), *FString::Join(Descriptions, TEXT(" 또는 "))));
			}
		}
	}
}

void FBathhouseSpaceValidation::ApplyMove(FBathhouseSpaceSnapshot& Snapshot, const FVector2D& DeltaXY, const double DeltaZ)
{
	using namespace BathhouseSpaceValidationDetail;
	Snapshot.ActorXY += DeltaXY;
	Snapshot.Interior = FBox2D(Snapshot.Interior.Min + DeltaXY, Snapshot.Interior.Max + DeltaXY);
	Snapshot.FloorZ += DeltaZ;
}

bool FBathhouseSpaceValidation::SuggestTouchingLocation(
	const TArray<FBathhouseSpaceSnapshot>& Snapshots,
	const int32 AIndex,
	const int32 BIndex,
	const EBathhouseSpaceSide Side,
	const double WallThicknessCm,
	const bool bMoveB,
	FBathhouseLocationSuggestion& OutSuggestion)
{
	using namespace BathhouseSpaceValidationDetail;
	OutSuggestion = FBathhouseLocationSuggestion();
	if (!Snapshots.IsValidIndex(AIndex) || !Snapshots.IsValidIndex(BIndex) || AIndex == BIndex)
	{
		return false;
	}
	const FBathhouseSpaceSnapshot& A = Snapshots[AIndex];
	const FBathhouseSpaceSnapshot& B = Snapshots[BIndex];
	const double EdgeA = EdgeCoordinate(FBathhouseSpaceLayout::OuterRect(A, WallThicknessCm), Side);
	const double EdgeB = EdgeCoordinate(FBathhouseSpaceLayout::OuterRect(B, WallThicknessCm), FBathhouseSpaceLayout::Opposite(Side));
	const double Gap = EdgeA - EdgeB;
	const double Delta = bMoveB ? Gap : -Gap;
	const bool bNormalX = FBathhouseSpaceLayout::IsAlongY(Side); // 동·서 변은 벽이 Y로 뻗고 법선이 X다.
	const FBathhouseSpaceSnapshot& Moved = bMoveB ? B : A;
	OutSuggestion.MoveIndex = bMoveB ? BIndex : AIndex;
	OutSuggestion.DeltaXY = bNormalX ? FVector2D(Delta, 0.0) : FVector2D(0.0, Delta);
	OutSuggestion.NewLocation = FVector(Moved.ActorXY.X + OutSuggestion.DeltaXY.X, Moved.ActorXY.Y + OutSuggestion.DeltaXY.Y, Moved.FloorZ);
	return true;
}

