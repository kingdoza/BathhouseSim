#include "Computer/ComputerFocusExitPlacement.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
constexpr float RingSpacingCm = 10.0f;

FCollisionShape MakeCapsuleShape(const UCapsuleComponent* PlayerCapsule)
{
	return FCollisionShape::MakeCapsule(
		PlayerCapsule->GetScaledCapsuleRadius(),
		PlayerCapsule->GetScaledCapsuleHalfHeight());
}

bool IsFree(
	UWorld* World,
	UCapsuleComponent* PlayerCapsule,
	AActor* PlayerActor,
	const FVector& CapsuleCenter)
{
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ComputerFocusExitPlacement), false, PlayerActor);
	const FCollisionResponseParams ResponseParams(PlayerCapsule->GetCollisionResponseToChannels());
	return !World->OverlapBlockingTestByChannel(
		CapsuleCenter,
		FQuat::Identity,
		PlayerCapsule->GetCollisionObjectType(),
		MakeCapsuleShape(PlayerCapsule),
		QueryParams,
		ResponseParams);
}

void CollectInitialBlockingComponents(
	UWorld* World,
	UCapsuleComponent* PlayerCapsule,
	AActor* PlayerActor,
	const FVector& CapsuleCenter,
	TArray<UPrimitiveComponent*>& OutComponents)
{
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ComputerFocusExitPlacement), false, PlayerActor);
	const FCollisionResponseParams ResponseParams(PlayerCapsule->GetCollisionResponseToChannels());
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(
		Overlaps,
		CapsuleCenter,
		FQuat::Identity,
		PlayerCapsule->GetCollisionObjectType(),
		MakeCapsuleShape(PlayerCapsule),
		QueryParams,
		ResponseParams);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (Overlap.bBlockingHit)
		{
			if (UPrimitiveComponent* Component = Overlap.GetComponent())
			{
				OutComponents.AddUnique(Component);
			}
		}
	}
}

bool HasClearSweep(
	UWorld* World,
	UCapsuleComponent* PlayerCapsule,
	AActor* PlayerActor,
	const FVector& Start,
	const FVector& End,
	const TArray<UPrimitiveComponent*>& IgnoredInitialOverlaps)
{
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ComputerFocusExitPlacement), false, PlayerActor);
	QueryParams.AddIgnoredComponents(IgnoredInitialOverlaps);
	const FCollisionResponseParams ResponseParams(PlayerCapsule->GetCollisionResponseToChannels());
	TArray<FHitResult> Hits;
	return !World->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		PlayerCapsule->GetCollisionObjectType(),
		MakeCapsuleShape(PlayerCapsule),
		QueryParams,
		ResponseParams);
}
}

bool FComputerFocusExitPlacement::Resolve(
	UWorld* World,
	UCapsuleComponent* PlayerCapsule,
	AActor* PlayerActor,
	const FVector& FootLocation,
	const float FixedYawDegrees,
	const float SearchRadiusCm,
	FComputerFocusExitPlacementResult& OutResult)
{
	OutResult = FComputerFocusExitPlacementResult();
	if (!IsValid(World) || !World->GetPhysicsScene() || !IsValid(PlayerCapsule) || !IsValid(PlayerActor))
	{
		return false;
	}

	const float CapsuleHalfHeight = PlayerCapsule->GetScaledCapsuleHalfHeight();
	const FVector FixedCenter = FootLocation + FVector::UpVector * CapsuleHalfHeight;
	const float SearchRadius = FMath::IsFinite(SearchRadiusCm) && SearchRadiusCm > 0.0f
		? SearchRadiusCm
		: 0.0f;
	OutResult.CapsuleCenter = FixedCenter;

	if (IsFree(World, PlayerCapsule, PlayerActor, FixedCenter))
	{
		OutResult.Path = FComputerFocusExitPlacementResult::EPath::Fixed;
		return true;
	}

	TArray<UPrimitiveComponent*> InitialBlockingComponents;
	CollectInitialBlockingComponents(World, PlayerCapsule, PlayerActor, FixedCenter, InitialBlockingComponents);
	if (SearchRadius <= 0.0f)
	{
		OutResult.Path = FComputerFocusExitPlacementResult::EPath::Forced;
		return true;
	}

	const float StartYawRadians = FMath::DegreesToRadians(FixedYawDegrees);
	const auto TryRing = [&](const float RingRadius) -> bool
	{
		const int32 AngleCount = FMath::Max(
			8,
			FMath::CeilToInt(2.0f * PI * RingRadius / RingSpacingCm));
		for (int32 AngleIndex = 0; AngleIndex < AngleCount; ++AngleIndex)
		{
			const float Angle = StartYawRadians + 2.0f * PI * static_cast<float>(AngleIndex) / static_cast<float>(AngleCount);
			const FVector Candidate = FixedCenter + FVector(
				FMath::Cos(Angle) * RingRadius,
				FMath::Sin(Angle) * RingRadius,
				0.0f);
			if (IsFree(World, PlayerCapsule, PlayerActor, Candidate)
				&& HasClearSweep(World, PlayerCapsule, PlayerActor, FixedCenter, Candidate, InitialBlockingComponents))
			{
				OutResult.CapsuleCenter = Candidate;
				OutResult.Path = FComputerFocusExitPlacementResult::EPath::Searched;
				OutResult.SearchDistanceCm = RingRadius;
				return true;
			}
		}
		return false;
	};

	for (float RingRadius = RingSpacingCm; RingRadius < SearchRadius; RingRadius += RingSpacingCm)
	{
		if (TryRing(RingRadius))
		{
			return true;
		}
	}

	if (TryRing(SearchRadius))
	{
		return true;
	}

	OutResult.Path = FComputerFocusExitPlacementResult::EPath::Forced;
	return true;
}
