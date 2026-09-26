#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Interaction/PlayerInteractionFocusObserver.h"
#include "Interaction/PlayerInteractable.h"
#include "UtilityLeverOperatingVolumeComponent.generated.h"

class UUtilityLeverLaborComponent;
class UPlayerInteractionComponent;

UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UUtilityLeverOperatingVolumeComponent
	: public UBoxComponent
	, public IPlayerInteractable
	, public IPlayerInteractionFocusObserver
{
	GENERATED_BODY()

public:
	UUtilityLeverOperatingVolumeComponent();
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	virtual void NotifyInteractionFocusChanged(
		const UPlayerInteractionComponent& Source,
		const FPlayerInteractionQuery& Query) override;
	virtual void NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source) override;

	void SetLeverLabor(UUtilityLeverLaborComponent* InLeverLabor);
	bool HasValidAuthoring(FText& OutFailureReason) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UUtilityLeverLaborComponent> LeverLabor = nullptr;
};
