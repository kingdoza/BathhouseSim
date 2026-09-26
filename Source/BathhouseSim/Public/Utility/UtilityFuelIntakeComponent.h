#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "UtilityFuelIntakeComponent.generated.h"

UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UUtilityFuelIntakeComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UUtilityFuelIntakeComponent();
	bool HasValidAuthoring(FText& OutFailureReason) const;
};
