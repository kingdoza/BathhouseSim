#include "Towel/TowelDisplayCueUtils.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Towel/TowelBasketActor.h"
#include "Towel/TowelInventoryComponent.h"
#include "Towel/Presentation/TowelQuantityVisualComponent.h"
#include "Service/DisplayCueComponent.h"

namespace TowelDisplayCueUtils
{
	int64 GetPresentationRevision(const FPlayerInteractionContext& Context, const UTowelInventoryComponent* Inventory)
	{
		const ATowelBasketActor* Basket =
			Context.CarryComponent ? Cast<ATowelBasketActor>(Context.CarryComponent->GetHeldObject()) : nullptr;
		const int64 TargetRevision = Inventory ? Inventory->GetSnapshot().Revision : 0;
		const int64 BasketRevision =
			Basket && Basket->GetInventory() ? Basket->GetInventory()->GetSnapshot().Revision : 0;
		return TargetRevision + BasketRevision;
	}

	void Update(const UPlayerInteractionComponent& Source, const FPlayerInteractionQuery& Query,
				const UTowelInventoryComponent* Inventory, const UTowelQuantityVisualComponent* Visual,
				UDisplayCueComponent* Cue)
	{
		if (!Cue)
		{
			return;
		}
		Cue->HideAll();
		if (!Inventory || !Visual)
		{
			return;
		}
		const auto Snapshot = Inventory->GetSnapshot();
		UStaticMesh* Mesh = nullptr;
		FTransform Transform;
		if (Query.bHeldApplyVisible && Query.bCanHeldApply)
		{
			const auto* Carry =
				Source.GetOwner() ? Source.GetOwner()->FindComponentByClass<UPlayerCarryComponent>() : nullptr;
			const auto* Basket = Carry ? Cast<ATowelBasketActor>(Carry->GetHeldObject()) : nullptr;
			if (Basket && Basket->GetInventory() &&
				Visual->GetIndexPresentation(Snapshot.Count, Basket->GetInventory()->GetSnapshot().State, Mesh,
											 Transform))
			{
				Cue->ShowInsertPreview(Mesh, Transform);
			}
		}
		if (Query.bHeldTakeVisible && Query.bCanHeldTake &&
			Visual->GetIndexPresentation(Snapshot.Count - 1, Snapshot.State, Mesh, Transform))
		{
			Cue->ShowTakeHighlight(Mesh, Transform);
		}
	}
} // namespace TowelDisplayCueUtils
