#include "Facility/BathWaterSettings.h"

float UBathWaterSettings::GetCustomerUsableThresholdPercent() const
{
	const float Value = FMath::IsFinite(CustomerUsableThresholdPercent)
		? CustomerUsableThresholdPercent
		: 80.0f;
	return FMath::Clamp(Value, 0.0f, 100.0f);
}

float UBathWaterSettings::GetCustomerUsableThresholdNormalized() const
{
	return GetCustomerUsableThresholdPercent() * 0.01f;
}
