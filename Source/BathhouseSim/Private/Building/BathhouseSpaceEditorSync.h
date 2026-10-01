#pragma once

#include "CoreMinimal.h"

class UWorld;

#if WITH_EDITOR
/**
 * 편집 world의 공간 형상 지연 일괄 재생성. 공간 하나가 바뀌면 다음 tick에 한 번 모든 공간의 shell만 다시 만든다.
 * OnConstruction을 다시 부르지 않고 transaction·Modify를 하지 않는다.
 */
class FBathhouseSpaceEditorSync
{
public:
	/** EWorldType::Editor world에서만 동작한다. world별로 다음 tick 한 번(coalesce). */
	static void RequestRebuild(UWorld* World);
	/** 지금 모든 공간의 shell을 다시 만든다(테스트와 tick이 쓴다). */
	static void RebuildNow(UWorld* World);
	static bool HasPendingRequest(const UWorld* World);
};
#endif
