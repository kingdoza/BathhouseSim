#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrashCollectionZoneActor.generated.h"
class UBoxComponent;
UCLASS(Blueprintable)

class BATHHOUSESIM_API ATrashCollectionZoneActor : public AActor
{
	GENERATED_BODY()
public:

	ATrashCollectionZoneActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	void CollectNow();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trash Collection")
	TObjectPtr<UBoxComponent> CollectionBounds;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trash Collection", meta = (ClampMin = "1.0"))
	float CollectionIntervalSeconds = 300;

private:

	FTimerHandle CollectionTimer;
};
