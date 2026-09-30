#pragma once

#include "CoreMinimal.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Interaction/PlayerInteractable.h"
#include "Interaction/PlayerInteractionFocusObserver.h"
#include "CleanTowelStackActor.generated.h"

class UTowelInventoryComponent;
class UTowelStackVisualComponent;
class UDisplayCueComponent;

UCLASS(Blueprintable)

class BATHHOUSESIM_API ACleanTowelStackActor : public ABathhouseFacilityActor, public IPlayerInteractionFocusObserver
{
	GENERATED_BODY()

public:
	ACleanTowelStackActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool SupportsFacilityActorConversion() const override { return false; }
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	virtual FPlayerInteractionResult ExecuteHeldTargetUse(
		const FPlayerInteractionContext& Context,
		EPlayerHeldTargetUseDirection Direction) override;

	virtual void NotifyInteractionFocusChanged(const UPlayerInteractionComponent& Source,
											   const FPlayerInteractionQuery& Query) override;
	virtual void NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source) override;

	UDisplayCueComponent* GetDisplayCue() const
	{
		return DisplayCue;
	}

	UFUNCTION(BlueprintPure, Category = "Towel")
	UTowelInventoryComponent* GetInventory() const { return Inventory; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Towel|Presentation")
	TObjectPtr<UDisplayCueComponent> DisplayCue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Towel")
	TObjectPtr<UTowelInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Towel|Presentation")
	TObjectPtr<UTowelStackVisualComponent> TowelPresentationVisual;

private:
	FPlayerInteractionResult TransferFromHeldBasket(
		const FPlayerInteractionContext& Context,
		EPlayerHeldTargetUseDirection Direction);
};
