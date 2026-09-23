#pragma once

#include "CoreMinimal.h"
#include "BathWaterOperationsTypes.generated.h"

class ABathhouseBathFacilityActor;

UENUM(BlueprintType)
enum class EBathWaterCapacityKind : uint8
{
	Circulation = 0,
	Heating = 1,
	Cooling = 2
};

UENUM(BlueprintType)
enum class EBathWaterThermalStatus : uint8
{
	Empty = 0,
	ReturningToAmbient = 1,
	Stalled = 2,
	MovingToTarget = 3,
	MaintainingTarget = 4,
	SuspendedByCapacity = 5
};

UENUM(BlueprintType)
enum class EBathWaterRequestFailure : uint8
{
	None = 0,
	InvalidTarget = 1,
	InvalidNumber = 2,
	InsufficientCirculationCapacity = 3,
	InsufficientHeatingCapacity = 4,
	InsufficientCoolingCapacity = 5,
	UnavailableBath = 6
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FBathWaterCapacitySnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Bath Water")
	EBathWaterCapacityKind Kind = EBathWaterCapacityKind::Circulation;

	UPROPERTY(BlueprintReadOnly, Category = "Bath Water")
	float UsedPoints = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Bath Water")
	float TotalPoints = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Bath Water")
	float DeficitPoints = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Bath Water")
	int64 Revision = 0;

	bool IsSatisfied() const { return DeficitPoints <= KINDA_SMALL_NUMBER; }
};

USTRUCT()
struct BATHHOUSESIM_API FBathWaterBathSnapshot
{
	GENERATED_BODY()

	TWeakObjectPtr<ABathhouseBathFacilityActor> BathActor;
	FTransform FootprintTransform = FTransform::Identity;
	FVector FootprintHalfExtent = FVector::ZeroVector;
	float WaterPercent = 0.0f;
	float CirculationPercent = 0.0f;
	float TargetTemperatureC = 20.0f;
	float ActualTemperatureC = 20.0f;
	float ContaminationPercent = 0.0f;
	float CirculationDemandPoints = 0.0f;
	float HeatingDemandPoints = 0.0f;
	float CoolingDemandPoints = 0.0f;
	float ThermalThresholdPercent = 0.0f;
	EBathWaterThermalStatus ThermalStatus = EBathWaterThermalStatus::Empty;
	bool bCirculationCapacityDeficit = false;
	bool bHeatingCapacityDeficit = false;
	bool bCoolingCapacityDeficit = false;
	uint64 Revision = 0;
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FBathWaterSettingRequestResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Bath Water")
	bool bSucceeded = false;

	UPROPERTY(BlueprintReadOnly, Category = "Bath Water")
	bool bWasLimited = false;

	UPROPERTY(BlueprintReadOnly, Category = "Bath Water")
	float RequestedValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Bath Water")
	float CommittedValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Bath Water")
	EBathWaterCapacityKind LimitedKind = EBathWaterCapacityKind::Circulation;

	UPROPERTY(BlueprintReadOnly, Category = "Bath Water")
	float RequiredAdditionalPoints = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Bath Water")
	EBathWaterRequestFailure Failure = EBathWaterRequestFailure::None;
};

USTRUCT()
struct BATHHOUSESIM_API FBathWaterOperationsSnapshot
{
	GENERATED_BODY()

	FBathWaterCapacitySnapshot Circulation;
	FBathWaterCapacitySnapshot Heating;
	FBathWaterCapacitySnapshot Cooling;
	TArray<FBathWaterBathSnapshot> Baths;
	uint64 TopologyRevision = 0;
	uint64 DataRevision = 0;
};
