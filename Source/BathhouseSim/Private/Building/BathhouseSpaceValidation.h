#pragma once

#include "CoreMinimal.h"
#include "Building/BathhouseSpaceLayout.h"

class UWorld;

enum class EBathhouseProblemSeverity : uint8
{
	Error,
	Warning
};

enum class EBathhouseProblemCode : uint8
{
	KindMissing,
	KindDuplicate,
	TransformInvalid,
	ValueInvalid,
	TagsEmpty,
	Overlap,
	OpeningInvalid,
	OpeningOutsideWall,
	OpeningOverlap,
	PassageInvalid,
	PassageNotTouching,
	PassageRangeOutside,
	PassageFloorZ,
	PassageHeight,
	OutsideOpeningBlocked,
	HallNoEntrance,
	WorkNoStairs,
	StairInvalid,
	StairOutside,
	StairLanding,
	StairSteep,
	StairBlocked,
	BoxMeshMissing,
	ChunkClassMissing,
	NavOutside,
	NavWorkCovered,
	MaterialMissing,
	WorkChunkKind,
	FacilityMisplaced
};

struct FBathhouseLayoutProblem
{
	EBathhouseProblemCode Code = EBathhouseProblemCode::KindMissing;
	EBathhouseProblemSeverity Severity = EBathhouseProblemSeverity::Error;
	/** 문제를 로그로 남기는 공간 인덱스(없으면 INDEX_NONE). */
	int32 OwnerIndex = INDEX_NONE;
	/** 관련된 다른 공간(통로·겹침 상대). */
	int32 OtherIndex = INDEX_NONE;
	/** 통로·출입구·계단 문제의 owner 쪽 목록 index. */
	int32 ItemIndex = INDEX_NONE;
	FText Message;

	bool Involves(const int32 Index) const { return OwnerIndex == Index || OtherIndex == Index; }
};

/** 검증이 읽는 Settings·엔진 값. 순수 검증은 값을 인자로 받는다. */
struct FBathhouseValidationInputs
{
	FBathhouseLayoutValues Layout;
	bool bBoxMeshValid = false;
	bool bLitterClassSet = false;
	bool bStainClassSet = false;
	double WalkableFloorAngleDegrees = 0.0;
};

/** 맞닿게 하는 Location 제안. 이동할 공간과 이동량(0이 아닌 축만 제안). */
struct FBathhouseLocationSuggestion
{
	int32 MoveIndex = INDEX_NONE;
	FVector2D DeltaXY = FVector2D::ZeroVector;
	double DeltaZ = 0.0;
	FVector NewLocation = FVector::ZeroVector;
};

class FBathhouseSpaceValidation
{
public:
	/** 같은 world의 모든 공간 Actor authored 값을 모은다(종류, 이름 순 정렬로 순서 무관). */
	static void GatherSnapshots(UWorld& World, TArray<FBathhouseSpaceSnapshot>& OutSnapshots);
	static FBathhouseValidationInputs ReadInputs();

	/** layout 규칙 검사(순수). 위치 제안 문구를 포함한다. */
	static void ValidateLayout(
		const TArray<FBathhouseSpaceSnapshot>& Snapshots,
		const FBathhouseValidationInputs& Inputs,
		TArray<FBathhouseLayoutProblem>& OutProblems);

	/** Nav 범위 검사(순수). NavBoxes는 NavMeshBoundsVolume bounds. */
	static void ValidateNavigation(
		const TArray<FBathhouseSpaceSnapshot>& Snapshots,
		const TArray<FBox>& NavBoxes,
		const FBathhouseValidationInputs& Inputs,
		TArray<FBathhouseLayoutProblem>& OutProblems);

	/** world 검사: layout + Nav 범위 + 계단 통로 장애물 + 배치된 설비 소속. */
	static void ValidateWorld(
		UWorld& World,
		TArray<FBathhouseSpaceSnapshot>& OutSnapshots,
		TArray<FBathhouseLayoutProblem>& OutProblems);

	/**
	 * A의 Side 변이 B의 반대 변과 맞닿도록 B(bMoveB) 또는 A를 벽 두께만 고려해 옮기는 Location 제안.
	 * 이미 맞닿아 있으면 이동량이 0이다.
	 */
	static bool SuggestTouchingLocation(
		const TArray<FBathhouseSpaceSnapshot>& Snapshots,
		int32 AIndex,
		int32 BIndex,
		EBathhouseSpaceSide Side,
		double WallThicknessCm,
		bool bMoveB,
		FBathhouseLocationSuggestion& OutSuggestion);

	/** 한 이동을 snapshot 복사본에 적용한다. */
	static void ApplyMove(FBathhouseSpaceSnapshot& Snapshot, const FVector2D& DeltaXY, double DeltaZ);
};
