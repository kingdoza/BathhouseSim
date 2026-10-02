#include "Building/BathhouseSpaceValidation.h"

#include "Building/BathhouseSpaceValidationInternal.h"

// 공간 넓힘 검사(순수). BathhouseSpaceValidation.cpp는 크기 한도 때문에 건드리지 않는다.
// 0회와 목록 끝까지 넓힌 모습만 본다. 넓힌 모습은 커지기만 하므로 끝 모습 검사가 모든 중간 조합을 덮는다.

namespace
{
	bool IsExpansionOutsideOpening(const FBathhouseOpeningSnapshot& Opening)
	{
		return Opening.ConnectedIndex == INDEX_NONE && !Opening.ConnectedActor.IsValid();
	}

	/** 두 공간이 0회 바깥 직사각형에서 A의 Side 변으로 맞닿는지(부피 Z 구간·변 좌표·변 길이 방향 겹침). */
	bool TouchesOnExpansionSide(
		const FBathhouseSpaceSnapshot& A, const FBathhouseSpaceSnapshot& B, const EBathhouseSpaceSide Side,
		const FBathhouseLayoutValues& Values)
	{
		using namespace BathhouseSpaceValidationDetail;
		const FBox2D OuterA = FBathhouseSpaceLayout::OuterRect(A, Values.WallThicknessCm);
		const FBox2D OuterB = FBathhouseSpaceLayout::OuterRect(B, Values.WallThicknessCm);
		double MinA, MaxA, MinB, MaxB;
		VolumeZ(A, Values.SlabThicknessCm, MinA, MaxA);
		VolumeZ(B, Values.SlabThicknessCm, MinB, MaxB);
		if (!(RectOverlap(MinA, MaxA, MinB, MaxB) > Tol))
		{
			return false;
		}
		if (FMath::Abs(EdgeCoordinate(OuterA, Side) - EdgeCoordinate(OuterB, FBathhouseSpaceLayout::Opposite(Side))) > Tol)
		{
			return false;
		}
		const double Along = FBathhouseSpaceLayout::IsAlongY(Side)
			? RectOverlap(OuterA.Min.Y, OuterA.Max.Y, OuterB.Min.Y, OuterB.Max.Y)
			: RectOverlap(OuterA.Min.X, OuterA.Max.X, OuterB.Min.X, OuterB.Max.X);
		return Along > Tol;
	}
}

void FBathhouseSpaceValidation::ValidateExpansion(
	const TArray<FBathhouseSpaceSnapshot>& Snapshots,
	const TArray<FBox>& NavBoxes,
	const FBathhouseValidationInputs& Inputs,
	const int32 HallEffectRowCount,
	TArray<FBathhouseLayoutProblem>& OutProblems)
{
	using namespace BathhouseSpaceValidationDetail;
	TArray<FBathhouseSpaceSnapshot> Base;
	TArray<FBathhouseSpaceSnapshot> End;
	for (const FBathhouseSpaceSnapshot& Snapshot : Snapshots)
	{
		Base.Add(FBathhouseSpaceLayout::WithExpansionCount(Snapshot, 0));
		End.Add(FBathhouseSpaceLayout::WithExpansionCount(Snapshot, Snapshot.Steps.Num()));
	}
	const FBathhouseLayoutValues& Values = Inputs.Layout;

	int32 HallIndex = INDEX_NONE;
	for (int32 I = 0; I < Snapshots.Num(); ++I)
	{
		const FBathhouseSpaceSnapshot& Space = Snapshots[I];
		if (Space.Kind == EBathhouseSpaceKind::Hall && HallIndex == INDEX_NONE)
		{
			HallIndex = I;
		}
		for (int32 StepIndex = 0; StepIndex < Space.Steps.Num(); ++StepIndex)
		{
			const FBathhouseExpansionStepSnapshot& Step = Space.Steps[StepIndex];
			const FString Label = FString::Printf(TEXT("%s %d번째 넓힘 줄"), *Space.DisplayName, StepIndex + 1);
			if (Step.Sides.IsEmpty())
			{
				AddProblem(OutProblems, EBathhouseProblemCode::ExpansionSidesEmpty, EBathhouseProblemSeverity::Error,
					I, INDEX_NONE, StepIndex,
					FString::Printf(TEXT("%s에 물러날 벽이 없습니다."), *Label));
			}
			if (!(Step.Price > 0))
			{
				AddProblem(OutProblems, EBathhouseProblemCode::ExpansionPriceInvalid, EBathhouseProblemSeverity::Error,
					I, INDEX_NONE, StepIndex,
					FString::Printf(TEXT("%s의 가격이 0 이하입니다."), *Label));
			}
			TArray<EBathhouseSpaceSide, TInlineAllocator<4>> Seen;
			TArray<EBathhouseSpaceSide, TInlineAllocator<4>> ReportedDuplicate;
			for (const FBathhouseExpansionSideSnapshot& Wall : Step.Sides)
			{
				if (Seen.Contains(Wall.Side))
				{
					if (!ReportedDuplicate.Contains(Wall.Side))
					{
						ReportedDuplicate.Add(Wall.Side);
						AddProblem(OutProblems, EBathhouseProblemCode::ExpansionSideDuplicate, EBathhouseProblemSeverity::Error,
							I, INDEX_NONE, StepIndex,
							FString::Printf(TEXT("%s에 %s 벽이 두 번 있습니다. 같은 벽은 한 번만 적을 수 있습니다."),
								*Label, *SideName(Wall.Side)));
					}
					continue;
				}
				Seen.Add(Wall.Side);
				if (!FBathhouseSpaceLayout::IsUsableSide(Wall))
				{
					AddProblem(OutProblems, EBathhouseProblemCode::ExpansionAmountInvalid, EBathhouseProblemSeverity::Error,
						I, INDEX_NONE, StepIndex,
						FString::Printf(TEXT("%s의 %s 벽: 넓히는 양은 0보다 큰 유한한 값이어야 합니다."), *Label, *SideName(Wall.Side)));
					continue;
				}
				if (IsUsable(Base[I]))
				{
					for (int32 J = 0; J < Snapshots.Num(); ++J)
					{
						if (J != I && IsUsable(Base[J]) && TouchesOnExpansionSide(Base[I], Base[J], Wall.Side, Values))
						{
							AddProblem(OutProblems, EBathhouseProblemCode::ExpansionTouchingSide, EBathhouseProblemSeverity::Error,
								I, INDEX_NONE, StepIndex,
								FString::Printf(TEXT("%s이(가) %s과(와) 맞닿은 %s 벽을 넓힙니다. 공간 사이 벽은 움직일 수 없습니다."),
									*Label, *KindName(Snapshots[J].Kind), *SideName(Wall.Side)));
							break;
						}
					}
				}
				for (const FBathhouseOpeningSnapshot& Opening : Space.Openings)
				{
					if (IsExpansionOutsideOpening(Opening) && Opening.Side == Wall.Side)
					{
						AddProblem(OutProblems, EBathhouseProblemCode::ExpansionEntranceSide, EBathhouseProblemSeverity::Warning,
							I, INDEX_NONE, StepIndex,
							FString::Printf(TEXT("%s이(가) 바깥 출입구가 있는 %s 벽을 넓힙니다. 출입구는 벽을 따라가지만 바깥 물건은 따라가지 않습니다."),
								*Label, *SideName(Wall.Side)));
						break;
					}
				}
			}
		}
	}

	// 목록 끝까지 넓힌 모습끼리 부피 겹침.
	for (int32 I = 0; I < End.Num(); ++I)
	{
		for (int32 J = I + 1; J < End.Num(); ++J)
		{
			if (IsUsable(End[I]) && IsUsable(End[J]) && VolumesOverlap(End[I], End[J], Values))
			{
				AddProblem(OutProblems, EBathhouseProblemCode::ExpansionOverlap, EBathhouseProblemSeverity::Error, I, J, INDEX_NONE,
					FString::Printf(TEXT("목록 끝까지 넓힌 모습에서 %s와 %s의 부피가 겹칩니다. 넓힘 줄의 방향이나 양을 줄이세요."),
						*Snapshots[I].DisplayName, *Snapshots[J].DisplayName));
			}
		}
	}

	// 끝 모습에서 바깥 출입구 앞을 다른 공간이 막는지(기존 판정 재사용).
	{
		FProblems EndProblems;
		CollectProblems(End, Inputs, EndProblems);
		for (const FBathhouseLayoutProblem& Problem : EndProblems)
		{
			if (Problem.Code == EBathhouseProblemCode::OutsideOpeningBlocked)
			{
				AddProblem(OutProblems, EBathhouseProblemCode::ExpansionOutsideOpeningBlocked, EBathhouseProblemSeverity::Error,
					Problem.OwnerIndex, INDEX_NONE, Problem.ItemIndex,
					FString::Printf(TEXT("목록 끝까지 넓힌 모습: %s"), *Problem.Message.ToString()));
			}
		}
	}

	// 끝 모습의 손님 길 범위(기존 Nav 규칙 재사용). 넓히지 않는 공간은 0회 검사가 이미 본다.
	{
		TArray<FBathhouseSpaceSnapshot> Expanded;
		TArray<int32> ExpandedIndices;
		for (int32 I = 0; I < End.Num(); ++I)
		{
			if (!Snapshots[I].Steps.IsEmpty())
			{
				Expanded.Add(End[I]);
				ExpandedIndices.Add(I);
			}
		}
		FProblems NavProblems;
		ValidateNavigation(Expanded, NavBoxes, Inputs, NavProblems);
		for (const FBathhouseLayoutProblem& Problem : NavProblems)
		{
			const bool bWork = Problem.Code == EBathhouseProblemCode::NavWorkCovered;
			if (!bWork && Problem.Code != EBathhouseProblemCode::NavOutside)
			{
				continue;
			}
			const int32 Owner = ExpandedIndices.IsValidIndex(Problem.OwnerIndex) ? ExpandedIndices[Problem.OwnerIndex] : INDEX_NONE;
			AddProblem(OutProblems,
				bWork ? EBathhouseProblemCode::ExpansionNavWorkCovered : EBathhouseProblemCode::ExpansionNavOutside,
				EBathhouseProblemSeverity::Error, Owner, INDEX_NONE, Problem.ItemIndex,
				FString::Printf(TEXT("목록 끝까지 넓힌 모습: %s"), *Problem.Message.ToString()));
		}
	}

	if (HallEffectRowCount != INDEX_NONE && HallIndex != INDEX_NONE)
	{
		const int32 HallSteps = Snapshots[HallIndex].Steps.Num();
		if (HallSteps + 1 > HallEffectRowCount)
		{
			AddProblem(OutProblems, EBathhouseProblemCode::ExpansionHallEffectShort, EBathhouseProblemSeverity::Error,
				HallIndex, INDEX_NONE, INDEX_NONE,
				FString::Printf(TEXT("홀 넓힘 효과 표(DA_BathhouseExpansion_Default Tiers)가 %d줄입니다. 홀 넓힘 %d개에는 %d줄이 필요합니다."),
					HallEffectRowCount, HallSteps, HallSteps + 1));
		}
	}
}
