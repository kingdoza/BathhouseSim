#pragma once

#include "CoreMinimal.h"
#include "Service/DisplaySpaceComponent.h"
#include "Service/DrinkFridgeActor.h"
#include "ServiceAutomationTestProbe.generated.h"

class UBathhouseFacilitySlotComponent;

/** Display space whose authoring fields the tests can set without a Blueprint. */
UCLASS(Transient, NotBlueprintable)
class UServiceAutomationDisplaySpace final : public UDisplaySpaceComponent
{
	GENERATED_BODY()

public:
	void ConfigureForTest(int32 InSpaceIndex, const FGameplayTag& InCategory, int32 SlotCount);
};

/**
 * Drink fridge whose display spaces and customer slot are created during construction (OnConstruction), like
 * Blueprint SCS components, instead of as native default subobjects. Four 3-slot spaces.
 */
UCLASS(Transient, NotBlueprintable)
class AServiceAutomationConstructedFridge final : public ADrinkFridgeActor
{
	GENERATED_BODY()

public:
	AServiceAutomationConstructedFridge();
	virtual void OnConstruction(const FTransform& Transform) override;
	class UFacilityPlacementComponent* GetPlacementForTest() const { return FacilityPlacement; }
	void GetSpacesForTest(TArray<UDisplaySpaceComponent*>& OutSpaces) const { GetComponents<UDisplaySpaceComponent>(OutSpaces); }
	UBathhouseFacilitySlotComponent* GetSlotForTest() const;
};

/** Drink fridge with two 3-slot display spaces and one customer slot. Meshes are engine cubes. */
UCLASS(Transient, NotBlueprintable)
class AServiceAutomationFridge : public ADrinkFridgeActor
{
	GENERATED_BODY()

public:
	AServiceAutomationFridge();

	UDisplaySpaceComponent* GetSpaceA() const { return SpaceA; }
	UDisplaySpaceComponent* GetSpaceB() const { return SpaceB; }
	UBathhouseFacilitySlotComponent* GetCustomerSlotForTest() const { return CustomerSlot; }
	class UFacilityPlacementComponent* GetPlacementForTest() const { return FacilityPlacement; }

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UServiceAutomationDisplaySpace> SpaceA;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UServiceAutomationDisplaySpace> SpaceB;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBathhouseFacilitySlotComponent> CustomerSlot;
};
