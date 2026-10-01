#include "Cleaning/CleaningSpawnRules.h"
#include "Cleaning/LitterActor.h"
#include "Cleaning/WaterStainActor.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Interaction/PhysicalCarryable.h"
#include "Placement/PlaceableFacility.h"

float FCleaningSpawnClock::SampleNext(FRandomStream& Stream)
{
	return -FMath::Loge(FMath::Max(1.0f - Stream.FRand(), SMALL_NUMBER));
}

bool FCleaningSpawnClock::Advance(float& Remaining, int32 CustomerCount, float DeltaSeconds, float MeanIntervalSeconds)
{
	if (CustomerCount <= 0 || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0 ||
		!FMath::IsFinite(MeanIntervalSeconds) || MeanIntervalSeconds <= 0)
	{
		return false;
	}
	Remaining -= CustomerCount * DeltaSeconds / MeanIntervalSeconds;
	return Remaining <= 0;
}

int32 CountCustomersInBox(const TArray<FVector>& Locations, const FTransform& BoxTransform, const FVector& Extent)
{
	int32 Count = 0;
	for (const FVector& Location : Locations)
	{
		const FVector Local = BoxTransform.InverseTransformPosition(Location);
		if (FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z)
		{
			++Count;
		}
	}
	return Count;
}

bool FCleaningFloorSpawnQuery::Find(UWorld& World, const AActor* Zone, const FCleaningFloorSpawnSettings& S,
									FRandomStream& Stream, TFunctionRef<bool(const FVector&, float)> IsSpacingClear,
									FTransform& OutTransform)
{
	if (S.Extent.GetMin() <= 0 || !FMath::IsFinite(S.Radius) || S.Radius <= 0 || !FMath::IsFinite(S.ClearanceHeight) ||
		S.ClearanceHeight <= 0 ||
		!FMath::IsFinite(S.ClearanceFloorOffset) || S.ClearanceFloorOffset < 0)
	{
		return false;
	}
	const FVector Local(Stream.FRandRange(-S.Extent.X, S.Extent.X), Stream.FRandRange(-S.Extent.Y, S.Extent.Y),
						S.Extent.Z);
	const FVector Start = S.BoxTransform.TransformPosition(Local);
	const FVector End =
		Start - FVector::UpVector * (S.Extent.Z * 2 * FMath::Abs(S.BoxTransform.GetScale3D().Z) + S.TraceDistance);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CleaningFloorSpawn), true, Zone);
	FHitResult Hit;
	// These presentation actors may overlap each other. Trace through them to the authoritative floor.
	for (;;)
	{
		if (!World.LineTraceSingleByChannel(Hit, Start, End, S.TraceChannel, Params) || !Hit.Component.IsValid())
		{
			return false;
		}
		AActor* Actor = Hit.GetActor();
		if (Actor && (Actor->IsA<AWaterStainActor>() || Actor->IsA<ALitterActor>()))
		{
			Params.AddIgnoredActor(Actor);
			continue;
		}
		break;
	}
	if (FMath::Abs(Hit.ImpactPoint.Z - S.FloorZ) > FMath::Max(0.0f, S.FloorTolerance) ||
		Cast<IPlaceableFacility>(Hit.GetActor()) || Cast<IPhysicalCarryable>(Hit.GetActor()) ||
		Hit.Component->GetCollisionObjectType() != ECC_WorldStatic ||
		Hit.Component->Mobility != EComponentMobility::Static)
	{
		return false;
	}
	const FVector HitLocal = S.BoxTransform.InverseTransformPosition(Hit.ImpactPoint);
	if (FMath::Abs(HitLocal.X) > S.Extent.X || FMath::Abs(HitLocal.Y) > S.Extent.Y ||
		(!S.RequiredFloorTag.IsNone() && !Hit.Component->ComponentHasTag(S.RequiredFloorTag)) ||
		FVector::DotProduct(Hit.ImpactNormal.GetSafeNormal(), FVector::UpVector) <
			FMath::Cos(FMath::DegreesToRadians(S.MaximumSlopeDegrees)))
	{
		return false;
	}
	FCollisionResponseParams Responses(ECR_Ignore);
	Responses.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);
	Responses.CollisionResponse.SetResponse(ECC_WorldDynamic, ECR_Block);
	Responses.CollisionResponse.SetResponse(ECC_PhysicsBody, ECR_Block);
	Params.AddIgnoredComponent(Hit.GetComponent());
	TArray<FOverlapResult> Overlaps;
	World.OverlapMultiByChannel(
		Overlaps, Hit.ImpactPoint + FVector(0, 0, S.ClearanceFloorOffset + S.ClearanceHeight / 2), FQuat::Identity, ECC_PhysicsBody,
		FCollisionShape::MakeBox(FVector(S.Radius, S.Radius, S.ClearanceHeight / 2)), Params, Responses);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const AActor* Actor = Overlap.GetActor();
		if (Overlap.bBlockingHit && IsValid(Overlap.GetComponent()) &&
			!(Actor && (Actor->IsA<AWaterStainActor>() || Actor->IsA<ALitterActor>() || Actor->IsA<APawn>())))
		{
			return false;
		}
	}
	if (!IsSpacingClear(Hit.ImpactPoint, S.Spacing))
	{
		return false;
	}
	OutTransform = FTransform(FRotationMatrix::MakeFromZ(Hit.ImpactNormal).ToQuat(), Hit.ImpactPoint);
	return true;
}

bool FCleaningFootprintOverlap::Intersects(const FVector& Center, float Radius, const FTransform& Transform,
										   const FVector& Extent, float HeightToleranceCm)
{
	const float Tolerance = FMath::IsFinite(HeightToleranceCm) ? FMath::Max(0.0f, HeightToleranceCm) : 0.0f;
	const FVector Scaled = Extent * Transform.GetScale3D().GetAbs();
	const FVector Local = Transform.GetRotation().UnrotateVector(Center - Transform.GetLocation());
	if (Local.Z < -Scaled.Z - Tolerance || Local.Z > Scaled.Z + Tolerance)
	{
		return false;
	}
	const float DX = FMath::Max(FMath::Abs(Local.X) - Scaled.X, 0.0);
	const float DY = FMath::Max(FMath::Abs(Local.Y) - Scaled.Y, 0.0);
	return DX * DX + DY * DY <= FMath::Square(FMath::Max(0.0f, Radius));
}
