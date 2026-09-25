#pragma once

#include "CoreMinimal.h"
#include "Interaction/HeldEquipmentUsable.h"
#include "Interaction/InteractionTypes.h"
#include "Utility/UtilityFuelTypes.h"

class AUtilityFuelSupplyActor;
class AUtilityShovelActor;
class UPrimitiveComponent;
class UPlayerInteractionComponent;
class UUtilityFuelIntakeComponent;

/** Synchronous two-owner fuel movement. All observers are notified after every owner is committed. */
class FUtilityFuelTransaction
{
public:
	static FUtilityFuelResult Scoop(
		AUtilityShovelActor& Shovel,
		AUtilityFuelSupplyActor& Supply,
		const FHeldEquipmentUseContext& Context);
	static FUtilityFuelResult Insert(
		AUtilityShovelActor& Shovel,
		UUtilityFuelIntakeComponent& Intake,
		const FHeldEquipmentUseContext& Context);
	static FUtilityFuelResult Return(
		AUtilityShovelActor& Shovel,
		AUtilityFuelSupplyActor& Supply,
		const FPlayerInteractionContext& Context);

private:
	static bool ValidateFreshHit(
		UPlayerInteractionComponent* Interaction,
		AActor* User,
		AActor* ExpectedActor,
		UPrimitiveComponent* ExpectedComponent,
		FText& OutFailureReason);
	static bool ValidateHeldContext(
		const AUtilityShovelActor& Shovel,
		const FHeldEquipmentUseContext& Context,
		FText& OutFailureReason);
	static bool ValidateHeldContext(
		const AUtilityShovelActor& Shovel,
		const FPlayerInteractionContext& Context,
		FText& OutFailureReason);
	static bool ValidateOwnerInput(AActor* User, FText& OutFailureReason);
};
