#pragma once

#include "CoreMinimal.h"
#include "Interaction/HeldEquipmentUsable.h"
#include "Utility/BathWaterBoilerFacilityActor.h"
#include "Utility/UtilityFuelTypes.h"
#include "UtilityLaborAutomationTestProbe.generated.h"

class AUtilityShovelActor;
class UUtilityFuelIntakeComponent;
class UUtilityOperationComponent;

UCLASS(Transient, NotBlueprintable)
class AUtilityLaborBoilerAutomationActor final : public ABathWaterBoilerFacilityActor
{
	GENERATED_BODY()

public:
	AUtilityLaborBoilerAutomationActor();
};

UCLASS(Transient, NotBlueprintable)
class UUtilityLaborFuelChangedAutomationProbe final : public UObject
{
	GENERATED_BODY()

public:
	void Bind(
		AUtilityShovelActor* InSourceShovel,
		AUtilityShovelActor* InNestedShovel,
		UUtilityOperationComponent* InOperation,
		UUtilityFuelIntakeComponent* InIntake,
		const FHeldEquipmentUseContext& InContext,
		float InExpectedOperationPoints);
	void Unbind();

	int32 CallbackCount = 0;
	bool bSawCompleteCommittedState = false;
	bool bReentrantInsertRejectedByGuard = false;

private:
	UFUNCTION()
	void HandleFuelLoadChanged(FUtilityFuelLoad Load);

	UPROPERTY(Transient)
	TObjectPtr<AUtilityShovelActor> SourceShovel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AUtilityShovelActor> NestedShovel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUtilityOperationComponent> Operation = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUtilityFuelIntakeComponent> Intake = nullptr;

	UPROPERTY(Transient)
	FHeldEquipmentUseContext Context;

	float ExpectedOperationPoints = 0.0f;
};
