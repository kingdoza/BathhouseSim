#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Interaction/PlayerInteractionFocusObserver.h"
#include "Interaction/PlayerInteractable.h"
#include "UtilityFuelIntakeVolumeComponent.generated.h"

class UPlayerInteractionComponent;
class UUtilityFuelDoorComponent;

UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UUtilityFuelIntakeVolumeComponent
	: public UBoxComponent
	, public IPlayerInteractable
	, public IPlayerInteractionFocusObserver
{
	GENERATED_BODY()

public:
	UUtilityFuelIntakeVolumeComponent();

	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	virtual void NotifyInteractionFocusChanged(
		const UPlayerInteractionComponent& Source,
		const FPlayerInteractionQuery& Query) override;
	virtual void NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source) override;

	void SetFuelDoorPresentation(UUtilityFuelDoorComponent* InPresentation);
	bool HasValidAuthoring(FText& OutFailureReason) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UUtilityFuelDoorComponent> FuelDoorPresentation = nullptr;
};
