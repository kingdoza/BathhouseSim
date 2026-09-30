#pragma once
#include "CoreMinimal.h"
#include "Interaction/InteractionTypes.h"
class UPlayerInteractionComponent;
class UTowelInventoryComponent;
class UTowelQuantityVisualComponent;
class UDisplayCueComponent;

namespace TowelDisplayCueUtils
{
	int64 GetPresentationRevision(const FPlayerInteractionContext& Context, const UTowelInventoryComponent* Inventory);

	void Update(const UPlayerInteractionComponent& Source, const FPlayerInteractionQuery& Query,
				const UTowelInventoryComponent* Inventory, const UTowelQuantityVisualComponent* Visual,
				UDisplayCueComponent* Cue);
}
