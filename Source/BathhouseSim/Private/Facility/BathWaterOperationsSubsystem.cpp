#include "Facility/BathWaterOperationsSubsystem.h"

#include "Components/BoxComponent.h"
#include "Facility/BathWaterConditionComponent.h"
#include "Facility/BathWaterSettings.h"
#include "Facility/BathWaterStateComponent.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"
#include "Facility/BathhouseBathFacilityActor.h"
#include "Placement/FacilityPlacementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogBathWaterOperations, Log, All);

namespace
{
float DemandFor(
	const UBathWaterConditionComponent& Condition,
	const EBathWaterCapacityKind Kind,
	const float Circulation,
	const float Target,
	const float Ambient)
{
	switch (Kind)
	{
	case EBathWaterCapacityKind::Circulation:
		return Condition.GetMaxCirculationDemandPoints() * FMath::Clamp(Circulation, 0.0f, 100.0f) * 0.01f;
	case EBathWaterCapacityKind::Heating:
		return FMath::Max(Target - Ambient, 0.0f) * Condition.GetHeatingDemandPointsPerC();
	case EBathWaterCapacityKind::Cooling:
		return FMath::Max(Ambient - Target, 0.0f) * Condition.GetCoolingDemandPointsPerC();
	default:
		return 0.0f;
	}
}
}

bool UBathWaterOperationsSubsystem::RegisterProvider(
	UBathWaterUtilityCapacityComponent* Provider,
	const bool bPublish)
{
	const bool bPruned = CompactInvalidEntries();
	FText FailureReason;
	if (!IsValid(Provider) || !Provider->HasValidAuthoring(FailureReason))
	{
		UE_LOG(LogBathWaterOperations, Warning, TEXT("Rejected invalid utility provider %s: %s"),
			*GetNameSafe(Provider), *FailureReason.ToString());
		if (bPruned && bPublish)
		{
			BroadcastMutation();
		}
		return false;
	}
	if (Providers.Contains(Provider))
	{
		if (bPruned && bPublish)
		{
			BroadcastMutation();
		}
		return true;
	}
	Providers.Add(Provider);
	++CapacityRevision;
	++DataRevision;
	UE_LOG(LogBathWaterOperations, Log, TEXT("Registered utility provider %s Kind=%d Points=%.2f"),
		*GetNameSafe(Provider->GetOwner()), static_cast<int32>(Provider->GetCapacityKind()), Provider->GetCapacityPoints());
	if (bPublish)
	{
		BroadcastMutation();
	}
	return true;
}

bool UBathWaterOperationsSubsystem::UnregisterProvider(
	UBathWaterUtilityCapacityComponent* Provider,
	const bool bPublish,
	const bool bUnexpected)
{
	const bool bPruned = CompactInvalidEntries();
	const int32 Removed = Providers.Remove(Provider);
	if (Removed == 0)
	{
		if (bPruned && bPublish)
		{
			BroadcastMutation();
		}
		return false;
	}
	++CapacityRevision;
	++DataRevision;
	UE_LOG(LogBathWaterOperations, Log, TEXT("Unregistered utility provider %s Unexpected=%s"),
		*GetNameSafe(Provider ? Provider->GetOwner() : nullptr), bUnexpected ? TEXT("true") : TEXT("false"));
	if (bPublish)
	{
		BroadcastMutation();
	}
	return true;
}

bool UBathWaterOperationsSubsystem::RegisterBath(
	UBathWaterConditionComponent* Condition,
	const bool bPublish)
{
	const bool bPruned = CompactInvalidEntries();
	FText FailureReason;
	if (!IsValid(Condition) || !Condition->HasValidAuthoring(FailureReason)
		|| !Cast<ABathhouseBathFacilityActor>(Condition->GetOwner()))
	{
		UE_LOG(LogBathWaterOperations, Warning, TEXT("Rejected invalid bath condition %s: %s"),
			*GetNameSafe(Condition), *FailureReason.ToString());
		if (bPruned && bPublish)
		{
			BroadcastMutation();
		}
		return false;
	}
	if (Baths.Contains(Condition))
	{
		if (bPruned && bPublish)
		{
			BroadcastMutation();
		}
		return true;
	}
	Baths.Add(Condition);
	++TopologyRevision;
	++DataRevision;
	UE_LOG(LogBathWaterOperations, Log, TEXT("Registered bath condition %s"), *GetNameSafe(Condition->GetOwner()));
	if (bPublish)
	{
		BroadcastMutation();
	}
	return true;
}

bool UBathWaterOperationsSubsystem::UnregisterBath(
	UBathWaterConditionComponent* Condition,
	const bool bPublish)
{
	const bool bPruned = CompactInvalidEntries();
	if (Baths.Remove(Condition) == 0)
	{
		if (bPruned && bPublish)
		{
			BroadcastMutation();
		}
		return false;
	}
	++TopologyRevision;
	++DataRevision;
	UE_LOG(LogBathWaterOperations, Log, TEXT("Unregistered bath condition %s"),
		*GetNameSafe(Condition ? Condition->GetOwner() : nullptr));
	if (bPublish)
	{
		BroadcastMutation();
	}
	return true;
}

bool UBathWaterOperationsSubsystem::CanRemoveProvider(
	const UBathWaterUtilityCapacityComponent* Provider,
	float& OutDeficitPoints)
{
	if (CompactInvalidEntries())
	{
		BroadcastMutation();
	}
	OutDeficitPoints = 0.0f;
	if (!IsValid(Provider) || !Providers.Contains(Provider))
	{
		return false;
	}
	const FBathWaterCapacitySnapshot Snapshot = GetCapacitySnapshot(Provider->GetCapacityKind());
	const float TotalAfter = FMath::Max(0.0f, Snapshot.TotalPoints - Provider->GetCapacityPoints());
	OutDeficitPoints = FMath::Max(0.0f, Snapshot.UsedPoints - TotalAfter);
	return OutDeficitPoints <= KINDA_SMALL_NUMBER;
}

FBathWaterSettingRequestResult UBathWaterOperationsSubsystem::RequestCirculationPercent(
	ABathhouseBathFacilityActor* Bath,
	const float RequestedPercent)
{
	FBathWaterSettingRequestResult Result;
	Result.RequestedValue = RequestedPercent;
	Result.LimitedKind = EBathWaterCapacityKind::Circulation;
	UBathWaterConditionComponent* Condition = FindCondition(Bath);
	if (!Condition)
	{
		Result.Failure = EBathWaterRequestFailure::UnavailableBath;
		return Result;
	}
	if (!FMath::IsFinite(RequestedPercent))
	{
		Result.Failure = EBathWaterRequestFailure::InvalidNumber;
		return Result;
	}

	const float Requested = FMath::Clamp(RequestedPercent, 0.0f, 100.0f);
	const float Current = Condition->GetCirculationPercent();
	const FDemandTotals CurrentTotals = CalculateDemandTotals();
	const float CurrentDemand = Condition->GetDemandPoints(EBathWaterCapacityKind::Circulation);
	const float CandidateDemand = DemandFor(*Condition, EBathWaterCapacityKind::Circulation,
		Requested, Condition->GetTargetTemperatureC(), GetDefault<UBathWaterSettings>()->GetAmbientTemperatureC());
	const float CandidateUsed = CurrentTotals.Circulation - CurrentDemand + CandidateDemand;
	const float Total = CalculateTotalCapacity(EBathWaterCapacityKind::Circulation);
	float Committed = Requested;
	if (CandidateUsed > Total + KINDA_SMALL_NUMBER && Requested > Current)
	{
		const float AvailableForBath = FMath::Max(0.0f, Total - (CurrentTotals.Circulation - CurrentDemand));
		Committed = Condition->GetMaxCirculationDemandPoints() > KINDA_SMALL_NUMBER
			? FMath::Clamp(AvailableForBath / Condition->GetMaxCirculationDemandPoints() * 100.0f, Current, Requested)
			: Requested;
		Result.bWasLimited = !FMath::IsNearlyEqual(Committed, Requested);
		Result.RequiredAdditionalPoints = FMath::Max(0.0f, CandidateUsed - Total);
		Result.Failure = Result.bWasLimited
			? EBathWaterRequestFailure::InsufficientCirculationCapacity
			: EBathWaterRequestFailure::None;
	}
	Result.bSucceeded = true;
	Result.CommittedValue = Committed;
	if (FMath::IsNearlyEqual(Committed, Current))
	{
		return Result;
	}
	Condition->SetCirculationPercentFromSubsystem(Committed);
	++DataRevision;
	BroadcastMutation();
	UE_LOG(LogBathWaterOperations, Verbose, TEXT("Circulation request Bath=%s Requested=%.2f Committed=%.2f"),
		*GetNameSafe(Bath), Requested, Committed);
	return Result;
}

FBathWaterSettingRequestResult UBathWaterOperationsSubsystem::RequestTargetTemperature(
	ABathhouseBathFacilityActor* Bath,
	const float RequestedTemperatureC)
{
	FBathWaterSettingRequestResult Result;
	Result.RequestedValue = RequestedTemperatureC;
	UBathWaterConditionComponent* Condition = FindCondition(Bath);
	if (!Condition)
	{
		Result.Failure = EBathWaterRequestFailure::UnavailableBath;
		return Result;
	}
	if (!FMath::IsFinite(RequestedTemperatureC))
	{
		Result.Failure = EBathWaterRequestFailure::InvalidNumber;
		return Result;
	}

	const UBathWaterSettings* Settings = GetDefault<UBathWaterSettings>();
	const float Ambient = Settings->GetAmbientTemperatureC();
	const float Requested = Settings->ClampAndQuantizeTargetTemperature(RequestedTemperatureC);
	const float Current = Condition->GetTargetTemperatureC();
	const FDemandTotals CurrentTotals = CalculateDemandTotals();
	const float CurrentHeat = Condition->GetDemandPoints(EBathWaterCapacityKind::Heating);
	const float CurrentCool = Condition->GetDemandPoints(EBathWaterCapacityKind::Cooling);
	const float CandidateHeat = DemandFor(*Condition, EBathWaterCapacityKind::Heating,
		Condition->GetCirculationPercent(), Requested, Ambient);
	const float CandidateCool = DemandFor(*Condition, EBathWaterCapacityKind::Cooling,
		Condition->GetCirculationPercent(), Requested, Ambient);
	const float HeatUsed = CurrentTotals.Heating - CurrentHeat + CandidateHeat;
	const float CoolUsed = CurrentTotals.Cooling - CurrentCool + CandidateCool;
	const float HeatTotal = CalculateTotalCapacity(EBathWaterCapacityKind::Heating);
	const float CoolTotal = CalculateTotalCapacity(EBathWaterCapacityKind::Cooling);

	float Committed = Requested;
	if (Requested > Ambient && HeatUsed > HeatTotal + KINDA_SMALL_NUMBER
		&& CandidateHeat > CurrentHeat + KINDA_SMALL_NUMBER)
	{
		const float Available = FMath::Max(0.0f, HeatTotal - (CurrentTotals.Heating - CurrentHeat));
		const float MaxDelta = Condition->GetHeatingDemandPointsPerC() > KINDA_SMALL_NUMBER
			? Available / Condition->GetHeatingDemandPointsPerC() : Requested - Ambient;
		const float FeasibleDelta = FMath::FloorToFloat(
			MaxDelta / Settings->GetTargetTemperatureStepC()) * Settings->GetTargetTemperatureStepC();
		const float Feasible = FMath::Min(Requested, Ambient + FeasibleDelta);
		const bool bIncreasingOnHeatingSide = Current >= Ambient - KINDA_SMALL_NUMBER
			&& Requested > Current + KINDA_SMALL_NUMBER;
		Committed = bIncreasingOnHeatingSide ? FMath::Max(Current, Feasible) : Feasible;
		Result.LimitedKind = EBathWaterCapacityKind::Heating;
		Result.RequiredAdditionalPoints = FMath::Max(0.0f, HeatUsed - HeatTotal);
		Result.Failure = EBathWaterRequestFailure::InsufficientHeatingCapacity;
	}
	else if (Requested < Ambient && CoolUsed > CoolTotal + KINDA_SMALL_NUMBER
		&& CandidateCool > CurrentCool + KINDA_SMALL_NUMBER)
	{
		const float Available = FMath::Max(0.0f, CoolTotal - (CurrentTotals.Cooling - CurrentCool));
		const float MaxDelta = Condition->GetCoolingDemandPointsPerC() > KINDA_SMALL_NUMBER
			? Available / Condition->GetCoolingDemandPointsPerC() : Ambient - Requested;
		const float FeasibleDelta = FMath::FloorToFloat(
			MaxDelta / Settings->GetTargetTemperatureStepC()) * Settings->GetTargetTemperatureStepC();
		const float Feasible = FMath::Max(Requested, Ambient - FeasibleDelta);
		const bool bIncreasingOnCoolingSide = Current <= Ambient + KINDA_SMALL_NUMBER
			&& Requested < Current - KINDA_SMALL_NUMBER;
		Committed = bIncreasingOnCoolingSide ? FMath::Min(Current, Feasible) : Feasible;
		Result.LimitedKind = EBathWaterCapacityKind::Cooling;
		Result.RequiredAdditionalPoints = FMath::Max(0.0f, CoolUsed - CoolTotal);
		Result.Failure = EBathWaterRequestFailure::InsufficientCoolingCapacity;
	}
	Result.bWasLimited = !FMath::IsNearlyEqual(Committed, Requested);
	if (!Result.bWasLimited)
	{
		Result.Failure = EBathWaterRequestFailure::None;
	}
	Result.bSucceeded = true;
	Result.CommittedValue = Committed;
	if (FMath::IsNearlyEqual(Committed, Current))
	{
		return Result;
	}
	Condition->SetTargetTemperatureFromSubsystem(Committed);
	++DataRevision;
	BroadcastMutation();
	UE_LOG(LogBathWaterOperations, Verbose, TEXT("Target request Bath=%s Requested=%.2f Committed=%.2f"),
		*GetNameSafe(Bath), Requested, Committed);
	return Result;
}

FBathWaterCapacitySnapshot UBathWaterOperationsSubsystem::GetCapacitySnapshot(
	const EBathWaterCapacityKind Kind)
{
	if (CompactInvalidEntries())
	{
		BroadcastMutation();
	}
	return BuildCapacitySnapshot(Kind);
}

FBathWaterCapacitySnapshot UBathWaterOperationsSubsystem::BuildCapacitySnapshot(
	const EBathWaterCapacityKind Kind) const
{
	const FDemandTotals Used = CalculateDemandTotals();
	FBathWaterCapacitySnapshot Result;
	Result.Kind = Kind;
	Result.TotalPoints = CalculateTotalCapacity(Kind);
	Result.ActivePoints = CalculateActiveCapacity(Kind);
	switch (Kind)
	{
	case EBathWaterCapacityKind::Heating: Result.UsedPoints = Used.Heating; break;
	case EBathWaterCapacityKind::Cooling: Result.UsedPoints = Used.Cooling; break;
	default: Result.UsedPoints = Used.Circulation; break;
	}
	Result.DeficitPoints = FMath::Max(0.0f, Result.UsedPoints - Result.ActivePoints);
	Result.InstalledDeficitPoints = FMath::Max(0.0f, Result.UsedPoints - Result.TotalPoints);
	Result.Revision = CapacityRevision;
	return Result;
}

FBathWaterOperationsSnapshot UBathWaterOperationsSubsystem::GetSnapshot()
{
	if (CompactInvalidEntries())
	{
		BroadcastMutation();
	}
	FBathWaterOperationsSnapshot Result;
	Result.Circulation = BuildCapacitySnapshot(EBathWaterCapacityKind::Circulation);
	Result.Heating = BuildCapacitySnapshot(EBathWaterCapacityKind::Heating);
	Result.Cooling = BuildCapacitySnapshot(EBathWaterCapacityKind::Cooling);
	Result.TopologyRevision = TopologyRevision;
	Result.DataRevision = DataRevision;
	for (const TWeakObjectPtr<UBathWaterConditionComponent>& Entry : Baths)
	{
		if (const ABathhouseBathFacilityActor* Bath = Entry.IsValid()
			? Cast<ABathhouseBathFacilityActor>(Entry->GetOwner()) : nullptr)
		{
			FBathWaterBathSnapshot Snapshot;
			if (BuildBathSnapshot(Bath, Snapshot))
			{
				Snapshot.bCirculationCapacityDeficit = Snapshot.CirculationDemandPoints > KINDA_SMALL_NUMBER
					&& !Result.Circulation.IsSatisfied();
				Snapshot.bHeatingCapacityDeficit = Snapshot.HeatingDemandPoints > KINDA_SMALL_NUMBER
					&& !Result.Heating.IsSatisfied();
				Snapshot.bCoolingCapacityDeficit = Snapshot.CoolingDemandPoints > KINDA_SMALL_NUMBER
					&& !Result.Cooling.IsSatisfied();
				Result.Baths.Add(MoveTemp(Snapshot));
			}
		}
	}
	return Result;
}

bool UBathWaterOperationsSubsystem::GetBathSnapshot(
	const ABathhouseBathFacilityActor* Bath,
	FBathWaterBathSnapshot& OutSnapshot)
{
	if (CompactInvalidEntries())
	{
		BroadcastMutation();
	}
	if (!BuildBathSnapshot(Bath, OutSnapshot))
	{
		return false;
	}
	const FBathWaterCapacitySnapshot Circulation = BuildCapacitySnapshot(EBathWaterCapacityKind::Circulation);
	const FBathWaterCapacitySnapshot Heating = BuildCapacitySnapshot(EBathWaterCapacityKind::Heating);
	const FBathWaterCapacitySnapshot Cooling = BuildCapacitySnapshot(EBathWaterCapacityKind::Cooling);
	OutSnapshot.bCirculationCapacityDeficit = OutSnapshot.CirculationDemandPoints > KINDA_SMALL_NUMBER
		&& !Circulation.IsSatisfied();
	OutSnapshot.bHeatingCapacityDeficit = OutSnapshot.HeatingDemandPoints > KINDA_SMALL_NUMBER
		&& !Heating.IsSatisfied();
	OutSnapshot.bCoolingCapacityDeficit = OutSnapshot.CoolingDemandPoints > KINDA_SMALL_NUMBER
		&& !Cooling.IsSatisfied();
	return true;
}

bool UBathWaterOperationsSubsystem::BuildBathSnapshot(
	const ABathhouseBathFacilityActor* Bath,
	FBathWaterBathSnapshot& OutSnapshot) const
{
	UBathWaterConditionComponent* Condition = FindCondition(Bath);
	if (!Condition || !Bath)
	{
		return false;
	}
	OutSnapshot = FBathWaterBathSnapshot();
	OutSnapshot.BathActor = const_cast<ABathhouseBathFacilityActor*>(Bath);
	if (const UFacilityPlacementComponent* Placement = Bath->GetFacilityPlacementComponent())
	{
		if (const UBoxComponent* Footprint = Placement->GetPlacementFootprint())
		{
			OutSnapshot.FootprintTransform = Footprint->GetComponentTransform();
			OutSnapshot.FootprintHalfExtent = Footprint->GetUnscaledBoxExtent();
		}
	}
	const UBathWaterStateComponent* Water = Bath->GetBathWaterState();
	OutSnapshot.WaterPercent = Water ? Water->GetWaterPercent() : 0.0f;
	OutSnapshot.CirculationPercent = Condition->GetCirculationPercent();
	OutSnapshot.TargetTemperatureC = Condition->GetTargetTemperatureC();
	OutSnapshot.ActualTemperatureC = Condition->GetActualTemperatureC();
	OutSnapshot.ContaminationPercent = Condition->GetContaminationPercent();
	OutSnapshot.CirculationDemandPoints = Condition->GetDemandPoints(EBathWaterCapacityKind::Circulation);
	OutSnapshot.HeatingDemandPoints = Condition->GetDemandPoints(EBathWaterCapacityKind::Heating);
	OutSnapshot.CoolingDemandPoints = Condition->GetDemandPoints(EBathWaterCapacityKind::Cooling);
	OutSnapshot.ThermalThresholdPercent = Condition->GetThermalThresholdPercent();
	OutSnapshot.ThermalStatus = Condition->GetThermalStatus();
	OutSnapshot.Revision = DataRevision;
	return true;
}

bool UBathWaterOperationsSubsystem::IsCapacitySatisfied(const EBathWaterCapacityKind Kind)
{
	return GetCapacitySnapshot(Kind).IsSatisfied();
}

void UBathWaterOperationsSubsystem::PublishMutation(
	const bool bTopologyChanged,
	const bool bCapacityChanged)
{
	if (bTopologyChanged)
	{
		++TopologyRevision;
	}
	if (bCapacityChanged)
	{
		++CapacityRevision;
	}
	if (bTopologyChanged || bCapacityChanged)
	{
		++DataRevision;
	}
	BroadcastMutation();
}

bool UBathWaterOperationsSubsystem::CompactInvalidEntries()
{
	const int32 RemovedProviders = Providers.RemoveAll([](const TWeakObjectPtr<UBathWaterUtilityCapacityComponent>& Entry)
	{
		return !Entry.IsValid();
	});
	const int32 RemovedBaths = Baths.RemoveAll([](const TWeakObjectPtr<UBathWaterConditionComponent>& Entry)
	{
		return !Entry.IsValid();
	});
	if (RemovedProviders > 0)
	{
		++CapacityRevision;
	}
	if (RemovedBaths > 0)
	{
		++TopologyRevision;
	}
	if (RemovedProviders > 0 || RemovedBaths > 0)
	{
		++DataRevision;
		return true;
	}
	return false;
}

float UBathWaterOperationsSubsystem::CalculateTotalCapacity(const EBathWaterCapacityKind Kind) const
{
	float Total = 0.0f;
	for (const TWeakObjectPtr<UBathWaterUtilityCapacityComponent>& Entry : Providers)
	{
		if (Entry.IsValid() && Entry->GetCapacityKind() == Kind)
		{
			Total += Entry->GetCapacityPoints();
		}
	}
	return Total;
}

float UBathWaterOperationsSubsystem::CalculateActiveCapacity(const EBathWaterCapacityKind Kind) const
{
	float Total = 0.0f;
	for (const TWeakObjectPtr<UBathWaterUtilityCapacityComponent>& Entry : Providers)
	{
		if (Entry.IsValid() && Entry->GetCapacityKind() == Kind)
		{
			Total += Entry->GetActiveCapacityPoints();
		}
	}
	return Total;
}

UBathWaterOperationsSubsystem::FDemandTotals UBathWaterOperationsSubsystem::CalculateDemandTotals(
	const UBathWaterConditionComponent* ReplacedCondition,
	const float CandidateCirculation,
	const float CandidateTarget,
	const bool bUseCandidate) const
{
	FDemandTotals Result;
	const float Ambient = GetDefault<UBathWaterSettings>()->GetAmbientTemperatureC();
	for (const TWeakObjectPtr<UBathWaterConditionComponent>& Entry : Baths)
	{
		const UBathWaterConditionComponent* Condition = Entry.Get();
		if (!Condition)
		{
			continue;
		}
		const bool bCandidate = bUseCandidate && Condition == ReplacedCondition;
		const float Circulation = bCandidate ? CandidateCirculation : Condition->GetCirculationPercent();
		const float Target = bCandidate ? CandidateTarget : Condition->GetTargetTemperatureC();
		Result.Circulation += DemandFor(*Condition, EBathWaterCapacityKind::Circulation, Circulation, Target, Ambient);
		Result.Heating += DemandFor(*Condition, EBathWaterCapacityKind::Heating, Circulation, Target, Ambient);
		Result.Cooling += DemandFor(*Condition, EBathWaterCapacityKind::Cooling, Circulation, Target, Ambient);
	}
	return Result;
}

UBathWaterConditionComponent* UBathWaterOperationsSubsystem::FindCondition(
	const ABathhouseBathFacilityActor* Bath) const
{
	if (!Bath)
	{
		return nullptr;
	}
	for (const TWeakObjectPtr<UBathWaterConditionComponent>& Entry : Baths)
	{
		if (Entry.IsValid() && Entry->GetOwner() == Bath)
		{
			return Entry.Get();
		}
	}
	return nullptr;
}

EBathWaterRequestFailure UBathWaterOperationsSubsystem::FailureForKind(const EBathWaterCapacityKind Kind)
{
	switch (Kind)
	{
	case EBathWaterCapacityKind::Heating: return EBathWaterRequestFailure::InsufficientHeatingCapacity;
	case EBathWaterCapacityKind::Cooling: return EBathWaterRequestFailure::InsufficientCoolingCapacity;
	default: return EBathWaterRequestFailure::InsufficientCirculationCapacity;
	}
}

void UBathWaterOperationsSubsystem::BroadcastMutation()
{
	if (bPublishing)
	{
		bPublishQueued = true;
		return;
	}
	do
	{
		bPublishQueued = false;
		TGuardValue<bool> Guard(bPublishing, true);
		OnOperationsChanged.Broadcast();
	}
	while (bPublishQueued);
}
