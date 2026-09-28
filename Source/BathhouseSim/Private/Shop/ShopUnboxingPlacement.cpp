#include "Shop/ShopUnboxingPlacement.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Math/RotationMatrix.h"
#include "Placement/FacilityPlacementCollisionUtils.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Shop/ShopUnboxingCluster.h"

#define LOCTEXT_NAMESPACE "ShopUnboxingPlacement"

namespace
{
constexpr int32 FrontLayoutAttempts = 4;
constexpr int32 UpperLayoutAttempts = 3;
constexpr int32 UpperHeightSteps = 8;
constexpr float UpperHeightStepCm = 25.0f;
constexpr float PlayerTopClearanceCm = 10.0f;
constexpr float FloorClearanceCm = 20.0f;

struct FUnboxCandidate
{
	FTransform Transform = FTransform::Identity;
	FVector Location = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
	FCollisionShape Shape;
	const UPrimitiveComponent* CollisionTemplate = nullptr;
};

bool BuildCandidateAtShapeCenter(
	const UFacilityPlacementDefinition& Definition,
	const FVector& ShapeCenter,
	const float Yaw,
	FUnboxCandidate& OutCandidate,
	FText& OutFailureReason)
{
	const FRotator CandidateRotation(0.0f, Yaw, 0.0f);
	FVector ItemScale = FVector::OneVector;
	if (!APlaceableFacilityItemActor::GetDefinitionItemScale(Definition, ItemScale, OutFailureReason))
	{
		return false;
	}
	const FTransform ProbeTransform(CandidateRotation, FVector::ZeroVector, ItemScale);
	FVector RelativeShapeCenter = FVector::ZeroVector;
	FQuat ProbeRotation = FQuat::Identity;
	if (!APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
		Definition,
		ProbeTransform,
		RelativeShapeCenter,
		ProbeRotation,
		OutCandidate.Shape,
		OutCandidate.CollisionTemplate,
		OutFailureReason))
	{
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

bool TryMakeLayout(
	UWorld& World,
	AActor& Player,
	AShopDeliveryBoxActor& Box,
	const TArray<UFacilityPlacementDefinition*>& Definitions,
	const TArray<FVector>& HalfExtents,
	const FVector& TargetXYCenter,
	const float TargetLowestZ,
	const FVector& VisibilityOrigin,
	const float OverlapDepthCm,
	FRandomStream& RandomStream,
	const bool bUseEnvironmentClearance,
	TArray<FTransform>& OutTransforms,
	FText& OutFailureReason)
{
	TArray<FShopUnboxClusterItem> Layout;
	if (!FShopUnboxingCluster::BuildLayout(HalfExtents, OverlapDepthCm, RandomStream, Layout))
	{
		return false;
	}

	const FVector Translation = GetClusterTranslation(
		Layout,
		HalfExtents,
		TargetXYCenter,
		TargetLowestZ);
	TArray<FTransform> CandidateTransforms;
	CandidateTransforms.Reserve(Definitions.Num());
	const float EnvironmentClearanceCm = bUseEnvironmentClearance
		? FMath::Clamp(OverlapDepthCm, 0.0f, 50.0f)
		: 0.0f;

	for (int32 Index = 0; Index < Definitions.Num(); ++Index)
	{
		FUnboxCandidate Candidate;
		if (!BuildCandidateAtShapeCenter(
			*Definitions[Index],
			Layout[Index].Center + Translation,
			Layout[Index].YawDegrees,
			Candidate,
			OutFailureReason))
		{
			return false;
		}
		if (bUseEnvironmentClearance
			&& !IsCandidateSafe(
				World,
				Candidate,
				Player,
				Box,
				VisibilityOrigin,
				EnvironmentClearanceCm))
		{
			return false;
		}
		CandidateTransforms.Add(Candidate.Transform);
	}

	OutTransforms = MoveTemp(CandidateTransforms);
	return true;
}
}

bool FShopUnboxingPlacement::FindSpawnTransforms(
	UWorld& World,
	AActor& Player,
	const UCapsuleComponent& PlayerCapsule,
	AShopDeliveryBoxActor& Box,
	const FVector& FootLocation,
	const float ViewYaw,
	const TArray<UFacilityPlacementDefinition*>& Definitions,
	const float ForwardDistanceCm,
	FRandomStream& RandomStream,
	const float OverlapDepthCm,
	TArray<FTransform>& OutTransforms,
	FText& OutFailureReason)
{
	OutTransforms.Reset();
	OutFailureReason = FText::GetEmpty();
	if (Definitions.IsEmpty() || FootLocation.ContainsNaN() || !FMath::IsFinite(ViewYaw)
		|| !FMath::IsFinite(ForwardDistanceCm) || ForwardDistanceCm < 0.0f
		|| !FMath::IsFinite(OverlapDepthCm) || OverlapDepthCm < 0.0f)
	{
		OutFailureReason = LOCTEXT("InvalidUnboxRequest", "상자 개봉 위치를 계산할 수 없습니다.");
		return false;
	}

	const FRotator ViewRotation(0.0f, ViewYaw, 0.0f);
	const FVector Forward = FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::X);
	const FVector PlayerCenter = PlayerCapsule.GetComponentLocation();
	const float CapsuleHalfHeight = PlayerCapsule.GetScaledCapsuleHalfHeight();
	if (PlayerCenter.ContainsNaN() || !FMath::IsFinite(CapsuleHalfHeight) || CapsuleHalfHeight <= 0.0f)
	{
		OutFailureReason = LOCTEXT("InvalidUnboxPlayer", "플레이어 위치를 확인할 수 없습니다.");
		return false;
	}
	const FVector CapsuleTop(PlayerCenter.X, PlayerCenter.Y, PlayerCenter.Z + CapsuleHalfHeight);

	TArray<FVector> HalfExtents;
	HalfExtents.Reserve(Definitions.Num());
	for (const UFacilityPlacementDefinition* Definition : Definitions)
	{
		if (!IsValid(Definition))
		{
			OutFailureReason = LOCTEXT("MissingPlacementDefinition", "주문 상품 배치 정의를 찾을 수 없습니다.");
			return false;
		}
		FUnboxCandidate Probe;
		if (!BuildCandidateAtShapeCenter(
			*Definition,
			FVector::ZeroVector,
			ViewYaw,
			Probe,
			OutFailureReason))
		{
			return false;
		}
		HalfExtents.Add(GetCandidateHalfExtent(Probe));
	}

	for (float DistanceCm = ForwardDistanceCm;; DistanceCm = FMath::Max(0.0f, DistanceCm - 10.0f))
	{
		const FVector TargetXYCenter = FootLocation + Forward * DistanceCm;
		for (int32 Attempt = 0; Attempt < FrontLayoutAttempts; ++Attempt)
		{
			if (TryMakeLayout(
				World,
				Player,
				Box,
				Definitions,
				HalfExtents,
				TargetXYCenter,
				FootLocation.Z + FloorClearanceCm,
				PlayerCenter,
				OverlapDepthCm,
				RandomStream,
				true,
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
		if (DistanceCm <= 0.0f)
		{
			break;
		}
	}

	for (int32 HeightIndex = 0; HeightIndex < UpperHeightSteps; ++HeightIndex)
	{
		const float LowestZ = CapsuleTop.Z + PlayerTopClearanceCm + UpperHeightStepCm * HeightIndex;
		const FVector TargetXYCenter(PlayerCenter.X, PlayerCenter.Y, 0.0f);
		for (int32 Attempt = 0; Attempt < UpperLayoutAttempts; ++Attempt)
		{
			if (TryMakeLayout(
				World,
				Player,
				Box,
				Definitions,
				HalfExtents,
				TargetXYCenter,
				LowestZ,
				CapsuleTop,
				OverlapDepthCm,
				RandomStream,
				true,
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

	OutTransforms.Reset();
	float BaseZ = FootLocation.Z;
	for (int32 Index = 0; Index < Definitions.Num(); ++Index)
	{
		const FVector& Extent = HalfExtents[Index];
		const FVector ShapeCenter(PlayerCenter.X, PlayerCenter.Y, BaseZ + Extent.Z);
		FUnboxCandidate Candidate;
		if (!BuildCandidateAtShapeCenter(
			*Definitions[Index],
			ShapeCenter,
			ViewYaw,
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
				*Definitions[Index]->GetPathName());
		}
		OutTransforms.Add(Candidate.Transform);
		BaseZ = ShapeCenter.Z + Extent.Z;
	}
	return !OutTransforms.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
