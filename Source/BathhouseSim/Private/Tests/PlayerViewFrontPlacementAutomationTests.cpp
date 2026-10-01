#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include <limits>

#include "Interaction/PlayerViewFrontPlacement.h"
#include "Math/RotationMatrix.h"
#include "Shop/ShopUnboxingCluster.h"
#include "Shop/ShopUnboxingTuning.h"
#include "Tests/ShopUnboxShapeTestSupport.h"

namespace
{
// Brute force: the 8 corners of each box.
void CollectCorners(const FViewFrontBox& Box, const FVector& Translation, TArray<FVector>& OutCorners)
{
	const FQuat Rotation = FRotator(0.0f, Box.YawDegrees, 0.0f).Quaternion();
	for (const float X : { -1.0f, 1.0f })
	{
		for (const float Y : { -1.0f, 1.0f })
		{
			for (const float Z : { -1.0f, 1.0f })
			{
				OutCorners.Add(Box.Center + Translation
					+ Rotation.RotateVector(FVector(X * Box.HalfExtent.X, Y * Box.HalfExtent.Y, Z * Box.HalfExtent.Z)));
			}
		}
	}
}

void GetBruteForceRange(const TArray<FVector>& Corners, const FVector& Origin, const FVector& Axis, double& OutMin,
	double& OutMax)
{
	OutMin = TNumericLimits<double>::Max();
	OutMax = -TNumericLimits<double>::Max();
	for (const FVector& Corner : Corners)
	{
		const double Projection = FVector::DotProduct(Corner - Origin, Axis);
		OutMin = FMath::Min(OutMin, Projection);
		OutMax = FMath::Max(OutMax, Projection);
	}
}

constexpr double ViewFrontGeometryTolerance = 0.01;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FViewFrontPlacementGeometryAutomationTest,
	"BathhouseSim.Interaction.ViewFrontPlacement.Geometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FViewFrontPlacementGeometryAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	// The helper takes every distance as an argument; these are test inputs, not product values.
	const FVector Camera(120.0f, -45.0f, 310.0f);
	const float DistanceCm = 77.0f;
	const FVector HalfExtent(31.0f, 17.0f, 44.0f);

	const auto CheckBoxes = [&](const TArray<FViewFrontBox>& Boxes, const FVector& Direction, const TCHAR* Label)
	{
		const FVector Translation =
			PlayerViewFrontPlacement::ComputeViewFrontTranslation(Boxes, Camera, Direction, DistanceCm);
		TArray<FVector> Corners;
		for (const FViewFrontBox& Box : Boxes)
		{
			CollectCorners(Box, Translation, Corners);
		}
		const FRotationMatrix ViewMatrix(Direction.Rotation());
		double Min = 0.0;
		double Max = 0.0;
		GetBruteForceRange(Corners, Camera, Direction, Min, Max);
		TestTrue(FString::Printf(TEXT("%s: nearest view projection equals the requested distance"), Label),
			FMath::Abs(Min - DistanceCm) <= ViewFrontGeometryTolerance);
		GetBruteForceRange(Corners, Camera, ViewMatrix.GetUnitAxis(EAxis::Y), Min, Max);
		TestTrue(FString::Printf(TEXT("%s: right range is centered on the camera"), Label),
			FMath::Abs(Min + Max) <= ViewFrontGeometryTolerance);
		GetBruteForceRange(Corners, Camera, ViewMatrix.GetUnitAxis(EAxis::Z), Min, Max);
		TestTrue(FString::Printf(TEXT("%s: up range is centered on the camera"), Label),
			FMath::Abs(Min + Max) <= ViewFrontGeometryTolerance);
		double RangeMin = 0.0;
		double RangeMax = 0.0;
		TArray<FViewFrontBox> Moved = Boxes;
		for (FViewFrontBox& Box : Moved)
		{
			Box.Center += Translation;
		}
		PlayerViewFrontPlacement::GetProjectionRange(Moved, Camera, Direction, RangeMin, RangeMax);
		GetBruteForceRange(Corners, Camera, Direction, Min, Max);
		TestTrue(FString::Printf(TEXT("%s: GetProjectionRange matches the brute-force corner range"), Label),
			FMath::Abs(RangeMin - Min) <= ViewFrontGeometryTolerance && FMath::Abs(RangeMax - Max) <= ViewFrontGeometryTolerance);
	};

	for (const float Yaw : { 0.0f, 45.0f })
	{
		for (const float Pitch : { 0.0f, 45.0f, -45.0f, 80.0f, -80.0f, -90.0f })
		{
			const FVector Direction = FRotator(Pitch, 23.0f, 0.0f).Vector();
			TArray<FViewFrontBox> Boxes;
			FViewFrontBox& Box = Boxes.AddDefaulted_GetRef();
			Box.Center = FVector(5.0f, 6.0f, 7.0f);
			Box.YawDegrees = Yaw;
			Box.HalfExtent = HalfExtent;
			CheckBoxes(Boxes, Direction, *FString::Printf(TEXT("single box yaw %.0f pitch %.0f"), Yaw, Pitch));
		}
	}

	// Cluster from the settings source: same assertions plus the camera is in no box.
	const FShopUnboxingTuning Tuning = ShopUnboxTest::MakeTuning();
	const TArray<FVector> Extents = { FVector(31, 17, 44), FVector(20, 20, 20), FVector(50, 40, 60), FVector(25, 35, 15) };
	for (const int32 Seed : { 1, 2, 3, 4, 5, 6, 7, 8 })
	{
		FRandomStream Stream(Seed);
		TArray<FShopUnboxClusterItem> Layout;
		if (!TestTrue(TEXT("Cluster layout builds"),
			FShopUnboxingCluster::BuildLayout(Extents, Tuning.OverlapDepthCm, Tuning.Cluster, Stream, Layout)))
		{
			continue;
		}
		TArray<FViewFrontBox> Boxes;
		for (int32 Index = 0; Index < Layout.Num(); ++Index)
		{
			FViewFrontBox& Box = Boxes.AddDefaulted_GetRef();
			Box.Center = Layout[Index].Center;
			Box.YawDegrees = Layout[Index].YawDegrees;
			Box.HalfExtent = Extents[Index];
		}
		const FVector Direction = FRotator(-30.0f, 140.0f, 0.0f).Vector();
		CheckBoxes(Boxes, Direction, *FString::Printf(TEXT("cluster seed %d"), Seed));
		const FVector Translation =
			PlayerViewFrontPlacement::ComputeViewFrontTranslation(Boxes, Camera, Direction, DistanceCm);
		const FRotationMatrix ViewMatrix(Direction.Rotation());
		for (const FViewFrontBox& Box : Boxes)
		{
			const FVector Local = FRotator(0.0f, -Box.YawDegrees, 0.0f).RotateVector(Camera - (Box.Center + Translation));
			TestFalse(TEXT("Cluster does not contain the camera"),
				FMath::Abs(Local.X) <= Box.HalfExtent.X && FMath::Abs(Local.Y) <= Box.HalfExtent.Y
					&& FMath::Abs(Local.Z) <= Box.HalfExtent.Z);
		}
	}

	// Camera clearance push.
	const float ClearanceCm = 12.0f;
	const FVector Forward = FVector::ForwardVector;
	{
		// Top below the camera by more than the clearance: no push even though it overlaps the camera in plan.
		FViewFrontBox Low;
		Low.Center = Camera + FVector(0.0f, 0.0f, -(ClearanceCm + HalfExtent.Z + 1.0f));
		Low.HalfExtent = HalfExtent;
		TestEqual(TEXT("Cluster below the camera is not pushed"),
			PlayerViewFrontPlacement::GetCameraClearancePushCm(MakeArrayView(&Low, 1), Camera, Forward, ClearanceCm),
			0.0f);

		// Tall cluster around the camera in plan: pushed until the nearest forward projection equals the clearance.
		FViewFrontBox Tall;
		Tall.Center = Camera;
		Tall.HalfExtent = HalfExtent;
		const float Push =
			PlayerViewFrontPlacement::GetCameraClearancePushCm(MakeArrayView(&Tall, 1), Camera, Forward, ClearanceCm);
		TestTrue(TEXT("Cluster around the camera is pushed"), Push > 0.0f);
		FViewFrontBox Pushed = Tall;
		Pushed.Center += Forward * Push;
		double Min = 0.0;
		double Max = 0.0;
		PlayerViewFrontPlacement::GetProjectionRange(MakeArrayView(&Pushed, 1), Camera, Forward, Min, Max);
		TestTrue(TEXT("Pushed forward projection minimum equals the clearance"), FMath::Abs(Min - ClearanceCm) <= ViewFrontGeometryTolerance);

		// Already in front: no push.
		FViewFrontBox Ahead = Tall;
		Ahead.Center += Forward * (HalfExtent.X + ClearanceCm + 5.0f);
		TestEqual(TEXT("Cluster already ahead is not pushed"),
			PlayerViewFrontPlacement::GetCameraClearancePushCm(MakeArrayView(&Ahead, 1), Camera, Forward, ClearanceCm),
			0.0f);
	}

	// Pull distances.
	TArray<float> Distances;
	PlayerViewFrontPlacement::BuildPullDistances(100.0f, 30.0f, 10.0f, Distances);
	TestTrue(TEXT("Multiple-of-step start: 100..30"),
		Distances == TArray<float>({ 100.0f, 90.0f, 80.0f, 70.0f, 60.0f, 50.0f, 40.0f, 30.0f }));
	PlayerViewFrontPlacement::BuildPullDistances(95.0f, 30.0f, 10.0f, Distances);
	TestTrue(TEXT("Non-multiple start ends with the minimum once"),
		Distances.Num() == 8 && Distances.Last() == 30.0f && Distances[6] == 35.0f);
	PlayerViewFrontPlacement::BuildPullDistances(30.0f, 30.0f, 10.0f, Distances);
	TestTrue(TEXT("Start equal to the minimum yields one entry"), Distances == TArray<float>({ 30.0f }));
	PlayerViewFrontPlacement::BuildPullDistances(10.0f, 30.0f, 10.0f, Distances);
	TestTrue(TEXT("Start below the minimum is raised to the minimum"), Distances == TArray<float>({ 30.0f }));
	PlayerViewFrontPlacement::BuildPullDistances(20.0f, 0.0f, 10.0f, Distances);
	TestTrue(TEXT("Zero minimum is included"), Distances == TArray<float>({ 20.0f, 10.0f, 0.0f }));
	PlayerViewFrontPlacement::BuildPullDistances(100.0f, 30.0f, 0.0f, Distances);
	TestTrue(TEXT("Step <= 0 yields no distances"), Distances.IsEmpty());
	PlayerViewFrontPlacement::BuildPullDistances(100.0f, 30.0f, std::numeric_limits<float>::quiet_NaN(), Distances);
	TestTrue(TEXT("NaN step yields no distances"), Distances.IsEmpty());
	PlayerViewFrontPlacement::BuildPullDistances(std::numeric_limits<float>::infinity(), 30.0f, 10.0f, Distances);
	TestTrue(TEXT("Non-finite start yields no distances"), Distances.IsEmpty());
	return true;
}

#endif
