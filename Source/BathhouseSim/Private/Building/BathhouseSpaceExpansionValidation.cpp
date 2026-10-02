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
	const int32 MaxPurchaseCount,
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

	int32 TotalSteps = 0;
	for (int32 I = 0; I < Snapshots.Num(); ++I)
	{
		const FBathhouseSpaceSnapshot& Space = Snapshots[I];
		TotalSteps += Space.Steps.Num();
		for (int32 StepIndex = 0; StepIndex < Space.Steps.Num(); ++StepIndex)
		{
			const FBathhouseExpansionStepSnapshot& Step = Space.Steps[StepIndex];
			const FString Label = FString::Printf(TEXT("%s %d번째 넓힘 줄"), *Space.DisplayName, StepIndex + 1);
			if (!(Step.AmountCm > 0.0) || !FMath::IsFinite(Step.AmountCm))
			{
				AddProblem(OutProblems, EBathhouseProblemCode::ExpansionAmountInvalid, EBathhouseProblemSeverity::Error,
					I, INDEX_NONE, StepIndex,
					FString::Printf(TEXT("%s: 넓히는 양은 0보다 큰 유한한 값이어야 합니다."), *Label));
				continue;
			}
			if (IsUsable(Base[I]))
			{
				for (int32 J = 0; J < Snapshots.Num(); ++J)
				{
					if (J != I && IsUsable(Base[J]) && TouchesOnExpansionSide(Base[I], Base[J], Step.Side, Values))
					{
						AddProblem(OutProblems, EBathhouseProblemCode::ExpansionTouchingSide, EBathhouseProblemSeverity::Error,
							I, INDEX_NONE, StepIndex,
							FString::Printf(TEXT("%s이(가) %s과(와) 맞닿은 %s 벽을 넓힙니다. 공간 사이 벽은 움직일 수 없습니다."),
								*Label, *KindName(Snapshots[J].Kind), *SideName(Step.Side)));
						break;
					}
				}
			}
			for (const FBathhouseOpeningSnapshot& Opening : Space.Openings)
			{
				if (IsExpansionOutsideOpening(Opening) && Opening.Side == Step.Side)
				{
					AddProblem(OutProblems, EBathhouseProblemCode::ExpansionEntranceSide, EBathhouseProblemSeverity::Warning,
						I, INDEX_NONE, StepIndex,
						FString::Printf(TEXT("%s이(가) 바깥 출입구가 있는 %s 벽을 넓힙니다. 출입구는 벽을 따라가지만 바깥 물건은 따라가지 않습니다."),
							*Label, *SideName(Step.Side)));
					break;
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

	if (MaxPurchaseCount != INDEX_NONE && !Snapshots.IsEmpty() && TotalSteps < MaxPurchaseCount)
	{
		int32 Owner = 0;
		for (int32 I = 0; I < Snapshots.Num(); ++I)
		{
			if (Snapshots[I].Kind == EBathhouseSpaceKind::Hall)
			{
				Owner = I;
				break;
			}
		}
		AddProblem(OutProblems, EBathhouseProblemCode::ExpansionStepsBelowCap, EBathhouseProblemSeverity::Warning,
			Owner, INDEX_NONE, INDEX_NONE,
			FString::Printf(TEXT("공간별 넓힘 줄 수의 합(%d)이 전체 구입 횟수 상한(%d)보다 작습니다. 상한에 닿기 전에 모든 선택지가 막힙니다."),
				TotalSteps, MaxPurchaseCount));
	}
}
