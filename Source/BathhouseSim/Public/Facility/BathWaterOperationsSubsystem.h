#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "BathWaterOperationsSubsystem.generated.h"

class ABathhouseBathFacilityActor;
class UBathWaterConditionComponent;
class UBathWaterUtilityCapacityComponent;

DECLARE_MULTICAST_DELEGATE(FOnBathWaterOperationsChanged);

UCLASS()
class BATHHOUSESIM_API UBathWaterOperationsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	bool RegisterProvider(UBathWaterUtilityCapacityComponent* Provider, bool bPublish = true);
	bool UnregisterProvider(UBathWaterUtilityCapacityComponent* Provider, bool bPublish = true, bool bUnexpected = false);
	bool RegisterBath(UBathWaterConditionComponent* Condition, bool bPublish = true);
	bool UnregisterBath(UBathWaterConditionComponent* Condition, bool bPublish = true);
	bool CanRemoveProvider(const UBathWaterUtilityCapacityComponent* Provider, float& OutDeficitPoints);

	FBathWaterSettingRequestResult RequestCirculationPercent(
		ABathhouseBathFacilityActor* Bath,
		float RequestedPercent);
	FBathWaterSettingRequestResult RequestTargetTemperature(
		ABathhouseBathFacilityActor* Bath,
		float RequestedTemperatureC);

	FBathWaterCapacitySnapshot GetCapacitySnapshot(EBathWaterCapacityKind Kind);
	FBathWaterOperationsSnapshot GetSnapshot();
	bool GetBathSnapshot(const ABathhouseBathFacilityActor* Bath, FBathWaterBathSnapshot& OutSnapshot);
	bool IsCapacitySatisfied(EBathWaterCapacityKind Kind);
	uint64 GetTopologyRevision() const { return TopologyRevision; }
	uint64 GetDataRevision() const { return DataRevision; }
	void PublishMutation(bool bTopologyChanged = false, bool bCapacityChanged = false);

	FOnBathWaterOperationsChanged OnOperationsChanged;

private:
	struct FDemandTotals
	{
		float Circulation = 0.0f;
		float Heating = 0.0f;
		float Cooling = 0.0f;
	};

	bool CompactInvalidEntries();
	FBathWaterCapacitySnapshot BuildCapacitySnapshot(EBathWaterCapacityKind Kind) const;
	bool BuildBathSnapshot(const ABathhouseBathFacilityActor* Bath, FBathWaterBathSnapshot& OutSnapshot) const;
	float CalculateTotalCapacity(EBathWaterCapacityKind Kind) const;
	FDemandTotals CalculateDemandTotals(const UBathWaterConditionComponent* ReplacedCondition = nullptr,
		float CandidateCirculation = 0.0f, float CandidateTarget = 0.0f, bool bUseCandidate = false) const;
	UBathWaterConditionComponent* FindCondition(const ABathhouseBathFacilityActor* Bath) const;
	static EBathWaterRequestFailure FailureForKind(EBathWaterCapacityKind Kind);
	void BroadcastMutation();

	TArray<TWeakObjectPtr<UBathWaterUtilityCapacityComponent>> Providers;
	TArray<TWeakObjectPtr<UBathWaterConditionComponent>> Baths;
	uint64 TopologyRevision = 0;
	uint64 CapacityRevision = 0;
	uint64 DataRevision = 0;
	bool bPublishing = false;
	bool bPublishQueued = false;
};
