#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Service/ServiceItemTypes.h"

class UServiceItemDefinition;

struct FServiceTransferEvaluation
{
	bool bCan = false;
	FText Reason;
};

/**
 * Pure item-box <-> display-space rules. Evaluate* never mutates; Try* re-evaluates and then moves exactly one unit,
 * changing both stacks and both revisions, or neither. Broadcasting is the caller's job after commit.
 * Stack order is LIFO for both stacks: put at index Count, remove index Count - 1.
 */
class FServiceItemTransfer
{
public:
	static FServiceTransferEvaluation EvaluateApply(
		const FServiceItemStack& Box,
		const FServiceItemStack& Space,
		const FGameplayTag& SpaceCategory,
		int32 SpaceCapacity);

	static FServiceTransferEvaluation EvaluateTake(
		const FServiceItemStack& Box,
		const FServiceItemStack& Space,
		const FGameplayTag& SpaceCategory,
		int32 SpaceCapacity);

	static bool TryApplyOne(
		FServiceItemStack& Box,
		FServiceItemStack& Space,
		const FGameplayTag& SpaceCategory,
		int32 SpaceCapacity,
		FText& OutFailureReason);

	static bool TryTakeOne(
		FServiceItemStack& Box,
		FServiceItemStack& Space,
		const FGameplayTag& SpaceCategory,
		int32 SpaceCapacity,
		FText& OutFailureReason);

	/** Customer removal from a display space. Bumps the revision and unlocks the kind at zero. */
	static bool TryRemoveOne(FServiceItemStack& Space, UServiceItemDefinition*& OutKind, FText& OutFailureReason);

	static int32 GetBoxCapacity(const FServiceItemStack& Box, const FServiceItemStack& Space);
	static bool IsConsistent(const FServiceItemStack& Stack);
};
