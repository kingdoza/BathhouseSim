#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Interaction/PlayerInteractable.h"
#include "Interaction/PlayerInteractionFocusObserver.h"
#include "TowelTransferPortComponent.generated.h"

UCLASS(ClassGroup = (Towel), meta = (BlueprintSpawnableComponent))

class BATHHOUSESIM_API UTowelTransferPortComponent : public UBoxComponent,
													 public IPlayerInteractable,
													 public IPlayerInteractionFocusObserver
{
	GENERATED_BODY()

public:
	UTowelTransferPortComponent();
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void NotifyInteractionFocusChanged(const UPlayerInteractionComponent& Source,
											   const FPlayerInteractionQuery& Query) override;
	virtual void NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source) override;
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	virtual FPlayerInteractionResult ExecuteHeldTargetUse(
		const FPlayerInteractionContext& Context,
		EPlayerHeldTargetUseDirection Direction) override;

private:
	FPlayerInteractionResult Transfer(
		const FPlayerInteractionContext& Context,
		EPlayerHeldTargetUseDirection Direction);
};
