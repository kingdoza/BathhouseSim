#pragma once

#include "CoreMinimal.h"
#include "Building/BathhouseSpaceTypes.h"

/** 확장 구입 시도의 결과. None이 성공이다. */
enum class EBathhouseExpansionFailure : uint8
{
	None,
	Unavailable,
	Busy,
	StaleState,
	SpaceUnavailable,
	SpaceMaxReached,
	InsufficientMoney,
	ApplyFailed
};

/** 확장 화면의 공간 선택지 한 칸. 순서는 EBathhouseSpaceKind(홀, 목욕공간, 작업공간)다. */
struct FBathhouseExpansionOptionView
{
	EBathhouseSpaceKind Kind = EBathhouseSpaceKind::Hall;
	/** 그 종류의 공간이 등록돼 있다. */
	bool bPresent = false;
	/** 공간별 상한 전이고 다음 줄을 적용할 수 있다. */
	bool bCanExpand = false;
	/** 이 공간을 넓힌 횟수와 줄 수(공간별 상한). */
	int32 AppliedCount = 0;
	int32 StepCount = 0;
	/** 줄을 모두 적용했다. */
	bool bAtLimit = false;
	/** bCanExpand일 때만 다음 줄 가격(원), 아니면 0. */
	int32 NextPrice = 0;
	FVector2D CurrentSizeCm = FVector2D::ZeroVector;
	FVector2D NextSizeCm = FVector2D::ZeroVector;
	/** 홀만 참. 열쇠와 락커 칸 한도의 전후 값. */
	bool bHasHallEffect = false;
	int32 KeysNow = 0;
	int32 KeysNext = 0;
	int32 LockerLimitNow = 0;
	int32 LockerLimitNext = 0;
};

/** 확장 화면이 매번 구입 subsystem에서 받는 읽기 전용 값. */
struct FBathhouseExpansionView
{
	bool bAvailable = false;
	/** transaction 진행 중. 화면은 이 view를 적용하지 않는다. */
	bool bBusy = false;
	/** 등록된 공간이 하나 이상이고 등록된 공간 모두 상한에 닿았다. */
	bool bAllAtLimit = false;
	int32 Balance = 0;
	int32 InstalledLockerSlots = 0;
	int32 LockerSlotLimit = 0;
	FBathhouseExpansionOptionView Options[3];

	FBathhouseExpansionView()
	{
		Options[0].Kind = EBathhouseSpaceKind::Hall;
		Options[1].Kind = EBathhouseSpaceKind::Bath;
		Options[2].Kind = EBathhouseSpaceKind::Work;
	}
};
