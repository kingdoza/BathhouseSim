#include "Shop/ShopUnboxingTuning.h"

#include "Shop/ShopSettings.h"

FShopUnboxingTuning FShopUnboxingTuning::FromSettings(const UShopSettings& Settings)
{
	FShopUnboxingTuning Tuning;
	Tuning.OverlapDepthCm = Settings.GetUnboxOverlapDepthCm();
	Tuning.ViewDistanceCm = Settings.GetUnboxViewDistanceCm();
	Tuning.ViewMinDistanceCm = Settings.GetUnboxViewMinDistanceCm();
	Tuning.ViewPullStepCm = Settings.GetUnboxViewPullStepCm();
	Tuning.ViewLayoutAttempts = Settings.GetUnboxViewLayoutAttempts();
	Tuning.ForwardDistanceCm = Settings.GetUnboxForwardDistanceCm();
	Tuning.MinForwardDistanceCm = Settings.GetUnboxMinForwardDistanceCm();
	Tuning.ForwardPullStepCm = Settings.GetUnboxForwardPullStepCm();
	Tuning.ForwardLayoutAttempts = Settings.GetUnboxForwardLayoutAttempts();
	Tuning.ForwardFloorClearanceCm = Settings.GetUnboxForwardFloorClearanceCm();
	Tuning.CameraClearanceCm = Settings.GetUnboxCameraClearanceCm();
	Tuning.OverheadClearanceCm = Settings.GetUnboxOverheadClearanceCm();
	Tuning.OverheadStepCm = Settings.GetUnboxOverheadStepCm();
	Tuning.OverheadStepCount = Settings.GetUnboxOverheadStepCount();
	Tuning.OverheadLayoutAttempts = Settings.GetUnboxOverheadLayoutAttempts();
	Tuning.Cluster.PlacementAttempts = Settings.GetUnboxClusterPlacementAttempts();
	Tuning.Cluster.MinElevationDegrees = Settings.GetUnboxClusterMinElevationDegrees();
	Tuning.Cluster.MaxElevationDegrees = Settings.GetUnboxClusterMaxElevationDegrees();
	Tuning.Cluster.DepthToleranceCm = Settings.GetUnboxClusterDepthToleranceCm();
	Tuning.Cluster.PairDepthExtentRatio = Settings.GetUnboxClusterPairDepthExtentRatio();
	return Tuning;
}
