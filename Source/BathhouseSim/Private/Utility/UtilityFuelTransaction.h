#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractionTypes.h"
#include "Utility/UtilityFuelTypes.h"

class AUtilityFuelSupplyActor;
class AUtilityShovelActor;
class UPrimitiveComponent;
class UPlayerInteractionComponent;
class UUtilityFuelIntakeVolumeComponent;

/** Synchronous fuel movement. Every mutation publishes only after all owners commit. */
class FUtilityFuelTransaction
{
public:
	static FUtilityFuelResult EvaluateScoop(
		const FPlayerInteractionContext& Context,
		const AUtilityFuelSupplyActor& Supply);
	static FUtilityFuelResult EvaluateReturn(
		const FPlayerInteractionContext& Context,
		const AUtilityFuelSupplyActor& Supply);
	static FUtilityFuelResult EvaluateInsert(
		const FPlayerInteractionContext& Context,
		const UUtilityFuelIntakeVolumeComponent& Intake);

	static FUtilityFuelResult Scoop(
		AUtilityShovelActor& Shovel,
		AUtilityFuelSupplyActor& Supply,
		const FPlayerInteractionContext& Context);
	static FUtilityFuelResult Insert(
		AUtilityShovelActor& Shovel,
		UUtilityFuelIntakeVolumeComponent& Intake,
		const FPlayerInteractionContext& Context);
	static FUtilityFuelResult Return(
		AUtilityShovelActor& Shovel,
		AUtilityFuelSupplyActor& Supply,
		const FPlayerInteractionContext& Context);

private:
	static AUtilityShovelActor* GetHeldShovel(const FPlayerInteractionContext& Context);
	static bool ValidateFreshHit(
		UPlayerInteractionComponent* Interaction,
		AActor* User,
		AActor* ExpectedActor,
		UPrimitiveComponent* ExpectedComponent,
		FText& OutFailureReason);
	static bool ValidateHeldContext(
		const AUtilityShovelActor& Shovel,
		const FPlayerInteractionContext& Context,
		FText& OutFailureReason);
};
