#pragma once

#include "CoreMinimal.h"
#include "Cleaning/CleaningTypes.h"
#include "GameFramework/Actor.h"
#include "StainSpawnZoneActor.generated.h"

class UBoxComponent;
class USceneComponent;

UCLASS(Blueprintable)
class BATHHOUSESIM_API AStainSpawnZoneActor : public AActor
{
	GENERATED_BODY()

public:
	AStainSpawnZoneActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool FindSpawnTransform(FRandomStream& RandomStream, float DefaultStainSpacing, FTransform& OutTransform,
							float FloorRadius, float ClearanceHeight, float ClearanceFloorOffset) const;

	UBoxComponent* GetSpawnBounds() const
	{
		return SpawnBounds;
	}
	int32 GetMaxActiveStains() const { return MaxActiveStainsInZone; }

protected:
	friend class FBathhouseCleaningInteractionTest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cleaning Zone")
	TObjectPtr<UBoxComponent> SpawnBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cleaning Zone")
	TObjectPtr<USceneComponent> SpawnFloor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning Zone", meta = (ClampMin = "0.0"))
	float FloorHeightToleranceCm = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning Zone", meta = (ClampMin = "1"))
	int32 MaxActiveStainsInZone = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning Zone")
	TEnumAsByte<ECollisionChannel> FloorTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning Zone", meta = (ClampMin = "1.0"))
	float FloorTraceDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning Zone")
	FName RequiredFloorComponentTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning Zone", meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float MaximumFloorSlopeDegrees = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning Zone", meta = (ClampMin = "0.0"))
	float StainSpacingOverride = 0.0f;
};
