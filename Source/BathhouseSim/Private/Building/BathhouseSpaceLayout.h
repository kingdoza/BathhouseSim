#pragma once

#include "CoreMinimal.h"
#include "Building/BathhouseSpaceTypes.h"
#include "Math/Box2D.h"

class ABathhouseSpaceActor;

/** 개구부 snapshot. 위치는 공간 Actor 기준 상대값이고 world 값은 helper가 계산한다. */
struct FBathhouseOpeningSnapshot
{
	EBathhouseSpaceSide Side = EBathhouseSpaceSide::East;
	double CenterOffsetCm = 0.0;
	double WidthCm = 0.0;
	double HeightCm = 0.0;
	TWeakObjectPtr<const ABathhouseSpaceActor> ConnectedActor;
	int32 ConnectedIndex = INDEX_NONE;
};

struct FBathhouseStairSnapshot
{
	TWeakObjectPtr<const ABathhouseSpaceActor> LowerActor;
	int32 LowerIndex = INDEX_NONE;
	FVector2D TopEdgeOffsetCm = FVector2D::ZeroVector;
	EBathhouseSpaceSide DownSide = EBathhouseSpaceSide::East;
	double WidthCm = 0.0;
	double RunCm = 0.0;
	int32 StepCount = 0;
	double GuardHeightCm = 0.0;
	bool bHasStepMaterial = false;
	bool bHasStairWallMaterial = false;
};

/** 공간 Actor의 authored 값만 담은 입력. 다른 공간의 생성 component는 읽지 않는다. */
struct FBathhouseSpaceSnapshot
{
	TWeakObjectPtr<const ABathhouseSpaceActor> Actor;
	FString DisplayName;
	EBathhouseSpaceKind Kind = EBathhouseSpaceKind::Hall;
	FVector2D ActorXY = FVector2D::ZeroVector;
	double FloorZ = 0.0;
	FBox2D Interior = FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector);
	double CeilingHeightCm = 0.0;
	bool bTransformValid = true;
	bool bAllowedTagsEmpty = false;
	double LightSpacingCm = 0.0;
	double LightCeilingOffsetCm = 0.0;
	EBathhouseCleaningChunkKind ChunkKind = EBathhouseCleaningChunkKind::None;
	bool bHasWallMaterial = true;
	bool bHasFloorMaterial = true;
	bool bHasCeilingMaterial = true;
	TArray<FBathhouseOpeningSnapshot> Openings;
	TArray<FBathhouseStairSnapshot> Stairs;
};

/** 순수 계산이 받는 Settings 값. layout은 world·Settings를 직접 읽지 않는다. */
struct FBathhouseLayoutValues
{
	double WallThicknessCm = 0.0;
	double SlabThicknessCm = 0.0;
	FVector2D ChunkMaxSizeCm = FVector2D::ZeroVector;
};

struct FBathhouseBoxPart
{
	FVector Center = FVector::ZeroVector;
	FVector HalfExtent = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
};

struct FBathhouseSpacePlan
{
	TArray<FBathhouseBoxPart> Parts[static_cast<int32>(EBathhouseShellPart::Count)];
	TArray<FVector> LightLocations;
	TArray<FBox2D> ChunkRects;

	TArray<FBathhouseBoxPart>& Get(EBathhouseShellPart Part) { return Parts[static_cast<int32>(Part)]; }
	const TArray<FBathhouseBoxPart>& Get(EBathhouseShellPart Part) const { return Parts[static_cast<int32>(Part)]; }
};

/** 계단 frame: 원점 = 위층 Actor 위치 + 맨 위 가장자리 offset, +x = DownSide, y = 폭 방향. */
struct FBathhouseStairFrame
{
	FVector2D Origin = FVector2D::ZeroVector;
	FVector2D Dir = FVector2D::ZeroVector;
	FVector2D Perp = FVector2D::ZeroVector;

	FVector2D ToWorld(double X, double Y) const { return Origin + Dir * X + Perp * Y; }
	/** frame 직사각형을 감싸는 world 직사각형. */
	FBox2D ToWorldRect(double X0, double X1, double Y0, double Y1) const;
};

class FBathhouseSpaceLayout
{
public:
	static FVector2D SideNormal(EBathhouseSpaceSide Side);
	static EBathhouseSpaceSide Opposite(EBathhouseSpaceSide Side);
	static bool IsAlongY(EBathhouseSpaceSide Side) { return Side == EBathhouseSpaceSide::East || Side == EBathhouseSpaceSide::West; }

	/** 바깥 직사각형 O = 안쪽 직사각형을 사방 두께만큼 넓힌 것. */
	static FBox2D OuterRect(const FBathhouseSpaceSnapshot& Space, double WallThicknessCm);
	static double CeilingZ(const FBathhouseSpaceSnapshot& Space) { return Space.FloorZ + Space.CeilingHeightCm; }

	/** 개구부 world 구간(벽 길이 방향 좌표). */
	static FVector2D OpeningWorldInterval(const FBathhouseSpaceSnapshot& Space, const FBathhouseOpeningSnapshot& Opening);

	static FBathhouseStairFrame MakeStairFrame(const FBathhouseSpaceSnapshot& Upper, const FBathhouseStairSnapshot& Stair);
	/** 구멍 R(world XY). */
	static FBox2D StairHole(const FBathhouseSpaceSnapshot& Upper, const FBathhouseStairSnapshot& Stair);

	/** guillotine 분할로 Base에서 Holes를 뺀 겹치지 않는 직사각형 목록. */
	static void SubtractRects(const FBox2D& Base, const TArray<FBox2D>& Holes, TArray<FBox2D>& OutRects);
	/** 직사각형을 축마다 ceil(크기/최대)개의 같은 칸으로 나눈다. */
	static void SplitChunks(const FBox2D& Rect, const FVector2D& MaxSize, TArray<FBox2D>& OutRects);
	/** 안쪽 직사각형을 간격 이하의 같은 칸으로 나눈 칸 중심. */
	static void SplitLightCenters(const FBox2D& Rect, double SpacingCm, TArray<FVector2D>& OutCenters);

	static FBathhouseSpacePlan BuildPlan(
		const TArray<FBathhouseSpaceSnapshot>& Snapshots, int32 Index, const FBathhouseLayoutValues& Values);
	static void Build(
		const TArray<FBathhouseSpaceSnapshot>& Snapshots,
		const FBathhouseLayoutValues& Values,
		TArray<FBathhouseSpacePlan>& OutPlans);
};
