#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PlayerInteractable.h"
#include "LitterActor.generated.h"
class ALitterSpawnZoneActor;
class USphereComponent;
class UStaticMeshComponent;
class UStaticMesh;
UCLASS(Blueprintable)

class BATHHOUSESIM_API ALitterActor : public AActor, public IPlayerInteractable
{
	GENERATED_BODY()
public:

	ALitterActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	void ConfigureVisualVariationSeed(int32 Seed);
	void SetSpawnZone(ALitterSpawnZoneActor* Zone);

	ALitterSpawnZoneActor* GetSpawnZone() const
	{
		return SpawnZone.Get();
	}

	bool IsActive() const
	{
		return !bRemoved;
	}

	float GetFloorRadius() const
	{
		return FloorRadiusCm;
	}

	float GetPlacementClearHeightToleranceCm() const
	{
		return PlacementClearHeightToleranceCm;
	}

	bool CommitCollected();
	void ClearForFacilityPlacement();
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Litter")
	TObjectPtr<USphereComponent> InteractionCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Litter")
	TObjectPtr<UStaticMeshComponent> LitterMesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter")
	TArray<TObjectPtr<UStaticMesh>> MeshVariants;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter", meta = (ClampMin = "0.1"))
	float FloorRadiusCm = 15;
	/** 설비 배치 확정 시 발밑 정리 판정의 Z 허용 오차 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Litter", meta = (ClampMin = "0.0"))
	float PlacementClearHeightToleranceCm = 5.0f;

private:

	TWeakObjectPtr<ALitterSpawnZoneActor> SpawnZone;
	int32 VisualSeed = 0;
	bool bSeedConfigured = false;
	bool bVisualInitialized = false;
	bool bRemoved = false;
};
