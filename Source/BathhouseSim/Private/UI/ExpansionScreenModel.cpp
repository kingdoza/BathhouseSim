#include "UI/ExpansionScreenModel.h"

#include "Internationalization/Text.h"

#define LOCTEXT_NAMESPACE "ExpansionScreen"

namespace
{
	/** 문구 형식 상수: cm를 m로 바꾸는 단위 변환. */
	constexpr double CentimetersPerMeter = 100.0;
}

FText FExpansionScreenModel::SpaceName(const EBathhouseSpaceKind Kind)
{
	switch (Kind)
	{
	case EBathhouseSpaceKind::Hall: return LOCTEXT("SpaceHall", "홀");
	case EBathhouseSpaceKind::Bath: return LOCTEXT("SpaceBath", "목욕공간");
	default: return LOCTEXT("SpaceWork", "작업공간");
	}
}

FText FExpansionScreenModel::FormatSize(const FVector2D& SizeCm)
{
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 0;
	Options.MaximumFractionalDigits = 1;
	Options.UseGrouping = false;
	return FText::Format(LOCTEXT("SizeFormat", "{0}m×{1}m"),
		FText::AsNumber(SizeCm.X / CentimetersPerMeter, &Options),
		FText::AsNumber(SizeCm.Y / CentimetersPerMeter, &Options));
}

FText FExpansionScreenModel::FormatMoney(const int32 Amount)
{
	return FText::Format(LOCTEXT("MoneyFormat", "{0}원"), FText::AsNumber(Amount));
}

FExpansionScreenDisplay FExpansionScreenModel::Build(const FBathhouseExpansionView& View, const FExpansionScreenState& State)
{
	FExpansionScreenDisplay Display;
	Display.Balance = FText::Format(LOCTEXT("Balance", "잔액 {0}"), FormatMoney(View.Balance));
	Display.bBalanceVisible = true;

	if (!View.bAvailable)
	{
		Display.Message = LOCTEXT("Unavailable", "확장을 사용할 수 없습니다");
		Display.bMessageVisible = true;
		Display.bClearSelection = true;
		return Display;
	}

	Display.Stage = FText::Format(LOCTEXT("Stage", "현재 확장 단계: {0}"), FText::AsNumber(View.PurchaseCount));
	Display.bStageVisible = true;
	Display.Locker = FText::Format(LOCTEXT("Locker", "설치된 락커 칸 {0}/{1}"),
		FText::AsNumber(View.InstalledLockerSlots), FText::AsNumber(View.LockerSlotLimit));
	Display.bLockerVisible = true;
	Display.Result = LOCTEXT("Completed", "확장 완료");
	Display.bResultVisible = State.bShowCompleted;

	if (View.bMaxReached)
	{
		Display.Message = LOCTEXT("MaxReached", "최대 확장 단계입니다");
		Display.bMessageVisible = true;
		Display.bClearSelection = true;
		return Display;
	}

	bool bSelectedUsable = false;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FBathhouseExpansionOptionView& Option = View.Options[Index];
		FExpansionOptionDisplay& Out = Display.Options[Index];
		Out.Name = SpaceName(Option.Kind);
		const bool bUsable = Option.bPresent && Option.bCanExpand;
		if (bUsable)
		{
			Out.Size = FText::Format(LOCTEXT("SizeChange", "{0} → {1}"),
				FormatSize(Option.CurrentSizeCm), FormatSize(Option.NextSizeCm));
			Out.bEnabled = true;
			if (Option.bHasHallEffect)
			{
				Out.Effect = FText::Format(
					LOCTEXT("HallEffect", "열쇠 {0}개 → {1}개\n락커 칸 한도 {2}칸 → {3}칸"),
					FText::AsNumber(Option.KeysNow), FText::AsNumber(Option.KeysNext),
					FText::AsNumber(Option.LockerLimitNow), FText::AsNumber(Option.LockerLimitNext));
				Out.bEffectVisible = true;
			}
		}
		else
		{
			Out.Size = Option.bPresent ? FormatSize(Option.CurrentSizeCm) : FText::GetEmpty();
			Out.Status = LOCTEXT("CannotExpand", "이 공간은 더 넓힐 수 없습니다");
			Out.bStatusVisible = true;
		}
		const bool bIsSelected = State.Selected.IsSet() && State.Selected.GetValue() == Option.Kind;
		Out.bSelected = bIsSelected && bUsable;
		if (bIsSelected)
		{
			bSelectedUsable = bUsable;
			if (!bUsable)
			{
				Display.bClearSelection = true;
			}
		}
	}
	Display.bOptionsVisible = true;

	Display.Price = FText::Format(LOCTEXT("Price", "이번 구입 가격 {0}"), FormatMoney(View.NextPrice));
	Display.bPriceVisible = true;
	Display.PurchaseButtonText = FText::Format(LOCTEXT("PurchaseButton", "확장 구입 ({0})"), FormatMoney(View.NextPrice));
	if (View.Shortfall > 0)
	{
		Display.Shortfall = FText::Format(LOCTEXT("Shortfall", "{0} 부족"), FormatMoney(View.Shortfall));
		Display.bShortfallVisible = true;
	}

	const bool bCanPurchase = bSelectedUsable && View.Shortfall == 0;
	if (State.bConfirmPending && !Display.bClearSelection)
	{
		Display.bConfirmPanelVisible = true;
		Display.bConfirmEnabled = bCanPurchase;
	}
	else
	{
		Display.bPurchasePanelVisible = true;
		Display.bPurchaseEnabled = bCanPurchase;
	}
	return Display;
}

#undef LOCTEXT_NAMESPACE
