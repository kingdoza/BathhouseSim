#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interaction/InteractionTypes.h"
#include "PlayerInteractionFocusObserver.generated.h"

class UPlayerInteractionComponent;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UPlayerInteractionFocusObserver : public UInterface
{
	GENERATED_BODY()
};

class BATHHOUSESIM_API IPlayerInteractionFocusObserver
{
	GENERATED_BODY()

public:
	virtual void NotifyInteractionFocusChanged(
		const UPlayerInteractionComponent& Source,
		const FPlayerInteractionQuery& Query) = 0;
	virtual void NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source) = 0;
};
