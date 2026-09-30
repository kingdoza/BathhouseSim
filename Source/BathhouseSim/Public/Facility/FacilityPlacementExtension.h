#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FacilityPlacementExtension.generated.h"
class UActorComponent;
UCLASS(Abstract, Transient, NotBlueprintable)

class BATHHOUSESIM_API UFacilityPlacementExtensionData : public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY(Transient)
	FName Key;

	virtual FText GetContentsSummary() const
	{
		return FText::GetEmpty();
	}
};
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))

class UFacilityPlacementExtension : public UInterface
{
	GENERATED_BODY()
};

class BATHHOUSESIM_API IFacilityPlacementExtension
{
	GENERATED_BODY()
public:

	virtual FName GetPlacementExtensionKey() const = 0;
	virtual UFacilityPlacementExtensionData* ExportPlacementExtension(UObject* Outer) const = 0;
	virtual bool ValidatePlacementExtension(const UFacilityPlacementExtensionData* Data, FText& OutFailure) const = 0;
	/** Called only after every extension has passed validation. Must not fail or publish partial state. */
	virtual void ApplyPlacementExtension(const UFacilityPlacementExtensionData* Data) = 0;
	virtual bool ValidateExtensionAuthoring(TConstArrayView<UActorComponent*> OwnerComponents,
											FText& OutFailure) const = 0;
};
