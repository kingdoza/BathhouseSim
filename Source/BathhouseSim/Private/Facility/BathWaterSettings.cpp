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

float UBathWaterSettings::GetAmbientTemperatureC() const
{
	if (!FMath::IsFinite(AmbientTemperatureC))
	{
		ensureMsgf(false, TEXT("Bath Water AmbientTemperatureC must be finite."));
		return 20.0f;
	}
	return AmbientTemperatureC;
}

float UBathWaterSettings::GetMinTargetTemperatureC() const
{
	const float Ambient = GetAmbientTemperatureC();
	if (!FMath::IsFinite(MinTargetTemperatureC) || MinTargetTemperatureC > Ambient)
	{
		ensureMsgf(false, TEXT("Bath Water MinTargetTemperatureC must be finite and no greater than ambient."));
		return FMath::Min(10.0f, Ambient);
	}
	return MinTargetTemperatureC;
}

float UBathWaterSettings::GetMaxTargetTemperatureC() const
{
	const float Ambient = GetAmbientTemperatureC();
	if (!FMath::IsFinite(MaxTargetTemperatureC) || MaxTargetTemperatureC < Ambient)
	{
		ensureMsgf(false, TEXT("Bath Water MaxTargetTemperatureC must be finite and no less than ambient."));
		return FMath::Max(50.0f, Ambient);
	}
	return MaxTargetTemperatureC;
}

float UBathWaterSettings::GetTargetTemperatureStepC() const
{
	if (!FMath::IsFinite(TargetTemperatureStepC) || TargetTemperatureStepC <= 0.0f)
	{
		ensureMsgf(false, TEXT("Bath Water TargetTemperatureStepC must be finite and positive."));
		return 1.0f;
	}
	return TargetTemperatureStepC;
}

float UBathWaterSettings::ClampAndQuantizeTargetTemperature(const float RequestedTemperatureC) const
{
	const float Ambient = GetAmbientTemperatureC();
	const float SafeRequested = FMath::IsFinite(RequestedTemperatureC) ? RequestedTemperatureC : Ambient;
	const float Step = GetTargetTemperatureStepC();
	const float Quantized = Ambient + FMath::RoundToFloat((SafeRequested - Ambient) / Step) * Step;
	return FMath::Clamp(Quantized, GetMinTargetTemperatureC(), GetMaxTargetTemperatureC());
}
