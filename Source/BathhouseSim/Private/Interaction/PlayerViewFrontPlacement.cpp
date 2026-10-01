#include "Interaction/PlayerViewFrontPlacement.h"

#include "Math/RotationMatrix.h"

namespace PlayerViewFrontPlacement
{
void GetProjectionRange(TConstArrayView<FViewFrontBox> Boxes, const FVector& Origin, const FVector& UnitAxis,
	double& OutMin, double& OutMax)
{
	OutMin = TNumericLimits<double>::Max();
	OutMax = -TNumericLimits<double>::Max();
	for (const FViewFrontBox& Box : Boxes)
	{
		const double Radians = FMath::DegreesToRadians(static_cast<double>(Box.YawDegrees));
		const FVector LocalX(FMath::Cos(Radians), FMath::Sin(Radians), 0.0);
		const FVector LocalY(-FMath::Sin(Radians), FMath::Cos(Radians), 0.0);
		const double Radius = FMath::Abs(FVector::DotProduct(UnitAxis, LocalX)) * Box.HalfExtent.X
			+ FMath::Abs(FVector::DotProduct(UnitAxis, LocalY)) * Box.HalfExtent.Y
			+ FMath::Abs(UnitAxis.Z) * Box.HalfExtent.Z;
		const double Center = FVector::DotProduct(Box.Center - Origin, UnitAxis);
		OutMin = FMath::Min(OutMin, Center - Radius);
		OutMax = FMath::Max(OutMax, Center + Radius);
	}
}

FVector ComputeViewFrontTranslation(TConstArrayView<FViewFrontBox> Boxes, const FVector& CameraOrigin,
	const FVector& UnitViewDirection, const float DistanceCm)
{
	if (Boxes.IsEmpty())
	{
		return FVector::ZeroVector;
	}
	const FRotationMatrix ViewMatrix(UnitViewDirection.Rotation());
	const FVector Right = ViewMatrix.GetUnitAxis(EAxis::Y);
	const FVector Up = ViewMatrix.GetUnitAxis(EAxis::Z);
	double ViewMin = 0.0;
	double ViewMax = 0.0;
	double RightMin = 0.0;
	double RightMax = 0.0;
	double UpMin = 0.0;
	double UpMax = 0.0;
	GetProjectionRange(Boxes, CameraOrigin, UnitViewDirection, ViewMin, ViewMax);
	GetProjectionRange(Boxes, CameraOrigin, Right, RightMin, RightMax);
	GetProjectionRange(Boxes, CameraOrigin, Up, UpMin, UpMax);
	return UnitViewDirection * (static_cast<double>(DistanceCm) - ViewMin)
		- Right * (0.5 * (RightMin + RightMax))
		- Up * (0.5 * (UpMin + UpMax));
}

float GetCameraClearancePushCm(TConstArrayView<FViewFrontBox> Boxes, const FVector& CameraOrigin,
	const FVector& UnitHorizontalForward, const float CameraClearanceCm)
{
	if (Boxes.IsEmpty())
	{
		return 0.0f;
	}
	double Min = 0.0;
	double Max = 0.0;
	GetProjectionRange(Boxes, CameraOrigin, FVector::UpVector, Min, Max);
	if (Max <= -static_cast<double>(CameraClearanceCm))
	{
		return 0.0f;
	}
	GetProjectionRange(Boxes, CameraOrigin, UnitHorizontalForward, Min, Max);
	return Min >= static_cast<double>(CameraClearanceCm) ? 0.0f : static_cast<float>(CameraClearanceCm - Min);
}

void BuildPullDistances(const float StartCm, const float MinCm, const float StepCm, TArray<float>& OutDistances)
{
	OutDistances.Reset();
	if (!FMath::IsFinite(StartCm) || !FMath::IsFinite(MinCm) || !FMath::IsFinite(StepCm) || StepCm <= 0.0f)
	{
		return;
	}
	for (float Distance = FMath::Max(StartCm, MinCm); Distance > MinCm; Distance -= StepCm)
	{
		OutDistances.Add(Distance);
	}
	OutDistances.Add(MinCm);
}
}
