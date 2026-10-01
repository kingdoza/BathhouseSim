#include "Building/BathhouseSpaceValidation.h"

#include "Building/BathhouseBuildingSettings.h"
#include "Building/BathhouseSpaceActor.h"
#include "CollisionQueryParams.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/PlaceableFacility.h"

namespace
{
	constexpr double Tol = UE_DOUBLE_KINDA_SMALL_NUMBER;
	using FSnapshots = TArray<FBathhouseSpaceSnapshot>;
	using FProblems = TArray<FBathhouseLayoutProblem>;

	FString FormatCm(const double Value)
	{
		return FString::Printf(TEXT("%scm(%sm)"),
			*FString::SanitizeFloat(Value, 0), *FString::SanitizeFloat(Value / 100.0, 0));
	}

	FString SideName(const EBathhouseSpaceSide Side)
	{
		switch (Side)
		{
		case EBathhouseSpaceSide::East: return TEXT("동쪽");
		case EBathhouseSpaceSide::West: return TEXT("서쪽");
		case EBathhouseSpaceSide::North: return TEXT("북쪽");
		default: return TEXT("남쪽");
		}
	}

	FString KindName(const EBathhouseSpaceKind Kind)
	{
		switch (Kind)
		{
		case EBathhouseSpaceKind::Hall: return TEXT("홀");
		case EBathhouseSpaceKind::Bath: return TEXT("목욕공간");
		default: return TEXT("작업공간");
		}
	}

	bool HasSize(const FBathhouseSpaceSnapshot& S)
	{
		return S.Interior.Max.X - S.Interior.Min.X > Tol && S.Interior.Max.Y - S.Interior.Min.Y > Tol
			&& S.CeilingHeightCm > Tol && FMath::IsFinite(S.CeilingHeightCm);
	}

	bool IsUsable(const FBathhouseSpaceSnapshot& S)
	{
		return S.bTransformValid && HasSize(S);
	}

	double AlongMin(const FBathhouseSpaceSnapshot& S, const EBathhouseSpaceSide Side)
	{
		return FBathhouseSpaceLayout::IsAlongY(Side) ? S.Interior.Min.Y : S.Interior.Min.X;
	}

	double AlongMax(const FBathhouseSpaceSnapshot& S, const EBathhouseSpaceSide Side)
	{
		return FBathhouseSpaceLayout::IsAlongY(Side) ? S.Interior.Max.Y : S.Interior.Max.X;
	}

	/** 변의 법선 축 좌표. */
	double EdgeCoordinate(const FBox2D& Rect, const EBathhouseSpaceSide Side)
	{
		switch (Side)
		{
		case EBathhouseSpaceSide::East: return Rect.Max.X;
		case EBathhouseSpaceSide::West: return Rect.Min.X;
		case EBathhouseSpaceSide::North: return Rect.Max.Y;
		default: return Rect.Min.Y;
		}
	}

	double RectOverlap(const double MinA, const double MaxA, const double MinB, const double MaxB)
	{
		return FMath::Min(MaxA, MaxB) - FMath::Max(MinA, MinB);
	}

	bool RectInside(const FBox2D& Inner, const FBox2D& Outer)
	{
		return Inner.Min.X >= Outer.Min.X - Tol && Inner.Min.Y >= Outer.Min.Y - Tol
			&& Inner.Max.X <= Outer.Max.X + Tol && Inner.Max.Y <= Outer.Max.Y + Tol;
	}

	void AddProblem(
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
	void VolumeZ(const FBathhouseSpaceSnapshot& S, const double Slab, double& OutMin, double& OutMax)
	{
		OutMin = S.FloorZ - Slab;
		OutMax = FBathhouseSpaceLayout::CeilingZ(S) + Slab;
	}

	bool VolumesOverlap(
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

	/** 위치 제안 없이 규칙만 검사한다. */
	void CollectProblems(const FSnapshots& S, const FBathhouseValidationInputs& In, FProblems& Out)
	{
		const double T = In.Layout.WallThicknessCm;
		const double Slab = In.Layout.SlabThicknessCm;

		// 종류마다 공간이 정확히 하나.
		for (const EBathhouseSpaceKind Kind : { EBathhouseSpaceKind::Hall, EBathhouseSpaceKind::Bath, EBathhouseSpaceKind::Work })
		{
			TArray<int32> Indices;
			for (int32 I = 0; I < S.Num(); ++I)
			{
				if (S[I].Kind == Kind)
				{
					Indices.Add(I);
				}
			}
			if (Indices.IsEmpty())
			{
				AddProblem(Out, EBathhouseProblemCode::KindMissing, EBathhouseProblemSeverity::Error,
					S.IsEmpty() ? INDEX_NONE : 0, INDEX_NONE, INDEX_NONE,
					FString::Printf(TEXT("%s 공간이 없습니다. 종류마다 공간 Actor가 하나씩 필요합니다."), *KindName(Kind)));
			}
			else if (Indices.Num() > 1)
			{
				for (const int32 I : Indices)
				{
					AddProblem(Out, EBathhouseProblemCode::KindDuplicate, EBathhouseProblemSeverity::Error,
						I, INDEX_NONE, INDEX_NONE,
						FString::Printf(TEXT("%s: 같은 종류(%s)의 공간 Actor가 %d개 있습니다. 하나만 두세요."),
							*S[I].DisplayName, *KindName(Kind), Indices.Num()));
				}
			}
		}

		for (int32 I = 0; I < S.Num(); ++I)
		{
			const FBathhouseSpaceSnapshot& Space = S[I];
			if (!Space.bTransformValid)
			{
				AddProblem(Out, EBathhouseProblemCode::TransformInvalid, EBathhouseProblemSeverity::Error, I, INDEX_NONE, INDEX_NONE,
					FString::Printf(TEXT("%s: Rotation은 0, Scale은 1이어야 합니다. 공간 크기는 Floor Size Cm로 바꾸세요."), *Space.DisplayName));
			}
			if (!HasSize(Space))
			{
				AddProblem(Out, EBathhouseProblemCode::ValueInvalid, EBathhouseProblemSeverity::Error, I, INDEX_NONE, INDEX_NONE,
					FString::Printf(TEXT("%s: Floor Size Cm와 Ceiling Height Cm는 0보다 큰 유한한 값이어야 합니다."), *Space.DisplayName));
			}
			if (!(Space.LightSpacingCm > 0.0) || !FMath::IsFinite(Space.LightSpacingCm))
			{
				AddProblem(Out, EBathhouseProblemCode::ValueInvalid, EBathhouseProblemSeverity::Error, I, INDEX_NONE, INDEX_NONE,
					FString::Printf(TEXT("%s: Lighting의 Spacing Cm는 0보다 큰 유한한 값이어야 합니다."), *Space.DisplayName));
			}
			if (Space.bAllowedTagsEmpty)
			{
				AddProblem(Out, EBathhouseProblemCode::TagsEmpty, EBathhouseProblemSeverity::Error, I, INDEX_NONE, INDEX_NONE,
					FString::Printf(TEXT("%s: 놓을 수 있는 설비 목록(Allowed Facility Tags)이 비어 있습니다."), *Space.DisplayName));
			}
			if (!In.bBoxMeshValid)
			{
				AddProblem(Out, EBathhouseProblemCode::BoxMeshMissing, EBathhouseProblemSeverity::Error, I, INDEX_NONE, INDEX_NONE,
					FString::Printf(TEXT("%s: Project Settings > Game > Bathhouse Building의 Shell Box Mesh가 비었거나 크기가 없습니다."), *Space.DisplayName));
			}
			if ((Space.ChunkKind == EBathhouseCleaningChunkKind::Litter && !In.bLitterClassSet)
				|| (Space.ChunkKind == EBathhouseCleaningChunkKind::Stain && !In.bStainClassSet))
			{
				AddProblem(Out, EBathhouseProblemCode::ChunkClassMissing, EBathhouseProblemSeverity::Error, I, INDEX_NONE, INDEX_NONE,
					FString::Printf(TEXT("%s: 고른 조각 종류의 구역 Blueprint가 Project Settings > Game > Bathhouse Building에 없습니다."), *Space.DisplayName));
			}
			if (Space.Kind == EBathhouseSpaceKind::Work && Space.ChunkKind != EBathhouseCleaningChunkKind::None)
			{
				AddProblem(Out, EBathhouseProblemCode::WorkChunkKind, EBathhouseProblemSeverity::Warning, I, INDEX_NONE, INDEX_NONE,
					FString::Printf(TEXT("%s: 작업공간에는 손님이 없어 청소 조각이 생기지 않습니다. 조각 종류를 없음으로 두세요."), *Space.DisplayName));
			}
			if (!Space.bHasWallMaterial || !Space.bHasFloorMaterial || !Space.bHasCeilingMaterial)
			{
				AddProblem(Out, EBathhouseProblemCode::MaterialMissing, EBathhouseProblemSeverity::Warning, I, INDEX_NONE, INDEX_NONE,
					FString::Printf(TEXT("%s: 벽·바닥·천장 재질이 비었습니다. 엔진 기본 격자 재질로 보입니다."), *Space.DisplayName));
			}
		}

		// 두 공간의 부피 겹침(닿음은 허용).
		for (int32 I = 0; I < S.Num(); ++I)
		{
			for (int32 J = I + 1; J < S.Num(); ++J)
			{
				if (IsUsable(S[I]) && IsUsable(S[J]) && VolumesOverlap(S[I], S[J], In.Layout))
				{
					AddProblem(Out, EBathhouseProblemCode::Overlap, EBathhouseProblemSeverity::Error, I, J, INDEX_NONE,
						FString::Printf(TEXT("%s와 %s의 부피가 겹칩니다. 지하는 높이로 떨어져 있으면 괜찮고, 같은 높이의 두 공간은 서로 맞닿게 떼어 두세요."),
							*S[I].DisplayName, *S[J].DisplayName));
				}
			}
		}

		// 개구부.
		struct FWallEntry
		{
			FVector2D Interval;
			int32 Owner;
			int32 Item;
		};
		for (int32 I = 0; I < S.Num(); ++I)
		{
			const FBathhouseSpaceSnapshot& Space = S[I];
			if (!IsUsable(Space))
			{
				continue;
			}
			TMap<EBathhouseSpaceSide, TArray<FWallEntry>> PerWall;
			for (int32 OpeningIndex = 0; OpeningIndex < Space.Openings.Num(); ++OpeningIndex)
			{
				const FBathhouseOpeningSnapshot& Opening = Space.Openings[OpeningIndex];
				const FString Label = FString::Printf(TEXT("%s의 %s 개구부 %d번"),
					*Space.DisplayName, *SideName(Opening.Side), OpeningIndex);
				if (!(Opening.WidthCm > 0.0) || !(Opening.HeightCm > 0.0)
					|| !FMath::IsFinite(Opening.WidthCm) || !FMath::IsFinite(Opening.HeightCm)
					|| !FMath::IsFinite(Opening.CenterOffsetCm))
				{
					AddProblem(Out, EBathhouseProblemCode::OpeningInvalid, EBathhouseProblemSeverity::Error, I, INDEX_NONE, OpeningIndex,
						FString::Printf(TEXT("%s: 폭과 높이는 0보다 큰 유한한 값이어야 합니다."), *Label));
					continue;
				}
				if (Opening.HeightCm > Space.CeilingHeightCm + Tol)
				{
					AddProblem(Out, EBathhouseProblemCode::OpeningInvalid, EBathhouseProblemSeverity::Error, I, INDEX_NONE, OpeningIndex,
						FString::Printf(TEXT("%s: 높이가 천장 높이보다 큽니다."), *Label));
				}
				const FVector2D Interval = FBathhouseSpaceLayout::OpeningWorldInterval(Space, Opening);
				if (Interval.X < AlongMin(Space, Opening.Side) - Tol || Interval.Y > AlongMax(Space, Opening.Side) + Tol)
				{
					AddProblem(Out, EBathhouseProblemCode::OpeningOutsideWall, EBathhouseProblemSeverity::Error, I, INDEX_NONE, OpeningIndex,
						FString::Printf(TEXT("%s: 벽 안쪽 길이 밖으로 나갑니다. Center Offset Cm 또는 폭을 줄이세요."), *Label));
				}
				PerWall.FindOrAdd(Opening.Side).Add({ Interval, I, OpeningIndex });
			}
			// 이 공간 벽으로 들어오는 통로.
			for (int32 OtherIndex = 0; OtherIndex < S.Num(); ++OtherIndex)
			{
				if (OtherIndex == I)
				{
					continue;
				}
				for (int32 OpeningIndex = 0; OpeningIndex < S[OtherIndex].Openings.Num(); ++OpeningIndex)
				{
					const FBathhouseOpeningSnapshot& Opening = S[OtherIndex].Openings[OpeningIndex];
					if (Opening.ConnectedIndex == I && Opening.WidthCm > 0.0)
					{
						PerWall.FindOrAdd(FBathhouseSpaceLayout::Opposite(Opening.Side)).Add({
							FBathhouseSpaceLayout::OpeningWorldInterval(S[OtherIndex], Opening), OtherIndex, OpeningIndex });
					}
				}
			}
			for (const TPair<EBathhouseSpaceSide, TArray<FWallEntry>>& Pair : PerWall)
			{
				const TArray<FWallEntry>& Entries = Pair.Value;
				for (int32 A = 0; A < Entries.Num(); ++A)
				{
					for (int32 B = A + 1; B < Entries.Num(); ++B)
					{
						if (RectOverlap(Entries[A].Interval.X, Entries[A].Interval.Y, Entries[B].Interval.X, Entries[B].Interval.Y) > Tol)
						{
							AddProblem(Out, EBathhouseProblemCode::OpeningOverlap, EBathhouseProblemSeverity::Error, I, INDEX_NONE, Entries[B].Item,
								FString::Printf(TEXT("%s의 %s 벽에서 개구부가 서로 겹칩니다."), *Space.DisplayName, *SideName(Pair.Key)));
						}
					}
				}
			}

			for (int32 OpeningIndex = 0; OpeningIndex < Space.Openings.Num(); ++OpeningIndex)
			{
				const FBathhouseOpeningSnapshot& Opening = Space.Openings[OpeningIndex];
				if (!(Opening.WidthCm > 0.0) || !(Opening.HeightCm > 0.0))
				{
					continue;
				}
				const FVector2D Interval = FBathhouseSpaceLayout::OpeningWorldInterval(Space, Opening);
				const FString Label = FString::Printf(TEXT("%s의 %s 통로 %d번"),
					*Space.DisplayName, *SideName(Opening.Side), OpeningIndex);
				if (Opening.ConnectedIndex == INDEX_NONE)
				{
					if (Opening.ConnectedActor.IsValid() || Opening.ConnectedIndex != INDEX_NONE)
					{
						AddProblem(Out, EBathhouseProblemCode::PassageInvalid, EBathhouseProblemSeverity::Error, I, INDEX_NONE, OpeningIndex,
							FString::Printf(TEXT("%s: 연결 공간이 같은 레벨에서 찾아지지 않습니다."), *Label));
						continue;
					}
					// 바깥 출입구: 구간에 다른 공간이 닿으면 안 된다.
					const FBox2D Outer = FBathhouseSpaceLayout::OuterRect(Space, T);
					const double Edge = EdgeCoordinate(Outer, Opening.Side);
					const bool bAlongY = FBathhouseSpaceLayout::IsAlongY(Opening.Side);
					for (int32 OtherIndex = 0; OtherIndex < S.Num(); ++OtherIndex)
					{
						if (OtherIndex == I || !IsUsable(S[OtherIndex]))
						{
							continue;
						}
						const FBox2D OtherOuter = FBathhouseSpaceLayout::OuterRect(S[OtherIndex], T);
						const double NormalMin = bAlongY ? OtherOuter.Min.X : OtherOuter.Min.Y;
						const double NormalMax = bAlongY ? OtherOuter.Max.X : OtherOuter.Max.Y;
						const double OtherAlongMin = bAlongY ? OtherOuter.Min.Y : OtherOuter.Min.X;
						const double OtherAlongMax = bAlongY ? OtherOuter.Max.Y : OtherOuter.Max.X;
						const bool bOutward = Opening.Side == EBathhouseSpaceSide::East || Opening.Side == EBathhouseSpaceSide::North;
						const bool bBeyond = bOutward
							? (NormalMax > Edge + Tol && NormalMin <= Edge + Tol)
							: (NormalMin < Edge - Tol && NormalMax >= Edge - Tol);
						double ZMin, ZMax;
						VolumeZ(S[OtherIndex], Slab, ZMin, ZMax);
						if (bBeyond
							&& RectOverlap(Interval.X, Interval.Y, OtherAlongMin, OtherAlongMax) > Tol
							&& RectOverlap(Space.FloorZ, Space.FloorZ + Opening.HeightCm, ZMin, ZMax) > Tol)
						{
							AddProblem(Out, EBathhouseProblemCode::OutsideOpeningBlocked, EBathhouseProblemSeverity::Error, I, OtherIndex, OpeningIndex,
								FString::Printf(TEXT("%s: 바깥 출입구 너머에 %s이(가) 붙어 있습니다. 통로라면 연결 공간을 지정하세요."),
									*Label, *S[OtherIndex].DisplayName));
						}
					}
					continue;
				}

				// 통로.
				if (!S.IsValidIndex(Opening.ConnectedIndex) || Opening.ConnectedIndex == I)
				{
					AddProblem(Out, EBathhouseProblemCode::PassageInvalid, EBathhouseProblemSeverity::Error, I, INDEX_NONE, OpeningIndex,
						FString::Printf(TEXT("%s: 연결 공간은 다른 공간이어야 합니다."), *Label));
					continue;
				}
				const int32 B = Opening.ConnectedIndex;
				const FBathhouseSpaceSnapshot& Target = S[B];
				if (!IsUsable(Target))
				{
					continue;
				}
				const FBox2D OuterA = FBathhouseSpaceLayout::OuterRect(Space, T);
				const FBox2D OuterB = FBathhouseSpaceLayout::OuterRect(Target, T);
				const EBathhouseSpaceSide Opp = FBathhouseSpaceLayout::Opposite(Opening.Side);
				const bool bTouching = FMath::Abs(EdgeCoordinate(OuterA, Opening.Side) - EdgeCoordinate(OuterB, Opp)) <= Tol;
				const bool bAlongY = FBathhouseSpaceLayout::IsAlongY(Opening.Side);
				const double OverlapAlong = bAlongY
					? RectOverlap(OuterA.Min.Y, OuterA.Max.Y, OuterB.Min.Y, OuterB.Max.Y)
					: RectOverlap(OuterA.Min.X, OuterA.Max.X, OuterB.Min.X, OuterB.Max.X);
				if (!bTouching || OverlapAlong <= Tol)
				{
					AddProblem(Out, EBathhouseProblemCode::PassageNotTouching, EBathhouseProblemSeverity::Error, I, B, OpeningIndex,
						FString::Printf(TEXT("%s가 두 공간을 잇지 않습니다. %s와 %s의 바깥 직사각형 변이 맞닿아 있어야 뚫립니다."),
							*Label, *Space.DisplayName, *Target.DisplayName));
					continue;
				}
				if (Interval.X < AlongMin(Target, Opp) - Tol || Interval.Y > AlongMax(Target, Opp) + Tol)
				{
					AddProblem(Out, EBathhouseProblemCode::PassageRangeOutside, EBathhouseProblemSeverity::Error, I, B, OpeningIndex,
						FString::Printf(TEXT("%s: 통로 구간이 %s의 벽 안쪽 길이 밖으로 나갑니다."), *Label, *Target.DisplayName));
				}
				if (FMath::Abs(Space.FloorZ - Target.FloorZ) > Tol)
				{
					AddProblem(Out, EBathhouseProblemCode::PassageFloorZ, EBathhouseProblemSeverity::Error, I, B, OpeningIndex,
						FString::Printf(TEXT("%s: 두 공간의 바닥 높이(Location Z)가 달라 통로로 이을 수 없습니다."), *Label));
				}
				if (Opening.HeightCm > Target.CeilingHeightCm + Tol)
				{
					AddProblem(Out, EBathhouseProblemCode::PassageHeight, EBathhouseProblemSeverity::Error, I, B, OpeningIndex,
						FString::Printf(TEXT("%s: 통로 높이가 %s의 천장 높이보다 큽니다."), *Label, *Target.DisplayName));
				}
			}
		}

		// 홀 출입구, 작업공간 계단.
		for (int32 I = 0; I < S.Num(); ++I)
		{
			if (S[I].Kind == EBathhouseSpaceKind::Hall)
			{
				bool bHasEntrance = false;
				for (const FBathhouseOpeningSnapshot& Opening : S[I].Openings)
				{
					bHasEntrance |= Opening.ConnectedIndex == INDEX_NONE && !Opening.ConnectedActor.IsValid();
				}
				if (!bHasEntrance)
				{
					AddProblem(Out, EBathhouseProblemCode::HallNoEntrance, EBathhouseProblemSeverity::Error, I, INDEX_NONE, INDEX_NONE,
						FString::Printf(TEXT("%s: 바깥 출입구가 하나도 없습니다. 손님은 홀 출입구로만 드나듭니다."), *S[I].DisplayName));
				}
			}
			if (S[I].Kind == EBathhouseSpaceKind::Work)
			{
				bool bHasStairs = false;
				for (const FBathhouseSpaceSnapshot& Other : S)
				{
					for (const FBathhouseStairSnapshot& Stair : Other.Stairs)
					{
						bHasStairs |= Stair.LowerIndex == I;
					}
				}
				if (!bHasStairs)
				{
					AddProblem(Out, EBathhouseProblemCode::WorkNoStairs, EBathhouseProblemSeverity::Error, I, INDEX_NONE, INDEX_NONE,
						FString::Printf(TEXT("%s: 이 공간으로 내려가는 계단이 없습니다."), *S[I].DisplayName));
				}
			}
		}

		// 계단.
		for (int32 I = 0; I < S.Num(); ++I)
		{
			const FBathhouseSpaceSnapshot& Upper = S[I];
			for (int32 StairIndex = 0; StairIndex < Upper.Stairs.Num(); ++StairIndex)
			{
				const FBathhouseStairSnapshot& Stair = Upper.Stairs[StairIndex];
				const FString Label = FString::Printf(TEXT("%s의 계단 %d번"), *Upper.DisplayName, StairIndex);
				if (!IsUsable(Upper))
				{
					continue;
				}
				if (!S.IsValidIndex(Stair.LowerIndex) || Stair.LowerIndex == I)
				{
					AddProblem(Out, EBathhouseProblemCode::StairInvalid, EBathhouseProblemSeverity::Error, I, INDEX_NONE, StairIndex,
						FString::Printf(TEXT("%s: 아래층 공간(Lower Space)이 비었거나 같은 공간입니다."), *Label));
					continue;
				}
				const FBathhouseSpaceSnapshot& Lower = S[Stair.LowerIndex];
				if (!(Stair.WidthCm > 0.0) || !(Stair.RunCm > 0.0) || Stair.StepCount <= 0
					|| !FMath::IsFinite(Stair.WidthCm) || !FMath::IsFinite(Stair.RunCm))
				{
					AddProblem(Out, EBathhouseProblemCode::StairInvalid, EBathhouseProblemSeverity::Error, I, Stair.LowerIndex, StairIndex,
						FString::Printf(TEXT("%s: 폭, 길이, 판 수는 0보다 커야 합니다."), *Label));
					continue;
				}
				if (!(Lower.FloorZ < Upper.FloorZ - Tol))
				{
					AddProblem(Out, EBathhouseProblemCode::StairInvalid, EBathhouseProblemSeverity::Error, I, Stair.LowerIndex, StairIndex,
						FString::Printf(TEXT("%s: 아래층 %s의 바닥이 더 낮아야 합니다."), *Label, *Lower.DisplayName));
					continue;
				}
				if (!IsUsable(Lower))
				{
					continue;
				}
				const FBathhouseStairFrame Frame = FBathhouseSpaceLayout::MakeStairFrame(Upper, Stair);
				const double W = Stair.WidthCm;
				const FBox2D Footprint = Frame.ToWorldRect(-T, Stair.RunCm + T, -W * 0.5 - T, W * 0.5 + T);
				if (!RectInside(Footprint, Upper.Interior) || !RectInside(Footprint, Lower.Interior))
				{
					AddProblem(Out, EBathhouseProblemCode::StairOutside, EBathhouseProblemSeverity::Error, I, Stair.LowerIndex, StairIndex,
						FString::Printf(TEXT("%s: 계단 벽까지 포함한 발자국이 %s 또는 %s의 안쪽 바닥 밖으로 나갑니다."),
							*Label, *Upper.DisplayName, *Lower.DisplayName));
				}
				const FBox2D UpperLanding = Frame.ToWorldRect(-T - W, -T, -W * 0.5, W * 0.5);
				const FBox2D LowerLanding = Frame.ToWorldRect(Stair.RunCm + T, Stair.RunCm + T + W, -W * 0.5, W * 0.5);
				if (!RectInside(UpperLanding, Upper.Interior) || !RectInside(LowerLanding, Lower.Interior))
				{
					AddProblem(Out, EBathhouseProblemCode::StairLanding, EBathhouseProblemSeverity::Error, I, Stair.LowerIndex, StairIndex,
						FString::Printf(TEXT("%s: 계단 위 입구 앞이나 아래 출구 앞에 계단 폭만큼의 바닥이 없습니다."), *Label));
				}
				const double Rise = Upper.FloorZ - Lower.FloorZ;
				const double SlopeDegrees = FMath::RadiansToDegrees(FMath::Atan2(Rise, Stair.RunCm));
				if (SlopeDegrees > In.WalkableFloorAngleDegrees + Tol)
				{
					AddProblem(Out, EBathhouseProblemCode::StairSteep, EBathhouseProblemSeverity::Error, I, Stair.LowerIndex, StairIndex,
						FString::Printf(TEXT("%s: 너무 가파릅니다(경사 %s도, 캐릭터 이동 설정 기준 걸을 수 있는 최대 %s도). Run Cm를 늘리세요."),
							*Label, *FString::SanitizeFloat(SlopeDegrees, 0), *FString::SanitizeFloat(In.WalkableFloorAngleDegrees, 0)));
				}
			}
		}
	}

	struct FMove
	{
		int32 Index = INDEX_NONE;
		FVector2D DeltaXY = FVector2D::ZeroVector;
		double DeltaZ = 0.0;
	};

	int32 CountCode(const FProblems& Problems, const EBathhouseProblemCode Code)
	{
		int32 Count = 0;
		for (const FBathhouseLayoutProblem& Problem : Problems)
		{
			Count += Problem.Code == Code ? 1 : 0;
		}
		return Count;
	}

	bool HasSameProblem(const FProblems& Problems, const FBathhouseLayoutProblem& Original)
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

	FString DescribeMove(const FSnapshots& S, const FMove& Move)
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
		const FSnapshots& S,
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
		FSnapshots Moved = S;
		FBathhouseSpaceValidation::ApplyMove(Moved[Move.Index], Move.DeltaXY, Move.DeltaZ);
		FProblems After;
		CollectProblems(Moved, In, After);
		const int32 Allowed = OverlapsBefore - (Problem.Code == EBathhouseProblemCode::Overlap ? 1 : 0);
		return !HasSameProblem(After, Problem) && CountCode(After, EBathhouseProblemCode::Overlap) <= Allowed;
	}

	void CollectCandidates(
		const FSnapshots& S,
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
		const FSnapshots& S, const FBathhouseValidationInputs& In, FProblems& Problems)
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

	bool PointInAnyBox(const TArray<FBox>& Boxes, const FVector& Point)
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
}

void FBathhouseSpaceValidation::ApplyMove(FBathhouseSpaceSnapshot& Snapshot, const FVector2D& DeltaXY, const double DeltaZ)
{
	Snapshot.ActorXY += DeltaXY;
	Snapshot.Interior = FBox2D(Snapshot.Interior.Min + DeltaXY, Snapshot.Interior.Max + DeltaXY);
	Snapshot.FloorZ += DeltaZ;
}

bool FBathhouseSpaceValidation::SuggestTouchingLocation(
	const FSnapshots& Snapshots,
	const int32 AIndex,
	const int32 BIndex,
	const EBathhouseSpaceSide Side,
	const double WallThicknessCm,
	const bool bMoveB,
	FBathhouseLocationSuggestion& OutSuggestion)
{
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

void FBathhouseSpaceValidation::ValidateLayout(
	const FSnapshots& Snapshots, const FBathhouseValidationInputs& Inputs, FProblems& OutProblems)
{
	OutProblems.Reset();
	CollectProblems(Snapshots, Inputs, OutProblems);
	AppendSuggestions(Snapshots, Inputs, OutProblems);
}

void FBathhouseSpaceValidation::ValidateNavigation(
	const FSnapshots& S,
	const TArray<FBox>& NavBoxes,
	const FBathhouseValidationInputs& Inputs,
	FProblems& Out)
{
	const double T = Inputs.Layout.WallThicknessCm;
	for (int32 I = 0; I < S.Num(); ++I)
	{
		const FBathhouseSpaceSnapshot& Space = S[I];
		if (!IsUsable(Space))
		{
			continue;
		}
		if (Space.Kind == EBathhouseSpaceKind::Work)
		{
			const FVector2D Center = Space.Interior.GetCenter();
			if (PointInAnyBox(NavBoxes, FVector(Center.X, Center.Y, Space.FloorZ)))
			{
				AddProblem(Out, EBathhouseProblemCode::NavWorkCovered, EBathhouseProblemSeverity::Error, I, INDEX_NONE, INDEX_NONE,
					FString::Printf(TEXT("%s: 바닥이 손님 길 범위(NavMeshBoundsVolume) 안에 있습니다. 지하는 덮지 않도록 범위의 높이를 줄이세요."), *Space.DisplayName));
			}
			continue;
		}
		bool bCovered = false;
		for (const FBox& Box : NavBoxes)
		{
			bCovered |= Box.IsInsideOrOn(FVector(Space.Interior.Min.X, Space.Interior.Min.Y, Space.FloorZ))
				&& Box.IsInsideOrOn(FVector(Space.Interior.Max.X, Space.Interior.Max.Y, Space.FloorZ));
		}
		if (!bCovered)
		{
			AddProblem(Out, EBathhouseProblemCode::NavOutside, EBathhouseProblemSeverity::Error, I, INDEX_NONE, INDEX_NONE,
				FString::Printf(TEXT("%s: 바닥이 하나의 손님 길 범위(NavMeshBoundsVolume)에 다 들어오지 않습니다. 범위를 넓히세요."), *Space.DisplayName));
		}
		const FBox2D Outer = FBathhouseSpaceLayout::OuterRect(Space, T);
		for (int32 OpeningIndex = 0; OpeningIndex < Space.Openings.Num(); ++OpeningIndex)
		{
			const FBathhouseOpeningSnapshot& Opening = Space.Openings[OpeningIndex];
			if (Opening.ConnectedIndex != INDEX_NONE || Opening.ConnectedActor.IsValid() || !(Opening.WidthCm > 0.0))
			{
				continue;
			}
			const FVector2D Interval = FBathhouseSpaceLayout::OpeningWorldInterval(Space, Opening);
			const double Along = (Interval.X + Interval.Y) * 0.5;
			const double Edge = EdgeCoordinate(Outer, Opening.Side);
			const FVector2D Normal = FBathhouseSpaceLayout::SideNormal(Opening.Side);
			const double NormalPos = Edge + (Normal.X + Normal.Y) * Opening.WidthCm;
			const FVector Point = FBathhouseSpaceLayout::IsAlongY(Opening.Side)
				? FVector(NormalPos, Along, Space.FloorZ) : FVector(Along, NormalPos, Space.FloorZ);
			if (!PointInAnyBox(NavBoxes, Point))
			{
				AddProblem(Out, EBathhouseProblemCode::NavOutside, EBathhouseProblemSeverity::Error, I, INDEX_NONE, OpeningIndex,
					FString::Printf(TEXT("%s의 %s 출입구 앞 바깥 지점이 손님 길 범위(NavMeshBoundsVolume) 밖입니다. 범위를 출입구 밖 마당까지 넓히세요."),
						*Space.DisplayName, *SideName(Opening.Side)));
			}
		}
	}
}

void FBathhouseSpaceValidation::GatherSnapshots(UWorld& World, FSnapshots& OutSnapshots)
{
	OutSnapshots.Reset();
	TArray<const ABathhouseSpaceActor*> Actors;
	for (TActorIterator<ABathhouseSpaceActor> It(&World); It; ++It)
	{
		if (IsValid(*It))
		{
			Actors.Add(*It);
		}
	}
	Actors.Sort([](const ABathhouseSpaceActor& A, const ABathhouseSpaceActor& B)
	{
		return A.GetSpaceKind() != B.GetSpaceKind()
			? A.GetSpaceKind() < B.GetSpaceKind() : A.GetName() < B.GetName();
	});
	for (const ABathhouseSpaceActor* Actor : Actors)
	{
		Actor->FillSnapshot(OutSnapshots.AddDefaulted_GetRef());
	}
	auto Resolve = [&OutSnapshots](const TWeakObjectPtr<const ABathhouseSpaceActor>& Weak) -> int32
	{
		if (!Weak.IsValid())
		{
			return INDEX_NONE;
		}
		for (int32 I = 0; I < OutSnapshots.Num(); ++I)
		{
			if (OutSnapshots[I].Actor == Weak)
			{
				return I;
			}
		}
		return INDEX_NONE;
	};
	for (FBathhouseSpaceSnapshot& Snapshot : OutSnapshots)
	{
		for (FBathhouseOpeningSnapshot& Opening : Snapshot.Openings)
		{
			Opening.ConnectedIndex = Resolve(Opening.ConnectedActor);
		}
		for (FBathhouseStairSnapshot& Stair : Snapshot.Stairs)
		{
			Stair.LowerIndex = Resolve(Stair.LowerActor);
		}
	}
}

FBathhouseValidationInputs FBathhouseSpaceValidation::ReadInputs()
{
	FBathhouseValidationInputs Inputs;
	const UBathhouseBuildingSettings* Settings = GetDefault<UBathhouseBuildingSettings>();
	Inputs.Layout.WallThicknessCm = Settings->GetWallThicknessCm();
	Inputs.Layout.SlabThicknessCm = Settings->GetSlabThicknessCm();
	Inputs.Layout.ChunkMaxSizeCm = Settings->GetCleaningChunkMaxSizeCm();
	const UStaticMesh* Mesh = Settings->LoadShellBoxMesh();
	if (Mesh)
	{
		const FVector Extent = Mesh->GetBounds().BoxExtent;
		Inputs.bBoxMeshValid = Extent.X > Tol && Extent.Y > Tol && Extent.Z > Tol;
	}
	Inputs.bLitterClassSet = Settings->LoadLitterChunkZoneClass() != nullptr;
	Inputs.bStainClassSet = Settings->LoadStainChunkZoneClass() != nullptr;
	Inputs.WalkableFloorAngleDegrees = GetDefault<UCharacterMovementComponent>()->GetWalkableFloorAngle();
	return Inputs;
}

void FBathhouseSpaceValidation::ValidateWorld(
	UWorld& World, FSnapshots& OutSnapshots, FProblems& OutProblems)
{
	GatherSnapshots(World, OutSnapshots);
	const FBathhouseValidationInputs Inputs = ReadInputs();
	ValidateLayout(OutSnapshots, Inputs, OutProblems);
	if (OutSnapshots.IsEmpty())
	{
		return;
	}

	// Nav 범위.
	TArray<FBox> NavBoxes;
	for (TActorIterator<ANavMeshBoundsVolume> It(&World); It; ++It)
	{
		if (IsValid(*It))
		{
			NavBoxes.Add(It->GetComponentsBoundingBox(true));
		}
	}
	ValidateNavigation(OutSnapshots, NavBoxes, Inputs, OutProblems);

	// 계단 통로(구멍 안)를 공간이 아닌 blocking 물체(지형 등)가 막는지 수직 trace로 본다.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	for (int32 I = 0; I < OutSnapshots.Num(); ++I)
	{
		const FBathhouseSpaceSnapshot& Upper = OutSnapshots[I];
		for (int32 StairIndex = 0; StairIndex < Upper.Stairs.Num(); ++StairIndex)
		{
			const FBathhouseStairSnapshot& Stair = Upper.Stairs[StairIndex];
			if (!OutSnapshots.IsValidIndex(Stair.LowerIndex) || Stair.LowerIndex == I
				|| !(Stair.WidthCm > 0.0) || !(Stair.RunCm > 0.0))
			{
				continue;
			}
			const FBathhouseSpaceSnapshot& Lower = OutSnapshots[Stair.LowerIndex];
			const double Top = Upper.FloorZ;
			const double Bottom = FBathhouseSpaceLayout::CeilingZ(Lower) + Inputs.Layout.SlabThicknessCm;
			if (Top <= Bottom)
			{
				continue;
			}
			const FBox2D Hole = FBathhouseSpaceLayout::StairHole(Upper, Stair);
			bool bBlocked = false;
			for (const double FractionX : { 0.0, 0.5, 1.0 })
			{
				for (const double FractionY : { 0.0, 0.5, 1.0 })
				{
					const FVector2D Point(
						FMath::Lerp(Hole.Min.X, Hole.Max.X, FractionX), FMath::Lerp(Hole.Min.Y, Hole.Max.Y, FractionY));
					TArray<FHitResult> Hits;
					World.LineTraceMultiByObjectType(
						Hits, FVector(Point.X, Point.Y, Top), FVector(Point.X, Point.Y, Bottom), ObjectParams);
					for (const FHitResult& Hit : Hits)
					{
						if (Hit.bBlockingHit && !Cast<ABathhouseSpaceActor>(Hit.GetActor()))
						{
							bBlocked = true;
						}
					}
				}
			}
			if (bBlocked)
			{
				AddProblem(OutProblems, EBathhouseProblemCode::StairBlocked, EBathhouseProblemSeverity::Error,
					I, Stair.LowerIndex, StairIndex,
					FString::Printf(TEXT("%s의 계단 %d번: 계단 통로를 지형 등 다른 물체가 막고 있습니다. 지형 구멍을 넓히거나 계단 위치를 옮기세요."),
						*Upper.DisplayName, StairIndex));
			}
		}
	}

	// 배치된 설비 소속.
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		AActor* Actor = *It;
		const IPlaceableFacility* Facility = Cast<IPlaceableFacility>(Actor);
		const UFacilityPlacementComponent* Placement = Facility ? Facility->GetFacilityPlacementComponent() : nullptr;
		if (!Placement || Placement->GetMode() != EPlaceableFacilityMode::Placed || Placement->IsStagedPlacement()
			|| !Placement->GetDefinition())
		{
			continue;
		}
		const FVector Location = Actor->GetActorLocation();
		int32 SpaceIndex = INDEX_NONE;
		for (int32 I = 0; I < OutSnapshots.Num(); ++I)
		{
			const FBathhouseSpaceSnapshot& Space = OutSnapshots[I];
			if (IsUsable(Space) && Space.Interior.IsInside(FVector2D(Location.X, Location.Y))
				&& Location.Z >= Space.FloorZ - Inputs.Layout.SlabThicknessCm
				&& Location.Z <= FBathhouseSpaceLayout::CeilingZ(Space))
			{
				SpaceIndex = I;
				break;
			}
		}
		if (SpaceIndex == INDEX_NONE)
		{
			AddProblem(OutProblems, EBathhouseProblemCode::FacilityMisplaced, EBathhouseProblemSeverity::Warning, 0, INDEX_NONE, INDEX_NONE,
				FString::Printf(TEXT("설비 %s이(가) 어느 공간 안에도 없습니다."), *Actor->GetActorNameOrLabel()));
			continue;
		}
		const ABathhouseSpaceActor* SpaceActor = OutSnapshots[SpaceIndex].Actor.Get();
		if (SpaceActor && !SpaceActor->IsDefinitionAllowed(*Placement->GetDefinition()))
		{
			AddProblem(OutProblems, EBathhouseProblemCode::FacilityMisplaced, EBathhouseProblemSeverity::Warning, SpaceIndex, INDEX_NONE, INDEX_NONE,
				FString::Printf(TEXT("설비 %s은(는) %s에 놓을 수 없는 종류입니다."),
					*Actor->GetActorNameOrLabel(), *OutSnapshots[SpaceIndex].DisplayName));
		}
	}
}
