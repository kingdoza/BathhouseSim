#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractionTypes.h"
#include "Towel/TowelTypes.h"

enum class ETowelHeldTransferTargetKind : uint8
{
	CleanShelf,
	UsedBin,
	MachinePort,
	WorldUsedTowel
};

struct FTowelHeldTransferDirection
{
	bool bVisible = false;
	bool bCanUse = false;
	FText ActionName;
	FText FailureReason;
	EPlayerInteractionActivationMode ActivationMode = EPlayerInteractionActivationMode::Instant;
};

struct FTowelHeldTransferQuery
{
	FTowelHeldTransferDirection Apply;
	FTowelHeldTransferDirection Take;
};

class FTowelHeldTransferRules
{
public:
	static FTowelHeldTransferQuery Build(
		ETowelHeldTransferTargetKind TargetKind,
		const FText& TargetName,
		const FTowelInventorySnapshot& TargetSnapshot,
		bool bHasBasket,
		const FTowelInventorySnapshot& BasketSnapshot,
		ETowelMachineState MachineState = ETowelMachineState::Waiting,
		ETowelState MachineInputState = ETowelState::None,
		ETowelState MachineOutputState = ETowelState::None);

	static void ApplyToQuery(FPlayerInteractionQuery& InteractionQuery, const FTowelHeldTransferQuery& TransferQuery);

private:

	static FTowelHeldTransferDirection EvaluateDirection(bool bApply, const FText& ActionName, const FText& TargetName,
														 const FTowelInventorySnapshot& TargetSnapshot, bool bHasBasket,
														 const FTowelInventorySnapshot& BasketSnapshot,
														 ETowelState RequiredState, bool bMachineProcessing,
														 EPlayerInteractionActivationMode ActivationMode,
														 bool bDisplayEmptyReason = false);
};