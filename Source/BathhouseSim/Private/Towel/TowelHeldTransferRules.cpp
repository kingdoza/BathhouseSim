#include "Towel/TowelHeldTransferRules.h"

#define LOCTEXT_NAMESPACE "TowelHeldTransferRules"

FTowelHeldTransferDirection FTowelHeldTransferRules::EvaluateDirection(
	const bool bApply,
	const FText& ActionName,
	const FText& TargetName,
	const FTowelInventorySnapshot& TargetSnapshot,
	const bool bHasBasket,
	const FTowelInventorySnapshot& BasketSnapshot,
	const ETowelState RequiredState,
	const bool bMachineProcessing,
	const EPlayerInteractionActivationMode ActivationMode)
{
	FTowelHeldTransferDirection Result;
	Result.bVisible = true;
	Result.ActionName = ActionName;
	Result.ActivationMode = ActivationMode;
	if (!bHasBasket)
	{
		Result.FailureReason = LOCTEXT("BasketRequired", "수건 바구니 필요");
		return Result;
	}
	if (bMachineProcessing)
	{
		Result.FailureReason = LOCTEXT("MachineProcessing", "작동 중");
		return Result;
	}
	if (BasketSnapshot.Count > 0 && BasketSnapshot.State != RequiredState)
	{
		Result.FailureReason = LOCTEXT("StateMismatch", "다른 상태의 수건");
		return Result;
	}

	if (bApply)
	{
		if (BasketSnapshot.Count <= 0)
		{
			Result.FailureReason = LOCTEXT("BasketEmpty", "바구니 비어 있음");
			return Result;
		}
		if (TargetSnapshot.Count >= TargetSnapshot.Capacity)
		{
			Result.FailureReason = FText::Format(
				LOCTEXT("TargetFull", "{0} 가득 참"),
				TargetName);
			return Result;
		}
	}
	else
	{
		if (TargetSnapshot.Count <= 0)
		{
			Result.FailureReason = FText::Format(
				LOCTEXT("TargetEmpty", "{0} 비어 있음"),
				TargetName);
			return Result;
		}
		if (BasketSnapshot.Count >= BasketSnapshot.Capacity)
		{
			Result.FailureReason = LOCTEXT("BasketFull", "바구니 가득 참");
			return Result;
		}
	}
	Result.bCanUse = true;
	return Result;
}

FTowelHeldTransferQuery FTowelHeldTransferRules::Build(
	const ETowelHeldTransferTargetKind TargetKind,
	const FText& TargetName,
	const FTowelInventorySnapshot& TargetSnapshot,
	const bool bHasBasket,
	const FTowelInventorySnapshot& BasketSnapshot,
	const ETowelMachineState MachineState,
	const ETowelState MachineInputState,
	const ETowelState MachineOutputState)
{
	FTowelHeldTransferQuery Result;
	const EPlayerInteractionActivationMode RepeatMode = EPlayerInteractionActivationMode::Repeat;
	const FText ApplyName = LOCTEXT("ApplyAction", "넣기");
	const FText TakeName = TargetKind == ETowelHeldTransferTargetKind::WorldUsedTowel
		? LOCTEXT("CollectAction", "줍기")
		: LOCTEXT("TakeAction", "빼기");

	switch (TargetKind)
	{
	case ETowelHeldTransferTargetKind::CleanShelf:
		Result.Apply = EvaluateDirection(
			true, ApplyName, TargetName, TargetSnapshot, bHasBasket, BasketSnapshot,
			ETowelState::Clean, false, RepeatMode);
		Result.Take.FailureReason = LOCTEXT("ShelfTakeUnavailable", "여기서는 꺼낼 수 없음");
		Result.Take.ActivationMode = RepeatMode;
		break;

	case ETowelHeldTransferTargetKind::UsedBin:
		Result.Apply.FailureReason = LOCTEXT("BinApplyUnavailable", "여기에는 넣을 수 없음");
		Result.Apply.ActivationMode = RepeatMode;
		Result.Take = EvaluateDirection(
			false, TakeName, TargetName, TargetSnapshot, bHasBasket, BasketSnapshot,
			ETowelState::Used, false, RepeatMode);
		break;

	case ETowelHeldTransferTargetKind::MachinePort:
		if (MachineState == ETowelMachineState::Waiting)
		{
			Result.Apply = EvaluateDirection(
				true, ApplyName, TargetName, TargetSnapshot, bHasBasket, BasketSnapshot,
				MachineInputState, false, RepeatMode);
			Result.Take.bVisible = true;
			Result.Take.ActionName = TakeName;
			Result.Take.FailureReason = LOCTEXT("WaitingTakeUnavailable", "완료 후 뺄 수 있음");
			Result.Take.ActivationMode = RepeatMode;
		}
		else if (MachineState == ETowelMachineState::Complete)
		{
			Result.Apply.bVisible = true;
			Result.Apply.ActionName = ApplyName;
			Result.Apply.FailureReason = LOCTEXT("CompleteApplyUnavailable", "비운 뒤 넣을 수 있음");
			Result.Apply.ActivationMode = RepeatMode;
			Result.Take = EvaluateDirection(
				false, TakeName, TargetName, TargetSnapshot, bHasBasket, BasketSnapshot,
				MachineOutputState, false, RepeatMode);
		}
		else
		{
			Result.Apply = EvaluateDirection(
				true, ApplyName, TargetName, TargetSnapshot, bHasBasket, BasketSnapshot,
				MachineInputState, true, RepeatMode);
			Result.Take = EvaluateDirection(
				false, TakeName, TargetName, TargetSnapshot, bHasBasket, BasketSnapshot,
				MachineOutputState, true, RepeatMode);
		}
		break;

	case ETowelHeldTransferTargetKind::WorldUsedTowel:
		Result.Apply.FailureReason = LOCTEXT("WorldTowelApplyUnavailable", "여기에는 넣을 수 없음");
		Result.Apply.ActivationMode = EPlayerInteractionActivationMode::Instant;
		Result.Take = EvaluateDirection(
			false, TakeName, TargetName, TargetSnapshot, bHasBasket, BasketSnapshot,
			ETowelState::Used, false, EPlayerInteractionActivationMode::Instant);
		break;
	}
	return Result;
}

void FTowelHeldTransferRules::ApplyToQuery(
	FPlayerInteractionQuery& InteractionQuery,
	const FTowelHeldTransferQuery& TransferQuery)
{
	InteractionQuery.bHeldApplyVisible = TransferQuery.Apply.bVisible;
	InteractionQuery.bCanHeldApply = TransferQuery.Apply.bCanUse;
	InteractionQuery.HeldApplyActionName = TransferQuery.Apply.ActionName;
	InteractionQuery.HeldApplyFailureReason = TransferQuery.Apply.FailureReason;
	InteractionQuery.HeldApplyActivationMode = TransferQuery.Apply.ActivationMode;
	InteractionQuery.bHeldTakeVisible = TransferQuery.Take.bVisible;
	InteractionQuery.bCanHeldTake = TransferQuery.Take.bCanUse;
	InteractionQuery.HeldTakeActionName = TransferQuery.Take.ActionName;
	InteractionQuery.HeldTakeFailureReason = TransferQuery.Take.FailureReason;
	InteractionQuery.HeldTakeActivationMode = TransferQuery.Take.ActivationMode;
}

#undef LOCTEXT_NAMESPACE