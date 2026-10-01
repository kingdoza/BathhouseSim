#include "Service/MassageChairPlacementInstanceData.h"

FText UMassageChairPlacementInstanceData::GetPlacementContentsSummary() const
{
	const FText Base = Super::GetPlacementContentsSummary();
	return bBroken ? FText::FromString(Base.IsEmpty() ? TEXT("고장") : Base.ToString() + TEXT(", 고장")) : Base;
}
