#pragma once

#include "CoreMinimal.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Facility/FacilityPlacementExtension.h"
#include "Facility/BathhouseExpansionAuthority.h"
#include "Service/DisplayFacilityTargetComponent.h"
#include "ServiceFacilityAutomationTestProbe.generated.h"

class UServiceItemDefinition;
class UServiceDisplayManagerComponent;
class UDisplaySpaceComponent;

/** SCS-like components must not exist until construction/finalize. */
UCLASS(Transient, NotBlueprintable)

class AServiceAutomationDisplayFacility : public ABathhouseFacilityActor
{
	GENERATED_BODY()
public:

	AServiceAutomationDisplayFacility();
	virtual void OnConstruction(const FTransform& Transform) override;
	UPROPERTY()
	TArray<TObjectPtr<UServiceItemDefinition>> FixtureKinds;

	class UFacilityPlacementComponent* GetPlacementForTest() const
	{
		return FacilityPlacement;
	}

	UServiceDisplayManagerComponent* GetManager() const;
	UDisplayFacilityTargetComponent* GetRouter() const;
	TArray<UDisplaySpaceComponent*> GetSpaces() const;
};

UCLASS(Transient, NotBlueprintable)

class AServiceAutomationShower final : public AServiceAutomationDisplayFacility
{
	GENERATED_BODY()
public:

	AServiceAutomationShower();
};

UCLASS(Transient, NotBlueprintable)

class UServiceAutomationExtensionData final : public UFacilityPlacementExtensionData
{
	GENERATED_BODY()
public:

	int32 Value = 0;
};

/** Independent extension verifies the generic validate-all-before-apply boundary. */
UCLASS(Transient, NotBlueprintable)

class UServiceAutomationExtension final : public UActorComponent, public IFacilityPlacementExtension
{
	GENERATED_BODY()
public:

	FName FixtureKey = TEXT("Other");
	bool bReject = false;
	int32 Value = 0;
	int32 ApplyCalls = 0;

	virtual FName GetPlacementExtensionKey() const override
	{
		return FixtureKey;
	}

	virtual UFacilityPlacementExtensionData* ExportPlacementExtension(UObject* Outer) const override;
	virtual bool ValidatePlacementExtension(const UFacilityPlacementExtensionData* Data, FText& Failure) const override;
	virtual void ApplyPlacementExtension(const UFacilityPlacementExtensionData* Data) override;

	virtual bool ValidateExtensionAuthoring(TConstArrayView<UActorComponent*> Components, FText& Failure) const override
	{
		return true;
	}
};

/** Same UObject with a changing key; independent of the corrected DISP-024 path. */
UCLASS(Transient, NotBlueprintable)

class UServiceAutomationKeyTarget final : public UDisplayFacilityTargetComponent
{
	GENERATED_BODY()
public:

	int32 Key = 0;
	int32 Executions = 0;
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteHeldTargetUse(const FPlayerInteractionContext& Context,
														  EPlayerHeldTargetUseDirection Direction) override;
};

UCLASS(Transient, NotBlueprintable)

class AServiceAutomationExpansion final : public ABathhouseExpansionAuthority
{
	GENERATED_BODY()
public:

	void ConfigureForTest(class UBathhouseExpansionDefinition* Definition)
	{
		ExpansionDefinition = Definition;
		InitialTierIndex = 0;
	}
};
