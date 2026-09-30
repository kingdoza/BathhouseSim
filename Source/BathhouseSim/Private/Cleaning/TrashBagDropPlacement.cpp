#include "Cleaning/TrashBagDropPlacement.h"
#include "Cleaning/TrashBagActor.h"
#include "Cleaning/LitterActor.h"
#include "Cleaning/WaterStainActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Placement/FacilityPlacementCollisionUtils.h"

bool FTrashBagDropPlacement::Find(UWorld& World, APawn* Pawn, const TArray<AActor*>& Ignored,
								  TSubclassOf<ATrashBagActor> BagClass, float Forward, float Minimum, FTransform& Out)
{
	auto* Capsule = Pawn ? Pawn->FindComponentByClass<UCapsuleComponent>() : nullptr;
	auto* Camera = Pawn ? Pawn->FindComponentByClass<UCameraComponent>() : nullptr;
	if (!Capsule || !Camera || !BagClass || !FMath::IsFinite(Forward) || !FMath::IsFinite(Minimum) || Minimum <= 0 ||
		Forward < Minimum)
	{
		return false;
	}
	const FVector Direction = FVector(Camera->GetForwardVector().X, Camera->GetForwardVector().Y, 0).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return false;
	}
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
	for (float Distance = Forward; Distance >= Minimum - KINDA_SMALL_NUMBER; Distance -= 10)
	{
		const FVector CandidateCenter = Feet + Direction * Distance + FVector(0, 0, Shape.GetBox().Z + 5);
		FTransform Candidate(Yaw, CandidateCenter - RootToBoundsCenter);
		if (!ATrashBagActor::BuildClassCollisionQuery(BagClass, Candidate, Location, Rotation, Shape, Template,
													  Failure))
		{
			return false;
		}
		FCollisionQueryParams CandidateParams = Params;
		TArray<FOverlapResult> Overlaps;
		World.OverlapMultiByObjectType(Overlaps, Location, Rotation, FCollisionObjectQueryParams::AllObjects, Shape,
									   CandidateParams);
		for (const auto& Hit : Overlaps)
		{
			auto* Actor = Hit.GetActor();
			if (Actor && (Actor->IsA<AWaterStainActor>() || Actor->IsA<ALitterActor>()))
			{
				CandidateParams.AddIgnoredActor(Actor);
			}
		}
		FHitResult PathHit;
		if (!World.LineTraceSingleByChannel(PathHit, Capsule->GetComponentLocation(), Location, ECC_Visibility,
											CandidateParams) &&
			!FacilityPlacementCollision::HasBlockingOverlap(World, Location, Rotation, Shape, *Template,
															CandidateParams))
		{
			Out = Candidate;
			Out.SetScale3D(Template->GetRelativeScale3D());
			return true;
		}
	}
	return false;
}
