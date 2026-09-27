#include "Shop/ShopDeliveryPointActor.h"

#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Shop/ShopOrderSubsystem.h"

AShopDeliveryPointActor::AShopDeliveryPointActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
#if WITH_EDITORONLY_DATA
	EditorBillboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorBillboard"));
	if (EditorBillboard)
	{
		EditorBillboard->SetupAttachment(SceneRoot);
		EditorBillboard->bIsScreenSizeScaled = true;
	}
	EditorArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("EditorArrow"));
	if (EditorArrow)
	{
		EditorArrow->SetupAttachment(SceneRoot);
	}
#endif
}

void AShopDeliveryPointActor::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		if (UShopOrderSubsystem* Orders = World->GetSubsystem<UShopOrderSubsystem>())
		{
			Orders->RegisterDeliveryPoint(this);
		}
	}
}

void AShopDeliveryPointActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UShopOrderSubsystem* Orders = World->GetSubsystem<UShopOrderSubsystem>())
		{
			Orders->UnregisterDeliveryPoint(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

bool AShopDeliveryPointActor::FindDropTransform(
	const FVector& BoxHalfExtent,
	const UPrimitiveComponent& BoxCollisionTemplate,
	FTransform& OutTransform) const
{
	UWorld* World = GetWorld();
	if (!World || BoxHalfExtent.ContainsNaN() || BoxHalfExtent.X <= 0.0f
		|| BoxHalfExtent.Y <= 0.0f || BoxHalfExtent.Z <= 0.0f
		|| !FMath::IsFinite(MaxSearchHeightCm) || MaxSearchHeightCm <= 0.0f
		|| !FMath::IsFinite(DropGapCm) || DropGapCm < 0.0f)
	{
		return false;
	}

	const FVector Base = GetActorLocation();
	const FVector TraceStart(Base.X, Base.Y, Base.Z + MaxSearchHeightCm);
	const FVector TraceEnd(Base.X, Base.Y, Base.Z);
	FCollisionObjectQueryParams CeilingTypes;
	CeilingTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(ShopDeliveryCeiling), false, this);
	FHitResult CeilingHit;
	const bool bHasCeiling = World->LineTraceSingleByObjectType(
		CeilingHit,
		TraceStart,
		TraceEnd,
		CeilingTypes,
		TraceParams);
	const float CeilingZ = bHasCeiling ? CeilingHit.ImpactPoint.Z : TraceStart.Z;

	const FCollisionShape BoxShape = FCollisionShape::MakeBox(BoxHalfExtent);
	const FVector SweepStart(Base.X, Base.Y, CeilingZ - BoxHalfExtent.Z - 1.0f);
	const FVector SweepEnd(Base.X, Base.Y, Base.Z + BoxHalfExtent.Z);
	FCollisionQueryParams SweepParams(SCENE_QUERY_STAT(ShopDeliveryStack), false, this);
	FHitResult StackHit;
	const bool bStartBlocked = World->OverlapBlockingTestByChannel(
		SweepStart,
		GetActorQuat(),
		BoxCollisionTemplate.GetCollisionObjectType(),
		BoxShape,
		SweepParams,
		FCollisionResponseParams(BoxCollisionTemplate.GetCollisionResponseToChannels()));
	if (bStartBlocked)
	{
		return false;
	}
	const bool bHitStack = World->SweepSingleByChannel(
		StackHit,
		SweepStart,
		SweepEnd,
		GetActorQuat(),
		BoxCollisionTemplate.GetCollisionObjectType(),
		BoxShape,
		SweepParams,
		FCollisionResponseParams(BoxCollisionTemplate.GetCollisionResponseToChannels()));
	const float BaseCenterZ = bHitStack ? StackHit.Location.Z : Base.Z + BoxHalfExtent.Z;
	const float SpawnCenterZ = BaseCenterZ + DropGapCm;
	if (SpawnCenterZ + BoxHalfExtent.Z > CeilingZ)
	{
		return false;
	}
	const FVector SpawnLocation(Base.X, Base.Y, SpawnCenterZ);
	if (World->OverlapBlockingTestByChannel(
		SpawnLocation,
		GetActorQuat(),
		BoxCollisionTemplate.GetCollisionObjectType(),
		BoxShape,
		SweepParams,
		FCollisionResponseParams(BoxCollisionTemplate.GetCollisionResponseToChannels())))
	{
		return false;
	}
	OutTransform = FTransform(GetActorQuat(), SpawnLocation, FVector::OneVector);
	return true;
}
