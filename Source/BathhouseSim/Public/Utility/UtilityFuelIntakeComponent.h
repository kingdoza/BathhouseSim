#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/PlayerInteractable.h"
#include "UtilityFuelIntakeComponent.generated.h"

UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UUtilityFuelIntakeComponent
	: public UStaticMeshComponent
	, public IPlayerInteractable
{
	GENERATED_BODY()

public:
	UUtilityFuelIntakeComponent();

	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	bool HasValidAuthoring(FText& OutFailureReason) const;
};
