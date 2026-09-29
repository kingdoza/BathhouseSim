#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Interaction/PlayerInteractable.h"
#include "TowelTransferPortComponent.generated.h"

UCLASS(ClassGroup = (Towel), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UTowelTransferPortComponent : public UBoxComponent, public IPlayerInteractable
{
	GENERATED_BODY()

public:
	UTowelTransferPortComponent();
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
