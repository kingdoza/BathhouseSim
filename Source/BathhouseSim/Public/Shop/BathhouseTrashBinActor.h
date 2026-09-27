#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PlayerInteractable.h"
#include "BathhouseTrashBinActor.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class BATHHOUSESIM_API ABathhouseTrashBinActor : public AActor, public IPlayerInteractable
{
	GENERATED_BODY()

public:
	ABathhouseTrashBinActor();
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UStaticMeshComponent> BinMesh;
};
