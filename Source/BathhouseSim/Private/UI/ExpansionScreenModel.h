#pragma once

#include "CoreMinimal.h"
#include "Building/BathhouseExpansionTypes.h"
#include "Misc/Optional.h"

/** 공간 선택지 한 칸의 표시 값. */
struct FExpansionOptionDisplay
{
	FText Name;
	FText Stage;
	FText Size;
	FText Price;
	FText Effect;
	FText Status;
	bool bStageVisible = false;
	bool bPriceVisible = false;
	bool bEffectVisible = false;
	bool bStatusVisible = false;
	bool bEnabled = false;
	bool bSelected = false;
};

/** 확장 화면의 위젯 표시 상태(domain 값이 아니다). */
struct FExpansionScreenState
{
	TOptional<EBathhouseSpaceKind> Selected;
	bool bConfirmPending = false;
	bool bShowCompleted = false;
};

/** 모델이 계산한 문구·활성·가시성. 위젯은 그대로 적용만 한다. */
struct FExpansionScreenDisplay
{
	FText Balance;
	FText Locker;
	bool bBalanceVisible = true;
	bool bLockerVisible = false;
	FExpansionOptionDisplay Options[3];
	bool bOptionsVisible = false;
	bool bPurchasePanelVisible = false;
	FText PurchaseButtonText;
	bool bPurchaseEnabled = false;
	bool bConfirmPanelVisible = false;
	bool bConfirmEnabled = false;
	FText Shortfall;
	bool bShortfallVisible = false;
	FText Result;
	bool bResultVisible = false;
	FText Message;
	bool bMessageVisible = false;
	/** 선택이 더 이상 유효하지 않다(위젯이 선택과 확인 대기를 지운다). */
	bool bClearSelection = false;
};

/** view + 표시 상태 -> 표시 값. 순수 계산이며 world·domain을 읽지 않는다. */
class FExpansionScreenModel
{
public:
	static FExpansionScreenDisplay Build(const FBathhouseExpansionView& View, const FExpansionScreenState& State);

	/** `{X}m×{Y}m`. cm를 m로 바꿔 world X를 먼저, 소수 최대 1자리, 천 단위 구분 없음. */
	static FText FormatSize(const FVector2D& SizeCm);
	/** `{N}원`, 천 단위 구분. */
	static FText FormatMoney(int32 Amount);
	static FText SpaceName(EBathhouseSpaceKind Kind);
};
