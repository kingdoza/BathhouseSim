#include "Shop/ShopSettings.h"

#include "Service/ItemBoxActor.h"
#include "Shop/ShopCatalog.h"
#include "Shop/ShopDeliveryBoxActor.h"

UShopCatalog* UShopSettings::LoadCatalog() const
{
	return Catalog.LoadSynchronous();
}

TSubclassOf<AShopDeliveryBoxActor> UShopSettings::LoadDeliveryBoxClass() const
{
	return DeliveryBoxClass.LoadSynchronous();
}

TSubclassOf<AItemBoxActor> UShopSettings::LoadItemBoxClass() const
{
	return ItemBoxClass.LoadSynchronous();
}

float UShopSettings::GetDeliveryDelaySeconds() const
{
	return FMath::IsFinite(DeliveryDelaySeconds) ? FMath::Max(0.0f, DeliveryDelaySeconds) : 10.0f;
}

float UShopSettings::GetUnboxForwardDistanceCm() const
{
	return FMath::IsFinite(UnboxForwardDistanceCm) ? FMath::Max(0.0f, UnboxForwardDistanceCm)
		: DefaultUnboxForwardDistanceCm;
}

float UShopSettings::GetUnboxOverlapDepthCm() const
{
	return FMath::IsFinite(UnboxOverlapDepthCm)
		? FMath::Clamp(UnboxOverlapDepthCm, 0.0f, MaxUnboxOverlapDepthCm)
		: DefaultUnboxOverlapDepthCm;
}

float UShopSettings::GetDeliveryNoticeSeconds() const
{
	return FMath::IsFinite(DeliveryNoticeSeconds) ? FMath::Max(0.01f, DeliveryNoticeSeconds) : 3.0f;
}

namespace
{
float FiniteAtLeast(const float Value, const float Minimum, const float Fallback)
{
	return FMath::IsFinite(Value) ? FMath::Max(Minimum, Value) : Fallback;
}

int32 AtLeastOne(const int32 Value)
{
	return FMath::Max(1, Value);
}
}

float UShopSettings::GetUnboxViewMinDistanceCm() const
{
	return FiniteAtLeast(UnboxViewMinDistanceCm, 0.0f, DefaultUnboxViewMinDistanceCm);
}

float UShopSettings::GetUnboxViewDistanceCm() const
{
	return FiniteAtLeast(UnboxViewDistanceCm, GetUnboxViewMinDistanceCm(), DefaultUnboxViewDistanceCm);
}

float UShopSettings::GetUnboxViewPullStepCm() const
{
	return FiniteAtLeast(UnboxViewPullStepCm, UnboxMinStepCm, DefaultUnboxViewPullStepCm);
}

int32 UShopSettings::GetUnboxViewLayoutAttempts() const
{
	return AtLeastOne(UnboxViewLayoutAttempts);
}

float UShopSettings::GetUnboxMinForwardDistanceCm() const
{
	return FiniteAtLeast(UnboxMinForwardDistanceCm, 0.0f, DefaultUnboxMinForwardDistanceCm);
}

float UShopSettings::GetUnboxForwardPullStepCm() const
{
	return FiniteAtLeast(UnboxForwardPullStepCm, UnboxMinStepCm, DefaultUnboxForwardPullStepCm);
}

int32 UShopSettings::GetUnboxForwardLayoutAttempts() const
{
	return AtLeastOne(UnboxForwardLayoutAttempts);
}

float UShopSettings::GetUnboxForwardFloorClearanceCm() const
{
	return FiniteAtLeast(UnboxForwardFloorClearanceCm, 0.0f, DefaultUnboxForwardFloorClearanceCm);
}

float UShopSettings::GetUnboxCameraClearanceCm() const
{
	return FiniteAtLeast(UnboxCameraClearanceCm, 0.0f, DefaultUnboxCameraClearanceCm);
}

float UShopSettings::GetUnboxOverheadClearanceCm() const
{
	return FiniteAtLeast(UnboxOverheadClearanceCm, 0.0f, DefaultUnboxOverheadClearanceCm);
}

float UShopSettings::GetUnboxOverheadStepCm() const
{
	return FiniteAtLeast(UnboxOverheadStepCm, UnboxMinStepCm, DefaultUnboxOverheadStepCm);
}

int32 UShopSettings::GetUnboxOverheadStepCount() const
{
	return AtLeastOne(UnboxOverheadStepCount);
}

int32 UShopSettings::GetUnboxOverheadLayoutAttempts() const
{
	return AtLeastOne(UnboxOverheadLayoutAttempts);
}

int32 UShopSettings::GetUnboxClusterPlacementAttempts() const
{
	return AtLeastOne(UnboxClusterPlacementAttempts);
}

float UShopSettings::GetUnboxClusterMinElevationDegrees() const
{
	return FMath::IsFinite(UnboxClusterMinElevationDegrees)
		? FMath::Clamp(UnboxClusterMinElevationDegrees, 0.0f, 90.0f)
		: DefaultUnboxClusterMinElevationDegrees;
}

float UShopSettings::GetUnboxClusterMaxElevationDegrees() const
{
	const float Maximum = FMath::IsFinite(UnboxClusterMaxElevationDegrees)
		? FMath::Clamp(UnboxClusterMaxElevationDegrees, 0.0f, 90.0f)
		: DefaultUnboxClusterMaxElevationDegrees;
	return FMath::Max(Maximum, GetUnboxClusterMinElevationDegrees());
}

float UShopSettings::GetUnboxClusterDepthToleranceCm() const
{
	return FiniteAtLeast(UnboxClusterDepthToleranceCm, 0.0f, DefaultUnboxClusterDepthToleranceCm);
}

float UShopSettings::GetUnboxClusterPairDepthExtentRatio() const
{
	return FMath::IsFinite(UnboxClusterPairDepthExtentRatio)
		? FMath::Clamp(UnboxClusterPairDepthExtentRatio, 0.0f, 1.0f)
		: DefaultUnboxClusterPairDepthExtentRatio;
}
