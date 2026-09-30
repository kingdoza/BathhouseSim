#pragma once
#include "CoreMinimal.h"
class AActor;
class UActorComponent;
class UBathhouseFacilityPlacementInstanceData;

namespace FacilityPlacementExtensionUtils
{
	void CollectAuthoringComponents(const AActor& Owner, TArray<UActorComponent*>& Out);
	bool ValidateAuthoring(const AActor& Owner, FText& OutFailure);
	bool Export(const AActor& Owner, UBathhouseFacilityPlacementInstanceData& Data, FText& OutFailure);
	bool Import(AActor& Owner, const UBathhouseFacilityPlacementInstanceData* Data, FText& OutFailure);
} // namespace FacilityPlacementExtensionUtils
