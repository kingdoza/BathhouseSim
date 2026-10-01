#pragma once

#include "CoreMinimal.h"
#include "Cleaning/CleaningTypes.h"
#include "GameFramework/Actor.h"
#include "LitterSpawnZoneActor.generated.h"

class UBoxComponent;
class USceneComponent;

UCLASS(Blueprintable)

class BATHHOUSESIM_API ALitterSpawnZoneActor : public AActor
{
	GENERATED_BODY()

public:

	ALitterSpawnZoneActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool FindSpawnTransform(FRandomStream& RandomStream, float DefaultLitterSpacing, FTransform& OutTransform,
							float FloorRadius, float ClearanceHeight, float ClearanceFloorOffset) const;

	UBoxComponent* GetSpawnBounds() const
	{
		return SpawnBounds;
	}

	int32 GetMaxActiveLitter() const
	{
		return MaxActiveLitterInZone;
	}

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Litter Zone")
	TObjectPtr<UBoxComponent> SpawnBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Litter Zone")
	TObjectPtr<USceneComponent> SpawnFloor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Litter Zone", meta = (ClampMin = "0.0"))
	float FloorHeightToleranceCm = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Litter Zone", meta = (ClampMin = "1"))
	int32 MaxActiveLitterInZone = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Litter Zone")
	TEnumAsByte<ECollisionChannel> FloorTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Litter Zone", meta = (ClampMin = "1.0"))
	float FloorTraceDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Litter Zone")
	FName RequiredFloorComponentTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Litter Zone", meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float MaximumFloorSlopeDegrees = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Litter Zone", meta = (ClampMin = "0.0"))
	float LitterSpacingOverride = 0.0f;
};
