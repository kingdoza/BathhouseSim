#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CleaningDirectorActor.generated.h"

class AStainSpawnZoneActor;
class AWaterStainActor;
class ALitterActor;
class ALitterSpawnZoneActor;

UCLASS(Blueprintable)
class BATHHOUSESIM_API ACleaningDirectorActor : public AActor
{
	GENERATED_BODY()

public:
	ACleaningDirectorActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// C++ 전용: 구역 후보 질의가 같은 값을 쓰도록 하는 테스트·호출자용 getter
	float GetSpawnClearanceHeightCm() const { return SpawnClearanceHeightCm; }
	float GetSpawnClearanceFloorOffsetCm() const { return SpawnClearanceFloorOffsetCm; }

#if WITH_DEV_AUTOMATION_TESTS
	void AdvanceSpawnScheduleForTesting(float DeltaSeconds, const TArray<FVector>& CustomerLocations);
	void SetSpawnRandomSeedForTesting(int32 Seed);
#endif
protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning Spawn", meta = (ClampMin = "0.05"))
	float SpawnUpdateIntervalSeconds = 0.25f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning Spawn", meta = (ClampMin = "1.0"))
	float LitterMeanIntervalPerCustomerSeconds = 120.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning Spawn", meta = (ClampMin = "1"))
	int32 MaxActiveLitter = 20;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning Spawn")
	TSubclassOf<ALitterActor> LitterClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning Spawn", meta = (ClampMin = "0.0"))
	float DefaultLitterSpacing = 40.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning Spawn", meta = (ClampMin = "0.1"))
	float SpawnClearanceHeightCm = 30.0f;
	/** clearance box를 바닥 접점에서 띄우는 높이 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning Spawn", meta = (ClampMin = "0.0"))
	float SpawnClearanceFloorOffsetCm = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning Spawn",
			  meta = (ClampMin = "0.1", ToolTip = "Mean interval per customer (seconds)."))
	float SpawnIntervalSeconds = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning Spawn", meta = (ClampMin = "1"))
	int32 MaxActiveStains = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning Spawn", meta = (ClampMin = "1"))
	int32 MaxPlacementAttemptsPerInterval = 12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning Spawn")
	TSubclassOf<AWaterStainActor> StainClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cleaning Spawn", meta = (ClampMin = "0.0"))
	float DefaultStainSpacing = 100.0f;

private:

	friend class FBathhouseCleaningInteractionTest;

	void UpdateSpawnSchedule();
	void AdvanceSpawnSchedule(float DeltaSeconds, const TArray<FVector>& CustomerLocations);
	void TrySpawnStain(AStainSpawnZoneActor& Zone);
	void TrySpawnLitter(ALitterSpawnZoneActor& Zone);
	TMap<TWeakObjectPtr<AStainSpawnZoneActor>, float> StainClocks;
	TMap<TWeakObjectPtr<ALitterSpawnZoneActor>, float> LitterClocks;
	FRandomStream SpawnRandom;
	FTimerHandle SpawnTimerHandle;
};
