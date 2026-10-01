#include "Shop/ShopUnboxingPlacement.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Math/RotationMatrix.h"
#include "Interaction/PlayerViewFrontPlacement.h"
#include "Placement/FacilityPlacementCollisionUtils.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Service/ItemBoxActor.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Shop/ShopUnboxingCluster.h"
#include "Shop/ShopUnboxItemShape.h"
#include "Shop/ShopUnboxingTuning.h"

#define LOCTEXT_NAMESPACE "ShopUnboxingPlacement"

namespace
{
struct FUnboxCandidate
{
	FTransform Transform = FTransform::Identity;
	FVector Location = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
	FCollisionShape Shape;
	const UPrimitiveComponent* CollisionTemplate = nullptr;
};

bool BuildCandidateAtShapeCenter(
	const FShopUnboxItemShape& Item,
	const FVector& ShapeCenter,
	const float Yaw,
	FUnboxCandidate& OutCandidate,
	FText& OutFailureReason)
{
	const FRotator CandidateRotation(0.0f, Yaw, 0.0f);
	const FVector ItemScale = Item.ItemScale;
	const FTransform ProbeTransform(CandidateRotation, FVector::ZeroVector, ItemScale);
	FVector RelativeShapeCenter = FVector::ZeroVector;
	FQuat ProbeRotation = FQuat::Identity;
	if (!Item.BuildCollisionQuery
		|| !Item.BuildCollisionQuery(
			ProbeTransform,
			RelativeShapeCenter,
			ProbeRotation,
			OutCandidate.Shape,
			OutCandidate.CollisionTemplate,
			OutFailureReason))
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT("MissingShapeQuery", "개봉 물품의 충돌 형태를 확인할 수 없습니다.");
		}
		return false;
	}

	OutCandidate.Location = ShapeCenter;
	OutCandidate.Rotation = ProbeRotation;
	OutCandidate.Transform = FTransform(
		ProbeRotation,
		ShapeCenter - RelativeShapeCenter,
		ItemScale);
	return true;
}

FVector GetCandidateHalfExtent(const FUnboxCandidate& Candidate)
{
	return Candidate.Shape.GetExtent().GetAbs();
}

FVector GetClusterTranslation(
	const TArray<FShopUnboxClusterItem>& Items,
	const TArray<FVector>& HalfExtents,
	const FVector& TargetXYCenter,
	const float TargetLowestZ)
{
	float MinX = TNumericLimits<float>::Max();
	float MinY = TNumericLimits<float>::Max();
	float MinZ = TNumericLimits<float>::Max();
	float MaxX = -TNumericLimits<float>::Max();
	float MaxY = -TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		const FShopUnboxClusterItem& Item = Items[Index];
		const FVector& Extent = HalfExtents[Index];
		const float YawRadians = FMath::DegreesToRadians(Item.YawDegrees);
		const float CosYaw = FMath::Abs(FMath::Cos(YawRadians));
		const float SinYaw = FMath::Abs(FMath::Sin(YawRadians));
		const float RadiusX = CosYaw * Extent.X + SinYaw * Extent.Y;
		const float RadiusY = SinYaw * Extent.X + CosYaw * Extent.Y;
		MinX = FMath::Min(MinX, Item.Center.X - RadiusX);
		MaxX = FMath::Max(MaxX, Item.Center.X + RadiusX);
		MinY = FMath::Min(MinY, Item.Center.Y - RadiusY);
		MaxY = FMath::Max(MaxY, Item.Center.Y + RadiusY);
		MinZ = FMath::Min(MinZ, Item.Center.Z - Extent.Z);
	}

	const FVector ClusterBoundsCenter(0.5f * (MinX + MaxX), 0.5f * (MinY + MaxY), 0.0f);
	return FVector(
		TargetXYCenter.X - ClusterBoundsCenter.X,
		TargetXYCenter.Y - ClusterBoundsCenter.Y,
		TargetLowestZ - MinZ);
}

bool IsCandidateSafe(
	UWorld& World,
	const FUnboxCandidate& Candidate,
	AActor& Player,
	AShopDeliveryBoxActor& Box,
	const FVector& VisibilityOrigin,
	const float EnvironmentClearanceCm)
{
	if (!Candidate.CollisionTemplate)
	{
		return false;
	}

	FCollisionQueryParams EnvironmentParams(SCENE_QUERY_STAT(ShopUnboxCandidateOverlap), false);
	EnvironmentParams.AddIgnoredActor(&Player);
	EnvironmentParams.AddIgnoredActor(&Box);
	const FVector CandidateHalfExtent = GetCandidateHalfExtent(Candidate);
	const FVector ClearanceExtent(
		CandidateHalfExtent.X + EnvironmentClearanceCm,
		CandidateHalfExtent.Y + EnvironmentClearanceCm,
		CandidateHalfExtent.Z + EnvironmentClearanceCm * 0.5f);
	const FVector ClearanceCenter = Candidate.Location + FVector(0.0f, 0.0f, EnvironmentClearanceCm * 0.5f);
	if (FacilityPlacementCollision::HasBlockingOverlap(
		World,
		ClearanceCenter,
		Candidate.Rotation,
		FCollisionShape::MakeBox(ClearanceExtent),
		*Candidate.CollisionTemplate,
		EnvironmentParams))
	{
		return false;
	}

	FCollisionObjectQueryParams PawnObjects;
	PawnObjects.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams PawnParams(SCENE_QUERY_STAT(ShopUnboxPawnOverlap), false);
	PawnParams.AddIgnoredActor(&Player);
	if (World.OverlapAnyTestByObjectType(
		ClearanceCenter,
		Candidate.Rotation,
		PawnObjects,
		FCollisionShape::MakeBox(ClearanceExtent),
		PawnParams))
	{
		return false;
	}

	FCollisionQueryParams VisibilityParams(SCENE_QUERY_STAT(ShopUnboxLineOfSight), false);
	VisibilityParams.AddIgnoredActor(&Player);
	VisibilityParams.AddIgnoredActor(&Box);
	FHitResult Hit;
	return !World.LineTraceSingleByChannel(
		Hit,
		VisibilityOrigin,
		Candidate.Location,
		ECC_Visibility,
		VisibilityParams);
}

void MakeWorldBoxes(
	const TArray<FShopUnboxClusterItem>& Layout,
	const TArray<FVector>& HalfExtents,
	const FVector& Translation,
	TArray<FViewFrontBox>& OutBoxes)
{
	OutBoxes.Reset(Layout.Num());
	for (int32 Index = 0; Index < Layout.Num(); ++Index)
	{
		FViewFrontBox& Box = OutBoxes.AddDefaulted_GetRef();
		Box.Center = Layout[Index].Center + Translation;
		Box.YawDegrees = Layout[Index].YawDegrees;
		Box.HalfExtent = HalfExtents[Index];
	}
}

struct FPlacementContext
{
	UWorld& World;
	AActor& Player;
	AShopDeliveryBoxActor& Box;
	const TArray<FShopUnboxItemShape>& Items;
	const TArray<FVector>& HalfExtents;
	const FShopUnboxingTuning& Tuning;
	FRandomStream& RandomStream;
};

// One layout attempt: build a cluster, place it with BuildTranslation, and validate every item.
bool TryMakeLayout(
	const FPlacementContext& Context,
	const TFunctionRef<FVector(const TArray<FShopUnboxClusterItem>&)>& BuildTranslation,
	const FVector& VisibilityOrigin,
	TArray<FTransform>& OutTransforms,
	FText& OutFailureReason)
{
	TArray<FShopUnboxClusterItem> Layout;
	if (!FShopUnboxingCluster::BuildLayout(
		Context.HalfExtents,
		Context.Tuning.OverlapDepthCm,
		Context.Tuning.Cluster,
		Context.RandomStream,
		Layout))
	{
		return false;
	}

	const FVector Translation = BuildTranslation(Layout);
	TArray<FTransform> CandidateTransforms;
	CandidateTransforms.Reserve(Context.Items.Num());

	for (int32 Index = 0; Index < Context.Items.Num(); ++Index)
	{
		FUnboxCandidate Candidate;
		if (!BuildCandidateAtShapeCenter(
			Context.Items[Index],
			Layout[Index].Center + Translation,
			Layout[Index].YawDegrees,
			Candidate,
			OutFailureReason))
		{
			return false;
		}
		if (!IsCandidateSafe(
			Context.World,
			Candidate,
			Context.Player,
			Context.Box,
			VisibilityOrigin,
			Context.Tuning.OverlapDepthCm))
		{
			return false;
		}
		CandidateTransforms.Add(Candidate.Transform);
	}

	OutTransforms = MoveTemp(CandidateTransforms);
	return true;
}

// Layout attempts for every pull distance. Returns true on the first success. A shape-query failure
// (non-empty OutFailureReason) is a hard failure the caller must propagate.
bool TryPullDistances(
	const FPlacementContext& Context,
	const TArray<float>& Distances,
	const int32 LayoutAttempts,
	const TFunctionRef<FVector(const TArray<FShopUnboxClusterItem>&, float)>& BuildTranslation,
	const FVector& VisibilityOrigin,
	TArray<FTransform>& OutTransforms,
	FText& OutFailureReason)
{
	for (const float DistanceCm : Distances)
	{
		for (int32 Attempt = 0; Attempt < LayoutAttempts; ++Attempt)
		{
			if (TryMakeLayout(
				Context,
				[&](const TArray<FShopUnboxClusterItem>& Layout) { return BuildTranslation(Layout, DistanceCm); },
				VisibilityOrigin,
				OutTransforms,
				OutFailureReason))
			{
				return true;
			}
			if (!OutFailureReason.IsEmpty())
			{
				return false;
			}
		}
	}
	return false;
}
}

bool FShopUnboxingPlacement::FindSpawnTransforms(
	UWorld& World,
	AActor& Player,
	const UCapsuleComponent& PlayerCapsule,
	AShopDeliveryBoxActor& Box,
	const TArray<FShopUnboxItemShape>& Items,
	const FShopUnboxingPlacementRequest& Request,
	const FShopUnboxingTuning& Tuning,
	FRandomStream& RandomStream,
	TArray<FTransform>& OutTransforms,
	FText& OutFailureReason,
	EShopUnboxPlacementStage* OutStage)
{
	OutTransforms.Reset();
	OutFailureReason = FText::GetEmpty();
	if (OutStage)
	{
		*OutStage = EShopUnboxPlacementStage::None;
	}
	if (Items.IsEmpty() || Request.FootLocation.ContainsNaN() || Request.CameraOrigin.ContainsNaN()
		|| Request.CameraDirection.ContainsNaN() || Request.CameraDirection.IsNearlyZero()
		|| !FMath::IsFinite(Tuning.OverlapDepthCm) || Tuning.OverlapDepthCm < 0.0f)
	{
		OutFailureReason = LOCTEXT("InvalidUnboxRequest", "상자 개봉 위치를 계산할 수 없습니다.");
		return false;
	}

	const FVector ViewDirection = Request.CameraDirection.GetSafeNormal();
	const FVector HorizontalView(ViewDirection.X, ViewDirection.Y, 0.0f);
	const bool bHasHorizontalForward = !HorizontalView.IsNearlyZero();
	const FVector Forward = bHasHorizontalForward ? HorizontalView.GetSafeNormal() : FVector::ForwardVector;
	const float ProbeYaw = bHasHorizontalForward ? Forward.Rotation().Yaw : 0.0f;
	const FVector PlayerCenter = PlayerCapsule.GetComponentLocation();
	const float CapsuleHalfHeight = PlayerCapsule.GetScaledCapsuleHalfHeight();
	if (PlayerCenter.ContainsNaN() || !FMath::IsFinite(CapsuleHalfHeight) || CapsuleHalfHeight <= 0.0f)
	{
		OutFailureReason = LOCTEXT("InvalidUnboxPlayer", "플레이어 위치를 확인할 수 없습니다.");
		return false;
	}
	const FVector CapsuleTop(PlayerCenter.X, PlayerCenter.Y, PlayerCenter.Z + CapsuleHalfHeight);

	TArray<FVector> HalfExtents;
	HalfExtents.Reserve(Items.Num());
	for (const FShopUnboxItemShape& Item : Items)
	{
		FUnboxCandidate Probe;
		if (!BuildCandidateAtShapeCenter(
			Item,
			FVector::ZeroVector,
			ProbeYaw,
			Probe,
			OutFailureReason))
		{
			return false;
		}
		HalfExtents.Add(GetCandidateHalfExtent(Probe));
	}

	const FPlacementContext Context{World, Player, Box, Items, HalfExtents, Tuning, RandomStream};
	const auto Succeed = [&OutStage](const EShopUnboxPlacementStage Stage)
	{
		if (OutStage)
		{
			*OutStage = Stage;
		}
		return true;
	};
	TArray<float> Distances;

	// Stage 1: in front of the camera along the view direction.
	PlayerViewFrontPlacement::BuildPullDistances(
		Tuning.ViewDistanceCm,
		Tuning.ViewMinDistanceCm,
		Tuning.ViewPullStepCm,
		Distances);
	const auto BuildViewTranslation = [&](const TArray<FShopUnboxClusterItem>& Layout, const float DistanceCm)
	{
		TArray<FViewFrontBox> Boxes;
		MakeWorldBoxes(Layout, HalfExtents, FVector::ZeroVector, Boxes);
		return PlayerViewFrontPlacement::ComputeViewFrontTranslation(
			Boxes,
			Request.CameraOrigin,
			ViewDirection,
			DistanceCm);
	};
	if (TryPullDistances(
		Context,
		Distances,
		Tuning.ViewLayoutAttempts,
		BuildViewTranslation,
		Request.CameraOrigin,
		OutTransforms,
		OutFailureReason))
	{
		return Succeed(EShopUnboxPlacementStage::ViewFront);
	}
	if (!OutFailureReason.IsEmpty())
	{
		return false;
	}

	// Stage 2: on the floor in front of the player's feet, pushed off the camera when it would wrap it.
	if (bHasHorizontalForward)
	{
		PlayerViewFrontPlacement::BuildPullDistances(
			Tuning.ForwardDistanceCm,
			Tuning.MinForwardDistanceCm,
			Tuning.ForwardPullStepCm,
			Distances);
		const auto BuildFloorTranslation = [&](const TArray<FShopUnboxClusterItem>& Layout, const float DistanceCm)
		{
			FVector Translation = GetClusterTranslation(
				Layout,
				HalfExtents,
				Request.FootLocation + Forward * DistanceCm,
				Request.FootLocation.Z + Tuning.ForwardFloorClearanceCm);
			TArray<FViewFrontBox> Boxes;
			MakeWorldBoxes(Layout, HalfExtents, Translation, Boxes);
			Translation += Forward * PlayerViewFrontPlacement::GetCameraClearancePushCm(
				Boxes,
				Request.CameraOrigin,
				Forward,
				Tuning.CameraClearanceCm);
			return Translation;
		};
		if (TryPullDistances(
			Context,
			Distances,
			Tuning.ForwardLayoutAttempts,
			BuildFloorTranslation,
			PlayerCenter,
			OutTransforms,
			OutFailureReason))
		{
			return Succeed(EShopUnboxPlacementStage::FloorFront);
		}
		if (!OutFailureReason.IsEmpty())
		{
			return false;
		}
	}

	// Stage 3: above the player's head.
	for (int32 HeightIndex = 0; HeightIndex < Tuning.OverheadStepCount; ++HeightIndex)
	{
		const float LowestZ = CapsuleTop.Z + Tuning.OverheadClearanceCm + Tuning.OverheadStepCm * HeightIndex;
		const FVector TargetXYCenter(PlayerCenter.X, PlayerCenter.Y, 0.0f);
		for (int32 Attempt = 0; Attempt < Tuning.OverheadLayoutAttempts; ++Attempt)
		{
			if (TryMakeLayout(
				Context,
				[&](const TArray<FShopUnboxClusterItem>& Layout)
				{
					return GetClusterTranslation(Layout, HalfExtents, TargetXYCenter, LowestZ);
				},
				CapsuleTop,
				OutTransforms,
				OutFailureReason))
			{
				return Succeed(EShopUnboxPlacementStage::Overhead);
			}
			if (!OutFailureReason.IsEmpty())
			{
				return false;
			}
		}
	}

	// Stage 4: final stack, always succeeds.
	OutTransforms.Reset();
	float BaseZ = Request.FootLocation.Z;
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		const FVector& Extent = HalfExtents[Index];
		const FVector ShapeCenter(PlayerCenter.X, PlayerCenter.Y, BaseZ + Extent.Z);
		FUnboxCandidate Candidate;
		if (!BuildCandidateAtShapeCenter(
			Items[Index],
			ShapeCenter,
			ProbeYaw,
			Candidate,
			OutFailureReason))
		{
			OutTransforms.Reset();
			return false;
		}
		FCollisionQueryParams FallbackParams(SCENE_QUERY_STAT(ShopUnboxFinalFallback), false);
		FallbackParams.AddIgnoredActor(&Player);
		FallbackParams.AddIgnoredActor(&Box);
		if (FacilityPlacementCollision::HasBlockingOverlap(
			World,
			Candidate.Location,
			Candidate.Rotation,
			Candidate.Shape,
			*Candidate.CollisionTemplate,
			FallbackParams))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Shop unboxing final fallback overlaps a world blocker for %s; continuing because all safe candidates were blocked."),
				*Items[Index].DebugName);
		}
		OutTransforms.Add(Candidate.Transform);
		BaseZ = ShapeCenter.Z + Extent.Z;
	}
	return !OutTransforms.IsEmpty() && Succeed(EShopUnboxPlacementStage::FinalStack);
}

#undef LOCTEXT_NAMESPACE
