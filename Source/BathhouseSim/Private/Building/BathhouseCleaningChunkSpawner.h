#pragma once

#include "CoreMinimal.h"
#include "Building/BathhouseSpaceTypes.h"

class ABathhouseSpaceActor;

/** 공간 BeginPlay에서 바닥 직사각형 칸마다 청소 조각 구역 Actor 하나를 만든다. */
class FBathhouseCleaningChunkSpawner
{
public:
	/**
	 * Kind가 None이면 아무것도 만들지 않는다. class가 없으면 그 종류만 건너뛰고 오류를 한 번 기록한다.
	 * 만든 조각은 OutChunks에 더한다. Owner는 Space다.
	 */
	static void Spawn(
		ABathhouseSpaceActor& Space,
		EBathhouseCleaningChunkKind Kind,
		const TArray<FBox2D>& Rects,
		double FloorZ,
		TArray<TWeakObjectPtr<AActor>>& OutChunks);
};
