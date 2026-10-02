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
	MaxPurchasesReached,
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
	int32 PurchaseCount = 0;
	int32 MaxPurchaseCount = 0;
	bool bMaxReached = false;
	int32 NextPrice = 0;
	int32 Balance = 0;
	/** max(0, NextPrice - Balance). 최대 도달·사용 불가면 0. */
	int32 Shortfall = 0;
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
