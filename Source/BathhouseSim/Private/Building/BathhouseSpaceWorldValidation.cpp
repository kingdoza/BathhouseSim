#include "Building/BathhouseSpaceValidation.h"

#include "Building/BathhouseBuildingSettings.h"
#include "Building/BathhouseSpaceActor.h"
#include "Building/BathhouseSpaceValidationInternal.h"
#include "CollisionQueryParams.h"
#include "Engine/StaticMesh.h"
#include "Facility/BathhouseExpansionAuthority.h"
#include "Facility/BathhouseExpansionDefinition.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/PlaceableFacility.h"

void FBathhouseSpaceValidation::ValidateNavigation(
	const TArray<FBathhouseSpaceSnapshot>& S,
	const TArray<FBox>& NavBoxes,
	const FBathhouseValidationInputs& Inputs,
	TArray<FBathhouseLayoutProblem>& Out)
{
	using namespace BathhouseSpaceValidationDetail;
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

void FBathhouseSpaceValidation::GatherSnapshots(UWorld& World, TArray<FBathhouseSpaceSnapshot>& OutSnapshots)
{
	using namespace BathhouseSpaceValidationDetail;
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
	using namespace BathhouseSpaceValidationDetail;
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
	UWorld& World, TArray<FBathhouseSpaceSnapshot>& OutSnapshots, TArray<FBathhouseLayoutProblem>& OutProblems)
{
	using namespace BathhouseSpaceValidationDetail;
	GatherSnapshots(World, OutSnapshots);
	const FBathhouseValidationInputs Inputs = ReadInputs();
	// 미리보기·적용 횟수와 무관하게 0회 모습을 검사한다. 넓힘은 ValidateExpansion이 따로 본다.
	TArray<FBathhouseSpaceSnapshot> BaseSnapshots;
	for (const FBathhouseSpaceSnapshot& Snapshot : OutSnapshots)
	{
		BaseSnapshots.Add(FBathhouseSpaceLayout::WithExpansionCount(Snapshot, 0));
	}
	ValidateLayout(BaseSnapshots, Inputs, OutProblems);
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
	ValidateNavigation(BaseSnapshots, NavBoxes, Inputs, OutProblems);

	int32 HallEffectRowCount = INDEX_NONE;
	for (TActorIterator<ABathhouseExpansionAuthority> It(&World); It; ++It)
	{
		if (IsValid(*It) && It->GetExpansionDefinition())
		{
			HallEffectRowCount = It->GetExpansionDefinition()->Tiers.Num();
			break;
		}
	}
	ValidateExpansion(OutSnapshots, NavBoxes, Inputs, HallEffectRowCount, OutProblems);

	// 계단 통로(구멍 안)를 공간이 아닌 blocking 물체(지형 등)가 막는지 수직 trace로 본다.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	// 게임 충돌 기준인 단순 충돌만 본다. 기본 query param의 complex trace는 편집 world의 WorldPartition HLOD 지형 mesh 등
	// 게임에 없는 삼각형 충돌까지 맞혀 지형 구멍이 뚫려 있어도 오류를 남긴다.
	FCollisionQueryParams StairQueryParams(SCENE_QUERY_STAT(BathhouseStairShaftObstacle), false);
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
			const double Bottom = FBathhouseSpaceLayout::CeilingZ(Lower);
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
						Hits, FVector(Point.X, Point.Y, Top), FVector(Point.X, Point.Y, Bottom), ObjectParams, StairQueryParams);
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
		for (int32 I = 0; I < BaseSnapshots.Num(); ++I)
		{
			const FBathhouseSpaceSnapshot& Space = BaseSnapshots[I];
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
