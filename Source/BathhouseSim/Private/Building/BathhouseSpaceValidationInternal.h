#pragma once

// 공간 검증 translation unit들이 공유하는 내부 helper. 이름 있는 namespace라 unity blob의 다른 파일과 겹치지 않는다.

#include "CoreMinimal.h"
#include "Building/BathhouseSpaceValidation.h"

namespace BathhouseSpaceValidationDetail
{
	inline constexpr double Tol = UE_DOUBLE_KINDA_SMALL_NUMBER;
	using FSnapshots = TArray<FBathhouseSpaceSnapshot>;
	using FProblems = TArray<FBathhouseLayoutProblem>;

	inline FString FormatCm(const double Value)
	{
		return FString::Printf(TEXT("%scm(%sm)"),
			*FString::SanitizeFloat(Value, 0), *FString::SanitizeFloat(Value / 100.0, 0));
	}

	inline FString SideName(const EBathhouseSpaceSide Side)
	{
		switch (Side)
		{
		case EBathhouseSpaceSide::East: return TEXT("동쪽");
		case EBathhouseSpaceSide::West: return TEXT("서쪽");
		case EBathhouseSpaceSide::North: return TEXT("북쪽");
		default: return TEXT("남쪽");
		}
	}

	inline FString KindName(const EBathhouseSpaceKind Kind)
	{
		switch (Kind)
		{
		case EBathhouseSpaceKind::Hall: return TEXT("홀");
		case EBathhouseSpaceKind::Bath: return TEXT("목욕공간");
		default: return TEXT("작업공간");
		}
	}

	inline bool HasSize(const FBathhouseSpaceSnapshot& S)
	{
		return S.Interior.Max.X - S.Interior.Min.X > Tol && S.Interior.Max.Y - S.Interior.Min.Y > Tol
			&& S.CeilingHeightCm > Tol && FMath::IsFinite(S.CeilingHeightCm);
	}

	inline bool IsUsable(const FBathhouseSpaceSnapshot& S)
	{
		return S.bTransformValid && HasSize(S);
	}

	inline double AlongMin(const FBathhouseSpaceSnapshot& S, const EBathhouseSpaceSide Side)
	{
		return FBathhouseSpaceLayout::IsAlongY(Side) ? S.Interior.Min.Y : S.Interior.Min.X;
	}

	inline double AlongMax(const FBathhouseSpaceSnapshot& S, const EBathhouseSpaceSide Side)
	{
		return FBathhouseSpaceLayout::IsAlongY(Side) ? S.Interior.Max.Y : S.Interior.Max.X;
	}

	/** 변의 법선 축 좌표. */
	inline double EdgeCoordinate(const FBox2D& Rect, const EBathhouseSpaceSide Side)
	{
		switch (Side)
		{
		case EBathhouseSpaceSide::East: return Rect.Max.X;
		case EBathhouseSpaceSide::West: return Rect.Min.X;
		case EBathhouseSpaceSide::North: return Rect.Max.Y;
		default: return Rect.Min.Y;
		}
	}

	inline double RectOverlap(const double MinA, const double MaxA, const double MinB, const double MaxB)
	{
		return FMath::Min(MaxA, MaxB) - FMath::Max(MinA, MinB);
	}

	inline bool RectInside(const FBox2D& Inner, const FBox2D& Outer)
	{
		return Inner.Min.X >= Outer.Min.X - Tol && Inner.Min.Y >= Outer.Min.Y - Tol
			&& Inner.Max.X <= Outer.Max.X + Tol && Inner.Max.Y <= Outer.Max.Y + Tol;
	}

	inline void AddProblem(
		FProblems& Out,
		const EBathhouseProblemCode Code,
		const EBathhouseProblemSeverity Severity,
		const int32 Owner,
		const int32 Other,
		const int32 Item,
		const FString& Message)
	{
		FBathhouseLayoutProblem& Problem = Out.AddDefaulted_GetRef();
		Problem.Code = Code;
		Problem.Severity = Severity;
		Problem.OwnerIndex = Owner;
		Problem.OtherIndex = Other;
		Problem.ItemIndex = Item;
		Problem.Message = FText::FromString(Message);
	}

	/** 수직 부피 [FloorZ - 판, CeilingZ + 판]. */
	inline void VolumeZ(const FBathhouseSpaceSnapshot& S, const double Slab, double& OutMin, double& OutMax)
	{
		OutMin = S.FloorZ - Slab;
		OutMax = FBathhouseSpaceLayout::CeilingZ(S) + Slab;
	}

	inline bool VolumesOverlap(
		const FBathhouseSpaceSnapshot& A, const FBathhouseSpaceSnapshot& B, const FBathhouseLayoutValues& Values)
	{
		const FBox2D OA = FBathhouseSpaceLayout::OuterRect(A, Values.WallThicknessCm);
		const FBox2D OB = FBathhouseSpaceLayout::OuterRect(B, Values.WallThicknessCm);
		double MinA, MaxA, MinB, MaxB;
		VolumeZ(A, Values.SlabThicknessCm, MinA, MaxA);
		VolumeZ(B, Values.SlabThicknessCm, MinB, MaxB);
		return RectOverlap(OA.Min.X, OA.Max.X, OB.Min.X, OB.Max.X) > Tol
			&& RectOverlap(OA.Min.Y, OA.Max.Y, OB.Min.Y, OB.Max.Y) > Tol
			&& RectOverlap(MinA, MaxA, MinB, MaxB) > Tol;
	}

	inline bool PointInAnyBox(const TArray<FBox>& Boxes, const FVector& Point)
	{
		for (const FBox& Box : Boxes)
		{
			if (Box.IsInsideOrOn(Point))
			{
				return true;
			}
		}
		return false;
	}

	/** 위치 제안 없이 규칙만 검사한다(BathhouseSpaceValidation.cpp). */
	void CollectProblems(const FSnapshots& S, const FBathhouseValidationInputs& In, FProblems& Out);
	/** 문제 목록에 위치 제안 문구를 붙인다(BathhouseSpacePositionSuggestion.cpp). */
	void AppendSuggestions(const FSnapshots& S, const FBathhouseValidationInputs& In, FProblems& Problems);
}
