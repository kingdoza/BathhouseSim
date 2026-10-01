#include "Cleaning/TrashBagDropPlacement.h"
#include "Cleaning/TrashBagActor.h"
#include "Cleaning/LitterActor.h"
#include "Cleaning/WaterStainActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Interaction/PlayerViewFrontPlacement.h"
#include "Placement/FacilityPlacementCollisionUtils.h"

namespace
{
bool IsStageValid(const float Distance, const float Minimum, const float Step)
{
	return FMath::IsFinite(Distance) && FMath::IsFinite(Minimum) && FMath::IsFinite(Step) && Step > 0.0f &&
		   Minimum > 0.0f && Distance >= Minimum;
}
} // namespace

bool FTrashBagDropPlacement::Find(UWorld& World, APawn* Pawn, const TArray<AActor*>& Ignored,
								  TSubclassOf<ATrashBagActor> BagClass, const FTrashBagDropRequest& Request,
								  FTransform& Out, ETrashBagDropStage* OutStage)
{
	if (OutStage)
	{
		*OutStage = ETrashBagDropStage::None;
	}
	auto* Capsule = Pawn ? Pawn->FindComponentByClass<UCapsuleComponent>() : nullptr;
	if (!Capsule || !BagClass || Request.CameraOrigin.ContainsNaN() || Request.CameraDirection.ContainsNaN() ||
		Request.CameraDirection.IsNearlyZero() || !FMath::IsFinite(Request.CameraClearanceCm) ||
		!FMath::IsFinite(Request.FloorClearanceCm))
	{
		return false;
	}
	const FVector ViewDirection = Request.CameraDirection.GetSafeNormal();
	const FVector Direction = FVector(ViewDirection.X, ViewDirection.Y, 0).GetSafeNormal();
	const FVector Feet = Capsule->GetComponentLocation() - FVector(0, 0, Capsule->GetScaledCapsuleHalfHeight());
	const FQuat Yaw = FRotator(0, Pawn->GetActorRotation().Yaw, 0).Quaternion();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TrashBagDrop), false);
	Params.AddIgnoredActors(Ignored);
	FVector Location;
	FQuat Rotation;
	FCollisionShape Shape;
	const UPrimitiveComponent* Template = nullptr;
	FText Failure;
	if (!ATrashBagActor::BuildClassCollisionQuery(BagClass, FTransform(Yaw, Feet), Location, Rotation, Shape, Template,
												  Failure))
	{
		return false;
	}
	const FVector RootToBoundsCenter = Location - Feet;
	const FVector HalfExtent = Shape.GetExtent().GetAbs();
	const float BoxYawDegrees = Rotation.Rotator().Yaw;

	// Checks one candidate whose collision-shape center is CandidateCenter. Litter and stains never block.
	const auto TryCandidate = [&](const FVector& CandidateCenter, const FVector& VisibilityOrigin) -> bool
	{
		FTransform Candidate(Yaw, CandidateCenter - RootToBoundsCenter);
		FVector CandidateLocation;
		FQuat CandidateRotation;
		FCollisionShape CandidateShape;
		const UPrimitiveComponent* CandidateTemplate = nullptr;
		FText CandidateFailure;
		if (!ATrashBagActor::BuildClassCollisionQuery(BagClass, Candidate, CandidateLocation, CandidateRotation,
													  CandidateShape, CandidateTemplate, CandidateFailure))
		{
			return false;
		}
		FCollisionQueryParams CandidateParams = Params;
		TArray<FOverlapResult> Overlaps;
		World.OverlapMultiByObjectType(Overlaps, CandidateLocation, CandidateRotation,
									   FCollisionObjectQueryParams::AllObjects, CandidateShape, CandidateParams);
		for (const auto& Hit : Overlaps)
		{
			auto* Actor = Hit.GetActor();
			if (Actor && (Actor->IsA<AWaterStainActor>() || Actor->IsA<ALitterActor>()))
			{
				CandidateParams.AddIgnoredActor(Actor);
			}
		}
		FHitResult PathHit;
		if (World.LineTraceSingleByChannel(PathHit, VisibilityOrigin, CandidateLocation, ECC_Visibility,
										   CandidateParams) ||
			FacilityPlacementCollision::HasBlockingOverlap(World, CandidateLocation, CandidateRotation, CandidateShape,
														   *CandidateTemplate, CandidateParams))
		{
			return false;
		}
		Out = Candidate;
		Out.SetScale3D(CandidateTemplate->GetRelativeScale3D());
		return true;
	};

	TArray<float> Distances;
	// Stage 1: in front of the camera along the view direction.
	if (IsStageValid(Request.ViewDistanceCm, Request.ViewMinDistanceCm, Request.ViewPullStepCm))
	{
		PlayerViewFrontPlacement::BuildPullDistances(Request.ViewDistanceCm, Request.ViewMinDistanceCm,
													 Request.ViewPullStepCm, Distances);
		for (const float Distance : Distances)
		{
			const FViewFrontBox Box{Location, BoxYawDegrees, HalfExtent};
			const FVector Translation = PlayerViewFrontPlacement::ComputeViewFrontTranslation(
				MakeArrayView(&Box, 1), Request.CameraOrigin, ViewDirection, Distance);
			if (TryCandidate(Location + Translation, Request.CameraOrigin))
			{
				if (OutStage)
				{
					*OutStage = ETrashBagDropStage::ViewFront;
				}
				return true;
			}
		}
	}

	// Stage 2: on the floor in front of the feet, pushed off the camera when it would wrap it.
	if (!Direction.IsNearlyZero() &&
		IsStageValid(Request.FloorForwardDistanceCm, Request.FloorMinForwardDistanceCm, Request.FloorPullStepCm))
	{
		PlayerViewFrontPlacement::BuildPullDistances(Request.FloorForwardDistanceCm, Request.FloorMinForwardDistanceCm,
													 Request.FloorPullStepCm, Distances);
		for (const float Distance : Distances)
		{
			FVector CandidateCenter =
				Feet + Direction * Distance + FVector(0, 0, HalfExtent.Z + Request.FloorClearanceCm);
			const FViewFrontBox Box{CandidateCenter, BoxYawDegrees, HalfExtent};
			CandidateCenter += Direction * PlayerViewFrontPlacement::GetCameraClearancePushCm(
									MakeArrayView(&Box, 1), Request.CameraOrigin, Direction, Request.CameraClearanceCm);
			if (TryCandidate(CandidateCenter, Capsule->GetComponentLocation()))
			{
				if (OutStage)
				{
					*OutStage = ETrashBagDropStage::FloorFront;
				}
				return true;
			}
		}
	}
	return false;
}
