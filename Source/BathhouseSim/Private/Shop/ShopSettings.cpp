#include "Shop/ShopSettings.h"

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

float UShopSettings::GetDeliveryDelaySeconds() const
{
	return FMath::IsFinite(DeliveryDelaySeconds) ? FMath::Max(0.0f, DeliveryDelaySeconds) : 10.0f;
}

float UShopSettings::GetUnboxForwardDistanceCm() const
{
	return FMath::IsFinite(UnboxForwardDistanceCm) ? FMath::Max(0.0f, UnboxForwardDistanceCm) : 100.0f;
}

float UShopSettings::GetUnboxOverlapDepthCm() const
{
	return FMath::IsFinite(UnboxOverlapDepthCm)
		? FMath::Clamp(UnboxOverlapDepthCm, 0.0f, 50.0f)
		: 8.0f;
}

float UShopSettings::GetDeliveryNoticeSeconds() const
{
	return FMath::IsFinite(DeliveryNoticeSeconds) ? FMath::Max(0.01f, DeliveryNoticeSeconds) : 3.0f;
}
