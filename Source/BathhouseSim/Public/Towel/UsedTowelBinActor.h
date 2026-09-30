#pragma once

#include "CoreMinimal.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Interaction/PlayerInteractable.h"
#include "Interaction/PlayerInteractionFocusObserver.h"
#include "UsedTowelBinActor.generated.h"

class AWorldUsedTowelActor;
class UTowelInventoryComponent;
class UTowelStackVisualComponent;
class UDisplayCueComponent;

UCLASS(Blueprintable)

class BATHHOUSESIM_API AUsedTowelBinActor : public ABathhouseFacilityActor, public IPlayerInteractionFocusObserver
{
	GENERATED_BODY()

public:
	AUsedTowelBinActor();
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

	bool TryStageOverflowTowel(AWorldUsedTowelActor*& OutTowel);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Towel|Presentation")
	TObjectPtr<UDisplayCueComponent> DisplayCue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Towel")
	TObjectPtr<UTowelInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Towel|Presentation")
	TObjectPtr<UTowelStackVisualComponent> TowelPresentationVisual;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Towel|Overflow", meta = (ClampMin = "0.0"))
	float OverflowMinRadius = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Towel|Overflow", meta = (ClampMin = "0.0"))
	float OverflowMaxRadius = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Towel|Overflow", meta = (ClampMin = "1.0"))
	float FloorTraceDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Towel|Overflow", meta = (ClampMin = "1"))
	int32 PlacementAttempts = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Towel|Overflow", meta = (ClampMin = "0.0"))
	float TowelSpacing = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Towel|Overflow", meta = (ClampMin = "0.0"))
	float PawnClearance = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Towel|Overflow")
	TEnumAsByte<ECollisionChannel> FloorTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Towel|Overflow")
	TSubclassOf<AWorldUsedTowelActor> WorldUsedTowelClass;

private:
	FPlayerInteractionResult TransferToHeldBasket(
		const FPlayerInteractionContext& Context,
		EPlayerHeldTargetUseDirection Direction);
};
