#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PlayerInteractable.h"
#include "DrinkCollectionBoxActor.generated.h"

class UPlayerWalletComponent;
class UStaticMeshComponent;

/** Fixed level collection box for drink sales. Not carryable, placeable or discardable. */
UCLASS(Blueprintable)
class BATHHOUSESIM_API ADrinkCollectionBoxActor : public AActor, public IPlayerInteractable
{
	GENERATED_BODY()

public:
	ADrinkCollectionBoxActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drink Collection Box")
	TObjectPtr<UStaticMeshComponent> BoxMesh;

private:
	UPlayerWalletComponent* ResolveWallet(const FPlayerInteractionContext& Context) const;

	bool bCollecting = false;
};
