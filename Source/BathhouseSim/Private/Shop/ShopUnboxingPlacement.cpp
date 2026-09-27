#include "Shop/ShopUnboxingPlacement.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Math/RotationMatrix.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Placement/FacilityPlacementCollisionUtils.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Shop/ShopDeliveryBoxActor.h"

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

bool BuildCandidate(
	const UFacilityPlacementDefinition& Definition,
	const FVector& Location,
	const float Yaw,
	FUnboxCandidate& OutCandidate,
	FText& OutFailureReason)
{
	const APlaceableFacilityItemActor* ItemCDO = Definition.RecoveryItemClass
		? Definition.RecoveryItemClass->GetDefaultObject<APlaceableFacilityItemActor>()
		: nullptr;
	if (!ItemCDO)
	{
		OutFailureReason = LOCTEXT("MissingRecoveryItemClass", "주문 상품의 설비 아이템 클래스를 찾을 수 없습니다.");
		return false;
	}
	OutCandidate.Transform = FTransform(FRotator(0.0f, Yaw, 0.0f), Location, ItemCDO->GetActorScale3D());
	return APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
		Definition,
		OutCandidate.Transform,
		OutCandidate.Location,
		OutCandidate.Rotation,
		OutCandidate.Shape,
		OutCandidate.CollisionTemplate,
		OutFailureReason);
}

bool CandidatesSeparate(
	const FUnboxCandidate& A,
	const FUnboxCandidate& B,
	const FVector& Forward,
	const FVector& Right)
{
	const FVector ExtentA = A.Shape.GetExtent().GetAbs();
	const FVector ExtentB = B.Shape.GetExtent().GetAbs();
	const FVector Delta = B.Location - A.Location;
	return FMath::Abs(Delta.Dot(Forward)) >= ExtentA.X + ExtentB.X
		|| FMath::Abs(Delta.Dot(Right)) >= ExtentA.Y + ExtentB.Y
		|| FMath::Abs(Delta.Z) >= ExtentA.Z + ExtentB.Z;
}

bool HasLineOfSight(
	UWorld& World,
	const FVector& Start,
	const FVector& End,
	AActor& Player,
	AShopDeliveryBoxActor& Box)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ShopUnboxLineOfSight), false);
	Params.AddIgnoredActor(&Player);
	Params.AddIgnoredActor(&Box);
	FHitResult Hit;
	return !World.LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
}

bool IsWorldSpaceClear(
	UWorld& World,
	const FUnboxCandidate& Candidate,
	AActor& Player,
	AShopDeliveryBoxActor& Box)
{
	if (!Candidate.CollisionTemplate)
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ShopUnboxCandidateOverlap), false);
	Params.AddIgnoredActor(&Player);
	Params.AddIgnoredActor(&Box);
	return !FacilityPlacementCollision::HasBlockingOverlap(
		World,
		Candidate.Location,
		Candidate.Rotation,
		Candidate.Shape,
		*Candidate.CollisionTemplate,
		Params);
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
	TArray<FTransform>& OutTransforms,
	FText& OutFailureReason)
{
	OutTransforms.Reset();
	if (Definitions.IsEmpty() || FootLocation.ContainsNaN() || !FMath::IsFinite(ViewYaw)
		|| !FMath::IsFinite(ForwardDistanceCm) || ForwardDistanceCm < 0.0f)
	{
		OutFailureReason = LOCTEXT("InvalidUnboxRequest", "상자 개봉 위치를 계산할 수 없습니다.");
		return false;
	}

	const FRotator ViewRotation(0.0f, ViewYaw, 0.0f);
	const FVector Forward = ViewRotation.Vector().GetSafeNormal();
	const FVector Right = FRotationMatrix(ViewRotation).GetScaledAxis(EAxis::Y).GetSafeNormal();
	const FVector PlayerCenter = PlayerCapsule.GetComponentLocation();
	const FVector CapsuleTop(PlayerCenter.X, PlayerCenter.Y,
		PlayerCenter.Z + PlayerCapsule.GetScaledCapsuleHalfHeight());

	auto TryFrontRow = [&](const float Distance, TArray<FTransform>& Out) -> bool
	{
		TArray<FUnboxCandidate> Candidates;
		Candidates.Reserve(Definitions.Num());
		float TotalWidth = 10.0f * FMath::Max(0, Definitions.Num() - 1);
		TArray<FVector> HalfExtents;
		HalfExtents.Reserve(Definitions.Num());
		for (UFacilityPlacementDefinition* Definition : Definitions)
		{
			if (!IsValid(Definition))
			{
				return false;
			}
			FUnboxCandidate Probe;
			if (!BuildCandidate(*Definition, FVector::ZeroVector, ViewYaw, Probe, OutFailureReason))
			{
				return false;
			}
			const FVector Extent = Probe.Shape.GetExtent().GetAbs();
			HalfExtents.Add(Extent);
			TotalWidth += Extent.Y * 2.0f;
		}
		float Cursor = -TotalWidth * 0.5f;
		for (int32 Index = 0; Index < Definitions.Num(); ++Index)
		{
			const FVector Extent = HalfExtents[Index];
			Cursor += Extent.Y;
			const FVector Location = FootLocation + Forward * Distance + Right * Cursor
				+ FVector::UpVector * (Extent.Z + 20.0f);
			FUnboxCandidate Candidate;
			if (!BuildCandidate(*Definitions[Index], Location, ViewYaw, Candidate, OutFailureReason)
				|| !IsWorldSpaceClear(World, Candidate, Player, Box)
				|| !HasLineOfSight(World, PlayerCenter, Candidate.Location, Player, Box))
			{
				return false;
			}
			for (const FUnboxCandidate& Previous : Candidates)
			{
				if (CandidatesSeparate(Candidate, Previous, Forward, Right) == false)
				{
					return false;
				}
			}
			Candidates.Add(Candidate);
			Cursor += Extent.Y + 10.0f;
		}
		Out.Reset();
		for (const FUnboxCandidate& Candidate : Candidates)
		{
			Out.Add(Candidate.Transform);
		}
		return true;
	};

	for (float Distance = ForwardDistanceCm;; Distance = FMath::Max(0.0f, Distance - 10.0f))
	{
		if (TryFrontRow(Distance, OutTransforms))
		{
			return true;
		}
		if (Distance <= 0.0f)
		{
			break;
		}
	}

	auto TryVerticalStack = [&](const bool bAboveCapsule, TArray<FTransform>& Out) -> bool
	{
		TArray<FUnboxCandidate> Candidates;
		float BaseZ = bAboveCapsule
			? CapsuleTop.Z + 10.0f
			: FootLocation.Z;
		for (UFacilityPlacementDefinition* Definition : Definitions)
		{
			FUnboxCandidate Probe;
			if (!IsValid(Definition)
				|| !BuildCandidate(*Definition, FVector::ZeroVector, ViewYaw, Probe, OutFailureReason))
			{
				return false;
			}
			const FVector Extent = Probe.Shape.GetExtent().GetAbs();
			bool bPlaced = false;
			const int32 MaxVerticalAttempts = bAboveCapsule ? 32 : 1;
			for (int32 Attempt = 0; Attempt < MaxVerticalAttempts; ++Attempt)
			{
				const float CandidateBaseZ = BaseZ + Attempt * 25.0f;
				const float CenterZ = CandidateBaseZ + Extent.Z;
				const FVector Location(PlayerCenter.X, PlayerCenter.Y, CenterZ);
				FUnboxCandidate Candidate;
				if (!BuildCandidate(*Definition, Location, ViewYaw, Candidate, OutFailureReason))
				{
					return false;
				}
				if (bAboveCapsule)
				{
					if (!HasLineOfSight(World, CapsuleTop, Candidate.Location, Player, Box)
						|| !IsWorldSpaceClear(World, Candidate, Player, Box))
					{
						continue;
					}
				}
				else if (!IsWorldSpaceClear(World, Candidate, Player, Box))
				{
					UE_LOG(LogTemp, Warning,
						TEXT("Shop unboxing final fallback overlaps a world blocker for %s; continuing because all safe candidates were blocked."),
						*Definition->GetPathName());
				}
				Candidates.Add(Candidate);
				BaseZ = CenterZ + Extent.Z + (bAboveCapsule ? 10.0f : 0.0f);
				bPlaced = true;
				break;
			}
			if (!bPlaced)
			{
				return false;
			}
		}
		Out.Reset();
		for (const FUnboxCandidate& Candidate : Candidates)
		{
			Out.Add(Candidate.Transform);
		}
		return true;
	};
	if (TryVerticalStack(true, OutTransforms) || TryVerticalStack(false, OutTransforms))
	{
		return true;
	}

	OutFailureReason = LOCTEXT("UnboxNoCandidate", "설비 아이템을 생성할 공간을 찾을 수 없습니다.");
	return false;
}

#undef LOCTEXT_NAMESPACE
