#include "Building/BathhouseSpaceValidation.h"

#include "Building/BathhouseSpaceValidationInternal.h"

namespace BathhouseSpaceValidationDetail
{
	void CollectProblems(const TArray<FBathhouseSpaceSnapshot>& S, const FBathhouseValidationInputs& In, TArray<FBathhouseLayoutProblem>& Out)
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

		// Project Settings 값은 숨은 하한 없이 그대로 쓰므로 0 이하·비유한 값을 오류로 알린다.
		if (!S.IsEmpty() && (!(In.Layout.WallThicknessCm > 0.0) || !(In.Layout.SlabThicknessCm > 0.0)
			|| !(In.Layout.ChunkMaxSizeCm.X > 0.0) || !(In.Layout.ChunkMaxSizeCm.Y > 0.0)
			|| !FMath::IsFinite(In.Layout.WallThicknessCm) || !FMath::IsFinite(In.Layout.SlabThicknessCm)
			|| !FMath::IsFinite(In.Layout.ChunkMaxSizeCm.X) || !FMath::IsFinite(In.Layout.ChunkMaxSizeCm.Y)))
		{
			AddProblem(Out, EBathhouseProblemCode::ValueInvalid, EBathhouseProblemSeverity::Error, 0, INDEX_NONE, INDEX_NONE,
				TEXT("Project Settings > Game > Bathhouse Building의 벽 두께, 판 두께, 조각 최대 크기는 0보다 큰 유한한 값이어야 합니다."));
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
				if (!Stair.bHasStepMaterial || !Stair.bHasStairWallMaterial)
				{
					AddProblem(Out, EBathhouseProblemCode::MaterialMissing, EBathhouseProblemSeverity::Warning, I, INDEX_NONE, StairIndex,
						FString::Printf(TEXT("%s: %s이(가) 비었습니다. 엔진 기본 격자 재질로 보입니다."), *Label,
							(!Stair.bHasStepMaterial && !Stair.bHasStairWallMaterial) ? TEXT("계단 판 재질과 계단 벽 재질")
							: (!Stair.bHasStepMaterial ? TEXT("계단 판 재질") : TEXT("계단 벽 재질"))));
				}
				if (StairIndex > 0)
				{
					AddProblem(Out, EBathhouseProblemCode::MaterialMissing, EBathhouseProblemSeverity::Warning, I, INDEX_NONE, StairIndex,
						FString::Printf(TEXT("%s: 형상은 첫 번째 계단 항목의 재질만 씁니다. 이 항목의 계단 재질은 무시됩니다."), *Label));
				}
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
}

void FBathhouseSpaceValidation::ValidateLayout(
	const TArray<FBathhouseSpaceSnapshot>& Snapshots, const FBathhouseValidationInputs& Inputs, TArray<FBathhouseLayoutProblem>& OutProblems)
{
	OutProblems.Reset();
	BathhouseSpaceValidationDetail::CollectProblems(Snapshots, Inputs, OutProblems);
	BathhouseSpaceValidationDetail::AppendSuggestions(Snapshots, Inputs, OutProblems);
}
