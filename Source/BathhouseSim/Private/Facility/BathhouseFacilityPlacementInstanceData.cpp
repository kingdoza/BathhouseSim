#include "Facility/BathhouseFacilityPlacementInstanceData.h"

#include "Facility/FacilityPlacementExtension.h"

FText UBathhouseFacilityPlacementInstanceData::GetPlacementContentsSummary() const
{
	TArray<FString> Parts;
	for (const UFacilityPlacementExtensionData* Extension : Extensions)
	{
		if (Extension)
		{
			const FText Summary = Extension->GetContentsSummary();
			if (!Summary.IsEmpty())
			{
				Parts.Add(Summary.ToString());
			}
		}
	}
	return FText::FromString(FString::Join(Parts, TEXT(", ")));
}
