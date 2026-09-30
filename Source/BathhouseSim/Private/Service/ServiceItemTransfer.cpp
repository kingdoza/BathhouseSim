#include "Service/ServiceItemTransfer.h"

#include "Service/ServiceItemDefinition.h"
#include "Service/DisplayStockRules.h"

#define LOCTEXT_NAMESPACE "ServiceItemTransfer"

namespace
{
FServiceTransferEvaluation Deny(const FText& Reason)
{
	FServiceTransferEvaluation Result;
	Result.Reason = Reason;
	return Result;
}

bool AcceptsKind(const UServiceItemDefinition* Kind, const FGameplayTag& Category, const UServiceItemDefinition* FixedKind)
{
	return FixedKind ? Kind == FixedKind : Kind && Category.IsValid() && Kind->DisplayCategories.HasTag(Category);
}
}

bool FServiceItemTransfer::IsConsistent(const FServiceItemStack& Stack)
{
	return FDisplayStockRules::IsConsistent(Stack);
}

int32 FServiceItemTransfer::GetBoxCapacity(const FServiceItemStack& Box, const FServiceItemStack& Space)
{
	if (Box.Kind)
	{
		return Box.Kind->BoxCapacity;
	}
	return Space.Kind ? Space.Kind->BoxCapacity : 0;
}

FServiceTransferEvaluation FServiceItemTransfer::EvaluateApply(
	const FServiceItemStack& Box,
	const FServiceItemStack& Space,
	const FGameplayTag& SpaceCategory,
	const int32 SpaceCapacity,
	const UServiceItemDefinition* FixedKind)
{
	if (!IsConsistent(Box) || !IsConsistent(Space))
	{
		return Deny(LOCTEXT("InconsistentStack", "물건 상태를 확인할 수 없음"));
	}
	if (Box.Kind && !AcceptsKind(Box.Kind, SpaceCategory, FixedKind))
	{
		return Deny(LOCTEXT("CategoryRejected", "여기에 넣을 수 없는 물건"));
	}
	if (Box.Kind && Space.Kind && Box.Kind != Space.Kind)
	{
		return Deny(LOCTEXT("DifferentKind", "다른 음료가 진열됨"));
	}
	if (Box.Count <= 0)
	{
		return Deny(LOCTEXT("BoxEmpty", "박스가 비어 있음"));
	}
	if (Space.Count >= SpaceCapacity)
	{
		return Deny(LOCTEXT("SpaceFull", "가득 참"));
	}
	FServiceTransferEvaluation Result;
	Result.bCan = true;
	return Result;
}

FServiceTransferEvaluation FServiceItemTransfer::EvaluateTake(
	const FServiceItemStack& Box,
	const FServiceItemStack& Space,
	const FGameplayTag& SpaceCategory,
	const int32 SpaceCapacity,
	const UServiceItemDefinition* FixedKind)
{
	(void)SpaceCapacity;
	if (!IsConsistent(Box) || !IsConsistent(Space))
	{
		return Deny(LOCTEXT("InconsistentStack", "물건 상태를 확인할 수 없음"));
	}
	if (Box.Kind && !AcceptsKind(Box.Kind, SpaceCategory, FixedKind))
	{
		return Deny(LOCTEXT("CategoryRejected", "여기에 넣을 수 없는 물건"));
	}
	if (Box.Kind && Space.Kind && Box.Kind != Space.Kind)
	{
		return Deny(LOCTEXT("DifferentKind", "다른 음료가 진열됨"));
	}
	if (Space.Count <= 0)
	{
		return Deny(LOCTEXT("SpaceEmpty", "꺼낼 물건 없음"));
	}
	if (FDisplayStockRules::GetTakeableCount(Space) == 0)
	{
		return Deny(LOCTEXT("InUseOnly", "사용 중인 것은 꺼낼 수 없음"));
	}
	if (Box.Count >= GetBoxCapacity(Box, Space))
	{
		return Deny(LOCTEXT("BoxFull", "박스 가득 참"));
	}
	FServiceTransferEvaluation Result;
	Result.bCan = true;
	return Result;
}

bool FServiceItemTransfer::TryApplyOne(
	FServiceItemStack& Box,
	FServiceItemStack& Space,
	const FGameplayTag& SpaceCategory,
	const int32 SpaceCapacity,
	FText& OutFailureReason,
	const UServiceItemDefinition* FixedKind)
{
	const FServiceTransferEvaluation Evaluation = EvaluateApply(Box, Space, SpaceCategory, SpaceCapacity, FixedKind);
	if (!Evaluation.bCan)
	{
		OutFailureReason = Evaluation.Reason;
		return false;
	}
	UServiceItemDefinition* Kind = Box.Kind;
	Space.Kind = Kind;
	++Space.Count;
	--Box.Count;
	if (Box.Count == 0)
	{
		Box.Kind = nullptr;
	}
	++Space.Revision;
	++Box.Revision;
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool FServiceItemTransfer::TryTakeOne(
	FServiceItemStack& Box,
	FServiceItemStack& Space,
	const FGameplayTag& SpaceCategory,
	const int32 SpaceCapacity,
	FText& OutFailureReason,
	const UServiceItemDefinition* FixedKind)
{
	const FServiceTransferEvaluation Evaluation = EvaluateTake(Box, Space, SpaceCategory, SpaceCapacity, FixedKind);
	if (!Evaluation.bCan)
	{
		OutFailureReason = Evaluation.Reason;
		return false;
	}
	UServiceItemDefinition* Kind = Space.Kind;
	Box.Kind = Kind;
	++Box.Count;
	--Space.Count;
	if (Space.Count == 0)
	{
		Space.Kind = nullptr;
	}
	++Space.Revision;
	++Box.Revision;
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool FServiceItemTransfer::TryRemoveOne(
	FServiceItemStack& Space,
	UServiceItemDefinition*& OutKind,
	FText& OutFailureReason)
{
	OutKind = nullptr;
	if (!IsConsistent(Space) || Space.Count <= 0)
	{
		OutFailureReason = LOCTEXT("NothingToRemove", "꺼낼 물건 없음");
		return false;
	}
	OutKind = Space.Kind;
	--Space.Count;
	if (Space.Count == 0)
	{
		Space.Kind = nullptr;
	}
	++Space.Revision;
	OutFailureReason = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
